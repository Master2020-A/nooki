#include <algorithm>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/cli/comandos.hpp"
#include "nooki/formato/formateador.hpp"
#include "nooki/cli/orden.hpp"
#include "nooki/rutas.hpp"

namespace nooki::cli {
namespace {

}  // namespace

int comando_formatear(std::span<const std::string_view> argumentos, std::ostream& salida,
                      std::ostream& error) {
    bool comprobar_solo = false;
    std::vector<std::string_view> archivos;
    for (const std::string_view argumento : argumentos) {
        if (argumento == "--comprobar") {
            comprobar_solo = true;
        } else if (!argumento.empty() && argumento.front() == '-') {
            error << "nooki: la opción «" << argumento << "» no existe\n";
            return USO_INCORRECTO;
        } else {
            archivos.push_back(argumento);
        }
    }

    if (archivos.empty()) {
        error << "nooki: falta el archivo a formatear\n";
        error << "uso: nooki formatear <archivo.oki>... [--comprobar]\n";
        return USO_INCORRECTO;
    }

    bool hubo_cambios = false;
    for (const std::string_view archivo : archivos) {
        const std::filesystem::path ruta = ruta_desde_utf8(archivo);
        std::ifstream flujo(ruta, std::ios::binary);
        if (!flujo) {
            error << "nooki: no se pudo leer «" << archivo << "»\n";
            return DATOS_INCORRECTOS;
        }
        std::ostringstream contenido;
        contenido << flujo.rdbuf();
        flujo.close();

        const std::string original = contenido.str();
        const std::string formateado = formato::formatear(original);
        if (formateado == original) {
            continue;
        }
        hubo_cambios = true;

        if (comprobar_solo) {
            error << "nooki: «" << archivo << "» no está formateado\n";
            continue;
        }
        std::ofstream escritura(ruta, std::ios::binary | std::ios::trunc);
        if (!escritura) {
            error << "nooki: no se pudo escribir «" << archivo << "»\n";
            return DATOS_INCORRECTOS;
        }
        escritura << formateado;
        salida << "formateado: " << archivo << '\n';
    }

    if (comprobar_solo && hubo_cambios) {
        return DATOS_INCORRECTOS;
    }
    return EXITO;
}

}  // namespace nooki::cli
