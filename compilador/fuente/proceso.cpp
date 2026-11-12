#include "nooki/proceso.hpp"
#ifdef _WIN32
#include <windows.h>
#endif
#include <string>

#include <cstdlib>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace nooki {

#ifdef _WIN32
namespace {

/// `cmd` no entiende UTF-8: cuando la orden lleva bytes no ASCII se entrega a la
/// consola como UTF-16, que es lo que espera.
int ejecutar_comando_utf16(const std::string& comando) {
    const int bytes = MultiByteToWideChar(CP_UTF8, 0, comando.c_str(), -1, nullptr, 0);
    if (bytes <= 0) {
        return -1;
    }
    std::wstring ancha(static_cast<std::size_t>(bytes), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, comando.c_str(), -1, ancha.data(), bytes);
    return _wsystem(ancha.c_str());
}

}  // namespace
#endif

int ejecutar_comando(const std::string& comando) {
#ifdef _WIN32
    // `cmd /c` recorta la primera y la última comilla de la orden, así que una
    // orden que empieza entrecomillada necesita un par extra alrededor.
    const std::string entrecomillado =
        (!comando.empty() && comando.front() == '"') ? "\"" + comando + "\"" : comando;
    bool ascii = true;
    for (const unsigned char caracter : entrecomillado) {
        if (caracter >= 0x80) {
            ascii = false;
            break;
        }
    }
    if (!ascii) {
        return ejecutar_comando_utf16(entrecomillado);
    }
    return std::system(entrecomillado.c_str());
#else
    // `system` devuelve el estado de espera, no el código: hay que extraerlo.
    const int estado = std::system(comando.c_str());
    if (estado == -1) {
        return -1;
    }
    if (WIFEXITED(estado)) {
        return WEXITSTATUS(estado);
    }
    return 128 + (WIFSIGNALED(estado) ? WTERMSIG(estado) : 0);
#endif
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
