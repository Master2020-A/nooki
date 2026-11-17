// Pruebas del sistema de diagnósticos: catálogo, composición de mensajes, render
// en texto y en JSON, y casos dorados (una salida esperada por diagnóstico).
//
// Los casos dorados se regeneran con `--actualizar`. Las salidas se revisan a mano
// antes de confirmarlas: un dorado que nadie lee no prueba nada.

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/catalogo.hpp"
#include "nooki/diagnostico/codigos.hpp"
#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/render.hpp"

namespace {

using namespace nooki::diagnostico;

int comprobaciones = 0;
int fallos = 0;

void comprobar(bool condicion, std::string_view descripcion) {
    ++comprobaciones;
    if (!condicion) {
        ++fallos;
        std::cerr << "    FALLO: " << descripcion << '\n';
    }
}

void caso(std::string_view titulo) {
    std::cout << "  · " << titulo << '\n';
}

std::string leer(const std::filesystem::path& ruta) {
    std::ifstream entrada(ruta, std::ios::binary);
    if (!entrada) {
        return {};
    }
    std::ostringstream contenido;
    contenido << entrada.rdbuf();
    return contenido.str();
}

void escribir(const std::filesystem::path& ruta, const std::string& contenido) {
    std::ofstream salida(ruta, std::ios::binary | std::ios::trunc);
    salida << contenido;
}

Span localizar(ArchivoId archivo, std::string_view contenido, std::string_view aguja,
               std::size_t desde = 0) {
    const std::size_t posicion = contenido.find(aguja, desde);
    if (posicion == std::string_view::npos) {
        return Span{};
    }
    return Span{archivo, static_cast<std::uint32_t>(posicion),
                static_cast<std::uint32_t>(posicion + aguja.size())};
}

/// Construye los diagnósticos de cada caso como lo haría el compilador real.
std::vector<Diagnostico> construir(std::string_view nombre, GestorFuentes& fuentes,
                                   const std::string& contenido) {
    Emisor emisor;
    const ArchivoId archivo = fuentes.anadir(std::string(nombre) + ".oki", contenido);

    if (nombre == "nok1001-tilde") {
        emisor.error(codigos::PALABRA_RESERVADA_CON_TILDE, localizar(archivo, contenido, "función"))
            .parametro("palabra", "función")
            .nota("en Nooki las palabras clave nunca llevan tilde")
            .ayuda("escriba «funcion»");
    } else if (nombre == "nok2001-parentesis") {
        const std::size_t llamada = contenido.find("imprimir(");
        emisor.error(codigos::PARENTESIS_SIN_CERRAR, localizar(archivo, contenido, "(", llamada))
            .parametro("apertura", "(")
            .nota("los paréntesis deben cerrarse en la misma expresión")
            .ayuda("añada «)» al final de la llamada");
    } else if (nombre == "nok3001-no-declarado") {
        emisor.error(codigos::NOMBRE_NO_DECLARADO, localizar(archivo, contenido, "subtotal"))
            .parametro("nombre", "subtotal")
            .ayuda("declare la variable antes de usarla: «variable subtotal = ...»");
    } else if (nombre == "nok4001-tipos") {
        emisor.error(codigos::TIPO_INCOMPATIBLE, localizar(archivo, contenido, "\"treinta\""))
            .parametro("esperado", "entero")
            .parametro("encontrado", "texto")
            .etiqueta(localizar(archivo, contenido, "entero"), "el tipo declarado es «entero»")
            .ayuda("cambie el tipo a «texto» o convierta el valor con «convertir<entero>»");
    } else if (nombre == "nok5001-movido") {
        const std::size_t primera = contenido.find("datos");
        const std::size_t segunda = contenido.find("datos", primera + 1);
        const std::size_t tercera = contenido.find("datos", segunda + 1);
        emisor.error(codigos::USO_TRAS_MOVER, localizar(archivo, contenido, "datos", tercera))
            .parametro("nombre", "datos")
            .etiqueta(localizar(archivo, contenido, "datos", segunda),
                      "el valor se movió aquí al asignarlo a «copia»")
            .nota("asignar un «vector» transfiere la propiedad")
            .ayuda("use «datos.clonar()» si necesita conservar el original");
    }

    return emisor.diagnosticos();
}

struct Caso {
    std::string_view nombre;
};

constexpr Caso CASOS[] = {
    {"nok1001-tilde"}, {"nok2001-parentesis"}, {"nok3001-no-declarado"},
    {"nok4001-tipos"}, {"nok5001-movido"},
};

}  // namespace

