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
    Construccion,
    Este,
    Super,
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

    /// Argumentos de llamada, índice o valores de una construcción.
    std::vector<ExpresionPtr> lista;

    /// Nombres de los campos en una construcción (`punto { x: 1, y: 2 }`).
    std::vector<std::string_view> nombres_campo;

    /// Argumentos de tipo en una llamada genérica: `identidad<entero>(42)`.
    std::vector<std::string_view> tipos_aplicados;

    /// La construcción va al montículo (`nuevo persona { ... }`).
    bool monticulo = false;

    diagnostico::Span span;
};

/// Clase de una sentencia.
enum class TipoSentencia : unsigned char {
    Expresion,
    Retornar,
    Variable,
    Constante,
    Si,
    Mientras,
    Para,
    Hacer,
    Bucle,
    Romper,
    Continuar,
    Bloque,
    Coincide,
    /// `inseguro { ... }`: dentro no se comprueba la aritmética.
    Inseguro,
    /// `asm!("...")`: ensamblador en línea, solo dentro de `inseguro`.
    Asm,
};

/// Un patrón de `coincide`: comodín, literal o variante de enumeración.
struct Patron {
    enum class Clase : unsigned char { Comodin, Literal, Variante };

    Clase clase = Clase::Comodin;
    /// Literal tal cual, o nombre de la variante.
    std::string_view valor;
    /// Nombre de la enumeración cuando el patrón es una variante.
    std::string_view enumeracion;
    /// Nombres ligados a los campos de la variante: `caso Bien(v)`.
    std::vector<std::string_view> ligaduras;
    diagnostico::Span span;
};

struct Sentencia;

struct Rama {
    Patron patron;
    std::vector<Sentencia> cuerpo;
};

struct Sentencia {
    TipoSentencia tipo = TipoSentencia::Expresion;

    /// Expresión de la sentencia: valor inicial, condición o expresión suelta.
    ExpresionPtr expresion;

    /// Paso de `para` y condición final de `hacer`.
    ExpresionPtr paso;

    /// Nombre declarado o variable de iteración.
    std::string_view nombre;

    /// En `para (i, x en coleccion)`, el nombre del índice. Vacío si no lo hay.
    std::string_view indice;

    /// Tipo escrito en la declaración, si lo hay.
    std::string_view tipo_declarado;

    /// Cuerpo del bloque o del bucle.
    std::vector<Sentencia> cuerpo;

    /// Rama `sino`. Un `sinosi` se representa como un `Si` anidado aquí dentro.
    std::vector<Sentencia> sino;

    /// Ramas de un `coincide`.
    std::vector<Rama> ramas;

    diagnostico::Span span;
};

/// Un parámetro de función: `nombre: tipo`.
struct Parametro {
    std::string_view nombre;
    std::string_view tipo;
    diagnostico::Span span;
};

struct Funcion {
    std::string_view nombre;

    /// Módulo al que pertenece (vacío = archivo principal).
    std::string_view modulo;

    /// Visibilidad para otros módulos. Por defecto, privada.
    bool publico = false;

    /// `externo funcion`: declara una función de C, sin cuerpo.
    bool es_externa = false;

    /// Parámetros de tipo de una función genérica: `funcion identidad<T>(...)`.
    std::vector<std::string_view> tipo_parametros;

    std::vector<Parametro> parametros;
    /// Tipo de retorno tal como se escribió (`vacio` si no se indicó).
    std::string_view tipo_retorno;
    std::vector<Sentencia> cuerpo;
    diagnostico::Span span;
};

/// Un campo declarado en una estructura o clase.
struct Campo {
    std::string_view nombre;
    std::string_view tipo;
    bool publico = false;
    diagnostico::Span span;
};

/// `estructura` o `clase`. En la v0.1 comparten representación (ADR-015).
struct DefinicionTipo {
    std::string_view nombre;

    /// Parámetros de tipo de un tipo genérico: `estructura par<T>`.
    std::vector<std::string_view> tipo_parametros;
    /// Módulo al que pertenece (vacío = archivo principal).
    std::string_view modulo;
    bool es_clase = false;
    /// Visibilidad para otros módulos.
    bool publico = false;
    /// Clase base (`clase cuadrado hereda figura`), vacío si no hereda.
    std::string_view base;
    /// Rasgos que implementa (`clase persona implementa Describible`).
    std::vector<std::string_view> rasgos;
    std::vector<Campo> campos;
    std::vector<Funcion> metodos;
    diagnostico::Span span;
};

/// Una variante de enumeración, con o sin datos: `Bien(valor: entero)`.
struct Variante {
    std::string_view nombre;
    std::vector<Campo> campos;
    diagnostico::Span span;
};

/// `enumeracion color { rojo, verde, azul }`.
struct Enumeracion {
    std::string_view nombre;
    std::string_view modulo;
    bool publico = false;
    std::vector<Variante> variantes;
    diagnostico::Span span;
};

/// `rasgo Describible { funcion describir() -> texto }`.
struct Rasgo {
    std::string_view nombre;
    std::vector<Funcion> metodos;
    diagnostico::Span span;
};

struct Programa {
    std::vector<Rasgo> rasgos;
    std::vector<Funcion> funciones;
    std::vector<DefinicionTipo> tipos;
    std::vector<Enumeracion> enumeraciones;

    /// Módulos que este archivo importa (`usar nombre`).
    std::vector<std::string_view> usos;
};

}  // namespace nooki::sintactico
