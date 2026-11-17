#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/ubicacion.hpp"

namespace nooki::diagnostico {

/// Posición legible extraída de un desplazamiento en bytes.
/// `columna` se cuenta en puntos de código, no en bytes (ADR-008).
struct LineaColumna {
    std::uint32_t linea = 1;
    std::uint32_t columna = 1;
};

/// Registro de los archivos fuente de una compilación.
///
/// Es el dueño del texto: los spans guardan solo el identificador del archivo y
/// desplazamientos, de modo que se pueden comparar y almacenar sin copiar cadenas.
class GestorFuentes {
public:
    GestorFuentes();

    /// Registra un archivo y devuelve su identificador. El identificador 0 está reservado.
    ArchivoId anadir(std::string ruta, std::string contenido);

    [[nodiscard]] bool existe(ArchivoId id) const noexcept;
    [[nodiscard]] std::string_view ruta(ArchivoId id) const;
    [[nodiscard]] std::string_view contenido(ArchivoId id) const;

    /// Traduce un desplazamiento en bytes a línea y columna (1-based).
    [[nodiscard]] LineaColumna ubicar(ArchivoId id, std::uint32_t desplazamiento) const;

    /// Devuelve el texto de una línea (1-based), sin el salto de línea ni el `\r`.
    [[nodiscard]] std::string_view linea(ArchivoId id, std::uint32_t numero) const;

    /// Desplazamiento en bytes donde comienza una línea (1-based).
    [[nodiscard]] std::uint32_t inicio_de_linea(ArchivoId id, std::uint32_t numero) const;

    [[nodiscard]] std::uint32_t numero_de_lineas(ArchivoId id) const noexcept;

    [[nodiscard]] std::size_t cantidad() const noexcept {
        return archivos_.size() - 1;
    }

private:
    struct Archivo {
        std::string ruta;
        std::string contenido;
        std::vector<std::uint32_t> inicios;  // byte de inicio de cada línea; inicios[0] = 0
    };

    [[nodiscard]] const Archivo* buscar(ArchivoId id) const noexcept;

    // El índice 0 es un marcador vacío: los identificadores empiezan en 1.
    std::vector<Archivo> archivos_;
};

}  // namespace nooki::diagnostico
