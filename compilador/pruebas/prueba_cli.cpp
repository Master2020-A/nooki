// Pruebas de la interfaz de línea de comandos.
//
// No se usa ningún marco externo: la v0.1 se compila sin dependencias de terceros.
// Cuando el proyecto crezca (S05) se añadirá el marco de conformidad y, si hace falta,
// Google Test mediante vcpkg.

#include "nooki/cli/comandos.hpp"
#include "nooki/cli/orden.hpp"
#include "nooki/version.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

int comprobaciones = 0;
int fallos = 0;

void comprobar(bool condicion, std::string_view descripcion) {
    ++comprobaciones;
    if (!condicion) {
        ++fallos;
        std::cerr << "    FALLO: " << descripcion << '\n';
    }
}

struct Resultado {
    int codigo = 0;
    std::string salida;
    std::string error;
};

Resultado invocar(std::vector<std::string_view> argumentos) {
    std::ostringstream salida;
    std::ostringstream error;
    const int codigo = nooki::cli::ejecutar(argumentos, salida, error);
    return Resultado{codigo, salida.str(), error.str()};
}

bool contiene(const std::string& texto, std::string_view buscado) {
    return texto.find(buscado) != std::string::npos;
}

void caso(std::string_view titulo) {
    std::cout << "  · " << titulo << '\n';
}

}  // namespace

int main() {
    const std::string version_esperada = std::string("nooki ") + nooki::version::TEXTO;

    caso("compilar sin archivo");
    {
        const Resultado resultado = invocar({"compilar"});
        comprobar(resultado.codigo == nooki::cli::USO_INCORRECTO, "devuelve uso incorrecto");
        comprobar(contiene(resultado.error, "uso: nooki compilar"), "explica cómo se usa");
    }

    caso("compilar con -o sin ruta");
    {
        const Resultado resultado = invocar({"compilar", "x.oki", "-o"});
        comprobar(resultado.codigo == nooki::cli::USO_INCORRECTO, "devuelve uso incorrecto");
        comprobar(contiene(resultado.error, "falta la ruta"), "dice qué falta");
    }

    caso("compilar un archivo que no existe");
    {
        const Resultado resultado = invocar({"compilar", "no-existe-jamas.oki"});
        comprobar(resultado.codigo == nooki::cli::DATOS_INCORRECTOS,
                  "devuelve datos incorrectos");
        comprobar(contiene(resultado.error, "NOK0001"), "lo marca con NOK0001");
    }

    caso("version");
    {
        const Resultado resultado = invocar({"version"});
        comprobar(resultado.codigo == nooki::cli::EXITO, "devuelve éxito");
        comprobar(contiene(resultado.salida, version_esperada), "imprime la versión");
        comprobar(resultado.error.empty(), "no escribe en la salida de error");
    }

    caso("--version y -v");
    comprobar(invocar({"--version"}).codigo == nooki::cli::EXITO, "--version devuelve éxito");
    comprobar(invocar({"-v"}).codigo == nooki::cli::EXITO, "-v devuelve éxito");

    caso("ayuda");
    {
        const Resultado resultado = invocar({"ayuda"});
        comprobar(resultado.codigo == nooki::cli::EXITO, "devuelve éxito");
        comprobar(contiene(resultado.salida, "Uso:"), "explica el uso");
        comprobar(contiene(resultado.salida, "Códigos de salida"),
                  "documenta los códigos de salida");
    }

    caso("sin argumentos");
    {
        const Resultado resultado = invocar({});
        comprobar(resultado.codigo == nooki::cli::EXITO, "devuelve éxito");
        comprobar(contiene(resultado.salida, "Comandos:"), "muestra la ayuda");
        comprobar(resultado.error.empty(), "no escribe en la salida de error");
    }

    caso("la ayuda incluye la tabla de comandos");
    {
        const std::string ayuda = invocar({"ayuda"}).salida;
        bool todos_presentes = true;
        for (const nooki::cli::Comando& comando : nooki::cli::COMANDOS) {
            if (!contiene(ayuda, comando.nombre)) {
                todos_presentes = false;
                std::cerr << "    falta en la ayuda: " << comando.nombre << '\n';
            }
        }
        comprobar(todos_presentes, "aparecen todos los comandos");
    }

    caso("comando todavía no disponible");
    {
        const Resultado resultado = invocar({"formatear", "hola.oki"});
        comprobar(resultado.codigo == nooki::cli::NO_DISPONIBLE, "devuelve 69");
        comprobar(contiene(resultado.error, "Sprint 56"), "indica en qué sprint llega");
        comprobar(resultado.salida.empty(), "no escribe en la salida estándar");
    }

    caso("comando inexistente");
    {
        const Resultado resultado = invocar({"compilra"});
        comprobar(resultado.codigo == nooki::cli::USO_INCORRECTO, "devuelve 64");
        comprobar(contiene(resultado.error, "no existe"), "explica el error");
        comprobar(contiene(resultado.error, "compilar"), "sugiere el comando correcto");
    }

    caso("opción inexistente");
    {
        const Resultado resultado = invocar({"--turbio"});
        comprobar(resultado.codigo == nooki::cli::USO_INCORRECTO, "devuelve 64");
        comprobar(contiene(resultado.error, "--turbio"), "menciona la opción");
    }

    std::cout << "\n  " << (comprobaciones - fallos) << "/" << comprobaciones
              << " comprobaciones correctas\n";
    return fallos == 0 ? 0 : 1;
}
