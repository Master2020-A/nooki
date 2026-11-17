#include "nooki/cli/compilar.hpp"
#include "nooki/lsp/servidor.hpp"
#include "nooki/manifiesto.hpp"
#include "nooki/rutas.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <string>
#include <system_error>
#include <vector>

#include "nooki/cli/orden.hpp"
#include "nooki/conductor/compilar.hpp"
#include "nooki/diagnostico/render.hpp"
#include "nooki/diagnostico/terminal.hpp"
#include "nooki/proceso.hpp"

namespace nooki::cli {
namespace {

void imprimir_uso(std::ostream& error) {
    error << "uso: nooki compilar <archivo.oki> [-o <salida>] [--emitir-c] [--conservar]\n"
             "  -o <salida>   ruta del ejecutable (por defecto, el nombre del fuente)\n"
             "  --emitir-c    muestra el C generado y no compila\n"
             "  --conservar   conserva el archivo .c intermedio\n";
}

int compilar_archivo(std::span<const std::string_view> argumentos, std::ostream& salida,
                     std::ostream& error, bool ejecutar_despues,
                     const std::vector<std::filesystem::path>& rutas_de_modulos = {}) {
    conductor::Opciones opciones;
    opciones.rutas_de_modulos = rutas_de_modulos;
    bool hay_fuente = false;

    for (std::size_t i = 0; i < argumentos.size(); ++i) {
        const std::string_view argumento = argumentos[i];

        if (argumento == "-o" || argumento == "--salida") {
            if (i + 1 >= argumentos.size()) {
                error << "nooki: falta la ruta después de «" << argumento << "»\n";
                return USO_INCORRECTO;
            }
            ++i;
            opciones.salida = ruta_desde_utf8(argumentos[i]);
        } else if (argumento == "--emitir-c") {
            opciones.solo_c = true;
        } else if (argumento == "--conservar") {
            opciones.conservar_intermedios = true;
        } else if (!argumento.empty() && argumento.front() == '-') {
            error << "nooki: la opción «" << argumento << "» no existe\n";
            return USO_INCORRECTO;
        } else if (!hay_fuente) {
            opciones.fuente = ruta_desde_utf8(argumento);
            hay_fuente = true;
        } else {
            error << "nooki: sobra el argumento «" << argumento << "»\n";
            return USO_INCORRECTO;
        }
    }

    if (!hay_fuente) {
        imprimir_uso(error);
        return USO_INCORRECTO;
    }

    diagnostico::GestorFuentes fuentes;
    diagnostico::Emisor emisor;

    const conductor::Resultado resultado =
        conductor::compilar(opciones, fuentes, emisor, salida, error);

    if (emisor.cantidad() > 0) {
        diagnostico::OpcionesRender render;
        render.color = diagnostico::admite_color_error();
        diagnostico::renderizar_texto(emisor.diagnosticos(), fuentes, error, render);
    }

    if (resultado.codigo != 0 || !ejecutar_despues || resultado.binario.empty()) {
        // Los fallos de compilación salen con el código documentado en `ayuda`,
        // no con el 1 genérico: quien automatiza puede distinguirlos.
        return resultado.codigo == 0 ? 0 : DATOS_INCORRECTOS;
    }

    // `make_preferred` convierte «/» en «\» en Windows: la consola no entiende «/».
    std::filesystem::path binario = resultado.binario;
    std::string orden = ruta_a_utf8(binario.make_preferred());
#if !defined(_WIN32)
    // En Unix la consola no busca en el directorio actual: un binario que se
    // llama «hola» hay que lanzarlo como «./hola».
    if (orden.find('/') == std::string::npos) {
        orden = "./" + orden;
    }
#endif
    return nooki::ejecutar_comando(nooki::entrecomillar_si_hace_falta(orden));
}

}  // namespace

int comando_compilar(std::span<const std::string_view> argumentos, std::ostream& salida,
                     std::ostream& error) {
    return compilar_archivo(argumentos, salida, error, false);
}

/// Compila con carpetas de módulos adicionales (las dependencias del manifiesto).
int compilar_con_rutas(std::span<const std::string_view> argumentos,
                       const std::vector<std::filesystem::path>& rutas, std::ostream& salida,
                       std::ostream& error) {
    return compilar_archivo(argumentos, salida, error, false, rutas);
}

/// `nooki lsp`: servidor de lenguaje por la entrada y salida estándar.
int comando_lsp(std::span<const std::string_view>, std::ostream& salida, std::ostream& error) {
    (void)salida;
    (void)error;
    return nooki::lsp::servir(std::cin, std::cout);
}

/// `nooki construir`: compila el proyecto del directorio actual.
int comando_construir(std::span<const std::string_view> argumentos, std::ostream& salida,
                      std::ostream& error) {
    // `nooki construir [carpeta]`: sin argumento, el proyecto del directorio
    // actual; con uno, el de esa carpeta.
    std::filesystem::path raiz = std::filesystem::current_path();
    if (!argumentos.empty()) {
        raiz = std::filesystem::path(std::string(argumentos.front()));
        if (!std::filesystem::is_directory(raiz)) {
            error << "error[NOK0001]: «" << argumentos.front() << "» no es una carpeta\n";
            return USO_INCORRECTO;
        }
    }

    // Entrada: `principal.oki` junto al manifiesto, o dentro de `fuente/`.
    const std::filesystem::path fuente = std::filesystem::exists(raiz / "principal.oki")
                                             ? raiz / "principal.oki"
                                             : raiz / "fuente" / "principal.oki";
    if (!std::filesystem::exists(fuente)) {
        // Sin manifiesto ni fuente no hay proyecto. Antes de dar el error, se
        // miran las carpetas de al lado: lo normal es estar un nivel por encima
        // de los programas, como en `ejemplos/`.
        std::vector<std::string> programas;
        std::error_code error_fs;
        for (const auto& entrada : std::filesystem::directory_iterator(raiz, error_fs)) {
            if (!entrada.is_directory(error_fs)) {
                continue;
            }
            const std::filesystem::path dentro = entrada.path() / "principal.oki";
            if (std::filesystem::exists(dentro)) {
                programas.push_back(entrada.path().filename().string());
            }
        }
        std::sort(programas.begin(), programas.end());

        const bool hay_manifiesto = std::filesystem::exists(raiz / "nooki.toml");
        error << "error[NOK0001]: aquí no hay un proyecto de Nooki\n"
                 "   = ayuda: no se encontró «principal.oki»"
              << (hay_manifiesto ? "" : " ni «nooki.toml»") << "\n";
        if (!programas.empty()) {
            error << "   = ayuda: esta carpeta contiene " << programas.size()
                  << (programas.size() == 1 ? " programa" : " programas")
                  << ", cada uno en su carpeta:\n";
            for (std::size_t i = 0; i < programas.size() && i < 8; ++i) {
                error << "            " << programas[i] << "/principal.oki\n";
            }
            error << "   = ayuda: «nooki construir " << programas.front()
                  << "» construye ese; «nooki ejecutar " << programas.front()
                  << "/principal.oki» lo ejecuta\n";
        } else {
            error << "   = ayuda: «nooki nuevo demo» crea uno; «nooki compilar archivo.oki»\n"
                     "            compila un archivo suelto\n";
        }
        return USO_INCORRECTO;
    }

    // El manifiesto aporta las dependencias: sus carpetas entran en la búsqueda.
    nooki::Manifiesto manifiesto;
    std::string motivo;
    std::vector<std::filesystem::path> rutas;
    if (nooki::leer_manifiesto(raiz, manifiesto, motivo)) {
        rutas = manifiesto.rutas_de_busqueda();
    } else if (std::filesystem::exists(raiz / "nooki.toml")) {
        error << "nooki: no se pudo leer nooki.toml: " << motivo << "\n";
        return USO_INCORRECTO;
    }

    // El primer argumento, si lo había, ya se usó como carpeta: el resto pasa
    // tal cual (por ejemplo, `-o`).
    const std::size_t desde = argumentos.empty() ? 0 : 1;
    const std::string fuente_texto = fuente.string();
    std::vector<std::string_view> nuevos;
    nuevos.push_back(fuente_texto);
    for (std::size_t i = desde; i < argumentos.size(); ++i) {
        nuevos.push_back(argumentos[i]);
    }
    return compilar_con_rutas(nuevos, rutas, salida, error);
}

int comando_ejecutar(std::span<const std::string_view> argumentos, std::ostream& salida,
                     std::ostream& error) {
    return compilar_archivo(argumentos, salida, error, true);
}

}  // namespace nooki::cli
