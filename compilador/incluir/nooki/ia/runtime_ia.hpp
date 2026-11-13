#ifndef NOOKI_IA_RUNTIME_IA_HPP
#define NOOKI_IA_RUNTIME_IA_HPP

#include <string_view>

namespace nooki::ia {

/// Código C de la capa de tensores. Se añade al programa solo cuando se usa el
/// módulo `tensor`.
[[nodiscard]] std::string_view fuente_runtime_ia();

}  // namespace nooki::ia

#endif  // NOOKI_IA_RUNTIME_IA_HPP
