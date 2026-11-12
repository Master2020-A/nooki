#include "nooki/cli/compilar.hpp"
#include "nooki/rutas.hpp"

#include <cstddef>
#include <ostream>
#include <string>

#include "nooki/cli/orden.hpp"
#include "nooki/conductor/compilar.hpp"
#include "nooki/diagnostico/render.hpp"
#include "nooki/diagnostico/terminal.hpp"
#include "nooki/proceso.hpp"

namespace nooki::cli {
namespace {

void imprimir_uso(std::ostream& error) {
    error << "uso: nooki compilar <archivo.oki> [-o <salida>] [--emitir-c] [--conservar]\n"
             "  -o <salida>   ruta del ejecutable (por defecto, el nombre del fuente)\n"
             "  --emitir-c    muestra el C generado y no compila\n"
             "  --conservar   conserva el archivo .c intermedio\n";
}

int compilar_archivo(std::span<const std::string_view> argumentos, std::ostream& salida,
                     std::ostream& error, bool ejecutar_despues) {
    conductor::Opciones opciones;
    bool hay_fuente = false;

    for (std::size_t i = 0; i < argumentos.size(); ++i) {
        const std::string_view argumento = argumentos[i];

        if (argumento == "-o" || argumento == "--salida") {
            if (i + 1 >= argumentos.size()) {
                error << "nooki: falta la ruta después de «" << argumento << "»\n";
                return USO_INCORRECTO;
            }
            ++i;
            opciones.salida = ruta_desde_utf8(argumentos[i]);
        } else if (argumento == "--emitir-c") {
            opciones.solo_c = true;
        } else if (argumento == "--conservar") {
            opciones.conservar_intermedios = true;
        } else if (!argumento.empty() && argumento.front() == '-') {
            error << "nooki: la opción «" << argumento << "» no existe\n";
            return USO_INCORRECTO;
        } else if (!hay_fuente) {
            opciones.fuente = ruta_desde_utf8(argumento);
            hay_fuente = true;
        } else {
            error << "nooki: sobra el argumento «" << argumento << "»\n";
            return USO_INCORRECTO;
        }
    }

    if (!hay_fuente) {
        imprimir_uso(error);
        return USO_INCORRECTO;
    }

    diagnostico::GestorFuentes fuentes;
    diagnostico::Emisor emisor;

    const conductor::Resultado resultado =
        conductor::compilar(opciones, fuentes, emisor, salida, error);

    if (emisor.cantidad() > 0) {
        diagnostico::OpcionesRender render;
        render.color = diagnostico::admite_color_error();
        diagnostico::renderizar_texto(emisor.diagnosticos(), fuentes, error, render);
    }

    if (resultado.codigo != 0 || !ejecutar_despues || resultado.binario.empty()) {
        return resultado.codigo;
    }

    // `make_preferred` convierte «/» en «\» en Windows: la consola no entiende «/».
    std::filesystem::path binario = resultado.binario;
    return nooki::ejecutar_comando(
        nooki::entrecomillar_si_hace_falta(ruta_a_utf8(binario.make_preferred())));
}

}  // namespace

int comando_compilar(std::span<const std::string_view> argumentos, std::ostream& salida,
                     std::ostream& error) {
    return compilar_archivo(argumentos, salida, error, false);
}

/// `nooki construir`: compila el proyecto del directorio actual.
int comando_construir(std::span<const std::string_view> argumentos, std::ostream& salida,
                      std::ostream& error) {
    // Entrada: `principal.oki` junto al manifiesto, o dentro de `fuente/`.
    const std::string fuente =
        std::filesystem::exists("principal.oki")
            ? std::string("principal.oki")
            : std::string("fuente/principal.oki");
    if (!std::filesystem::exists(fuente)) {
        error << "error[NOK0001]: no se encontró el punto de entrada del proyecto\n"
                 "   = ayuda: cree «principal.oki» o «fuente/principal.oki»\n";
        return USO_INCORRECTO;
    }

    std::vector<std::string_view> nuevos;
    nuevos.push_back(fuente);
    for (const std::string_view argumento : argumentos) {
        nuevos.push_back(argumento);
    }
    return comando_compilar(nuevos, salida, error);
}

int comando_ejecutar(std::span<const std::string_view> argumentos, std::ostream& salida,
                     std::ostream& error) {
    return compilar_archivo(argumentos, salida, error, true);
}

}  // namespace nooki::cli
