#include "nooki/diagnostico/terminal.hpp"

#include <cstdio>

#include "nooki/entorno.hpp"

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

bool color_desactivado_por_entorno() { return !nooki::leer_entorno("NO_COLOR").empty(); }

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
