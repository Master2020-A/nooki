// Documentación verificada: cada ejemplo del manual compila.
//
// El manual (MANUAL.md) está lleno de bloques ```nooki que son programas
// completos. Esta prueba los extrae, los pasa por el compilador y falla si
// alguno deja de ser válido. Así la documentación no puede mentir: si el
// lenguaje cambia y un ejemplo se rompe, `ctest` lo dice.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/conductor/compilar.hpp"
#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"

namespace fs = std::filesystem;

namespace {

/// Ruta de un documento de la raíz, subiendo desde el directorio actual.
fs::path ruta_del_documento(const std::string& nombre) {
    fs::path actual = fs::current_path();
    for (int subida = 0; subida < 6; ++subida) {
        const fs::path candidato = actual / nombre;
        std::error_code error_fs;
        if (fs::exists(candidato, error_fs)) {
            return candidato;
        }
        if (!actual.has_parent_path()) {
            break;
        }
        actual = actual.parent_path();
    }
    return {};
}

/// Bloques de código marcados como nooki, en orden.
std::vector<std::string> ejemplos_de(const fs::path& manual) {
    std::ifstream flujo(manual);
    std::vector<std::string> bloques;
    std::string linea;
    std::ostringstream actual;
    bool dentro = false;

    while (std::getline(flujo, linea)) {
        if (!linea.empty() && linea.back() == '\r') {
            linea.pop_back();
        }
        if (!dentro && linea == "```nooki") {
            dentro = true;
            actual.str({});
            actual.clear();
            continue;
        }
        if (dentro && linea == "```") {
            dentro = false;
            bloques.push_back(actual.str());
            continue;
        }
        if (dentro) {
            actual << linea << '\n';
        }
    }
    return bloques;
}

}  // namespace

int main() {
    // El manual y la referencia: los dos llevan programas que deben compilar.
    std::vector<std::string> ejemplos;
    for (const std::string_view documento : {"MANUAL.md", "REFERENCIA.md"}) {
        const fs::path ruta = ruta_del_documento(std::string(documento));
        if (ruta.empty()) {
            std::cout << "FALLO: no se encontró " << documento << "\n";
            return 1;
        }
        const std::vector<std::string> suyos = ejemplos_de(ruta);
        if (suyos.empty()) {
            std::cout << "FALLO: " << documento << " no tiene ejemplos nooki\n";
            return 1;
        }
        ejemplos.insert(ejemplos.end(), suyos.begin(), suyos.end());
    }

    const fs::path directorio = fs::temp_directory_path() / "nooki-manual";
    std::error_code error_fs;
    fs::create_directories(directorio, error_fs);

    int fallos = 0;
    for (std::size_t i = 0; i < ejemplos.size(); ++i) {
        const fs::path fuente = directorio / ("ejemplo" + std::to_string(i + 1) + ".oki");
        {
            std::ofstream flujo(fuente, std::ios::binary);
            flujo << ejemplos[i];
        }

        nooki::conductor::Opciones opciones;
        opciones.fuente = fuente;
        opciones.solo_verificar = true;

        nooki::diagnostico::GestorFuentes fuentes;
        nooki::diagnostico::Emisor emisor;
        std::ostringstream salida;
        std::ostringstream errores;
        const auto resultado =
            nooki::conductor::compilar(opciones, fuentes, emisor, salida, errores);

        if (resultado.codigo != 0 || emisor.hay_errores()) {
            ++fallos;
            std::cout << "FALLO el ejemplo " << (i + 1) << " del manual\n";
            for (const auto& diagnostico : emisor.diagnosticos()) {
                std::cout << "  " << diagnostico.codigo() << "\n";
            }
        }
    }

    fs::remove_all(directorio, error_fs);

    if (fallos > 0) {
        std::cout << fallos << " ejemplo(s) del manual no compilan\n";
        return 1;
    }
    std::cout << "documentación: " << ejemplos.size() << " ejemplos compilan (manual y referencia)\n";
    return 0;
}
