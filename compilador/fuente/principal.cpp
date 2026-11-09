#include "nooki/cli/orden.hpp"

#include <cstddef>
#include <iostream>
#include <string_view>
#include <vector>

int main(int argc, char* argv[]) {
    std::vector<std::string_view> argumentos;

    if (argc > 1) {
        const std::size_t cantidad = static_cast<std::size_t>(argc - 1);
        argumentos.reserve(cantidad);
        for (std::size_t i = 1; i <= cantidad; ++i) {
            argumentos.emplace_back(argv[i]);
        }
    }

    return nooki::cli::ejecutar(argumentos, std::cout, std::cerr);
}
