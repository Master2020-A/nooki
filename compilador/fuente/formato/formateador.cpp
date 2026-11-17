#include "nooki/formato/formateador.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace nooki::formato {
namespace {

/// Quita los espacios del final de una línea.
std::string sin_espacios_finales(const std::string& linea) {
    std::size_t fin = linea.size();
    while (fin > 0 && (linea[fin - 1] == ' ' || linea[fin - 1] == '\t' || linea[fin - 1] == '\r')) {
        --fin;
    }
    return linea.substr(0, fin);
}

/// Cuenta las llaves que abren y cierran, sin mirar dentro de textos ni comentarios.
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

}  // namespace

/// Cuenta las apariciones de `"""` en una línea, que abren o cierran un texto
/// multilínea.
int comillas_triples(const std::string& linea) {
    int cuantas = 0;
    for (std::size_t i = 0; i + 2 < linea.size(); ++i) {
        if (linea[i] == '"' && linea[i + 1] == '"' && linea[i + 2] == '"') {
            ++cuantas;
            i += 2;
        }
    }
    return cuantas;
}

std::string formatear(std::string_view texto) {
    std::istringstream entrada{std::string(texto)};
    std::ostringstream salida;
    std::string linea;
    int profundidad = 0;
    // Dentro de un texto multilínea no se toca nada: la sangría forma parte del
    // contenido del texto y cambiarla cambiaría el programa.
    bool en_texto_largo = false;

    while (std::getline(entrada, linea)) {
        const std::string limpia = sin_espacios_finales(linea);

        if (en_texto_largo) {
            salida << limpia << '\n';
            if (comillas_triples(limpia) % 2 == 1) {
                en_texto_largo = false;
            }
            continue;
        }

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
        salida << std::string(static_cast<std::size_t>(nivel) * SANGRIA, ' ')
               << vista.substr(primero) << '\n';

        const auto [abre, cierra] = llaves_de(limpia);
        profundidad += abre - cierra;
        if (profundidad < 0) {
            profundidad = 0;
        }
        if (comillas_triples(limpia) % 2 == 1) {
            en_texto_largo = true;
        }
    }
    return salida.str();
}

}  // namespace nooki::formato
