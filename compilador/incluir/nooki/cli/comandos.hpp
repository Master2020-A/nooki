#pragma once

#include <array>
#include <string_view>

namespace nooki::cli {

/// Un comando de la interfaz de línea de comandos.
struct Comando {
    std::string_view nombre;
    std::string_view resumen;

    /// Sprint del plan en el que se implementa. 0 significa "ya disponible".
    int sprint;

    [[nodiscard]] constexpr bool disponible() const noexcept { return sprint == 0; }
};

/// Tabla única de comandos: la usan el despachador y la ayuda.
/// Añadir un comando aquí lo hace aparecer en `nooki ayuda`.
inline constexpr std::array<Comando, 13> COMANDOS{{
    {"ayuda", "Muestra esta ayuda", 0},
    {"version", "Muestra la versión del compilador", 0},
    {"compilar", "Compila un archivo .oki y genera un ejecutable", 0},
    {"ejecutar", "Compila y ejecuta un archivo .oki", 0},
    {"ast", "Muestra el árbol de sintaxis del archivo", 0},
    {"construir", "Compila el proyecto del directorio actual", 0},
    {"lsp", "Atiende el protocolo de servidor de lenguaje", 0},
    {"nuevo", "Crea un proyecto nuevo", 0},
    {"probar", "Ejecuta las pruebas del proyecto", 0},
    {"limpiar", "Borra los artefactos de compilación", 0},
    {"verificar", "Comprueba el proyecto sin compilar", 59},
    {"formatear", "Formatea el código fuente", 56},
    {"documentar", "Genera la documentación del proyecto", 60},
}};

}  // namespace nooki::cli
