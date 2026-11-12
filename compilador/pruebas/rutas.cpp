// Rutas y nombres con tildes: en un lenguaje en español es el caso corriente.
// Comprueba que se pueden compilar y ejecutar archivos cuyo nombre y carpeta
// llevan acentos y eñe, en la plataforma en la que se ejecute la prueba.
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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


}  // namespace

int main() {
    const fs::path directorio = fs::temp_directory_path() / "nooki-rutas" / "carpeta con tildes á";
    std::error_code error_fs;
    fs::create_directories(directorio, error_fs);

    const fs::path fuente = directorio / "programa ñ.oki";
    {
        std::ofstream flujo(fuente, std::ios::binary);
        flujo << "funcion principal() -> entero {\n"
                 "    imprimir(\"con tilde funciona\")\n"
                 "    retornar 0\n"
                 "}\n";
    }

    nooki::diagnostico::GestorFuentes fuentes;
    nooki::diagnostico::Emisor emisor;
    nooki::conductor::Opciones opciones;
    opciones.fuente = fuente;
    opciones.salida = directorio / ("programa ñ" + std::string(EXTENSION));

    std::ostringstream salida;
    std::ostringstream errores;
    const auto resultado =
        nooki::conductor::compilar(opciones, fuentes, emisor, salida, errores);

    if (resultado.codigo != 0) {
        std::cout << "FALLO: no compiló un archivo con tildes\n" << errores.str();
        return 1;
    }

    // Se ejecuta sin redirección: lo que se comprueba es que una ruta con tildes
    // llega intacta al sistema, no el entrecomillado del intérprete.
    const std::string comando =
        nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(resultado.binario));
    const int codigo = nooki::ejecutar_comando(comando);

    fs::remove(resultado.binario, error_fs);
    fs::remove(fuente, error_fs);

    if (codigo != 0) {
        std::cout << "FALLO: el programa con tildes terminó con " << codigo << "\n";
        return 1;
    }

    std::cout << "rutas con tildes: compila y ejecuta\n";
    return 0;
}
