#include <algorithm>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/cli/comandos.hpp"
#include "nooki/cli/orden.hpp"
#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/rutas.hpp"
#include "nooki/sintactico/ast.hpp"
#include "nooki/sintactico/parser.hpp"

namespace nooki::cli {
namespace {

/// Firma legible de una función: `nombre(a: entero, b: texto) -> entero`.
std::string firma_de(const sintactico::Funcion& funcion) {
    std::string salida(funcion.nombre);
    salida += '(';
    bool primero = true;
    for (const sintactico::Parametro& parametro : funcion.parametros) {
        if (!primero) {
            salida += ", ";
        }
        primero = false;
        salida += parametro.nombre;
        salida += ": ";
        salida += parametro.tipo;
    }
    salida += ')';
    if (funcion.tipo_retorno != "vacio" && !funcion.tipo_retorno.empty()) {
        salida += " -> ";
        salida += funcion.tipo_retorno;
    }
    return salida;
}

/// Marca de un tipo: `estructura`, `clase` o `rasgo`.
std::string_view marca_de(const sintactico::DefinicionTipo& tipo) {
    if (tipo.es_clase) {
        return "clase";
    }
    if (!tipo.base.empty()) {
        return "estructura";
    }
    return "estructura";
}

/// Añade la documentación de un archivo al flujo.
void documentar_archivo(const std::string& nombre, const sintactico::Programa& programa,
                        std::ostream& salida) {
    const bool hay_algo = std::any_of(programa.funciones.begin(), programa.funciones.end(),
                                      [](const sintactico::Funcion& f) { return f.publico; }) ||
                          std::any_of(programa.tipos.begin(), programa.tipos.end(),
                                      [](const sintactico::DefinicionTipo& t) { return t.publico; }) ||
                          std::any_of(programa.enumeraciones.begin(),
                                      programa.enumeraciones.end(),
                                      [](const sintactico::Enumeracion& e) { return e.publico; });
    if (!hay_algo) {
        return;
    }

    salida << "## Módulo `" << nombre << "`\n\n";

    std::vector<const sintactico::DefinicionTipo*> tipos_publicos;
    for (const sintactico::DefinicionTipo& tipo : programa.tipos) {
        if (tipo.publico) {
            tipos_publicos.push_back(&tipo);
        }
    }
    if (!tipos_publicos.empty()) {
        salida << "### Tipos\n\n";
        for (const sintactico::DefinicionTipo* tipo : tipos_publicos) {
            salida << "- **`" << tipo->nombre << "`** (" << marca_de(*tipo) << ')';
            if (!tipo->base.empty()) {
                salida << ", hereda de `" << tipo->base << '`';
            }
            salida << "\n";
            for (const sintactico::Campo& campo : tipo->campos) {
                salida << "  - `" << campo.nombre << "`: " << campo.tipo << "\n";
            }
        }
        salida << "\n";
    }

    std::vector<const sintactico::Funcion*> funciones_publicas;
    for (const sintactico::Funcion& funcion : programa.funciones) {
        if (funcion.publico) {
            funciones_publicas.push_back(&funcion);
        }
    }
    if (!funciones_publicas.empty()) {
        std::sort(funciones_publicas.begin(), funciones_publicas.end(),
                  [](const sintactico::Funcion* a, const sintactico::Funcion* b) {
                      return a->nombre < b->nombre;
                  });
        salida << "### Funciones\n\n";
        for (const sintactico::Funcion* funcion : funciones_publicas) {
            salida << "- `" << firma_de(*funcion) << "`\n";
        }
        salida << "\n";
    }

    std::vector<const sintactico::Enumeracion*> enumeraciones_publicas;
    for (const sintactico::Enumeracion& enumeracion : programa.enumeraciones) {
        if (enumeracion.publico) {
            enumeraciones_publicas.push_back(&enumeracion);
        }
    }
    if (!enumeraciones_publicas.empty()) {
        salida << "### Variantes\n\n";
        for (const sintactico::Enumeracion* enumeracion : enumeraciones_publicas) {
            salida << "- **`" << enumeracion->nombre << "`**: ";
            bool primero = true;
            for (const sintactico::Variante& variante : enumeracion->variantes) {
                if (!primero) {
                    salida << ", ";
                }
                primero = false;
                salida << '`' << variante.nombre << '`';
            }
            salida << "\n";
        }
        salida << "\n";
    }
}

}  // namespace

int comando_documentar(std::span<const std::string_view> argumentos, std::ostream& salida,
                       std::ostream& error) {
    // Los archivos a documentar: los del argumento o todos los `.oki` del
    // directorio actual.
    std::filesystem::path directorio = std::filesystem::current_path();
    bool a_salida = false;
    for (const std::string_view argumento : argumentos) {
        if (argumento == "--salida") {
            a_salida = true;
        } else if (!argumento.empty() && argumento.front() == '-') {
            error << "nooki: la opción «" << argumento << "» no existe\n";
            return USO_INCORRECTO;
        } else {
            directorio = ruta_desde_utf8(argumento);
        }
    }

    std::vector<std::filesystem::path> archivos;
    std::error_code ignorado;
    if (std::filesystem::is_directory(directorio, ignorado)) {
        for (const auto& entrada :
             std::filesystem::recursive_directory_iterator(directorio, ignorado)) {
            if (entrada.is_regular_file(ignorado) && entrada.path().extension() == ".oki") {
                archivos.push_back(entrada.path());
            }
        }
    } else {
        archivos.push_back(directorio);
    }
    std::sort(archivos.begin(), archivos.end());

    if (archivos.empty()) {
        error << "nooki: no hay archivos .oki que documentar\n";
        return DATOS_INCORRECTOS;
    }

    std::ostringstream documento;
    documento << "# API de Nooki\n\n";
    documento << "Lo que cada módulo expone con `publico`.\n\n";

    std::size_t documentados = 0;
    for (const std::filesystem::path& archivo : archivos) {
        std::ifstream flujo(archivo, std::ios::binary);
        if (!flujo) {
            continue;
        }
        std::ostringstream contenido;
        contenido << flujo.rdbuf();

        diagnostico::GestorFuentes fuentes;
        diagnostico::Emisor emisor;
        const auto fuente = fuentes.anadir(ruta_a_utf8(archivo), contenido.str());
        lexico::Lexer lexer(fuentes, fuente, emisor);
        const auto tokens = lexer.todos();
        if (emisor.hay_errores()) {
            continue;  // un archivo roto no se documenta
        }
        sintactico::Parser parser(fuentes, fuente, emisor, tokens);
        const sintactico::Programa programa = parser.analizar();
        if (emisor.hay_errores()) {
            continue;
        }

        std::ostringstream antes_flujo;
        antes_flujo << documento.str();
        documentar_archivo(ruta_a_utf8(archivo.filename()), programa, documento);
        if (documento.str() != antes_flujo.str()) {
            ++documentados;
        }
    }

    if (documentados == 0) {
        error << "nooki: ningún archivo expone nada con «publico»\n";
        return DATOS_INCORRECTOS;
    }

    if (a_salida) {
        salida << documento.str();
    } else {
        std::ofstream archivo_salida("API.md", std::ios::binary | std::ios::trunc);
        if (!archivo_salida) {
            error << "nooki: no se pudo escribir API.md\n";
            return DATOS_INCORRECTOS;
        }
        archivo_salida << documento.str();
        salida << "documentado: API.md (" << documentados << " archivos)\n";
    }
    return EXITO;
}

}  // namespace nooki::cli
