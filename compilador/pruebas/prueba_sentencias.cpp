// Pruebas del análisis de sentencias y control de flujo.
//
// Cada caso escribe un programa y su forma canónica esperada. El impresor fija por
// escrito cómo se agrupa el árbol, así que un cambio de estructura se ve al instante.

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

constexpr Caso CASOS[] = {
    // --- Funciones y bloques ---------------------------------------------
    {"funcion f() {\n}", "(programa (funcion f -> vacio))"},
    {"funcion f() -> entero {\n    retornar 1\n}",
     "(programa (funcion f -> entero (retornar 1)))"},
    {"funcion f() {\n    retornar\n}", "(programa (funcion f -> vacio (retornar)))"},
    {"funcion f() {\n    a()\n}", "(programa (funcion f -> vacio (llamada a)))"},
    {"funcion f() {\n    {}\n}", "(programa (funcion f -> vacio (bloque)))"},
    {"funcion f() {\n    { variable x = 1 }\n}",
     "(programa (funcion f -> vacio (bloque (variable x 1))))"},

    // --- Declaraciones -----------------------------------------------------
    {"funcion f() {\n    variable x = 1\n}", "(programa (funcion f -> vacio (variable x 1)))"},
    {"funcion f() {\n    variable x: entero = 1\n}",
     "(programa (funcion f -> vacio (variable x entero 1)))"},
    {"funcion f() {\n    constante M: entero = 10\n}",
     "(programa (funcion f -> vacio (constante M entero 10)))"},
    {"funcion f() {\n    variable x = 1 + 2 * 3\n}",
     "(programa (funcion f -> vacio (variable x (+ 1 (* 2 3)))))"},

    // --- Condicionales -----------------------------------------------------
    {"funcion f() {\n    si (x) { a() }\n}",
     "(programa (funcion f -> vacio (si x (llamada a))))"},
    {"funcion f() {\n    si (x) { a() } sino { b() }\n}",
     "(programa (funcion f -> vacio (si x (llamada a) (sino (llamada b)))))"},
    {"funcion f() {\n    si (x) { a() } sinosi (y) { b() } sino { c() }\n}",
     "(programa (funcion f -> vacio (si x (llamada a) (sino (si y (llamada b) (sino "
     "(llamada c)))))))"},
    {"funcion f() {\n    si (a && b) { a() }\n}",
     "(programa (funcion f -> vacio (si (&& a b) (llamada a))))"},

    // --- Bucles ------------------------------------------------------------
    {"funcion f() {\n    mientras (x) { a() }\n}",
     "(programa (funcion f -> vacio (mientras x (llamada a))))"},
    {"funcion f() {\n    para (i en 0..10) { a(i) }\n}",
     "(programa (funcion f -> vacio (para i en (.. 0 10) (llamada a i))))"},
    {"funcion f() {\n    para (i en 0..100 paso 5) { a(i) }\n}",
     "(programa (funcion f -> vacio (para i en (.. 0 100) paso 5 (llamada a i))))"},
    {"funcion f() {\n    para (e en lista) { a(e) }\n}",
     "(programa (funcion f -> vacio (para e en lista (llamada a e))))"},
    {"funcion f() {\n    hacer { a() } mientras (x)\n}",
     "(programa (funcion f -> vacio (hacer (llamada a) mientras x)))"},
    {"funcion f() {\n    bucle { romper }\n}",
     "(programa (funcion f -> vacio (bucle (romper))))"},
    {"funcion f() {\n    mientras (x) { continuar }\n}",
     "(programa (funcion f -> vacio (mientras x (continuar))))"},

    // --- Combinaciones -----------------------------------------------------
    {"funcion f() {\n    mientras (x) { si (y) { romper } }\n}",
     "(programa (funcion f -> vacio (mientras x (si y (romper)))))"},
    {"funcion f() {\n    para (i en 0..3) { bucle { romper } }\n}",
     "(programa (funcion f -> vacio (para i en (.. 0 3) (bucle (romper)))))"},
    {"funcion f() {\n    x = 1\n}",
     "(programa (funcion f -> vacio (= x 1)))"},
    {"funcion f() {\n    a()\n}\nfuncion g() {\n    b()\n}",
     "(programa (funcion f -> vacio (llamada a)) (funcion g -> vacio (llamada b)))"},
    {"funcion f() {\n    si (x) {\n        variable y = 1\n        a(y)\n    }\n}",
     "(programa (funcion f -> vacio (si x (variable y 1) (llamada a y))))"},

    // --- Sentencias tras un retorno ---------------------------------------
    {"funcion f() -> entero {\n    retornar 0\n    retornar 1\n}",
     "(programa (funcion f -> entero (retornar 0) (retornar 1)))"},

    // --- Parámetros --------------------------------------------------------
    {"funcion f(a: entero) -> entero {\n    retornar a\n}",
     "(programa (funcion f (a entero) -> entero (retornar a)))"},
    {"funcion f(a: entero, b: texto) {\n}",
     "(programa (funcion f (a entero, b texto) -> vacio))"},
    {"funcion f() {\n    g(1, \"dos\")\n}", "(programa (funcion f -> vacio (llamada g 1 \"dos\")))"},

    // --- Tipos -------------------------------------------------------------
    {"estructura punto {\n    publico x: entero\n    publico y: entero\n}",
     "(programa (estructura punto (x entero) (y entero)))"},
    {"clase persona {\n    privado edad: entero\n}",
     "(programa (clase persona (edad entero)))"},
    {"funcion f() {\n    variable p = punto { x: 1, y: 2 }\n}",
     "(programa (funcion f -> vacio (variable p (construir punto (x 1) (y 2)))))"},
    {"funcion f() {\n    variable p = punto { x: 1 }\n    p.x = 2\n}",
     "(programa (funcion f -> vacio (variable p (construir punto (x 1))) (= (. p x) 2)))"},

    // --- Métodos -----------------------------------------------------------
    {"estructura punto {\n    publico funcion f() -> entero {\n        retornar este.x\n    }\n}",
     "(programa (estructura punto (funcion f -> entero (retornar (. este x)))))"},
    {"funcion f() {\n    variable p = punto { x: 1 }\n    p.mover(5)\n}",
     "(programa (funcion f -> vacio (variable p (construir punto (x 1))) (llamada (. p mover) "
     "5)))"},

    // --- Enumeraciones y patrones -----------------------------------------
    {"enumeracion color {\n    rojo,\n    verde,\n}",
     "(programa (enumeracion color rojo verde))"},
    {"funcion f() {\n    coincide (x) {\n        caso 1 => a()\n        caso _ => b()\n    }\n}",
     "(programa (funcion f -> vacio (coincide x (rama 1 (llamada a)) (rama _ (llamada b)))))"},
    {"funcion f() {\n    coincide (c) {\n        caso color.rojo => a()\n        caso _ => b()\n"
     "    }\n}",
     "(programa (funcion f -> vacio (coincide c (rama color.rojo (llamada a)) (rama _ "
     "(llamada b)))))"},
};

