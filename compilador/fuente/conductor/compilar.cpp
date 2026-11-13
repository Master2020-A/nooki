#include "nooki/conductor/compilar.hpp"
#include "nooki/rutas.hpp"

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

/// Biblioteca estándar mínima, escrita en Nooki y empaquetada con el compilador.
/// Archivo que se está compilando: un módulo no puede ser él mismo.
std::filesystem::path archivo_de_entrada;

/// Carpetas de dependencias donde buscar módulos, del manifiesto.
std::vector<std::filesystem::path> rutas_de_modulos;

std::string_view fuente_biblioteca(std::string_view modulo) {
    if (modulo == "resultado") {
        return R"(// Biblioteca estándar: el resultado de una operación que puede fallar.
publico enumeracion resultado {
    Ok(valor: entero)
    Error(mensaje: texto)
}
)";
    }
    if (modulo == "textos") {
        return R"(// Biblioteca estándar: utilidades sobre listas de textos.
publico funcion unir(partes: &Vector<texto>, separador: texto) -> texto {
    variable junto = ""
    variable i = 0
    mientras (i < partes.largo()) {
        si (i > 0) {
            junto = junto + separador
        }
        junto = junto + partes.obtener(i)
        i = i + 1
    }
    retornar junto
}

publico funcion repetir(pieza: &texto, veces: entero) -> texto {
    variable junto = ""
    variable i = 0
    mientras (i < veces) {
        junto = junto + *pieza
        i = i + 1
    }
    retornar junto
}

publico funcion contar(pieza: &texto, aguja: &texto) -> entero {
    variable total = 0
    variable resto = *pieza
    mientras (resto.largo() > 0) {
        variable posicion = resto.buscar(*aguja)
        si (posicion < 0) {
            romper
        }
        total = total + 1
        resto = resto.subtexto(posicion + 1, resto.largo())
    }
    retornar total
}
)";
    }
    if (modulo == "listas") {
        return R"(// Biblioteca estándar: operaciones sobre listas de enteros.
publico funcion suma(numeros: &Vector<entero>) -> entero {
    variable total = 0
    variable i = 0
    mientras (i < numeros.largo()) {
        total = total + numeros.obtener(i)
        i = i + 1
    }
    retornar total
}

publico funcion mayor(numeros: &Vector<entero>) -> entero {
    variable mejor = numeros.obtener(0)
    variable i = 1
    mientras (i < numeros.largo()) {
        si (numeros.obtener(i) > mejor) {
            mejor = numeros.obtener(i)
        }
        i = i + 1
    }
    retornar mejor
}

publico funcion menor(numeros: &Vector<entero>) -> entero {
    variable peor = numeros.obtener(0)
    variable i = 1
    mientras (i < numeros.largo()) {
        si (numeros.obtener(i) < peor) {
            peor = numeros.obtener(i)
        }
        i = i + 1
    }
    retornar peor
}

publico funcion contiene(numeros: &Vector<entero>, buscado: entero) -> booleano {
    variable i = 0
    mientras (i < numeros.largo()) {
        si (numeros.obtener(i) == buscado) {
            retornar verdadero
        }
        i = i + 1
    }
    retornar falso
}

publico funcion promedio(numeros: &Vector<entero>) -> entero {
    si (numeros.largo() == 0) {
        retornar 0
    }
    variable total = 0
    variable i = 0
    mientras (i < numeros.largo()) {
        total = total + numeros.obtener(i)
        i = i + 1
    }
    retornar total / numeros.largo()
}

)";
    }
    if (modulo == "fechas") {
        return R"(// Biblioteca estándar: aritmética de calendario.
publico funcion bisiesto(anio: entero) -> booleano {
    si (anio % 400 == 0) {
        retornar verdadero
    }
    si (anio % 100 == 0) {
        retornar falso
    }
    retornar anio % 4 == 0
}

publico funcion dias_del_mes(mes: entero, anio: entero) -> entero {
    si (mes == 2) {
        si (bisiesto(anio)) {
            retornar 29
        }
        retornar 28
    }
    si (mes == 4 || mes == 6 || mes == 9 || mes == 11) {
        retornar 30
    }
    retornar 31
}

publico funcion dias_del_anio(anio: entero) -> entero {
    si (bisiesto(anio)) {
        retornar 366
    }
    retornar 365
}
)";
    }
    if (modulo == "matematicas") {
        return R"(// Biblioteca estándar: operaciones matemáticas básicas.
publico funcion maximo(a: entero, b: entero) -> entero {
    si (a > b) {
        retornar a
    }
    retornar b
}

publico funcion minimo(a: entero, b: entero) -> entero {
    si (a < b) {
        retornar a
    }
    retornar b
}

publico funcion absoluto(n: entero) -> entero {
    si (n < 0) {
        retornar 0 - n
    }
    retornar n
}

publico funcion cuadrado(n: entero) -> entero {
    retornar n * n
}
)";
    }
    return {};
}

