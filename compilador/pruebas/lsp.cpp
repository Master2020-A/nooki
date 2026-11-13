// Servidor de lenguaje: se le entregan mensajes LSP y se comprueba que responde
// lo que el protocolo espera, incluidos los diagnósticos del documento.
#include <iostream>
#include <sstream>
#include <string>

#include "nooki/lsp/json.hpp"
#include "nooki/lsp/servidor.hpp"

namespace {

int fallos = 0;

void comprobar(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        std::cout << "FALLO: " << mensaje << "\n";
        ++fallos;
    }
}

/// Envuelve un mensaje con la cabecera del protocolo.
std::string con_cabecera(const std::string& cuerpo) {
    return "Content-Length: " + std::to_string(cuerpo.size()) + "\r\n\r\n" + cuerpo;
}

/// Separa los mensajes de la salida: devuelve los cuerpos JSON.
std::vector<std::string> cuerpos_de(const std::string& salida) {
    std::vector<std::string> cuerpos;
    std::size_t posicion = 0;
    while (posicion < salida.size()) {
        const auto cabecera = salida.find("Content-Length: ", posicion);
        if (cabecera == std::string::npos) {
            break;
        }
        const auto fin_cabecera = salida.find("\r\n\r\n", cabecera);
        if (fin_cabecera == std::string::npos) {
            break;
        }
        const std::size_t longitud = static_cast<std::size_t>(
            std::stoul(salida.substr(cabecera + 16, fin_cabecera - cabecera - 16)));
        const std::size_t inicio = fin_cabecera + 4;
        cuerpos.push_back(salida.substr(inicio, longitud));
        posicion = inicio + longitud;
    }
    return cuerpos;
}

}  // namespace