int main(int argc, char* argv[]) {
    const bool actualizar = argc > 1 && std::string_view(argv[1]) == "--actualizar";
    const std::filesystem::path directorio = NOOKI_DIR_CASOS;

    // --- Casos dorados en texto ------------------------------------------------
    for (const Caso& caso_actual : CASOS) {
        GestorFuentes fuentes;
        const std::string contenido = leer(directorio / (std::string(caso_actual.nombre) + ".oki"));
        if (contenido.empty()) {
            comprobar(false, std::string("no se pudo leer ") + std::string(caso_actual.nombre));
            continue;
        }

        const std::vector<Diagnostico> diagnosticos =
            construir(caso_actual.nombre, fuentes, contenido);

        std::ostringstream salida;
        renderizar_texto(diagnosticos, fuentes, salida, OpcionesRender{});

        const std::filesystem::path dorado =
            directorio / (std::string(caso_actual.nombre) + ".esperado.txt");

        if (actualizar) {
            escribir(dorado, salida.str());
            std::cout << "  actualizado: " << dorado.filename().string() << '\n';
        } else {
            caso(caso_actual.nombre);
            comprobar(salida.str() == leer(dorado),
                      std::string("la salida coincide con el dorado de ") +
                          std::string(caso_actual.nombre));
        }
    }

    // --- Caso dorado en JSON ---------------------------------------------------
    {
        GestorFuentes fuentes;
        const std::string contenido = leer(directorio / "nok5001-movido.oki");
        const std::vector<Diagnostico> diagnosticos =
            construir("nok5001-movido", fuentes, contenido);

        std::ostringstream salida;
        renderizar_json(diagnosticos, fuentes, salida, OpcionesRender{});

        const std::filesystem::path dorado = directorio / "nok5001-movido.esperado.json";
        if (actualizar) {
            escribir(dorado, salida.str());
            std::cout << "  actualizado: " << dorado.filename().string() << '\n';
        } else {
            caso("json");
            comprobar(salida.str() == leer(dorado), "el JSON coincide con el dorado");
        }
    }

    // --- Comprobaciones sin dorado --------------------------------------------
    caso("catálogo");
    {
        comprobar(!plantilla_mensaje(codigos::USO_TRAS_MOVER, Idioma::Espanol).empty(),
                  "el código tiene plantilla en español");
        comprobar(componer(codigos::USO_TRAS_MOVER, {{"nombre", "datos"}}, Idioma::Ingles)
                          .find("cannot use") != std::string::npos,
                  "el catálogo traduce al inglés");
        comprobar(componer("NOK9999", {}, Idioma::Espanol).find("sin descripción") !=
                      std::string::npos,
                  "un código sin catálogo no rompe el render");
        comprobar(componer(codigos::NOMBRE_NO_DECLARADO, {{"nombre", "x"}}, Idioma::Espanol) ==
                      "no se ha declarado «x»",
                  "se sustituyen los parámetros");
    }

    caso("color");
    {
        GestorFuentes fuentes;
        const std::string contenido = leer(directorio / "nok1001-tilde.oki");
        const std::vector<Diagnostico> diagnosticos =
            construir("nok1001-tilde", fuentes, contenido);

        OpcionesRender con_color;
        con_color.color = true;
        std::ostringstream salida;
        renderizar_texto(diagnosticos, fuentes, salida, con_color);
        comprobar(salida.str().find("\x1b[1;31m") != std::string::npos,
                  "el color se emite cuando está activado");

        std::ostringstream sin_color;
        renderizar_texto(diagnosticos, fuentes, sin_color, OpcionesRender{});
        comprobar(sin_color.str().find('\x1b') == std::string::npos,
                  "sin color no hay secuencias ANSI");
    }

    caso("ubicaciones");
    {
        GestorFuentes fuentes;
        const ArchivoId archivo = fuentes.anadir("prueba.oki", "uno\ndos\ntres\n");
        comprobar(fuentes.ubicar(archivo, 0).linea == 1, "el inicio es la línea 1");
        comprobar(fuentes.ubicar(archivo, 4).linea == 2, "la segunda línea empieza en 4");
        comprobar(fuentes.ubicar(archivo, 4).columna == 1, "columna 1 al inicio de línea");
        comprobar(fuentes.numero_de_lineas(archivo) == 3, "cuenta tres líneas");
        comprobar(fuentes.linea(archivo, 2) == "dos", "extrae la línea sin salto");
    }

    std::cout << "\n  " << (comprobaciones - fallos) << "/" << comprobaciones
              << " comprobaciones correctas\n";
    return fallos == 0 ? 0 : 1;
}
