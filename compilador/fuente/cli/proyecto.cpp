#include <filesystem>
#include <fstream>
#include <ostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/cli/comandos.hpp"
#include "nooki/cli/compilar.hpp"
#include "nooki/manifiesto.hpp"
#include "nooki/proceso.hpp"

namespace nooki::cli {
namespace {

constexpr int USO_INCORRECTO = 2;
constexpr int FALLOS = 1;

/// `nooki nuevo <nombre>`: crea un proyecto mínimo que compila y ejecuta.
int crear_proyecto(std::span<const std::string_view> argumentos, std::ostream& salida,
                   std::ostream& error) {
    if (argumentos.empty()) {
        error << "uso: nooki nuevo <nombre>\n";
        return USO_INCORRECTO;
    }

    const std::string nombre(argumentos.front());
    const std::filesystem::path raiz(nombre);
    if (std::filesystem::exists(raiz)) {
        error << "nooki: «" << nombre << "» ya existe.\n";
        return USO_INCORRECTO;
    }

    std::error_code error_fs;
    std::filesystem::create_directories(raiz / "biblioteca", error_fs);
    if (error_fs) {
        error << "nooki: no se pudo crear «" << nombre << "».\n";
        return FALLOS;
    }

    {
        std::ofstream principal(raiz / "principal.oki");
        principal << "// Programa principal de «" << nombre
                  << "».\n"
                     "usar herramientas\n"
                     "\n"
                     "funcion principal() -> entero {\n"
                     "    imprimir(herramientas.saludo(\"Nooki\"))\n"
                     "    retornar 0\n"
                     "}\n";
    }
    {
        std::string motivo;
        if (!nooki::escribir_manifiesto(raiz, nombre, motivo)) {
            error << "nooki: " << motivo << "\n";
            return FALLOS;
        }
    }
    {
        std::ofstream modulo(raiz / "biblioteca" / "herramientas.oki");
        modulo << "// Módulo del proyecto.\n"
                  "publico funcion saludo(nombre: texto) -> texto {\n"
                  "    retornar \"Hola, \" + nombre\n"
                  "}\n";
    }

    salida << "proyecto «" << nombre << "» creado\n"
           << "  " << nombre << "/principal.oki\n"
           << "  " << nombre << "/nooki.toml\n"
           << "  " << nombre << "/biblioteca/herramientas.oki\n"
           << "\nentre en la carpeta y compile con «nooki construir»\n";
    return 0;
}

/// `nooki probar`: compila y ejecuta cada prueba; compara con su «.esperado».
int probar_proyecto(std::span<const std::string_view>, std::ostream& salida, std::ostream& error) {
    std::error_code error_fs;
    const std::filesystem::path pruebas("pruebas");
    if (!std::filesystem::exists(pruebas)) {
        error << "nooki: no hay carpeta «pruebas».\n";
        return USO_INCORRECTO;
    }

    std::vector<std::filesystem::path> archivos;
    for (const auto& entrada : std::filesystem::directory_iterator(pruebas, error_fs)) {
        if (entrada.is_regular_file() && entrada.path().extension() == ".oki") {
            archivos.push_back(entrada.path());
        }
    }

    if (archivos.empty()) {
        error << "nooki: no hay archivos .oki en «pruebas».\n";
        return USO_INCORRECTO;
    }

    int fallos = 0;
    for (const std::filesystem::path& archivo : archivos) {
        // Compilar con `-o` explícito.
        const std::filesystem::path binario = archivo.string() + ".bin";
        std::vector<std::string> opciones{"-o", binario.string()};
        std::vector<std::string_view> vistas;
        for (const std::string& opcion : opciones) {
            vistas.push_back(opcion);
        }
        vistas.push_back(archivo.string());

        std::ostringstream salida_compilacion;
        if (comando_compilar(vistas, salida_compilacion, salida_compilacion) != 0) {
            error << "FALLO " << archivo.string() << "\n" << salida_compilacion.str();
            ++fallos;
            continue;
        }

        // Ejecutar y capturar la salida.
        const std::filesystem::path temporal = archivo.string() + ".salida";
        const std::string comando = entrecomillar_si_hace_falta(binario.string()) + " > " +
                                    entrecomillar_si_hace_falta(temporal.string());
        if (ejecutar_comando(comando) != 0) {
            error << "FALLO " << archivo.string() << ": terminó con error\n";
            ++fallos;
            continue;
        }

        std::ifstream obtenido(temporal, std::ios::binary);
        std::ostringstream texto_obtenido;
        texto_obtenido << obtenido.rdbuf();

        const std::filesystem::path esperado = archivo.string() + ".esperado";
        if (std::filesystem::exists(esperado)) {
            std::ifstream esperado_flujo(esperado, std::ios::binary);
            std::ostringstream texto_esperado;
            texto_esperado << esperado_flujo.rdbuf();

            std::string a = texto_obtenido.str();
            std::string b = texto_esperado.str();
            // Los saltos de línea de Windows no deben afectar la comparación.
            std::string limpio_a;
            std::string limpio_b;
            for (const char c : a) {
                if (c != '\r') {
                    limpio_a += c;
                }
            }
            for (const char c : b) {
                if (c != '\r') {
                    limpio_b += c;
                }
            }
            if (limpio_a != limpio_b) {
                error << "FALLO " << archivo.string() << "\n  esperado: " << limpio_b
                      << "  obtenido: " << limpio_a;
                ++fallos;
            } else {
                salida << "ok    " << archivo.string() << "\n";
            }
        } else {
            salida << "ok    " << archivo.string() << " (sin .esperado)\n";
        }

        std::filesystem::remove(temporal, error_fs);
        std::filesystem::remove(binario, error_fs);
    }

    if (fallos == 0) {
        salida << "\ntodas las pruebas pasaron\n";
        return 0;
    }
    error << "\n" << fallos << " prueba(s) fallida(s)\n";
    return FALLOS;
}

/// `nooki limpiar`: borra los artefactos, sin tocar el código.
int limpiar_proyecto(std::span<const std::string_view>, std::ostream& salida, std::ostream& error) {
    (void)error;
    std::error_code error_fs;
    int borrados = 0;

    for (const char* carpeta : {"build", "artefactos"}) {
        borrados += static_cast<int>(std::filesystem::remove_all(carpeta, error_fs));
    }

    for (const auto& entrada : std::filesystem::recursive_directory_iterator(".", error_fs)) {
        if (!entrada.is_regular_file()) {
            continue;
        }
        const std::string extension = entrada.path().extension().string();
        if (extension == ".exe" || extension == ".bin" || extension == ".salida") {
            std::filesystem::remove(entrada.path(), error_fs);
            ++borrados;
        }
    }

    salida << "limpieza terminada: " << borrados << " elemento(s)\n";
    return 0;
}

}  // namespace

int comando_nuevo(std::span<const std::string_view> argumentos, std::ostream& salida,
                  std::ostream& error) {
    return crear_proyecto(argumentos, salida, error);
}

int comando_probar(std::span<const std::string_view> argumentos, std::ostream& salida,
                   std::ostream& error) {
    return probar_proyecto(argumentos, salida, error);
}

int comando_limpiar(std::span<const std::string_view> argumentos, std::ostream& salida,
                    std::ostream& error) {
    return limpiar_proyecto(argumentos, salida, error);
}

}  // namespace nooki::cli
