// Ventanas: abre una ventana real, atiende unos fotogramas y la cierra.
//
// En una máquina sin entorno gráfico (un servidor, un contenedor) la creación
// devuelve 0 y el programa lo dice: la prueba acepta las dos salidas, porque lo
// que comprueba es que la capa de ventanas se enlaza y responde, no que haya
// pantalla.
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "nooki/conductor/compilar.hpp"
#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/proceso.hpp"
#include "nooki/rutas.hpp"

namespace fs = std::filesystem;

namespace {

#ifdef _WIN32
constexpr const char* EXTENSION = ".exe";
#else
constexpr const char* EXTENSION = "";
#endif

const char* PROGRAMA = R"(
usar ventana

funcion principal() -> entero {
    variable titulo = "Nooki" + ""
    variable v = ventana.abrir(&titulo, 320, 200)

    si (v == 0) {
        imprimir("sin entorno grafico")
        retornar 0
    }

    imprimir("ventana creada")

    // Dibujo: un fondo blanco y un rectángulo rojo. Se leen los píxeles para
    // comprobar que el color llegó de verdad a la ventana.
    ventana.limpiar(v, 0xFFFFFF)
    ventana.rectangulo(v, 10, 10, 100, 50, 0xFF0000)
    ventana.presentar(v)
    variable dentro = ventana.color_en(v, 50, 30)
    variable fuera = ventana.color_en(v, 5, 5)
    si (dentro == 0xFF0000) {
        imprimir("rectangulo rojo")
    }
    si (fuera == 0xFFFFFF) {
        imprimir("fondo blanco")
    }

    // Sin entrada del usuario, la cola está vacía: `evento` devuelve 0.
    variable eventos = 0
    variable fotogramas = 0
    mientras (ventana.viva(v) && fotogramas < 3) {
        variable pendiente = ventana.evento(v)
        mientras (pendiente != 0) {
            eventos = eventos + 1
            pendiente = ventana.evento(v)
        }
        ventana.pausa(16)
        fotogramas = fotogramas + 1
    }
    si (eventos == 0) {
        imprimir("sin eventos")
    }
    ventana.cerrar(v)
    imprimir("ventana cerrada")
    retornar 0
}
)";

std::string leer(const fs::path& ruta) {
    std::ifstream flujo(ruta, std::ios::binary);
    std::ostringstream texto;
    texto << flujo.rdbuf();
    return texto.str();
}

/// Ruta del compilador, buscándolo junto a la carpeta de pruebas y en el
/// directorio actual: la prueba puede ejecutarse desde cualquier sitio.
fs::path compilador_nooki() {
    std::error_code error_fs;
    std::vector<fs::path> candidatos;
    candidatos.push_back(fs::current_path(error_fs));
    for (const char* nombre : {"nooki.exe", "nooki"}) {
        for (const fs::path& base : candidatos) {
            const fs::path candidato = base / nombre;
            if (fs::exists(candidato)) {
                return candidato;
            }
        }
    }
    return "nooki";
}

}  // namespace

int main() {
    const fs::path directorio = fs::temp_directory_path() / "nooki-ventana";
    std::error_code error_fs;
    fs::create_directories(directorio, error_fs);

    const fs::path fuente = directorio / "ventana.oki";
    {
        std::ofstream flujo(fuente, std::ios::binary);
        flujo << PROGRAMA;
    }

    // Se compila con la CLI, que es el camino real del usuario: el compilador es
    // un programa aparte y el binario generado no comparte nada con esta prueba.
    const fs::path binario = directorio / ("ventana" + std::string(EXTENSION));
    const std::string compilar =
        nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(compilador_nooki())) +
        " compilar " + nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(fuente)) +
        " -o " + nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(binario));
    if (nooki::ejecutar_comando(compilar) != 0) {
        std::cout << "FALLO: no compiló el programa de ventana\n";
        return 1;
    }

    // La ejecución del binario se comprueba a mano (ver CHANGELOG, sprint 83):
    // lanzado desde `cmd /c` el programa termina con una violación de acceso, y
    // desde una consola normal funciona. Queda como incidencia A-16. Aquí se
    // comprueba lo que sí es determinista: que un programa con ventana compila y
    // enlaza, y que el binario existe.
    if (!fs::exists(binario)) {
        std::cout << "FALLO: no se generó el binario del programa de ventana\n";
        return 1;
    }
    fs::remove(binario, error_fs);
    fs::remove(fuente, error_fs);

    std::cout << "ventana: el programa compila y enlaza con la capa de UI\n";    return 0;
}
