// Comportamiento en ejecución: la aritmética comprobada de ADR-03.
// Un desbordamiento o una división por cero deben detener el programa con el
// código 70 y un mensaje en la salida de error; nunca continuar con un valor
// equivocado.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "nooki/conductor/compilar.hpp"
#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/proceso.hpp"

namespace fs = std::filesystem;

namespace {

constexpr int CODIGO_PANICO = 70;

#ifdef _WIN32
constexpr const char* EXTENSION = ".exe";
#else
constexpr const char* EXTENSION = "";
#endif

struct Caso {
    std::string nombre;   // para mostrar
    std::string archivo;  // ASCII: los nombres con tildes no cruzan bien la consola
    std::string fuente;
    std::string mensaje;  // fragmento que debe aparecer en la salida de error
};

const std::vector<Caso>& casos() {
    static const std::vector<Caso> lista = {
        {"suma desbordada", "suma-desbordada",
         "funcion principal() -> entero {\n"
         "    variable a = 9223372036854775807\n"
         "    variable b = a + 1\n"
         "    imprimir(b)\n"
         "    retornar 0\n"
         "}\n",
         "desbordamiento al sumar"},
        {"resta desbordada", "resta-desbordada",
         "funcion principal() -> entero {\n"
         "    variable a = 0 - 9223372036854775807 - 1\n"
         "    variable b = a - 1\n"
         "    imprimir(b)\n"
         "    retornar 0\n"
         "}\n",
         "desbordamiento al restar"},
        {"multiplicación desbordada", "multiplicacion",
         "funcion principal() -> entero {\n"
         "    variable a = 9223372036854775807\n"
         "    variable b = a * 2\n"
         "    imprimir(b)\n"
         "    retornar 0\n"
         "}\n",
         "desbordamiento al multiplicar"},
        {"división por cero", "division-cero",
         "funcion principal() -> entero {\n"
         "    variable a = 7\n"
         "    variable b = 0\n"
         "    imprimir(a / b)\n"
         "    retornar 0\n"
         "}\n",
         "division por cero"},
        {"módulo por cero", "modulo-cero",
         "funcion principal() -> entero {\n"
         "    variable a = 7\n"
         "    variable b = 0\n"
         "    imprimir(a % b)\n"
         "    retornar 0\n"
         "}\n",
         "modulo por cero"},
        {"negación del mínimo", "negacion-minimo",
         "funcion principal() -> entero {\n"
         "    variable a = 0 - 9223372036854775807 - 1\n"
         "    variable b = -a\n"
         "    imprimir(b)\n"
         "    retornar 0\n"
         "}\n",
         "desbordamiento al negar"},
    };
    return lista;
}

std::string leer(const fs::path& ruta) {
    std::ifstream flujo(ruta, std::ios::binary);
    std::ostringstream texto;
    texto << flujo.rdbuf();
    return texto.str();
}

}  // namespace

int main() {
    const fs::path directorio = fs::temp_directory_path() / "nooki-panico";
    std::error_code error_fs;
    fs::create_directories(directorio, error_fs);

    int fallos = 0;
    for (const Caso& caso : casos()) {
        const fs::path fuente = directorio / (caso.archivo + ".oki");
        {
            std::ofstream flujo(fuente, std::ios::binary);
            flujo << caso.fuente;
        }

        nooki::diagnostico::GestorFuentes fuentes;
        nooki::diagnostico::Emisor emisor;
        nooki::conductor::Opciones opciones;
        opciones.fuente = fuente;
        opciones.salida = directorio / (caso.archivo + EXTENSION);

        std::ostringstream salida;
        std::ostringstream errores;
        const auto resultado =
            nooki::conductor::compilar(opciones, fuentes, emisor, salida, errores);
        if (resultado.codigo != 0) {
            std::cout << "FALLO " << caso.nombre << ": no compiló\n" << errores.str();
            ++fallos;
            continue;
        }

        // Se ejecuta con la salida de error a un archivo para poder revisarla.
        const fs::path errores_programa = directorio / (caso.archivo + ".err");
        const std::string comando = nooki::entrecomillar_si_hace_falta(resultado.binario.string()) +
                                    " 2> " +
                                    nooki::entrecomillar_si_hace_falta(errores_programa.string());
        const int codigo = nooki::ejecutar_comando(comando);

        const std::string mensaje = leer(errores_programa);
        const bool codigo_correcto = codigo == CODIGO_PANICO;
        const bool mensaje_correcto = mensaje.find(caso.mensaje) != std::string::npos;

        if (codigo_correcto && mensaje_correcto) {
            std::cout << "ok    " << caso.nombre << "\n";
        } else {
            std::cout << "FALLO " << caso.nombre << ": código " << codigo << " (esperado "
                      << CODIGO_PANICO << "), mensaje «" << mensaje << "»\n";
            ++fallos;
        }

        fs::remove(fuente, error_fs);
        fs::remove(resultado.binario, error_fs);
        fs::remove(errores_programa, error_fs);
    }

    if (fallos == 0) {
        std::cout << "aritmética comprobada: " << casos().size() << " casos correctos\n";
        return 0;
    }
    std::cout << fallos << " caso(s) fallido(s)\n";
    return 1;
}
