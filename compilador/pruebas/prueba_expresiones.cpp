// Pruebas de precedencia y asociatividad del parser de expresiones.
//
// Cada caso escribe una expresión y su forma canónica esperada (notación prefija).
// La forma canónica procede de la tabla de la especificación §7.1: si el parser
// agrupa distinto, el texto no coincide y el caso falla.

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/sintactico/impresor.hpp"
#include "nooki/sintactico/parser.hpp"

namespace {

using namespace nooki;

int comprobaciones = 0;
int fallos = 0;

struct Caso {
    std::string_view fuente;
    std::string_view esperado;
};

// --- Literales -------------------------------------------------------------
constexpr Caso LITERALES[] = {
    {"1", "1"},
    {"3.14", "3.14"},
    {"\"hola\"", "\"hola\""},
    {"'a'", "'a'"},
    {"verdadero", "verdadero"},
    {"falso", "falso"},
    {"nulo", "nulo"},
    {"x", "x"},
    {"año", "año"},
    {"0xFF", "0xFF"},
    {"1_000", "1_000"},
    {"\"año\"", "\"año\""},
};

// --- Aritmética ------------------------------------------------------------
constexpr Caso ARITMETICA[] = {
    {"1 + 2", "(+ 1 2)"},
    {"1 - 2", "(- 1 2)"},
    {"1 * 2", "(* 1 2)"},
    {"1 / 2", "(/ 1 2)"},
    {"1 % 2", "(% 1 2)"},
    {"2 ** 3", "(** 2 3)"},
    {"1 + 2 * 3", "(+ 1 (* 2 3))"},
    {"1 * 2 + 3", "(+ (* 1 2) 3)"},
    {"1 + 2 - 3", "(- (+ 1 2) 3)"},
    {"1 - 2 + 3", "(+ (- 1 2) 3)"},
    {"1 * 2 / 3", "(/ (* 1 2) 3)"},
    {"1 / 2 * 3", "(* (/ 1 2) 3)"},
    {"1 + 2 * 3 - 4 / 5", "(- (+ 1 (* 2 3)) (/ 4 5))"},
    {"a * b + c * d", "(+ (* a b) (* c d))"},
    {"a + b * c + d", "(+ (+ a (* b c)) d)"},
    {"2 ** 3 ** 2", "(** 2 (** 3 2))"},
    {"2 * 3 ** 2", "(* 2 (** 3 2))"},
    {"2 ** 3 * 4", "(* (** 2 3) 4)"},
    {"-2 ** 2", "(** (- 2) 2)"},
    {"2 ** -3", "(** 2 (- 3))"},
    {"a % b * c", "(* (% a b) c)"},
    {"a * b % c", "(% (* a b) c)"},
    {"1 + 2 + 3 + 4", "(+ (+ (+ 1 2) 3) 4)"},
    {"1 * 2 * 3 * 4", "(* (* (* 1 2) 3) 4)"},
};

// --- Asociatividad ---------------------------------------------------------
constexpr Caso ASOCIATIVIDAD[] = {
    {"a - b - c", "(- (- a b) c)"},
    {"a / b / c", "(/ (/ a b) c)"},
    {"a ** b ** c", "(** a (** b c))"},
    {"a = b = c", "(= a (= b c))"},
    {"a ?? b ?? c", "(?? a (?? b c))"},
    {"a && b && c", "(&& (&& a b) c)"},
    {"a || b || c", "(|| (|| a b) c)"},
    {"a | b | c", "(| (| a b) c)"},
    {"a & b & c", "(& (& a b) c)"},
    {"a ^ b ^ c", "(^ (^ a b) c)"},
    {"a << b << c", "(<< (<< a b) c)"},
    {"a + b + c", "(+ (+ a b) c)"},
    {"a * b * c", "(* (* a b) c)"},
    {"a += b += c", "(+= a (+= b c))"},
    {"a << b >> c", "(>> (<< a b) c)"},
};

// --- Comparaciones ---------------------------------------------------------
constexpr Caso COMPARACIONES[] = {
    {"a == b", "(== a b)"},
    {"a != b", "(!= a b)"},
    {"a < b", "(< a b)"},
    {"a <= b", "(<= a b)"},
    {"a > b", "(> a b)"},
    {"a >= b", "(>= a b)"},
    {"a + 1 < b * 2", "(< (+ a 1) (* b 2))"},
    {"a == b + 1", "(== a (+ b 1))"},
    {"a < b == c", "(== (< a b) c)"},
    {"a && b == c", "(&& a (== b c))"},
    {"a == b && c", "(&& (== a b) c)"},
    {"a < b && c < d", "(&& (< a b) (< c d))"},
    {"!a == b", "(== (! a) b)"},
    {"!(a == b)", "(! (== a b))"},
    {"a & b == c", "(== (& a b) c)"},
    {"a == b & c", "(== a (& b c))"},
    {"a | b == c", "(== (| a b) c)"},
    {"a == b | c", "(== a (| b c))"},
    {"a < b < c", "(< (< a b) c)"},
    {"a > b >= c", "(>= (> a b) c)"},
};

// --- Lógicos ---------------------------------------------------------------
constexpr Caso LOGICOS[] = {
    {"a && b", "(&& a b)"},
    {"a || b", "(|| a b)"},
    {"a && b || c", "(|| (&& a b) c)"},
    {"a || b && c", "(|| a (&& b c))"},
    {"a && b && c && d", "(&& (&& (&& a b) c) d)"},
    {"!a && b", "(&& (! a) b)"},
    {"a || !b", "(|| a (! b))"},
    {"a && b | c", "(&& a (| b c))"},
    {"a | b && c", "(&& (| a b) c)"},
    {"a && b ?? c", "(?? (&& a b) c)"},
    {"a && b && c || d", "(|| (&& (&& a b) c) d)"},
};

// --- Bit a bit -------------------------------------------------------------
constexpr Caso BITS[] = {
    {"a | b", "(| a b)"},
    {"a ^ b", "(^ a b)"},
    {"a & b", "(& a b)"},
    {"a | b ^ c", "(| a (^ b c))"},
    {"a ^ b & c", "(^ a (& b c))"},
    {"a & b | c", "(| (& a b) c)"},
    {"a | b & c", "(| a (& b c))"},
    {"a << b | c", "(| (<< a b) c)"},
    {"a | b << c", "(| a (<< b c))"},
    {"a & b << c", "(& a (<< b c))"},
    {"a << b & c", "(& (<< a b) c)"},
    {"a & b & c | d", "(| (& (& a b) c) d)"},
};

// --- Rangos ----------------------------------------------------------------
constexpr Caso RANGOS[] = {
    {"0..10", "(.. 0 10)"},
    {"0..=10", "(..= 0 10)"},
    {"a..b", "(.. a b)"},
    {"a + 1..b * 2", "(.. (+ a 1) (* b 2))"},
    {"0..n + 1", "(.. 0 (+ n 1))"},
    {"a < b..c", "(< a (.. b c))"},
    {"a..b < c", "(< (.. a b) c)"},
    {"0..=n - 1", "(..= 0 (- n 1))"},
};

// --- Desplazamientos -------------------------------------------------------
constexpr Caso DESPLAZAMIENTOS[] = {
    {"a << b", "(<< a b)"},
    {"a >> b", "(>> a b)"},
    {"a << b + c", "(<< a (+ b c))"},
    {"a + b << c", "(<< (+ a b) c)"},
    {"a >> b | c", "(| (>> a b) c)"},
};

// --- Unarios ---------------------------------------------------------------
constexpr Caso UNARIOS[] = {
    {"-a", "(- a)"},
    {"!a", "(! a)"},
    {"- -a", "(- (- a))"},
    {"!!a", "(! (! a))"},
    {"-a + b", "(+ (- a) b)"},
    {"a + -b", "(+ a (- b))"},
    {"-a * b", "(* (- a) b)"},
    {"-(a + b)", "(- (+ a b))"},
    {"!a || b", "(|| (! a) b)"},
    {"*p", "(* p)"},
    {"&x", "(& x)"},
    {"&mutable x", "(&mutable x)"},
    {"*&x", "(* (& x))"},
    {"- - -a", "(- (- (- a)))"},
};

// --- Conversión ------------------------------------------------------------
constexpr Caso CONVERSIONES[] = {
    {"x como entero", "(como x entero)"},
    {"x como texto", "(como x texto)"},
    {"a + b como entero", "(+ a (como b entero))"},
    {"a como entero + b", "(+ (como a entero) b)"},
    {"x como &texto", "(como x &texto)"},
    {"x como &mutable texto", "(como x &mutable texto)"},
    {"-x como entero", "(como (- x) entero)"},
    {"x como entero como texto", "(como (como x entero) texto)"},
    {"x como entero == y", "(== (como x entero) y)"},
    {"f(x) como entero", "(como (llamada f x) entero)"},
    {"x como entero ** 2", "(** (como x entero) 2)"},
};

// --- Postfijos -------------------------------------------------------------
constexpr Caso POSTFIJOS[] = {
    {"f()", "(llamada f)"},
    {"f(x)", "(llamada f x)"},
    {"f(x, y)", "(llamada f x y)"},
    {"f(g(x))", "(llamada f (llamada g x))"},
    {"f(x)(y)", "(llamada (llamada f x) y)"},
    {"a.b", "(. a b)"},
    {"a.b.c", "(. (. a b) c)"},
    {"a?.b", "(?. a b)"},
    {"a.b?.c", "(?. (. a b) c)"},
    {"a[0]", "(indice a 0)"},
    {"a[i + 1]", "(indice a (+ i 1))"},
    {"a[0][1]", "(indice (indice a 0) 1)"},
    {"a.b[0]", "(indice (. a b) 0)"},
    {"a.b(c)", "(llamada (. a b) c)"},
    {"f(x)?", "(? (llamada f x))"},
    {"a?.b?", "(? (?. a b))"},
    {"a.b.c.d", "(. (. (. a b) c) d)"},
    {"f(x).y", "(. (llamada f x) y)"},
    {"a[b].c", "(. (indice a b) c)"},
    {"f()()", "(llamada (llamada f))"},
};

// --- Coalescencia ----------------------------------------------------------
constexpr Caso COALESCENCIA[] = {
    {"a ?? b", "(?? a b)"},
    {"a ?? b + c", "(?? a (+ b c))"},
    {"a + b ?? c", "(?? (+ a b) c)"},
    {"a ?? b || c", "(?? a (|| b c))"},
    {"a || b ?? c", "(?? (|| a b) c)"},
    {"x ?? 0", "(?? x 0)"},
    {"a?.b ?? c", "(?? (?. a b) c)"},
};

// --- Asignaciones ----------------------------------------------------------
constexpr Caso ASIGNACIONES[] = {
    {"a = 1", "(= a 1)"},
    {"a += 1", "(+= a 1)"},
    {"a -= 1", "(-= a 1)"},
    {"a *= 2", "(*= a 2)"},
    {"a /= 2", "(/= a 2)"},
    {"a %= 2", "(%= a 2)"},
    {"a &= 1", "(&= a 1)"},
    {"a |= 1", "(|= a 1)"},
    {"a ^= 1", "(^= a 1)"},
    {"a <<= 1", "(<<= a 1)"},
    {"a >>= 1", "(>>= a 1)"},
    {"a = b + 1", "(= a (+ b 1))"},
    {"a.b = 1", "(= (. a b) 1)"},
    {"a[0] = 1", "(= (indice a 0) 1)"},
    {"x = y ?? 1", "(= x (?? y 1))"},
};

// --- Mezclas ---------------------------------------------------------------
constexpr Caso MEZCLAS[] = {
    {"a + b * c - d / e", "(- (+ a (* b c)) (/ d e))"},
    {"(a + b) * c", "(* (+ a b) c)"},
    {"a * (b + c)", "(* a (+ b c))"},
    {"!(a && b)", "(! (&& a b))"},
    {"-a.b", "(- (. a b))"},
    {"*p + 1", "(+ (* p) 1)"},
    {"&x.y", "(& (. x y))"},
    {"a < b && c > d || e", "(|| (&& (< a b) (> c d)) e)"},
    {"x ?? y || z && w", "(?? x (|| y (&& z w)))"},
    {"f(a + b, c * d)", "(llamada f (+ a b) (* c d))"},
    {"a[b + 1] * 2", "(* (indice a (+ b 1)) 2)"},
    {"a && b || c && d", "(|| (&& a b) (&& c d))"},
    {"a == b ?? c", "(?? (== a b) c)"},
    {"a = b < c", "(= a (< b c))"},
    {"a || b || c && d", "(|| (|| a b) (&& c d))"},
    {"a ^ b ^ c ^ d", "(^ (^ (^ a b) c) d)"},
};

// --- Expresiones que deben fallar -----------------------------------------
constexpr std::string_view INVALIDAS[] = {
    "1 +",
    "(1",
    "a.",
    "f(",
    "x como",
    "1 2",
    "a..b..=c",
};

template <std::size_t N>
void ejecutar(const Caso (&casos)[N], std::string_view grupo) {
    std::cout << "  · " << grupo << " (" << N << ")\n";
    for (const Caso& caso : casos) {
        diagnostico::GestorFuentes fuentes;
        diagnostico::Emisor emisor;
        const auto archivo = fuentes.anadir("expresion.oki", std::string(caso.fuente));

        lexico::Lexer lexer(fuentes, archivo, emisor);
        sintactico::Parser parser(fuentes, archivo, emisor, lexer.todos());
        const sintactico::ExpresionPtr arbol = parser.analizar_expresion_completa();

        const std::string obtenido =
            arbol ? sintactico::imprimir(*arbol) : std::string("<sin árbol>");

        ++comprobaciones;
        if (obtenido != caso.esperado || emisor.hay_errores()) {
            ++fallos;
            std::cout << "      FALLO: " << caso.fuente << "\n"
                      << "        esperado: " << caso.esperado << "\n"
                      << "        obtenido: " << obtenido << '\n';
            if (emisor.hay_errores()) {
                std::cout << "        diagnóstico: ";
                for (const auto& diagnostico : emisor.diagnosticos()) {
                    std::cout << diagnostico.codigo() << ' ';
                }
                std::cout << '\n';
            }
        }
    }
}

}  // namespace

