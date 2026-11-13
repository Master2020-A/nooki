#ifndef NOOKI_UI_RUNTIME_UI_HPP
#define NOOKI_UI_RUNTIME_UI_HPP

#include <string_view>

namespace nooki::ui {

/// Código C de la capa de ventanas, con una implementación por plataforma.
/// Se añade al programa solo cuando se usa el módulo `ventana`.
[[nodiscard]] std::string_view fuente_runtime_ui();

}  // namespace nooki::ui

#endif  // NOOKI_UI_RUNTIME_UI_HPP
