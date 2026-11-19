cmake_minimum_required(VERSION 3.25)

project(Nooki
    VERSION 1.0.14
    DESCRIPTION "Compilador del lenguaje Nooki"
    HOMEPAGE_URL "https://nooki.dev"
    LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Perfil y metadatos del compilador
# ---------------------------------------------------------------------------

set(NOOKI_EDICION "2026" CACHE STRING "Edición del lenguaje que implementa el compilador")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(NOOKI_SISTEMA "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(NOOKI_SISTEMA "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(NOOKI_SISTEMA "macos")
else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" NOOKI_SISTEMA)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(AMD64|x86_64|amd64)$")
    set(NOOKI_ARQUITECTURA "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
    set(NOOKI_ARQUITECTURA "arm64")
else()
    set(NOOKI_ARQUITECTURA "${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(NOOKI_OBJETIVO "${NOOKI_SISTEMA}-${NOOKI_ARQUITECTURA}")

# ---------------------------------------------------------------------------
# Opciones del proyecto
# ---------------------------------------------------------------------------

option(NOOKI_PRUEBAS "Construir y registrar las pruebas" ON)
option(NOOKI_SANEAMIENTO "Activar AddressSanitizer y UndefinedBehaviorSanitizer" OFF)
option(NOOKI_AVISOS_COMO_ERRORES "Tratar los avisos del compilador como errores" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Módulos propios
# ---------------------------------------------------------------------------

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(NookiCompilador)

if(NOOKI_PRUEBAS)
    enable_testing()
endif()

add_subdirectory(compilador)

message(STATUS "Nooki ${PROJECT_VERSION} — objetivo ${NOOKI_OBJETIVO} — edición ${NOOKI_EDICION}")
message(STATUS "  compilador : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  pruebas    : ${NOOKI_PRUEBAS} · saneamiento: ${NOOKI_SANEAMIENTO} · avisos como errores: ${NOOKI_AVISOS_COMO_ERRORES}")
