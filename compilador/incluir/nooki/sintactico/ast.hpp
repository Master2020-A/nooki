#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/ubicacion.hpp"

namespace nooki::sintactico {

/// Clase de un nodo de expresión.
///
/// La forma canónica de cada nodo la define el impresor (`impresor.hpp`), que es
/// lo que permite probar la precedencia y la asociatividad comparando texto.
enum class TipoExpresion : unsigned char {
    Texto,
    Caracter,
    Entero,
    Flotante,
    Booleano,
    Nulo,
    Identificador,
    Unaria,
    Binaria,
    Llamada,
    Miembro,
    MiembroOpcional,
    Indice,
    Propagacion,
    Como,
    Asignacion,
};

struct Expresion;
using ExpresionPtr = std::unique_ptr<Expresion>;

struct Expresion {
    TipoExpresion tipo = TipoExpresion::Entero;

    /// Operador (`+`, `&&`, `=`, `..`) o literal.
    std::string_view lexema;

    /// Tipo en `como`, o nombre del miembro en un acceso.
    std::string anotacion;

    /// Operando, objeto o destino, según el nodo.
    ExpresionPtr izquierda;

    /// Segundo operando o valor asignado.
    ExpresionPtr derecha;

    /// Argumentos de llamada o índice.
    std::vector<ExpresionPtr> lista;

    diagnostico::Span span;
};

enum class TipoSentencia : unsigned char {
    Expresion,
    Retornar,
};

struct Sentencia {
    TipoSentencia tipo = TipoSentencia::Expresion;
    ExpresionPtr expresion;  // puede ser nula en `retornar` sin valor
    diagnostico::Span span;
};

struct Funcion {
    std::string_view nombre;
    /// Tipo de retorno tal como se escribió (`vacio` si no se indicó).
    std::string_view tipo_retorno;
    std::vector<Sentencia> cuerpo;
    diagnostico::Span span;
};

struct Programa {
    std::vector<Funcion> funciones;
};

}  // namespace nooki::sintactico
