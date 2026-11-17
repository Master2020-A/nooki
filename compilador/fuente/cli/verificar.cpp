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
namespace {}  // namespace

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

    // Se usa el conductor para que cargue los módulos igual que al compilar: si
    // no, cualquier programa que use la biblioteca daría falsos errores.
    conductor::Opciones opciones;
    opciones.fuente = fuente;
    opciones.solo_verificar = true;

    diagnostico::GestorFuentes fuentes;
    diagnostico::Emisor emisor;
    std::ostringstream descartado;
    const conductor::Resultado resultado =
        conductor::compilar(opciones, fuentes, emisor, descartado, descartado);

    if (emisor.cantidad() > 0) {
        diagnostico::OpcionesRender render;
        render.color = diagnostico::admite_color_error();
        diagnostico::renderizar_texto(emisor.diagnosticos(), fuentes, error, render);
    }

    if (resultado.codigo != 0 || emisor.hay_errores()) {
        return DATOS_INCORRECTOS;
    }
    salida << "correcto: " << argumentos.front() << '\n';
    return EXITO;
}

}  // namespace nooki::cli
