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
    // Un texto multilínea lleva tres comillas a cada lado: «"""..."""».
    if (lexema.size() >= 6 && lexema.substr(0, 3) == "\"\"\"" &&
        lexema.substr(lexema.size() - 3) == "\"\"\"") {
        return lexema.substr(3, lexema.size() - 6);
    }
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
        if (token.es(TipoToken::PalabraClave) && token.lexema == "usar") {
            avanzar();
            if (const Token& modulo = actual(); modulo.es(TipoToken::Identificador)) {
                programa.usos.push_back(modulo.lexema);
                avanzar();
            } else {
                error_en(modulo, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de módulo");
            }
            continue;
        }
        if (token.lexema == "publico" || token.lexema == "privado") {
            const bool publico = token.lexema == "publico";
            avanzar();
            if (actual().lexema == "funcion") {
                Funcion funcion = analizar_funcion();
                funcion.publico = publico;
                programa.funciones.push_back(std::move(funcion));
            } else if (actual().lexema == "estructura" || actual().lexema == "clase") {
                programa.tipos.push_back(analizar_definicion_tipo(actual().lexema == "clase"));
                programa.tipos.back().publico = publico;
            } else if (actual().lexema == "enumeracion") {
                programa.enumeraciones.push_back(analizar_enumeracion());
                programa.enumeraciones.back().publico = publico;
            } else {
                error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO,
                         "«funcion», «estructura», «clase» o «enumeracion»");
            }
            continue;
        }
        if (token.es(TipoToken::PalabraClave) && token.lexema == "funcion") {
            programa.funciones.push_back(analizar_funcion());
            continue;
        }
        if (token.es(TipoToken::PalabraClave) && token.lexema == "externo") {
            const std::size_t inicio = indice_;
            avanzar();
            if (actual().lexema != "funcion") {
                error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«funcion»");
                continue;
            }
            programa.funciones.push_back(analizar_funcion(true));
            (void)inicio;
            continue;
        }
        if (token.es(TipoToken::PalabraClave) &&
            (token.lexema == "estructura" || token.lexema == "clase")) {
            programa.tipos.push_back(analizar_definicion_tipo(token.lexema == "clase"));
            continue;
        }
        if (token.es(TipoToken::PalabraClave) && token.lexema == "enumeracion") {
            programa.enumeraciones.push_back(analizar_enumeracion());
            continue;
        }
        if (token.es(TipoToken::PalabraClave) && token.lexema == "rasgo") {
            programa.rasgos.push_back(analizar_rasgo());
            continue;
        }
        error_en(token, diagnostico::codigos::TOKEN_INESPERADO, "«funcion»");
        avanzar();
    }

    return programa;
}