int main() {
    const std::string apertura =
        R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})";
    const std::string documento =
        R"({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":)"
        R"({"uri":"file:///prueba.oki","text":"funcion principal() -> entero {\n)"
        R"(    variable a = \n    retornar 0\n}\n"}}})";
    const std::string cierre = R"({"jsonrpc":"2.0","id":2,"method":"shutdown"})";
    const std::string salida_protocolo = R"({"jsonrpc":"2.0","method":"exit"})";

    std::istringstream entrada(con_cabecera(apertura) + con_cabecera(documento) +
                               con_cabecera(cierre) + con_cabecera(salida_protocolo));
    std::ostringstream salida;

    const int codigo = nooki::lsp::servir(entrada, salida);
    comprobar(codigo == 0, "el servidor debería terminar con 0");

    const std::vector<std::string> cuerpos = cuerpos_de(salida.str());
    comprobar(cuerpos.size() == 3, "se esperaban tres mensajes: capacidades, diagnósticos y cierre");

    // 1. Respuesta a `initialize` con las capacidades de sincronización.
    if (cuerpos.size() >= 1) {
        nooki::lsp::Json respuesta;
        comprobar(nooki::lsp::Json::leer(cuerpos[0], respuesta), "la respuesta no es JSON");
        comprobar(respuesta.campo("id").entero() == 1, "la respuesta no lleva id 1");
        const auto& capacidades = respuesta.campo("result").campo("capabilities");
        comprobar(capacidades.es_objeto(), "faltan las capacidades");
        comprobar(capacidades.campo("textDocumentSync").es_objeto(),
                  "falta la sincronización de documentos");
    }

    // 2. Aviso de diagnósticos con el error de sintaxis del documento.
    if (cuerpos.size() >= 2) {
        nooki::lsp::Json aviso;
        comprobar(nooki::lsp::Json::leer(cuerpos[1], aviso), "el aviso no es JSON");
        comprobar(aviso.campo("method").cadena() == "textDocument/publishDiagnostics",
                  "no es un aviso de diagnósticos");
        const auto& diagnosticos = aviso.campo("params").campo("diagnostics");
        comprobar(diagnosticos.elementos().size() == 1, "se esperaba un diagnóstico");
        if (!diagnosticos.elementos().empty()) {
            const nooki::lsp::Json& diagnostico = diagnosticos.elementos().front();
            comprobar(diagnostico.campo("code").cadena() == "NOK2002",
                      "el código del diagnóstico no es NOK2002");
            comprobar(diagnostico.campo("severity").entero() == 1,
                      "un error debe tener severidad 1");
            const auto& inicio = diagnostico.campo("range").campo("start");
            comprobar(inicio.campo("line").entero() == 2,
                      "el diagnóstico debería empezar en la línea 2 (base 0)");
        }
    }

    // 3. Respuesta al cierre.
    if (cuerpos.size() >= 3) {
        nooki::lsp::Json respuesta;
        comprobar(nooki::lsp::Json::leer(cuerpos[2], respuesta), "la respuesta no es JSON");
        comprobar(respuesta.campo("id").entero() == 2, "la respuesta no lleva id 2");
    }

    // Y un documento sin errores no debe producir diagnósticos.
    {
        const std::string limpio =
            R"({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":)"
            R"({"uri":"file:///bien.oki","text":"funcion principal() -> entero {\n)"
            R"(    retornar 0\n}\n"}}})";
        std::istringstream entrada_limpia(con_cabecera(limpio) + con_cabecera(salida_protocolo));
        std::ostringstream salida_limpia;
        (void)nooki::lsp::servir(entrada_limpia, salida_limpia);
        const std::vector<std::string> cuerpos_limpios = cuerpos_de(salida_limpia.str());
        comprobar(!cuerpos_limpios.empty(), "un documento abierto debe producir un aviso");
        if (!cuerpos_limpios.empty()) {
            nooki::lsp::Json aviso;
            comprobar(nooki::lsp::Json::leer(cuerpos_limpios[0], aviso), "el aviso no es JSON");
            comprobar(aviso.campo("params").campo("diagnostics").elementos().empty(),
                      "un documento correcto no debe tener diagnósticos");
        }
    }

    // Autocompletado: se pide en un documento con una función declarada.
    {
        const std::string apertura_dos =
            R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})";
        const std::string abrir =
            R"({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":)"
            R"({"uri":"file:///comp.oki","text":"funcion suma(a: entero, b: entero) -> entero {\n)"
            R"(    retornar a + b\n}\n"}}})";
        const std::string pedir =
            R"({"jsonrpc":"2.0","id":5,"method":"textDocument/completion","params":{)"
            R"("textDocument":{"uri":"file:///comp.oki"},"position":{"line":3,"character":0}}})";
        const std::string cerrar = R"({"jsonrpc":"2.0","id":6,"method":"shutdown"})";

        std::istringstream entrada_dos(con_cabecera(apertura_dos) + con_cabecera(abrir) +
                                       con_cabecera(pedir) + con_cabecera(cerrar) +
                                       con_cabecera(salida_protocolo));
        std::ostringstream salida_dos;
        (void)nooki::lsp::servir(entrada_dos, salida_dos);
        const std::vector<std::string> cuerpos_dos = cuerpos_de(salida_dos.str());

        comprobar(cuerpos_dos.size() == 4, "se esperaban cuatro mensajes con la petición");
        if (cuerpos_dos.size() >= 3) {
            nooki::lsp::Json lista;
            comprobar(nooki::lsp::Json::leer(cuerpos_dos[2], lista), "la lista no es JSON");
            comprobar(lista.campo("id").entero() == 5, "la respuesta no lleva id 5");
            const auto& opciones = lista.campo("result");
            comprobar(opciones.es_arreglo(), "el resultado no es una lista");
            bool hay_palabra = false;
            bool hay_funcion = false;
            for (const auto& opcion : opciones.elementos()) {
                const std::string& etiqueta = opcion.campo("label").cadena();
                if (etiqueta == "funcion") {
                    hay_palabra = true;
                }
                if (etiqueta == "suma") {
                    hay_funcion = true;
                }
            }
            comprobar(hay_palabra, "debería ofrecer palabras del lenguaje");
            comprobar(hay_funcion, "debería ofrecer la función declarada");
        }
    }

    // Ir a la definición: `suma` se declara en la línea 0 del documento.
    {
        const std::string apertura_tres =
            R"({"jsonrpc":"2.0","id":1,"method":"initialize","params":{}})";
        const std::string abrir =
            R"({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":)"
            R"({"uri":"file:///def.oki","text":"funcion suma(a: entero) -> entero {\n)"
            R"(    retornar a\n}\n\nfuncion principal() -> entero {\n)"
            R"(    retornar suma(1)\n}\n"}}})";
        const std::string pedir =
            R"({"jsonrpc":"2.0","id":7,"method":"textDocument/definition","params":{)"
            R"("textDocument":{"uri":"file:///def.oki"},"position":{"line":5,"character":15}}})";
        const std::string cerrar = R"({"jsonrpc":"2.0","id":8,"method":"shutdown"})";

        std::istringstream entrada_tres(con_cabecera(apertura_tres) + con_cabecera(abrir) +
                                        con_cabecera(pedir) + con_cabecera(cerrar) +
                                        con_cabecera(salida_protocolo));
        std::ostringstream salida_tres;
        (void)nooki::lsp::servir(entrada_tres, salida_tres);
        const std::vector<std::string> cuerpos_tres = cuerpos_de(salida_tres.str());

        comprobar(cuerpos_tres.size() == 4, "se esperaban cuatro mensajes con la definición");

        if (cuerpos_tres.size() >= 3) {
            nooki::lsp::Json ubicacion;
            comprobar(nooki::lsp::Json::leer(cuerpos_tres[2], ubicacion), "no es JSON");
            comprobar(ubicacion.campo("id").entero() == 7, "la respuesta no lleva id 7");
            const auto& resultado = ubicacion.campo("result");
            comprobar(resultado.es_objeto(), "el resultado no es una ubicación");
            comprobar(resultado.campo("range").campo("start").campo("line").entero() == 0,
                      "la definición debería estar en la línea 0");
        }
    }

    if (fallos == 0) {
        std::cout << "lsp: capacidades, diagnósticos, autocompletado, definición y cierre\n";
        return 0;
    }
    std::cout << fallos << " comprobación(es) fallida(s)\n";
    return 1;
}
