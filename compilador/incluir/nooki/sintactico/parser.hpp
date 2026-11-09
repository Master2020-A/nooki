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
/// Sentencias: expresión y `retornar`.
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

    Funcion analizar_funcion();
    Sentencia analizar_sentencia();

    /// Núcleo Pratt: analiza una expresión cuyos operadores tengan al menos
    /// `potencia_minima`.
    ExpresionPtr expresion(int potencia_minima);
    ExpresionPtr prefijo();
    ExpresionPtr llamada(ExpresionPtr destino);
    ExpresionPtr indice(ExpresionPtr objeto);
    ExpresionPtr miembro(ExpresionPtr objeto, bool opcional);
    std::string analizar_tipo();

    const diagnostico::GestorFuentes& fuentes_;
    diagnostico::ArchivoId archivo_;
    diagnostico::Emisor& emisor_;
    std::vector<lexico::Token> tokens_;
    std::size_t indice_ = 0;
};

}  // namespace nooki::sintactico
