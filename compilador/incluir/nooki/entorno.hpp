#pragma once

#include <string>

namespace nooki {

/// Lee una variable de entorno. Devuelve una cadena vacía si no existe.
/// Aísla el uso de `_dupenv_s` en Windows (MSVC marca `getenv` como insegura).
[[nodiscard]] std::string leer_entorno(const char* nombre);

}  // namespace nooki
