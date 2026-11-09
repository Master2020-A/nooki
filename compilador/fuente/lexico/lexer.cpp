#include "nooki/lexico/lexer.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "nooki/diagnostico/codigos.hpp"
#include "nooki/utf8.hpp"

namespace nooki::lexico {
namespace {

// ---------------------------------------------------------------------------
// Clasificación de caracteres
// ---------------------------------------------------------------------------

constexpr bool es_digito(char c) noexcept { return c >= '0' && c <= '9'; }

constexpr bool es_hexadecimal(char c) noexcept {
    return es_digito(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

constexpr bool es_octal(char c) noexcept { return c >= '0' && c <= '7'; }

constexpr bool es_binario(char c) noexcept { return c == '0' || c == '1'; }

constexpr bool es_letra_ascii(char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool es_letra(char32_t c) noexcept {
    if (c < 0x80) {
        return es_letra_ascii(static_cast<char>(c));
    }
    switch (c) {
        case U'á':
        case U'é':
        case U'í':
        case U'ó':
        case U'ú':
        case U'ü':
        case U'ñ':
        case U'Á':
        case U'É':
        case U'Í':
        case U'Ó':
        case U'Ú':
        case U'Ü':
        case U'Ñ':
            return true;
        default:
            return false;
    }
}

bool es_continuacion(char32_t c) noexcept {
    return es_letra(c) || c == U'_' || (c < 0x80 && es_digito(static_cast<char>(c)));
}

// ---------------------------------------------------------------------------
// Palabras reservadas (especificación §2.3: 63 palabras + 21 tipos)
// ---------------------------------------------------------------------------

constexpr std::array<std::string_view, 84> PALABRAS_RESERVADAS{{
    "Este",
    "abstracta",
    "asincrono",
    "asm",
    "atrapar",
    "booleano",
    "bucle",
    "byte",
    "caracter",
    "caso",
    "clase",
    "coincide",
    "como",
    "constante",
    "constructor",
    "continuar",
    "destructor",
    "donde",
    "en",
    "entero",
    "entero128",
    "entero16",
    "entero32",
    "entero64",
    "entero8",
    "enumeracion",
    "es",
    "esperar",
    "establecer",
    "estatico",
    "este",
    "estructura",
    "exportar",
    "externo",
    "falso",
    "finalmente",
    "flotante",
    "flotante32",
    "funcion",
    "hacer",
    "hereda",
    "implementa",
    "implementar",
    "importar",
    "inseguro",
    "intentar",
    "interfaz",
    "interna",
    "lanzar",
    "macro",
    "mientras",
    "mixin",
    "modulo",
    "mutable",
    "natural",
    "natural128",
    "natural16",
    "natural32",
    "natural64",
    "natural8",
    "nuevo",
    "nulo",
    "obtener",
    "para",
    "paso",
    "privado",
    "propiedad",
    "protegido",
    "publico",
    "rasgo",
    "retornar",
    "romper",
    "si",
    "sino",
    "sinosi",
    "super",
    "tamano",
    "tamano_signado",
    "texto",
    "tipo",
    "union",
    "vacio",
    "variable",
    "verdadero",
}};

static_assert(std::is_sorted(PALABRAS_RESERVADAS.begin(), PALABRAS_RESERVADAS.end()),
              "la tabla de palabras reservadas debe estar en orden ordinal para permitir la "
              "búsqueda binaria");

bool es_palabra_reservada(std::string_view texto) {
    return std::binary_search(PALABRAS_RESERVADAS.begin(), PALABRAS_RESERVADAS.end(), texto);
}

/// Quita las tildes y la diéresis para detectar palabras clave escritas con acento
/// («función» → «funcion»), que es el error léxico más probable en Nooki.
std::string quitar_tildes(std::string_view texto) {
    std::string salida;
    salida.reserve(texto.size());

    std::size_t i = 0;
    while (i < texto.size()) {
        const utf8::Punto punto = utf8::decodificar(texto, i);
        switch (punto.valor) {
            case U'á':
            case U'Á':
                salida += 'a';
                break;
            case U'é':
            case U'É':
                salida += 'e';
                break;
            case U'í':
            case U'Í':
                salida += 'i';
                break;
            case U'ó':
            case U'Ó':
                salida += 'o';
                break;
            case U'ú':
            case U'ü':
            case U'Ú':
            case U'Ü':
                salida += 'u';
                break;
            case U'ñ':
            case U'Ñ':
                salida += 'n';
                break;
            default:
                salida.append(texto.substr(i, punto.bytes));
                break;
        }
        i += punto.bytes;
    }
    return salida;
}

// ---------------------------------------------------------------------------
// Operadores
// ---------------------------------------------------------------------------

// Los más largos primero: `..=` antes que `..`, `<<=` antes que `<<`.
constexpr std::array<std::string_view, 25> OPERADORES{{
    "..=", "<<=", ">>=", "->", "=>", "==", "!=", "<=", ">=", "&&", "||", "??",
    "?.",  "**",  "<<",  ">>", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
    "..",
}};

bool es_operador_simple(char c) {
    constexpr std::string_view SIMPLES = "+-*/%=<>!&|^~?.,:;()[]{}@";
    return SIMPLES.find(c) != std::string_view::npos;
}

bool es_escape_valido(char c) {
    return c == 'n' || c == 'r' || c == 't' || c == '\\' || c == '"' || c == '\'' || c == '0' ||
           c == 'u';
}

/// Texto imprimible para un carácter que no pertenece al lenguaje.
std::string describir_caracter(char32_t valor) {
    if (valor >= 0x20 && valor < 0x7F) {
        return std::string(1, static_cast<char>(valor));
    }
    constexpr const char* HEXADECIMAL = "0123456789ABCDEF";
    std::string salida = "U+";
    for (int desplazamiento = 16; desplazamiento >= 0; desplazamiento -= 4) {
        salida += HEXADECIMAL[(valor >> desplazamiento) & 0x0F];
    }
    return salida;
}

}  // namespace

// ---------------------------------------------------------------------------
// Lexer
// ---------------------------------------------------------------------------

Lexer::Lexer(const diagnostico::GestorFuentes& fuentes, diagnostico::ArchivoId archivo,
             diagnostico::Emisor& emisor)
    : fuentes_(fuentes), archivo_(archivo), emisor_(emisor), contenido_(fuentes.contenido(archivo)) {}

char Lexer::actual() const noexcept { return pos_ < contenido_.size() ? contenido_[pos_] : '\0'; }

char Lexer::mirar(std::size_t distancia) const noexcept {
    return (pos_ + distancia) < contenido_.size() ? contenido_[pos_ + distancia] : '\0';
}

void Lexer::avanzar(std::size_t cantidad) noexcept { pos_ += cantidad; }

Token Lexer::crear(TipoToken tipo, std::size_t inicio) const {
    return Token{tipo, contenido_.substr(inicio, pos_ - inicio),
                 diagnostico::Span{archivo_, static_cast<std::uint32_t>(inicio),
                                   static_cast<std::uint32_t>(pos_)}};
}

diagnostico::Diagnostico& Lexer::error(std::string_view codigo, std::size_t inicio,
                                       std::size_t fin) {
    return emisor_.error(codigo, diagnostico::Span{archivo_, static_cast<std::uint32_t>(inicio),
                                                   static_cast<std::uint32_t>(fin)});
}

Token Lexer::siguiente() {
    // Espacios y comentarios no producen tokens (salvo los de documentación).
    for (;;) {
        while (!fin() && (actual() == ' ' || actual() == '\t' || actual() == '\r')) {
            avanzar();
        }
        if (fin()) {
            return crear(TipoToken::FinDeArchivo, pos_);
        }

        if (actual() == '/' && mirar(1) == '/') {
            const bool documentacion = mirar(2) == '/' || mirar(2) == '!';
            const std::size_t inicio = pos_;
            while (!fin() && actual() != '\n') {
                avanzar();
            }
            if (documentacion) {
                return crear(TipoToken::Documentacion, inicio);
            }
            continue;
        }

        if (actual() == '/' && mirar(1) == '*') {
            const std::size_t inicio = pos_;
            avanzar(2);
            int profundidad = 1;
            while (!fin() && profundidad > 0) {
                if (actual() == '/' && mirar(1) == '*') {
                    avanzar(2);
                    ++profundidad;
                } else if (actual() == '*' && mirar(1) == '/') {
                    avanzar(2);
                    --profundidad;
                } else {
                    avanzar();
                }
            }
            if (profundidad > 0) {
                error(diagnostico::codigos::COMENTARIO_SIN_CERRAR, inicio, pos_);
            }
            continue;
        }

        break;
    }

    const std::size_t inicio = pos_;

    if (actual() == '\n') {
        avanzar();
        return crear(TipoToken::NuevaLinea, inicio);
    }

    const utf8::Punto punto = utf8::decodificar(contenido_, pos_);

    if (punto.valor == 0xFFFD && !utf8::valido_en(contenido_, pos_)) {
        avanzar();
        error(diagnostico::codigos::UTF8_INVALIDO, inicio, pos_);
        return crear(TipoToken::Operador, inicio);
    }

    if (es_letra(punto.valor) || (punto.valor == U'_' && es_continuacion(
                                     utf8::decodificar(contenido_, pos_ + 1).valor))) {
        return leer_identificador();
    }

    if (es_digito(actual())) {
        return leer_numero();
    }

    if (actual() == '"') {
        return leer_texto();
    }

    if (actual() == '\'') {
        return leer_caracter_o_vida();
    }

    if (es_operador_simple(actual())) {
        return leer_operador();
    }

    // Cualquier otra cosa es un carácter que Nooki no usa.
    avanzar(punto.bytes);
    auto& diagnostico = error(diagnostico::codigos::CARACTER_INVALIDO, inicio, pos_);
    diagnostico.parametro("caracter", describir_caracter(punto.valor));
    return crear(TipoToken::Operador, inicio);
}

Token Lexer::leer_identificador() {
    const std::size_t inicio = pos_;
    bool tiene_no_ascii = false;

    while (!fin()) {
        const utf8::Punto punto = utf8::decodificar(contenido_, pos_);
        if (!es_continuacion(punto.valor)) {
            break;
        }
        if (punto.valor >= 0x80) {
            tiene_no_ascii = true;
        }
        pos_ += punto.bytes;
    }

    const std::string_view texto = contenido_.substr(inicio, pos_ - inicio);

    if (es_palabra_reservada(texto)) {
        return crear(TipoToken::PalabraClave, inicio);
    }

    // Solo un identificador con bytes no ASCII puede ser una palabra clave con tilde.
    // La comprobación evita asignar una cadena por cada identificador del archivo.
    if (tiene_no_ascii) {
        const std::string sin_tilde = quitar_tildes(texto);
        if (sin_tilde != texto && es_palabra_reservada(sin_tilde)) {
            auto& diagnostico =
                error(diagnostico::codigos::PALABRA_RESERVADA_CON_TILDE, inicio, pos_);
            diagnostico.parametro("palabra", std::string(texto))
                .ayuda("escriba «" + sin_tilde + "»");
        }
    }

    return crear(TipoToken::Identificador, inicio);
}

Token Lexer::leer_numero() {
    const std::size_t inicio = pos_;
    bool flotante = false;

    auto consumir_digitos = [&](bool (*permitido)(char)) {
        std::size_t digitos = 0;
        while (!fin()) {
            if (actual() == '_') {
                avanzar();
                continue;
            }
            if (!permitido(actual())) {
                break;
            }
            ++digitos;
            avanzar();
        }
        return digitos;
    };

    if (actual() == '0' && (mirar(1) == 'x' || mirar(1) == 'X')) {
        avanzar(2);
        if (consumir_digitos(es_hexadecimal) == 0) {
            error(diagnostico::codigos::NUMERO_MAL_FORMADO, inicio, pos_);
        }
        return crear(TipoToken::Entero, inicio);
    }
    if (actual() == '0' && (mirar(1) == 'b' || mirar(1) == 'B')) {
        avanzar(2);
        if (consumir_digitos(es_binario) == 0) {
            error(diagnostico::codigos::NUMERO_MAL_FORMADO, inicio, pos_);
        }
        return crear(TipoToken::Entero, inicio);
    }
    if (actual() == '0' && (mirar(1) == 'o' || mirar(1) == 'O')) {
        avanzar(2);
        if (consumir_digitos(es_octal) == 0) {
            error(diagnostico::codigos::NUMERO_MAL_FORMADO, inicio, pos_);
        }
        return crear(TipoToken::Entero, inicio);
    }

    consumir_digitos(es_digito);

    // `1.5` es flotante; `0..10` es un rango y no se toca.
    if (actual() == '.' && es_digito(mirar(1))) {
        flotante = true;
        avanzar();
        consumir_digitos(es_digito);
    }

    if (actual() == 'e' || actual() == 'E') {
        const std::size_t marca = pos_;
        avanzar();
        if (actual() == '+' || actual() == '-') {
            avanzar();
        }
        if (es_digito(actual())) {
            consumir_digitos(es_digito);
            flotante = true;
        } else {
            pos_ = marca;
            error(diagnostico::codigos::NUMERO_MAL_FORMADO, marca, pos_ + 1);
            avanzar();
        }
    }

    // `123abc` no es un número seguido de un identificador: es un error.
    if (es_letra(utf8::decodificar(contenido_, pos_).valor)) {
        while (!fin() && es_continuacion(utf8::decodificar(contenido_, pos_).valor)) {
            avanzar(utf8::decodificar(contenido_, pos_).bytes);
        }
        error(diagnostico::codigos::NUMERO_MAL_FORMADO, inicio, pos_);
    }

    return crear(flotante ? TipoToken::Flotante : TipoToken::Entero, inicio);
}

Token Lexer::leer_texto() {
    const std::size_t inicio = pos_;

    // Texto multilínea: """ ... """.
    if (mirar(1) == '"' && mirar(2) == '"') {
        avanzar(3);
        while (!fin()) {
            if (actual() == '"' && mirar(1) == '"' && mirar(2) == '"') {
                avanzar(3);
                return crear(TipoToken::Texto, inicio);
            }
            avanzar(utf8::decodificar(contenido_, pos_).bytes);
        }
        error(diagnostico::codigos::TEXTO_SIN_CERRAR, inicio, pos_);
        return crear(TipoToken::Texto, inicio);
    }

    avanzar();  // comilla inicial

    while (!fin()) {
        if (actual() == '"') {
            avanzar();
            return crear(TipoToken::Texto, inicio);
        }
        if (actual() == '\n') {
            break;  // un texto normal no cruza de línea
        }
        if (actual() == '\\') {
            if (!es_escape_valido(mirar(1))) {
                const std::size_t escape_inicio = pos_;
                avanzar(2);
                auto& diagnostico =
                    error(diagnostico::codigos::ESCAPE_DESCONOCIDO, escape_inicio, pos_);
                diagnostico.parametro(
                    "escape", std::string(contenido_.substr(escape_inicio, pos_ - escape_inicio)));
                continue;
            }
            if (mirar(1) == 'u') {
                // \u{XXXX}
                const std::size_t escape_inicio = pos_;
                avanzar(2);
                while (!fin() && actual() != '}' && actual() != '\n') {
                    avanzar();
                }
                if (actual() == '}') {
                    avanzar();
                } else {
                    auto& diagnostico =
                        error(diagnostico::codigos::ESCAPE_DESCONOCIDO, escape_inicio, pos_);
                    diagnostico.parametro(
                        "escape",
                        std::string(contenido_.substr(escape_inicio, pos_ - escape_inicio)));
                }
                continue;
            }
            avanzar(2);
            continue;
        }
        avanzar(utf8::decodificar(contenido_, pos_).bytes);
    }

    error(diagnostico::codigos::TEXTO_SIN_CERRAR, inicio, pos_);
    return crear(TipoToken::Texto, inicio);
}

Token Lexer::leer_caracter_o_vida() {
    const std::size_t inicio = pos_;

    // Un tiempo de vida es ' + identificador.
    const utf8::Punto siguiente = utf8::decodificar(contenido_, pos_ + 1);

    if (es_letra(siguiente.valor)) {
        const std::size_t tras_letra = pos_ + 1 + siguiente.bytes;
        const bool parece_caracter =
            tras_letra < contenido_.size() && contenido_[tras_letra] == '\'';
        if (!parece_caracter) {
            avanzar(1 + siguiente.bytes);
            while (!fin()) {
                const utf8::Punto punto = utf8::decodificar(contenido_, pos_);
                if (!es_continuacion(punto.valor)) {
                    break;
                }
                pos_ += punto.bytes;
            }
            return crear(TipoToken::Vida, inicio);
        }
    }

    avanzar();  // comilla
    if (fin() || actual() == '\n') {
        error(diagnostico::codigos::CARACTER_LITERAL_SIN_CERRAR, inicio, pos_);
        return crear(TipoToken::Caracter, inicio);
    }

    if (actual() == '\\') {
        if (!es_escape_valido(mirar(1))) {
            avanzar(2);
            auto& diagnostico =
                error(diagnostico::codigos::ESCAPE_DESCONOCIDO, inicio, pos_);
            diagnostico.parametro(
                "escape", std::string(contenido_.substr(inicio, pos_ - inicio)));
            return crear(TipoToken::Caracter, inicio);
        }
        if (mirar(1) == 'u') {
            avanzar(2);
            while (!fin() && actual() != '}' && actual() != '\'' && actual() != '\n') {
                avanzar();
            }
            if (actual() == '}') {
                avanzar();
            } else {
                auto& diagnostico =
                    error(diagnostico::codigos::ESCAPE_DESCONOCIDO, inicio, pos_);
                diagnostico.parametro(
                    "escape", std::string(contenido_.substr(inicio, pos_ - inicio)));
                return crear(TipoToken::Caracter, inicio);
            }
        } else {
            avanzar(2);
        }
    } else {
        avanzar(utf8::decodificar(contenido_, pos_).bytes);
    }

    if (actual() != '\'') {
        error(diagnostico::codigos::CARACTER_LITERAL_SIN_CERRAR, inicio, pos_);
        return crear(TipoToken::Caracter, inicio);
    }
    avanzar();
    return crear(TipoToken::Caracter, inicio);
}

Token Lexer::leer_operador() {
    const std::size_t inicio = pos_;

    for (const std::string_view operador : OPERADORES) {
        if (contenido_.compare(pos_, operador.size(), operador) == 0) {
            avanzar(operador.size());
            return crear(TipoToken::Operador, inicio);
        }
    }

    avanzar();
    return crear(TipoToken::Operador, inicio);
}

std::vector<Token> Lexer::todos() {
    std::vector<Token> tokens;
    // Un token cada tres bytes es una estimación conservadora: evita reasignaciones
    // en el caso habitual sin desperdiciar memoria.
    tokens.reserve(contenido_.size() / 3 + 8);

    for (;;) {
        Token token = siguiente();
        const bool fin_de_archivo = token.es(TipoToken::FinDeArchivo);
        tokens.push_back(token);
        if (fin_de_archivo) {
            break;
        }
    }
    return tokens;
}

}  // namespace nooki::lexico
