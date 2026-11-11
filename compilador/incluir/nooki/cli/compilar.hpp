#pragma once

#include <iosfwd>
#include <span>
#include <string_view>

namespace nooki::cli {

/// Implementa `nooki compilar`.
int comando_construir(std::span<const std::string_view> argumentos, std::ostream& salida,
                      std::ostream& error);

int comando_compilar(std::span<const std::string_view> argumentos, std::ostream& salida,
                     std::ostream& error);

/// Implementa `nooki ejecutar`: compila y ejecuta el resultado.
int comando_ejecutar(std::span<const std::string_view> argumentos, std::ostream& salida,
                     std::ostream& error);

}  // namespace nooki::cli
