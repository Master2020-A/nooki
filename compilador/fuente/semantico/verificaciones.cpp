#include "nooki/semantico/verificaciones.hpp"

#include <set>
#include <string>
#include <string_view>

#include "nooki/diagnostico/codigos.hpp"

namespace nooki::semantico {
namespace {

using sintactico::Expresion;
using sintactico::Sentencia;
using sintactico::TipoExpresion;

struct Contexto {
    const std::set<std::string_view>& funciones;
    diagnostico::Emisor& emisor;
    int errores = 0;
};

void revisar_expresion(const Expresion& expresion, Contexto& contexto);

void revisar_hijos(const Expresion& expresion, Contexto& contexto) {
    if (expresion.izquierda) {
        revisar_expresion(*expresion.izquierda, contexto);
    }
    if (expresion.derecha) {
        revisar_expresion(*expresion.derecha, contexto);
    }
    for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
        if (argumento) {
            revisar_expresion(*argumento, contexto);
        }
    }
}

void revisar_expresion(const Expresion& expresion, Contexto& contexto) {
    if (expresion.tipo == TipoExpresion::Llamada && expresion.izquierda &&
        expresion.izquierda->tipo == TipoExpresion::Identificador) {
        const std::string_view nombre = expresion.izquierda->lexema;
        const bool es_preludio = nombre == "imprimir";
        if (!es_preludio && contexto.funciones.find(nombre) == contexto.funciones.end()) {
            contexto.emisor.error(diagnostico::codigos::FUNCION_NO_DECLARADA, expresion.span)
                .parametro("nombre", std::string(nombre))
                .ayuda("compruebe el nombre o declare la función");
            ++contexto.errores;
        }
    }

    revisar_hijos(expresion, contexto);
}

void revisar_sentencia(const Sentencia& sentencia, Contexto& contexto) {
    if (sentencia.expresion) {
        revisar_expresion(*sentencia.expresion, contexto);
    }
    if (sentencia.paso) {
        revisar_expresion(*sentencia.paso, contexto);
    }
    for (const Sentencia& hija : sentencia.cuerpo) {
        revisar_sentencia(hija, contexto);
    }
    for (const Sentencia& hija : sentencia.sino) {
        revisar_sentencia(hija, contexto);
    }
}

}  // namespace

int verificar_llamadas(const sintactico::Programa& programa, diagnostico::Emisor& emisor) {
    std::set<std::string_view> funciones;
    for (const sintactico::Funcion& funcion : programa.funciones) {
        funciones.insert(funcion.nombre);
    }

    Contexto contexto{funciones, emisor, 0};
    for (const sintactico::Funcion& funcion : programa.funciones) {
        for (const Sentencia& sentencia : funcion.cuerpo) {
            revisar_sentencia(sentencia, contexto);
        }
    }
    return contexto.errores;
}

}  // namespace nooki::semantico
