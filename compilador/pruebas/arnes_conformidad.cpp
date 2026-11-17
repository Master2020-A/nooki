// Arnés de conformidad.
//
// Recorre `pruebas/conformidad/` y ejecuta cada caso:
//
//   valido/<caso>/principal.oki + esperado.txt   compila a nativo, ejecuta y compara la salida
//   valido/<caso>/principal.oki                  solo comprueba que el léxico no da errores
//   invalido/<caso>/principal.oki + esperado.txt debe producir exactamente esos códigos
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

#include "nooki/conductor/compilar.hpp"
#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/proceso.hpp"
#include "nooki/semantico/verificaciones.hpp"
#include "nooki/sintactico/parser.hpp"

namespace fs = std::filesystem;

namespace {

using nooki::diagnostico::Emisor;
using nooki::diagnostico::GestorFuentes;

#ifdef _WIN32
constexpr const char* EXTENSION_BINARIO = ".exe";
#else
constexpr const char* EXTENSION_BINARIO = "";
#endif

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
            if (!entrada.is_directory() || !fs::exists(entrada.path() / "principal.oki")) {
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

/// Ejecuta un binario y devuelve su salida estándar.
std::string ejecutar_binario(const fs::path& binario) {
    const fs::path captura = fs::temp_directory_path() / "nooki_conformidad_salida.txt";
    nooki::ejecutar_comando("\"" + binario.string() + "\" > \"" + captura.string() + "\" 2>&1");

    const std::string salida = leer(captura);
    std::error_code ignorado;
    fs::remove(captura, ignorado);
    return salida;
}

/// Compila un caso válido y compara su salida con `esperado.txt`.
bool compilar_y_comparar(const Caso& caso, std::string& esperado, std::string& obtenido) {
    const fs::path directorio = fs::temp_directory_path() / "nooki_conformidad";
    fs::create_directories(directorio);

    nooki::conductor::Opciones opciones;
    opciones.fuente = caso.directorio / "principal.oki";
    opciones.salida = directorio / (caso.directorio.filename().string() + EXTENSION_BINARIO);

    GestorFuentes fuentes;
    Emisor emisor;
    std::ostringstream silencio;

    const auto resultado =
        nooki::conductor::compilar(opciones, fuentes, emisor, silencio, silencio);

    esperado = nooki::normalizar_saltos(leer(caso.directorio / "esperado.txt"));
    if (resultado.codigo != 0) {
        obtenido = "la compilación falló";
        if (emisor.cantidad() > 0) {
            obtenido += " (" + unir(codigos_de(emisor)) + ")";
        }
        return false;
    }

    obtenido = nooki::normalizar_saltos(ejecutar_binario(resultado.binario));
    std::error_code ignorado;
    fs::remove(resultado.binario, ignorado);
    return obtenido == esperado;
}

/// Pasa el caso por todas las fases (léxico, sintáctico y semántico) y compara
/// los códigos con los esperados. Un caso inválido debe fallar en la fase que sea.
bool analizar_codigos(const Caso& caso, std::vector<std::string>& esperados,
                      std::vector<std::string>& obtenidos) {
    const std::string contenido = leer(caso.directorio / "principal.oki");

    GestorFuentes fuentes;
    Emisor emisor;
    const auto archivo = fuentes.anadir(caso.nombre + "/principal.oki", contenido);

    nooki::lexico::Lexer lexer(fuentes, archivo, emisor);
    const auto tokens = lexer.todos();

    // Los casos válidos se compilan y ejecutan aparte; aquí solo se comprueba que
    // el léxico no se queje. Los inválidos, en cambio, pasan por todas las fases:
    // un error semántico también es un error del caso.
    if (caso.invalido && !emisor.hay_errores()) {
        nooki::sintactico::Parser parser(fuentes, archivo, emisor, tokens);
        const nooki::sintactico::Programa programa = parser.analizar();

        if (!emisor.hay_errores()) {
            nooki::semantico::verificar(programa, emisor);
        }
    }

    obtenidos = codigos_de(emisor);
    esperados =
        caso.invalido ? leer_codigos(caso.directorio / "esperado.txt") : std::vector<std::string>{};
    return obtenidos == esperados;
}

/// Mide el rendimiento del lexer sobre un corpus representativo.
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
        tokens_totales = lexer.todos().size();
    }
    const auto fin = std::chrono::steady_clock::now();

    const double segundos = std::chrono::duration<double>(fin - inicio).count();
    const double megabytes = static_cast<double>(corpus.size() * repeticiones) / (1024.0 * 1024.0);
    const double millones_tokens =
        static_cast<double>(tokens_totales * repeticiones) / (1000000.0 * segundos);

    std::cout << "\n  rendimiento del lexer: " << (megabytes / segundos) << " MB/s · "
              << millones_tokens << " M tokens/s (" << corpus.size() << " bytes × " << repeticiones
              << " pasadas)\n";
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

    const bool hay_compilador_c = !nooki::conductor::compilador_c().empty();

    int correctos = 0;
    int omitidos = 0;
    std::vector<std::string> fallidos;

    for (const Caso& caso : casos) {
        const bool es_programa = !caso.invalido && fs::exists(caso.directorio / "esperado.txt");

        if (es_programa && !hay_compilador_c) {
            ++omitidos;
            if (detalle) {
                std::cout << "  omitido " << caso.nombre << " (sin compilador de C)\n";
            }
            continue;
        }

        std::string esperado_texto;
        std::string obtenido_texto;
        bool correcto = false;

        if (es_programa) {
            correcto = compilar_y_comparar(caso, esperado_texto, obtenido_texto);
        } else {
            std::vector<std::string> esperados;
            std::vector<std::string> obtenidos;
            correcto = analizar_codigos(caso, esperados, obtenidos);
            esperado_texto = unir(esperados);
            obtenido_texto = unir(obtenidos);
        }

        if (correcto) {
            ++correctos;
            if (detalle) {
                std::cout << "  ok   " << caso.nombre << '\n';
            }
        } else {
            fallidos.push_back(caso.nombre);
            std::cout << "  FALLO " << caso.nombre << '\n'
                      << "        esperado: " << esperado_texto << '\n'
                      << "        obtenido: " << obtenido_texto << '\n';
        }
    }

    const int evaluados = static_cast<int>(casos.size()) - omitidos;
    std::cout << "\n  conformidad: " << correctos << '/' << evaluados << " casos correctos";
    if (omitidos > 0) {
        std::cout << " (" << omitidos << " omitidos por falta de compilador de C)";
    }
    std::cout << '\n';

    if (medir_rendimiento) {
        medir(casos);
    }

    if (!fallidos.empty()) {
        std::cout << "  casos fallidos: " << unir(fallidos) << '\n';
        return 1;
    }
    return 0;
}
