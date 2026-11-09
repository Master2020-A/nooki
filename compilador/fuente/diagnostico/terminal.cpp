#include "nooki/diagnostico/terminal.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <io.h>
#else
#include <unistd.h>
#endif

namespace nooki::diagnostico {
namespace {

/// Lee una variable de entorno sin silenciar los avisos de seguridad de MSVC.
std::string leer_entorno(const char* nombre) {
#ifdef _WIN32
    char* valor = nullptr;
    std::size_t longitud = 0;
    if (_dupenv_s(&valor, &longitud, nombre) != 0 || valor == nullptr) {
        return {};
    }
    std::string resultado(valor);
    std::free(valor);
    return resultado;
#else
    const char* valor = std::getenv(nombre);
    return valor == nullptr ? std::string{} : std::string{valor};
#endif
}

bool color_desactivado_por_entorno() { return !leer_entorno("NO_COLOR").empty(); }

bool es_terminal(std::FILE* flujo) {
#ifdef _WIN32
    return _isatty(_fileno(flujo)) != 0;
#else
    return isatty(fileno(flujo)) != 0;
#endif
}

bool habilitar_ansi(std::FILE* flujo) {
#ifdef _WIN32
    // Windows solo interpreta las secuencias ANSI si la consola lo permite.
    const HANDLE manejador =
        flujo == stderr ? GetStdHandle(STD_ERROR_HANDLE) : GetStdHandle(STD_OUTPUT_HANDLE);
    if (manejador == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD modo = 0;
    if (GetConsoleMode(manejador, &modo) == 0) {
        return false;
    }
    return SetConsoleMode(manejador, modo | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    (void)flujo;
    return true;
#endif
}

bool admite(std::FILE* flujo) {
    if (color_desactivado_por_entorno()) {
        return false;
    }
    if (!es_terminal(flujo)) {
        return false;
    }
    return habilitar_ansi(flujo);
}

}  // namespace

bool admite_color_estandar() { return admite(stdout); }

bool admite_color_error() { return admite(stderr); }

}  // namespace nooki::diagnostico
