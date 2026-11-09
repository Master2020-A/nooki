// Pruebas de la rebanada vertical: analizar, generar C y, si hay compilador de C,
// producir un binario y ejecutarlo.

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "nooki/conductor/compilar.hpp"
#include "nooki/diagnostico/codigos.hpp"
#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/generador/generador_c.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/proceso.hpp"
#include "nooki/sintactico/parser.hpp"

namespace {

using namespace nooki;

int comprobaciones = 0;
int fallos = 0;
int omitidas = 0;

void comprobar(bool condicion, std::string_view descripcion) {
    ++comprobaciones;
    if (!condicion) {
        ++fallos;
        std::cerr << "    FALLO: " << descripcion << '\n';
    }
}

void omitir(std::string_view descripcion) {
    ++omitidas;
    std::cout << "    omitido: " << descripcion << '\n';
}

void caso(std::string_view titulo) { std::cout << "  · " << titulo << '\n'; }

bool contiene(const std::string& texto, std::string_view buscado) {
    return texto.find(buscado) != std::string::npos;
}

std::string leer(const std::filesystem::path& ruta) {
    std::ifstream entrada(ruta, std::ios::binary);
    std::ostringstream contenido;
    contenido << entrada.rdbuf();
    return contenido.str();
}

void escribir(const std::filesystem::path& ruta, const std::string& contenido) {
    std::ofstream salida(ruta, std::ios::binary | std::ios::trunc);
    salida << contenido;
}

std::string ejecutar(const std::filesystem::path& binario) {
    const std::filesystem::path captura =
        std::filesystem::temp_directory_path() / "nooki_prueba_salida.txt";
    nooki::ejecutar_comando("\"" + binario.string() + "\" > \"" + captura.string() + "\" 2>&1");
    const std::string salida = leer(captura);
    std::error_code ignorado;
    std::filesystem::remove(captura, ignorado);
    return salida;
}

constexpr std::string_view FUENTE_HOLA =
    "funcion principal() -> entero {\n"
    "    imprimir(\"¡Hola, Nooki!\")\n"
    "    retornar 0\n"
    "}\n";

}  // namespace

int main() {
    caso("generación de C");
    {
        diagnostico::GestorFuentes fuentes;
        diagnostico::Emisor emisor;
        const auto archivo = fuentes.anadir("hola.oki", std::string(FUENTE_HOLA));
        lexico::Lexer lexer(fuentes, archivo, emisor);
        sintactico::Parser parser(fuentes, archivo, emisor, lexer.todos());
        const auto programa = parser.analizar();

        comprobar(emisor.cantidad() == 0, "el programa no tiene errores");
        comprobar(programa.funciones.size() == 1, "hay una función");

        const generador::ResultadoGeneracion generado = generador::generar_c(programa);
        const std::string& c = generado.codigo;
        comprobar(generado.soportado, "el backend soporta este programa");
        comprobar(contiene(c, "int main(void)"), "principal se convierte en main");
        comprobar(contiene(c, "nooki_imprimir_texto(\"¡Hola, Nooki!\")"),
                  "la llamada se traduce al runtime");
        comprobar(contiene(c, "return 0;"), "el retorno se traduce");
        comprobar(contiene(c, "#include <stdio.h>"), "incluye la cabecera de C");
    }

    caso("error sintáctico");
    {
        diagnostico::GestorFuentes fuentes;
        diagnostico::Emisor emisor;
        const auto archivo = fuentes.anadir("malo.oki", "funcion principal( {\n}\n");
        lexico::Lexer lexer(fuentes, archivo, emisor);
        sintactico::Parser parser(fuentes, archivo, emisor, lexer.todos());
        parser.analizar();

        comprobar(emisor.hay_errores(), "se detecta el error");
        bool hay_sintactico = false;
        for (const auto& diagnostico : emisor.diagnosticos()) {
            if (diagnostico.codigo() == diagnostico::codigos::TOKEN_INESPERADO) {
                hay_sintactico = true;
            }
        }
        comprobar(hay_sintactico, "el código es NOK2002");
    }

    caso("solo generar C");
    {
        conductor::Opciones opciones;
        opciones.fuente = std::filesystem::temp_directory_path() / "nooki_solo_c.oki";
        opciones.solo_c = true;
        escribir(opciones.fuente, std::string(FUENTE_HOLA));

        diagnostico::GestorFuentes fuentes;
        diagnostico::Emisor emisor;
        std::ostringstream salida;
        std::ostringstream error;

        const auto resultado = conductor::compilar(opciones, fuentes, emisor, salida, error);
        comprobar(resultado.codigo == 0, "termina bien");
        comprobar(contiene(salida.str(), "int main(void)"), "escribe el C en la salida");

        std::error_code ignorado;
        std::filesystem::remove(opciones.fuente, ignorado);
    }

    caso("compilar y ejecutar");
    {
        if (conductor::compilador_c().empty()) {
            omitir("no hay compilador de C en esta máquina");
        } else {
            const std::filesystem::path directorio =
                std::filesystem::temp_directory_path() / "nooki_prueba_compilador";
            std::filesystem::create_directories(directorio);

            conductor::Opciones opciones;
            opciones.fuente = directorio / "hola.oki";
#ifdef _WIN32
            opciones.salida = directorio / "hola.exe";
#else
            opciones.salida = directorio / "hola";
#endif
            escribir(opciones.fuente, std::string(FUENTE_HOLA));

            diagnostico::GestorFuentes fuentes;
            diagnostico::Emisor emisor;
            std::ostringstream salida;
            std::ostringstream error;

            const auto resultado = conductor::compilar(opciones, fuentes, emisor, salida, error);
            comprobar(resultado.codigo == 0, "la compilación termina bien");
            comprobar(std::filesystem::exists(resultado.binario), "el binario existe");

            const std::string salida_programa = nooki::normalizar_saltos(ejecutar(resultado.binario));
            comprobar(salida_programa == "¡Hola, Nooki!\n",
                      "el programa imprime el saludo esperado");

            std::error_code ignorado;
            std::filesystem::remove_all(directorio, ignorado);
        }
    }

    std::cout << "\n  " << (comprobaciones - fallos) << "/" << comprobaciones
              << " comprobaciones correctas";
    if (omitidas > 0) {
        std::cout << " (" << omitidas << " omitidas)";
    }
    std::cout << '\n';
    return fallos == 0 ? 0 : 1;
}