DefinicionTipo Parser::analizar_definicion_tipo(bool es_clase) {
    DefinicionTipo definicion;
    definicion.es_clase = es_clase;
    definicion.span = actual().span;
    avanzar();  // "estructura" | "clase"

    if (const Token& nombre = actual(); nombre.es(TipoToken::Identificador)) {
        definicion.nombre = nombre.lexema;
        avanzar();
    } else {
        error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de tipo");
        return definicion;
    }

    // Parámetros de tipo: `estructura par<T>`.
    if (actual().lexema == "<") {
        avanzar();
        while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != ">") {
            if (const Token& parametro = actual();
                parametro.es(TipoToken::Identificador) ||
                parametro.es(TipoToken::PalabraClave)) {
                definicion.tipo_parametros.push_back(parametro.lexema);
                avanzar();
            } else {
                error_en(parametro, diagnostico::codigos::TOKEN_INESPERADO,
                         "un parámetro de tipo");
                break;
            }
            if (!aceptar_lexema(",")) {
                break;
            }
        }
        if (!aceptar_lexema(">")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«>»");
        }
    }

    // Rasgos: `clase persona implementa Describible, Comparable`.
    while (actual().lexema == "implementa") {
        avanzar();
        for (;;) {
            if (const Token& rasgo = actual();
                rasgo.es(TipoToken::Identificador) || rasgo.es(TipoToken::PalabraClave)) {
                definicion.rasgos.push_back(rasgo.lexema);
                avanzar();
            } else {
                error_en(rasgo, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de rasgo");
                break;
            }
            if (!aceptar_lexema(",")) {
                break;
            }
        }
    }

    // Herencia simple: `clase cuadrado hereda figura`.
    if (actual().lexema == "hereda") {
        avanzar();
        if (const Token& base = actual();
            base.es(TipoToken::Identificador) || base.es(TipoToken::PalabraClave)) {
            definicion.base = base.lexema;
            avanzar();
        } else {
            error_en(base, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de clase");
        }
    }

    if (!aceptar_lexema("{")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«{»");
        return definicion;
    }

    while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != "}") {
        Campo campo;
        campo.span = actual().span;

        if (actual().lexema == "publico" || actual().lexema == "privado" ||
            actual().lexema == "protegido" || actual().lexema == "interna") {
            campo.publico = actual().lexema == "publico";
            avanzar();
        }

        // Un método se analiza con el mismo código que una función.
        if (actual().es(TipoToken::PalabraClave) && actual().lexema == "funcion") {
            definicion.metodos.push_back(analizar_funcion());
            continue;
        }

        if (const Token& nombre = actual(); nombre.es(TipoToken::Identificador)) {
            campo.nombre = nombre.lexema;
            avanzar();
        } else {
            error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de campo");
            break;
        }

        if (!aceptar_lexema(":")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«:»");
            break;
        }

        campo.tipo = analizar_tipo_texto();
        if (campo.tipo.empty()) {
            break;
        }

        definicion.campos.push_back(campo);
        aceptar_lexema(",");
    }

    if (!aceptar_lexema("}")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«}»");
    }
    return definicion;
}

bool Parser::viene_construccion() {
    std::size_t i = indice_;
    while (i < tokens_.size() && tokens_[i].es(TipoToken::NuevaLinea)) {
        ++i;
    }
    if (i >= tokens_.size() || tokens_[i].lexema != "{") {
        return false;
    }
    ++i;
    while (i < tokens_.size() && tokens_[i].es(TipoToken::NuevaLinea)) {
        ++i;
    }
    if (i >= tokens_.size() || !tokens_[i].es(TipoToken::Identificador)) {
        return false;
    }
    ++i;
    while (i < tokens_.size() && tokens_[i].es(TipoToken::NuevaLinea)) {
        ++i;
    }
    return i < tokens_.size() && tokens_[i].lexema == ":";
}

ExpresionPtr Parser::analizar_construccion(std::string_view tipo) {
    auto nodo = std::make_unique<Expresion>();
    nodo->tipo = TipoExpresion::Construccion;
    nodo->anotacion = std::string(tipo);
    nodo->span = actual().span;

    avanzar();  // "{"

    while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != "}") {
        if (!actual().es(TipoToken::Identificador)) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "un nombre de campo");
            break;
        }
        const std::string_view nombre_campo = actual().lexema;
        avanzar();

        if (!aceptar_lexema(":")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«:»");
            break;
        }

        ExpresionPtr valor = expresion(0);
        nodo->nombres_campo.push_back(nombre_campo);
        nodo->lista.push_back(std::move(valor));

        if (!aceptar_lexema(",")) {
            break;
        }
    }

    if (!aceptar_lexema("}")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«}»");
    }
    return nodo;
}

