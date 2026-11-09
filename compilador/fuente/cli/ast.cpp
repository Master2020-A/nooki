#include "nooki/cli/ast.hpp"

#include <fstream>
#include <ostream>
#include <sstream>
#include <string>

#include "nooki/cli/orden.hpp"
#include "nooki/diagnostico/render.hpp"
#include "nooki/diagnostico/terminal.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/sintactico/impresor.hpp"
#include "nooki/sintactico/parser.hpp"

namespace nooki::cli {

int comando_ast(std::span<const std::string_view> argumentos, std::ostream& salida,
                std::ostream& error) {
    if (argumentos.empty()) {
        error << "uso: nooki ast <archivo.oki>\n";
        return USO_INCORRECTO;
    }

    const std::string ruta(argumentos.front());
    std::ifstream entrada(ruta, std::ios::binary);
    if (!entrada) {
        error << "nooki: no se pudo leer «" << ruta << "»\n";
        return DATOS_INCORRECTOS;
    }
    std::ostringstream contenido;
    contenido << entrada.rdbuf();

    diagnostico::GestorFuentes fuentes;
    diagnostico::Emisor emisor;
    const auto archivo = fuentes.anadir(ruta, contenido.str());

    lexico::Lexer lexer(fuentes, archivo, emisor);
    sintactico::Parser parser(fuentes, archivo, emisor, lexer.todos());
    const sintactico::Programa programa = parser.analizar();

    if (emisor.cantidad() > 0) {
        diagnostico::OpcionesRender render;
        render.color = diagnostico::admite_color_error();
        diagnostico::renderizar_texto(emisor.diagnosticos(), fuentes, error, render);
    }
    if (emisor.hay_errores()) {
        return DATOS_INCORRECTOS;
    }

    salida << sintactico::imprimir(programa) << '\n';
    return EXITO;
}

}  // namespace nooki::cli
