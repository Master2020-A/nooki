#include "nooki/cli/ayuda.hpp"

#include "nooki/cli/comandos.hpp"
#include "nooki/version.hpp"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <ostream>

namespace nooki::cli {
namespace {

/// Ancho mínimo de la columna de nombres de comando.
constexpr std::size_t ANCHO_MINIMO = 12;

}  // namespace

void imprimir_version(std::ostream& salida) {
    salida << "nooki " << version::TEXTO << " (" << version::OBJETIVO << ", edición "
           << version::EDICION << ")\n";
}

void imprimir_ayuda(std::ostream& salida) {
    salida << "nooki " << version::TEXTO << " — compilador del lenguaje Nooki (edición "
           << version::EDICION << ")\n";
    salida << "Objetivo: " << version::OBJETIVO << "\n\n";

    salida << "Uso:\n";
    salida << "  nooki <comando> [opciones]\n\n";

    salida << "Comandos:\n";
    std::size_t ancho = ANCHO_MINIMO;
    for (const Comando& comando : COMANDOS) {
        ancho = std::max(ancho, comando.nombre.size());
    }
    for (const Comando& comando : COMANDOS) {
        salida << "  " << std::left << std::setw(static_cast<int>(ancho) + 2) << comando.nombre
               << comando.resumen;
        if (!comando.disponible()) {
            salida << "  (Sprint " << comando.sprint << ')';
        }
        salida << '\n';
    }

    salida << "\nOpciones:\n"
              "  -v, --version            Equivale a «version»\n"
              "  -h, --ayuda, --help      Equivale a «ayuda»\n"
              "\nCódigos de salida:\n"
              "   0  éxito\n"
              "  64  uso incorrecto\n"
              "  65  error en los datos de entrada\n"
              "  69  funcionalidad todavía no disponible\n"
              "  70  error interno\n";
}

}  // namespace nooki::cli
