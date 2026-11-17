#include "nooki/diagnostico/emisor.hpp"

#include <utility>

namespace nooki::diagnostico {

Diagnostico& Emisor::error(std::string_view codigo, Span span) {
    return agregar(codigo, Gravedad::Error, span);
}

Diagnostico& Emisor::aviso(std::string_view codigo, Span span) {
    return agregar(codigo, Gravedad::Advertencia, span);
}

Diagnostico& Emisor::nota(std::string_view codigo, Span span) {
    return agregar(codigo, Gravedad::Nota, span);
}

Diagnostico& Emisor::agregar(std::string_view codigo, Gravedad gravedad, Span span) {
    diagnosticos_.emplace_back(codigo, gravedad, span);
    return diagnosticos_.back();
}

std::size_t Emisor::contar(Gravedad gravedad) const noexcept {
    std::size_t total = 0;
    for (const Diagnostico& diagnostico : diagnosticos_) {
        if (diagnostico.gravedad() == gravedad) {
            ++total;
        }
    }
    return total;
}

bool Emisor::hay_errores() const noexcept {
    return contar(Gravedad::Error) > 0;
}

}  // namespace nooki::diagnostico
