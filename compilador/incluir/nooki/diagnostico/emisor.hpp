#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/diagnostico.hpp"

namespace nooki::diagnostico {

/// Acumulador de diagnósticos de una compilación.
///
/// El emisor no imprime nada: solo construye y guarda. Quién los muestra (texto,
/// JSON, LSP) se decide después, lo que permite tener un compilador callado en las
/// pruebas y una CLI con colores en la terminal.
class Emisor {
public:
    Diagnostico& error(std::string_view codigo, Span span);
    Diagnostico& aviso(std::string_view codigo, Span span);
    Diagnostico& nota(std::string_view codigo, Span span);

    [[nodiscard]] const std::vector<Diagnostico>& diagnosticos() const noexcept {
        return diagnosticos_;
    }
    [[nodiscard]] std::size_t contar(Gravedad gravedad) const noexcept;
    [[nodiscard]] bool hay_errores() const noexcept;
    [[nodiscard]] std::size_t cantidad() const noexcept { return diagnosticos_.size(); }

    void limpiar() noexcept { diagnosticos_.clear(); }

private:
    Diagnostico& agregar(std::string_view codigo, Gravedad gravedad, Span span);

    std::vector<Diagnostico> diagnosticos_;
};

}  // namespace nooki::diagnostico
