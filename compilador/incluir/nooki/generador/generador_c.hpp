#pragma once

#include <string>

#include "nooki/sintactico/ast.hpp"

namespace nooki::generador {

struct ResultadoGeneracion {
    std::string codigo;
    /// Falso si el árbol usa algo que el backend C todavía no sabe traducir.
    bool soportado = true;
    /// Descripción de la primera construcción no soportada, para el diagnóstico.
    std::string motivo;
};

/// Traduce el árbol de sintaxis a C, que es el lenguaje intermedio de la v0.1
/// (ver ADR-014). El C generado se entrega a un compilador del sistema.
[[nodiscard]] ResultadoGeneracion generar_c(const sintactico::Programa& programa);

}  // namespace nooki::generador
