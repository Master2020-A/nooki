#pragma once

#include <cstdint>

namespace nooki::diagnostico {

/// Identificador de un archivo registrado en el GestorFuentes.
/// El valor 0 está reservado para "ningún archivo".
using ArchivoId = std::uint32_t;

inline constexpr ArchivoId ARCHIVO_NINGUNO = 0;

/// Región de un archivo fuente.
///
/// Los desplazamientos son **bytes** (lo que produce el lexer de forma natural);
/// la conversión a línea y columna en puntos de código la hace el GestorFuentes,
/// según la decisión ADR-008.
struct Span {
    ArchivoId archivo = ARCHIVO_NINGUNO;
    std::uint32_t inicio = 0;
    std::uint32_t fin = 0;

    [[nodiscard]] constexpr bool valido() const noexcept {
        return archivo != ARCHIVO_NINGUNO && fin >= inicio;
    }

    [[nodiscard]] constexpr bool vacio() const noexcept {
        return inicio >= fin;
    }

    [[nodiscard]] constexpr std::uint32_t longitud() const noexcept {
        return fin > inicio ? fin - inicio : 0;
    }
};

}  // namespace nooki::diagnostico