constexpr std::string_view INVALIDOS[] = {
    "variable = 1",
    "si x { }",
    "mientras { }",
    "para (i 0..10) { }",
    "funcion f( {",
    "coincide (x) { }",
    "funcion f(a) { }",
};

void analizar_y_comparar(const Caso& caso) {
    diagnostico::GestorFuentes fuentes;
    diagnostico::Emisor emisor;
    const auto archivo = fuentes.anadir("programa.oki", std::string(caso.fuente));

    lexico::Lexer lexer(fuentes, archivo, emisor);
    sintactico::Parser parser(fuentes, archivo, emisor, lexer.todos());
    const sintactico::Programa programa = parser.analizar();
    const std::string obtenido = sintactico::imprimir(programa);

    ++comprobaciones;
    if (obtenido != caso.esperado || emisor.hay_errores()) {
        ++fallos;
        std::cout << "    FALLO en: " << caso.fuente << "\n"
                  << "      esperado: " << caso.esperado << "\n"
                  << "      obtenido: " << obtenido << '\n';
        for (const auto& diagnostico : emisor.diagnosticos()) {
            std::cout << "      diagnóstico: " << diagnostico.codigo() << '\n';
        }
    }
}

}  // namespace

int main() {
    std::cout << "  · programas válidos (" << std::size(CASOS) << ")\n";
    for (const Caso& caso : CASOS) {
        analizar_y_comparar(caso);
    }

    std::cout << "  · programas inválidos (" << std::size(INVALIDOS) << ")\n";
    for (const std::string_view fuente : INVALIDOS) {
        diagnostico::GestorFuentes fuentes;
        diagnostico::Emisor emisor;
        const auto archivo = fuentes.anadir("programa.oki", std::string(fuente));

        lexico::Lexer lexer(fuentes, archivo, emisor);
        sintactico::Parser parser(fuentes, archivo, emisor, lexer.todos());
        parser.analizar();

        ++comprobaciones;
        if (!emisor.hay_errores()) {
            ++fallos;
            std::cout << "    FALLO: «" << fuente << "» debería producir un error\n";
        }
    }

    std::cout << "\n  " << (comprobaciones - fallos) << "/" << comprobaciones
              << " comprobaciones correctas\n";
    return fallos == 0 ? 0 : 1;
}
