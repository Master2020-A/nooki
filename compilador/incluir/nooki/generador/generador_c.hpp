#pragma once

#include <string>

#include "nooki/sintactico/ast.hpp"

namespace nooki::generador {

/// Traduce el árbol de sintaxis a C, que es el lenguaje intermedio de la v0.1
/// (ver ADR-014). El C generado se entrega a un compilador del sistema.
[[nodiscard]] std::string generar_c(const sintactico::Programa& programa);

}  // namespace nooki::generador
