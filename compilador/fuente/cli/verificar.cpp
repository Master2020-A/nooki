#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

#include "nooki/cli/comandos.hpp"
#include "nooki/cli/compilar.hpp"
#include "nooki/cli/orden.hpp"
#include "nooki/conductor/compilar.hpp"
#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/diagnostico/render.hpp"
#include "nooki/diagnostico/terminal.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/rutas.hpp"
#include "nooki/semantico/verificaciones.hpp"
#include "nooki/sintactico/parser.hpp"

namespace nooki::cli {
namespace {

}  // namespace

int comando_verificar(std::span<const std::string_view> argumentos, std::ostream& salida,
                      std::ostream& error) {
    if (argumentos.empty()) {
        error << "nooki: falta el archivo a verificar\n";
        error << "uso: nooki verificar <archivo.oki>\n";
        return USO_INCORRECTO;
    }

    const std::filesystem::path fuente = ruta_desde_utf8(argumentos.front());
    std::ifstream flujo(fuente, std::ios::binary);
    if (!flujo) {
        error << "error[NOK0001]: no se pudo leer el archivo «" << argumentos.front() << "»\n";
        return DATOS_INCORRECTOS;
    }
    std::ostringstream contenido;
    contenido << flujo.rdbuf();

    diagnostico::GestorFuentes fuentes;
    diagnostico::Emisor emisor;
    const auto archivo = fuentes.anadir(ruta_a_utf8(fuente), contenido.str());

    // Solo el front end: léxico, parser y verificación. No se genera C ni se
    // invoca al compilador: es lo que hace útil a `verificar` en la integración
    // continua, donde no hace falta un binario.
    lexico::Lexer lexer(fuentes, archivo, emisor);
    const auto tokens = lexer.todos();
    if (!emisor.hay_errores()) {
        sintactico::Parser parser(fuentes, archivo, emisor, tokens);
        const sintactico::Programa programa = parser.analizar();
        if (!emisor.hay_errores()) {
            semantico::verificar(programa, emisor);
        }
    }

    if (emisor.cantidad() > 0) {
        diagnostico::OpcionesRender render;
        render.color = diagnostico::admite_color_error();
        diagnostico::renderizar_texto(emisor.diagnosticos(), fuentes, error, render);
    }

    if (emisor.hay_errores()) {
        return DATOS_INCORRECTOS;
    }
    salida << "correcto: " << argumentos.front() << '\n';
    return EXITO;
}

}  // namespace nooki::cli