Funcion Parser::analizar_funcion(bool externa) {
    Funcion funcion;
    funcion.es_externa = externa;
    funcion.span = actual().span;
    avanzar();  // "funcion"

    if (const Token& nombre = actual(); nombre.es(TipoToken::Identificador)) {
        funcion.nombre = nombre.lexema;
        funcion.span.fin = nombre.span.fin;
        avanzar();
    } else {
        error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de función");
    }

    // Parámetros de tipo: `funcion identidad<T>(...)`.
    if (actual().lexema == "<") {
        avanzar();
        while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != ">") {
            if (const Token& parametro = actual();
                parametro.es(TipoToken::Identificador) ||
                parametro.es(TipoToken::PalabraClave)) {
                funcion.tipo_parametros.push_back(parametro.lexema);
                avanzar();
            } else {
                error_en(parametro, diagnostico::codigos::TOKEN_INESPERADO,
                         "un parámetro de tipo");
                break;
            }
            if (!aceptar_lexema(",")) {
                break;
            }
        }
        if (!aceptar_lexema(">")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«>»");
        }
    }

    if (!aceptar_lexema("(")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«(»");
    }

    if (actual().lexema != ")") {
        for (;;) {
            Parametro parametro;
            parametro.span = actual().span;

            if (const Token& nombre = actual(); nombre.es(TipoToken::Identificador)) {
                parametro.nombre = nombre.lexema;
                avanzar();
            } else {
                error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de parámetro");
                break;
            }

            if (!aceptar_lexema(":")) {
                error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«:»");
                break;
            }
            parametro.tipo = analizar_tipo_texto();
            if (parametro.tipo.empty()) {
                break;
            }

            funcion.parametros.push_back(parametro);
            if (!aceptar_lexema(",")) {
                break;
            }
        }
    }

    if (!aceptar_lexema(")")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
    }

    funcion.tipo_retorno = "vacio";
    if (aceptar_lexema("->")) {
        const std::string_view tipo = analizar_tipo_texto();
        if (!tipo.empty()) {
            funcion.tipo_retorno = tipo;
        }
    }

    // Una función externa se declara, no se define: no lleva cuerpo.
    if (funcion.es_externa) {
        if (actual().lexema == "{") {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "nada tras la firma");
            analizar_bloque();
        }
        return funcion;
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

Rasgo Parser::analizar_rasgo() {
    Rasgo rasgo;
    rasgo.span = actual().span;
    avanzar();  // "rasgo"

    if (const Token& nombre = actual(); nombre.es(TipoToken::Identificador)) {
        rasgo.nombre = nombre.lexema;
        avanzar();
    } else {
        error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de rasgo");
        return rasgo;
    }

    if (!aceptar_lexema("{")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«{»");
        return rasgo;
    }

    while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != "}") {
        if (actual().lexema != "funcion") {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«funcion»");
            avanzar();
            continue;
        }

        Funcion firma;
        firma.span = actual().span;
        avanzar();  // "funcion"

        if (const Token& nombre = actual();
            nombre.es(TipoToken::Identificador) || nombre.es(TipoToken::PalabraClave)) {
            firma.nombre = nombre.lexema;
            avanzar();
        } else {
            error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de método");
            break;
        }

        if (!aceptar_lexema("(")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«(»");
            break;
        }
        while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != ")") {
            Parametro parametro;
            parametro.span = actual().span;
            if (const Token& nombre = actual(); nombre.es(TipoToken::Identificador)) {
                parametro.nombre = nombre.lexema;
                avanzar();
            } else {
                error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO,
                         "un nombre de parámetro");
                break;
            }
            if (!aceptar_lexema(":")) {
                error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«:»");
                break;
            }
            parametro.tipo = analizar_tipo_texto();
            if (parametro.tipo.empty()) {
                break;
            }
            firma.parametros.push_back(std::move(parametro));
            if (!aceptar_lexema(",")) {
                break;
            }
        }
        if (!aceptar_lexema(")")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
            break;
        }

        if (aceptar_lexema("->")) {
            const std::string_view tipo = analizar_tipo_texto();
            if (!tipo.empty()) {
                firma.tipo_retorno = tipo;
            }
        }
        rasgo.metodos.push_back(std::move(firma));
    }

    if (!aceptar_lexema("}")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«}»");
    }
    return rasgo;
}

