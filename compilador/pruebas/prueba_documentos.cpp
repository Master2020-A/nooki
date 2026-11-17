// Comprueba que la documentación esté al día entre sí: que cada sprint del plan
// aparezca en el historial y en el README, y que las versiones coincidan.
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

int fallos = 0;

std::string leer(const fs::path& ruta) {
    std::ifstream flujo(ruta, std::ios::binary);
    std::ostringstream texto;
    texto << flujo.rdbuf();
    std::string contenido = texto.str();
    contenido.erase(std::remove(contenido.begin(), contenido.end(), '\r'), contenido.end());
    return contenido;
}

void comprobar(bool condicion, const std::string& mensaje) {
    if (!condicion) {
        std::cout << "FALLO: " << mensaje << "\n";
        ++fallos;
    }
}

/// Números de sprint declarados en el plan, en orden.
std::vector<int> sprints_del_plan(const std::string& plan) {
    std::vector<int> sprints;
    // Solo los sprints ya completados: los planificados aún no tienen entrada.
    const std::regex patron(R"(\| \*\*S(\d+)\*\* \|([^\n]*)\|)");
    for (auto it = std::sregex_iterator(plan.begin(), plan.end(), patron);
         it != std::sregex_iterator(); ++it) {
        const std::string fila = (*it)[2].str();
        if (fila.find("✅") != std::string::npos) {
            sprints.push_back(std::stoi((*it)[1].str()));
        }
    }
    return sprints;
}

/// Primer número de versión de un texto: `VERSION 0.9.0` o `nooki 0.9.0`.
std::string version_de(const std::string& texto, const std::regex& patron) {
    std::smatch coincidencia;
    if (std::regex_search(texto, coincidencia, patron)) {
        return coincidencia[1].str();
    }
    return {};
}

}  // namespace

int main() {
    const char* desde_entorno = std::getenv("NOOKI_DIR_RAIZ");
    const fs::path raiz = desde_entorno != nullptr
                              ? fs::path(desde_entorno)
                              : fs::path(__FILE__).parent_path().parent_path().parent_path();

    const std::string plan = leer(raiz / "PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md");
    const std::string historial = leer(raiz / "CHANGELOG.md");
    const std::string leeme = leer(raiz / "LEEME.md");
    const std::string readme = leer(raiz / "README.md");
    const std::string cmake = leer(raiz / "CMakeLists.txt");
    const std::string especificacion = leer(raiz / "spec" / "especificacion-v0.1.md");

    comprobar(!plan.empty(), "no se pudo leer el plan de sprints");
    comprobar(!historial.empty(), "no se pudo leer el historial de cambios");

    // Cada sprint del plan debe figurar en el historial y en el README.
    const std::vector<int> sprints = sprints_del_plan(plan);
    comprobar(!sprints.empty(), "el plan no declara ningún sprint");

    int sin_historial = 0;
    int sin_readme = 0;
    for (const int sprint : sprints) {
        const std::string marca = "Sprint " + std::to_string(sprint);
        if (historial.find(marca + " ") == std::string::npos &&
            historial.find(marca + "—") == std::string::npos) {
            std::cout << "  el sprint " << sprint << " no está en CHANGELOG.md\n";
            ++sin_historial;
        }
        if (readme.find(marca) == std::string::npos) {
            std::cout << "  el sprint " << sprint << " no está en README.md\n";
            ++sin_readme;
        }
    }
    comprobar(sin_historial == 0, "hay sprints del plan sin entrada en el historial");
    comprobar(sin_readme == 0, "hay sprints del plan sin entrada en el README");

    // La versión debe coincidir en CMake, historial, LEEME y especificación.
    const std::string version_cmake =
        version_de(cmake, std::regex(R"(project\([^)]*VERSION\s+([0-9]+\.[0-9]+\.[0-9]+))"));
    comprobar(!version_cmake.empty(), "no se encontró la versión en CMakeLists.txt");

    const std::string version_historial =
        version_de(historial, std::regex(R"(## \[([0-9]+\.[0-9]+\.[0-9]+)\])"));
    const std::string version_leeme =
        version_de(leeme, std::regex(R"(Versión actual: ([0-9]+\.[0-9]+\.[0-9]+))"));

    comprobar(version_cmake == version_historial,
              "la última versión del historial («" + version_historial +
                  "») no coincide con CMake («" + version_cmake + "»)");
    comprobar(version_cmake == version_leeme, "la versión de LEEME («" + version_leeme +
                                                  "») no coincide con CMake («" + version_cmake +
                                                  "»)");

    // La especificación declara la misma versión menor.
    const std::string version_spec = version_de(
        especificacion, std::regex(R"(Especificación del lenguaje NOOKI v([0-9]+\.[0-9]+))"));
    const std::string menor_cmake = version_cmake.substr(0, version_cmake.rfind('.'));
    comprobar(!version_spec.empty(), "la especificación no declara versión");
    comprobar(version_spec == menor_cmake, "la especificación («v" + version_spec +
                                               "») no coincide con CMake («" + version_cmake +
                                               "»)");

    if (fallos == 0) {
        std::cout << "documentación coherente: " << sprints.size() << " sprints, versión "
                  << version_cmake << "\n";
        return 0;
    }
    std::cout << fallos << " incoherencia(s) en la documentación\n";
    return 1;
}
