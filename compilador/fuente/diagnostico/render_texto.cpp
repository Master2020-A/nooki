#include "nooki/diagnostico/render.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/catalogo.hpp"
#include "nooki/utf8.hpp"

namespace nooki::diagnostico {
namespace {

constexpr std::uint32_t ANCHO_TABULADOR = 4;

constexpr const char* ANSI_ROJO = "\x1b[1;31m";
constexpr const char* ANSI_AMARILLO = "\x1b[1;33m";
constexpr const char* ANSI_CIAN = "\x1b[1;36m";
constexpr const char* ANSI_VERDE = "\x1b[1;32m";
constexpr const char* ANSI_AZUL = "\x1b[1;34m";
constexpr const char* ANSI_FIN = "\x1b[0m";

const char* color_de(Gravedad gravedad) {
    switch (gravedad) {
        case Gravedad::Error:
            return ANSI_ROJO;
        case Gravedad::Advertencia:
            return ANSI_AMARILLO;
        case Gravedad::Nota:
            return ANSI_CIAN;
        case Gravedad::Ayuda:
            return ANSI_VERDE;
    }
    return ANSI_ROJO;
}

/// Escribe `texto` con el color indicado, o tal cual si el color está desactivado.
void escribir(std::ostream& salida, bool color, const char* codigo, std::string_view texto) {
    if (color) {
        salida << codigo << texto << ANSI_FIN;
    } else {
        salida << texto;
    }
}

/// Línea de código ya preparada para pintar: tabuladores expandidos y la columna
/// de pantalla de cada punto de código.
struct LineaVisible {
    std::string texto;
    std::vector<std::uint32_t> columna;
    std::uint32_t ancho = 0;  // columnas de pantalla totales
};

LineaVisible preparar(std::string_view cruda) {
    LineaVisible visible;
    visible.columna.reserve(cruda.size());

    std::uint32_t columna = 0;
    std::size_t i = 0;
    while (i < cruda.size()) {
        const utf8::Punto punto = utf8::decodificar(cruda, i);
        visible.columna.push_back(columna);
        if (punto.valor == '\t') {
            const std::uint32_t siguiente = ((columna / ANCHO_TABULADOR) + 1) * ANCHO_TABULADOR;
            while (columna < siguiente) {
                visible.texto.push_back(' ');
                ++columna;
            }
        } else {
            visible.texto.append(cruda.substr(i, punto.bytes));
            ++columna;
        }
        i += punto.bytes;
    }
    visible.ancho = columna;
    return visible;
}

/// Una marca sobre una línea: dónde empieza, cuánto ocupa y qué dice.
struct Marca {
    std::uint32_t inicio = 0;
    std::uint32_t longitud = 1;
    std::string mensaje;
};

struct Bloque {
    ArchivoId archivo = ARCHIVO_NINGUNO;
    std::uint32_t numero = 0;
    std::vector<Marca> marcas;
};

std::uint32_t ancho_de_digitos(std::uint32_t valor) {
    std::uint32_t digitos = 1;
    while (valor >= 10) {
        valor /= 10;
        ++digitos;
    }
    return digitos;
}

/// Convierte un span en una marca, recortándola al final de su línea.
Marca construir_marca(const GestorFuentes& fuentes, ArchivoId archivo, Span span,
                      std::string mensaje) {
    Marca marca;
    marca.mensaje = std::move(mensaje);

    const LineaColumna ubicacion = fuentes.ubicar(archivo, span.inicio);
    marca.inicio = ubicacion.columna > 0 ? ubicacion.columna - 1 : 0;

    const std::uint32_t inicio_linea = fuentes.inicio_de_linea(archivo, ubicacion.linea);
    const std::string_view texto_linea = fuentes.linea(archivo, ubicacion.linea);
    const std::uint32_t fin_linea =
        inicio_linea + static_cast<std::uint32_t>(texto_linea.size());

    const std::uint32_t desde = std::min(span.inicio, fin_linea);
    const std::uint32_t hasta = std::min(span.fin, fin_linea);
    if (hasta > desde) {
        marca.longitud = static_cast<std::uint32_t>(
            utf8::contar_puntos(fuentes.contenido(archivo).substr(desde, hasta - desde)));
    }
    if (marca.longitud == 0) {
        marca.longitud = 1;
    }
    return marca;
}

std::vector<Bloque> agrupar(const Diagnostico& diagnostico, const GestorFuentes& fuentes) {
    std::vector<Bloque> bloques;

    auto agregar = [&](Span span, std::string mensaje) {
        if (!span.valido() || !fuentes.existe(span.archivo)) {
            return;
        }
        const std::uint32_t linea = fuentes.ubicar(span.archivo, span.inicio).linea;
        const Marca marca = construir_marca(fuentes, span.archivo, span, std::move(mensaje));

        auto it = std::find_if(bloques.begin(), bloques.end(), [&](const Bloque& b) {
            return b.archivo == span.archivo && b.numero == linea;
        });
        if (it == bloques.end()) {
            bloques.push_back(Bloque{span.archivo, linea, {}});
            it = bloques.end() - 1;
        }
        it->marcas.push_back(marca);
    };

    agregar(diagnostico.span(), std::string{});
    for (const Etiqueta& etiqueta : diagnostico.etiquetas()) {
        agregar(etiqueta.span, etiqueta.mensaje);
    }

    std::stable_sort(bloques.begin(), bloques.end(), [](const Bloque& a, const Bloque& b) {
        if (a.archivo != b.archivo) {
            return a.archivo < b.archivo;
        }
        return a.numero < b.numero;
    });
    for (Bloque& bloque : bloques) {
        std::stable_sort(bloque.marcas.begin(), bloque.marcas.end(),
                         [](const Marca& a, const Marca& b) { return a.inicio < b.inicio; });
    }
    return bloques;
}

void escribir_marca(std::ostream& salida, std::uint32_t ancho, const LineaVisible& linea,
                    const Marca& marca, const char* color, bool usar_color) {
    salida << std::string(ancho, ' ') << " | ";

    const std::uint32_t columna =
        marca.inicio < linea.columna.size() ? linea.columna[marca.inicio] : linea.ancho;

    salida << std::string(columna, ' ');
    escribir(salida, usar_color, color, std::string(marca.longitud, '^'));
    if (!marca.mensaje.empty()) {
        salida << ' ';
        escribir(salida, usar_color, color, marca.mensaje);
    }
    salida << '\n';
}

}  // namespace

void renderizar_texto(const Diagnostico& diagnostico, const GestorFuentes& fuentes,
                      std::ostream& salida, const OpcionesRender& opciones) {
    const Idioma idioma = opciones.idioma;
    const bool color = opciones.color;
    const char* color_gravedad = color_de(diagnostico.gravedad());

    // 1. Cabecera: error[NOK5001]: mensaje
    escribir(salida, color, color_gravedad, nombre_gravedad(diagnostico.gravedad(), idioma));
    salida << '[';
    escribir(salida, color, color_gravedad, diagnostico.codigo());
    salida << "]: " << componer(diagnostico.codigo(), diagnostico.parametros(), idioma) << '\n';

    // 2. Ubicación principal
    const Span principal = diagnostico.span();
    if (principal.valido() && fuentes.existe(principal.archivo)) {
        const LineaColumna ubicacion = fuentes.ubicar(principal.archivo, principal.inicio);
        salida << "  --> ";
        if (color) {
            salida << ANSI_AZUL;
        }
        salida << fuentes.ruta(principal.archivo) << ':' << ubicacion.linea << ':'
               << ubicacion.columna;
        if (color) {
            salida << ANSI_FIN;
        }
        salida << '\n';
    }

    // 3. Líneas de código con sus marcas
    const std::vector<Bloque> bloques = agrupar(diagnostico, fuentes);
    if (!bloques.empty()) {
        std::uint32_t ancho = 1;
        for (const Bloque& bloque : bloques) {
            ancho = std::max(ancho, ancho_de_digitos(bloque.numero));
        }

        salida << std::string(ancho, ' ') << " |\n";

        for (const Bloque& bloque : bloques) {
            const LineaVisible visible = preparar(fuentes.linea(bloque.archivo, bloque.numero));
            salida << std::setw(static_cast<int>(ancho)) << bloque.numero << " | " << visible.texto
                   << '\n';
            for (const Marca& marca : bloque.marcas) {
                escribir_marca(salida, ancho, visible, marca, color_gravedad, color);
            }
        }
        salida << std::string(ancho, ' ') << " |\n";
    }

    // 4. Notas y ayudas
    for (const std::string& nota : diagnostico.notas()) {
        salida << "   = ";
        escribir(salida, color, ANSI_CIAN, texto_fijo("nota", idioma));
        salida << ": " << nota << '\n';
    }
    for (const std::string& ayuda : diagnostico.ayudas()) {
        salida << "   = ";
        escribir(salida, color, ANSI_VERDE, texto_fijo("ayuda", idioma));
        salida << ": " << ayuda << '\n';
    }
}

void renderizar_texto(std::span<const Diagnostico> diagnosticos, const GestorFuentes& fuentes,
                      std::ostream& salida, const OpcionesRender& opciones) {
    bool primero = true;
    for (const Diagnostico& diagnostico : diagnosticos) {
        if (!primero) {
            salida << '\n';
        }
        primero = false;
        renderizar_texto(diagnostico, fuentes, salida, opciones);
    }
}

}  // namespace nooki::diagnostico
