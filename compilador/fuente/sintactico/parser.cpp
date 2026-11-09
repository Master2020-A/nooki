#include "nooki/sintactico/parser.hpp"

#include <string>
#include <utility>

#include "nooki/diagnostico/codigos.hpp"
#include "nooki/lexico/token.hpp"

namespace nooki::sintactico {

using lexico::TipoToken;
using lexico::Token;

namespace {

// Potencias de enlace: mayor número = se une más fuerte (especificación §7.1).
constexpr int POTENCIA_ASIGNACION = 10;
constexpr int POTENCIA_COALESCENCIA = 30;
constexpr int POTENCIA_O = 40;
constexpr int POTENCIA_Y = 50;
constexpr int POTENCIA_COMPARACION = 60;
constexpr int POTENCIA_O_BITS = 70;
constexpr int POTENCIA_XOR = 75;
constexpr int POTENCIA_Y_BITS = 80;
constexpr int POTENCIA_DESPLAZAMIENTO = 90;
constexpr int POTENCIA_RANGO = 100;
constexpr int POTENCIA_SUMA = 110;
constexpr int POTENCIA_PRODUCTO = 120;
constexpr int POTENCIA_POTENCIA = 130;
constexpr int POTENCIA_COMO = 135;
constexpr int POTENCIA_UNARIA = 140;

struct Enlace {
    int potencia = 0;
    bool derecha = false;
};

Enlace enlace_de(std::string_view operador) {
    if (operador == "=" || operador == "+=" || operador == "-=" || operador == "*=" ||
        operador == "/=" || operador == "%=" || operador == "&=" || operador == "|=" ||
        operador == "^=" || operador == "<<=" || operador == ">>=") {
        return {POTENCIA_ASIGNACION, true};
    }
    if (operador == "??") {
        return {POTENCIA_COALESCENCIA, true};
    }
    if (operador == "||") {
        return {POTENCIA_O, false};
    }
    if (operador == "&&") {
        return {POTENCIA_Y, false};
    }
    if (operador == "==" || operador == "!=" || operador == "<" || operador == ">" ||
        operador == "<=" || operador == ">=") {
        return {POTENCIA_COMPARACION, false};
    }
    if (operador == "|") {
        return {POTENCIA_O_BITS, false};
    }
    if (operador == "^") {
        return {POTENCIA_XOR, false};
    }
    if (operador == "&") {
        return {POTENCIA_Y_BITS, false};
    }
    if (operador == "<<" || operador == ">>") {
        return {POTENCIA_DESPLAZAMIENTO, false};
    }
    if (operador == ".." || operador == "..=") {
        return {POTENCIA_RANGO, false};
    }
    if (operador == "+" || operador == "-") {
        return {POTENCIA_SUMA, false};
    }
    if (operador == "*" || operador == "/" || operador == "%") {
        return {POTENCIA_PRODUCTO, false};
    }
    if (operador == "**") {
        return {POTENCIA_POTENCIA, true};
    }
    if (operador == "como") {
        return {POTENCIA_COMO, false};
    }
    return {};
}

bool es_asignacion(std::string_view operador) {
    return enlace_de(operador).potencia == POTENCIA_ASIGNACION;
}

diagnostico::Span unir(diagnostico::Span primero, diagnostico::Span segundo) {
    primero.fin = segundo.fin;
    return primero;
}

std::string_view sin_comillas(std::string_view lexema) {
    if (lexema.size() >= 2) {
        return lexema.substr(1, lexema.size() - 2);
    }
    return lexema;
}

}  // namespace

Parser::Parser(const diagnostico::GestorFuentes& fuentes, diagnostico::ArchivoId archivo,
               diagnostico::Emisor& emisor, std::vector<Token> tokens)
    : fuentes_(fuentes), archivo_(archivo), emisor_(emisor), tokens_(std::move(tokens)) {}

void Parser::saltar_lineas() {
    while (indice_ < tokens_.size() && tokens_[indice_].es(TipoToken::NuevaLinea)) {
        ++indice_;
        salto_previo_ = true;
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
    salto_previo_ = false;
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

// ---------------------------------------------------------------------------
// Programa
// ---------------------------------------------------------------------------

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

    funcion.cuerpo = analizar_bloque();
    return funcion;
}

// ---------------------------------------------------------------------------
// Sentencias
// ---------------------------------------------------------------------------

std::vector<Sentencia> Parser::analizar_bloque() {
    std::vector<Sentencia> cuerpo;

    if (!aceptar_lexema("{")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«{»");
        return cuerpo;
    }

    while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != "}") {
        cuerpo.push_back(analizar_sentencia());
    }

    if (!aceptar_lexema("}")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«}»");
    }
    return cuerpo;
}

Sentencia Parser::analizar_sentencia() {
    Sentencia sentencia;
    sentencia.span = actual().span;

    if (actual().lexema == "{") {
        sentencia.tipo = TipoSentencia::Bloque;
        sentencia.cuerpo = analizar_bloque();
        return sentencia;
    }

    if (actual().es(TipoToken::PalabraClave)) {
        const std::string_view palabra = actual().lexema;

        if (palabra == "variable") {
            return analizar_declaracion(false);
        }
        if (palabra == "constante") {
            return analizar_declaracion(true);
        }
        if (palabra == "si") {
            return analizar_si();
        }
        if (palabra == "mientras") {
            return analizar_mientras();
        }
        if (palabra == "para") {
            return analizar_para();
        }
        if (palabra == "hacer") {
            return analizar_hacer();
        }
        if (palabra == "bucle") {
            return analizar_bucle();
        }
        if (palabra == "romper" || palabra == "continuar") {
            sentencia.tipo =
                palabra == "romper" ? TipoSentencia::Romper : TipoSentencia::Continuar;
            avanzar();
            return sentencia;
        }
        if (palabra == "retornar") {
            sentencia.tipo = TipoSentencia::Retornar;
            avanzar();
            const Token& siguiente = actual();
            const bool hay_valor =
                !salto_previo_ && siguiente.lexema != "}" && !siguiente.es(TipoToken::FinDeArchivo);
            if (hay_valor) {
                sentencia.expresion = expresion(0);
            }
            return sentencia;
        }
        if (palabra == "coincide") {
            auto& diagnostico = emisor_.error(diagnostico::codigos::NO_SOPORTADO, actual().span);
            diagnostico.parametro("construccion", "«coincide»")
                .ayuda("el emparejamiento de patrones llega en el Sprint 17");
            // Se consume el bloque entero para no encadenar errores en cascada.
            avanzar();
            int profundidad = 0;
            while (!actual().es(TipoToken::FinDeArchivo)) {
                if (actual().lexema == "{") {
                    ++profundidad;
                } else if (actual().lexema == "}") {
                    if (profundidad <= 0) {
                        avanzar();
                        break;
                    }
                    --profundidad;
                }
                avanzar();
            }
            return sentencia;
        }
        if (palabra != "verdadero" && palabra != "falso" && palabra != "nulo") {
            auto& diagnostico = emisor_.error(diagnostico::codigos::NO_SOPORTADO, actual().span);
            diagnostico.parametro("construccion", std::string(palabra))
                .ayuda("esta construcción todavía no se analiza");
            avanzar();
            return sentencia;
        }
    }

    sentencia.tipo = TipoSentencia::Expresion;
    sentencia.expresion = expresion(0);
    return sentencia;
}

Sentencia Parser::analizar_declaracion(bool constante) {
    Sentencia sentencia;
    sentencia.span = actual().span;
    sentencia.tipo = constante ? TipoSentencia::Constante : TipoSentencia::Variable;
    avanzar();  // "variable" | "constante"

    if (const Token& nombre = actual(); nombre.es(TipoToken::Identificador)) {
        sentencia.nombre = nombre.lexema;
        avanzar();
    } else {
        error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de variable");
        return sentencia;
    }

    if (aceptar_lexema(":")) {
        if (const Token& tipo = actual();
            tipo.es(TipoToken::PalabraClave) || tipo.es(TipoToken::Identificador)) {
            sentencia.tipo_declarado = tipo.lexema;
            avanzar();
        } else {
            error_en(tipo, diagnostico::codigos::TOKEN_INESPERADO, "un tipo");
        }
    }

    if (!aceptar_lexema("=")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«=»");
        return sentencia;
    }

    sentencia.expresion = expresion(0);
    return sentencia;
}

Sentencia Parser::analizar_si() {
    Sentencia sentencia;
    sentencia.tipo = TipoSentencia::Si;
    sentencia.span = actual().span;
    avanzar();  // "si"
    return analizar_rama_si(std::move(sentencia));
}

Sentencia Parser::analizar_rama_si(Sentencia sentencia) {
    if (!aceptar_lexema("(")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«(»");
    }
    sentencia.expresion = expresion(0);
    if (!aceptar_lexema(")")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
    }
    sentencia.cuerpo = analizar_bloque();

    if (actual().lexema == "sinosi") {
        Sentencia rama;
        rama.tipo = TipoSentencia::Si;
        rama.span = actual().span;
        avanzar();
        sentencia.sino.push_back(analizar_rama_si(std::move(rama)));
    } else if (actual().lexema == "sino") {
        avanzar();
        sentencia.sino = analizar_bloque();
    }

    return sentencia;
}

Sentencia Parser::analizar_mientras() {
    Sentencia sentencia;
    sentencia.tipo = TipoSentencia::Mientras;
    sentencia.span = actual().span;
    avanzar();  // "mientras"

    if (!aceptar_lexema("(")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«(»");
    }
    sentencia.expresion = expresion(0);
    if (!aceptar_lexema(")")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
    }
    sentencia.cuerpo = analizar_bloque();
    return sentencia;
}

Sentencia Parser::analizar_para() {
    Sentencia sentencia;
    sentencia.tipo = TipoSentencia::Para;
    sentencia.span = actual().span;
    avanzar();  // "para"

    if (!aceptar_lexema("(")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«(»");
    }

    if (const Token& variable = actual(); variable.es(TipoToken::Identificador)) {
        sentencia.nombre = variable.lexema;
        avanzar();
    } else {
        error_en(variable, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de variable");
    }

    if (!aceptar_lexema("en")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«en»");
    }
    sentencia.expresion = expresion(0);

    if (actual().lexema == "paso") {
        avanzar();
        sentencia.paso = expresion(0);
    }

    if (!aceptar_lexema(")")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
    }
    sentencia.cuerpo = analizar_bloque();
    return sentencia;
}

Sentencia Parser::analizar_hacer() {
    Sentencia sentencia;
    sentencia.tipo = TipoSentencia::Hacer;
    sentencia.span = actual().span;
    avanzar();  // "hacer"

    sentencia.cuerpo = analizar_bloque();

    if (!aceptar_lexema("mientras")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«mientras»");
        return sentencia;
    }
    if (!aceptar_lexema("(")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«(»");
    }
    sentencia.paso = expresion(0);
    if (!aceptar_lexema(")")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
    }
    return sentencia;
}

Sentencia Parser::analizar_bucle() {
    Sentencia sentencia;
    sentencia.tipo = TipoSentencia::Bucle;
    sentencia.span = actual().span;
    avanzar();  // "bucle"
    sentencia.cuerpo = analizar_bloque();
    return sentencia;
}

// ---------------------------------------------------------------------------
// Expresiones (Pratt)
// ---------------------------------------------------------------------------

ExpresionPtr Parser::analizar_expresion_completa() {
    ExpresionPtr resultado = expresion(0);
    if (!actual().es(TipoToken::FinDeArchivo)) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "el final de la expresión");
    }
    return resultado;
}

ExpresionPtr Parser::expresion(int potencia_minima) {
    ExpresionPtr izquierda = prefijo();
    if (!izquierda) {
        return nullptr;
    }

    for (;;) {
        const Token& token = actual();

        // Postfijos: se unen más fuerte que cualquier operador.
        if (token.lexema == "(") {
            izquierda = llamada(std::move(izquierda));
            continue;
        }
        if (token.lexema == "[") {
            izquierda = indice(std::move(izquierda));
            continue;
        }
        if (token.lexema == ".") {
            izquierda = miembro(std::move(izquierda), false);
            continue;
        }
        if (token.lexema == "?.") {
            izquierda = miembro(std::move(izquierda), true);
            continue;
        }
        if (token.lexema == "?") {
            const diagnostico::Span span = token.span;
            avanzar();
            auto nodo = std::make_unique<Expresion>();
            nodo->tipo = TipoExpresion::Propagacion;
            nodo->span = unir(izquierda->span, span);
            nodo->izquierda = std::move(izquierda);
            izquierda = std::move(nodo);
            continue;
        }

        const Enlace enlace = enlace_de(token.lexema);
        if (enlace.potencia == 0 || enlace.potencia < potencia_minima) {
            break;
        }

        const std::string_view operador = token.lexema;
        avanzar();

        if (operador == "como") {
            auto nodo = std::make_unique<Expresion>();
            nodo->tipo = TipoExpresion::Como;
            nodo->lexema = operador;
            nodo->span = izquierda->span;
            nodo->izquierda = std::move(izquierda);
            nodo->anotacion = analizar_tipo();
            izquierda = std::move(nodo);
            continue;
        }

        const int siguiente = enlace.derecha ? enlace.potencia : enlace.potencia + 1;
        ExpresionPtr derecha = expresion(siguiente);
        if (!derecha) {
            break;
        }

        auto nodo = std::make_unique<Expresion>();
        nodo->tipo = es_asignacion(operador) ? TipoExpresion::Asignacion : TipoExpresion::Binaria;
        nodo->lexema = operador;
        nodo->span = unir(izquierda->span, derecha->span);
        nodo->izquierda = std::move(izquierda);
        nodo->derecha = std::move(derecha);
        izquierda = std::move(nodo);

        // Los rangos no son asociativos: `a..b..c` es un error, no `(a..b)..c`.
        if ((operador == ".." || operador == "..=") &&
            (actual().lexema == ".." || actual().lexema == "..=")) {
            error_en(actual(), diagnostico::codigos::RANGO_ENCADENADO, "");
            break;
        }
    }

    return izquierda;
}

ExpresionPtr Parser::prefijo() {
    const Token& token = actual();

    auto literal = [this, &token](TipoExpresion tipo) {
        auto nodo = std::make_unique<Expresion>();
        nodo->tipo = tipo;
        nodo->lexema = token.lexema;
        nodo->span = token.span;
        avanzar();
        return nodo;
    };

    switch (token.tipo) {
        case TipoToken::Texto: {
            auto nodo = literal(TipoExpresion::Texto);
            nodo->lexema = sin_comillas(nodo->lexema);
            return nodo;
        }
        case TipoToken::Caracter: {
            auto nodo = literal(TipoExpresion::Caracter);
            nodo->lexema = sin_comillas(nodo->lexema);
            return nodo;
        }
        case TipoToken::Entero:
            return literal(TipoExpresion::Entero);
        case TipoToken::Flotante:
            return literal(TipoExpresion::Flotante);
        case TipoToken::Identificador:
            return literal(TipoExpresion::Identificador);
        default:
            break;
    }

    if (token.es(TipoToken::PalabraClave)) {
        if (token.lexema == "verdadero" || token.lexema == "falso") {
            return literal(TipoExpresion::Booleano);
        }
        if (token.lexema == "nulo") {
            return literal(TipoExpresion::Nulo);
        }
    }

    if (token.lexema == "(") {
        const diagnostico::Span span = token.span;
        avanzar();
        ExpresionPtr interior = expresion(0);
        if (!aceptar_lexema(")")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
        }
        if (interior) {
            interior->span = unir(span, interior->span);
        }
        return interior;
    }

    if (token.lexema == "-" || token.lexema == "!" || token.lexema == "*" || token.lexema == "&") {
        std::string_view operador = token.lexema;
        const diagnostico::Span span = token.span;
        avanzar();
        if (operador == "&" && actual().lexema == "mutable") {
            operador = "&mutable";
            avanzar();
        }

        auto nodo = std::make_unique<Expresion>();
        nodo->tipo = TipoExpresion::Unaria;
        nodo->lexema = operador;
        nodo->izquierda = expresion(POTENCIA_UNARIA);
        nodo->span = nodo->izquierda ? unir(span, nodo->izquierda->span) : span;
        return nodo;
    }

    error_en(token, diagnostico::codigos::TOKEN_INESPERADO, "una expresión");
    avanzar();
    return nullptr;
}

ExpresionPtr Parser::llamada(ExpresionPtr destino) {
    const diagnostico::Span span = destino->span;
    avanzar();  // "("

    auto nodo = std::make_unique<Expresion>();
    nodo->tipo = TipoExpresion::Llamada;
    nodo->izquierda = std::move(destino);

    while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != ")") {
        ExpresionPtr argumento = expresion(0);
        if (!argumento) {
            break;
        }
        nodo->lista.push_back(std::move(argumento));
        if (!aceptar_lexema(",")) {
            break;
        }
    }

    if (!aceptar_lexema(")")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
    }

    nodo->span = span;
    return nodo;
}

ExpresionPtr Parser::indice(ExpresionPtr objeto) {
    const diagnostico::Span span = objeto->span;
    avanzar();  // "["

    ExpresionPtr posicion = expresion(0);
    if (!aceptar_lexema("]")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«]»");
    }

    auto nodo = std::make_unique<Expresion>();
    nodo->tipo = TipoExpresion::Indice;
    nodo->span = span;
    nodo->izquierda = std::move(objeto);
    if (posicion) {
        nodo->lista.push_back(std::move(posicion));
    }
    return nodo;
}

ExpresionPtr Parser::miembro(ExpresionPtr objeto, bool opcional) {
    const diagnostico::Span span = objeto->span;
    avanzar();  // "." o "?."

    auto nodo = std::make_unique<Expresion>();
    nodo->tipo = opcional ? TipoExpresion::MiembroOpcional : TipoExpresion::Miembro;
    nodo->span = span;
    nodo->izquierda = std::move(objeto);

    if (const Token& nombre = actual();
        nombre.es(TipoToken::Identificador) || nombre.es(TipoToken::PalabraClave)) {
        nodo->anotacion = std::string(nombre.lexema);
        avanzar();
    } else {
        error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de miembro");
    }
    return nodo;
}

std::string Parser::analizar_tipo() {
    std::string tipo;

    if (actual().lexema == "&" || actual().lexema == "*") {
        tipo = std::string(actual().lexema);
        avanzar();
        if (actual().lexema == "mutable") {
            tipo += "mutable ";
            avanzar();
        }
    }

    if (actual().es(TipoToken::PalabraClave) || actual().es(TipoToken::Identificador)) {
        tipo += std::string(actual().lexema);
        avanzar();
    } else {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "un tipo");
    }

    return tipo;
}

}  // namespace nooki::sintactico
