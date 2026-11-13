#ifndef NOOKI_FORMATO_FORMATEADOR_HPP
#define NOOKI_FORMATO_FORMATEADOR_HPP

#include <string>
#include <string_view>

namespace nooki::formato {

/// Indentación en espacios por nivel de anidamiento.
inline constexpr int SANGRIA = 4;

/// Devuelve el texto con la sangría normalizada y sin espacios al final de línea.
///
/// No toca el contenido: ni comentarios, ni textos, ni el orden del código. Las
/// llaves dentro de textos y comentarios no cuentan para la profundidad.
[[nodiscard]] std::string formatear(std::string_view texto);

}  // namespace nooki::formato

#endif  // NOOKI_FORMATO_FORMATEADOR_HPP
