#include "nooki/sintactico/impresor.hpp"

#include <string>

namespace nooki::sintactico {
namespace {

std::string entre_parentesis(std::string_view operador, const Expresion& izquierda,
                             const Expresion* derecha) {
    std::string salida = "(";
    salida += operador;
    salida += ' ';
    salida += imprimir(izquierda);
    if (derecha != nullptr) {
        salida += ' ';
        salida += imprimir(*derecha);
    }
    salida += ')';
    return salida;
}

}  // namespace

std::string imprimir(const Expresion& expresion) {
    switch (expresion.tipo) {
        case TipoExpresion::Texto:
            return "\"" + std::string(expresion.lexema) + "\"";
        case TipoExpresion::Caracter:
            return "'" + std::string(expresion.lexema) + "'";
        case TipoExpresion::Entero:
        case TipoExpresion::Flotante:
        case TipoExpresion::Booleano:
        case TipoExpresion::Nulo:
        case TipoExpresion::Identificador:
            return std::string(expresion.lexema);

        case TipoExpresion::Unaria:
            return entre_parentesis(expresion.lexema, *expresion.izquierda, nullptr);

        case TipoExpresion::Binaria:
        case TipoExpresion::Asignacion:
            return entre_parentesis(expresion.lexema, *expresion.izquierda,
                                    expresion.derecha.get());

        case TipoExpresion::Llamada: {
            std::string salida = "(llamada ";
            salida += expresion.izquierda ? imprimir(*expresion.izquierda) : "?";
            for (const ExpresionPtr& argumento : expresion.lista) {
                salida += ' ';
                salida += argumento ? imprimir(*argumento) : "?";
            }
            salida += ')';
            return salida;
        }

        case TipoExpresion::Miembro:
        case TipoExpresion::MiembroOpcional: {
            std::string salida = expresion.tipo == TipoExpresion::Miembro ? "(." : "(?.";
            salida += ' ';
            salida += expresion.izquierda ? imprimir(*expresion.izquierda) : "?";
            salida += ' ';
            salida += expresion.anotacion;
            salida += ')';
            return salida;
        }

        case TipoExpresion::Indice: {
            std::string salida = "(indice ";
            salida += expresion.izquierda ? imprimir(*expresion.izquierda) : "?";
            for (const ExpresionPtr& posicion : expresion.lista) {
                salida += ' ';
                salida += posicion ? imprimir(*posicion) : "?";
            }
            salida += ')';
            return salida;
        }

        case TipoExpresion::Propagacion:
            return "(? " + (expresion.izquierda ? imprimir(*expresion.izquierda) : "?") + ")";

        case TipoExpresion::Como:
            return "(como " + (expresion.izquierda ? imprimir(*expresion.izquierda) : "?") + ' ' +
                   expresion.anotacion + ')';
    }
    return "?";
}

}  // namespace nooki::sintactico
