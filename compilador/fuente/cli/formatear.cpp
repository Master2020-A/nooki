#include <algorithm>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/cli/comandos.hpp"
#include "nooki/cli/orden.hpp"
#include "nooki/rutas.hpp"

namespace nooki::cli {
namespace {

/// Indentación en espacios por nivel de anidamiento.
constexpr int SANGRIA = 4;

/// Quita los espacios del final de una línea.
std::string sin_espacios_finales(const std::string& linea) {
    std::size_t fin = linea.size();
    while (fin > 0 && (linea[fin - 1] == ' ' || linea[fin - 1] == '\t' || linea[fin - 1] == '\r')) {
        --fin;
    }
    return linea.substr(0, fin);
}

/// Cuenta las llaves que abren y cierran, sin mirar dentro de textos ni comentarios.
/// Devuelve el cambio de profundidad y si la línea empieza cerrando un bloque.
std::pair<int, int> llaves_de(const std::string& linea) {
    int abre = 0;
    int cierra = 0;
    bool en_texto = false;
    bool en_comentario = false;
    bool escapado = false;

    for (std::size_t i = 0; i < linea.size(); ++i) {
        const char c = linea[i];
        if (en_comentario) {
            break;
        }
        if (en_texto) {
            if (escapado) {
                escapado = false;
            } else if (c == '\\') {
                escapado = true;
            } else if (c == '"') {
                en_texto = false;
            }
            continue;
        }
        if (c == '"') {
            en_texto = true;
            continue;
        }
        // Un comentario de línea: lo que quede no cuenta.
        if (c == '/' && i + 1 < linea.size() && linea[i + 1] == '/') {
            en_comentario = true;
            continue;
        }
        if (c == '{') {
            ++abre;
        } else if (c == '}') {
            ++cierra;
        }
    }
    return {abre, cierra};
}

/// Formatea el texto de un archivo entero.
std::string formatear(std::string_view texto) {
    std::istringstream entrada{std::string(texto)};
    std::ostringstream salida;
    std::string linea;
    int profundidad = 0;

    while (std::getline(entrada, linea)) {
        const std::string limpia = sin_espacios_finales(linea);

        // Las líneas vacías se respetan tal cual.
        if (limpia.find_first_not_of(" \t") == std::string::npos) {
            salida << '\n';
            continue;
        }

        // Una línea que empieza cerrando va un nivel por detrás.
        const std::string_view vista(limpia);
        const std::string::size_type primero = vista.find_first_not_of(" \t");
        const bool empieza_cerrando = primero != std::string_view::npos && vista[primero] == '}';

        const int nivel = std::max(0, profundidad - (empieza_cerrando ? 1 : 0));
        salida << std::string(static_cast<std::size_t>(nivel) * SANGRIA, ' ') << vista.substr(primero)
               << '\n';

        const auto [abre, cierra] = llaves_de(limpia);
        profundidad += abre - cierra;
        if (profundidad < 0) {
            profundidad = 0;
        }
    }
    return salida.str();
}

}  // namespace

int comando_formatear(std::span<const std::string_view> argumentos, std::ostream& salida,
                      std::ostream& error) {
    bool comprobar_solo = false;
    std::vector<std::string_view> archivos;
    for (const std::string_view argumento : argumentos) {
        if (argumento == "--comprobar") {
            comprobar_solo = true;
        } else if (!argumento.empty() && argumento.front() == '-') {
            error << "nooki: la opción «" << argumento << "» no existe\n";
            return USO_INCORRECTO;
        } else {
            archivos.push_back(argumento);
        }
    }

    if (archivos.empty()) {
        error << "nooki: falta el archivo a formatear\n";
        error << "uso: nooki formatear <archivo.oki>... [--comprobar]\n";
        return USO_INCORRECTO;
    }

    bool hubo_cambios = false;
    for (const std::string_view archivo : archivos) {
        const std::filesystem::path ruta = ruta_desde_utf8(archivo);
        std::ifstream flujo(ruta, std::ios::binary);
        if (!flujo) {
            error << "nooki: no se pudo leer «" << archivo << "»\n";
            return DATOS_INCORRECTOS;
        }
        std::ostringstream contenido;
        contenido << flujo.rdbuf();
        flujo.close();

        const std::string original = contenido.str();
        const std::string formateado = formatear(original);
        if (formateado == original) {
            continue;
        }
        hubo_cambios = true;

        if (comprobar_solo) {
            error << "nooki: «" << archivo << "» no está formateado\n";
            continue;
        }
        std::ofstream escritura(ruta, std::ios::binary | std::ios::trunc);
        if (!escritura) {
            error << "nooki: no se pudo escribir «" << archivo << "»\n";
            return DATOS_INCORRECTOS;
        }
        escritura << formateado;
        salida << "formateado: " << archivo << '\n';
    }

    if (comprobar_solo && hubo_cambios) {
        return DATOS_INCORRECTOS;
    }
    return EXITO;
}

}  // namespace nooki::cli
