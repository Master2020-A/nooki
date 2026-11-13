#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"

namespace nooki::conductor {

struct Opciones {
    std::filesystem::path fuente;
    /// Ruta del binario. Vacía = mismo nombre que el fuente, junto a él.
    std::filesystem::path salida;
    /// Solo genera el C y lo escribe en el flujo de salida; no invoca nada.
    bool solo_c = false;
    /// Se detiene tras la verificación semántica: ni genera C ni compila.
    bool solo_verificar = false;
    /// No borra el archivo .c intermedio.
    bool conservar_intermedios = false;
    /// Carpetas adicionales donde buscar módulos (las dependencias del manifiesto).
    std::vector<std::filesystem::path> rutas_de_modulos;
};

struct Resultado {
    int codigo = 0;
    std::string c_generado;
    std::filesystem::path binario;
};

/// Compila un archivo `.oki` hasta un ejecutable nativo.
///
/// Etapas: leer → analizar léxicamente → analizar sintácticamente → generar C →
/// invocar el compilador de C del sistema (ADR-014).
Resultado compilar(const Opciones& opciones, diagnostico::GestorFuentes& fuentes,
                   diagnostico::Emisor& emisor, std::ostream& salida, std::ostream& error);

/// Compilador de C disponible, o cadena vacía si no hay ninguno.
/// Orden: `NOOKI_CC`, `cc`, `gcc`, `clang`.
[[nodiscard]] std::string compilador_c();

}  // namespace nooki::conductor
