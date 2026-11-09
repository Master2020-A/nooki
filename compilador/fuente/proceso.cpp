#include "nooki/proceso.hpp"

#include <cstdlib>

namespace nooki {

int ejecutar_comando(const std::string& comando) {
#ifdef _WIN32
    if (!comando.empty() && comando.front() == '"') {
        return std::system(("\"" + comando + "\"").c_str());
    }
#endif
    return std::system(comando.c_str());
}

std::string entrecomillar_si_hace_falta(const std::string& valor) {
    if (valor.find(' ') == std::string::npos && valor.find('\t') == std::string::npos) {
        return valor;
    }
    return "\"" + valor + "\"";
}

std::string normalizar_saltos(std::string texto) {
    std::string salida;
    salida.reserve(texto.size());
    for (std::size_t i = 0; i < texto.size(); ++i) {
        if (texto[i] == '\r' && i + 1 < texto.size() && texto[i + 1] == '\n') {
            continue;
        }
        salida += texto[i];
    }
    return salida;
}

}  // namespace nooki
