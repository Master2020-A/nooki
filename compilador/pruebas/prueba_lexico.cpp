// Pruebas unitarias del analizador léxico.

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/lexico/lexer.hpp"

namespace {

using namespace nooki;

int comprobaciones = 0;
int fallos = 0;

void comprobar(bool condicion, std::string_view descripcion) {
    ++comprobaciones;
    if (!condicion) {
        ++fallos;
        std::cerr << "    FALLO: " << descripcion << '\n';
    }
}

void caso(std::string_view titulo) { std::cout << "  · " << titulo << '\n'; }

struct Analisis {
    diagnostico::GestorFuentes fuentes;
    diagnostico::Emisor emisor;
    std::vector<lexico::Token> tokens;
};

void analizar(Analisis& analisis, const std::string& fuente) {
    const auto archivo = analisis.fuentes.anadir("prueba.oki", fuente);
    lexico::Lexer lexer(analisis.fuentes, archivo, analisis.emisor);
    analisis.tokens = lexer.todos();
}

std::size_t cantidad(const Analisis& analisis, lexico::TipoToken tipo) {
    std::size_t total = 0;
    for (const auto& token : analisis.tokens) {
        if (token.tipo == tipo) {
            ++total;
        }
    }
    return total;
}

std::string primer_codigo(const Analisis& analisis) {
    return analisis.emisor.cantidad() == 0
               ? std::string{}
               : std::string(analisis.emisor.diagnosticos().front().codigo());
}

}  // namespace

int main() {
    caso("archivo vacío");
    {
        Analisis a;
        analizar(a, "");
        comprobar(a.tokens.size() == 1, "solo produce el fin de archivo");
        comprobar(a.tokens.front().es(lexico::TipoToken::FinDeArchivo), "el token es FinDeArchivo");
        comprobar(a.emisor.cantidad() == 0, "sin diagnósticos");
    }

    caso("declaración simple");
    {
        Analisis a;
        analizar(a, "variable x = 5");
        comprobar(a.tokens.size() == 5, "cinco tokens");
        comprobar(a.tokens[0].es(lexico::TipoToken::PalabraClave), "variable es palabra clave");
        comprobar(a.tokens[1].es(lexico::TipoToken::Identificador), "x es identificador");
        comprobar(a.tokens[2].es(lexico::TipoToken::Operador), "= es operador");
        comprobar(a.tokens[3].es(lexico::TipoToken::Entero), "5 es entero");
        comprobar(a.tokens[1].span.inicio == 9 && a.tokens[1].span.fin == 10,
                  "el span de x es correcto");
        comprobar(a.tokens[3].lexema == "5", "el lexema se conserva");
    }

    caso("palabras clave");
    {
        Analisis a;
        analizar(a, "si mientras entero Este");
        comprobar(cantidad(a, lexico::TipoToken::PalabraClave) == 4, "cuatro palabras clave");
        comprobar(a.emisor.cantidad() == 0, "sin diagnósticos");
    }

    caso("identificadores con tilde y eñe");
    {
        Analisis a;
        analizar(a, "año niño _privado valor2");
        comprobar(cantidad(a, lexico::TipoToken::Identificador) == 4, "cuatro identificadores");
        comprobar(a.emisor.cantidad() == 0, "sin diagnósticos");
    }

    caso("palabra clave con tilde");
    {
        Analisis a;
        analizar(a, "función principal()");
        comprobar(a.tokens[0].es(lexico::TipoToken::Identificador), "se tokeniza como identificador");
        comprobar(primer_codigo(a) == "NOK1001", "emite NOK1001");
        comprobar(a.emisor.cantidad() == 1, "un solo diagnóstico");
    }

    caso("números");
    {
        Analisis a;
        analizar(a, "0xFF 0b1010 0o755 1_000");
        comprobar(cantidad(a, lexico::TipoToken::Entero) == 4, "cuatro enteros");
        comprobar(a.emisor.cantidad() == 0, "sin diagnósticos");
    }
    {
        Analisis a;
        analizar(a, "3.14 1e-9 2.5E10");
        comprobar(cantidad(a, lexico::TipoToken::Flotante) == 3, "tres flotantes");
    }
    {
        Analisis a;
        analizar(a, "0..10");
        comprobar(cantidad(a, lexico::TipoToken::Entero) == 2, "dos enteros");
        comprobar(a.tokens[1].lexema == "..", "el rango no se confunde con un flotante");
    }

    caso("textos");
    {
        Analisis a;
        analizar(a, R"(variable s = "hola\n\tmundo \u{1F600}")");
        comprobar(cantidad(a, lexico::TipoToken::Texto) == 1, "un texto");
        comprobar(a.emisor.cantidad() == 0, "sin diagnósticos");
    }
    {
        Analisis a;
        analizar(a, "variable s = \"sin cerrar");
        comprobar(primer_codigo(a) == "NOK1003", "emite NOK1003");
    }

    caso("caracteres y tiempos de vida");
    {
        Analisis a;
        analizar(a, "variable c = 'ñ'");
        comprobar(cantidad(a, lexico::TipoToken::Caracter) == 1, "un carácter");
        comprobar(a.emisor.cantidad() == 0, "sin diagnósticos");
    }
    {
        Analisis a;
        analizar(a, "funcion f<'v>(x: &'v texto)");
        comprobar(cantidad(a, lexico::TipoToken::Vida) == 2, "dos tiempos de vida");
        comprobar(a.emisor.cantidad() == 0, "sin diagnósticos");
    }

    caso("comentarios");
    {
        Analisis a;
        analizar(a, "// línea\n/// documento\nvariable y = 1");
        comprobar(cantidad(a, lexico::TipoToken::Documentacion) == 1, "un comentario de documento");
        comprobar(cantidad(a, lexico::TipoToken::PalabraClave) == 1, "la declaración se reconoce");
    }

    caso("UTF-8 inválido");
    {
        Analisis a;
        analizar(a, std::string("a") + "\xFF" + " b");
        comprobar(primer_codigo(a) == "NOK1006", "emite NOK1006");
    }

    caso("nuevas líneas");
    {
        Analisis a;
        analizar(a, "a\nb\n");
        comprobar(cantidad(a, lexico::TipoToken::NuevaLinea) == 2, "dos saltos de línea");
    }

    std::cout << "\n  " << (comprobaciones - fallos) << "/" << comprobaciones
              << " comprobaciones correctas\n";
    return fallos == 0 ? 0 : 1;
}
