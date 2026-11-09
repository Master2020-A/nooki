#pragma once

#include <iosfwd>
#include <span>
#include <string_view>

namespace nooki::cli {

// Códigos de salida del proceso. Se documentan en `nooki ayuda`.
inline constexpr int EXITO = 0;
inline constexpr int USO_INCORRECTO = 64;
inline constexpr int DATOS_INCORRECTOS = 65;
inline constexpr int NO_DISPONIBLE = 69;
inline constexpr int ERROR_INTERNO = 70;

/// Despacha la línea de comandos. No imprime por stdout/stderr directamente:
/// recibe los flujos para que sea comprobable desde las pruebas.
int ejecutar(std::span<const std::string_view> argumentos,
             std::ostream& salida,
             std::ostream& error);

}  // namespace nooki::cli
