#pragma once

#include <cstdint>
#include <string_view>

namespace nooki::diagnostico {

/// Idioma de los mensajes del compilador.
///
/// El español es el idioma por defecto y el único completo durante la v0.1;
/// el inglés existe desde el primer día para que traducir no exija reescribir
/// el compilador (decisión ADR-008 y `DECISIONES.md`).
enum class Idioma : std::uint8_t { Espanol, Ingles };

[[nodiscard]] constexpr const char* codigo_iso(Idioma idioma) noexcept {
    return idioma == Idioma::Espanol ? "es" : "en";
}

}  // namespace nooki::diagnostico
