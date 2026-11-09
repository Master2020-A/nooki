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

void agregar_sentencias(std::string& salida, const std::vector<Sentencia>& sentencias) {
    for (const Sentencia& sentencia : sentencias) {
        salida += ' ';
        salida += imprimir(sentencia);
    }
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

        case TipoExpresion::Construccion: {
            std::string salida = "(construir ";
            salida += expresion.anotacion;
            for (std::size_t i = 0; i < expresion.lista.size(); ++i) {
                salida += " (";
                salida += i < expresion.nombres_campo.size() ? expresion.nombres_campo[i] : "?";
                salida += ' ';
                salida += expresion.lista[i] ? imprimir(*expresion.lista[i]) : "?";
                salida += ')';
            }
            salida += ')';
            return salida;
        }
    }
    return "?";
}

std::string imprimir(const Sentencia& sentencia) {
    switch (sentencia.tipo) {
        case TipoSentencia::Expresion:
            return sentencia.expresion ? imprimir(*sentencia.expresion) : "(?)";

        case TipoSentencia::Retornar:
            return sentencia.expresion ? "(retornar " + imprimir(*sentencia.expresion) + ")"
                                       : "(retornar)";

        case TipoSentencia::Variable:
        case TipoSentencia::Constante: {
            std::string salida =
                sentencia.tipo == TipoSentencia::Variable ? "(variable " : "(constante ";
            salida += sentencia.nombre;
            if (!sentencia.tipo_declarado.empty()) {
                salida += ' ';
                salida += sentencia.tipo_declarado;
            }
            if (sentencia.expresion) {
                salida += ' ';
                salida += imprimir(*sentencia.expresion);
            }
            salida += ')';
            return salida;
        }

        case TipoSentencia::Si: {
            std::string salida = "(si ";
            salida += sentencia.expresion ? imprimir(*sentencia.expresion) : "?";
            agregar_sentencias(salida, sentencia.cuerpo);
            if (!sentencia.sino.empty()) {
                salida += " (sino";
                agregar_sentencias(salida, sentencia.sino);
                salida += ')';
            }
            salida += ')';
            return salida;
        }

        case TipoSentencia::Mientras: {
            std::string salida = "(mientras ";
            salida += sentencia.expresion ? imprimir(*sentencia.expresion) : "?";
            agregar_sentencias(salida, sentencia.cuerpo);
            salida += ')';
            return salida;
        }

        case TipoSentencia::Para: {
            std::string salida = "(para ";
            salida += sentencia.nombre;
            salida += " en ";
            salida += sentencia.expresion ? imprimir(*sentencia.expresion) : "?";
            if (sentencia.paso) {
                salida += " paso ";
                salida += imprimir(*sentencia.paso);
            }
            agregar_sentencias(salida, sentencia.cuerpo);
            salida += ')';
            return salida;
        }

        case TipoSentencia::Hacer: {
            std::string salida = "(hacer";
            agregar_sentencias(salida, sentencia.cuerpo);
            salida += " mientras ";
            salida += sentencia.paso ? imprimir(*sentencia.paso) : "?";
            salida += ')';
            return salida;
        }

        case TipoSentencia::Bucle: {
            std::string salida = "(bucle";
            agregar_sentencias(salida, sentencia.cuerpo);
            salida += ')';
            return salida;
        }

        case TipoSentencia::Romper:
            return "(romper)";

        case TipoSentencia::Continuar:
            return "(continuar)";

        case TipoSentencia::Bloque: {
            std::string salida = "(bloque";
            agregar_sentencias(salida, sentencia.cuerpo);
            salida += ')';
            return salida;
        }
    }
    return "?";
}

std::string imprimir(const Funcion& funcion) {
    std::string salida = "(funcion ";
    salida += funcion.nombre;
    if (!funcion.parametros.empty()) {
        salida += " (";
        bool primero = true;
        for (const Parametro& parametro : funcion.parametros) {
            if (!primero) {
                salida += ", ";
            }
            primero = false;
            salida += parametro.nombre;
            salida += ' ';
            salida += parametro.tipo;
        }
        salida += ')';
    }
    salida += " -> ";
    salida += funcion.tipo_retorno;
    agregar_sentencias(salida, funcion.cuerpo);
    salida += ')';
    return salida;
}

std::string imprimir(const DefinicionTipo& definicion) {
    std::string salida = definicion.es_clase ? "(clase " : "(estructura ";
    salida += definicion.nombre;
    for (const Campo& campo : definicion.campos) {
        salida += " (";
        salida += campo.nombre;
        salida += ' ';
        salida += campo.tipo;
        salida += ')';
    }
    salida += ')';
    return salida;
}

std::string imprimir(const Programa& programa) {
    std::string salida = "(programa";
    for (const DefinicionTipo& definicion : programa.tipos) {
        salida += ' ';
        salida += imprimir(definicion);
    }
    for (const Funcion& funcion : programa.funciones) {
        salida += ' ';
        salida += imprimir(funcion);
    }
    salida += ')';
    return salida;
}

}  // namespace nooki::sintactico