Enumeracion Parser::analizar_enumeracion() {
    Enumeracion enumeracion;
    enumeracion.span = actual().span;
    avanzar();  // "enumeracion"

    if (const Token& nombre = actual(); nombre.es(TipoToken::Identificador)) {
        enumeracion.nombre = nombre.lexema;
        avanzar();
    } else {
        error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO, "un nombre de enumeración");
        return enumeracion;
    }

    if (!aceptar_lexema("{")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«{»");
        return enumeracion;
    }

    while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != "}") {
        if (const Token& token_variante = actual();
            token_variante.es(TipoToken::Identificador)) {
            Variante variante;
            variante.nombre = token_variante.lexema;
            variante.span = token_variante.span;
            avanzar();

            // Campos de la variante: `Bien(valor: entero)`.
            if (aceptar_lexema("(")) {
                while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != ")") {
                    Campo campo;
                    campo.span = actual().span;
                    if (const Token& nombre = actual(); nombre.es(TipoToken::Identificador)) {
                        campo.nombre = nombre.lexema;
                        avanzar();
                    } else {
                        error_en(nombre, diagnostico::codigos::TOKEN_INESPERADO,
                                 "un nombre de campo");
                        break;
                    }
                    if (!aceptar_lexema(":")) {
                        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«:»");
                        break;
                    }
                    campo.tipo = analizar_tipo_texto();
                    if (campo.tipo.empty()) {
                        break;
                    }
                    variante.campos.push_back(std::move(campo));
                    if (!aceptar_lexema(",")) {
                        break;
                    }
                }
                if (!aceptar_lexema(")")) {
                    error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
                }
            }

            enumeracion.variantes.push_back(std::move(variante));
        } else {
            error_en(token_variante, diagnostico::codigos::TOKEN_INESPERADO,
                     "un nombre de variante");
            break;
        }
        // La coma es opcional: también vale una variante por línea.
        if (!aceptar_lexema(",") && !actual().es(TipoToken::Identificador)) {
            break;
        }
    }

    if (!aceptar_lexema("}")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«}»");
    }
    return enumeracion;
}

Patron Parser::analizar_patron() {
    Patron patron;
    patron.span = actual().span;

    if (actual().lexema == "_") {
        patron.clase = Patron::Clase::Comodin;
        avanzar();
        return patron;
    }

    if (actual().es(TipoToken::Entero) || actual().es(TipoToken::Texto) ||
        actual().es(TipoToken::Caracter) ||
        (actual().es(TipoToken::PalabraClave) &&
         (actual().lexema == "verdadero" || actual().lexema == "falso"))) {
        patron.clase = Patron::Clase::Literal;
        patron.valor = actual().lexema;
        avanzar();
        return patron;
    }

    if (actual().es(TipoToken::Identificador)) {
        const std::string_view nombre = actual().lexema;
        avanzar();

        if (aceptar_lexema("(")) {
            // Ligaduras de una variante con datos: `caso Bien(v)`.
            patron.clase = Patron::Clase::Variante;
            patron.enumeracion = nombre;
            patron.valor = nombre;
            while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != ")") {
                if (const Token& ligadura = actual(); ligadura.es(TipoToken::Identificador)) {
                    patron.ligaduras.push_back(ligadura.lexema);
                    avanzar();
                } else {
                    error_en(ligadura, diagnostico::codigos::TOKEN_INESPERADO,
                             "un nombre de variable");
                    break;
                }
                if (!aceptar_lexema(",")) {
                    break;
                }
            }
            if (!aceptar_lexema(")")) {
                error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
            }
            return patron;
        }

        if (aceptar_lexema(".")) {
            if (const Token& variante = actual(); variante.es(TipoToken::Identificador)) {
                patron.clase = Patron::Clase::Variante;
                patron.enumeracion = nombre;
                patron.valor = variante.lexema;
                avanzar();
                return patron;
            }
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "un nombre de variante");
            return patron;
        }

        patron.clase = Patron::Clase::Literal;
        patron.valor = nombre;
        return patron;
    }

    error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "un patrón");
    avanzar();
    return patron;
}

