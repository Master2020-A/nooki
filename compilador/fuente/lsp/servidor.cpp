#include "nooki/lsp/servidor.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "nooki/diagnostico/catalogo.hpp"
#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/formato/formateador.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/lsp/json.hpp"
#include "nooki/semantico/verificaciones.hpp"
#include "nooki/sintactico/parser.hpp"

namespace nooki::lsp {
namespace {

/// Lee un mensaje del protocolo: cabeceras, línea en blanco y cuerpo.
bool leer_mensaje(std::istream& entrada, std::string& cuerpo) {
    std::size_t longitud = 0;
    bool hay_longitud = false;

    std::string linea;
    while (std::getline(entrada, linea)) {
        if (!linea.empty() && linea.back() == '\r') {
            linea.pop_back();
        }
        if (linea.empty()) {
            break;
        }
        const std::string_view vista(linea);
        if (vista.rfind("Content-Length:", 0) == 0) {
            longitud = static_cast<std::size_t>(
                std::strtoul(std::string(vista.substr(15)).c_str(), nullptr, 10));
            hay_longitud = true;
        }
    }

    if (!hay_longitud || longitud == 0) {
        return false;
    }

    cuerpo.resize(longitud);
    entrada.read(cuerpo.data(), static_cast<std::streamsize>(longitud));
    return entrada.gcount() == static_cast<std::streamsize>(longitud);
}

void escribir_mensaje(std::ostream& salida, const Json& mensaje) {
    const std::string cuerpo = mensaje.escribir();
    salida << "Content-Length: " << cuerpo.size() << "\r\n\r\n" << cuerpo;
    salida.flush();
}

/// Diagnósticos de un documento, ya en el formato que espera el editor.
Json diagnosticos_de(std::string_view texto) {
    diagnostico::GestorFuentes fuentes;
    diagnostico::Emisor emisor;
    const auto archivo = fuentes.anadir("documento.oki", std::string(texto));

    lexico::Lexer lexer(fuentes, archivo, emisor);
    const auto tokens = lexer.todos();
    if (!emisor.hay_errores()) {
        sintactico::Parser parser(fuentes, archivo, emisor, tokens);
        const sintactico::Programa programa = parser.analizar();
        if (!emisor.hay_errores()) {
            semantico::verificar(programa, emisor);
        }
    }

    Json lista = Json::arreglo();
    for (const diagnostico::Diagnostico& dato : emisor.diagnosticos()) {
        const auto ubicacion_inicio = fuentes.ubicar(archivo, dato.span().inicio);
        const auto ubicacion_fin = fuentes.ubicar(archivo, dato.span().fin);

        Json rango = Json::objeto();
        Json desde = Json::objeto();
        desde.poner("line", static_cast<int>(ubicacion_inicio.linea) - 1);
        desde.poner("character", static_cast<int>(ubicacion_inicio.columna) - 1);
        Json hasta = Json::objeto();
        hasta.poner("line", static_cast<int>(ubicacion_fin.linea) - 1);
        hasta.poner("character",
                    static_cast<int>(ubicacion_fin.columna) -
                        (ubicacion_fin.linea == ubicacion_inicio.linea ? 1 : 0));
        rango.poner("start", std::move(desde));
        rango.poner("end", std::move(hasta));

        Json diagnostico_json = Json::objeto();
        diagnostico_json.poner("range", std::move(rango));
        diagnostico_json.poner(
            "severity",
            dato.gravedad() == diagnostico::Gravedad::Error ? Json(1) : Json(2));
        diagnostico_json.poner("code", Json(std::string(dato.codigo())));
        diagnostico_json.poner("source", Json("nooki"));

        std::string mensaje = diagnostico::componer(dato.codigo(), dato.parametros(),
                                                    diagnostico::Idioma::Espanol);
        for (const std::string& nota : dato.notas()) {
            mensaje += "\n= nota: " + nota;
        }
        for (const std::string& ayuda : dato.ayudas()) {
            mensaje += "\n= ayuda: " + ayuda;
        }
        diagnostico_json.poner("message", Json(std::move(mensaje)));

        lista.poner({}, std::move(diagnostico_json));
    }
    return lista;
}

/// Palabras del lenguaje, para ofrecerlas al escribir.
const char* const PALABRAS[] = {
    "funcion",   "variable", "constante", "retornar", "si",       "sino",
    "mientras",  "para",     "en",        "romper",   "continuar", "coincide",
    "caso",      "clase",    "estructura", "rasgo",   "implementa", "hereda",
    "publico",   "privado",  "nuevo",     "este",     "usar",     "como",
    "externo",   "inseguro", "asm",       "verdadero", "falso",   "nulo",
    "entero",    "texto",    "flotante",  "booleano", "caracter", "vacio",
    "Vector",    "Mapa",     "Caja",      "resultado"};

/// Nombres declarados en el documento: tipos, funciones y variables conocidas.
void recoger_nombres(const sintactico::Programa& programa, std::vector<std::string>& salida) {
    for (const sintactico::DefinicionTipo& tipo : programa.tipos) {
        salida.emplace_back(tipo.nombre);
    }
    for (const sintactico::Funcion& funcion : programa.funciones) {
        if (funcion.modulo.empty()) {
            salida.emplace_back(funcion.nombre);
        }
    }
    for (const sintactico::Enumeracion& enumeracion : programa.enumeraciones) {
        salida.emplace_back(enumeracion.nombre);
    }
}

/// Palabra completa que hay en esa posición del documento.
std::string palabra_en(const std::string& texto, int linea, int columna) {
    std::size_t inicio_linea = 0;
    int linea_actual = 0;
    for (std::size_t i = 0; i < texto.size(); ++i) {
        if (texto[i] == '\n') {
            if (linea_actual == linea) {
                break;
            }
            ++linea_actual;
            inicio_linea = i + 1;
        }
    }
    if (linea_actual != linea) {
        return {};
    }
    std::size_t fin_linea = texto.find('\n', inicio_linea);
    if (fin_linea == std::string::npos) {
        fin_linea = texto.size();
    }
    const auto vale = [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9') || c == '_' || c >= 0x80;
    };
    std::size_t centro = inicio_linea + static_cast<std::size_t>(columna);
    if (centro > fin_linea) {
        return {};
    }
    std::size_t desde = centro;
    while (desde > inicio_linea && vale(static_cast<unsigned char>(texto[desde - 1]))) {
        --desde;
    }
    std::size_t hasta = centro;
    while (hasta < fin_linea && vale(static_cast<unsigned char>(texto[hasta]))) {
        ++hasta;
    }
    if (desde == hasta) {
        return {};
    }
    return texto.substr(desde, hasta - desde);
}

/// Prefijo que se está escribiendo antes del cursor.
std::string prefijo_en(const std::string& texto, int linea, int columna) {
    int linea_actual = 0;
    std::size_t inicio = 0;
    for (std::size_t i = 0; i < texto.size(); ++i) {
        if (texto[i] == '\n') {
            if (linea_actual == linea) {
                break;
            }
            ++linea_actual;
            inicio = i + 1;
        }
    }
    if (linea_actual != linea) {
        return {};
    }
    const std::size_t hasta = inicio + static_cast<std::size_t>(columna);
    if (hasta > texto.size()) {
        return {};
    }
    std::size_t desde = hasta;
    while (desde > inicio) {
        const unsigned char c = static_cast<unsigned char>(texto[desde - 1]);
        const bool vale = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                          (c >= '0' && c <= '9') || c == '_' || c >= 0x80;
        if (!vale) {
            break;
        }
        --desde;
    }
    return texto.substr(desde, hasta - desde);
}

}  // namespace

int servir(std::istream& entrada, std::ostream& salida) {
    bool terminado = false;
    std::map<std::string, std::string> documentos;
    while (!terminado) {
        std::string cuerpo;
        if (!leer_mensaje(entrada, cuerpo)) {
            break;
        }

        Json mensaje;
        if (!Json::leer(cuerpo, mensaje) || !mensaje.es_objeto()) {
            continue;
        }

        const std::string metodo = mensaje.campo("method").cadena();
        const Json& identificador = mensaje.campo("id");
        const Json& parametros = mensaje.campo("params");

        auto responder = [&](Json resultado) {
            Json respuesta = Json::objeto();
            respuesta.poner("jsonrpc", Json("2.0"));
            respuesta.poner("id", identificador);
            respuesta.poner("result", std::move(resultado));
            escribir_mensaje(salida, respuesta);
        };

        if (metodo == "initialize") {
            Json capacidades = Json::objeto();
            Json sincronizacion = Json::objeto();
            sincronizacion.poner("openClose", Json(true));
            sincronizacion.poner("change", Json(1));  // 1 = completo
            capacidades.poner("textDocumentSync", std::move(sincronizacion));
            // El carácter que dispara el autocompletado no hace falta: el editor
            // pide la lista y aquí se filtra por el prefijo.
            Json completado = Json::objeto();
            completado.poner("triggerCharacters", Json::arreglo());
            capacidades.poner("completionProvider", std::move(completado));
            capacidades.poner("definitionProvider", Json(true));
            capacidades.poner("documentFormattingProvider", Json(true));

            Json servidor = Json::objeto();
            servidor.poner("name", Json("nooki-lsp"));
            servidor.poner("version", Json("1.0.0"));

            Json resultado = Json::objeto();
            resultado.poner("capabilities", std::move(capacidades));
            resultado.poner("serverInfo", std::move(servidor));
            responder(std::move(resultado));
            continue;
        }

        if (metodo == "textDocument/didOpen" || metodo == "textDocument/didChange") {
            const Json& documento = parametros.campo("textDocument");
            std::string texto;
            if (metodo == "textDocument/didOpen") {
                texto = documento.campo("text").cadena();
            } else {
                const Json& cambios = parametros.campo("contentChanges");
                if (!cambios.elementos().empty()) {
                    texto = cambios.elementos().back().campo("text").cadena();
                }
            }

            documentos[documento.campo("uri").cadena()] = texto;

            Json aviso = Json::objeto();
            aviso.poner("jsonrpc", Json("2.0"));
            aviso.poner("method", Json("textDocument/publishDiagnostics"));
            Json contenido = Json::objeto();
            contenido.poner("uri", documento.campo("uri"));
            contenido.poner("diagnostics", diagnosticos_de(texto));
            aviso.poner("params", std::move(contenido));
            escribir_mensaje(salida, aviso);
            continue;
        }

        if (metodo == "textDocument/completion") {
            const Json& documento = parametros.campo("textDocument");
            const Json& posicion = parametros.campo("position");
            const int linea = posicion.campo("line").entero();
            const int columna = posicion.campo("character").entero();

            const std::string& uri = documento.campo("uri").cadena();
            const auto guardado = documentos.find(uri);
            if (guardado == documentos.end()) {
                responder(Json::arreglo());
                continue;
            }
            const std::string& texto = guardado->second;

            // Nombres del documento, si se puede analizar.
            std::vector<std::string> nombres;
            {
                diagnostico::GestorFuentes fuentes;
                diagnostico::Emisor emisor;
                const auto archivo = fuentes.anadir("documento.oki", texto);
                lexico::Lexer lexer(fuentes, archivo, emisor);
                const auto tokens = lexer.todos();
                if (!emisor.hay_errores()) {
                    sintactico::Parser parser(fuentes, archivo, emisor, tokens);
                    const sintactico::Programa programa = parser.analizar();
                    recoger_nombres(programa, nombres);
                }
            }

            const std::string prefijo = prefijo_en(texto, linea, columna);
            Json lista = Json::arreglo();
            auto anadir = [&](const std::string& texto_opcion, int clase) {
                if (!prefijo.empty() && texto_opcion.rfind(prefijo, 0) != 0) {
                    return;
                }
                if (prefijo.empty() && clase != 3 && clase != 14) {
                    return;  // sin prefijo solo se ofrecen palabras y funciones
                }
                Json opcion = Json::objeto();
                opcion.poner("label", Json(texto_opcion));
                opcion.poner("kind", Json(clase));
                lista.poner({}, std::move(opcion));
            };

            for (const char* palabra : PALABRAS) {
                anadir(palabra, 14);  // 14 = palabra clave
            }
            for (const std::string& nombre : nombres) {
                anadir(nombre, 3);  // 3 = función
            }

            responder(std::move(lista));
            continue;
        }

        if (metodo == "textDocument/definition") {
            const Json& documento = parametros.campo("textDocument");
            const Json& posicion = parametros.campo("position");
            const std::string& uri = documento.campo("uri").cadena();
            const auto guardado = documentos.find(uri);
            if (guardado == documentos.end()) {
                responder(Json());
                continue;
            }
            const std::string& texto = guardado->second;
            const std::string palabra = palabra_en(texto, posicion.campo("line").entero(),
                                                   posicion.campo("character").entero());
            if (palabra.empty()) {
                responder(Json());
                continue;
            }

            diagnostico::GestorFuentes fuentes;
            diagnostico::Emisor emisor;
            const auto archivo = fuentes.anadir("documento.oki", texto);
            lexico::Lexer lexer(fuentes, archivo, emisor);
            const auto tokens = lexer.todos();
            diagnostico::Span encontrado{};
            bool hay = false;
            if (!emisor.hay_errores()) {
                sintactico::Parser parser(fuentes, archivo, emisor, tokens);
                const sintactico::Programa programa = parser.analizar();
                for (const sintactico::Funcion& funcion : programa.funciones) {
                    if (funcion.nombre == palabra) {
                        encontrado = funcion.span;
                        hay = true;
                        break;
                    }
                }
                if (!hay) {
                    for (const sintactico::DefinicionTipo& tipo : programa.tipos) {
                        if (tipo.nombre == palabra) {
                            encontrado = tipo.span;
                            hay = true;
                            break;
                        }
                    }
                }
                if (!hay) {
                    for (const sintactico::Enumeracion& enumeracion : programa.enumeraciones) {
                        if (enumeracion.nombre == palabra) {
                            encontrado = enumeracion.span;
                            hay = true;
                            break;
                        }
                    }
                }
            }
            if (!hay) {
                responder(Json());
                continue;
            }

            const auto inicio = fuentes.ubicar(archivo, encontrado.inicio);
            const auto fin = fuentes.ubicar(archivo, encontrado.fin);
            Json rango = Json::objeto();
            Json desde = Json::objeto();
            desde.poner("line", static_cast<int>(inicio.linea) - 1);
            desde.poner("character", static_cast<int>(inicio.columna) - 1);
            Json hasta = Json::objeto();
            hasta.poner("line", static_cast<int>(fin.linea) - 1);
            hasta.poner("character", static_cast<int>(fin.columna) - 1);
            rango.poner("start", std::move(desde));
            rango.poner("end", std::move(hasta));

            Json ubicacion = Json::objeto();
            ubicacion.poner("uri", documento.campo("uri"));
            ubicacion.poner("range", std::move(rango));
            responder(std::move(ubicacion));
            continue;
        }

        if (metodo == "textDocument/formatting") {
            const Json& documento = parametros.campo("textDocument");
            const std::string& uri = documento.campo("uri").cadena();
            const auto guardado = documentos.find(uri);
            if (guardado == documentos.end()) {
                responder(Json());
                continue;
            }
            const std::string formateado = formato::formatear(guardado->second);
            if (formateado == guardado->second) {
                responder(Json::arreglo());
                continue;
            }

            // Una sola edición: reemplaza el documento entero.
            const int lineas = static_cast<int>(std::count(
                guardado->second.begin(), guardado->second.end(), '\n'));
            Json rango = Json::objeto();
            Json desde = Json::objeto();
            desde.poner("line", Json(0));
            desde.poner("character", Json(0));
            Json hasta = Json::objeto();
            hasta.poner("line", Json(lineas + 1));
            hasta.poner("character", Json(0));
            rango.poner("start", std::move(desde));
            rango.poner("end", std::move(hasta));

            Json edicion = Json::objeto();
            edicion.poner("range", std::move(rango));
            edicion.poner("newText", Json(formateado));

            Json lista = Json::arreglo();
            lista.poner({}, std::move(edicion));
            responder(std::move(lista));
            continue;
        }

        if (metodo == "shutdown") {
            responder(Json());
            continue;
        }

        if (metodo == "exit") {
            terminado = true;
            continue;
        }

        // Cualquier otro método con identificador recibe una respuesta nula.
        if (!identificador.es_objeto() && !identificador.cadena().empty()) {
            responder(Json());
        }
    }
    return 0;
}

}  // namespace nooki::lsp
