// Arnés de conformidad.
//
// Recorre `pruebas/conformidad/` y ejecuta cada caso contra el analizador léxico:
//
//   valido/<caso>/principal.oki     debe analizarse sin diagnósticos
//   invalido/<caso>/principal.oki   debe producir exactamente los códigos de esperado.txt
//
// Cuando exista el compilador completo (S06 en adelante), los casos válidos además
// se compilarán y se comparará la salida estándar con `esperado.txt`.
//
// Uso:
//   nooki_conformidad [--directorio=<ruta>] [--detalle] [--sin-medicion]

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/lexico/lexer.hpp"

namespace fs = std::filesystem;

namespace {

using nooki::diagnostico::Emisor;
using nooki::diagnostico::GestorFuentes;

struct Caso {
    fs::path directorio;
    bool invalido = false;
    std::string nombre;  // "valido/0001-palabras-clave"
};

std::string leer(const fs::path& ruta) {
    std::ifstream entrada(ruta, std::ios::binary);
    if (!entrada) {
        return {};
    }
    std::ostringstream contenido;
    contenido << entrada.rdbuf();
    return contenido.str();
}

std::string recortar(std::string texto) {
    const auto inicio = texto.find_first_not_of(" \t\r\n");
    if (inicio == std::string::npos) {
        return {};
    }
    const auto fin = texto.find_last_not_of(" \t\r\n");
    return texto.substr(inicio, fin - inicio + 1);
}

std::vector<std::string> leer_codigos(const fs::path& ruta) {
    std::vector<std::string> codigos;
    std::ifstream entrada(ruta);
    std::string linea;
    while (std::getline(entrada, linea)) {
        const std::string limpia = recortar(linea);
        if (limpia.empty() || limpia[0] == '#') {
            continue;
        }
        codigos.push_back(limpia);
    }
    std::sort(codigos.begin(), codigos.end());
    return codigos;
}

std::vector<std::string> codigos_de(const Emisor& emisor) {
    std::vector<std::string> codigos;
    codigos.reserve(emisor.cantidad());
    for (const auto& diagnostico : emisor.diagnosticos()) {
        codigos.emplace_back(diagnostico.codigo());
    }
    std::sort(codigos.begin(), codigos.end());
    return codigos;
}

std::string unir(const std::vector<std::string>& valores) {
    if (valores.empty()) {
        return "ninguno";
    }
    std::string salida;
    for (std::size_t i = 0; i < valores.size(); ++i) {
        if (i != 0) {
            salida += ", ";
        }
        salida += valores[i];
    }
    return salida;
}

std::vector<Caso> descubrir(const fs::path& raiz) {
    std::vector<Caso> casos;

    for (const char* grupo : {"valido", "invalido"}) {
        const fs::path directorio_grupo = raiz / grupo;
        if (!fs::is_directory(directorio_grupo)) {
            continue;
        }
        for (const fs::directory_entry& entrada : fs::directory_iterator(directorio_grupo)) {
            if (!entrada.is_directory()) {
                continue;
            }
            if (!fs::exists(entrada.path() / "principal.oki")) {
                continue;
            }
            Caso caso;
            caso.directorio = entrada.path();
            caso.invalido = std::string_view(grupo) == "invalido";
            caso.nombre = std::string(grupo) + "/" + entrada.path().filename().string();
            casos.push_back(caso);
        }
    }

    std::sort(casos.begin(), casos.end(),
              [](const Caso& a, const Caso& b) { return a.nombre < b.nombre; });
    return casos;
}

/// Mide el rendimiento del lexer sobre un corpus representativo.
///
/// Los casos válidos son pequeños y densos en palabras clave, así que se repiten
/// hasta ~128 KB: medir sobre 3 KB daría un número halagador y falso.
void medir(const std::vector<Caso>& casos) {
    std::string base;
    for (const Caso& caso : casos) {
        if (!caso.invalido) {
            base += leer(caso.directorio / "principal.oki");
        }
    }
    if (base.empty()) {
        return;
    }

    constexpr std::size_t TAMANO_OBJETIVO = 128 * 1024;
    std::string corpus;
    corpus.reserve(TAMANO_OBJETIVO);
    while (corpus.size() < TAMANO_OBJETIVO) {
        corpus += base;
    }

    GestorFuentes fuentes;
    const auto archivo = fuentes.anadir("corpus.oki", corpus);

    constexpr int repeticiones = 100;
    std::size_t tokens_totales = 0;

    const auto inicio = std::chrono::steady_clock::now();
    for (int i = 0; i < repeticiones; ++i) {
        Emisor emisor;
        nooki::lexico::Lexer lexer(fuentes, archivo, emisor);
        const auto tokens = lexer.todos();
        tokens_totales = tokens.size();
    }
    const auto fin = std::chrono::steady_clock::now();

    const double segundos = std::chrono::duration<double>(fin - inicio).count();
    const double megabytes =
        static_cast<double>(corpus.size() * repeticiones) / (1024.0 * 1024.0);
    const double millones_tokens =
        static_cast<double>(tokens_totales * repeticiones) / (1000000.0 * segundos);

    std::cout << "\n  rendimiento del lexer: " << (megabytes / segundos) << " MB/s · "
              << millones_tokens << " M tokens/s (" << corpus.size() << " bytes × "
              << repeticiones << " pasadas)\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    fs::path raiz = NOOKI_DIR_CONFORMIDAD;
    bool detalle = false;
    bool medir_rendimiento = true;

    for (int i = 1; i < argc; ++i) {
        const std::string_view argumento = argv[i];
        constexpr std::string_view PREFIJO = "--directorio=";
        if (argumento.rfind(PREFIJO, 0) == 0) {
            raiz = std::string(argumento.substr(PREFIJO.size()));
        } else if (argumento == "--detalle") {
            detalle = true;
        } else if (argumento == "--sin-medicion") {
            medir_rendimiento = false;
        }
    }

    if (!fs::is_directory(raiz)) {
        std::cerr << "nooki_conformidad: no existe el directorio " << raiz.string() << '\n';
        return 2;
    }

    const std::vector<Caso> casos = descubrir(raiz);
    if (casos.empty()) {
        std::cerr << "nooki_conformidad: no se encontraron casos en " << raiz.string() << '\n';
        return 2;
    }

    int correctos = 0;
    std::vector<std::string> fallidos;

    for (const Caso& caso : casos) {
        const std::string contenido = leer(caso.directorio / "principal.oki");

        GestorFuentes fuentes;
        Emisor emisor;
        const auto archivo = fuentes.anadir(caso.nombre + "/principal.oki", contenido);
        nooki::lexico::Lexer lexer(fuentes, archivo, emisor);
        lexer.todos();

        const std::vector<std::string> obtenidos = codigos_de(emisor);
        const std::vector<std::string> esperados =
            caso.invalido ? leer_codigos(caso.directorio / "esperado.txt")
                          : std::vector<std::string>{};

        const bool correcto = obtenidos == esperados;
        if (correcto) {
            ++correctos;
            if (detalle) {
                std::cout << "  ok   " << caso.nombre << '\n';
            }
        } else {
            fallidos.push_back(caso.nombre);
            std::cout << "  FALLO " << caso.nombre << '\n'
                      << "        esperado: " << unir(esperados) << '\n'
                      << "        obtenido: " << unir(obtenidos) << '\n';
        }
    }

    std::cout << "\n  conformidad: " << correctos << '/' << casos.size() << " casos correctos\n";

    if (medir_rendimiento) {
        medir(casos);
    }

    if (!fallidos.empty()) {
        std::cout << "  casos fallidos: " << unir(fallidos) << '\n';
        return 1;
    }
    return 0;
}
