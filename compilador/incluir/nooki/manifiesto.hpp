#ifndef NOOKI_MANIFIESTO_HPP
#define NOOKI_MANIFIESTO_HPP

#include <filesystem>
#include <string>
#include <vector>

namespace nooki {

/// Una dependencia declarada en `nooki.toml`.
struct Dependencia {
    std::string nombre;
    /// Ruta a la carpeta del otro proyecto, ya resuelta respecto al manifiesto.
    std::filesystem::path ruta;
};

/// `nooki.toml`: lo mínimo para que un proyecto declare qué es y de qué depende.
struct Manifiesto {
    std::string nombre;
    std::string version = "0.1.0";
    std::vector<Dependencia> dependencias;

    /// Carpeta donde está el manifiesto.
    std::filesystem::path directorio;

    /// Rutas donde buscar módulos: la `biblioteca` de cada dependencia y su raíz.
    [[nodiscard]] std::vector<std::filesystem::path> rutas_de_busqueda() const;
};

/// Lee `nooki.toml` de una carpeta. Devuelve `false` y explica el motivo si falla.
bool leer_manifiesto(const std::filesystem::path& directorio, Manifiesto& manifiesto,
                     std::string& error);

/// Escribe un `nooki.toml` mínimo para un proyecto nuevo.
bool escribir_manifiesto(const std::filesystem::path& directorio, const std::string& nombre,
                         std::string& error);

}  // namespace nooki

#endif  // NOOKI_MANIFIESTO_HPP
