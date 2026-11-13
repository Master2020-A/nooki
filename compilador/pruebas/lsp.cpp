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

    if (fallos == 0) {
        std::cout << "lsp: capacidades, diagnósticos y cierre correctos\n";
        return 0;
    }
    std::cout << fallos << " comprobación(es) fallida(s)\n";
    return 1;
}
