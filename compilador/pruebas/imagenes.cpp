// Imágenes: carga un BMP generado al vuelo y comprueba sus píxeles.
//
// Se generan dos archivos: uno válido (4x3 con cuatro colores conocidos) y uno
// inválido, para comprobar que un archivo que no es un BMP no se acepta.
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "nooki/proceso.hpp"
#include "nooki/rutas.hpp"

namespace fs = std::filesystem;

namespace {

#ifdef _WIN32
constexpr const char* EXTENSION = ".exe";
#else
constexpr const char* EXTENSION = "";
#endif

void escribir_bmp(const fs::path& ruta, int ancho, int alto,
                  const std::vector<std::vector<std::uint8_t>>& filas) {
    const int tamano_datos = ancho * 3 * alto;
    std::vector<std::uint8_t> bytes;
    bytes.reserve(static_cast<std::size_t>(54 + tamano_datos));

    const auto poner_entero = [&bytes](std::int32_t valor) {
        bytes.push_back(static_cast<std::uint8_t>(valor & 0xFF));
        bytes.push_back(static_cast<std::uint8_t>((valor >> 8) & 0xFF));
        bytes.push_back(static_cast<std::uint8_t>((valor >> 16) & 0xFF));
        bytes.push_back(static_cast<std::uint8_t>((valor >> 24) & 0xFF));
    };

    bytes.push_back('B');
    bytes.push_back('M');
    poner_entero(54 + tamano_datos);
    poner_entero(0);
    poner_entero(54);
    poner_entero(40);
    poner_entero(ancho);
    poner_entero(alto);
    bytes.push_back(1);
    bytes.push_back(0);
    bytes.push_back(24);
    bytes.push_back(0);
    poner_entero(0);
    poner_entero(tamano_datos);
    poner_entero(2835);
    poner_entero(2835);
    poner_entero(0);
    poner_entero(0);

    // El BMP guarda las filas de abajo arriba.
    for (int y = alto - 1; y >= 0; --y) {
        for (int x = 0; x < ancho; ++x) {
            const std::vector<std::uint8_t>& color = filas[static_cast<std::size_t>(y)];
            bytes.push_back(color[static_cast<std::size_t>(x) * 3 + 2]);
            bytes.push_back(color[static_cast<std::size_t>(x) * 3 + 1]);
            bytes.push_back(color[static_cast<std::size_t>(x) * 3 + 0]);
        }
    }

    std::ofstream flujo(ruta, std::ios::binary);
    flujo.write(reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
}

std::string leer(const fs::path& ruta) {
    std::ifstream flujo(ruta, std::ios::binary);
    std::ostringstream texto;
    texto << flujo.rdbuf();
    // En Windows las líneas terminan en \r\n; se comparan como \n.
    std::string contenido = texto.str();
    std::string limpio;
    limpio.reserve(contenido.size());
    for (const char caracter : contenido) {
        if (caracter != '\r') {
            limpio.push_back(caracter);
        }
    }
    return limpio;
}

fs::path compilador_nooki() {
    std::error_code error_fs;
    for (const char* nombre : {"nooki.exe", "nooki"}) {
        const fs::path candidato = fs::current_path(error_fs) / nombre;
        if (fs::exists(candidato)) {
            return candidato;
        }
    }
    return "nooki";
}

const char* PROGRAMA = R"(
usar imagenes

funcion principal() -> entero {
    variable ruta = "imagen.bmp" + ""
    variable img = imagenes.cargar(&ruta)

    si (!imagenes.cargada(img)) {
        imprimir("no se pudo cargar")
        retornar 1
    }

    imprimir(imagenes.ancho(img))
    imprimir(imagenes.alto(img))
    imprimir(imagenes.color_en(img, 0, 0))
    imprimir(imagenes.color_en(img, 1, 0))
    imprimir(imagenes.color_en(img, 2, 0))
    imprimir(imagenes.color_en(img, 3, 0))
    // Fuera de la imagen devuelve -1.
    imprimir(imagenes.color_en(img, 99, 0))
    imagenes.soltar(img)

    variable mala = "no-es-imagen.bmp" + ""
    variable ninguna = imagenes.cargar(&mala)
    si (imagenes.cargada(ninguna)) {
        imprimir("acepto un archivo invalido")
    } sino {
        imprimir("rechazo el archivo invalido")
    }
    retornar 0
}
)";

}  // namespace

int main() {
    const fs::path directorio = fs::temp_directory_path() / "nooki-imagenes";
    std::error_code error_fs;
    fs::create_directories(directorio, error_fs);

    // Cuatro píxeles: rojo, verde, azul, blanco (en RGB).
    const std::vector<std::vector<std::uint8_t>> filas = {
        {255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255},
        {255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255},
        {255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255},
    };
    escribir_bmp(directorio / "imagen.bmp", 4, 3, filas);
    {
        std::ofstream invalido(directorio / "no-es-imagen.bmp", std::ios::binary);
        invalido << "esto no es un BMP";
    }

    const fs::path fuente = directorio / "imagen.oki";
    {
        std::ofstream flujo(fuente, std::ios::binary);
        flujo << PROGRAMA;
    }

    const fs::path binario = directorio / ("imagen" + std::string(EXTENSION));
    const std::string compilar =
        nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(compilador_nooki())) +
        " compilar " + nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(fuente)) +
        " -o " + nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(binario));
    if (nooki::ejecutar_comando(compilar) != 0) {
        std::cout << "FALLO: no compiló el programa de imágenes\n";
        return 1;
    }

    // El programa busca la imagen en su directorio de trabajo: se ejecuta allí.
    const fs::path texto_salida = directorio / "salida.txt";
#ifdef _WIN32
    const std::string entrar = "cd /d " + nooki::entrecomillar_si_hace_falta(
                                               nooki::ruta_a_utf8(directorio));
#else
    const std::string entrar = "cd " + nooki::entrecomillar_si_hace_falta(
                                            nooki::ruta_a_utf8(directorio));
#endif
    const std::string comando =
        entrar + " && " + nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(binario)) +
        " > " + nooki::entrecomillar_si_hace_falta(nooki::ruta_a_utf8(texto_salida));
    const int codigo = nooki::ejecutar_comando(comando);
    const std::string obtenido = leer(texto_salida);

    fs::remove(texto_salida, error_fs);
    fs::remove(binario, error_fs);
    fs::remove(fuente, error_fs);
    fs::remove(directorio / "imagen.bmp", error_fs);
    fs::remove(directorio / "no-es-imagen.bmp", error_fs);

    if (codigo != 0) {
        std::cout << "FALLO: el programa de imágenes terminó con " << codigo << "\n";
        return 1;
    }

    const std::string esperado =
        "4\n3\n16711680\n65280\n255\n16777215\n-1\nrechazo el archivo invalido\n";
    if (obtenido != esperado) {
        std::cout << "FALLO: salida inesperada\n--- esperado ---\n" << esperado
                  << "--- obtenido ---\n" << obtenido;
        return 1;
    }

    std::cout << "imágenes: carga un BMP de 4x3, lee sus píxeles y rechaza lo que no es BMP\n";
    return 0;
}
