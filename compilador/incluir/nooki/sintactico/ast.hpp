#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/ubicacion.hpp"

namespace nooki::sintactico {

/// Árbol de sintaxis de la rebanada vertical.
///
/// Es deliberadamente pequeño: solo lo necesario para compilar un programa que
/// llame a `imprimir`. Crecerá en los sprints S12–S19 hasta cubrir la gramática.

enum class TipoExpresion : unsigned char {
    Texto,
    Entero,
    Identificador,
    Llamada,
};

struct Expresion;
using ExpresionPtr = std::unique_ptr<Expresion>;

struct Expresion {
    TipoExpresion tipo = TipoExpresion::Entero;

    /// Lexema original: el texto sin comillas, el número o el nombre.
    std::string_view lexema;

    /// Solo para llamadas.
    ExpresionPtr destino;
    std::vector<ExpresionPtr> argumentos;

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
