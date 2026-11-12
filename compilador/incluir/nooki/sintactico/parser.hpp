#pragma once

#include <cstddef>
#include <vector>

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/lexico/token.hpp"
#include "nooki/sintactico/ast.hpp"

namespace nooki::sintactico {

/// Parser descendente recursivo con expresiones Pratt.
///
/// Declaraciones: `funcion <nombre>() [-> <tipo>] { ... }`.
/// Sentencias: `variable`, `constante`, `si`/`sinosi`/`sino`, `mientras`, `para`,
/// `hacer`, `bucle`, `romper`, `continuar`, `retornar`, bloques y expresiones.
/// Expresiones: todos los operadores de la especificación §7.1, con su precedencia
/// y su asociatividad exactas.
class Parser {
public:
    Parser(const diagnostico::GestorFuentes& fuentes, diagnostico::ArchivoId archivo,
           diagnostico::Emisor& emisor, std::vector<lexico::Token> tokens);

    /// Analiza el programa completo. Devuelve un árbol parcial si hubo errores.
    Programa analizar();

    /// Analiza una única expresión y exige que se consuma todo el archivo.
    /// Es el punto de entrada que usan las pruebas de precedencia.
    ExpresionPtr analizar_expresion_completa();

private:
    [[nodiscard]] const lexico::Token& actual();
    const lexico::Token& avanzar();
    bool aceptar(lexico::TipoToken tipo);
    bool aceptar_lexema(std::string_view lexema);
    void saltar_lineas();

    void error_en(const lexico::Token& token, std::string_view codigo, std::string_view esperado);

    Funcion analizar_funcion(bool externa = false);
    DefinicionTipo analizar_definicion_tipo(bool es_clase);
    /// ¿La `{` actual inicia una construcción (`punto { x: 1 }`) y no un bloque?
    bool viene_construccion();
    ExpresionPtr analizar_construccion(std::string_view tipo);
    std::vector<Sentencia> analizar_bloque();
    Sentencia analizar_sentencia();
    Sentencia analizar_declaracion(bool constante);
    Sentencia analizar_si();
    Sentencia analizar_rama_si(Sentencia sentencia);
    Sentencia analizar_mientras();
    Sentencia analizar_para();
    Sentencia analizar_hacer();
    Sentencia analizar_bucle();
    Sentencia analizar_coincide();
    Enumeracion analizar_enumeracion();
    Rasgo analizar_rasgo();
    Patron analizar_patron();

    /// Analiza una expresión cuyos operadores tengan al menos `potencia_minima`.
    ExpresionPtr expresion(int potencia_minima);
    ExpresionPtr prefijo();
    ExpresionPtr llamada(ExpresionPtr destino);
    ExpresionPtr indice(ExpresionPtr objeto);
    ExpresionPtr miembro(ExpresionPtr objeto, bool opcional);
    std::string analizar_tipo();

    /// Si el objeto seguido de `<tipos>(` es una llamada genérica, consume los
    /// tipos y los deja en el nodo. Devuelve verdadero si los consumió.
    bool intentar_tipos_aplicados(ExpresionPtr& nodo);

    /// Analiza un tipo y devuelve su texto tal cual aparece en el fuente.
    /// Admite `&`, `&mutable`, `*` y genéricos `Caja<entero>`.
    std::string_view analizar_tipo_texto();

    const diagnostico::GestorFuentes& fuentes_;
    diagnostico::ArchivoId archivo_;
    diagnostico::Emisor& emisor_;
    std::vector<lexico::Token> tokens_;
    std::size_t indice_ = 0;

    /// Verdadero si entre el token anterior y el actual había un salto de línea.
    /// Es lo que distingue `retornar` de `retornar valor`.
    bool salto_previo_ = false;
};

}  // namespace nooki::sintactico
