// Pruebas de la interfaz de línea de comandos.
//
// No se usa ningún marco externo: la v0.1 se compila sin dependencias de terceros.
// Cuando el proyecto crezca (S05) se añadirá el marco de conformidad y, si hace falta,
// Google Test mediante vcpkg.

#include "nooki/cli/comandos.hpp"
#include "nooki/cli/orden.hpp"
#include "nooki/rutas.hpp"
#include "nooki/version.hpp"

#include <filesystem>
#include <fstream>
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
    caso("verificar");
    {
        const std::filesystem::path directorio =
            std::filesystem::temp_directory_path() / "nooki-verificar";
        std::error_code ignorado;
        std::filesystem::create_directories(directorio, ignorado);
        const std::filesystem::path bueno = directorio / "bueno.oki";
        const std::filesystem::path malo = directorio / "malo.oki";
        {
            std::ofstream flujo(bueno, std::ios::binary);
            flujo << "funcion principal() -> entero {\n    retornar 0\n}\n";
        }
        {
            std::ofstream flujo(malo, std::ios::binary);
            flujo << "funcion principal() -> entero {\n    variable x = y + 1\n"
                     "    retornar 0\n}\n";
        }

        const Resultado correcto = invocar({"verificar", nooki::ruta_a_utf8(bueno)});
        comprobar(correcto.codigo == nooki::cli::EXITO, "un archivo correcto pasa");
        comprobar(contiene(correcto.salida, "correcto"), "lo dice en la salida");

        const Resultado con_error = invocar({"verificar", nooki::ruta_a_utf8(malo)});
        comprobar(con_error.codigo == nooki::cli::DATOS_INCORRECTOS,
                  "un archivo con error devuelve datos incorrectos");
        comprobar(contiene(con_error.error, "NOK3001"), "señala el código del error");

        const Resultado sin_archivo = invocar({"verificar"});
        comprobar(sin_archivo.codigo == nooki::cli::USO_INCORRECTO, "sin archivo es uso incorrecto");

        std::filesystem::remove_all(directorio, ignorado);
    }

    caso("formatear");
    {
        const std::filesystem::path directorio =
            std::filesystem::temp_directory_path() / "nooki-formatear";
        std::error_code ignorado;
        std::filesystem::create_directories(directorio, ignorado);
        const std::filesystem::path archivo = directorio / "desordenado.oki";
        {
            std::ofstream flujo(archivo, std::ios::binary);
            flujo << "funcion principal() -> entero {\n"
                     "        variable x = 1\n"
                     "   retornar x\n"
                     "}\n";
        }

        const std::string ruta = nooki::ruta_a_utf8(archivo);
        const Resultado resultado = invocar({"formatear", ruta});
        comprobar(resultado.codigo == nooki::cli::EXITO, "formatea sin error");
        comprobar(contiene(resultado.salida, "formateado"), "avisa de lo que ha hecho");

        std::ifstream lectura(archivo, std::ios::binary);
        std::ostringstream contenido;
        contenido << lectura.rdbuf();
        const std::string texto = contenido.str();
        comprobar(texto.find("    variable x = 1") != std::string::npos,
                  "deja la variable con cuatro espacios");
        comprobar(texto.find("    retornar x") != std::string::npos,
                  "deja el retorno al mismo nivel");

        // Formatear dos veces no cambia nada, y `--comprobar` lo detecta.
        const Resultado segunda = invocar({"formatear", ruta, "--comprobar"});
        comprobar(segunda.codigo == nooki::cli::EXITO, "ya estaba formateado");

        std::filesystem::remove_all(directorio, ignorado);
    }

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
        const Resultado resultado = invocar({"documentar", "hola.oki"});
        comprobar(resultado.codigo == nooki::cli::NO_DISPONIBLE, "devuelve 69");
        comprobar(contiene(resultado.error, "Sprint 60"), "indica en qué sprint llega");
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
