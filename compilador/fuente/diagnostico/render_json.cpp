#include "nooki/diagnostico/render.hpp"

#include <algorithm>
#include <cstdint>
#include <ostream>
#include <string_view>

#include "nooki/diagnostico/catalogo.hpp"
#include "nooki/utf8.hpp"

namespace nooki::diagnostico {
namespace {

constexpr const char* HEXADECIMAL = "0123456789abcdef";

/// Escribe una cadena JSON con los escapes obligatorios.
/// Los bytes UTF-8 válidos se emiten tal cual: JSON es UTF-8 por definición.
void escribir_cadena(std::ostream& salida, std::string_view texto) {
    salida << '"';
    for (const char crudo : texto) {
        const auto c = static_cast<unsigned char>(crudo);
        switch (c) {
        case '"':
            salida << "\\\"";
            break;
        case '\\':
            salida << "\\\\";
            break;
        case '\n':
            salida << "\\n";
            break;
        case '\r':
            salida << "\\r";
            break;
        case '\t':
            salida << "\\t";
            break;
        case '\b':
            salida << "\\b";
            break;
        case '\f':
            salida << "\\f";
            break;
        default:
            if (c < 0x20) {
                salida << "\\u00" << HEXADECIMAL[c >> 4] << HEXADECIMAL[c & 0x0F];
            } else {
                salida << static_cast<char>(c);
            }
        }
    }
    salida << '"';
}

const char* nombre_gravedad_json(Gravedad gravedad) {
    switch (gravedad) {
    case Gravedad::Error:
        return "error";
    case Gravedad::Advertencia:
        return "advertencia";
    case Gravedad::Nota:
        return "nota";
    case Gravedad::Ayuda:
        return "ayuda";
    }
    return "error";
}

void escribir_ubicacion(std::ostream& salida, const GestorFuentes& fuentes, Span span) {
    if (!span.valido() || !fuentes.existe(span.archivo)) {
        salida << "null";
        return;
    }

    const LineaColumna ubicacion = fuentes.ubicar(span.archivo, span.inicio);
    const std::string_view contenido = fuentes.contenido(span.archivo);
    const std::uint32_t limite = static_cast<std::uint32_t>(contenido.size());
    const std::uint32_t desde = std::min(span.inicio, limite);
    const std::uint32_t hasta = std::min(span.fin, limite);
    const std::uint32_t longitud =
        static_cast<std::uint32_t>(utf8::contar_puntos(contenido.substr(desde, hasta - desde)));

    salida << "{\"archivo\": ";
    escribir_cadena(salida, fuentes.ruta(span.archivo));
    salida << ", \"linea\": " << ubicacion.linea << ", \"columna\": " << ubicacion.columna
           << ", \"longitud\": " << longitud << '}';
}

void escribir_lista(std::ostream& salida, const std::vector<std::string>& textos) {
    salida << '[';
    bool primero = true;
    for (const std::string& texto : textos) {
        if (!primero) {
            salida << ", ";
        }
        primero = false;
        escribir_cadena(salida, texto);
    }
    salida << ']';
}

}  // namespace

void renderizar_json(std::span<const Diagnostico> diagnosticos, const GestorFuentes& fuentes,
                     std::ostream& salida, const OpcionesRender& opciones) {
    salida << "{\"idioma\": ";
    escribir_cadena(salida, codigo_iso(opciones.idioma));
    salida << ", \"diagnosticos\": [";

    bool primero = true;
    for (const Diagnostico& diagnostico : diagnosticos) {
        if (!primero) {
            salida << ',';
        }
        primero = false;

        salida << "\n  {\"codigo\": ";
        escribir_cadena(salida, diagnostico.codigo());
        salida << ", \"gravedad\": ";
        escribir_cadena(salida, nombre_gravedad_json(diagnostico.gravedad()));
        salida << ", \"mensaje\": ";
        escribir_cadena(salida,
                        componer(diagnostico.codigo(), diagnostico.parametros(), opciones.idioma));
        salida << ", \"ubicacion\": ";
        escribir_ubicacion(salida, fuentes, diagnostico.span());

        salida << ", \"etiquetas\": [";
        bool primera_etiqueta = true;
        for (const Etiqueta& etiqueta : diagnostico.etiquetas()) {
            if (!primera_etiqueta) {
                salida << ", ";
            }
            primera_etiqueta = false;
            salida << "{\"ubicacion\": ";
            escribir_ubicacion(salida, fuentes, etiqueta.span);
            salida << ", \"mensaje\": ";
            escribir_cadena(salida, etiqueta.mensaje);
            salida << '}';
        }
        salida << ']';

        salida << ", \"notas\": ";
        escribir_lista(salida, diagnostico.notas());
        salida << ", \"ayudas\": ";
        escribir_lista(salida, diagnostico.ayudas());
        salida << '}';
    }

    salida << "\n]}\n";
}

}  // namespace nooki::diagnostico
