#include "nooki/sintactico/parser.hpp"

#include <string>
#include <utility>

#include "nooki/diagnostico/codigos.hpp"
#include "nooki/lexico/token.hpp"

namespace nooki::sintactico {

using lexico::TipoToken;
using lexico::Token;

Parser::Parser(const diagnostico::GestorFuentes& fuentes, diagnostico::ArchivoId archivo,
               diagnostico::Emisor& emisor, std::vector<Token> tokens)
    : fuentes_(fuentes), archivo_(archivo), emisor_(emisor), tokens_(std::move(tokens)) {}

void Parser::saltar_lineas() {
    while (indice_ < tokens_.size() && tokens_[indice_].es(TipoToken::NuevaLinea)) {
        ++indice_;
    }
}

const Token& Parser::actual() {
    saltar_lineas();
    return tokens_[indice_ < tokens_.size() ? indice_ : tokens_.size() - 1];
}

const Token& Parser::avanzar() {
    const Token& token = actual();
    if (indice_ + 1 < tokens_.size()) {
        ++indice_;
    }
    return token;
}

bool Parser::aceptar(TipoToken tipo) {
    if (!actual().es(tipo)) {
        return false;
    }
    avanzar();
    return true;
}

bool Parser::aceptar_lexema(std::string_view lexema) {
    if (actual().lexema != lexema) {
        return false;
    }
    avanzar();
    return true;
}

void Parser::error_en(const Token& token, std::string_view codigo, std::string_view esperado) {
    auto& diagnostico = emisor_.error(codigo, token.span);
    diagnostico.parametro("esperado", std::string(esperado));
    diagnostico.parametro("encontrado", token.lexema.empty()
                                            ? std::string(lexico::nombre_tipo(token.tipo))
                                            : std::string(token.lexema));
}

Programa Parser::analizar() {
    Programa programa;

    for (;;) {
        const Token& token = actual();
        if (token.es(TipoToken::FinDeArchivo)) {
            break;
        }
        if (token.es(TipoToken::PalabraClave) && token.lexema == "funcion") {
            programa.funciones.push_back(analizar_funcion());
            continue;
        }
        error_en(token, diagnostico::codigos::TOKEN_INESPERADO, "«funcion»");
        avanzar();
    }

    return programa;
}

Funcion Parser::analizar_funcion() {
    Funcion funcion;
    funcion.span = actual().span;
    avanzar();  // "funcion"

    if (const Token& nombre = actual(); nombre.es(TipoToken::Identificador)) {
        funcion.nombre = nombre.lexema;
        funcion.span.fin = nombre.span.fin;
        avanzar();
    } else {
        error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de función");
    }

    if (!aceptar_lexema("(")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«(»");
    }
    if (!aceptar_lexema(")")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
    }

    funcion.tipo_retorno = "vacio";
    if (aceptar_lexema("->")) {
        if (const Token& tipo = actual();
            tipo.es(TipoToken::PalabraClave) || tipo.es(TipoToken::Identificador)) {
            funcion.tipo_retorno = tipo.lexema;
            avanzar();
        } else {
            error_en(tipo, diagnostico::codigos::TOKEN_INESPERADO, "un tipo");
        }
    }

    if (!aceptar_lexema("{")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«{»");
        return funcion;
    }

    while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != "}") {
        funcion.cuerpo.push_back(analizar_sentencia());
    }

    if (!aceptar_lexema("}")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«}»");
    }
    return funcion;
}

Sentencia Parser::analizar_sentencia() {
    Sentencia sentencia;
    sentencia.span = actual().span;

    if (actual().es(TipoToken::PalabraClave) && actual().lexema == "retornar") {
        sentencia.tipo = TipoSentencia::Retornar;
        avanzar();
        const Token& siguiente = actual();
        const bool hay_valor = !siguiente.es(TipoToken::NuevaLinea) &&
                               siguiente.lexema != "}" && !siguiente.es(TipoToken::FinDeArchivo);
        if (hay_valor) {
            sentencia.expresion = analizar_expresion();
        }
        return sentencia;
    }

    if (actual().es(TipoToken::PalabraClave)) {
        auto& diagnostico = emisor_.error(diagnostico::codigos::NO_SOPORTADO, actual().span);
        diagnostico.parametro("construccion", std::string(actual().lexema))
            .ayuda("en esta versión solo se admiten llamadas y «retornar»");
        avanzar();
        return sentencia;
    }

    sentencia.tipo = TipoSentencia::Expresion;
    sentencia.expresion = analizar_expresion();
    return sentencia;
}

ExpresionPtr Parser::analizar_expresion() {
    ExpresionPtr expresion = analizar_primario();
    if (!expresion) {
        return nullptr;
    }

    while (actual().lexema == "(") {
        avanzar();  // "("
        auto llamada = std::make_unique<Expresion>();
        llamada->tipo = TipoExpresion::Llamada;
        llamada->span = expresion->span;
        llamada->destino = std::move(expresion);

        while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != ")") {
            ExpresionPtr argumento = analizar_expresion();
            if (!argumento) {
                break;
            }
            llamada->argumentos.push_back(std::move(argumento));
            if (actual().lexema == ",") {
                avanzar();
                continue;
            }
            break;
        }

        if (!aceptar_lexema(")")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
        }
        llamada->span.fin = actual().span.fin;
        expresion = std::move(llamada);
    }

    return expresion;
}

ExpresionPtr Parser::analizar_primario() {
    const Token& token = actual();

    auto crear = [&token](TipoExpresion tipo) {
        auto expresion = std::make_unique<Expresion>();
        expresion->tipo = tipo;
        expresion->lexema = token.lexema;
        expresion->span = token.span;
        return expresion;
    };

    switch (token.tipo) {
        case TipoToken::Texto: {
            std::string_view texto = token.lexema;
            if (texto.size() >= 2) {
                texto = texto.substr(1, texto.size() - 2);  // sin comillas
            }
            auto expresion = crear(TipoExpresion::Texto);
            expresion->lexema = texto;
            avanzar();
            return expresion;
        }
        case TipoToken::Entero:
            avanzar();
            return crear(TipoExpresion::Entero);
        case TipoToken::Identificador:
            avanzar();
            return crear(TipoExpresion::Identificador);
        default:
            error_en(token, diagnostico::codigos::TOKEN_INESPERADO, "una expresión");
            avanzar();
            return nullptr;
    }
}

}  // namespace nooki::sintactico
