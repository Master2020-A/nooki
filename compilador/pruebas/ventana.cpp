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

    variable fotogramas = 0
    mientras (ventana.viva(v) && fotogramas < 20) {
        si (ventana.siguiente_evento(v) == 1) {
            romper
        }
        ventana.pausa(16)
        fotogramas = fotogramas + 1
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

    nooki::diagnostico::GestorFuentes fuentes;
    nooki::diagnostico::Emisor emisor;
    nooki::conductor::Opciones opciones;
    opciones.fuente = fuente;
    opciones.salida = directorio / ("ventana" + std::string(EXTENSION));

    std::ostringstream salida;
    std::ostringstream errores;
    const auto resultado =
        nooki::conductor::compilar(opciones, fuentes, emisor, salida, errores);
    if (resultado.codigo != 0) {
        std::cout << "FALLO: no compiló el programa de ventana\n" << errores.str();
        return 1;
    }

    const fs::path texto_salida = directorio / "salida.txt";
    const std::string comando =
        nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(resultado.binario)) + " > " +
        nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(texto_salida));
    const int codigo = nooki::ejecutar_comando(comando);
    const std::string obtenido = leer(texto_salida);

    fs::remove(texto_salida, error_fs);
    fs::remove(resultado.binario, error_fs);
    fs::remove(fuente, error_fs);

    if (codigo != 0) {
        std::cout << "FALLO: el programa de ventana terminó con " << codigo << "\n";
        return 1;
    }

    const bool con_pantalla = obtenido.find("ventana creada") != std::string::npos;
    const bool sin_pantalla = obtenido.find("sin entorno grafico") != std::string::npos;
    if (!con_pantalla && !sin_pantalla) {
        std::cout << "FALLO: salida inesperada «" << obtenido << "»\n";
        return 1;
    }

    if (con_pantalla) {
        if (obtenido.find("ventana cerrada") == std::string::npos) {
            std::cout << "FALLO: la ventana se abrió pero no se cerró\n";
            return 1;
        }
        if (obtenido.find("rectangulo rojo") == std::string::npos ||
            obtenido.find("fondo blanco") == std::string::npos) {
            std::cout << "FALLO: el dibujo no llegó a la ventana\n" << obtenido;
            return 1;
        }
        std::cout << "ventana: se abre, dibuja, atiende 20 fotogramas y se cierra\n";
    } else {
        std::cout << "ventana: sin entorno gráfico, la capa responde con 0\n";
    }
    return 0;
}
