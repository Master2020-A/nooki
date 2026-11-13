# compilador/CMakeLists.txt

# ---------------------------------------------------------------------------
# Cabecera de versión generada: única fuente de verdad (CMakeLists.txt raíz).
# ---------------------------------------------------------------------------

set(NOOKI_DIR_GENERADO "${CMAKE_BINARY_DIR}/generado")
file(MAKE_DIRECTORY "${NOOKI_DIR_GENERADO}/nooki")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/incluir/nooki/version.hpp.in"
    "${NOOKI_DIR_GENERADO}/nooki/version.hpp"
    @ONLY)

# ---------------------------------------------------------------------------
# Núcleo del compilador: biblioteca estática que concentra la lógica.
# El ejecutable solo traduce la línea de comandos y llama al núcleo.
# ---------------------------------------------------------------------------

add_library(nooki_nucleo STATIC
    fuente/utf8.cpp
    fuente/entorno.cpp
    fuente/proceso.cpp
    fuente/manifiesto.cpp
    fuente/lsp/json.cpp
    fuente/lsp/servidor.cpp
    fuente/ui/runtime_ui.cpp
    fuente/cli/orden.cpp
    fuente/cli/ayuda.cpp
    fuente/cli/compilar.cpp
    fuente/cli/ast.cpp
    fuente/cli/proyecto.cpp
    fuente/diagnostico/gestor_fuentes.cpp
    fuente/diagnostico/catalogo.cpp
    fuente/diagnostico/diagnostico.cpp
    fuente/diagnostico/emisor.cpp
    fuente/diagnostico/render_texto.cpp
    fuente/diagnostico/render_json.cpp
    fuente/diagnostico/terminal.cpp
    fuente/lexico/token.cpp
    fuente/lexico/lexer.cpp
    fuente/sintactico/parser.cpp
    fuente/sintactico/impresor.cpp
    fuente/semantico/tipos.cpp
    fuente/semantico/verificaciones.cpp
    fuente/generador/generador_c.cpp
    fuente/conductor/compilar.cpp
)

target_include_directories(nooki_nucleo
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/incluir"
        "${NOOKI_DIR_GENERADO}")

nooki_aplicar_opciones(nooki_nucleo)

# ---------------------------------------------------------------------------
# Ejecutable `nooki`
# ---------------------------------------------------------------------------

add_executable(nooki fuente/principal.cpp)
target_link_libraries(nooki PRIVATE nooki_nucleo)
if(WIN32)
    target_link_libraries(nooki PRIVATE shell32)
endif()
nooki_aplicar_opciones(nooki)

set_target_properties(nooki PROPERTIES OUTPUT_NAME "nooki")

# ---------------------------------------------------------------------------
# Pruebas
# ---------------------------------------------------------------------------

if(NOOKI_PRUEBAS)
    add_subdirectory(pruebas)
endif()
