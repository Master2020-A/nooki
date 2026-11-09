// Pruebas de la verificación semántica: nombres, aridad, tipos, condiciones y retornos.

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/semantico/verificaciones.hpp"
#include "nooki/sintactico/parser.hpp"

namespace {

using namespace nooki;

int comprobaciones = 0;
int fallos = 0;

struct Caso {
    std::string_view fuente;
    std::string_view esperado;  // "" = no debe haber errores
};

/// Extrae los códigos de los diagnósticos, ordenados, para compararlos como conjunto.
std::vector<std::string> codigos(const diagnostico::Emisor& emisor) {
    std::vector<std::string> lista;
    for (const auto& diagnostico : emisor.diagnosticos()) {
        lista.emplace_back(diagnostico.codigo());
    }
    std::sort(lista.begin(), lista.end());
    return lista;
}

std::string unir(const std::vector<std::string>& valores) {
    if (valores.empty()) {
        return "ninguno";
    }
    std::string salida;
    for (std::size_t i = 0; i < valores.size(); ++i) {
        if (i != 0) {
            salida += ", ";
        }
        salida += valores[i];
    }
    return salida;
}

void ejecutar(const Caso& caso) {
    diagnostico::GestorFuentes fuentes;
    diagnostico::Emisor emisor;
    const auto archivo = fuentes.anadir("programa.oki", std::string(caso.fuente));

    lexico::Lexer lexer(fuentes, archivo, emisor);
    sintactico::Parser parser(fuentes, archivo, emisor, lexer.todos());
    const sintactico::Programa programa = parser.analizar();

    ++comprobaciones;

    if (emisor.hay_errores()) {
        ++fallos;
        std::cout << "    FALLO de sintaxis en: " << caso.fuente << '\n';
        return;
    }

    semantico::verificar(programa, emisor);
    const std::vector<std::string> obtenidos = codigos(emisor);

    std::vector<std::string> esperados;
    if (!caso.esperado.empty()) {
        esperados.emplace_back(caso.esperado);
    }

    if (obtenidos != esperados) {
        ++fallos;
        std::cout << "    FALLO en: " << caso.fuente << "\n"
                  << "      esperado: " << unir(esperados) << "\n"
                  << "      obtenido: " << unir(obtenidos) << '\n';
    }
}

// Programas que deben pasar la verificación sin un solo diagnóstico.
constexpr Caso VALIDOS[] = {
    {"funcion principal() -> entero {\n    retornar 0\n}", ""},
    {"funcion f(n: entero) -> entero {\n    retornar n\n}\n"
     "funcion principal() -> entero {\n    retornar f(1)\n}",
     ""},
    {"funcion principal() -> entero {\n    variable x = 1\n    x = 2\n    retornar x\n}", ""},
    {"funcion principal() -> entero {\n    si (1 < 2) { imprimir(\"sí\") }\n    retornar 0\n}",
     ""},
    {"funcion principal() -> entero {\n    variable b = 1 < 2\n    si (b) { imprimir(\"sí\") }\n"
     "    retornar 0\n}",
     ""},
    {"funcion principal() -> entero {\n    para (i en 0..10) { imprimir(i) }\n    retornar 0\n}",
     ""},
    {"funcion eco(t: texto) -> texto {\n    retornar t\n}\n"
     "funcion principal() -> entero {\n    imprimir(eco(\"hola\"))\n    retornar 0\n}",
     ""},
    {"funcion principal() -> entero {\n    variable x = 0\n    mientras (x < 10) {\n"
     "        x = x + 1\n    }\n    retornar x\n}",
     ""},
    {"funcion factorial(n: entero) -> entero {\n    si (n <= 1) { retornar 1 }\n"
     "    retornar n * factorial(n - 1)\n}\n"
     "funcion principal() -> entero {\n    retornar factorial(5)\n}",
     ""},
    {"funcion principal() -> entero {\n    si (1 < 2) { imprimir(1) } sino { imprimir(\"dos\") }\n"
     "    retornar 0\n}",
     ""},

    // --- Estructuras -------------------------------------------------------
    {"estructura punto {\n    publico x: entero\n    publico y: entero\n}\n"
     "funcion principal() -> entero {\n    variable p = punto { x: 1, y: 2 }\n"
     "    retornar p.x\n}",
     ""},
    {"clase persona {\n    publico nombre: texto\n    publico edad: entero\n}\n"
     "funcion principal() -> entero {\n    variable a = persona { nombre: \"Ana\", edad: 30 }\n"
     "    retornar a.edad\n}",
     ""},
    {"estructura punto {\n    publico x: entero\n\n"
     "    publico funcion doble() -> entero {\n        retornar este.x + este.x\n    }\n}\n"
     "funcion principal() -> entero {\n    variable p = punto { x: 2 }\n"
     "    retornar p.doble()\n}",
     ""},

    // --- Enumeraciones y patrones -----------------------------------------
    {"enumeracion color { rojo, verde }\n"
     "funcion principal() -> entero {\n    variable c = color.verde\n"
     "    coincide (c) {\n        caso color.rojo => imprimir(\"rojo\")\n"
     "        caso _ => imprimir(\"otro\")\n    }\n    retornar 0\n}",
     ""},
};

// Programas que deben producir exactamente el código indicado.
constexpr Caso INVALIDOS[] = {
    {"funcion principal() -> entero {\n    retornar x\n}", "NOK3001"},
    {"funcion principal() -> entero {\n    imprimir(noexiste())\n    retornar 0\n}", "NOK3002"},
    {"funcion f(n: entero) -> entero {\n    retornar n\n}\n"
     "funcion principal() -> entero {\n    retornar f()\n}",
     "NOK4003"},
    {"funcion f(n: entero) -> entero {\n    retornar n\n}\n"
     "funcion principal() -> entero {\n    retornar f(\"texto\")\n}",
     "NOK4002"},
    {"funcion principal() -> entero {\n    variable x: entero = \"a\"\n    retornar 0\n}", "NOK4001"},
    {"funcion principal() -> entero {\n    variable x = 1 + \"a\"\n    retornar 0\n}", "NOK4001"},
    {"funcion principal() -> entero {\n    si (1) { }\n    retornar 0\n}", "NOK4005"},
    {"funcion principal() -> entero {\n    mientras (\"a\") { }\n    retornar 0\n}", "NOK4005"},
    {"funcion f() -> entero {\n    retornar \"a\"\n}\n"
     "funcion principal() -> entero {\n    retornar f()\n}",
     "NOK4004"},
    {"funcion f() -> entero {\n    retornar\n}\n"
     "funcion principal() -> entero {\n    retornar 0\n}",
     "NOK4004"},
    {"funcion f() {\n    retornar 1\n}\n"
     "funcion principal() -> entero {\n    retornar 0\n}",
     "NOK4004"},
    {"funcion principal() -> entero {\n    variable x = 1\n    x = \"a\"\n    retornar x\n}",
     "NOK4001"},

    // --- Estructuras -------------------------------------------------------
    {"funcion principal() -> entero {\n    variable p = desconocido { x: 1 }\n    retornar 0\n}",
     "NOK3003"},
    {"estructura punto {\n    publico x: entero\n    publico y: entero\n}\n"
     "funcion principal() -> entero {\n    variable p = punto { x: 1, y: 2, z: 3 }\n"
     "    retornar 0\n}",
     "NOK3004"},
    {"estructura punto {\n    publico x: entero\n    publico y: entero\n}\n"
     "funcion principal() -> entero {\n    variable p = punto { x: 1 }\n    retornar 0\n}",
     "NOK3005"},
    {"estructura punto {\n    publico x: entero\n}\n"
     "funcion principal() -> entero {\n    variable p = punto { x: \"a\" }\n    retornar 0\n}",
     "NOK4001"},

    // --- Métodos -----------------------------------------------------------
    {"estructura punto {\n    publico x: entero\n}\n"
     "funcion principal() -> entero {\n    variable p = punto { x: 1 }\n"
     "    retornar p.desconocido()\n}",
     "NOK3006"},
    {"estructura punto {\n    publico x: entero\n\n"
     "    publico funcion suma(otro: punto) -> entero {\n        retornar este.x\n    }\n}\n"
     "funcion principal() -> entero {\n    variable p = punto { x: 1 }\n"
     "    variable q = punto { x: 2 }\n    retornar p.suma()\n}",
     "NOK4003"},
    {"estructura punto {\n    publico x: entero\n\n"
     "    publico funcion suma(otro: punto) -> entero {\n        retornar este.x\n    }\n}\n"
     "funcion principal() -> entero {\n    variable p = punto { x: 1 }\n"
     "    retornar p.suma(5)\n}",
     "NOK4002"},

    // --- Enumeraciones y patrones -----------------------------------------
    {"enumeracion color { rojo, verde }\n"
     "funcion principal() -> entero {\n    variable c = color.azul\n    retornar 0\n}",
     "NOK3007"},
    {"enumeracion color { rojo, verde }\n"
     "funcion principal() -> entero {\n    variable c = color.rojo\n"
     "    coincide (c) {\n        caso color.rojo => imprimir(\"rojo\")\n    }\n"
     "    retornar 0\n}",
     "NOK4006"},
};

}  // namespace

int main() {
    std::cout << "  · programas válidos (" << std::size(VALIDOS) << ")\n";
    for (const Caso& caso : VALIDOS) {
        ejecutar(caso);
    }

    std::cout << "  · programas inválidos (" << std::size(INVALIDOS) << ")\n";
    for (const Caso& caso : INVALIDOS) {
        ejecutar(caso);
    }

    std::cout << "\n  " << (comprobaciones - fallos) << "/" << comprobaciones
              << " comprobaciones correctas\n";
    return fallos == 0 ? 0 : 1;
}
