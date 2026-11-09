#pragma once

#include <string_view>

namespace nooki::diagnostico::codigos {

// Esquema definido en la especificación §11.1: NOK + 4 dígitos, estables para siempre.

// --- Uso del compilador (NOK0xxx) ------------------------------------------
inline constexpr std::string_view ARCHIVO_NO_ENCONTRADO = "NOK0001";

// --- Léxico (NOK1xxx) -------------------------------------------------------
inline constexpr std::string_view PALABRA_RESERVADA_CON_TILDE = "NOK1001";
inline constexpr std::string_view TEXTO_SIN_CERRAR = "NOK1003";
inline constexpr std::string_view CARACTER_INVALIDO = "NOK1004";
inline constexpr std::string_view NUMERO_MAL_FORMADO = "NOK1005";
inline constexpr std::string_view UTF8_INVALIDO = "NOK1006";
inline constexpr std::string_view CARACTER_LITERAL_SIN_CERRAR = "NOK1007";
inline constexpr std::string_view COMENTARIO_SIN_CERRAR = "NOK1008";
inline constexpr std::string_view ESCAPE_DESCONOCIDO = "NOK1009";

// --- Sintáctico (NOK2xxx) ---------------------------------------------------
inline constexpr std::string_view PARENTESIS_SIN_CERRAR = "NOK2001";
inline constexpr std::string_view TOKEN_INESPERADO = "NOK2002";
inline constexpr std::string_view SIN_PRINCIPAL = "NOK2003";
inline constexpr std::string_view NO_SOPORTADO = "NOK2004";
inline constexpr std::string_view RANGO_ENCADENADO = "NOK2005";

// --- Semántico (NOK3xxx) ----------------------------------------------------
inline constexpr std::string_view NOMBRE_NO_DECLARADO = "NOK3001";
inline constexpr std::string_view FUNCION_NO_DECLARADA = "NOK3002";
inline constexpr std::string_view TIPO_DESCONOCIDO = "NOK3003";
inline constexpr std::string_view CAMPO_DESCONOCIDO = "NOK3004";
inline constexpr std::string_view CAMPO_FALTANTE = "NOK3005";
inline constexpr std::string_view MIEMBRO_DESCONOCIDO = "NOK3006";
inline constexpr std::string_view VARIANTE_DESCONOCIDA = "NOK3007";

// --- Tipos (NOK4xxx) --------------------------------------------------------
inline constexpr std::string_view TIPO_INCOMPATIBLE = "NOK4001";
inline constexpr std::string_view ARGUMENTO_INCOMPATIBLE = "NOK4002";
inline constexpr std::string_view ARIDAD_INCORRECTA = "NOK4003";
inline constexpr std::string_view RETORNO_INCOMPATIBLE = "NOK4004";
inline constexpr std::string_view CONDICION_NO_BOOLEANA = "NOK4005";
inline constexpr std::string_view COINCIDE_NO_EXHAUSTIVO = "NOK4006";

// --- Propiedad y préstamos (NOK5xxx) ---------------------------------------
inline constexpr std::string_view USO_TRAS_MOVER = "NOK5001";

// --- Advertencias (NOK8xxx) -------------------------------------------------
inline constexpr std::string_view VARIABLE_SIN_MUTAR = "NOK8001";

// --- Generación y enlazado (NOK6xxx) ---------------------------------------
inline constexpr std::string_view SIN_COMPILADOR_C = "NOK6001";
inline constexpr std::string_view FALLO_COMPILADOR_C = "NOK6002";
inline constexpr std::string_view BACKEND_NO_SOPORTA = "NOK6003";

}  // namespace nooki::diagnostico::codigos
