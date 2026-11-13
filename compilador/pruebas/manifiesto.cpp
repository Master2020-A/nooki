// Manifiesto del proyecto: lectura de `nooki.toml` y rutas de dependencias.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "nooki/manifiesto.hpp"

namespace fs = std::filesystem;

namespace {

int fallos = 0;

void comprobar(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        std::cout << "FALLO: " << mensaje << "\n";
        ++fallos;
    }
}

void escribir(const fs::path& ruta, const std::string& contenido) {
    std::ofstream flujo(ruta, std::ios::binary | std::ios::trunc);
    flujo << contenido;
}

}  // namespace

int main() {
    const fs::path raiz = fs::temp_directory_path() / "nooki-manifiesto";
    std::error_code error_fs;
    fs::remove_all(raiz, error_fs);
    fs::create_directories(raiz / "agenda", error_fs);
    fs::create_directories(raiz / "saludos" / "biblioteca", error_fs);

    escribir(raiz / "agenda" / "nooki.toml",
             "# Manifiesto de prueba\n"
             "[proyecto]\n"
             "nombre = \"agenda\"\n"
             "version = \"1.2.3\"\n"
             "\n"
             "[dependencias]\n"
             "saludos = { ruta = \"../saludos\" }   # comentario al final\n");

    nooki::Manifiesto manifiesto;
    std::string motivo;
    comprobar(nooki::leer_manifiesto(raiz / "agenda", manifiesto, motivo),
              "no se pudo leer el manifiesto: " + motivo);
    comprobar(manifiesto.nombre == "agenda", "el nombre leído no es «agenda»");
    comprobar(manifiesto.version == "1.2.3", "la versión leída no es «1.2.3»");
    comprobar(manifiesto.dependencias.size() == 1, "debería haber una dependencia");
    if (!manifiesto.dependencias.empty()) {
        comprobar(manifiesto.dependencias[0].nombre == "saludos",
                  "la dependencia no se llama «saludos»");
        comprobar(manifiesto.dependencias[0].ruta == (raiz / "saludos"),
                  "la ruta de la dependencia no se resolvió bien");
    }

    // Las rutas de búsqueda incluyen la raíz del otro proyecto y su biblioteca.
    const auto rutas = manifiesto.rutas_de_busqueda();
    bool tiene_raiz = false;
    bool tiene_biblioteca = false;
    for (const fs::path& ruta : rutas) {
        if (ruta == (raiz / "saludos")) {
            tiene_raiz = true;
        }
        if (ruta == (raiz / "saludos" / "biblioteca")) {
            tiene_biblioteca = true;
        }
    }
    comprobar(tiene_raiz, "falta la raíz de la dependencia en las rutas de búsqueda");
    comprobar(tiene_biblioteca, "falta «biblioteca» de la dependencia");

    // Un manifiesto inexistente se informa, no se inventa.
    nooki::Manifiesto vacio;
    comprobar(!nooki::leer_manifiesto(raiz / "no-existe", vacio, motivo),
              "leer un manifiesto inexistente debería fallar");

    // Y se puede escribir uno nuevo que se vuelve a leer igual.
    comprobar(nooki::escribir_manifiesto(raiz / "saludos", "saludos", motivo),
              "no se pudo escribir el manifiesto: " + motivo);
    nooki::Manifiesto releido;
    comprobar(nooki::leer_manifiesto(raiz / "saludos", releido, motivo),
              "no se pudo releer el manifiesto escrito: " + motivo);
    comprobar(releido.nombre == "saludos", "el nombre releído no es «saludos»");

    fs::remove_all(raiz, error_fs);

    if (fallos == 0) {
        std::cout << "manifiesto: lectura, dependencias y escritura correctas\n";
        return 0;
    }
    std::cout << fallos << " comprobación(es) fallida(s)\n";
    return 1;
}
