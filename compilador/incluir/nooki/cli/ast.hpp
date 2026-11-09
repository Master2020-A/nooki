#pragma once

#include <iosfwd>
#include <span>
#include <string_view>

namespace nooki::cli {

/// Implementa `nooki ast`: analiza un archivo y muestra el árbol en forma canónica.
int comando_ast(std::span<const std::string_view> argumentos, std::ostream& salida,
                std::ostream& error);

}  // namespace nooki::cli
