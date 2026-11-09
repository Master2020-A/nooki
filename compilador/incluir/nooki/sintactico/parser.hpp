#pragma once

#include <cstddef>
#include <vector>

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/lexico/token.hpp"
#include "nooki/sintactico/ast.hpp"

namespace nooki::sintactico {

/// Parser descendente recursivo de la rebanada vertical.
///
/// Acepta, por ahora:
///
///   funcion <nombre>() [-> <tipo>] { <sentencias> }
///
/// con sentencias de expresión (`imprimir("...")`) y `retornar`. Todo lo demás
/// produce un diagnóstico claro en lugar de un árbol inválido.
class Parser {
public:
    Parser(const diagnostico::GestorFuentes& fuentes, diagnostico::ArchivoId archivo,
           diagnostico::Emisor& emisor, std::vector<lexico::Token> tokens);

    /// Analiza el programa completo. Devuelve un árbol parcial si hubo errores.
    Programa analizar();

private:
    [[nodiscard]] const lexico::Token& actual();
    const lexico::Token& avanzar();
    bool aceptar(lexico::TipoToken tipo);
    bool aceptar_lexema(std::string_view lexema);
    void saltar_lineas();

    void error_en(const lexico::Token& token, std::string_view codigo, std::string_view esperado);

    Funcion analizar_funcion();
    Sentencia analizar_sentencia();
    ExpresionPtr analizar_expresion();
    ExpresionPtr analizar_primario();

    const diagnostico::GestorFuentes& fuentes_;
    diagnostico::ArchivoId archivo_;
    diagnostico::Emisor& emisor_;
    std::vector<lexico::Token> tokens_;
    std::size_t indice_ = 0;
};

}  // namespace nooki::sintactico
