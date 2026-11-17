#pragma once

#include <cstdint>
#include <string_view>

#include "nooki/diagnostico/ubicacion.hpp"

namespace nooki::lexico {

/// Clase de un lexema. El parser decide con estos valores, no comparando cadenas.
enum class TipoToken : std::uint8_t {
    FinDeArchivo,
    NuevaLinea,
    Identificador,
    PalabraClave,
    Entero,
    Flotante,
    Texto,
    Caracter,
    Vida,          // 'v, un tiempo de vida
    Operador,      // cualquier signo o delimitador
    Documentacion  // /// o //!
};

struct Token {
    TipoToken tipo = TipoToken::FinDeArchivo;

    /// Vista al texto del archivo: no se copia nada.
    std::string_view lexema;

    diagnostico::Span span;

    [[nodiscard]] bool es(TipoToken esperado) const noexcept {
        return tipo == esperado;
    }
};

[[nodiscard]] const char* nombre_tipo(TipoToken tipo) noexcept;

}  // namespace nooki::lexico
