#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "nooki/diagnostico/idioma.hpp"
#include "nooki/diagnostico/ubicacion.hpp"

namespace nooki::diagnostico {

/// Gravedad de un diagnóstico. Coincide con los rangos de código de la especificación:
/// NOK1xxx–NOK7xxx son errores, NOK8xxx advertencias, NOK9xxx errores internos.
enum class Gravedad : std::uint8_t { Error, Advertencia, Nota, Ayuda };

/// Nombre visible de la gravedad, ya traducido ("error", "advertencia", "nota", "ayuda").
[[nodiscard]] const char* nombre_gravedad(Gravedad gravedad, Idioma idioma) noexcept;

/// Etiqueta secundaria: un span adicional con su propia explicación.
struct Etiqueta {
    Span span;
    std::string mensaje;
};

/// Un diagnóstico completo.
///
/// Los mensajes se guardan como **código + parámetros**, no como texto ya formado:
/// así el mismo diagnóstico puede renderizarse en español o en inglés sin volver a
/// construirlo, y los mensajes viven en un único catálogo (`catalogo.hpp`).
class Diagnostico {
public:
    Diagnostico(std::string_view codigo, Gravedad gravedad, Span span);

    /// Rellena el parámetro `{nombre}` de la plantilla del mensaje.
    Diagnostico& parametro(std::string nombre, std::string valor);

    /// Añade una etiqueta secundaria con su span y su explicación.
    Diagnostico& etiqueta(Span span, std::string mensaje);

    /// Añade una nota al pie ("= nota: ...").
    Diagnostico& nota(std::string texto);

    /// Añade una ayuda con la acción recomendada ("= ayuda: ...").
    Diagnostico& ayuda(std::string texto);

    [[nodiscard]] std::string_view codigo() const noexcept {
        return codigo_;
    }
    [[nodiscard]] Gravedad gravedad() const noexcept {
        return gravedad_;
    }
    [[nodiscard]] Span span() const noexcept {
        return span_;
    }

    [[nodiscard]] const std::vector<std::pair<std::string, std::string>>&
    parametros() const noexcept {
        return parametros_;
    }
    [[nodiscard]] const std::vector<Etiqueta>& etiquetas() const noexcept {
        return etiquetas_;
    }
    [[nodiscard]] const std::vector<std::string>& notas() const noexcept {
        return notas_;
    }
    [[nodiscard]] const std::vector<std::string>& ayudas() const noexcept {
        return ayudas_;
    }

private:
    std::string_view codigo_;
    Gravedad gravedad_;
    Span span_;
    std::vector<std::pair<std::string, std::string>> parametros_;
    std::vector<Etiqueta> etiquetas_;
    std::vector<std::string> notas_;
    std::vector<std::string> ayudas_;
};

}  // namespace nooki::diagnostico
