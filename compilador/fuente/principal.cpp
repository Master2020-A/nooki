#include "nooki/cli/orden.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
// `windows.h` ya trae `shellapi.h`; incluirlo antes rompe el orden que necesita
// MinGW (`EXTERN_C` se define en `windef.h`, que viene de `windows.h`).
#include <windows.h>
#endif

namespace {

#if defined(_WIN32)
/// En Windows los argumentos llegan en la página de códigos local, así que una
/// ruta con «ñ» o tildes se corrompe. Se leen de la línea de comandos en UTF-16 y
/// se convierten a UTF-8, que es lo que usa todo el compilador.
std::vector<std::string> argumentos_utf8() {
    std::vector<std::string> argumentos;

    int cantidad = 0;
    LPWSTR* lista = CommandLineToArgvW(GetCommandLineW(), &cantidad);
    if (lista == nullptr) {
        return argumentos;
    }

    for (int i = 1; i < cantidad; ++i) {
        const int bytes =
            WideCharToMultiByte(CP_UTF8, 0, lista[i], -1, nullptr, 0, nullptr, nullptr);
        if (bytes <= 1) {
            argumentos.emplace_back();
            continue;
        }
        std::string texto(static_cast<std::size_t>(bytes - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, lista[i], -1, texto.data(), bytes, nullptr, nullptr);
        argumentos.push_back(std::move(texto));
    }

    LocalFree(lista);
    return argumentos;
}
#endif

}  // namespace

int main(int argc, char* argv[]) {
#if defined(_WIN32)
    (void)argc;
    (void)argv;
    const std::vector<std::string> cadenas = argumentos_utf8();
    std::vector<std::string_view> argumentos;
    argumentos.reserve(cadenas.size());
    for (const std::string& cadena : cadenas) {
        argumentos.emplace_back(cadena);
    }
    return nooki::cli::ejecutar(argumentos, std::cout, std::cerr);
#else
    std::vector<std::string_view> argumentos;
    if (argc > 1) {
        const std::size_t cantidad = static_cast<std::size_t>(argc - 1);
        argumentos.reserve(cantidad);
        for (std::size_t i = 1; i <= cantidad; ++i) {
            argumentos.emplace_back(argv[i]);
        }
    }
    return nooki::cli::ejecutar(argumentos, std::cout, std::cerr);
#endif
}