Sentencia Parser::analizar_coincide() {
    Sentencia sentencia;
    sentencia.tipo = TipoSentencia::Coincide;
    sentencia.span = actual().span;
    avanzar();  // "coincide"

    if (!aceptar_lexema("(")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«(»");
    }
    sentencia.expresion = expresion(0);
    if (!aceptar_lexema(")")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
    }

    if (!aceptar_lexema("{")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«{»");
        return sentencia;
    }

    while (!actual().es(TipoToken::FinDeArchivo) && actual().lexema != "}") {
        if (actual().lexema != "caso") {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«caso»");
            avanzar();
            continue;
        }
        avanzar();

        Rama rama;
        rama.patron = analizar_patron();

        if (!aceptar_lexema("=>")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«=>»");
            break;
        }

        if (actual().lexema == "{") {
            rama.cuerpo = analizar_bloque();
        } else {
            Sentencia cuerpo;
            cuerpo.tipo = TipoSentencia::Expresion;
            cuerpo.span = actual().span;
            cuerpo.expresion = expresion(0);
            rama.cuerpo.push_back(std::move(cuerpo));
        }

        sentencia.ramas.push_back(std::move(rama));
        aceptar_lexema(",");
    }

    if (!aceptar_lexema("}")) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«}»");
    }
    return sentencia;
}

