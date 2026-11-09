#pragma once

#include <string_view>

namespace nooki::semantico {

/// Tipo de un valor o de una expresión.
///
/// Es deliberadamente pequeño: los tipos compuestos (tuplas, vectores, referencias)
/// llegan con el sistema de tipos completo de S20–S24.
enum class Tipo : unsigned char {
    Desconocido,
    Vacio,
    Entero,
    Texto,
    Booleano,
};

/// Nombre del tipo para los mensajes ("entero", "texto", …).
[[nodiscard]] const char* nombre(Tipo tipo) noexcept;

/// Traduce el tipo tal como se escribió en el código fuente.
/// Devuelve `Desconocido` si el nombre no corresponde a ningún tipo conocido.
[[nodiscard]] Tipo desde_nombre(std::string_view escrito) noexcept;

}  // namespace nooki::semantico
