#pragma once

#include <string_view>

namespace nooki::diagnostico::codigos {

// Esquema definido en la especificación §11.1: NOK + 4 dígitos, estables para siempre.

// --- Léxico (NOK1xxx) -------------------------------------------------------
inline constexpr std::string_view PALABRA_RESERVADA_CON_TILDE = "NOK1001";

// --- Sintáctico (NOK2xxx) ---------------------------------------------------
inline constexpr std::string_view PARENTESIS_SIN_CERRAR = "NOK2001";

// --- Semántico (NOK3xxx) ----------------------------------------------------
inline constexpr std::string_view NOMBRE_NO_DECLARADO = "NOK3001";

// --- Tipos (NOK4xxx) --------------------------------------------------------
inline constexpr std::string_view TIPO_INCOMPATIBLE = "NOK4001";

// --- Propiedad y préstamos (NOK5xxx) ---------------------------------------
inline constexpr std::string_view USO_TRAS_MOVER = "NOK5001";

// --- Advertencias (NOK8xxx) -------------------------------------------------
inline constexpr std::string_view VARIABLE_SIN_MUTAR = "NOK8001";

}  // namespace nooki::diagnostico::codigos
