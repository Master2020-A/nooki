#pragma once

#include <iosfwd>
#include <span>

#include "nooki/diagnostico/diagnostico.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/diagnostico/idioma.hpp"

namespace nooki::diagnostico {

struct OpcionesRender {
    Idioma idioma = Idioma::Espanol;
    /// Si es falso, la salida no contiene secuencias ANSI. Las pruebas doradas
    /// siempre renderizan sin color.
    bool color = false;
};

/// Renderiza un diagnóstico en texto legible, con la línea de código y los caretos:
///
/// ```
/// error[NOK5001]: no se puede usar «datos» porque se movió
///   --> principal.oki:3:5
///    |
///  2 |     variable otro = datos
///    |                     ^^^^^ el valor se movió aquí
/// ```
void renderizar_texto(const Diagnostico& diagnostico, const GestorFuentes& fuentes,
                      std::ostream& salida, const OpcionesRender& opciones = {});

/// Renderiza una lista de diagnósticos en texto, uno detrás de otro.
void renderizar_texto(std::span<const Diagnostico> diagnosticos, const GestorFuentes& fuentes,
                      std::ostream& salida, const OpcionesRender& opciones = {});

/// Renderiza en el formato JSON que consume el LSP y las herramientas
/// (`nooki --diagnosticos=json`, especificación §11.1).
void renderizar_json(std::span<const Diagnostico> diagnosticos, const GestorFuentes& fuentes,
                     std::ostream& salida, const OpcionesRender& opciones = {});

}  // namespace nooki::diagnostico
