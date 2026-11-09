#pragma once

namespace nooki::diagnostico {

/// Indica si la salida estándar admite color.
/// Respeta la variable de entorno `NO_COLOR` (https://no-color.org).
[[nodiscard]] bool admite_color_estandar();

/// Igual que la anterior, para la salida de error.
[[nodiscard]] bool admite_color_error();

}  // namespace nooki::diagnostico