/// Carga un módulo, marca sus funciones con su nombre y añade también los suyos.
bool cargar_modulo(const std::filesystem::path& directorio, std::string_view nombre,
                   diagnostico::GestorFuentes& fuentes, diagnostico::Emisor& emisor,
                   std::vector<std::string>& cargados, sintactico::Programa& destino) {
    const std::string clave(nombre);
    if (std::find(cargados.begin(), cargados.end(), clave) != cargados.end()) {
        return true;  // ya cargado: un ciclo no duplica el módulo
    }
    cargados.push_back(clave);

    // Se busca junto al archivo que importa, en su carpeta «biblioteca» y
    // subiendo por los directorios padre hasta encontrar una.
    // Se parte de una ruta absoluta para poder subir por los directorios padre.
    std::error_code error_ruta;
    const std::filesystem::path base =
        directorio.empty() ? std::filesystem::current_path(error_ruta)
                           : std::filesystem::absolute(directorio, error_ruta);
    std::filesystem::path ruta = base / (clave + ".oki");

    // Las dependencias del manifiesto se miran antes de subir por los padres.
    for (const std::filesystem::path& extra : rutas_de_modulos) {
        const std::filesystem::path candidato = extra / (clave + ".oki");
        if (std::filesystem::exists(candidato)) {
            ruta = candidato;
            break;
        }
    }
    for (std::filesystem::path actual = base; !std::filesystem::exists(ruta);) {
        const std::filesystem::path candidato = actual / "biblioteca" / (clave + ".oki");
        if (std::filesystem::exists(candidato)) {
            ruta = candidato;
            break;
        }
        if (!actual.has_parent_path() || actual.parent_path() == actual) {
            break;
        }
        actual = actual.parent_path();
    }

    // Un archivo no puede importarse a sí mismo.
    if (!archivo_de_entrada.empty() && std::filesystem::exists(ruta)) {
        std::error_code error_canonico;
        const std::filesystem::path modulo_real =
            std::filesystem::weakly_canonical(ruta, error_canonico);
        if (!error_canonico && modulo_real == archivo_de_entrada) {
            emisor.error(diagnostico::codigos::MODULO_NO_ENCONTRADO, diagnostico::Span{})
                .parametro("modulo", clave)
                .ayuda("un archivo no puede usarse a sí mismo como módulo");
            return false;
        }
    }

    std::string contenido = leer_archivo(ruta);
    if (contenido.empty() && !std::filesystem::exists(ruta)) {
        // Los módulos de la biblioteca viajan dentro del compilador.
        const std::string_view integrado = fuente_biblioteca(clave);
        if (integrado.empty()) {
            emisor.error(diagnostico::codigos::MODULO_NO_ENCONTRADO, diagnostico::Span{})
                .parametro("modulo", clave)
                .ayuda("se buscó «" + ruta_a_utf8(ruta) + "»");
            return false;
        }
        contenido = std::string(integrado);
    }

    const auto archivo = fuentes.anadir(ruta_a_utf8(ruta), contenido);
    lexico::Lexer lexer(fuentes, archivo, emisor);
    const auto tokens = lexer.todos();
    sintactico::Parser parser(fuentes, archivo, emisor, tokens);
    sintactico::Programa sub = parser.analizar();
    if (emisor.hay_errores()) {
        return false;
    }

    // Un módulo es una biblioteca: no tiene punto de entrada.
    for (const sintactico::Funcion& funcion : sub.funciones) {
        if (funcion.nombre == "principal") {
            emisor.error(diagnostico::codigos::TOKEN_INESPERADO, funcion.span)
                .parametro("esperado", "un módulo sin «principal»")
                .parametro("encontrado", std::string(clave) + ".principal")
                .ayuda("un módulo es una biblioteca: quite «principal» de «" +
                       std::string(clave) + ".oki»");
            return false;
        }
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

/// Sustituye por «_» todo byte que no sea ASCII imprimible.
std::string nombre_ascii(const std::string& nombre) {
    std::string salida;
    salida.reserve(nombre.size());
    for (const unsigned char caracter : nombre) {
        salida += (caracter >= 0x20 && caracter < 0x7F) ? static_cast<char>(caracter) : '_';
    }
    if (salida.empty()) {
        salida = "programa";
    }
    return salida;
}

/// ¿Todos los bytes de la ruta son ASCII imprimible?
bool ruta_es_ascii(const std::filesystem::path& ruta) {
    for (const unsigned char caracter : ruta_a_utf8(ruta)) {
        if (caracter >= 0x80) {
            return false;
        }
    }
    return true;
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
            .parametro("archivo", ruta_a_utf8(opciones.fuente));
        return Resultado{1, {}, {}};
    }

    rutas_de_modulos = opciones.rutas_de_modulos;
    std::error_code error_entrada;
    archivo_de_entrada = std::filesystem::weakly_canonical(opciones.fuente, error_entrada);
    if (error_entrada) {
        archivo_de_entrada = opciones.fuente;
    }

    const auto archivo = fuentes.anadir(ruta_a_utf8(opciones.fuente), contenido);

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

    // El archivo intermedio lleva un nombre ASCII: la consola y el compilador de C
    // no tratan igual los nombres con tildes en todas las plataformas, y el
    // contenido ya identifica el programa por su ruta en los diagnósticos.
    const std::filesystem::path intermedio =
        std::filesystem::temp_directory_path() /
        ("nooki_" + nombre_ascii(ruta_a_utf8(opciones.fuente.stem())) + ".c");

    {
        std::ofstream archivo_c(intermedio, std::ios::binary | std::ios::trunc);
        archivo_c << resultado.c_generado;
    }

    const std::filesystem::path salida_binario = ruta_de_salida(opciones);
    // El enlazador de MinGW no abre rutas con tildes: se enlaza en una ruta ASCII
    // y el binario se mueve después, que sí entiende Unicode.
    std::error_code error_mover;
    const bool necesita_mover =
        !ruta_es_ascii(salida_binario) &&
        std::filesystem::path(salida_binario).has_parent_path();
    std::filesystem::path destino_enlace = salida_binario;
    if (necesita_mover) {
        destino_enlace = std::filesystem::temp_directory_path() /
                         ("nooki_salida_" + nombre_ascii(ruta_a_utf8(opciones.fuente.stem())) +
                          EXTENSION_BINARIO);
    }

    // La consola del sistema no entiende «/» como separador: se usa el nativo.
    std::filesystem::path binario_nativo = destino_enlace;
    std::filesystem::path intermedio_nativo = intermedio;
    const std::string comando =
        entrecomillar_si_hace_falta(compilador) + " -O2 -o " +
        entrecomillar_si_hace_falta(ruta_a_utf8(binario_nativo.make_preferred())) + " " +
        entrecomillar_si_hace_falta(ruta_a_utf8(intermedio_nativo.make_preferred()));

    const int codigo = ejecutar_comando(comando);
    if (codigo != 0) {
        emisor.error(diagnostico::codigos::FALLO_COMPILADOR_C, diagnostico::Span{})
            .parametro("compilador", compilador)
            .parametro("codigo", std::to_string(codigo))
            .nota("C generado en " + ruta_a_utf8(intermedio));
        return Resultado{1, resultado.c_generado, {}};
    }

    // El binario se enlazó en una ruta ASCII si el destino tenía tildes: se mueve.
    if (necesita_mover) {
        std::filesystem::copy_file(destino_enlace, salida_binario,
                                   std::filesystem::copy_options::overwrite_existing,
                                   error_mover);
        if (error_mover) {
            emisor.error(diagnostico::codigos::FALLO_COMPILADOR_C, diagnostico::Span{})
                .parametro("compilador", compilador)
                .parametro("codigo", std::to_string(error_mover.value()))
                .nota("no se pudo mover el binario a «" + ruta_a_utf8(salida_binario) + "»");
            return Resultado{1, resultado.c_generado, {}};
        }
        std::error_code ignorado;
        std::filesystem::remove(destino_enlace, ignorado);
    }

    if (opciones.conservar_intermedios) {
        error << "nooki: C intermedio conservado en " << ruta_a_utf8(intermedio) << '\n';
    } else {
        std::error_code ignorado;
        std::filesystem::remove(intermedio, ignorado);
    }

    resultado.binario = salida_binario;
    return resultado;
}

}  // namespace nooki::conductor