int main() {
    ejecutar(LITERALES, "literales");
    ejecutar(ARITMETICA, "aritmética");
    ejecutar(ASOCIATIVIDAD, "asociatividad");
    ejecutar(COMPARACIONES, "comparaciones");
    ejecutar(LOGICOS, "lógicos");
    ejecutar(BITS, "bit a bit");
    ejecutar(RANGOS, "rangos");
    ejecutar(DESPLAZAMIENTOS, "desplazamientos");
    ejecutar(UNARIOS, "unarios");
    ejecutar(CONVERSIONES, "conversiones");
    ejecutar(POSTFIJOS, "postfijos");
    ejecutar(COALESCENCIA, "coalescencia");
    ejecutar(ASIGNACIONES, "asignaciones");
    ejecutar(MEZCLAS, "mezclas");

    std::cout << "  · inválidas (" << std::size(INVALIDAS) << ")\n";
    for (const std::string_view fuente : INVALIDAS) {
        diagnostico::GestorFuentes fuentes;
        diagnostico::Emisor emisor;
        const auto archivo = fuentes.anadir("expresion.oki", std::string(fuente));

        lexico::Lexer lexer(fuentes, archivo, emisor);
        sintactico::Parser parser(fuentes, archivo, emisor, lexer.todos());
        parser.analizar_expresion_completa();

        ++comprobaciones;
        if (!emisor.hay_errores()) {
            ++fallos;
            std::cout << "      FALLO: «" << fuente << "» debería producir un error\n";
        }
    }

    std::cout << "\n  " << (comprobaciones - fallos) << "/" << comprobaciones
              << " comprobaciones correctas\n";
    return fallos == 0 ? 0 : 1;
}
