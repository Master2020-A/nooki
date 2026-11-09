#pragma once

#include <iosfwd>

namespace nooki::cli {

/// Escribe la ayuda completa en el flujo indicado.
void imprimir_ayuda(std::ostream& salida);

/// Escribe la línea de versión en el flujo indicado.
void imprimir_version(std::ostream& salida);

}  // namespace nooki::cli
