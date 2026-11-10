#include "nooki/conductor/compilar.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <algorithm>
#include <vector>

#include "nooki/diagnostico/codigos.hpp"
#include "nooki/entorno.hpp"
#include "nooki/generador/generador_c.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/proceso.hpp"
#include "nooki/semantico/verificaciones.hpp"
#include "nooki/sintactico/parser.hpp"

namespace nooki::conductor {
namespace {

#ifdef _WIN32
constexpr const char* NULO = "nul";
constexpr const char* EXTENSION_BINARIO = ".exe";
#else
constexpr const char* NULO = "/dev/null";
constexpr const char* EXTENSION_BINARIO = "";
#endif

std::string leer_archivo(const std::filesystem::path& ruta);

/// Carga un módulo, marca sus funciones con su nombre y añade también los suyos.
bool cargar_modulo(const std::filesystem::path& directorio, std::string_view nombre,
                   diagnostico::GestorFuentes& fuentes, diagnostico::Emisor& emisor,
                   std::vector<std::string>& cargados, sintactico::Programa& destino) {
    const std::string clave(nombre);
    if (std::find(cargados.begin(), cargados.end(), clave) != cargados.end()) {
        return true;  // ya cargado: un ciclo no duplica el módulo
    }
    cargados.push_back(clave);

    const std::filesystem::path ruta = directorio / (clave + ".oki");
    const std::string contenido = leer_archivo(ruta);
    if (contenido.empty() && !std::filesystem::exists(ruta)) {
        emisor.error(diagnostico::codigos::MODULO_NO_ENCONTRADO, diagnostico::Span{})
            .parametro("modulo", clave)
            .ayuda("se buscó «" + ruta.string() + "»");
        return false;
    }

    const auto archivo = fuentes.anadir(ruta.string(), contenido);
    lexico::Lexer lexer(fuentes, archivo, emisor);
    const auto tokens = lexer.todos();
    sintactico::Parser parser(fuentes, archivo, emisor, tokens);
    sintactico::Programa sub = parser.analizar();
    if (emisor.hay_errores()) {
        return false;
    }

    for (const std::string_view uso : sub.usos) {
        if (!cargar_modulo(directorio, uso, fuentes, emisor, cargados, destino)) {
            return false;
        }
    }

    for (sintactico::Funcion& funcion : sub.funciones) {
        funcion.modulo = nombre;
        destino.funciones.push_back(std::move(funcion));
    }
    for (sintactico::DefinicionTipo& tipo : sub.tipos) {
        tipo.modulo = nombre;
        destino.tipos.push_back(std::move(tipo));
    }
    for (sintactico::Enumeracion& enumeracion : sub.enumeraciones) {
        enumeracion.modulo = nombre;
        destino.enumeraciones.push_back(std::move(enumeracion));
    }
    return true;
}

bool existe_compilador(const std::string& candidato) {
    const std::string comando =
        entrecomillar_si_hace_falta(candidato) + " --version > " + NULO + " 2>&1";
    return ejecutar_comando(comando) == 0;
}

std::string leer_archivo(const std::filesystem::path& ruta) {
    std::ifstream entrada(ruta, std::ios::binary);
    if (!entrada) {
        return {};
    }
    std::ostringstream contenido;
    contenido << entrada.rdbuf();
    return contenido.str();
}

std::filesystem::path ruta_de_salida(const Opciones& opciones) {
    if (!opciones.salida.empty()) {
        return opciones.salida;
    }
    std::filesystem::path salida = opciones.fuente;
    salida.replace_extension(EXTENSION_BINARIO);
    return salida;
}

}  // namespace

std::string compilador_c() {
    const std::string preferido = leer_entorno("NOOKI_CC");
    if (!preferido.empty() && existe_compilador(preferido)) {
        return preferido;
    }
    for (const char* candidato : {"cc", "gcc", "clang"}) {
        if (existe_compilador(candidato)) {
            return candidato;
        }
    }
    return {};
}

Resultado compilar(const Opciones& opciones, diagnostico::GestorFuentes& fuentes,
                   diagnostico::Emisor& emisor, std::ostream& salida, std::ostream& error) {
    Resultado resultado;

    const std::string contenido = leer_archivo(opciones.fuente);
    if (contenido.empty() && !std::filesystem::exists(opciones.fuente)) {
        emisor.error(diagnostico::codigos::ARCHIVO_NO_ENCONTRADO, diagnostico::Span{})
            .parametro("archivo", opciones.fuente.string());
        return Resultado{1, {}, {}};
    }

    const auto archivo = fuentes.anadir(opciones.fuente.string(), contenido);

    lexico::Lexer lexer(fuentes, archivo, emisor);
    const auto tokens = lexer.todos();

    sintactico::Parser parser(fuentes, archivo, emisor, tokens);
    sintactico::Programa programa = parser.analizar();

    if (emisor.hay_errores()) {
        return Resultado{1, {}, {}};
    }

    // Módulos: se cargan los que el archivo principal importa, y los suyos.
    std::vector<std::string> cargados;
    const std::filesystem::path directorio = opciones.fuente.parent_path();
    for (const std::string_view uso : programa.usos) {
        if (!cargar_modulo(directorio, uso, fuentes, emisor, cargados, programa)) {
            return Resultado{1, {}, {}};
        }
    }

    // Verificación semántica: nombres, aridad, tipos, condiciones y retornos.
    if (semantico::verificar(programa, emisor) > 0) {
        return Resultado{1, {}, {}};
    }

    const generador::ResultadoGeneracion generado = generador::generar_c(programa);
    if (!generado.soportado) {
        emisor.error(diagnostico::codigos::BACKEND_NO_SOPORTA, diagnostico::Span{})
            .parametro("construccion", generado.motivo)
            .ayuda("el backend C solo traduce llamadas, textos, enteros e identificadores");
        return Resultado{1, generado.codigo, {}};
    }
    resultado.c_generado = generado.codigo;

    if (opciones.solo_c) {
        salida << resultado.c_generado;
        return resultado;
    }

    const std::string compilador = compilador_c();
    if (compilador.empty()) {
        emisor.error(diagnostico::codigos::SIN_COMPILADOR_C, diagnostico::Span{});
        return Resultado{1, resultado.c_generado, {}};
    }

    const std::filesystem::path intermedio =
        std::filesystem::temp_directory_path() /
        ("nooki_" + opciones.fuente.stem().string() + ".c");

    {
        std::ofstream archivo_c(intermedio, std::ios::binary | std::ios::trunc);
        archivo_c << resultado.c_generado;
    }

    const std::filesystem::path salida_binario = ruta_de_salida(opciones);
    const std::string comando = entrecomillar_si_hace_falta(compilador) + " -O2 -o " +
                                entrecomillar_si_hace_falta(salida_binario.string()) + " " +
                                entrecomillar_si_hace_falta(intermedio.string());

    const int codigo = ejecutar_comando(comando);
    if (codigo != 0) {
        emisor.error(diagnostico::codigos::FALLO_COMPILADOR_C, diagnostico::Span{})
            .parametro("compilador", compilador)
            .parametro("codigo", std::to_string(codigo))
            .nota("C generado en " + intermedio.string());
        return Resultado{1, resultado.c_generado, {}};
    }

    if (opciones.conservar_intermedios) {
        error << "nooki: C intermedio conservado en " << intermedio.string() << '\n';
    } else {
        std::error_code ignorado;
        std::filesystem::remove(intermedio, ignorado);
    }

    resultado.binario = salida_binario;
    return resultado;
}

}  // namespace nooki::conductor
