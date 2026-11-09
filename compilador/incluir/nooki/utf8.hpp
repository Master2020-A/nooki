#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace nooki::utf8 {

/// Un punto de código decodificado de una cadena UTF-8.
struct Punto {
    char32_t valor = 0;
    /// Número de bytes que ocupa en la cadena (1 si la secuencia es inválida).
    std::size_t bytes = 1;
};

/// Decodifica el punto de código que comienza en `indice`.
/// Si la secuencia es inválida, devuelve U+FFFD y avanza un solo byte:
/// el lexer puede así emitir un diagnóstico en lugar de abortar.
[[nodiscard]] Punto decodificar(std::string_view texto, std::size_t indice) noexcept;

/// Cuenta los puntos de código de `texto` (no los bytes).
[[nodiscard]] std::size_t contar_puntos(std::string_view texto) noexcept;

/// Indica si la secuencia que empieza en `indice` es UTF-8 válido.
[[nodiscard]] bool valido_en(std::string_view texto, std::size_t indice) noexcept;

}  // namespace nooki::utf8
