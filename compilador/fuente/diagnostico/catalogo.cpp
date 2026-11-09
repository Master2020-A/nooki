#include "nooki/diagnostico/catalogo.hpp"

#include <array>
#include <cstddef>
#include <string>

#include "nooki/diagnostico/codigos.hpp"

namespace nooki::diagnostico {
namespace {

struct EntradaMensaje {
    std::string_view codigo;
    std::string_view espanol;
    std::string_view ingles;
};

// Los mensajes viven aquí y en ningún otro sitio: cambiar una redacción es cambiar
// una línea, y traducir es rellenar la tercera columna.
constexpr std::array<EntradaMensaje, 29> MENSAJES{{
    {codigos::ARCHIVO_NO_ENCONTRADO, "no se pudo leer el archivo «{archivo}»",
     "cannot read file “{archivo}”"},

    {codigos::PALABRA_RESERVADA_CON_TILDE,
     "«{palabra}» no es una palabra reservada: las palabras clave se escriben sin tilde",
     "“{palabra}” is not a reserved word: keywords are written without accents"},

    {codigos::TEXTO_SIN_CERRAR, "texto sin cerrar", "unterminated string literal"},

    {codigos::CARACTER_INVALIDO, "el carácter «{caracter}» no se usa en Nooki",
     "character “{caracter}” is not used in Nooki"},

    {codigos::NUMERO_MAL_FORMADO, "número mal formado", "malformed number"},

    {codigos::UTF8_INVALIDO, "secuencia UTF-8 no válida", "invalid UTF-8 sequence"},

    {codigos::CARACTER_LITERAL_SIN_CERRAR, "falta cerrar el carácter literal",
     "unterminated character literal"},

    {codigos::COMENTARIO_SIN_CERRAR, "comentario sin cerrar", "unterminated comment"},

    {codigos::ESCAPE_DESCONOCIDO, "la secuencia de escape «{escape}» no existe",
     "unknown escape sequence “{escape}”"},

    {codigos::PARENTESIS_SIN_CERRAR, "falta cerrar «{apertura}»",
     "unclosed “{apertura}”"},

    {codigos::TOKEN_INESPERADO, "se esperaba {esperado} y se encontró «{encontrado}»",
     "expected {esperado} but found “{encontrado}”"},

    {codigos::SIN_PRINCIPAL, "el programa no declara una función «principal»",
     "the program does not declare a “principal” function"},

    {codigos::NO_SOPORTADO, "«{construccion}» todavía no está soportado",
     "“{construccion}” is not supported yet"},

    {codigos::RANGO_ENCADENADO, "los rangos no se pueden encadenar",
     "ranges cannot be chained"},

    {codigos::SIN_COMPILADOR_C,
     "no se encontró un compilador de C; instale gcc, clang o MSVC, o indique uno con "
     "la variable NOOKI_CC",
     "no C compiler found; install gcc, clang or MSVC, or set the NOOKI_CC variable"},

    {codigos::FALLO_COMPILADOR_C, "el compilador de C «{compilador}» terminó con el código {codigo}",
     "the C compiler “{compilador}” exited with code {codigo}"},

    {codigos::BACKEND_NO_SOPORTA,
     "el backend todavía no sabe traducir {construccion}",
     "the backend cannot translate {construccion} yet"},

    {codigos::NOMBRE_NO_DECLARADO, "no se ha declarado «{nombre}»",
     "cannot find “{nombre}” in this scope"},

    {codigos::FUNCION_NO_DECLARADA, "no existe ninguna función «{nombre}»",
     "there is no function named “{nombre}”"},

    {codigos::TIPO_DESCONOCIDO, "no existe ningún tipo «{nombre}»",
     "there is no type named “{nombre}”"},

    {codigos::CAMPO_DESCONOCIDO, "«{tipo}» no tiene ningún campo «{campo}»",
     "“{tipo}” has no field named “{campo}”"},

    {codigos::CAMPO_FALTANTE, "a «{tipo}» le falta el campo «{campo}»",
     "“{tipo}” is missing the field “{campo}”"},

    {codigos::TIPO_INCOMPATIBLE,
     "se esperaba un valor de tipo «{esperado}» pero se encontró «{encontrado}»",
     "expected a value of type “{esperado}” but found “{encontrado}”"},

    {codigos::ARGUMENTO_INCOMPATIBLE,
     "el argumento «{parametro}» de «{funcion}» espera «{esperado}» y se pasa «{encontrado}»",
     "argument “{parametro}” of “{funcion}” expects “{esperado}” but got “{encontrado}”"},

    {codigos::ARIDAD_INCORRECTA,
     "«{funcion}» espera {esperados} argumento(s) y se han pasado {dados}",
     "“{funcion}” expects {esperados} argument(s) but {dados} were given"},

    {codigos::RETORNO_INCOMPATIBLE, "«{funcion}» debe devolver «{esperado}» y se devuelve «{encontrado}»",
     "“{funcion}” must return “{esperado}” but “{encontrado}” is returned"},

    {codigos::CONDICION_NO_BOOLEANA, "la condición debe ser de tipo «booleano» y es «{tipo}»",
     "the condition must be “booleano” but is “{tipo}”"},

    {codigos::USO_TRAS_MOVER, "no se puede usar «{nombre}»: el valor se movió antes",
     "cannot use “{nombre}”: the value was moved before"},

    {codigos::VARIABLE_SIN_MUTAR, "la variable «{nombre}» nunca se modifica",
     "variable “{nombre}” is never mutated"},
}};

struct EntradaFija {
    std::string_view clave;
    std::string_view espanol;
    std::string_view ingles;
};

constexpr std::array<EntradaFija, 5> FIJOS{{
    {"error", "error", "error"},
    {"advertencia", "advertencia", "warning"},
    {"nota", "nota", "note"},
    {"ayuda", "ayuda", "help"},
    {"sin_descripcion", "diagnóstico sin descripción", "diagnostic without description"},
}};

}  // namespace

std::string_view plantilla_mensaje(std::string_view codigo, Idioma idioma) {
    for (const EntradaMensaje& entrada : MENSAJES) {
        if (entrada.codigo == codigo) {
            return idioma == Idioma::Espanol ? entrada.espanol : entrada.ingles;
        }
    }
    return {};
}

std::string_view texto_fijo(std::string_view clave, Idioma idioma) {
    for (const EntradaFija& entrada : FIJOS) {
        if (entrada.clave == clave) {
            return idioma == Idioma::Espanol ? entrada.espanol : entrada.ingles;
        }
    }
    return {};
}

namespace {

void sustituir(std::string& texto, std::string_view nombre, std::string_view valor) {
    const std::string marca = "{" + std::string(nombre) + "}";
    std::size_t posicion = 0;
    while ((posicion = texto.find(marca, posicion)) != std::string::npos) {
        texto.replace(posicion, marca.size(), valor);
        posicion += valor.size();
    }
}

}  // namespace

std::string componer(std::string_view codigo, const Parametros& parametros, Idioma idioma) {
    const std::string_view plantilla = plantilla_mensaje(codigo, idioma);
    if (plantilla.empty()) {
        std::string salida(texto_fijo("sin_descripcion", idioma));
        salida += " (";
        salida += codigo;
        salida += ')';
        return salida;
    }

    std::string salida(plantilla);
    for (const auto& [nombre, valor] : parametros) {
        sustituir(salida, nombre, valor);
    }
    return salida;
}

}  // namespace nooki::diagnostico
