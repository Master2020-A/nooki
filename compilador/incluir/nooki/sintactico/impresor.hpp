#pragma once

#include <string>

#include "nooki/sintactico/ast.hpp"

namespace nooki::sintactico {

/// Imprime una expresión en forma canónica (prefija, con paréntesis).
///
///   `1 + 2 * 3`      →  `(+ 1 (* 2 3))`
///   `a.b[0]? ?? c`   →  `(?? (? (indice (. a b) 0)) c)`
///
/// Es la representación que usan las pruebas: si el árbol cambia, el texto cambia,
/// y la precedencia queda fijada por escrito.
[[nodiscard]] std::string imprimir(const Expresion& expresion);

}  // namespace nooki::sintactico
