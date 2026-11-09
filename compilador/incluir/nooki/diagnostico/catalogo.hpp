#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "nooki/diagnostico/idioma.hpp"

namespace nooki::diagnostico {

using Parametros = std::vector<std::pair<std::string, std::string>>;

/// Plantilla del mensaje asociado a un código, en el idioma pedido.
/// Devuelve una vista vacía si el código no está catalogado (error de programación
/// del propio compilador, no del usuario).
[[nodiscard]] std::string_view plantilla_mensaje(std::string_view codigo, Idioma idioma);

/// Textos fijos que aparecen en todos los diagnósticos ("error", "ayuda", …).
[[nodiscard]] std::string_view texto_fijo(std::string_view clave, Idioma idioma);

/// Compone el mensaje final sustituyendo los `{parametro}` de la plantilla.
[[nodiscard]] std::string componer(std::string_view codigo, const Parametros& parametros,
                                   Idioma idioma);

}  // namespace nooki::diagnostico