Sentencia Parser::analizar_sentencia() {
    Sentencia sentencia;
    sentencia.span = actual().span;

    if (actual().lexema == "{") {
        sentencia.tipo = TipoSentencia::Bloque;
        sentencia.cuerpo = analizar_bloque();
        return sentencia;
    }

    // `asm!("...")`: ensamblador en línea.
    if (actual().lexema == "asm") {
        sentencia.tipo = TipoSentencia::Asm;
        avanzar();
        if (!aceptar_lexema("!")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«!»");
            return sentencia;
        }
        if (!aceptar_lexema("(")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«(»");
            return sentencia;
        }
        if (const Token& codigo = actual(); codigo.es(TipoToken::Texto)) {
            sentencia.nombre = sin_comillas(codigo.lexema);
            avanzar();
        } else {
            error_en(codigo, diagnostico::codigos::TOKEN_INESPERADO, "un texto con el ensamblador");
        }
        if (!aceptar_lexema(")")) {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«)»");
        }
        return sentencia;
    }

    // `inseguro { ... }`: dentro no se comprueba la aritmética.
    if (actual().lexema == "inseguro") {
        sentencia.tipo = TipoSentencia::Inseguro;
        avanzar();
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
            return analizar_coincide();
        }
        if (palabra != "verdadero" && palabra != "falso" && palabra != "nulo" &&
            palabra != "este" && palabra != "super") {
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
        sentencia.tipo_declarado = analizar_tipo_texto();
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
        if (token.lexema == "<" && izquierda->tipo == TipoExpresion::Identificador) {
            if (intentar_tipos_aplicados(izquierda)) {
                continue;
            }
        }
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

        // Un operador al principio de una línea nueva no continúa la expresión:
        // `variable x = 1` seguido de `*p = 2` son dos sentencias, no una multiplicación.
        if (salto_previo_) {
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
        case TipoToken::Identificador: {
            auto nodo = literal(TipoExpresion::Identificador);
            if (actual().lexema == "{" && viene_construccion()) {
                return analizar_construccion(nodo->lexema);
            }
            // Construcción de un tipo genérico: `par<entero> { ... }`.
            if (actual().lexema == "<") {
                const std::size_t inicio_tipo = nodo->span.inicio;
                const std::size_t guardado = indice_;
                avanzar();
                int profundidad = 1;
                std::size_t fin_tipo = 0;
                while (!actual().es(TipoToken::FinDeArchivo) && profundidad > 0) {
                    if (actual().lexema == "<") {
                        ++profundidad;
                    } else if (actual().lexema == ">") {
                        --profundidad;
                        if (profundidad == 0) {
                            fin_tipo = actual().span.fin;
                        }
                    }
                    avanzar();
                }
                if (fin_tipo != 0 && actual().lexema == "{") {
                    const std::string_view contenido = fuentes_.contenido(archivo_);
                    return analizar_construccion(
                        contenido.substr(inicio_tipo, fin_tipo - inicio_tipo));
                }
                indice_ = guardado;
            }
            return nodo;
        }
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
        if (token.lexema == "nuevo") {
            avanzar();
            if (actual().es(TipoToken::Identificador) || actual().es(TipoToken::PalabraClave)) {
                const std::string_view tipo = actual().lexema;
                avanzar();
                if (actual().lexema == "{") {
                    ExpresionPtr nodo = analizar_construccion(tipo);
                    if (nodo) {
                        nodo->monticulo = true;
                    }
                    return nodo;
                }
                error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "«{»");
                return nullptr;
            }
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "un tipo");
            return nullptr;
        }
        if (token.lexema == "super") {
            const diagnostico::Span span = token.span;
            avanzar();
            auto nodo = std::make_unique<Expresion>();
            nodo->tipo = TipoExpresion::Super;
            nodo->span = span;
            return nodo;
        }
        if (token.lexema == "este") {
            const diagnostico::Span span = token.span;
            avanzar();
            auto nodo = std::make_unique<Expresion>();
            nodo->tipo = TipoExpresion::Este;
            nodo->span = span;
            return nodo;
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
    nodo->span = span;
    // Los tipos genéricos se escriben en el destino (`f<entero>(...)`) y la
    // llamada los hereda para que el resto del compilador los vea en un solo sitio.
    nodo->tipos_aplicados = destino->tipos_aplicados;
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

bool Parser::intentar_tipos_aplicados(ExpresionPtr& nodo) {
    const std::size_t inicio = indice_;
    std::vector<std::string_view> tipos;

    avanzar();  // "<"
    for (;;) {
        if (actual().es(TipoToken::Identificador) || actual().es(TipoToken::PalabraClave)) {
            tipos.push_back(actual().lexema);
            avanzar();
        } else {
            break;
        }
        if (aceptar_lexema(",")) {
            continue;
        }
        break;
    }

    if (tipos.empty() || !aceptar_lexema(">") ||
        (actual().lexema != "(" && actual().lexema != ".")) {
        indice_ = inicio;  // no era una aplicación de tipos: retroceder
        return false;
    }

    nodo->tipos_aplicados = std::move(tipos);
    return true;
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

std::string_view Parser::analizar_tipo_texto() {
    const std::size_t inicio = actual().span.inicio;
    std::size_t fin = actual().span.fin;

    if (actual().lexema == "&" || actual().lexema == "*") {
        avanzar();
        if (actual().lexema == "mutable") {
            avanzar();
        }
    }

    if (!actual().es(TipoToken::PalabraClave) && !actual().es(TipoToken::Identificador)) {
        error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "un tipo");
        return {};
    }

    fin = actual().span.fin;
    avanzar();

    // Nombres cualificados: `modulo.tipo`.
    if (actual().lexema == ".") {
        avanzar();
        if (actual().es(TipoToken::Identificador) || actual().es(TipoToken::PalabraClave)) {
            fin = actual().span.fin;
            avanzar();
        } else {
            error_en(actual(), diagnostico::codigos::TOKEN_INESPERADO, "un nombre de tipo");
        }
    }

    // Genéricos: `Caja<entero>`.
    if (actual().lexema == "<") {
        int profundidad = 0;
        while (!actual().es(TipoToken::FinDeArchivo)) {
            const std::string_view lexema = actual().lexema;
            fin = actual().span.fin;
            if (lexema == "<") {
                ++profundidad;
            } else if (lexema == ">") {
                --profundidad;
            }
            avanzar();
            if (profundidad == 0) {
                break;
            }
        }
    }

    const std::string_view contenido = fuentes_.contenido(archivo_);
    if (fin <= contenido.size() && inicio < fin) {
        return contenido.substr(inicio, fin - inicio);
    }
    return {};
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
