// Fuzzing del compilador: alimenta entradas aleatorias y mutadas y comprueba
// que ninguna hace caer el proceso. No comprueba mensajes: solo que el
// compilador sobreviva, que es lo que un fuzz busca.
//
// Uso:  nooki_fuzz [iteraciones] [semilla]
//   Sin argumentos: 2000 iteraciones con semilla fija (lo que corre en ctest).
//   En noche:       nooki_fuzz 200000
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/generador/generador_c.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/semantico/verificaciones.hpp"
#include "nooki/sintactico/parser.hpp"

namespace fs = std::filesystem;

namespace {

/// Generador xorshift: reproducible y suficiente para esto.
struct Azar {
    std::uint64_t estado;

    explicit Azar(std::uint64_t semilla) : estado(semilla == 0 ? 0x9E3779B97F4A7C15ULL : semilla) {}

    std::uint32_t siguiente() {
        estado ^= estado << 13;
        estado ^= estado >> 7;
        estado ^= estado << 17;
        return static_cast<std::uint32_t>(estado >> 32);
    }

    std::uint32_t hasta(std::uint32_t limite) {
        return limite == 0 ? 0 : siguiente() % limite;
    }
};

std::string leer(const fs::path& ruta) {
    std::ifstream flujo(ruta, std::ios::binary);
    std::ostringstream texto;
    texto << flujo.rdbuf();
    return texto.str();
}

/// Trozos sueltos del lenguaje, para entradas que lleguen lejos en el análisis.
const char* PIEZAS[] = {
    "funcion",    "variable",  "constante",   "clase",
    "estructura", "rasgo",     "enumeracion", "publico",
    "privado",    "si",        "sino",        "mientras",
    "para",       "en",        "retornar",    "romper",
    "continuar",  "coincide",  "caso",        "nuevo",
    "este",       "super",     "hereda",      "implementa",
    "usar",       "modulo",    "entero",      "texto",
    "booleano",   "vacio",     "verdadero",   "falso",
    "imprimir",   "principal", "Vector",      "Mapa",
    "Caja",       "resultado", "{",           "}",
    "(",          ")",         "[",           "]",
    ";",          ",",         ":",           ".",
    "->",         "=",         "==",          "!=",
    "<",          ">",         "<=",          ">=",
    "+",          "-",         "*",           "/",
    "%",          "&&",        "||",          "!",
    "&",          "&mutable",  "?",           "..",
    "..=",        "\"texto\"", "'c'",         "123",
    "4.5",        "0xFF",      "1_000",       "// comentario\n",
    "\n",         "  ",
};

std::string entrada_aleatoria(Azar& azar, std::uint32_t longitud) {
    std::string salida;
    salida.reserve(longitud);
    for (std::uint32_t i = 0; i < longitud; ++i) {
        // Bytes imprimibles y UTF-8 válido la mayor parte del tiempo.
        salida += static_cast<char>(' ' + azar.hasta(95));
    }
    return salida;
}

std::string entrada_de_piezas(Azar& azar, std::uint32_t piezas) {
    std::string salida;
    for (std::uint32_t i = 0; i < piezas; ++i) {
        salida += PIEZAS[azar.hasta(std::size(PIEZAS))];
        salida += ' ';
    }
    return salida;
}

std::string mutar(Azar& azar, std::string base) {
    if (base.empty()) {
        return base;
    }
    const std::uint32_t cambios = 1 + azar.hasta(8);
    for (std::uint32_t i = 0; i < cambios; ++i) {
        const std::uint32_t posicion = azar.hasta(static_cast<std::uint32_t>(base.size()));
        switch (azar.hasta(4)) {
        case 0:  // borrar
            base.erase(posicion, 1 + azar.hasta(4));
            break;
        case 1:  // duplicar un trozo
            base.insert(posicion, base.substr(posicion, 1 + azar.hasta(8)));
            break;
        case 2:  // sustituir por un byte cualquiera
            base[posicion] = static_cast<char>(azar.hasta(256));
            break;
        default:  // insertar una pieza del lenguaje
            base.insert(posicion, PIEZAS[azar.hasta(std::size(PIEZAS))]);
            break;
        }
        if (base.empty()) {
            break;
        }
    }
    return base;
}

/// Pasa una entrada por todas las fases. Ninguna excepción ni caída es aceptable.
void analizar(const std::string& contenido, const std::string& nombre, bool generar) {
    nooki::diagnostico::GestorFuentes fuentes;
    nooki::diagnostico::Emisor emisor;
    const auto archivo = fuentes.anadir(nombre, contenido);

    nooki::lexico::Lexer lexer(fuentes, archivo, emisor);
    const auto tokens = lexer.todos();
    if (emisor.hay_errores()) {
        return;
    }

    nooki::sintactico::Parser parser(fuentes, archivo, emisor, tokens);
    const nooki::sintactico::Programa programa = parser.analizar();
    if (emisor.hay_errores()) {
        return;
    }

    nooki::semantico::verificar(programa, emisor);
    if (emisor.hay_errores() || !generar) {
        return;
    }

    // La generación es la última fase: también debe sobrevivir.
    (void)nooki::generador::generar_c(programa);
}

}  // namespace

int main(int argc, char** argv) {
    const int iteraciones = argc > 1 ? std::atoi(argv[1]) : 2000;
    const std::uint64_t semilla = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 0x5EED1234ULL;

    if (iteraciones <= 0) {
        std::cout << "uso: nooki_fuzz [iteraciones] [semilla]\n";
        return 2;
    }

    // Corpus: los programas que ya sabemos que son válidos.
    std::vector<std::string> corpus;
    std::error_code error_fs;
    const fs::path raiz(NOOKI_DIR_CONFORMIDAD);
    for (const auto& entrada : fs::recursive_directory_iterator(raiz, error_fs)) {
        if (entrada.is_regular_file() && entrada.path().extension() == ".oki") {
            corpus.push_back(leer(entrada.path()));
        }
    }
    if (corpus.empty()) {
        std::cout << "aviso: no se encontró corpus en " << raiz << "\n";
    }

    Azar azar(semilla);
    std::cout << "fuzzing: " << iteraciones << " entradas, semilla " << semilla << ", corpus "
              << corpus.size() << " archivos\n";

    for (int i = 0; i < iteraciones; ++i) {
        const std::uint32_t clase = azar.hasta(10);
        std::string contenido;
        bool generar = false;

        if (clase < 4 && !corpus.empty()) {
            // Mutación de un programa válido: la que más lejos llega.
            contenido = mutar(azar, corpus[azar.hasta(static_cast<std::uint32_t>(corpus.size()))]);
            generar = azar.hasta(2) == 0;
        } else if (clase < 7) {
            contenido = entrada_de_piezas(azar, 1 + azar.hasta(60));
        } else {
            contenido = entrada_aleatoria(azar, azar.hasta(400));
        }

        // Una excepción del compilador es un fallo del compilador: se guarda la
        // entrada que la provoca para poder reproducirla.
        try {
            analizar(contenido, "fuzz-" + std::to_string(i) + ".oki", generar);
        } catch (const std::exception& error) {
            const fs::path fallo("fallo-fuzz.oki");
            std::ofstream(fallo, std::ios::binary) << contenido;
            std::cout << "FALLO en la entrada " << i << ": " << error.what() << "\n"
                      << "  entrada guardada en «" << fallo.string() << "»\n";
            return 1;
        }
    }

    std::cout << "fuzzing: " << iteraciones << " entradas sin caídas ni excepciones\n";
    return 0;
}
