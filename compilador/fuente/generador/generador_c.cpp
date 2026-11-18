#include "nooki/generador/generador_c.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/semantico/tipos.hpp"

namespace nooki::generador {
namespace {

/// Tipo de C de un tipo escrito en Nooki.
/// Nombres de los rasgos del programa, para resolver `&Rasgo` en los tipos.
inline std::set<std::string>* RASGOS_DEL_PROGRAMA = nullptr;
/// Estructuras que tienen vector propio; lo fija el emisor al empezar.
inline const std::set<std::string>* VECTORES_DE_ESTRUCTURA = nullptr;

/// Un puntero al valor de una expresión. Si no es un sitio (una variable, un
/// campo), se le da uno temporal con un literal compuesto: así una llamada como
/// `sistema.argumentos()` también se puede pasar a un método de colección.
inline std::string direccion_de_expresion(const sintactico::Expresion& expresion,
                                          const std::string& tipo, const std::string& codigo) {
    const bool es_sitio = expresion.tipo == sintactico::TipoExpresion::Identificador ||
                          expresion.tipo == sintactico::TipoExpresion::Miembro ||
                          expresion.tipo == sintactico::TipoExpresion::Indice ||
                          expresion.tipo == sintactico::TipoExpresion::Este;
    if (es_sitio || tipo.empty() || tipo == "void") {
        return "&" + codigo;
    }
    return "(&((" + tipo + "[]){ " + codigo + " })[0])";
}
/// Nombre en Nooki → nombre en C de las funciones de punteros.
inline const std::map<std::string, std::string> PUNTEROS_C = {
    {"reservar", "reservar"},
    {"liberar", "liberar"},
    {"leer_en", "leer_entero"},
    {"escribir_en", "escribir_entero"},
    {"leer_flotante_en", "leer_flotante"},
    {"escribir_flotante_en", "escribir_flotante"},
    {"desde_direccion", "desde_direccion"},
    {"direccion_de", "direccion"},
};

bool es_rasgo_c(std::string_view nombre) {
    return RASGOS_DEL_PROGRAMA != nullptr && RASGOS_DEL_PROGRAMA->count(std::string(nombre)) != 0;
}

std::string tipo_c(std::string_view escrito) {
    if (!escrito.empty() && (escrito.front() == '&' || escrito.front() == '*')) {
        std::string_view resto = escrito.substr(1);
        if (resto.rfind("mutable ", 0) == 0) {
            resto = resto.substr(8);
        }
        // Una referencia a un rasgo lleva el objeto y su tabla.
        if (escrito.front() == '&' && es_rasgo_c(resto)) {
            return "nooki_ref_" + std::string(resto);
        }

        return tipo_c(resto) + "*";
    }
    if (escrito.rfind("Caja<", 0) == 0) {
        return "nooki_caja";
    }
    if (escrito.rfind("Mapa<", 0) == 0) {
        // El valor del mapa puede ser entero, flotante o texto.
        if (escrito.find("flotante") != std::string_view::npos) {
            return "nooki_mapa_texto_flotante";
        }
        if (escrito.rfind("Mapa<texto, texto>", 0) == 0) {
            return "nooki_mapa_texto_texto";
        }
        return "nooki_mapa_texto_entero";
    }

    if (escrito.rfind("Vector<", 0) == 0) {
        if (escrito.find("texto") != std::string_view::npos) {
            return "nooki_vector_texto";
        }
        if (escrito.find("flotante") != std::string_view::npos) {
            return "nooki_vector_flotante";
        }
        // `Vector<MiEstructura>` tiene su vector propio.
        const std::size_t cierra = escrito.find('>');
        if (cierra != std::string_view::npos) {
            const std::string dentro(escrito.substr(7, cierra - 7));
            if (VECTORES_DE_ESTRUCTURA != nullptr && VECTORES_DE_ESTRUCTURA->count(dentro) != 0) {
                return "nooki_vector_" + dentro;
            }
        }
        return "nooki_vector_entero";
    }
    switch (nooki::semantico::desde_nombre(escrito)) {
    case nooki::semantico::Tipo::Texto:
        return "nooki_texto";
    case nooki::semantico::Tipo::Booleano:
        return "int";
    case nooki::semantico::Tipo::Flotante:
        return "double";
    case nooki::semantico::Tipo::Vacio:
        return "void";
    case nooki::semantico::Tipo::Desconocido:
    case nooki::semantico::Tipo::Entero:
        return "long long";
    }
    return "long long";
}

/// Convierte las secuencias de escape del fuente en los caracteres que
/// representan: `\n` en un salto, `\t` en un tabulador, `\\` en una barra.
/// El lexer ya comprobó que son válidas; aquí solo se aplican.
std::string desescapar(std::string_view texto) {
    std::string salida;
    salida.reserve(texto.size());
    for (std::size_t i = 0; i < texto.size(); ++i) {
        if (texto[i] != '\\' || i + 1 >= texto.size()) {
            salida += texto[i];
            continue;
        }
        switch (texto[i + 1]) {
        case 'n':
            salida += '\n';
            break;
        case 'r':
            salida += '\r';
            break;
        case 't':
            salida += '\t';
            break;
        case '0':
            salida += '\0';
            break;
        case '\\':
            salida += '\\';
            break;
        case '"':
            salida += '"';
            break;
        case '\'':
            salida += '\'';
            break;
        default:
            // Una secuencia desconocida (el lexer ya avisó) se deja tal cual.
            salida += '\\';
            salida += texto[i + 1];
            break;
        }
        ++i;
    }
    return salida;
}

std::string escapar_c(std::string_view texto) {
    std::string salida;
    salida.reserve(texto.size() + 2);
    for (const char caracter : texto) {
        switch (caracter) {
        case '"':
            salida += "\\\"";
            break;
        case '\\':
            salida += "\\\\";
            break;
        case '\n':
            salida += "\\n";
            break;
        case '\r':
            salida += "\\r";
            break;
        case '\t':
            salida += "\\t";
            break;
        default:
            salida += caracter;
            break;
        }
    }
    return salida;
}

std::string limpiar_separadores(std::string_view numero) {
    std::string salida;
    salida.reserve(numero.size());
    for (const char caracter : numero) {
        if (caracter != '_') {
            salida += caracter;
        }
    }
    return salida;
}

std::string nombre_c(std::string_view nombre) {
    if (nombre == "principal") {
        return "main";
    }
    return "nooki_" + std::string(nombre);
}

/// Nombre de C de una función, con el módulo por delante cuando lo tiene.
std::string nombre_funcion_c(const sintactico::Funcion& funcion) {
    if (funcion.nombre == "principal") {
        return "main";
    }
    std::string salida = "nooki_";
    if (!funcion.modulo.empty()) {
        salida += funcion.modulo;
        salida += "_";
    }
    salida += funcion.nombre;
    return salida;
}

std::string mayusculas(std::string_view texto) {
    std::string salida;
    salida.reserve(texto.size());
    for (const char caracter : texto) {
        if (caracter >= 'a' && caracter <= 'z') {
            salida += static_cast<char>(caracter - 'a' + 'A');
        } else {
            salida += caracter;
        }
    }
    return salida;
}

/// El tipo en C del elemento de un vector, a partir del tipo del vector.
std::string elemento_de_vector(const std::string& tipo_vector) {
    if (tipo_vector.rfind("nooki_vector_texto", 0) == 0) {
        return "nooki_texto";
    }
    if (tipo_vector.rfind("nooki_vector_flotante", 0) == 0) {
        return "double";
    }
    // Un vector de estructura devuelve la estructura. El tipo puede venir con
    // un `*` detrás si se pasó prestado: eso no cambia el elemento.
    if (tipo_vector.rfind("nooki_vector_", 0) == 0) {
        std::string dentro = tipo_vector.substr(std::string("nooki_vector_").size());
        while (!dentro.empty() && dentro.back() == '*') {
            dentro.pop_back();
        }
        if (dentro != "entero" && dentro != "texto" && dentro != "flotante") {
            return "nooki_" + dentro;
        }
    }
    return "long long";
}

std::string constante_enum(std::string_view enumeracion, std::string_view variante) {
    return "NOOKI_" + mayusculas(enumeracion) + "_" + mayusculas(variante);
}

std::string tipo_estructura(std::string_view nombre) {
    return "nooki_" + std::string(nombre);
}

/// Nombre de C de una estructura, con el módulo por delante cuando lo tiene.
std::string tipo_estructura(const sintactico::DefinicionTipo& definicion) {
    std::string salida = "nooki_";
    if (!definicion.modulo.empty()) {
        salida += definicion.modulo;
        salida += "_";
    }
    salida += definicion.nombre;
    return salida;
}

const char* PRELUDIO =
    "/* Generado por Nooki. No editar: se regenera en cada compilación. */\n"
    "#include <limits.h>\n"
    "#include <stdint.h>\n"
    "#include <stdio.h>\n"
    "#include <stdlib.h>\n"
    "#include <string.h>\n"
    "\n"
    "/* texto: cadena con dueno opcional (propio = 1 cuando hay que liberarla). */\n"
    "typedef struct {\n"
    "    char* datos;\n"
    "    long long largo;\n"
    "    int propio;\n"
    "} nooki_texto;\n"
    "\n"
    "static nooki_texto nooki_texto_desde_c(const char* datos) {\n"
    "    nooki_texto t;\n"
    "    if (datos == NULL) {\n"
    "        t.datos = NULL;\n"
    "        t.largo = 0;\n"
    "        t.propio = 0;\n"
    "        return t;\n"
    "    }\n"
    "    const size_t largo = strlen(datos);\n"
    "    t.datos = (char*)malloc(largo + 1);\n"
    "    if (t.datos == NULL) {\n"
    "        t.largo = 0;\n"
    "        t.propio = 0;\n"
    "        return t;\n"
    "    }\n"
    "    memcpy(t.datos, datos, largo + 1);\n"
    "    t.largo = (long long)largo;\n"
    "    t.propio = 1;\n"
    "    return t;\n"
    "}\n"
    "\n"
    "/* Asigna un texto liberando el búfer que tuviera: sin fugas al reasignar. */\n"
    "static void nooki_texto_asignar(nooki_texto* destino, nooki_texto nuevo) {\n"
    "    if (destino->propio && destino->datos != NULL && destino->datos != nuevo.datos) {\n"
    "        free(destino->datos);\n"
    "    }\n"
    "    *destino = nuevo;\n"
    "}\n"
    "\n"
    "/* Copia un texto: el resultado tiene su propio búfer. */\n"
    "static nooki_texto nooki_texto_copiar(nooki_texto t) {\n"
    "    nooki_texto salida;\n"
    "    if (t.datos == NULL || t.largo <= 0) {\n"
    "        salida.datos = NULL;\n"
    "        salida.largo = 0;\n"
    "        salida.propio = 0;\n"
    "        return salida;\n"
    "    }\n"
    "    salida.datos = (char*)malloc((size_t)t.largo + 1);\n"
    "    if (salida.datos == NULL) {\n"
    "        salida.largo = 0;\n"
    "        salida.propio = 0;\n"
    "        return salida;\n"
    "    }\n"
    "    memcpy(salida.datos, t.datos, (size_t)t.largo);\n"
    "    salida.datos[t.largo] = '\\0';\n"
    "    salida.largo = t.largo;\n"
    "    salida.propio = 1;\n"
    "    return salida;\n"
    "}\n"
    "\n"
    "/* Añade un código de carácter (UTF-8) al final del texto. */\n"
    "static nooki_texto nooki_texto_agregar_codigo(nooki_texto t, long long codigo) {\n"
    "    char bytes[5];\n"
    "    int cuantos = 0;\n"
    "    const unsigned long punto = (unsigned long)codigo;\n"
    "    if (punto < 0x80) {\n"
    "        bytes[cuantos++] = (char)punto;\n"
    "    } else if (punto < 0x800) {\n"
    "        bytes[cuantos++] = (char)(0xC0 | (punto >> 6));\n"
    "        bytes[cuantos++] = (char)(0x80 | (punto & 0x3F));\n"
    "    } else if (punto < 0x10000) {\n"
    "        bytes[cuantos++] = (char)(0xE0 | (punto >> 12));\n"
    "        bytes[cuantos++] = (char)(0x80 | ((punto >> 6) & 0x3F));\n"
    "        bytes[cuantos++] = (char)(0x80 | (punto & 0x3F));\n"
    "    } else if (punto < 0x110000) {\n"
    "        bytes[cuantos++] = (char)(0xF0 | (punto >> 18));\n"
    "        bytes[cuantos++] = (char)(0x80 | ((punto >> 12) & 0x3F));\n"
    "        bytes[cuantos++] = (char)(0x80 | ((punto >> 6) & 0x3F));\n"
    "        bytes[cuantos++] = (char)(0x80 | (punto & 0x3F));\n"
    "    }\n"
    "    nooki_texto salida;\n"
    "    salida.largo = t.largo + cuantos;\n"
    "    salida.datos = (char*)malloc((size_t)salida.largo + 1);\n"
    "    if (salida.datos == NULL) {\n"
    "        salida.largo = 0;\n"
    "        salida.propio = 0;\n"
    "        return salida;\n"
    "    }\n"
    "    if (t.largo > 0) {\n"
    "        memcpy(salida.datos, t.datos, (size_t)t.largo);\n"
    "    }\n"
    "    for (int i = 0; i < cuantos; ++i) {\n"
    "        salida.datos[t.largo + i] = bytes[i];\n"
    "    }\n"
    "    salida.datos[salida.largo] = '\\0';\n"
    "    salida.propio = 1;\n"
    "    return salida;\n"
    "}\n"
    "\n"
    "/* Quita el último carácter (un punto de código, no un byte). */\n"
    "static nooki_texto nooki_texto_sin_ultimo(nooki_texto t) {\n"
    "    long long fin = t.largo;\n"
    "    while (fin > 0 && ((unsigned char)t.datos[fin - 1] & 0xC0) == 0x80) {\n"
    "        --fin;\n"
    "    }\n"
    "    if (fin > 0) {\n"
    "        --fin;\n"
    "    }\n"
    "    nooki_texto salida;\n"
    "    salida.largo = fin;\n"
    "    salida.datos = (char*)malloc((size_t)fin + 1);\n"
    "    if (salida.datos == NULL) {\n"
    "        salida.largo = 0;\n"
    "        salida.propio = 0;\n"
    "        return salida;\n"
    "    }\n"
    "    if (fin > 0) {\n"
    "        memcpy(salida.datos, t.datos, (size_t)fin);\n"
    "    }\n"
    "    salida.datos[fin] = '\\0';\n"
    "    salida.propio = 1;\n"
    "    return salida;\n"
    "}\n"
    "\n"
    "static nooki_texto nooki_texto_literal(const char* datos) {\n"
    "    nooki_texto t;\n"
    "    t.datos = (char*)datos;\n"
    "    t.largo = (long long)strlen(datos);\n"
    "    t.propio = 0;\n"
    "    return t;\n"
    "}\n"
    "\n"
    "static nooki_texto nooki_texto_concatenar(nooki_texto a, nooki_texto b) {\n"
    "    nooki_texto t;\n"
    "    t.largo = a.largo + b.largo;\n"
    "    t.propio = 1;\n"
    "    t.datos = (char*)malloc((size_t)t.largo + 1);\n"
    "    if (t.datos != NULL) {\n"
    "        memcpy(t.datos, a.datos, (size_t)a.largo);\n"
    "        memcpy(t.datos + a.largo, b.datos, (size_t)b.largo);\n"
    "        t.datos[t.largo] = 0;\n"
    "    } else {\n"
    "        t.largo = 0;\n"
    "        t.propio = 0;\n"
    "    }\n"
    "    return t;\n"
    "}\n"
    "\n"
    "static long long nooki_texto_comparar(nooki_texto a, nooki_texto b) {\n"
    "    long long minimo = a.largo < b.largo ? a.largo : b.largo;\n"
    "    int resultado = memcmp(a.datos, b.datos, (size_t)minimo);\n"
    "    if (resultado != 0) {\n"
    "        return resultado < 0 ? -1 : 1;\n"
    "    }\n"
    "    if (a.largo == b.largo) {\n"
    "        return 0;\n"
    "    }\n"
    "    return a.largo < b.largo ? -1 : 1;\n"
    "}\n"
    "\n"
    "static long long nooki_texto_largo(nooki_texto t) {\n"
    "    return t.largo;\n"
    "}\n"
    "\n"
    "/* Operaciones de texto: cortar, buscar y convertir. */\n"
    "static nooki_texto nooki_texto_subtexto(nooki_texto t, long long desde, long long cuantos) {\n"
    "    nooki_texto r;\n"
    "    r.propio = 1;\n"
    "    r.largo = 0;\n"
    "    r.datos = NULL;\n"
    "    if (desde < 0 || desde >= t.largo || cuantos <= 0) {\n"
    "        r.propio = 0;\n"
    "        return r;\n"
    "    }\n"
    "    r.largo = (desde + cuantos > t.largo) ? (t.largo - desde) : cuantos;\n"
    "    r.datos = (char*)malloc((size_t)r.largo + 1);\n"
    "    if (r.datos == NULL) {\n"
    "        r.largo = 0;\n"
    "        r.propio = 0;\n"
    "        return r;\n"
    "    }\n"
    "    memcpy(r.datos, t.datos + desde, (size_t)r.largo);\n"
    "    r.datos[r.largo] = 0;\n"
    "    return r;\n"
    "}\n"
    "\n"
    "static long long nooki_texto_caracter(nooki_texto t, long long indice) {\n"
    "    if (indice < 0 || indice >= t.largo) {\n"
    "        return -1;\n"
    "    }\n"
    "    return (long long)(unsigned char)t.datos[indice];\n"
    "}\n"
    "\n"
    "static long long nooki_texto_buscar(nooki_texto t, nooki_texto aguja) {\n"
    "    if (aguja.largo == 0 || aguja.largo > t.largo) {\n"
    "        return -1;\n"
    "    }\n"
    "    for (long long i = 0; i + aguja.largo <= t.largo; ++i) {\n"
    "        if (memcmp(t.datos + i, aguja.datos, (size_t)aguja.largo) == 0) {\n"
    "            return i;\n"
    "        }\n"
    "    }\n"
    "    return -1;\n"
    "}\n"
    "\n"
    "static int nooki_texto_contiene(nooki_texto t, nooki_texto aguja) {\n"
    "    return nooki_texto_buscar(t, aguja) >= 0;\n"
    "}\n"
    "\n"
    "static int nooki_texto_empieza_con(nooki_texto t, nooki_texto prefijo) {\n"
    "    if (prefijo.largo > t.largo) {\n"
    "        return 0;\n"
    "    }\n"
    "    return memcmp(t.datos, prefijo.datos, (size_t)prefijo.largo) == 0;\n"
    "}\n"
    "\n"
    "static long long nooki_texto_a_entero(nooki_texto t) {\n"
    "    long long valor = 0;\n"
    "    long long signo = 1;\n"
    "    long long i = 0;\n"
    "    if (t.largo > 0 && t.datos[0] == '-') {\n"
    "        signo = -1;\n"
    "        i = 1;\n"
    "    }\n"
    "    for (; i < t.largo; ++i) {\n"
    "        const char d = t.datos[i];\n"
    "        if (d < '0' || d > '9') {\n"
    "            break;\n"
    "        }\n"
    "        valor = valor * 10 + (d - '0');\n"
    "    }\n"
    "    return signo * valor;\n"
    "}\n"
    "\n"
    "/* Convierte un texto a flotante: signo, digitos, punto decimal y nada mas.\n"
    " * Si no hay ningun digito devuelve 0. Acepta la coma como separador. */\n"
    "static double nooki_texto_a_flotante(nooki_texto t) {\n"
    "    double valor = 0.0;\n"
    "    double signo = 1.0;\n"
    "    long long i = 0;\n"
    "    int decimales = 0;\n"
    "    double escala = 1.0;\n"
    "    if (t.largo > 0 && (t.datos[0] == '-' || t.datos[0] == '+')) {\n"
    "        if (t.datos[0] == '-') {\n"
    "            signo = -1.0;\n"
    "        }\n"
    "        i = 1;\n"
    "    }\n"
    "    for (; i < t.largo; ++i) {\n"
    "        const char d = t.datos[i];\n"
    "        if (d >= '0' && d <= '9') {\n"
    "            valor = valor * 10.0 + (double)(d - '0');\n"
    "            if (decimales > 0) {\n"
    "                escala *= 10.0;\n"
    "            }\n"
    "        } else if ((d == '.' || d == ',') && decimales == 0) {\n"
    "            decimales = 1;\n"
    "        } else {\n"
    "            break;\n"
    "        }\n"
    "    }\n"
    "    return signo * valor / escala;\n"
    "}\n"
    "\n"
    "static void nooki_texto_soltar(nooki_texto* t) {\n"
    "    if (t->propio && t->datos != NULL) {\n"
    "        free(t->datos);\n"
    "    }\n"
    "    t->datos = NULL;\n"
    "    t->largo = 0;\n"
    "    t->propio = 0;\n"
    "}\n"
    "\n"
    "static void nooki_imprimir_texto(nooki_texto t) {\n"
    "    if (t.datos != NULL) {\n"
    "        fwrite(t.datos, 1, (size_t)t.largo, stdout);\n"
    "    }\n"
    "    fputc(10, stdout);\n"
    "}\n"
    "\n"
    "/* Vector<T>: arreglo dinámico con dueño (S25). */\n"
    "typedef struct {\n"
    "    long long* datos;\n"
    "    long long largo;\n"
    "    long long capacidad;\n"
    "} nooki_vector_entero;\n"
    "\n"
    "static nooki_vector_entero nooki_vector_entero_nuevo(void) {\n"
    "    nooki_vector_entero v;\n"
    "    v.datos = NULL;\n"
    "    v.largo = 0;\n"
    "    v.capacidad = 0;\n"
    "    return v;\n"
    "}\n"
    "\n"
    "static void nooki_vector_entero_agregar(nooki_vector_entero* v, long long valor) {\n"
    "    if (v->largo == v->capacidad) {\n"
    "        long long nueva = v->capacidad == 0 ? 4 : v->capacidad * 2;\n"
    "        long long* datos = (long long*)realloc(v->datos, sizeof(long long) * (size_t)nueva);\n"
    "        if (datos == NULL) {\n"
    "            return;\n"
    "        }\n"
    "        v->datos = datos;\n"
    "        v->capacidad = nueva;\n"
    "    }\n"
    "    v->datos[v->largo] = valor;\n"
    "    v->largo += 1;\n"
    "}\n"
    "\n"
    "static long long nooki_vector_entero_obtener(nooki_vector_entero* v, long long indice) {\n"
    "    if (indice < 0 || indice >= v->largo) {\n"
    "        return 0;\n"
    "    }\n"
    "    return v->datos[indice];\n"
    "}\n"
    "\n"
    "static long long nooki_vector_entero_largo(nooki_vector_entero* v) {\n"
    "    return v->largo;\n"
    "}\n"
    "\n"
    "static void nooki_vector_entero_modificar(nooki_vector_entero* v, long long indice, long long "
    "valor) {\n"
    "    if (indice < 0 || indice >= v->largo) {\n"
    "        return;\n"
    "    }\n"
    "    v->datos[indice] = valor;\n"
    "}\n"
    "\n"
    "static void nooki_vector_entero_vaciar(nooki_vector_entero* v) {\n"
    "    v->largo = 0;\n"
    "}\n"
    "\n"
    "static void nooki_vector_entero_soltar(nooki_vector_entero* v) {\n"
    "    free(v->datos);\n"
    "    v->datos = NULL;\n"
    "    v->largo = 0;\n"
    "    v->capacidad = 0;\n"
    "}\n"
    "\n"
    "/* Vector<texto>: los elementos son del vector; obtener devuelve una vista. */\n"
    "typedef struct {\n"
    "    nooki_texto* datos;\n"
    "    long long largo;\n"
    "    long long capacidad;\n"
    "} nooki_vector_texto;\n"
    "\n"
    "static nooki_vector_texto nooki_vector_texto_nuevo(void) {\n"
    "    nooki_vector_texto v;\n"
    "    v.datos = NULL;\n"
    "    v.largo = 0;\n"
    "    v.capacidad = 0;\n"
    "    return v;\n"
    "}\n"
    "\n"
    "static void nooki_vector_texto_agregar(nooki_vector_texto* v, nooki_texto valor) {\n"
    "    if (v->largo == v->capacidad) {\n"
    "        long long nueva = v->capacidad == 0 ? 4 : v->capacidad * 2;\n"
    "        nooki_texto* datos = (nooki_texto*)realloc(v->datos, sizeof(nooki_texto) * "
    "(size_t)nueva);\n"
    "        if (datos == NULL) {\n"
    "            return;\n"
    "        }\n"
    "        v->datos = datos;\n"
    "        v->capacidad = nueva;\n"
    "    }\n"
    "    v->datos[v->largo] = valor;\n"
    "    v->largo += 1;\n"
    "}\n"
    "\n"
    "static nooki_texto nooki_vector_texto_obtener(nooki_vector_texto* v, long long indice) {\n"
    "    nooki_texto vista;\n"
    "    if (indice < 0 || indice >= v->largo) {\n"
    "        vista.datos = NULL;\n"
    "        vista.largo = 0;\n"
    "        vista.propio = 0;\n"
    "        return vista;\n"
    "    }\n"
    "    vista = v->datos[indice];\n"
    "    vista.propio = 0;\n"
    "    return vista;\n"
    "}\n"
    "\n"
    "static long long nooki_vector_texto_largo(nooki_vector_texto* v) {\n"
    "    return v->largo;\n"
    "}\n"
    "\n"
    "static void nooki_vector_texto_modificar(nooki_vector_texto* v, long long indice, nooki_texto "
    "valor) {\n"
    "    if (indice < 0 || indice >= v->largo) {\n"
    "        return;\n"
    "    }\n"
    "    nooki_texto_soltar(&v->datos[indice]);\n"
    "    v->datos[indice] = valor;\n"
    "}\n"
    "\n"
    "static void nooki_vector_texto_vaciar(nooki_vector_texto* v) {\n"
    "    for (long long i = 0; i < v->largo; ++i) {\n"
    "        nooki_texto_soltar(&v->datos[i]);\n"
    "        v->datos[i] = nooki_texto_literal(\"\");\n"
    "    }\n"
    "    v->largo = 0;\n"
    "}\n"
    "\n"
    "/* Vector<flotante>: como el de enteros, con elementos de doble precision. */\n"
    "typedef struct {\n"
    "    double* datos;\n"
    "    long long largo;\n"
    "    long long capacidad;\n"
    "} nooki_vector_flotante;\n"
    "\n"
    "static nooki_vector_flotante nooki_vector_flotante_nuevo(void) {\n"
    "    nooki_vector_flotante v;\n"
    "    v.datos = NULL;\n"
    "    v.largo = 0;\n"
    "    v.capacidad = 0;\n"
    "    return v;\n"
    "}\n"
    "\n"
    "static void nooki_vector_flotante_agregar(nooki_vector_flotante* v, double valor) {\n"
    "    if (v->largo >= v->capacidad) {\n"
    "        const long long nueva = v->capacidad == 0 ? 8 : v->capacidad * 2;\n"
    "        double* datos = (double*)realloc(v->datos, (size_t)nueva * sizeof(double));\n"
    "        if (datos == NULL) {\n"
    "            return;\n"
    "        }\n"
    "        v->datos = datos;\n"
    "        v->capacidad = nueva;\n"
    "    }\n"
    "    v->datos[v->largo] = valor;\n"
    "    v->largo += 1;\n"
    "}\n"
    "\n"
    "static double nooki_vector_flotante_obtener(nooki_vector_flotante* v, long long indice) {\n"
    "    if (indice < 0 || indice >= v->largo) {\n"
    "        return 0.0;\n"
    "    }\n"
    "    return v->datos[indice];\n"
    "}\n"
    "\n"
    "static long long nooki_vector_flotante_largo(nooki_vector_flotante* v) {\n"
    "    return v->largo;\n"
    "}\n"
    "\n"
    "static void nooki_vector_flotante_modificar(nooki_vector_flotante* v, long long indice, "
    "double valor) {\n"
    "    if (indice < 0 || indice >= v->largo) {\n"
    "        return;\n"
    "    }\n"
    "    v->datos[indice] = valor;\n"
    "}\n"
    "\n"
    "static void nooki_vector_flotante_vaciar(nooki_vector_flotante* v) {\n"
    "    v->largo = 0;\n"
    "}\n"
    "\n"
    "static void nooki_vector_flotante_soltar(nooki_vector_flotante* v) {\n"
    "    free(v->datos);\n"
    "    v->datos = NULL;\n"
    "    v->largo = 0;\n"
    "    v->capacidad = 0;\n"
    "}\n"
    "\n"
    "static void nooki_vector_texto_soltar(nooki_vector_texto* v) {\n"
    "    for (long long i = 0; i < v->largo; ++i) {\n"
    "        nooki_texto_soltar(&v->datos[i]);\n"
    "    }\n"
    "    free(v->datos);\n"
    "    v->datos = NULL;\n"
    "    v->largo = 0;\n"
    "    v->capacidad = 0;\n"
    "}\n"
    "\n"
    "/* Mapa<texto, entero>: tabla hash con encadenamiento (S29). */\n"
    "typedef struct nooki_mapa_entrada {\n"
    "    nooki_texto clave;\n"
    "    long long valor;\n"
    "    struct nooki_mapa_entrada* siguiente;\n"
    "} nooki_mapa_entrada;\n"
    "\n"
    "typedef struct {\n"
    "    nooki_mapa_entrada** cubetas;\n"
    "    long long capacidad;\n"
    "    long long largo;\n"
    "} nooki_mapa_texto_entero;\n"
    "\n"
    "static unsigned long long nooki_mapa_hash(nooki_texto clave) {\n"
    "    unsigned long long hash = 1469598103934665603ULL;  /* FNV-1a */\n"
    "    for (long long i = 0; i < clave.largo; ++i) {\n"
    "        hash ^= (unsigned long long)(unsigned char)clave.datos[i];\n"
    "        hash *= 1099511628211ULL;\n"
    "    }\n"
    "    return hash;\n"
    "}\n"
    "\n"
    "static nooki_mapa_texto_entero nooki_mapa_texto_entero_nuevo(void) {\n"
    "    nooki_mapa_texto_entero m;\n"
    "    m.capacidad = 16;\n"
    "    m.largo = 0;\n"
    "    m.cubetas = (nooki_mapa_entrada**)calloc((size_t)m.capacidad, "
    "sizeof(nooki_mapa_entrada*));\n"
    "    return m;\n"
    "}\n"
    "\n"
    "static int nooki_texto_igual(nooki_texto a, nooki_texto b) {\n"
    "    return nooki_texto_comparar(a, b) == 0;\n"
    "}\n"
    "\n"
    "static void nooki_mapa_texto_entero_rehash(nooki_mapa_texto_entero* m);\n"
    "\n"
    "static void nooki_mapa_texto_entero_insertar(nooki_mapa_texto_entero* m, nooki_texto clave, "
    "long long valor) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    for (nooki_mapa_entrada* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            /* La clave es del llamante: aquí no se libera. */\n"
    "            e->valor = valor;\n"
    "            return;\n"
    "        }\n"
    "    }\n"
    "    nooki_mapa_entrada* nueva = (nooki_mapa_entrada*)malloc(sizeof(nooki_mapa_entrada));\n"
    "    if (nueva == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    /* El mapa guarda su propia copia: la clave del llamante sigue siendo suya. */\n"
    "    nueva->clave = nooki_texto_copiar(clave);\n"
    "    nueva->valor = valor;\n"
    "    nueva->siguiente = m->cubetas[indice];\n"
    "    m->cubetas[indice] = nueva;\n"
    "    m->largo += 1;\n"
    "    if (m->largo * 4 > m->capacidad * 3) {\n"
    "        nooki_mapa_texto_entero_rehash(m);\n"
    "    }\n"
    "}\n"
    "\n"
    "static long long nooki_mapa_texto_entero_obtener(nooki_mapa_texto_entero* m, nooki_texto "
    "clave) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    for (nooki_mapa_entrada* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            return e->valor;\n"
    "        }\n"
    "    }\n"
    "    return 0;\n"
    "}\n"
    "\n"
    "static int nooki_mapa_texto_entero_contiene(nooki_mapa_texto_entero* m, nooki_texto clave) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    for (nooki_mapa_entrada* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            return 1;\n"
    "        }\n"
    "    }\n"
    "    return 0;\n"
    "}\n"
    "\n"
    "static void nooki_mapa_texto_entero_rehash(nooki_mapa_texto_entero* m) {\n"
    "    const long long nueva = m->capacidad * 2;\n"
    "    nooki_mapa_entrada** cubetas = (nooki_mapa_entrada**)calloc((size_t)nueva, "
    "sizeof(nooki_mapa_entrada*));\n"
    "    if (cubetas == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    for (long long i = 0; i < m->capacidad; ++i) {\n"
    "        nooki_mapa_entrada* e = m->cubetas[i];\n"
    "        while (e != NULL) {\n"
    "            nooki_mapa_entrada* siguiente = e->siguiente;\n"
    "            const unsigned long long indice = nooki_mapa_hash(e->clave) % (unsigned long "
    "long)nueva;\n"
    "            e->siguiente = cubetas[indice];\n"
    "            cubetas[indice] = e;\n"
    "            e = siguiente;\n"
    "        }\n"
    "    }\n"
    "    free(m->cubetas);\n"
    "    m->cubetas = cubetas;\n"
    "    m->capacidad = nueva;\n"
    "}\n"
    "\n"
    "static nooki_vector_texto nooki_mapa_texto_entero_claves(nooki_mapa_texto_entero* m);\n"
    "\n"
    "static int nooki_mapa_texto_entero_quitar(nooki_mapa_texto_entero* m, nooki_texto clave) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    nooki_mapa_entrada* anterior = NULL;\n"
    "    for (nooki_mapa_entrada* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            if (anterior == NULL) {\n"
    "                m->cubetas[indice] = e->siguiente;\n"
    "            } else {\n"
    "                anterior->siguiente = e->siguiente;\n"
    "            }\n"
    "            nooki_texto_soltar(&e->clave);\n"
    "            free(e);\n"
    "            m->largo -= 1;\n"
    "            return 1;\n"
    "        }\n"
    "        anterior = e;\n"
    "    }\n"
    "    return 0;\n"
    "}\n"
    "\n"
    "static long long nooki_mapa_texto_entero_largo(nooki_mapa_texto_entero* m) {\n"
    "    return m->largo;\n"
    "}\n"
    "\n"
    "static nooki_vector_texto nooki_mapa_texto_entero_claves(nooki_mapa_texto_entero* m) {\n"
    "    nooki_vector_texto claves = nooki_vector_texto_nuevo();\n"
    "    if (m->cubetas == NULL) {\n"
    "        return claves;\n"
    "    }\n"
    "    for (long long i = 0; i < m->capacidad; ++i) {\n"
    "        for (nooki_mapa_entrada* e = m->cubetas[i]; e != NULL; e = e->siguiente) {\n"
    "            nooki_texto vista = e->clave;\n"
    "            vista.propio = 0;  /* el dueno sigue siendo el mapa */\n"
    "            nooki_vector_texto_agregar(&claves, vista);\n"
    "        }\n"
    "    }\n"
    "    return claves;\n"
    "}\n"
    "/* Mapa<texto, texto>: tabla hash cuyos valores tambien son textos. */\n"
    "typedef struct nooki_mapa_entrada_texto {\n"
    "    nooki_texto clave;\n"
    "    nooki_texto valor;\n"
    "    struct nooki_mapa_entrada_texto* siguiente;\n"
    "} nooki_mapa_entrada_texto;\n"
    "\n"
    "typedef struct {\n"
    "    nooki_mapa_entrada_texto** cubetas;\n"
    "    long long capacidad;\n"
    "    long long largo;\n"
    "} nooki_mapa_texto_texto;\n"
    "\n"
    "static void nooki_mapa_texto_texto_rehash(nooki_mapa_texto_texto* m);\n"
    "\n"
    "static nooki_mapa_texto_texto nooki_mapa_texto_texto_nuevo(void) {\n"
    "    nooki_mapa_texto_texto m;\n"
    "    m.capacidad = 16;\n"
    "    m.largo = 0;\n"
    "    m.cubetas = (nooki_mapa_entrada_texto**)calloc((size_t)m.capacidad,\n"
    "                                                   sizeof(nooki_mapa_entrada_texto*));\n"
    "    if (m.cubetas == NULL) {\n"
    "        m.capacidad = 0;\n"
    "    }\n"
    "    return m;\n"
    "}\n"
    "\n"
    "static void nooki_mapa_texto_texto_rehash(nooki_mapa_texto_texto* m) {\n"
    "    const long long vieja = m->capacidad;\n"
    "    nooki_mapa_entrada_texto** anteriores = m->cubetas;\n"
    "    m->capacidad = vieja * 2;\n"
    "    m->cubetas = (nooki_mapa_entrada_texto**)calloc((size_t)m->capacidad,\n"
    "                                                    sizeof(nooki_mapa_entrada_texto*));\n"
    "    if (m->cubetas == NULL) {\n"
    "        m->cubetas = anteriores;\n"
    "        m->capacidad = vieja;\n"
    "        return;\n"
    "    }\n"
    "    for (long long i = 0; i < vieja; ++i) {\n"
    "        nooki_mapa_entrada_texto* e = anteriores[i];\n"
    "        while (e != NULL) {\n"
    "            nooki_mapa_entrada_texto* siguiente = e->siguiente;\n"
    "            const unsigned long long indice =\n"
    "                nooki_mapa_hash(e->clave) % (unsigned long long)m->capacidad;\n"
    "            e->siguiente = m->cubetas[indice];\n"
    "            m->cubetas[indice] = e;\n"
    "            e = siguiente;\n"
    "        }\n"
    "    }\n"
    "    free(anteriores);\n"
    "}\n"
    "\n"
    "static void nooki_mapa_texto_texto_insertar(nooki_mapa_texto_texto* m, nooki_texto clave,\n"
    "                                            nooki_texto valor) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    for (nooki_mapa_entrada_texto* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            /* La clave es del llamante; el valor viejo si es del mapa. */\n"
    "            nooki_texto_soltar(&e->valor);\n"
    "            e->valor = nooki_texto_copiar(valor);\n"
    "            return;\n"
    "        }\n"
    "    }\n"
    "    nooki_mapa_entrada_texto* nueva =\n"
    "        (nooki_mapa_entrada_texto*)malloc(sizeof(nooki_mapa_entrada_texto));\n"
    "    if (nueva == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    /* El mapa guarda copias: ni la clave ni el valor son suyos. */\n"
    "    nueva->clave = nooki_texto_copiar(clave);\n"
    "    nueva->valor = nooki_texto_copiar(valor);\n"
    "    nueva->siguiente = m->cubetas[indice];\n"
    "    m->cubetas[indice] = nueva;\n"
    "    m->largo += 1;\n"
    "    if (m->largo * 4 > m->capacidad * 3) {\n"
    "        nooki_mapa_texto_texto_rehash(m);\n"
    "    }\n"
    "}\n"
    "\n"
    "static nooki_texto nooki_mapa_texto_texto_obtener(nooki_mapa_texto_texto* m,\n"
    "                                                  nooki_texto clave) {\n"
    "    nooki_texto vista;\n"
    "    vista.datos = NULL;\n"
    "    vista.largo = 0;\n"
    "    vista.propio = 0;\n"
    "    if (m->cubetas == NULL) {\n"
    "        return vista;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    for (nooki_mapa_entrada_texto* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            /* Vista: el texto sigue siendo del mapa. */\n"
    "            vista = e->valor;\n"
    "            vista.propio = 0;\n"
    "            return vista;\n"
    "        }\n"
    "    }\n"
    "    return vista;\n"
    "}\n"
    "\n"
    "static int nooki_mapa_texto_texto_contiene(nooki_mapa_texto_texto* m, nooki_texto clave) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    for (nooki_mapa_entrada_texto* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            return 1;\n"
    "        }\n"
    "    }\n"
    "    return 0;\n"
    "}\n"
    "\n"
    "static int nooki_mapa_texto_texto_quitar(nooki_mapa_texto_texto* m, nooki_texto clave) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    nooki_mapa_entrada_texto* anterior = NULL;\n"
    "    for (nooki_mapa_entrada_texto* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            if (anterior == NULL) {\n"
    "                m->cubetas[indice] = e->siguiente;\n"
    "            } else {\n"
    "                anterior->siguiente = e->siguiente;\n"
    "            }\n"
    "            nooki_texto_soltar(&e->clave);\n"
    "            nooki_texto_soltar(&e->valor);\n"
    "            free(e);\n"
    "            m->largo -= 1;\n"
    "            return 1;\n"
    "        }\n"
    "        anterior = e;\n"
    "    }\n"
    "    return 0;\n"
    "}\n"
    "\n"
    "static long long nooki_mapa_texto_texto_largo(nooki_mapa_texto_texto* m) {\n"
    "    return m->largo;\n"
    "}\n"
    "\n"
    "static void nooki_mapa_texto_texto_soltar(nooki_mapa_texto_texto* m) {\n"
    "    if (m->cubetas != NULL) {\n"
    "        for (long long i = 0; i < m->capacidad; ++i) {\n"
    "            nooki_mapa_entrada_texto* e = m->cubetas[i];\n"
    "            while (e != NULL) {\n"
    "                nooki_mapa_entrada_texto* siguiente = e->siguiente;\n"
    "                nooki_texto_soltar(&e->clave);\n"
    "                nooki_texto_soltar(&e->valor);\n"
    "                free(e);\n"
    "                e = siguiente;\n"
    "            }\n"
    "        }\n"
    "        free(m->cubetas);\n"
    "        m->cubetas = NULL;\n"
    "    }\n"
    "    m->largo = 0;\n"
    "    m->capacidad = 0;\n"
    "}\n"
    "\n"
    "static nooki_vector_texto nooki_mapa_texto_texto_claves(nooki_mapa_texto_texto* m) {\n"
    "    nooki_vector_texto claves = nooki_vector_texto_nuevo();\n"
    "    if (m->cubetas == NULL) {\n"
    "        return claves;\n"
    "    }\n"
    "    for (long long i = 0; i < m->capacidad; ++i) {\n"
    "        for (nooki_mapa_entrada_texto* e = m->cubetas[i]; e != NULL; e = e->siguiente) {\n"
    "            nooki_vector_texto_agregar(&claves, nooki_texto_copiar(e->clave));\n"
    "        }\n"
    "    }\n"
    "    return claves;\n"
    "}\n"
    "\n"
    "/* Mapa<texto, flotante>: tabla hash con encadenamiento (S29). */\n"
    "typedef struct nooki_mapa_entrada_flotante {\n"
    "    nooki_texto clave;\n"
    "    double valor;\n"
    "    struct nooki_mapa_entrada_flotante* siguiente;\n"
    "} nooki_mapa_entrada_flotante;\n"
    "\n"
    "typedef struct {\n"
    "    nooki_mapa_entrada_flotante** cubetas;\n"
    "    long long capacidad;\n"
    "    long long largo;\n"
    "} nooki_mapa_texto_flotante;\n"
    "\n"
    "static nooki_mapa_texto_flotante nooki_mapa_texto_flotante_nuevo(void) {\n"
    "    nooki_mapa_texto_flotante m;\n"
    "    m.capacidad = 16;\n"
    "    m.largo = 0;\n"
    "    m.cubetas = (nooki_mapa_entrada_flotante**)calloc((size_t)m.capacidad, "
    "sizeof(nooki_mapa_entrada_flotante*));\n"
    "    return m;\n"
    "}\n"
    "\n"
    "static void nooki_mapa_texto_flotante_rehash(nooki_mapa_texto_flotante* m);\n"
    "\n"
    "static void nooki_mapa_texto_flotante_insertar(nooki_mapa_texto_flotante* m, nooki_texto "
    "clave, double valor) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    for (nooki_mapa_entrada_flotante* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            /* La clave es del llamante: aquí no se libera. */\n"
    "            e->valor = valor;\n"
    "            return;\n"
    "        }\n"
    "    }\n"
    "    nooki_mapa_entrada_flotante* nueva = "
    "(nooki_mapa_entrada_flotante*)malloc(sizeof(nooki_mapa_entrada_flotante));\n"
    "    if (nueva == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    /* El mapa guarda su propia copia: la clave del llamante sigue siendo suya. */\n"
    "    nueva->clave = nooki_texto_copiar(clave);\n"
    "    nueva->valor = valor;\n"
    "    nueva->siguiente = m->cubetas[indice];\n"
    "    m->cubetas[indice] = nueva;\n"
    "    m->largo += 1;\n"
    "    if (m->largo * 4 > m->capacidad * 3) {\n"
    "        nooki_mapa_texto_flotante_rehash(m);\n"
    "    }\n"
    "}\n"
    "\n"
    "static double nooki_mapa_texto_flotante_obtener(nooki_mapa_texto_flotante* m, nooki_texto "
    "clave) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return 0.0;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    for (nooki_mapa_entrada_flotante* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            return e->valor;\n"
    "        }\n"
    "    }\n"
    "    return 0.0;\n"
    "}\n"
    "\n"
    "static int nooki_mapa_texto_flotante_contiene(nooki_mapa_texto_flotante* m, nooki_texto "
    "clave) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return 0.0;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    for (nooki_mapa_entrada_flotante* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            return 1;\n"
    "        }\n"
    "    }\n"
    "    return 0.0;\n"
    "}\n"
    "\n"
    "static void nooki_mapa_texto_flotante_rehash(nooki_mapa_texto_flotante* m) {\n"
    "    const long long nueva = m->capacidad * 2;\n"
    "    nooki_mapa_entrada_flotante** cubetas = "
    "(nooki_mapa_entrada_flotante**)calloc((size_t)nueva, sizeof(nooki_mapa_entrada_flotante*));\n"
    "    if (cubetas == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    for (long long i = 0; i < m->capacidad; ++i) {\n"
    "        nooki_mapa_entrada_flotante* e = m->cubetas[i];\n"
    "        while (e != NULL) {\n"
    "            nooki_mapa_entrada_flotante* siguiente = e->siguiente;\n"
    "            const unsigned long long indice = nooki_mapa_hash(e->clave) % (unsigned long "
    "long)nueva;\n"
    "            e->siguiente = cubetas[indice];\n"
    "            cubetas[indice] = e;\n"
    "            e = siguiente;\n"
    "        }\n"
    "    }\n"
    "    free(m->cubetas);\n"
    "    m->cubetas = cubetas;\n"
    "    m->capacidad = nueva;\n"
    "}\n"
    "\n"
    "static nooki_vector_texto nooki_mapa_texto_flotante_claves(nooki_mapa_texto_flotante* m);\n"
    "\n"
    "static int nooki_mapa_texto_flotante_quitar(nooki_mapa_texto_flotante* m, nooki_texto clave) "
    "{\n"
    "    if (m->cubetas == NULL) {\n"
    "        return 0.0;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long "
    "long)m->capacidad;\n"
    "    nooki_mapa_entrada_flotante* anterior = NULL;\n"
    "    for (nooki_mapa_entrada_flotante* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            if (anterior == NULL) {\n"
    "                m->cubetas[indice] = e->siguiente;\n"
    "            } else {\n"
    "                anterior->siguiente = e->siguiente;\n"
    "            }\n"
    "            nooki_texto_soltar(&e->clave);\n"
    "            free(e);\n"
    "            m->largo -= 1;\n"
    "            return 1;\n"
    "        }\n"
    "        anterior = e;\n"
    "    }\n"
    "    return 0.0;\n"
    "}\n"
    "\n"
    "static long long nooki_mapa_texto_flotante_largo(nooki_mapa_texto_flotante* m) {\n"
    "    return m->largo;\n"
    "}\n"
    "\n"
    "static void nooki_mapa_texto_flotante_soltar(nooki_mapa_texto_flotante* m) {\n"
    "    if (m->cubetas != NULL) {\n"
    "        for (long long i = 0; i < m->capacidad; ++i) {\n"
    "            nooki_mapa_entrada_flotante* e = m->cubetas[i];\n"
    "            while (e != NULL) {\n"
    "                nooki_mapa_entrada_flotante* siguiente = e->siguiente;\n"
    "                nooki_texto_soltar(&e->clave);\n"
    "                free(e);\n"
    "                e = siguiente;\n"
    "            }\n"
    "        }\n"
    "        free(m->cubetas);\n"
    "        m->cubetas = NULL;\n"
    "    }\n"
    "    m->largo = 0;\n"
    "    m->capacidad = 0;\n"
    "}\n"
    "\n"
    "static nooki_vector_texto nooki_mapa_texto_flotante_claves(nooki_mapa_texto_flotante* m) {\n"
    "    nooki_vector_texto claves = nooki_vector_texto_nuevo();\n"
    "    if (m->cubetas == NULL) {\n"
    "        return claves;\n"
    "    }\n"
    "    for (long long i = 0; i < m->capacidad; ++i) {\n"
    "        for (nooki_mapa_entrada_flotante* e = m->cubetas[i]; e != NULL; e = e->siguiente) {\n"
    "            nooki_texto vista = e->clave;\n"
    "            vista.propio = 0;  /* el dueno sigue siendo el mapa */\n"
    "            nooki_vector_texto_agregar(&claves, vista);\n"
    "        }\n"
    "    }\n"
    "    return claves;\n"
    "}\n"
    "\n"
    "static void nooki_mapa_texto_entero_soltar(nooki_mapa_texto_entero* m) {\n"
    "    if (m->cubetas != NULL) {\n"
    "        for (long long i = 0; i < m->capacidad; ++i) {\n"
    "            nooki_mapa_entrada* e = m->cubetas[i];\n"
    "            while (e != NULL) {\n"
    "                nooki_mapa_entrada* siguiente = e->siguiente;\n"
    "                nooki_texto_soltar(&e->clave);\n"
    "                free(e);\n"
    "                e = siguiente;\n"
    "            }\n"
    "        }\n"
    "        free(m->cubetas);\n"
    "        m->cubetas = NULL;\n"
    "    }\n"
    "    m->largo = 0;\n"
    "    m->capacidad = 0;\n"
    "}\n"
    "\n"
    "/* Operaciones que reservan: partir, cambiar de caja y recortar. */\n"
    "static nooki_vector_texto nooki_texto_dividir(nooki_texto t, nooki_texto separador) {\n"
    "    nooki_vector_texto partes = nooki_vector_texto_nuevo();\n"
    "    if (separador.largo == 0) {\n"
    "        nooki_vector_texto_agregar(&partes, nooki_texto_subtexto(t, 0, t.largo));\n"
    "        return partes;\n"
    "    }\n"
    "    long long inicio = 0;\n"
    "    for (;;) {\n"
    "        long long pos = -1;\n"
    "        for (long long i = inicio; i + separador.largo <= t.largo; ++i) {\n"
    "            if (memcmp(t.datos + i, separador.datos, (size_t)separador.largo) == 0) {\n"
    "                pos = i;\n"
    "                break;\n"
    "            }\n"
    "        }\n"
    "        const long long fin = pos < 0 ? t.largo : pos;\n"
    "        nooki_vector_texto_agregar(&partes, nooki_texto_subtexto(t, inicio, fin - inicio));\n"
    "        if (pos < 0) {\n"
    "            break;\n"
    "        }\n"
    "        inicio = pos + separador.largo;\n"
    "    }\n"
    "    return partes;\n"
    "}\n"
    "\n"
    "static nooki_texto nooki_texto_mayusculas(nooki_texto t) {\n"
    "    nooki_texto r = nooki_texto_subtexto(t, 0, t.largo);\n"
    "    for (long long i = 0; i < r.largo; ++i) {\n"
    "        if (r.datos[i] >= 'a' && r.datos[i] <= 'z') {\n"
    "            r.datos[i] = (char)(r.datos[i] - 'a' + 'A');\n"
    "        }\n"
    "    }\n"
    "    return r;\n"
    "}\n"
    "\n"
    "static nooki_texto nooki_texto_minusculas(nooki_texto t) {\n"
    "    nooki_texto r = nooki_texto_subtexto(t, 0, t.largo);\n"
    "    for (long long i = 0; i < r.largo; ++i) {\n"
    "        if (r.datos[i] >= 'A' && r.datos[i] <= 'Z') {\n"
    "            r.datos[i] = (char)(r.datos[i] - 'A' + 'a');\n"
    "        }\n"
    "    }\n"
    "    return r;\n"
    "}\n"
    "\n"
    "static nooki_texto nooki_texto_recortar(nooki_texto t) {\n"
    "    long long desde = 0;\n"
    "    long long hasta = t.largo;\n"
    "    while (desde < hasta && (t.datos[desde] == ' ' || t.datos[desde] == 9 ||\n"
    "                             t.datos[desde] == 10 || t.datos[desde] == 13)) {\n"
    "        desde += 1;\n"
    "    }\n"
    "    while (hasta > desde && (t.datos[hasta - 1] == ' ' || t.datos[hasta - 1] == 9 ||\n"
    "                             t.datos[hasta - 1] == 10 || t.datos[hasta - 1] == 13)) {\n"
    "        hasta -= 1;\n"
    "    }\n"
    "    return nooki_texto_subtexto(t, desde, hasta - desde);\n"
    "}\n"
    "\n"
    "/* Caja<T>: valor en el montículo con un único dueño (S20). */\n"
    "typedef struct {\n"
    "    long long* datos;\n"
    "} nooki_caja;\n"
    "\n"
    "static nooki_caja nooki_caja_nuevo(long long valor) {\n"
    "    nooki_caja caja;\n"
    "    caja.datos = (long long*)malloc(sizeof(long long));\n"
    "    if (caja.datos != NULL) {\n"
    "        *caja.datos = valor;\n"
    "    }\n"
    "    return caja;\n"
    "}\n"
    "\n"
    "/* Leer y escribir el valor de la caja: los dos métodos de Caja. */\n"
    "static long long nooki_caja_sacar(nooki_caja* caja) {\n"
    "    return caja->datos != NULL ? *caja->datos : 0;\n"
    "}\n"
    "\n"
    "static void nooki_caja_poner(nooki_caja* caja, long long valor) {\n"
    "    if (caja->datos != NULL) {\n"
    "        *caja->datos = valor;\n"
    "    }\n"
    "}\n"
    "\n"
    "static void nooki_caja_soltar(nooki_caja* caja) {\n"
    "    if (caja->datos != NULL) {\n"
    "        free(caja->datos);\n"
    "        caja->datos = NULL;\n"
    "    }\n"
    "}\n"
    "\n"
    "/* Aritmetica comprobada (ADR-03): un desbordamiento no se ignora. */\n"
    "static void nooki_panico(const char* mensaje) {\n"
    "    fflush(stdout);\n"
    "    fprintf(stderr, \"error en ejecucion: %s\\n\", mensaje);\n"
    "    exit(70);\n"
    "}\n"
    "\n"
    "static long long nooki_ent_sumar(long long a, long long b) {\n"
    "#if defined(__GNUC__) || defined(__clang__)\n"
    "    long long r;\n"
    "    if (__builtin_add_overflow(a, b, &r)) {\n"
    "        nooki_panico(\"desbordamiento al sumar\");\n"
    "    }\n"
    "    return r;\n"
    "#else\n"
    "    if ((b > 0 && a > LLONG_MAX - b) || (b < 0 && a < LLONG_MIN - b)) {\n"
    "        nooki_panico(\"desbordamiento al sumar\");\n"
    "    }\n"
    "    return a + b;\n"
    "#endif\n"
    "}\n"
    "\n"
    "static long long nooki_ent_restar(long long a, long long b) {\n"
    "#if defined(__GNUC__) || defined(__clang__)\n"
    "    long long r;\n"
    "    if (__builtin_sub_overflow(a, b, &r)) {\n"
    "        nooki_panico(\"desbordamiento al restar\");\n"
    "    }\n"
    "    return r;\n"
    "#else\n"
    "    if ((b < 0 && a > LLONG_MAX + b) || (b > 0 && a < LLONG_MIN + b)) {\n"
    "        nooki_panico(\"desbordamiento al restar\");\n"
    "    }\n"
    "    return a - b;\n"
    "#endif\n"
    "}\n"
    "\n"
    "static long long nooki_ent_multiplicar(long long a, long long b) {\n"
    "#if defined(__GNUC__) || defined(__clang__)\n"
    "    long long r;\n"
    "    if (__builtin_mul_overflow(a, b, &r)) {\n"
    "        nooki_panico(\"desbordamiento al multiplicar\");\n"
    "    }\n"
    "    return r;\n"
    "#else\n"
    "    if (a == 0 || b == 0) {\n"
    "        return 0;\n"
    "    }\n"
    "    if (a == -1 && b == LLONG_MIN) nooki_panico(\"desbordamiento al multiplicar\");\n"
    "    if (b == -1 && a == LLONG_MIN) nooki_panico(\"desbordamiento al multiplicar\");\n"
    "    if (a > 0) {\n"
    "        if (b > 0 ? a > LLONG_MAX / b : b < LLONG_MIN / a) {\n"
    "            nooki_panico(\"desbordamiento al multiplicar\");\n"
    "        }\n"
    "    } else {\n"
    "        if (b > 0 ? a < LLONG_MIN / b : a < LLONG_MAX / b) {\n"
    "            nooki_panico(\"desbordamiento al multiplicar\");\n"
    "        }\n"
    "    }\n"
    "    return a * b;\n"
    "#endif\n"
    "}\n"
    "\n"
    "static long long nooki_ent_dividir(long long a, long long b) {\n"
    "    if (b == 0) {\n"
    "        nooki_panico(\"division por cero\");\n"
    "    }\n"
    "    if (a == LLONG_MIN && b == -1) {\n"
    "        nooki_panico(\"desbordamiento al dividir\");\n"
    "    }\n"
    "    return a / b;\n"
    "}\n"
    "\n"
    "static long long nooki_ent_modulo(long long a, long long b) {\n"
    "    if (b == 0) {\n"
    "        nooki_panico(\"modulo por cero\");\n"
    "    }\n"
    "    if (a == LLONG_MIN && b == -1) {\n"
    "        nooki_panico(\"desbordamiento al calcular el modulo\");\n"
    "    }\n"
    "    return a % b;\n"
    "}\n"
    "\n"
    "static long long nooki_ent_negativo(long long a) {\n"
    "    if (a == LLONG_MIN) {\n"
    "        nooki_panico(\"desbordamiento al negar\");\n"
    "    }\n"
    "    return -a;\n"
    "}\n"
    "\n"
    "/* Archivos: lectura y escritura completas (FFI con la libc). */\n"
    "static nooki_texto nooki_leer_archivo(const char* ruta) {\n"
    "    FILE* archivo = fopen(ruta, \"rb\");\n"
    "    if (archivo == NULL) {\n"
    "        return nooki_texto_literal(\"\");\n"
    "    }\n"
    "    nooki_texto t;\n"
    "    if (fseek(archivo, 0, SEEK_END) != 0) {\n"
    "        fclose(archivo);\n"
    "        return nooki_texto_literal(\"\");\n"
    "    }\n"
    "    long tamano = ftell(archivo);\n"
    "    if (tamano < 0 || fseek(archivo, 0, SEEK_SET) != 0) {\n"
    "        fclose(archivo);\n"
    "        return nooki_texto_literal(\"\");\n"
    "    }\n"
    "    t.largo = (long long)tamano;\n"
    "    t.propio = 1;\n"
    "    t.datos = (char*)malloc((size_t)tamano + 1);\n"
    "    if (t.datos == NULL) {\n"
    "        fclose(archivo);\n"
    "        t.largo = 0;\n"
    "        t.propio = 0;\n"
    "        return t;\n"
    "    }\n"
    "    size_t leido = fread(t.datos, 1, (size_t)tamano, archivo);\n"
    "    t.datos[leido] = 0;\n"
    "    t.largo = (long long)leido;\n"
    "    fclose(archivo);\n"
    "    return t;\n"
    "}\n"
    "\n"
    "/* ------------------------------------------------------------------ */\n"
    "/* Datos: DuckDB cargado en tiempo de ejecución.                      */\n"
    "/*                                                                    */\n"
    "/* El programa generado no se enlaza contra DuckDB: abre la           */\n"
    "/* biblioteca la primera vez que se usa (duckdb.dll o libduckdb.so),   */\n"
    "/* busca las funciones que necesita y, si no está, lo dice. Así un     */\n"
    "/* programa que no analiza datos no arrastra 60 MB.                    */\n"
    "/*                                                                    */\n"
    "/* La forma de `nooki_dd_resultado` es la de la API de C de DuckDB     */\n"
    "/* 2.x (campos `deprecated_*` e `internal_data`); si esa estructura    */\n"
    "/* cambiara, hay que actualizar este bloque.                           */\n"
    "/* ------------------------------------------------------------------ */\n"
    "\n"
    "#if defined(_WIN32)\n"
    "__declspec(dllimport) void* __stdcall LoadLibraryA(const char* ruta);\n"
    "__declspec(dllimport) void* __stdcall GetProcAddress(void* modulo, const char* nombre);\n"
    "typedef void* nooki_dd_modulo;\n"
    "#define NOOKI_DD_NOMBRE \"duckdb.dll\"\n"
    "#else\n"
    "#include <dlfcn.h>\n"
    "typedef void* nooki_dd_modulo;\n"
    "#define NOOKI_DD_NOMBRE \"libduckdb.so\"\n"
    "#endif\n"
    "\n"
    "/* El `duckdb_result` real de la 2.0 ocupa más de lo que declara su cabecera:\n"
    " * con 48 bytes el programa corrompía el montón. La API solo usa el puntero, así\n"
    " * que se reserva un colchón holgado y alineado en vez de replicar la estructura.\n"
    " */\n"
    "typedef union {\n"
    "    unsigned long long reserva[512];\n"
    "    long double alineacion;\n"
    "} nooki_dd_resultado;\n"
    "\n"
    "typedef struct {\n"
    "    int intentado;\n"
    "    int disponible;\n"
    "    nooki_dd_modulo modulo;\n"
    "    void* base;\n"
    "    void* conexion;\n"
    "    char error[512];\n"
    "    int (*abrir)(const char*, void**);\n"
    "    int (*conectar)(void*, void**);\n"
    "    int (*consulta)(void*, const char*, nooki_dd_resultado*);\n"
    "    unsigned long long (*cuenta_columnas)(nooki_dd_resultado*);\n"
    "    unsigned long long (*cuenta_filas)(nooki_dd_resultado*);\n"
    "    char* (*nombre_columna)(nooki_dd_resultado*, unsigned long long);\n"
    "    char* (*valor)(nooki_dd_resultado*, unsigned long long, unsigned long long);\n"
    "    void (*liberar)(void*);\n"
    "    void (*destruir)(nooki_dd_resultado*);\n"
    "    const char* (*version)(void);\n"
    "} nooki_dd_estado;\n"
    "\n"
    "static nooki_dd_estado NOOKI_DD;\n"
    "\n"
    "/* El mensaje de error es el quinto campo de la estructura de DuckDB. */\n"
    "static char* nooki_dd_mensaje(nooki_dd_resultado* resultado) {\n"
    "    void** campos = (void**)resultado;\n"
    "    return (char*)campos[4];\n"
    "}\n"
    "\n"
    "static void nooki_dd_anotar(const char* texto) {\n"
    "    size_t i = 0;\n"
    "    if (texto == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    while (texto[i] != '\\0' && i + 1 < sizeof NOOKI_DD.error) {\n"
    "        NOOKI_DD.error[i] = texto[i];\n"
    "        ++i;\n"
    "    }\n"
    "    NOOKI_DD.error[i] = '\\0';\n"
    "}\n"
    "\n"
    "static int nooki_dd_simbolo(void** destino, const char* nombre) {\n"
    "#if defined(_WIN32)\n"
    "    *destino = GetProcAddress((void*)NOOKI_DD.modulo, nombre);\n"
    "#else\n"
    "    *destino = dlsym(NOOKI_DD.modulo, nombre);\n"
    "#endif\n"
    "    return *destino != NULL;\n"
    "}\n"
    "\n"
    "static int nooki_dd_cargar(void) {\n"
    "    if (NOOKI_DD.intentado) {\n"
    "        return NOOKI_DD.disponible;\n"
    "    }\n"
    "    NOOKI_DD.intentado = 1;\n"
    "\n"
    "    /* NOOKI_DUCKDB permite decir dónde está el archivo. */\n"
    "    const char* ruta = getenv(\"NOOKI_DUCKDB\");\n"
    "    if (ruta == NULL || ruta[0] == '\\0') {\n"
    "        ruta = NOOKI_DD_NOMBRE;\n"
    "    }\n"
    "#if defined(_WIN32)\n"
    "    NOOKI_DD.modulo = (nooki_dd_modulo)LoadLibraryA(ruta);\n"
    "    if (NOOKI_DD.modulo == NULL && strcmp(ruta, NOOKI_DD_NOMBRE) == 0) {\n"
    "        NOOKI_DD.modulo = (nooki_dd_modulo)LoadLibraryA(\"datos\\\\\" NOOKI_DD_NOMBRE);\n"
    "    }\n"
    "#else\n"
    "    NOOKI_DD.modulo = dlopen(ruta, RTLD_NOW | RTLD_LOCAL);\n"
    "    if (NOOKI_DD.modulo == NULL && strcmp(ruta, NOOKI_DD_NOMBRE) == 0) {\n"
    "        NOOKI_DD.modulo = dlopen(\"datos/\" NOOKI_DD_NOMBRE, RTLD_NOW | RTLD_LOCAL);\n"
    "    }\n"
    "#endif\n"
    "    if (NOOKI_DD.modulo == NULL) {\n"
    "        nooki_dd_anotar(ruta);\n"
    "        return 0;\n"
    "    }\n"
    "\n"
    "    if (!nooki_dd_simbolo((void**)&NOOKI_DD.abrir, \"duckdb_open\") ||\n"
    "        !nooki_dd_simbolo((void**)&NOOKI_DD.conectar, \"duckdb_connect\") ||\n"
    "        !nooki_dd_simbolo((void**)&NOOKI_DD.consulta, \"duckdb_query\") ||\n"
    "        !nooki_dd_simbolo((void**)&NOOKI_DD.cuenta_columnas, \"duckdb_column_count\") ||\n"
    "        !nooki_dd_simbolo((void**)&NOOKI_DD.cuenta_filas, \"duckdb_row_count\") ||\n"
    "        !nooki_dd_simbolo((void**)&NOOKI_DD.nombre_columna, \"duckdb_column_name\") ||\n"
    "        !nooki_dd_simbolo((void**)&NOOKI_DD.valor, \"duckdb_value_varchar\") ||\n"
    "        !nooki_dd_simbolo((void**)&NOOKI_DD.liberar, \"duckdb_free\") ||\n"
    "        !nooki_dd_simbolo((void**)&NOOKI_DD.destruir, \"duckdb_destroy_result\") ||\n"
    "        !nooki_dd_simbolo((void**)&NOOKI_DD.version, \"duckdb_library_version\")) {\n"
    "        nooki_dd_anotar(\"la biblioteca no tiene la API que se esperaba\");\n"
    "        return 0;\n"
    "    }\n"
    "\n"
    "    if (NOOKI_DD.abrir(NULL, &NOOKI_DD.base) != 0 || NOOKI_DD.base == NULL) {\n"
    "        nooki_dd_anotar(\"no se pudo abrir la base de datos\");\n"
    "        return 0;\n"
    "    }\n"
    "    if (NOOKI_DD.conectar(NOOKI_DD.base, &NOOKI_DD.conexion) != 0 ||\n"
    "        NOOKI_DD.conexion == NULL) {\n"
    "        nooki_dd_anotar(\"no se pudo abrir la conexión\");\n"
    "        return 0;\n"
    "    }\n"
    "\n"
    "    NOOKI_DD.disponible = 1;\n"
    "    return 1;\n"
    "}\n"
    "\n"
    "/* Un acumulador de texto, para armar el CSV sin saber el tamaño de antemano. */\n"
    "typedef struct {\n"
    "    char* datos;\n"
    "    size_t largo;\n"
    "    size_t capacidad;\n"
    "} nooki_dd_texto;\n"
    "\n"
    "static int nooki_dd_agregar(nooki_dd_texto* t, const char* texto, size_t largo) {\n"
    "    if (t->largo + largo + 1 > t->capacidad) {\n"
    "        size_t nueva = t->capacidad == 0 ? 4096 : t->capacidad;\n"
    "        while (nueva < t->largo + largo + 1) {\n"
    "            nueva *= 2;\n"
    "        }\n"
    "        char* crecido = (char*)realloc(t->datos, nueva);\n"
    "        if (crecido == NULL) {\n"
    "            return 0;\n"
    "        }\n"
    "        t->datos = crecido;\n"
    "        t->capacidad = nueva;\n"
    "    }\n"
    "    memcpy(t->datos + t->largo, texto, largo);\n"
    "    t->largo += largo;\n"
    "    t->datos[t->largo] = '\\0';\n"
    "    return 1;\n"
    "}\n"
    "\n"
    "static int nooki_dd_agregar_campo(nooki_dd_texto* t, const char* campo) {\n"
    "    const char* texto = campo == NULL ? \"\" : campo;\n"
    "    int comillas = 0;\n"
    "    for (const char* c = texto; *c != '\\0'; ++c) {\n"
    "        if (*c == ',' || *c == '\"' || *c == '\\n' || *c == '\\r') {\n"
    "            comillas = 1;\n"
    "            break;\n"
    "        }\n"
    "    }\n"
    "    if (!comillas) {\n"
    "        return nooki_dd_agregar(t, texto, strlen(texto));\n"
    "    }\n"
    "    if (!nooki_dd_agregar(t, \"\\\"\", 1)) {\n"
    "        return 0;\n"
    "    }\n"
    "    for (const char* c = texto; *c != '\\0'; ++c) {\n"
    "        if (*c == '\"') {\n"
    "            if (!nooki_dd_agregar(t, \"\\\"\\\"\", 2)) {\n"
    "                return 0;\n"
    "            }\n"
    "        } else if (!nooki_dd_agregar(t, c, 1)) {\n"
    "            return 0;\n"
    "        }\n"
    "    }\n"
    "    return nooki_dd_agregar(t, \"\\\"\", 1);\n"
    "}\n"
    "\n"
    "static nooki_texto nooki_dd_texto_de(const char* texto) {\n"
    "    return nooki_texto_desde_c(texto == NULL ? \"\" : texto);\n"
    "}\n"
    "\n"
    "/* Ejecuta la consulta y devuelve las filas en CSV, con cabecera. */\n"
    "nooki_texto nooki_dd_consultar(const char* consulta) {\n"
    "    if (consulta == NULL || consulta[0] == '\\0') {\n"
    "        return nooki_dd_texto_de(\"\");\n"
    "    }\n"
    "    if (!nooki_dd_cargar()) {\n"
    "        return nooki_dd_texto_de(\"\");\n"
    "    }\n"
    "\n"
    "    nooki_dd_resultado resultado;\n"
    "    memset(&resultado, 0, sizeof resultado);\n"
    "    if (NOOKI_DD.consulta(NOOKI_DD.conexion, consulta, &resultado) != 0) {\n"
    "        const char* motivo = nooki_dd_mensaje(&resultado) != NULL ? "
    "nooki_dd_mensaje(&resultado)\n"
    "                                                             : \"la consulta falló\";\n"
    "        nooki_dd_anotar(motivo);\n"
    "        if (nooki_dd_mensaje(&resultado) != NULL) {\n"
    "            NOOKI_DD.liberar(nooki_dd_mensaje(&resultado));\n"
    "        }\n"
    "        NOOKI_DD.destruir(&resultado);\n"
    "        return nooki_dd_texto_de(\"\");\n"
    "    }\n"
    "\n"
    "    const unsigned long long filas = NOOKI_DD.cuenta_filas(&resultado);\n"
    "    const unsigned long long columnas = NOOKI_DD.cuenta_columnas(&resultado);\n"
    "    nooki_dd_texto salida;\n"
    "    salida.datos = NULL;\n"
    "    salida.largo = 0;\n"
    "    salida.capacidad = 0;\n"
    "\n"
    "    for (unsigned long long c = 0; c < columnas; ++c) {\n"
    "        char* nombre = NOOKI_DD.nombre_columna(&resultado, c);\n"
    "        if (c > 0) {\n"
    "            nooki_dd_agregar(&salida, \",\", 1);\n"
    "        }\n"
    "        nooki_dd_agregar_campo(&salida, nombre);\n"
    "        /* El nombre de la columna es de la biblioteca: liberarlo corrompe el\n"
    "         * montón y el programa revienta al destruir el resultado. */\n"
    "    }\n"
    "    nooki_dd_agregar(&salida, \"\\n\", 1);\n"
    "\n"
    "    for (unsigned long long f = 0; f < filas; ++f) {\n"
    "        for (unsigned long long c = 0; c < columnas; ++c) {\n"
    "            char* valor = NOOKI_DD.valor(&resultado, c, f);\n"
    "            if (c > 0) {\n"
    "                nooki_dd_agregar(&salida, \",\", 1);\n"
    "            }\n"
    "            nooki_dd_agregar_campo(&salida, valor);\n"
    "            if (valor != NULL) {\n"
    "                NOOKI_DD.liberar(valor);\n"
    "            }\n"
    "        }\n"
    "        nooki_dd_agregar(&salida, \"\\n\", 1);\n"
    "    }\n"
    "\n"
    "    NOOKI_DD.destruir(&resultado);\n"
    "\n"
    "    nooki_texto devuelto = nooki_dd_texto_de(salida.datos == NULL ? \"\" : salida.datos);\n"
    "    free(salida.datos);\n"
    "    return devuelto;\n"
    "}\n"
    "\n"
    "/* Para órdenes que no devuelven filas: 0 si fue bien. */\n"
    "long long nooki_dd_ejecutar(const char* consulta) {\n"
    "    if (consulta == NULL || consulta[0] == '\\0') {\n"
    "        return 1;\n"
    "    }\n"
    "    if (!nooki_dd_cargar()) {\n"
    "        return 1;\n"
    "    }\n"
    "    nooki_dd_resultado resultado;\n"
    "    memset(&resultado, 0, sizeof resultado);\n"
    "    const int estado = NOOKI_DD.consulta(NOOKI_DD.conexion, consulta, &resultado);\n"
    "    if (estado != 0) {\n"
    "        const char* motivo = nooki_dd_mensaje(&resultado) != NULL ? "
    "nooki_dd_mensaje(&resultado)\n"
    "                                                             : \"la orden falló\";\n"
    "        nooki_dd_anotar(motivo);\n"
    "        if (nooki_dd_mensaje(&resultado) != NULL) {\n"
    "            NOOKI_DD.liberar(nooki_dd_mensaje(&resultado));\n"
    "        }\n"
    "        NOOKI_DD.destruir(&resultado);\n"
    "        return 1;\n"
    "    }\n"
    "    NOOKI_DD.destruir(&resultado);\n"
    "    return 0;\n"
    "}\n"
    "\n"
    "nooki_texto nooki_dd_version(void) {\n"
    "    if (!nooki_dd_cargar()) {\n"
    "        return nooki_dd_texto_de(\"\");\n"
    "    }\n"
    "    return nooki_dd_texto_de(NOOKI_DD.version());\n"
    "}\n"
    "\n"
    "long long nooki_dd_disponible(void) {\n"
    "    return nooki_dd_cargar() ? 1 : 0;\n"
    "}\n"
    "\n"
    "nooki_texto nooki_dd_error(void) {\n"
    "    if (NOOKI_DD.error[0] == '\\0') {\n"
    "        return nooki_dd_texto_de(\"\");\n"
    "    }\n"
    "    return nooki_dd_texto_de(NOOKI_DD.error);\n"
    "}\n"
    "\n"
    "/* ------------------------------------------------------------------ */\n"
    "/* Tablas: el resultado de una consulta, en memoria.                   */\n"
    "/*                                                                    */\n"
    "/* Un manejador guarda los nombres de las columnas y los valores ya    */\n"
    "/* convertidos a texto. La tabla se libera con `nooki_dd_tabla_soltar`. */\n"
    "/* ------------------------------------------------------------------ */\n"
    "\n"
    "typedef struct {\n"
    "    long long columnas;\n"
    "    long long filas;\n"
    "    char** nombres;\n"
    "    char** valores; /* filas * columnas, por filas */\n"
    "} nooki_dd_tabla_datos;\n"
    "\n"
    "static void nooki_dd_tabla_soltar(nooki_dd_tabla_datos* tabla) {\n"
    "    if (tabla == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    if (tabla->nombres != NULL) {\n"
    "        for (long long c = 0; c < tabla->columnas; ++c) {\n"
    "            free(tabla->nombres[c]);\n"
    "        }\n"
    "        free(tabla->nombres);\n"
    "    }\n"
    "    if (tabla->valores != NULL) {\n"
    "        for (long long i = 0; i < tabla->columnas * tabla->filas; ++i) {\n"
    "            free(tabla->valores[i]);\n"
    "        }\n"
    "        free(tabla->valores);\n"
    "    }\n"
    "    free(tabla);\n"
    "}\n"
    "\n"
    "static char* nooki_dd_copiar_cadena(const char* texto) {\n"
    "    const char* origen = texto == NULL ? \"\" : texto;\n"
    "    const size_t largo = strlen(origen);\n"
    "    char* copia = (char*)malloc(largo + 1);\n"
    "    if (copia == NULL) {\n"
    "        return NULL;\n"
    "    }\n"
    "    memcpy(copia, origen, largo + 1);\n"
    "    return copia;\n"
    "}\n"
    "\n"
    "/* Ejecuta la consulta y devuelve un manejador a la tabla, o 0 si falló. */\n"
    "long long nooki_dd_tabla(const char* consulta) {\n"
    "    if (consulta == NULL || consulta[0] == '\\0') {\n"
    "        return 0;\n"
    "    }\n"
    "    if (!nooki_dd_cargar()) {\n"
    "        return 0;\n"
    "    }\n"
    "\n"
    "    nooki_dd_resultado resultado;\n"
    "    memset(&resultado, 0, sizeof resultado);\n"
    "    if (NOOKI_DD.consulta(NOOKI_DD.conexion, consulta, &resultado) != 0) {\n"
    "        nooki_dd_anotar(nooki_dd_mensaje(&resultado) != NULL ? nooki_dd_mensaje(&resultado)\n"
    "                                                           : \"la consulta falló\");\n"
    "        if (nooki_dd_mensaje(&resultado) != NULL) {\n"
    "            NOOKI_DD.liberar(nooki_dd_mensaje(&resultado));\n"
    "        }\n"
    "        NOOKI_DD.destruir(&resultado);\n"
    "        return 0;\n"
    "    }\n"
    "\n"
    "    nooki_dd_tabla_datos* tabla = (nooki_dd_tabla_datos*)calloc(1, "
    "sizeof(nooki_dd_tabla_datos));\n"
    "    if (tabla == NULL) {\n"
    "        NOOKI_DD.destruir(&resultado);\n"
    "        return 0;\n"
    "    }\n"
    "    tabla->columnas = (long long)NOOKI_DD.cuenta_columnas(&resultado);\n"
    "    tabla->filas = (long long)NOOKI_DD.cuenta_filas(&resultado);\n"
    "\n"
    "    if (tabla->columnas > 0) {\n"
    "        tabla->nombres = (char**)calloc((size_t)tabla->columnas, sizeof(char*));\n"
    "    }\n"
    "    if (tabla->columnas > 0 && tabla->filas > 0) {\n"
    "        tabla->valores = (char**)calloc((size_t)(tabla->columnas * tabla->filas), "
    "sizeof(char*));\n"
    "    }\n"
    "    if ((tabla->columnas > 0 && tabla->nombres == NULL) ||\n"
    "        (tabla->columnas > 0 && tabla->filas > 0 && tabla->valores == NULL)) {\n"
    "        nooki_dd_tabla_soltar(tabla);\n"
    "        NOOKI_DD.destruir(&resultado);\n"
    "        nooki_dd_anotar(\"sin memoria para la tabla\");\n"
    "        return 0;\n"
    "    }\n"
    "\n"
    "    for (long long c = 0; c < tabla->columnas; ++c) {\n"
    "        char* nombre = NOOKI_DD.nombre_columna(&resultado, (unsigned long long)c);\n"
    "        tabla->nombres[c] = nooki_dd_copiar_cadena(nombre);\n"
    "    }\n"
    "    for (long long f = 0; f < tabla->filas; ++f) {\n"
    "        for (long long c = 0; c < tabla->columnas; ++c) {\n"
    "            char* valor = NOOKI_DD.valor(&resultado, (unsigned long long)c, (unsigned long "
    "long)f);\n"
    "            tabla->valores[f * tabla->columnas + c] = nooki_dd_copiar_cadena(valor);\n"
    "            if (valor != NULL) {\n"
    "                NOOKI_DD.liberar(valor);\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "\n"
    "    NOOKI_DD.destruir(&resultado);\n"
    "    return (long long)(intptr_t)tabla;\n"
    "}\n"
    "\n"
    "static nooki_dd_tabla_datos* nooki_dd_tabla_de(long long manejador) {\n"
    "    return (nooki_dd_tabla_datos*)(intptr_t)manejador;\n"
    "}\n"
    "\n"
    "long long nooki_dd_tabla_filas(long long manejador) {\n"
    "    nooki_dd_tabla_datos* tabla = nooki_dd_tabla_de(manejador);\n"
    "    return tabla == NULL ? 0 : tabla->filas;\n"
    "}\n"
    "\n"
    "long long nooki_dd_tabla_columnas(long long manejador) {\n"
    "    nooki_dd_tabla_datos* tabla = nooki_dd_tabla_de(manejador);\n"
    "    return tabla == NULL ? 0 : tabla->columnas;\n"
    "}\n"
    "\n"
    "nooki_texto nooki_dd_tabla_nombre(long long manejador, long long columna) {\n"
    "    nooki_dd_tabla_datos* tabla = nooki_dd_tabla_de(manejador);\n"
    "    if (tabla == NULL || columna < 0 || columna >= tabla->columnas) {\n"
    "        return nooki_dd_texto_de(\"\");\n"
    "    }\n"
    "    return nooki_dd_texto_de(tabla->nombres[columna]);\n"
    "}\n"
    "\n"
    "nooki_texto nooki_dd_tabla_valor(long long manejador, long long fila, long long columna) {\n"
    "    nooki_dd_tabla_datos* tabla = nooki_dd_tabla_de(manejador);\n"
    "    if (tabla == NULL || fila < 0 || fila >= tabla->filas || columna < 0 ||\n"
    "        columna >= tabla->columnas) {\n"
    "        return nooki_dd_texto_de(\"\");\n"
    "    }\n"
    "    return nooki_dd_texto_de(tabla->valores[fila * tabla->columnas + columna]);\n"
    "}\n"
    "\n"
    "void nooki_dd_tabla_liberar(long long manejador) {\n"
    "    nooki_dd_tabla_soltar(nooki_dd_tabla_de(manejador));\n"
    "}\n"
    "/* Un valor de la tabla como número, sin pasar por texto. */\n"
    "double nooki_dd_tabla_numero(long long manejador, long long fila, long long columna) {\n"
    "    nooki_dd_tabla_datos* tabla = nooki_dd_tabla_de(manejador);\n"
    "    if (tabla == NULL || fila < 0 || fila >= tabla->filas || columna < 0 ||\n"
    "        columna >= tabla->columnas) {\n"
    "        return 0.0;\n"
    "    }\n"
    "    return strtod(tabla->valores[fila * tabla->columnas + columna], NULL);\n"
    "}\n"
    "\n"
    "long long nooki_dd_tabla_entero(long long manejador, long long fila, long long columna) {\n"
    "    nooki_dd_tabla_datos* tabla = nooki_dd_tabla_de(manejador);\n"
    "    if (tabla == NULL || fila < 0 || fila >= tabla->filas || columna < 0 ||\n"
    "        columna >= tabla->columnas) {\n"
    "        return 0;\n"
    "    }\n"
    "    return strtoll(tabla->valores[fila * tabla->columnas + columna], NULL, 10);\n"
    "}\n"
    "/* Añade una columna con los valores dados. Devuelve 1 si se pudo.\n"
    " * Los valores tienen que ser tantos como filas: si no, no se añade nada. */\n"
    "long long nooki_dd_tabla_agregar_columna(long long manejador, const char* nombre,\n"
    "                                        nooki_vector_texto valores) {\n"
    "    nooki_dd_tabla_datos* tabla = nooki_dd_tabla_de(manejador);\n"
    "    if (tabla == NULL || nombre == NULL || valores.largo != tabla->filas) {\n"
    "        return 0;\n"
    "    }\n"
    "    if (tabla->columnas <= 0) {\n"
    "        return 0;\n"
    "    }\n"
    "\n"
    "    char** nombres = (char**)realloc(tabla->nombres, (size_t)(tabla->columnas + 1) * "
    "sizeof(char*));\n"
    "    if (nombres == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    tabla->nombres = nombres;\n"
    "\n"
    "    const long long celdas = (tabla->columnas + 1) * tabla->filas;\n"
    "    char** nuevos = (char**)calloc((size_t)celdas, sizeof(char*));\n"
    "    if (nuevos == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    /* Se copian las columnas que había, haciendo hueco a la nueva al final. */\n"
    "    for (long long f = 0; f < tabla->filas; ++f) {\n"
    "        for (long long c = 0; c < tabla->columnas; ++c) {\n"
    "            nuevos[f * (tabla->columnas + 1) + c] = tabla->valores[f * tabla->columnas + c];\n"
    "        }\n"
    "    }\n"
    "    free(tabla->valores);\n"
    "    tabla->valores = nuevos;\n"
    "\n"
    "    for (long long f = 0; f < tabla->filas; ++f) {\n"
    "        const nooki_texto* valor = &valores.datos[f];\n"
    "        tabla->valores[f * (tabla->columnas + 1) + tabla->columnas] =\n"
    "            nooki_dd_copiar_cadena(valor->datos == NULL ? \"\" : valor->datos);\n"
    "    }\n"
    "\n"
    "    tabla->nombres[tabla->columnas] = nooki_dd_copiar_cadena(nombre);\n"
    "    tabla->columnas += 1;\n"
    "    return 1;\n"
    "}\n"
    "/* El índice de una columna por su nombre, o -1 si no está. */\n"
    "long long nooki_dd_tabla_indice(long long manejador, const char* nombre) {\n"
    "    nooki_dd_tabla_datos* tabla = nooki_dd_tabla_de(manejador);\n"
    "    if (tabla == NULL || nombre == NULL) {\n"
    "        return -1;\n"
    "    }\n"
    "    for (long long c = 0; c < tabla->columnas; ++c) {\n"
    "        const char* suyo = tabla->nombres[c];\n"
    "        if (suyo != NULL && strcmp(suyo, nombre) == 0) {\n"
    "            return c;\n"
    "        }\n"
    "    }\n"
    "    return -1;\n"
    "}\n"

    "/* ------------------------------------------------------------------ */\n"
    "/* Argumentos de la línea de órdenes.                                  */\n"
    "/* ------------------------------------------------------------------ */\n"
    "\n"
    "static int NOOKI_ARGC = 0;\n"
    "static char** NOOKI_ARGV = NULL;\n"
    "\n"
    "long long nooki_cuantos_argumentos(void) {\n"
    "    return NOOKI_ARGC > 0 ? (long long)(NOOKI_ARGC - 1) : 0;\n"
    "}\n"
    "\n"
    "nooki_texto nooki_argumento(long long indice) {\n"
    "    nooki_texto vacio = nooki_texto_desde_c(\"\");\n"
    "    if (indice < 0 || NOOKI_ARGV == NULL || indice + 1 >= NOOKI_ARGC) {\n"
    "        return vacio;\n"
    "    }\n"
    "    return nooki_texto_desde_c(NOOKI_ARGV[indice + 1]);\n"
    "}\n"
    "\n"
    "/* Todos los argumentos, sin el nombre del programa. */\n"
    "nooki_vector_texto nooki_argumentos(void) {\n"
    "    nooki_vector_texto lista = nooki_vector_texto_nuevo();\n"
    "    for (long long i = 1; i < (long long)NOOKI_ARGC; ++i) {\n"
    "        nooki_vector_texto_agregar(&lista, nooki_texto_desde_c(NOOKI_ARGV[i]));\n"
    "    }\n"
    "    return lista;\n"
    "}\n"
    "\n"
    "/* ------------------------------------------------------------------ */\n"
    "/* Procesos externos: para hablar con un programa de fuera (Python,   */\n"
    "/* una herramienta del sistema…). La orden va como una línea de       */\n"
    "/* comandos; las comillas son cosa de quien la escribe.               */\n"
    "/* ------------------------------------------------------------------ */\n"
    "\n"
    "#if defined(_WIN32)\n"
    "#define NOOKI_POPEN _popen\n"
    "#define NOOKI_PCLOSE _pclose\n"
    "#else\n"
    "#define NOOKI_POPEN popen\n"
    "#define NOOKI_PCLOSE pclose\n"
    "#endif\n"
    "\n"
    "long long nooki_orden_ejecutar(const char* comando) {\n"
    "    if (comando == NULL) {\n"
    "        return -1;\n"
    "    }\n"
    "    return (long long)system(comando);\n"
    "}\n"
    "\n"
    "/* Lanza la orden y devuelve lo que escriba por la salida estándar. */\n"
    "nooki_texto nooki_orden_capturar(const char* comando) {\n"
    "    nooki_texto salida = nooki_texto_desde_c(\"\");\n"
    "    if (comando == NULL) {\n"
    "        return salida;\n"
    "    }\n"
    "    FILE* tubo = NOOKI_POPEN(comando, \"r\");\n"
    "    if (tubo == NULL) {\n"
    "        return salida;\n"
    "    }\n"
    "    char* datos = NULL;\n"
    "    size_t largo = 0;\n"
    "    size_t capacidad = 0;\n"
    "    char trozo[4096];\n"
    "    size_t leidos = 0;\n"
    "    while ((leidos = fread(trozo, 1, sizeof trozo, tubo)) > 0) {\n"
    "        if (largo + leidos + 1 > capacidad) {\n"
    "            size_t nueva = capacidad == 0 ? 8192 : capacidad * 2;\n"
    "            while (nueva < largo + leidos + 1) {\n"
    "                nueva *= 2;\n"
    "            }\n"
    "            char* crecido = (char*)realloc(datos, nueva);\n"
    "            if (crecido == NULL) {\n"
    "                free(datos);\n"
    "                NOOKI_PCLOSE(tubo);\n"
    "                return salida;\n"
    "            }\n"
    "            datos = crecido;\n"
    "            capacidad = nueva;\n"
    "        }\n"
    "        memcpy(datos + largo, trozo, leidos);\n"
    "        largo += leidos;\n"
    "    }\n"
    "    NOOKI_PCLOSE(tubo);\n"
    "    if (datos == NULL) {\n"
    "        return salida;\n"
    "    }\n"
    "    datos[largo] = '\\0';\n"
    "    nooki_texto_soltar(&salida);\n"
    "    salida = nooki_texto_desde_c(datos);\n"
    "    free(datos);\n"
    "    return salida;\n"
    "}\n"
    "\n"
    "/* ------------------------------------------------------------------ */\n"
    "/* Punteros y memoria manual. Solo se usan dentro de «inseguro».       */\n"
    "/* ------------------------------------------------------------------ */\n"
    "\n"
    "/* Una dirección de memoria. Se pasa por valor, como un puntero de C:  */\n"
    "/* ni se copia lo apuntado ni hay dueño. */\n"
    "typedef struct {\n"
    "    unsigned char* datos;\n"
    "} nooki_puntero;\n"
    "\n"
    "static nooki_puntero nooki_puntero_reservar(long long bytes) {\n"
    "    nooki_puntero p;\n"
    "    p.datos = NULL;\n"
    "    if (bytes <= 0) {\n"
    "        return p;\n"
    "    }\n"
    "    p.datos = (unsigned char*)calloc((size_t)bytes, 1);\n"
    "    return p;\n"
    "}\n"
    "\n"
    "static void nooki_puntero_liberar(nooki_puntero p) {\n"
    "    free(p.datos);\n"
    "}\n"
    "\n"
    "/* Las lecturas y escrituras van con memcpy: así valen para cualquier   */\n"
    "/* dirección, esté alineada o no. */\n"
    "static long long nooki_puntero_leer_entero(nooki_puntero p, long long desplazamiento) {\n"
    "    long long valor = 0;\n"
    "    if (p.datos == NULL || desplazamiento < 0) {\n"
    "        return 0;\n"
    "    }\n"
    "    memcpy(&valor, p.datos + desplazamiento, sizeof valor);\n"
    "    return valor;\n"
    "}\n"
    "\n"
    "static void nooki_puntero_escribir_entero(nooki_puntero p, long long desplazamiento,\n"
    "                                          long long valor) {\n"
    "    if (p.datos == NULL || desplazamiento < 0) {\n"
    "        return;\n"
    "    }\n"
    "    memcpy(p.datos + desplazamiento, &valor, sizeof valor);\n"
    "}\n"
    "\n"
    "static double nooki_puntero_leer_flotante(nooki_puntero p, long long desplazamiento) {\n"
    "    double valor = 0.0;\n"
    "    if (p.datos == NULL || desplazamiento < 0) {\n"
    "        return 0.0;\n"
    "    }\n"
    "    memcpy(&valor, p.datos + desplazamiento, sizeof valor);\n"
    "    return valor;\n"
    "}\n"
    "\n"
    "static void nooki_puntero_escribir_flotante(nooki_puntero p, long long desplazamiento,\n"
    "                                            double valor) {\n"
    "    if (p.datos == NULL || desplazamiento < 0) {\n"
    "        return;\n"
    "    }\n"
    "    memcpy(p.datos + desplazamiento, &valor, sizeof valor);\n"
    "}\n"
    "\n"
    "/* Para hablar con el hardware: un registro está en una dirección. */\n"
    "static nooki_puntero nooki_puntero_desde_direccion(long long direccion) {\n"
    "    nooki_puntero p;\n"
    "    p.datos = (unsigned char*)(intptr_t)direccion;\n"
    "    return p;\n"
    "}\n"
    "\n"
    "static long long nooki_puntero_direccion(nooki_puntero p) {\n"
    "    return (long long)(intptr_t)p.datos;\n"
    "}\n"
    "\n"
    "/* Añade al final del archivo, sin borrar lo que hubiera. */\n"
    "static int nooki_agregar_archivo(const char* ruta, const char* contenido) {\n"
    "    if (ruta == NULL || contenido == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    FILE* archivo = fopen(ruta, \"ab\");\n"
    "    if (archivo == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    const size_t largo = strlen(contenido);\n"
    "    const int bien = fwrite(contenido, 1, largo, archivo) == largo;\n"
    "    if (fclose(archivo) != 0) {\n"
    "        return 0;\n"
    "    }\n"
    "    return bien;\n"
    "}\n"
    "\n"
    "static int nooki_escribir_archivo(const char* ruta, const char* contenido) {\n"
    "    FILE* archivo = fopen(ruta, \"wb\");\n"
    "    if (archivo == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    size_t largo = strlen(contenido);\n"
    "    size_t escrito = fwrite(contenido, 1, largo, archivo);\n"
    "    fclose(archivo);\n"
    "    return escrito == largo;\n"
    "}\n"
    "\n"
    "static int nooki_existe_archivo(const char* ruta) {\n"
    "    FILE* archivo = fopen(ruta, \"rb\");\n"
    "    if (archivo == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    fclose(archivo);\n"
    "    return 1;\n"
    "}\n"
    "\n"
    "/* El mismo formato que al imprimir, pero devuelto como texto: sirve para\n"
    " * componer mensajes con números. */\n"
    "static nooki_texto nooki_texto_de_flotante(double valor) {\n"
    "    char buffer[64];\n"
    "    snprintf(buffer, sizeof buffer, \"%.6f\", valor);\n"
    "    size_t largo = strlen(buffer);\n"
    "    while (largo > 0 && buffer[largo - 1] == '0') {\n"
    "        --largo;\n"
    "    }\n"
    "    if (largo > 0 && buffer[largo - 1] == '.') {\n"
    "        --largo;\n"
    "    }\n"
    "    buffer[largo] = '\\0';\n"
    "    for (size_t i = 0; i < largo; ++i) {\n"
    "        if (buffer[i] == '.') {\n"
    "            buffer[i] = ',';\n"
    "        }\n"
    "    }\n"
    "    return nooki_texto_desde_c(buffer);\n"
    "}\n"
    "\n"
    "static nooki_texto nooki_texto_de_entero(long long valor) {\n"
    "    char buffer[32];\n"
    "    snprintf(buffer, sizeof buffer, \"%lld\", valor);\n"
    "    return nooki_texto_desde_c(buffer);\n"
    "}\n"
    "\n"
    "static void nooki_imprimir_flotante(double valor) {\n"
    "    char buffer[64];\n"
    "    snprintf(buffer, sizeof buffer, \"%.6f\", valor);\n"
    "    /* Se quitan los ceros que sobran y, si queda, el punto; el decimal va con coma. */\n"
    "    size_t largo = strlen(buffer);\n"
    "    while (largo > 0 && buffer[largo - 1] == '0') {\n"
    "        --largo;\n"
    "    }\n"
    "    if (largo > 0 && buffer[largo - 1] == '.') {\n"
    "        --largo;\n"
    "    }\n"
    "    buffer[largo] = '\\0';\n"
    "    for (size_t i = 0; i < largo; ++i) {\n"
    "        if (buffer[i] == '.') {\n"
    "            buffer[i] = ',';\n"
    "        }\n"
    "    }\n"
    "    printf(\"%s\\n\", buffer);\n"
    "}\n"
    "\n"
    "static void nooki_imprimir_entero(long long valor) {\n"
    "    printf(\"%lld\\n\", valor);\n"
    "}\n"
    "\n"
    "/* Entrada estándar: la línea llega sin el salto y con dueño. */\n"
    "static nooki_texto nooki_leer_linea(void) {\n"
    "    nooki_texto t;\n"
    "    long long capacidad = 64;\n"
    "    t.largo = 0;\n"
    "    t.propio = 1;\n"
    "    t.datos = (char*)malloc((size_t)capacidad);\n"
    "    if (t.datos == NULL) {\n"
    "        t.propio = 0;\n"
    "        return t;\n"
    "    }\n"
    "    int c;\n"
    "    while ((c = fgetc(stdin)) != EOF && c != '\\n') {\n"
    "        if (t.largo + 1 >= capacidad) {\n"
    "            capacidad *= 2;\n"
    "            char* datos = (char*)realloc(t.datos, (size_t)capacidad);\n"
    "            if (datos == NULL) {\n"
    "                break;\n"
    "            }\n"
    "            t.datos = datos;\n"
    "        }\n"
    "        t.datos[t.largo] = (char)c;\n"
    "        t.largo += 1;\n"
    "    }\n"
    "    t.datos[t.largo] = 0;\n"
    "    return t;\n"
    "}\n"
    "\n"
    "static long long nooki_leer_entero(void) {\n"
    "    nooki_texto linea = nooki_leer_linea();\n"
    "    long long valor = 0;\n"
    "    long long signo = 1;\n"
    "    long long i = 0;\n"
    "    if (linea.largo > 0 && linea.datos[0] == '-') {\n"
    "        signo = -1;\n"
    "        i = 1;\n"
    "    }\n"
    "    for (; i < linea.largo; ++i) {\n"
    "        const char d = linea.datos[i];\n"
    "        if (d < '0' || d > '9') {\n"
    "            break;\n"
    "        }\n"
    "        valor = valor * 10 + (d - '0');\n"
    "    }\n"
    "    nooki_texto_soltar(&linea);\n"
    "    return signo * valor;\n"
    "}\n"
    "\n"
    "static long long nooki_potencia(long long base, long long exponente) {\n"
    "    long long resultado = 1;\n"
    "    for (long long i = 0; i < exponente; ++i) {\n"
    "        resultado *= base;\n"
    "    }\n"
    "    return resultado;\n"
    "}\n";

std::string manglear(const std::string& nombre, const std::vector<std::string_view>& tipos) {
    std::string salida = "nooki_" + nombre;
    for (const std::string_view tipo : tipos) {
        salida += "_";
        salida += tipo;
    }
    return salida;
}

bool es_comparacion(std::string_view operador) {
    return operador == "==" || operador == "!=" || operador == "<" || operador == ">" ||
           operador == "<=" || operador == ">=";
}

class EmisorC {
public:
    std::string generar(const sintactico::Programa& programa) {
        registrar(programa);

        // Los cuerpos se generan primero en un búfer aparte para descubrir las
        // instanciaciones de funciones genéricas que aparecen en las llamadas.
        std::string cuerpos = generar_cuerpos(programa);

        // Instanciaciones, hasta que no aparezcan nuevas (una genérica puede
        // llamar a otra).
        std::set<std::string> emitidas;
        for (int vuelta = 0; vuelta < 4; ++vuelta) {
            std::vector<
                std::pair<std::string, std::pair<std::string, std::vector<std::string_view>>>>
                pendientes;
            for (const auto& [mangle, datos] : instancias_) {
                if (emitidas.count(mangle) == 0) {
                    pendientes.emplace_back(mangle, datos);
                }
            }
            if (pendientes.empty()) {
                break;
            }

            std::ostringstream original = std::move(salida_);
            salida_ = std::ostringstream{};
            for (const auto& [mangle, datos] : pendientes) {
                emitir_instancia(mangle, datos.first, datos.second);
                emitidas.insert(mangle);
            }
            cuerpos += salida_.str();
            salida_ = std::move(original);
        }

        salida_ << PRELUDIO;
        emitir_enumeraciones(programa);
        emitir_tipos(programa);

        for (const sintactico::Funcion& funcion : programa.funciones) {
            if (funcion.nombre == "principal" || !funcion.tipo_parametros.empty()) {
                continue;
            }
            salida_ << firma(funcion) << ";\n";
        }
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            for (const sintactico::Funcion& metodo : definicion.metodos) {
                salida_ << firma_metodo(definicion, metodo) << ";\n";
            }
        }
        for (const auto& [mangle, datos] : instancias_) {
            const auto generico = genericos_.find(datos.first);
            if (generico == genericos_.end()) {
                continue;
            }
            aplicar_sustitucion(*generico->second, datos.second);
            salida_ << firma_con_nombre(*generico->second, mangle) << ";\n";
            sustitucion_.clear();
        }

        salida_ << cuerpos;
        return salida_.str();
    }

    /// Genera los cuerpos de funciones y métodos en un búfer propio.
    std::string generar_cuerpos(const sintactico::Programa& programa) {
        std::ostringstream original = std::move(salida_);
        salida_ = std::ostringstream{};

        emitir_clases_runtime(programa);
        emitir_tablas_de_rasgo(programa);

        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            for (const sintactico::Funcion& metodo : definicion.metodos) {
                salida_ << '\n';
                emitir_metodo(definicion, metodo);
            }
        }
        // Las funciones de C se declaran; las de Nooki se definen.
        for (const sintactico::Funcion& funcion : programa.funciones) {
            if (!funcion.es_externa) {
                continue;
            }
            salida_ << firma_externa(funcion) << '\n';
        }

        for (const sintactico::Funcion& funcion : programa.funciones) {
            if (!funcion.tipo_parametros.empty() || funcion.es_externa) {
                continue;
            }
            salida_ << '\n';
            emitir_funcion(funcion);
        }

        std::string texto = salida_.str();
        salida_ = std::move(original);
        return texto;
    }

    void aplicar_sustitucion(const sintactico::Funcion& funcion,
                             const std::vector<std::string_view>& tipos) {
        sustitucion_.clear();
        for (std::size_t i = 0; i < funcion.tipo_parametros.size() && i < tipos.size(); ++i) {
            sustitucion_[std::string(funcion.tipo_parametros[i])] = std::string(tipos[i]);
        }
    }

    std::string firma_con_nombre(const sintactico::Funcion& funcion,
                                 const std::string& nombre) const {
        std::string salida =
            "static " + tipo_declarado_c(funcion.tipo_retorno) + " " + nombre + "(";
        if (funcion.parametros.empty()) {
            salida += "void";
        } else {
            for (std::size_t i = 0; i < funcion.parametros.size(); ++i) {
                if (i != 0) {
                    salida += ", ";
                }
                salida += tipo_declarado_c(funcion.parametros[i].tipo);
                salida += ' ';
                salida += funcion.parametros[i].nombre;
            }
        }
        salida += ')';
        return salida;
    }

    void emitir_instancia(const std::string& mangle, std::string_view nombre,
                          const std::vector<std::string_view>& tipos) {
        const auto generico = genericos_.find(std::string(nombre));
        if (generico == genericos_.end()) {
            no_soportado("una función genérica que no existe");
            return;
        }
        const sintactico::Funcion& funcion = *generico->second;
        aplicar_sustitucion(funcion, tipos);

        salida_ << '\n' << firma_con_nombre(funcion, mangle) << " {\n";
        abrir_ambito();
        ++nivel_;
        for (const sintactico::Parametro& parametro : funcion.parametros) {
            declarar(parametro.nombre, tipo_declarado_c(parametro.tipo));
        }
        for (const sintactico::Sentencia& sentencia : funcion.cuerpo) {
            emitir_sentencia(sentencia, false);
        }
        --nivel_;
        cerrar_ambito();
        salida_ << "}\n";

        sustitucion_.clear();
    }

    [[nodiscard]] bool soportado() const noexcept {
        return soportado_;
    }
    [[nodiscard]] const std::string& motivo() const noexcept {
        return motivo_;
    }

private:
    // --- Registro ----------------------------------------------------------
    void registrar(const sintactico::Programa& programa) {
        // Tipos genéricos e instanciaciones que el programa usa.
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            if (!definicion.tipo_parametros.empty()) {
                genericos_tipo_[std::string(definicion.nombre)] = &definicion;
            }
        }
        for (const sintactico::Funcion& funcion : programa.funciones) {
            if (funcion.es_externa) {
                externas_.insert(std::string(funcion.nombre));
            }
        }
        for (const sintactico::Rasgo& rasgo : programa.rasgos) {
            rasgos_.insert(std::string(rasgo.nombre));
            rasgos_def_[std::string(rasgo.nombre)] = &rasgo;
        }
        RASGOS_DEL_PROGRAMA = &rasgos_;
        VECTORES_DE_ESTRUCTURA = &vectores_de_estructura_;
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            if (definicion.es_clase) {
                clases_.insert(std::string(definicion.nombre));
            }
        }
        recoger_instancias(programa);

        // `Vector<MiEstructura>`: se busca en todo el programa qué estructuras
        // se meten en un vector, para generarles su vector en C.
        {
            const auto es_estructura = [&programa](const std::string& nombre) {
                for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
                    if (definicion.nombre == nombre && !definicion.es_clase) {
                        return true;
                    }
                }
                return false;
            };
            const auto anotar = [&](std::string_view texto) {
                const std::size_t abre = texto.find("Vector<");
                if (abre == std::string_view::npos) {
                    return;
                }
                const std::size_t cierra = texto.find('>', abre);
                if (cierra == std::string_view::npos) {
                    return;
                }
                const std::string dentro(texto.substr(abre + 7, cierra - abre - 7));
                if (!es_estructura(dentro)) {
                    return;
                }
                // Un campo de texto sí se admite: el vector guarda su propia copia
                // y la libera. Una colección o una caja dentro de la estructura
                // exigiría copiarlas también, y eso todavía no está.
                for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
                    if (definicion.nombre != dentro) {
                        continue;
                    }
                    for (const sintactico::Campo& campo : definicion.campos) {
                        const std::string_view tipo_campo = campo.tipo;
                        if (tipo_campo.rfind("Vector<", 0) == 0 ||
                            tipo_campo.rfind("Mapa<", 0) == 0 ||
                            tipo_campo.rfind("Caja<", 0) == 0) {
                            no_soportado("un Vector de la estructura «" + dentro +
                                         "», que tiene una colección o una caja dentro");
                            return;
                        }
                        if (tipo_campo == "texto") {
                            estructuras_con_texto_.insert(dentro);
                        }
                    }
                }
                vectores_de_estructura_.insert(dentro);
            };
            std::function<void(const sintactico::Expresion&)> por_expresion;
            std::function<void(const std::vector<sintactico::Sentencia>&)> por_cuerpo;
            por_expresion = [&](const sintactico::Expresion& expresion) {
                anotar(expresion.anotacion);
                for (const std::string_view tipo : expresion.tipos_aplicados) {
                    // Se pasa por `anotar` para que la guarda de los campos con
                    // dueño se aplique también aquí.
                    anotar("Vector<" + std::string(tipo) + ">");
                }
                if (expresion.izquierda) {
                    por_expresion(*expresion.izquierda);
                }
                if (expresion.derecha) {
                    por_expresion(*expresion.derecha);
                }
                for (const sintactico::ExpresionPtr& hija : expresion.lista) {
                    if (hija) {
                        por_expresion(*hija);
                    }
                }
            };
            por_cuerpo = [&](const std::vector<sintactico::Sentencia>& cuerpo) {
                for (const sintactico::Sentencia& sentencia : cuerpo) {
                    anotar(sentencia.tipo_declarado);
                    if (sentencia.expresion) {
                        por_expresion(*sentencia.expresion);
                    }
                    if (sentencia.paso) {
                        por_expresion(*sentencia.paso);
                    }
                    por_cuerpo(sentencia.cuerpo);
                    por_cuerpo(sentencia.sino);
                    for (const sintactico::Rama& rama : sentencia.ramas) {
                        por_cuerpo(rama.cuerpo);
                    }
                }
            };
            for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
                for (const sintactico::Campo& campo : definicion.campos) {
                    anotar(campo.tipo);
                }
            }
            for (const sintactico::Funcion& funcion : programa.funciones) {
                anotar(funcion.tipo_retorno);
                for (const sintactico::Parametro& parametro : funcion.parametros) {
                    anotar(parametro.tipo);
                }
                por_cuerpo(funcion.cuerpo);
            }
        }
        for (const std::string& instancia : instancias_tipo_) {
            tipo_c_de_[instancia] = nombre_de_instancia(instancia);

            // Los campos se registran ya, porque los cuerpos se generan antes
            // de emitir las estructuras.
            const std::size_t abre = instancia.find('<');
            if (abre == std::string::npos) {
                continue;
            }
            const auto generico = genericos_tipo_.find(instancia.substr(0, abre));
            if (generico == genericos_tipo_.end()) {
                continue;
            }
            const std::string nombre_c = nombre_de_instancia(instancia);
            const std::vector<std::string> argumentos = argumentos_de_instancia(instancia);
            bool con_dueno = false;
            for (const sintactico::Campo& campo : generico->second->campos) {
                const std::string tipo_campo =
                    tipo_declarado_c(sustituir_tipo(campo.tipo, *generico->second, argumentos));
                campos_[nombre_c][std::string(campo.nombre)] = tipo_campo;
                if (tipo_campo == "nooki_texto" || tipo_campo == "nooki_caja" ||
                    tipo_campo.rfind("nooki_vector_", 0) == 0 ||
                    tipo_campo.rfind("nooki_mapa_", 0) == 0) {
                    con_dueno = true;
                }
            }
            if (con_dueno) {
                compuestos_con_dueno_.insert(nombre_c);
            }
        }

        // Primera pasada: nombres, para que un tipo pueda referirse a otro declarado después.
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            campos_[tipo_estructura(definicion)];
            for (const sintactico::Funcion& metodo : definicion.metodos) {
                metodos_[tipo_estructura(definicion)][std::string(metodo.nombre)] = &metodo;
            }
        }
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            const std::string nombre_c = tipo_estructura(definicion);
            tipo_ptr_[std::string(definicion.nombre)] = &definicion;
            por_c_[tipo_estructura(definicion)] = &definicion;
            if (!definicion.modulo.empty()) {
                tipo_ptr_[std::string(definicion.modulo) + "." + std::string(definicion.nombre)] =
                    &definicion;
            }
            tipo_c_de_[std::string(definicion.nombre)] = nombre_c;
            if (!definicion.modulo.empty()) {
                tipo_c_de_[std::string(definicion.modulo) + "." + std::string(definicion.nombre)] =
                    nombre_c;
            }
        }
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            if (definicion.base.empty()) {
                continue;
            }
            const auto padre = tipo_c_de_.find(std::string(definicion.base));
            if (padre != tipo_c_de_.end()) {
                base_de_[tipo_estructura(definicion)] = padre->second;
            }
        }
        // Raíz de cada jerarquía y sus métodos virtuales.
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            const std::string nombre_c = tipo_estructura(definicion);
            std::string raiz = nombre_c;
            for (int guarda = 0; guarda < 32; ++guarda) {
                const auto base = base_de_.find(raiz);
                if (base == base_de_.end()) {
                    break;
                }
                raiz = base->second;
            }
            const auto raiz_tipo = por_c_.find(raiz);
            if (raiz_tipo != por_c_.end()) {
                for (const sintactico::Funcion& metodo : raiz_tipo->second->metodos) {
                    const std::string nombre_metodo(metodo.nombre);
                    if (std::find(virtuales_de_[raiz].begin(), virtuales_de_[raiz].end(),
                                  nombre_metodo) == virtuales_de_[raiz].end()) {
                        virtuales_de_[raiz].push_back(nombre_metodo);
                    }
                }
            }

            // Campos heredados primero, después los propios.
            std::vector<std::string> cadena;
            for (std::string actual = nombre_c;;) {
                cadena.insert(cadena.begin(), actual);
                const auto base = base_de_.find(actual);
                if (base == base_de_.end()) {
                    break;
                }
                actual = base->second;
            }
            for (const std::string& clase : cadena) {
                const auto tipo = por_c_.find(clase);
                if (tipo == por_c_.end()) {
                    continue;
                }
                for (const sintactico::Campo& campo : tipo->second->campos) {
                    campos_planos_[nombre_c].emplace_back(std::string(campo.nombre),
                                                          tipo_declarado_c(campo.tipo));
                }
            }
        }
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            const std::string nombre_c = tipo_estructura(definicion);
            // Los campos del mapa incluyen los heredados, para resolver accesos.
            for (const auto& [campo, tipo_campo] : campos_planos_[nombre_c]) {
                campos_[nombre_c][campo] = tipo_campo;
            }
        }
        // Los nombres de enumeración también son tipos resolubles.
        for (const sintactico::Enumeracion& enumeracion : programa.enumeraciones) {
            tipo_c_de_[std::string(enumeracion.nombre)] =
                "nooki_" + std::string(enumeracion.nombre);
            if (!enumeracion.modulo.empty()) {
                tipo_c_de_[std::string(enumeracion.modulo) + "." +
                           std::string(enumeracion.nombre)] =
                    "nooki_" + std::string(enumeracion.nombre);
            }
        }
        for (const sintactico::Funcion& funcion : programa.funciones) {
            retornos_[std::string(funcion.nombre)] = tipo_declarado_c(funcion.tipo_retorno);
            retorno_actual_ = retornos_[std::string(funcion.nombre)];

            std::vector<std::string> tipos;
            tipos.reserve(funcion.parametros.size());
            for (const sintactico::Parametro& parametro : funcion.parametros) {
                tipos.emplace_back(parametro.tipo);
            }
            parametros_[std::string(funcion.nombre)] = std::move(tipos);
            if (!funcion.modulo.empty()) {
                modulos_[funcion.modulo].emplace(funcion.nombre, &funcion);
            } else {
                funciones_ast_[std::string(funcion.nombre)] = &funcion;
            }
        }
        for (const sintactico::Enumeracion& enumeracion : programa.enumeraciones) {
            enumeraciones_[std::string(enumeracion.nombre)] = &enumeracion;
            // El nombre del tipo de una enumeración se resuelve a su nombre de C.
            tipo_c_de_[std::string(enumeracion.nombre)] =
                "nooki_" + std::string(enumeracion.nombre);
            if (!enumeracion.modulo.empty()) {
                tipo_c_de_[std::string(enumeracion.modulo) + "." +
                           std::string(enumeracion.nombre)] =
                    "nooki_" + std::string(enumeracion.nombre);
            }
            for (std::size_t i = 0; i < enumeracion.variantes.size(); ++i) {
                variantes_[std::string(enumeracion.variantes[i].nombre)] = {&enumeracion, i};
            }
        }
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            if (!definicion.tipo_parametros.empty()) {
                genericos_tipo_[std::string(definicion.nombre)] = &definicion;
            }
        }
        for (const sintactico::Funcion& funcion : programa.funciones) {
            if (!funcion.tipo_parametros.empty()) {
                genericos_[std::string(funcion.nombre)] = &funcion;
            }
        }
    }

    void emitir_enumeraciones(const sintactico::Programa& programa) {
        if (programa.enumeraciones.empty()) {
            return;
        }
        salida_ << '\n';
        for (const sintactico::Enumeracion& enumeracion : programa.enumeraciones) {
            bool con_datos = false;
            for (const sintactico::Variante& variante : enumeracion.variantes) {
                if (!variante.campos.empty()) {
                    con_datos = true;
                }
            }

            if (!con_datos) {
                salida_ << "typedef int nooki_" << enumeracion.nombre << ";\n";
                salida_ << "enum {";
                for (std::size_t i = 0; i < enumeracion.variantes.size(); ++i) {
                    if (i != 0) {
                        salida_ << ", ";
                    }
                    salida_ << constante_enum(enumeracion.nombre, enumeracion.variantes[i].nombre)
                            << " = " << i;
                }
                salida_ << "};\n\n";
                continue;
            }

            // Variantes con datos: unión etiquetada.
            const std::string nombre_c = "nooki_" + std::string(enumeracion.nombre);
            salida_ << "typedef struct " << nombre_c << " " << nombre_c << ";\n";
            salida_ << "struct " << nombre_c << " {\n";
            salida_ << "    int etiqueta;\n";
            salida_ << "    union {\n";
            for (const sintactico::Variante& variante : enumeracion.variantes) {
                for (const sintactico::Campo& campo : variante.campos) {
                    salida_ << "        " << tipo_declarado_c(campo.tipo) << ' ' << variante.nombre
                            << "_" << campo.nombre << ";\n";
                }
            }
            salida_ << "    } datos;\n";
            salida_ << "};\n\n";

            salida_ << "enum {";
            for (std::size_t i = 0; i < enumeracion.variantes.size(); ++i) {
                if (i != 0) {
                    salida_ << ", ";
                }
                salida_ << constante_enum(enumeracion.nombre, enumeracion.variantes[i].nombre)
                        << " = " << i;
            }
            salida_ << "};\n\n";

            // Liberación de las variantes que llevan texto.
            bool lleva_texto = false;
            for (const sintactico::Variante& variante : enumeracion.variantes) {
                for (const sintactico::Campo& campo : variante.campos) {
                    if (campo.tipo == "texto") {
                        lleva_texto = true;
                    }
                }
            }
            if (lleva_texto) {
                salida_ << "static void " << nombre_c << "_soltar(" << nombre_c << "* valor) {\n";
                for (std::size_t i = 0; i < enumeracion.variantes.size(); ++i) {
                    const sintactico::Variante& variante = enumeracion.variantes[i];
                    bool texto = false;
                    for (const sintactico::Campo& campo : variante.campos) {
                        if (campo.tipo == "texto") {
                            texto = true;
                        }
                    }
                    if (!texto) {
                        continue;
                    }
                    salida_ << "    if (valor->etiqueta == " << i << ") {\n";
                    for (const sintactico::Campo& campo : variante.campos) {
                        if (campo.tipo == "texto") {
                            salida_ << "        nooki_texto_soltar(&valor->datos."
                                    << variante.nombre << "_" << campo.nombre << ");\n";
                        }
                    }
                    salida_ << "    }\n";
                }
                salida_ << "}\n\n";
            }
        }
    }

    void emitir_tipos(const sintactico::Programa& programa) {
        if (programa.tipos.empty()) {
            return;
        }
        salida_ << '\n';

        // Tablas virtuales: una por raíz de jerarquía, con un hueco por método suyo.
        // Se declaran antes de las estructuras (que solo guardan un puntero) y se
        // definen después, porque sus firmas pueden nombrar tipos ya definidos.
        for (const auto& [raiz, metodos] : virtuales_de_) {
            if (!metodos.empty() && por_c_.count(raiz) != 0) {
                salida_ << "typedef struct " << raiz << "_vtabla " << raiz << "_vtabla;\n";
            }
        }
        salida_ << '\n';

        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            const std::string nombre_c = tipo_estructura(definicion);
            salida_ << "typedef struct " << nombre_c << " {\n";

            // Las clases llevan la tabla virtual delante, para poder despachar.
            if (definicion.es_clase && tiene_virtuales(nombre_c)) {
                salida_ << "    const " << raiz_c(nombre_c) << "_vtabla* vtabla;\n";
            }
            for (const auto& [campo, tipo_campo] : campos_planos_[nombre_c]) {
                salida_ << "    " << tipo_campo << ' ' << campo << ";\n";
            }
            salida_ << "} " << nombre_c << ";\n\n";
        }

        // Vector de una estructura: se genera uno por cada estructura que se
        // meta en un vector. Solo hay estructuras sin campos con dueño, así que
        // copiar el valor es copiar todo lo que hay que copiar.
        for (const std::string& nombre : vectores_de_estructura_) {
            const std::string tipo = "nooki_" + nombre;
            const std::string vec = "nooki_vector_" + nombre;
            const bool con_texto = estructuras_con_texto_.count(nombre) != 0;

            // Con campos de texto hacen falta tres ayudantes: copiar de verdad,
            // dar una vista (para leer sin liberar) y liberar.
            if (con_texto) {
                const auto& campos = campos_planos_[tipo];
                salida_ << "static " << tipo << " " << tipo << "_copiar(" << tipo << " valor) {\n";
                for (const auto& [campo, tipo_campo] : campos) {
                    if (tipo_campo == "nooki_texto") {
                        salida_ << "    valor." << campo << " = nooki_texto_copiar(valor." << campo
                                << ");\n";
                    }
                }
                salida_ << "    return valor;\n}\n\n";

                salida_ << "static " << tipo << " " << tipo << "_vista(" << tipo << " valor) {\n";
                for (const auto& [campo, tipo_campo] : campos) {
                    if (tipo_campo == "nooki_texto") {
                        salida_ << "    valor." << campo << ".propio = 0;\n";
                    }
                }
                salida_ << "    return valor;\n}\n\n";

                salida_ << "static void " << tipo << "_liberar(" << tipo << "* valor) {\n";
                for (const auto& [campo, tipo_campo] : campos) {
                    if (tipo_campo == "nooki_texto") {
                        salida_ << "    nooki_texto_soltar(&valor->" << campo << ");\n";
                    }
                }
                salida_ << "}\n\n";
            }
            salida_ << "typedef struct {\n"
                    << "    " << tipo << "* datos;\n"
                    << "    long long largo;\n"
                    << "    long long capacidad;\n"
                    << "} " << vec << ";\n\n";

            salida_ << "static " << vec << " " << vec << "_nuevo(void) {\n"
                    << "    " << vec << " v;\n"
                    << "    v.datos = NULL;\n"
                    << "    v.largo = 0;\n"
                    << "    v.capacidad = 0;\n"
                    << "    return v;\n"
                    << "}\n\n";

            salida_ << "static void " << vec << "_agregar(" << vec << "* v, " << tipo
                    << " valor) {\n"
                    << "    if (v->largo >= v->capacidad) {\n"
                    << "        const long long nueva = v->capacidad == 0 ? 8 : v->capacidad * 2;\n"
                    << "        " << tipo << "* datos = (" << tipo
                    << "*)realloc(v->datos, (size_t)nueva * sizeof(" << tipo << "));\n"
                    << "        if (datos == NULL) {\n"
                    << "            return;\n"
                    << "        }\n"
                    << "        v->datos = datos;\n"
                    << "        v->capacidad = nueva;\n"
                    << "    }\n"
                    << "    v->datos[v->largo] = "
                    << (con_texto ? tipo + "_copiar(valor)" : std::string("valor")) << ";\n"
                    << "    v->largo += 1;\n"
                    << "}\n\n";

            salida_ << "static " << tipo << " " << vec << "_obtener(" << vec
                    << "* v, long long indice) {\n"
                    << "    " << tipo << " vacio;\n"
                    << "    memset(&vacio, 0, sizeof vacio);\n"
                    << "    if (indice < 0 || indice >= v->largo) {\n"
                    << "        return vacio;\n"
                    << "    }\n"
                    << "    return "
                    << (con_texto ? tipo + "_vista(v->datos[indice])"
                                  : std::string("v->datos[indice]"))
                    << ";\n"
                    << "}\n\n";

            salida_ << "static long long " << vec << "_largo(" << vec << "* v) {\n"
                    << "    return v->largo;\n"
                    << "}\n\n";

            salida_ << "static void " << vec << "_modificar(" << vec << "* v, long long indice, "
                    << tipo << " valor) {\n"
                    << "    if (indice < 0 || indice >= v->largo) {\n"
                    << "        return;\n"
                    << "    }\n"
                    << "    "
                    << (con_texto ? tipo + "_liberar(&v->datos[indice]);\n    v->datos[indice] = " +
                                        tipo + "_copiar(valor);"
                                  : std::string("v->datos[indice] = valor;"))
                    << "\n}\n\n";

            salida_ << "static void " << vec << "_vaciar(" << vec << "* v) {\n";
            if (con_texto) {
                salida_ << "    for (long long i = 0; i < v->largo; ++i) {\n"
                        << "        " << tipo << "_liberar(&v->datos[i]);\n"
                        << "    }\n";
            }
            salida_ << "    v->largo = 0;\n"
                    << "}\n\n";

            salida_ << "static void " << vec << "_soltar(" << vec << "* v) {\n";
            if (con_texto) {
                salida_ << "    for (long long i = 0; i < v->largo; ++i) {\n"
                        << "        " << tipo << "_liberar(&v->datos[i]);\n"
                        << "    }\n";
            }
            salida_ << "    free(v->datos);\n"
                    << "    v->datos = NULL;\n"
                    << "    v->largo = 0;\n"
                    << "    v->capacidad = 0;\n"
                    << "}\n\n";
        }

        // Rasgos: tabla de métodos por rasgo y referencia (objeto + tabla).
        if (!programa.rasgos.empty()) {
            for (const sintactico::Rasgo& rasgo : programa.rasgos) {
                salida_ << "typedef struct nooki_" << rasgo.nombre << "_vtabla {\n";
                for (const sintactico::Funcion& metodo : rasgo.metodos) {
                    salida_ << "    " << tipo_declarado_c(metodo.tipo_retorno) << " (*"
                            << metodo.nombre << ")(void*";
                    for (const sintactico::Parametro& parametro : metodo.parametros) {
                        salida_ << ", " << tipo_declarado_c(parametro.tipo);
                    }
                    salida_ << ");\n";
                }
                salida_ << "} nooki_" << rasgo.nombre << "_vtabla;\n\n";
                salida_ << "typedef struct {\n"
                        << "    void* objeto;\n"
                        << "    const nooki_" << rasgo.nombre << "_vtabla* tabla;\n"
                        << "} nooki_ref_" << rasgo.nombre << ";\n\n";
            }
        }

        // Instanciaciones de tipos genéricos: una estructura por combinación.
        for (const std::string& instancia : instancias_tipo_) {
            const std::size_t abre = instancia.find('<');
            if (abre == std::string::npos) {
                continue;
            }
            const std::string base = instancia.substr(0, abre);
            const auto generico = genericos_tipo_.find(base);
            if (generico == genericos_tipo_.end()) {
                continue;
            }

            // Argumentos separados por comas.
            std::vector<std::string> argumentos;
            std::string actual;
            for (std::size_t i = abre + 1; i < instancia.size(); ++i) {
                const char caracter = instancia[i];
                if (caracter == ',' || caracter == '>') {
                    if (!actual.empty()) {
                        argumentos.push_back(actual);
                        actual.clear();
                    }
                    if (caracter == '>') {
                        break;
                    }
                    continue;
                }
                if (caracter != ' ') {
                    actual += caracter;
                }
            }

            const std::string nombre_c = nombre_de_instancia(instancia);
            salida_ << "typedef struct " << nombre_c << " {\n";
            for (const sintactico::Campo& campo : generico->second->campos) {
                std::string tipo_campo(campo.tipo);
                for (std::size_t i = 0; i < generico->second->tipo_parametros.size(); ++i) {
                    if (tipo_campo == generico->second->tipo_parametros[i] &&
                        i < argumentos.size()) {
                        tipo_campo = argumentos[i];
                        break;
                    }
                }
                campos_[nombre_c][std::string(campo.nombre)] = tipo_declarado_c(tipo_campo);
                salida_ << "    " << tipo_declarado_c(tipo_campo) << ' ' << campo.nombre << ";\n";
            }
            salida_ << "} " << nombre_c << ";\n\n";

            // Si algún campo tiene dueño, el compuesto lo libera al salir.
            bool con_dueno = false;
            salida_ << "static void " << nombre_c << "_soltar(" << nombre_c << "* valor) {\n";
            for (const sintactico::Campo& campo : generico->second->campos) {
                const std::string tipo_campo =
                    tipo_declarado_c(sustituir_tipo(campo.tipo, *generico->second, argumentos));
                if (tipo_campo == "nooki_texto") {
                    salida_ << "    nooki_texto_soltar(&valor->" << campo.nombre << ");\n";
                    con_dueno = true;
                } else if (tipo_campo == "nooki_caja") {
                    salida_ << "    nooki_caja_soltar(&valor->" << campo.nombre << ");\n";
                    con_dueno = true;
                } else if (tipo_campo.rfind("nooki_vector_", 0) == 0 ||
                           tipo_campo.rfind("nooki_mapa_", 0) == 0) {
                    salida_ << "    " << tipo_campo << "_soltar(&valor->" << campo.nombre << ");\n";
                    con_dueno = true;
                }
            }
            salida_ << "}\n\n";
            if (con_dueno) {
                compuestos_con_dueno_.insert(nombre_c);
            }
        }

        // Definición de las tablas virtuales, ya con todos los tipos disponibles.
        for (const auto& [raiz, metodos] : virtuales_de_) {
            if (metodos.empty()) {
                continue;
            }
            const auto tipo = por_c_.find(raiz);
            if (tipo == por_c_.end()) {
                continue;
            }
            salida_ << "struct " << raiz << "_vtabla {\n";
            for (const std::string& nombre : metodos) {
                const sintactico::Funcion* metodo = nullptr;
                for (const sintactico::Funcion& candidato : tipo->second->metodos) {
                    if (candidato.nombre == nombre) {
                        metodo = &candidato;
                        break;
                    }
                }
                if (metodo == nullptr) {
                    continue;
                }
                salida_ << "    " << tipo_declarado_c(metodo->tipo_retorno) << " (*" << nombre
                        << ")(void*";
                for (const sintactico::Parametro& parametro : metodo->parametros) {
                    salida_ << ", " << tipo_declarado_c(parametro.tipo);
                }
                salida_ << ");\n";
            }
            salida_ << "};\n\n";
        }
    }

    bool es_virtual(const sintactico::DefinicionTipo& definicion,
                    const sintactico::Funcion& metodo) const {
        const auto virtuales = virtuales_de_.find(raiz_c(tipo_estructura(definicion)));
        if (virtuales == virtuales_de_.end()) {
            return false;
        }
        return std::find(virtuales->second.begin(), virtuales->second.end(),
                         std::string(metodo.nombre)) != virtuales->second.end();
    }

    /// Registra una instanciación de tipo genérico si el texto lo es.
    void recoger_tipo(std::string_view texto) {
        const std::size_t abre = texto.find('<');
        if (abre == std::string_view::npos) {
            return;
        }
        const std::string base(texto.substr(0, abre));
        if (genericos_tipo_.count(base) != 0) {
            instancias_tipo_.insert(std::string(texto));
        }
    }

    void recoger_tipos_de(const std::vector<sintactico::Sentencia>& cuerpo) {
        for (const sintactico::Sentencia& sentencia : cuerpo) {
            recoger_tipo(sentencia.tipo_declarado);
            if (sentencia.expresion) {
                recoger_tipo_de_expresion(*sentencia.expresion);
            }
            recoger_tipos_de(sentencia.cuerpo);
            recoger_tipos_de(sentencia.sino);
            for (const sintactico::Rama& rama : sentencia.ramas) {
                recoger_tipos_de(rama.cuerpo);
            }
        }
    }

    void recoger_tipo_de_expresion(const sintactico::Expresion& expresion) {
        if (expresion.tipo == sintactico::TipoExpresion::Construccion) {
            recoger_tipo(expresion.anotacion);
        }
        for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
            if (argumento) {
                recoger_tipo_de_expresion(*argumento);
            }
        }
        if (expresion.izquierda) {
            recoger_tipo_de_expresion(*expresion.izquierda);
        }
        if (expresion.derecha) {
            recoger_tipo_de_expresion(*expresion.derecha);
        }
    }

    void recoger_instancias(const sintactico::Programa& programa) {
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            for (const sintactico::Campo& campo : definicion.campos) {
                recoger_tipo(campo.tipo);
            }
        }
        for (const sintactico::Funcion& funcion : programa.funciones) {
            recoger_tipo(funcion.tipo_retorno);
            for (const sintactico::Parametro& parametro : funcion.parametros) {
                recoger_tipo(parametro.tipo);
            }
            recoger_tipos_de(funcion.cuerpo);
        }
    }

    /// Argumentos de una instanciación: `par<entero, texto>` → {"entero", "texto"}.
    static std::vector<std::string> argumentos_de_instancia(std::string_view texto) {
        std::vector<std::string> argumentos;
        std::string actual;
        const std::size_t abre = texto.find('<');
        if (abre == std::string_view::npos) {
            return argumentos;
        }
        for (std::size_t i = abre + 1; i < texto.size(); ++i) {
            const char caracter = texto[i];
            if (caracter == ',' || caracter == '>') {
                if (!actual.empty()) {
                    argumentos.push_back(actual);
                    actual.clear();
                }
                if (caracter == '>') {
                    break;
                }
                continue;
            }
            if (caracter != ' ') {
                actual += caracter;
            }
        }
        return argumentos;
    }

    /// Sustituye los parámetros de tipo de un campo por sus argumentos.
    static std::string sustituir_tipo(std::string_view tipo,
                                      const sintactico::DefinicionTipo& generico,
                                      const std::vector<std::string>& argumentos) {
        for (std::size_t i = 0; i < generico.tipo_parametros.size() && i < argumentos.size(); ++i) {
            if (tipo == generico.tipo_parametros[i]) {
                return argumentos[i];
            }
        }
        return std::string(tipo);
    }

    /// Nombre de C de una instanciación: `par<entero>` → `nooki_par_entero`.
    static std::string nombre_de_instancia(std::string_view texto) {
        std::string salida = "nooki_";
        for (const char caracter : texto) {
            if (caracter == '<' || caracter == '>' || caracter == ',' || caracter == ' ') {
                if (caracter == ',') {
                    salida += "_";
                }
                continue;
            }
            salida += caracter;
        }
        return salida;
    }

    /// Raíz de la jerarquía de una clase (por nombre de C).
    std::string raiz_c(const std::string& nombre) const {
        std::string actual = nombre;
        for (int guarda = 0; guarda < 32; ++guarda) {
            const auto base = base_de_.find(actual);
            if (base == base_de_.end()) {
                break;
            }
            actual = base->second;
        }
        return actual;
    }

    bool tiene_virtuales(const std::string& nombre) const {
        const auto virtuales = virtuales_de_.find(raiz_c(nombre));
        return virtuales != virtuales_de_.end() && !virtuales->second.empty();
    }

    /// Tablas de rasgo para cada tipo que los implementa, con un adaptador por
    /// método para que la firma sea la del rasgo y no la del tipo.
    void emitir_tablas_de_rasgo(const sintactico::Programa& programa) {
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            for (const std::string_view nombre_rasgo : definicion.rasgos) {
                const sintactico::Rasgo* rasgo = nullptr;
                for (const sintactico::Rasgo& candidato : programa.rasgos) {
                    if (candidato.nombre == nombre_rasgo) {
                        rasgo = &candidato;
                        break;
                    }
                }
                if (rasgo == nullptr) {
                    continue;
                }

                const std::string tipo_c = tipo_estructura(definicion);
                const std::string tabla_c = tipo_c + "_" + std::string(nombre_rasgo) + "_tabla";

                for (const sintactico::Funcion& exigido : rasgo->metodos) {
                    salida_ << "static " << tipo_declarado_c(exigido.tipo_retorno) << " " << tabla_c
                            << "_" << exigido.nombre << "(void* objeto";
                    for (const sintactico::Parametro& parametro : exigido.parametros) {
                        salida_ << ", " << tipo_declarado_c(parametro.tipo) << ' '
                                << parametro.nombre;
                    }
                    salida_ << ") {\n";
                    salida_ << "    return " << tipo_c << "_" << exigido.nombre << "((" << tipo_c
                            << "*)objeto";
                    for (const sintactico::Parametro& parametro : exigido.parametros) {
                        salida_ << ", " << parametro.nombre;
                    }
                    salida_ << ");\n}\n\n";
                }

                salida_ << "static const nooki_" << nombre_rasgo << "_vtabla " << tabla_c
                        << " = {\n";
                for (const sintactico::Funcion& exigido : rasgo->metodos) {
                    salida_ << "    " << tabla_c << "_" << exigido.nombre << ",\n";
                }
                salida_ << "};\n\n";
            }
        }
    }

    /// Reserva, libera y tabla virtual de cada clase. Va después de los prototipos,
    /// porque la tabla necesita la dirección de los métodos.
    void emitir_clases_runtime(const sintactico::Programa& programa) {
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            if (!definicion.es_clase) {
                continue;
            }
            const std::string nombre_c = tipo_estructura(definicion);
            const auto& campos = campos_planos_[nombre_c];

            if (tiene_virtuales(nombre_c)) {
                salida_ << "static const " << raiz_c(nombre_c) << "_vtabla " << nombre_c
                        << "_tabla = {\n";
                for (const std::string& metodo : virtuales_de_at(nombre_c)) {
                    // Si la clase no lo redefine, hereda la implementación de su base.
                    salida_ << "    " << implementacion_de(nombre_c, metodo) << "_" << metodo
                            << ",\n";
                }
                salida_ << "};\n\n";
            }

            salida_ << "static " << nombre_c << "* " << nombre_c << "_nuevo(";
            if (campos.empty()) {
                salida_ << "void";
            } else {
                for (std::size_t i = 0; i < campos.size(); ++i) {
                    if (i != 0) {
                        salida_ << ", ";
                    }
                    salida_ << campos[i].second << ' ' << campos[i].first;
                }
            }
            salida_ << ") {\n";
            salida_ << "    " << nombre_c << "* objeto = (" << nombre_c << "*)malloc(sizeof("
                    << nombre_c << "));\n";
            salida_ << "    if (objeto == NULL) {\n        return NULL;\n    }\n";
            if (tiene_virtuales(nombre_c)) {
                salida_ << "    objeto->vtabla = &" << nombre_c << "_tabla;\n";
            }
            for (const auto& [campo, tipo_campo] : campos) {
                salida_ << "    objeto->" << campo << " = " << campo << ";\n";
            }
            salida_ << "    return objeto;\n}\n\n";
            salida_ << "static void " << nombre_c << "_soltar(" << nombre_c
                    << "* objeto) {\n    free(objeto);\n}\n\n";
        }
    }

    /// Clase de la jerarquía que implementa el método (la más derivada que lo define).
    std::string implementacion_de(const std::string& clase, const std::string& metodo) const {
        std::string actual = clase;
        for (int guarda = 0; guarda < 32; ++guarda) {
            const auto tipo = por_c_.find(actual);
            if (tipo != por_c_.end()) {
                for (const sintactico::Funcion& declarado : tipo->second->metodos) {
                    if (declarado.nombre == metodo) {
                        return actual;
                    }
                }
            }
            const auto base = base_de_.find(actual);
            if (base == base_de_.end()) {
                break;
            }
            actual = base->second;
        }
        return clase;
    }

    std::vector<std::string> virtuales_de_at(const std::string& nombre) const {
        const auto virtuales = virtuales_de_.find(raiz_c(nombre));
        return virtuales != virtuales_de_.end() ? virtuales->second : std::vector<std::string>{};
    }

    std::string tipo_de_campo(const std::string& clase, const std::string& campo,
                              const sintactico::Programa& programa) const {
        std::string actual = clase;
        for (int guarda = 0; guarda < 32; ++guarda) {
            for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
                if (tipo_estructura(definicion) != actual) {
                    continue;
                }
                for (const sintactico::Campo& declarado : definicion.campos) {
                    if (declarado.nombre == campo) {
                        return tipo_declarado_c(declarado.tipo);
                    }
                }
            }
            const auto base = base_de_.find(actual);
            if (base == base_de_.end()) {
                break;
            }
            actual = base->second;
        }
        return "long long";
    }

    // --- Ámbito ------------------------------------------------------------
    struct Liberable {
        std::string nombre;
        std::string funcion;
        bool por_puntero = false;
    };

    struct Ambito {
        std::map<std::string, std::string> tipos;
        // Qué liberar al salir, en orden de declaración.
        std::vector<Liberable> liberables;
    };

    void abrir_ambito() {
        ambitos_.emplace_back();
    }
    void cerrar_ambito() {
        if (ambitos_.empty()) {
            return;
        }
        liberar_cajas(ambitos_.back());
        ambitos_.pop_back();
    }

    /// Libera las cajas de un ámbito, de la última declarada a la primera.
    void liberar_cajas(Ambito& ambito, const std::set<std::string>& conservar = {}) {
        for (auto it = ambito.liberables.rbegin(); it != ambito.liberables.rend(); ++it) {
            if (conservar.count(it->nombre) != 0) {
                continue;  // se devuelve: la propiedad pasa al llamante
            }
            salida_ << indentacion() << it->funcion << "(" << (it->por_puntero ? "" : "&")
                    << it->nombre << ");\n";
        }
        ambito.liberables.clear();
    }

    /// Incrementa la profundidad de expresión mientras dura.
    struct GuardiaProfundidad {
        explicit GuardiaProfundidad(EmisorC& emisor) : emisor_(emisor) {
            ++emisor_.profundidad_expresion_;
        }
        ~GuardiaProfundidad() {
            --emisor_.profundidad_expresion_;
        }
        GuardiaProfundidad(const GuardiaProfundidad&) = delete;
        GuardiaProfundidad& operator=(const GuardiaProfundidad&) = delete;

    private:
        EmisorC& emisor_;
    };

    /// Libera los ámbitos abiertos por encima de `profundidad` (sin cerrarlos:
    /// el cierre normal ya no emitirá nada porque las listas quedan vacías).
    void liberar_desde(std::size_t profundidad) {
        for (std::size_t i = ambitos_.size(); i > profundidad; --i) {
            liberar_cajas(ambitos_[i - 1]);
        }
    }

    /// Antes de un `retornar` hay que liberar todas las cajas vivas.
    void liberar_todo(const std::set<std::string>& conservar = {}) {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            liberar_cajas(*it, conservar);
        }
    }

    /// Nombres que aparecen en una expresión: si se devuelve un compuesto, sus
    /// variables con dueño se transfieren al llamante y no se liberan.
    static void recoger_identificadores(const sintactico::Expresion& expresion,
                                        std::set<std::string>& nombres) {
        if (expresion.tipo == sintactico::TipoExpresion::Identificador) {
            nombres.insert(std::string(expresion.lexema));
        }
        for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
            if (argumento) {
                recoger_identificadores(*argumento, nombres);
            }
        }
        if (expresion.izquierda) {
            recoger_identificadores(*expresion.izquierda, nombres);
        }
        if (expresion.derecha) {
            recoger_identificadores(*expresion.derecha, nombres);
        }
    }

    /// Nombres que viajan dentro del valor devuelto. El objeto de una llamada a
    /// método no viaja: solo sus argumentos y lo que se construye con ellos.
    static void recoger_devueltos(const sintactico::Expresion& expresion,
                                  std::set<std::string>& nombres) {
        if (expresion.tipo == sintactico::TipoExpresion::Identificador) {
            nombres.insert(std::string(expresion.lexema));
        }
        for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
            if (argumento) {
                recoger_devueltos(*argumento, nombres);
            }
        }
        if (expresion.tipo == sintactico::TipoExpresion::Miembro) {
            // `objeto.metodo(...)`: el objeto no se devuelve.
            return;
        }
        if (expresion.izquierda) {
            recoger_devueltos(*expresion.izquierda, nombres);
        }
        if (expresion.derecha) {
            recoger_devueltos(*expresion.derecha, nombres);
        }
    }

    void declarar(std::string_view nombre, std::string tipo) {
        if (ambitos_.empty()) {
            return;
        }
        Ambito& ambito = ambitos_.back();
        ambito.tipos[std::string(nombre)] = std::move(tipo);
    }

    void registrar_liberable(std::string_view nombre, std::string funcion,
                             bool por_puntero = false) {
        if (!ambitos_.empty()) {
            ambitos_.back().liberables.push_back(
                Liberable{std::string(nombre), std::move(funcion), por_puntero});
        }
    }

    /// ¿Queda algo con dueño por liberar en los ámbitos abiertos?
    bool hay_liberables() const {
        for (const Ambito& ambito : ambitos_) {
            if (!ambito.liberables.empty()) {
                return true;
            }
        }
        return false;
    }

    /// Prepara un argumento para un parámetro prestado. Un literal no tiene
    /// dirección: se le da una con un literal compuesto de C.
    std::string argumento_prestado(const sintactico::Expresion& argumento,
                                   std::string_view declarado) {
        std::string valor = expresion(argumento);
        if (declarado.empty() || declarado.front() != '&') {
            return valor;
        }
        std::string_view base = declarado.substr(1);
        if (base.rfind("mutable ", 0) == 0) {
            base = base.substr(8);
        }
        if (argumento.tipo == sintactico::TipoExpresion::Texto) {
            const std::string contenido = "\"" + escapar_c(argumento.lexema) + "\"";
            return "&(nooki_texto){(char*)" + contenido + ", (long long)strlen(" + contenido +
                   "), 0}";
        }
        if (argumento.tipo == sintactico::TipoExpresion::Entero) {
            return "&(" + tipo_declarado_c(base) + "){" + valor + "}";
        }
        return valor;
    }

    bool declarada(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->tipos.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }

    std::string buscar(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            const auto encontrado = it->tipos.find(std::string(nombre));
            if (encontrado != it->tipos.end()) {
                return encontrado->second;
            }
        }
        return "long long";
    }

    // --- Utilidades --------------------------------------------------------
    /// Tipo de C de un tipo declarado: si el nombre corresponde a una estructura
    /// del programa, se usa su `typedef`; si no, el tipo primitivo.
    std::string tipo_declarado_c(std::string_view escrito) const {
        // Un préstamo o un puntero se resuelve sobre el tipo que envuelve, para
        // que una clase conserve su referencia (`&persona` → `nooki_persona*`).
        if (!escrito.empty() && (escrito.front() == '&' || escrito.front() == '*')) {
            std::string_view resto = escrito.substr(1);
            if (resto.rfind("mutable ", 0) == 0) {
                resto = resto.substr(8);
            }
            if (escrito.front() == '&' && es_rasgo_c(resto)) {
                return "nooki_ref_" + std::string(resto);
            }
            // Prestar una clase es prestar el objeto: ya es una referencia.
            const auto definicion = tipo_ptr_.find(std::string(resto));
            if (definicion != tipo_ptr_.end() && definicion->second->es_clase) {
                return tipo_declarado_c(resto);
            }
            return tipo_declarado_c(resto) + "*";
        }

        const auto sustituido = sustitucion_.find(std::string(escrito));
        if (sustituido != sustitucion_.end()) {
            return tipo_declarado_c(sustituido->second);
        }
        const auto conocido = tipo_c_de_.find(std::string(escrito));
        if (conocido != tipo_c_de_.end()) {
            // Una clase es siempre una referencia: el tipo declarado lleva el puntero.
            const auto definicion = tipo_ptr_.find(std::string(escrito));
            const bool es_clase = definicion != tipo_ptr_.end() && definicion->second->es_clase;
            return conocido->second + (es_clase ? "*" : "");
        }
        return tipo_c(escrito);
    }

    std::string indentacion() const {
        return std::string(static_cast<std::size_t>(nivel_) * 4, ' ');
    }

    void no_soportado(std::string descripcion) {
        if (soportado_) {
            soportado_ = false;
            motivo_ = std::move(descripcion);
        }
    }

    // --- Funciones ---------------------------------------------------------
    /// Tipo de C de un parámetro de una función externa. Un `&texto` viaja como
    /// `const char*`, que es lo que espera una función de C.
    static std::string tipo_externo(std::string_view escrito) {
        if (escrito == "&texto" || escrito == "texto") {
            return "const char*";
        }
        // Un préstamo de un tipo escalar es un puntero a ese tipo.
        if (!escrito.empty() && escrito.front() == '&') {
            const std::string dentro = tipo_externo(escrito.substr(1));
            if (!dentro.empty() && dentro != "void") {
                return dentro + "*";
            }
        }
        // Un vector viaja como puntero a su estructura de C.
        if (escrito.rfind("Vector<", 0) == 0) {
            if (escrito.find("flotante") != std::string_view::npos) {
                return "nooki_vector_flotante";
            }
            if (escrito.find("texto") != std::string_view::npos) {
                return "nooki_vector_texto";
            }
            return "nooki_vector_entero";
        }
        if (escrito == "entero") {
            return "long long";
        }
        if (escrito == "booleano") {
            return "int";
        }
        if (escrito == "flotante") {
            return "double";
        }
        if (escrito == "caracter") {
            return "char";
        }
        if (escrito == "Puntero") {
            return "nooki_puntero";
        }
        if (escrito == "vacio") {
            return "void";
        }
        return {};
    }

    /// Declaración de una función de C: `extern long long strlen(const char*);`
    std::string firma_externa(const sintactico::Funcion& funcion) const {
        const std::string retorno = tipo_externo(funcion.tipo_retorno);
        std::string salida = "extern " + (retorno.empty() ? "long long" : retorno) + " " +
                             std::string(funcion.nombre) + "(";
        if (funcion.parametros.empty()) {
            salida += "void";
        } else {
            bool primero = true;
            for (const sintactico::Parametro& parametro : funcion.parametros) {
                if (!primero) {
                    salida += ", ";
                }
                primero = false;
                const std::string tipo = tipo_externo(parametro.tipo);
                salida += tipo.empty() ? "long long" : tipo;
            }
        }
        salida += ");";
        return salida;
    }

    std::string firma(const sintactico::Funcion& funcion) const {
        std::string salida = "static ";
        salida += tipo_declarado_c(funcion.tipo_retorno);
        salida += ' ';
        salida += nombre_funcion_c(funcion);
        salida += '(';

        if (funcion.parametros.empty()) {
            salida += "void";
        } else {
            bool primero = true;
            for (const sintactico::Parametro& parametro : funcion.parametros) {
                if (!primero) {
                    salida += ", ";
                }
                primero = false;
                salida += tipo_declarado_c(parametro.tipo);
                salida += ' ';
                salida += parametro.nombre;
            }
        }

        salida += ')';
        return salida;
    }

    /// Firma de C de un método: el objeto va primero, como puntero.
    std::string firma_metodo(const sintactico::DefinicionTipo& definicion,
                             const sintactico::Funcion& metodo) const {
        const std::string estructura = tipo_estructura(definicion);
        std::string salida = "static ";
        salida += tipo_declarado_c(metodo.tipo_retorno);
        salida += ' ';
        salida += estructura + "_" + std::string(metodo.nombre);
        salida += '(';
        salida += es_virtual(definicion, metodo) ? "void* este_v" : (estructura + "* este");

        for (const sintactico::Parametro& parametro : metodo.parametros) {
            salida += ", ";
            salida += tipo_declarado_c(parametro.tipo);
            salida += ' ';
            salida += parametro.nombre;
        }

        salida += ')';
        return salida;
    }

    void emitir_metodo(const sintactico::DefinicionTipo& definicion,
                       const sintactico::Funcion& metodo) {
        salida_ << firma_metodo(definicion, metodo) << " {\n";

        const std::string anterior = estructura_actual_c_;
        estructura_actual_c_ = tipo_estructura(definicion);
        if (es_virtual(definicion, metodo)) {
            salida_ << "    " << estructura_actual_c_ << "* este = (" << estructura_actual_c_
                    << "*)este_v;\n";
        }

        abrir_ambito();
        ++nivel_;
        for (const sintactico::Parametro& parametro : metodo.parametros) {
            declarar(parametro.nombre, tipo_declarado_c(parametro.tipo));
        }
        for (const sintactico::Sentencia& sentencia : metodo.cuerpo) {
            emitir_sentencia(sentencia, false);
        }
        --nivel_;
        cerrar_ambito();

        estructura_actual_c_ = anterior;
        salida_ << "}\n";
    }

    void emitir_funcion(const sintactico::Funcion& funcion) {
        const bool principal = funcion.nombre == "principal";
        const std::string_view anterior = modulo_actual_;
        modulo_actual_ = funcion.modulo;

        if (principal) {
            if (!funcion.parametros.empty()) {
                no_soportado("una función «principal» con parámetros");
            }
            salida_ << "int main(int argc, char** argv) {\n";
            salida_ << "    NOOKI_ARGC = argc;\n";
            salida_ << "    NOOKI_ARGV = argv;\n";
            retorno_actual_ = "int";
        } else {
            salida_ << firma(funcion) << " {\n";
            retorno_actual_ = tipo_declarado_c(funcion.tipo_retorno);
        }

        abrir_ambito();
        ++nivel_;
        for (const sintactico::Parametro& parametro : funcion.parametros) {
            declarar(parametro.nombre, tipo_declarado_c(parametro.tipo));
        }
        for (const sintactico::Sentencia& sentencia : funcion.cuerpo) {
            emitir_sentencia(sentencia, principal);
        }
        --nivel_;

        const bool termina_en_retorno =
            !funcion.cuerpo.empty() &&
            funcion.cuerpo.back().tipo == sintactico::TipoSentencia::Retornar;
        if (principal && !termina_en_retorno) {
            liberar_todo();
            salida_ << indentacion() << "return 0;\n";
        }
        cerrar_ambito();
        salida_ << "}\n";
        modulo_actual_ = anterior;
    }

    // --- Sentencias --------------------------------------------------------
    void emitir_cuerpo(const std::vector<sintactico::Sentencia>& cuerpo, bool principal = false) {
        abrir_ambito();
        ++nivel_;
        for (const sintactico::Sentencia& sentencia : cuerpo) {
            emitir_sentencia(sentencia, principal);
        }
        --nivel_;
        cerrar_ambito();
    }

    void emitir_sentencia(const sintactico::Sentencia& sentencia, bool principal) {
        using sintactico::TipoSentencia;

        switch (sentencia.tipo) {
        case TipoSentencia::Expresion:
            if (sentencia.expresion) {
                salida_ << indentacion() << expresion(*sentencia.expresion) << ";\n";
                emitir_movimientos_de_llamada(*sentencia.expresion);
            }
            return;

        case TipoSentencia::Retornar: {
            // Devolver un valor con dueño lo mueve: no se libera aquí, ni él
            // ni las variables que aparezcan dentro de un compuesto devuelto.
            // Lo que viaja dentro del valor devuelto se conserva; el objeto de
            // una llamada a método, no: `v.largo()` devuelve un número, no `v`,
            // así que `v` se libera.
            std::set<std::string> conservar;
            if (sentencia.expresion) {
                recoger_devueltos(*sentencia.expresion, conservar);
            }
            // Si hay algo que liberar y la expresión usa esos valores (por
            // ejemplo `v.largo()`), se evalúa primero en un temporal: liberar
            // antes sería leer memoria liberada.
            const bool quedan_liberables = hay_liberables();
            if (sentencia.expresion && quedan_liberables) {
                const std::string temporal =
                    "__nooki_retorno" + std::to_string(contador_temporal_++);
                const std::string tipo_retorno =
                    (retorno_actual_.empty() || retorno_actual_ == "void") ? "long long"
                                                                           : retorno_actual_;
                salida_ << indentacion() << tipo_retorno << " " << temporal << " = "
                        << expresion(*sentencia.expresion) << ";\n";
                liberar_todo(conservar);
                salida_ << indentacion() << "return " << temporal << ";\n";
                return;
            }
            liberar_todo(conservar);
            if (sentencia.expresion) {
                salida_ << indentacion() << "return " << expresion(*sentencia.expresion) << ";\n";
            } else {
                salida_ << indentacion() << (principal ? "return 0;\n" : "return;\n");
            }
            return;
        }

        case TipoSentencia::Variable:
        case TipoSentencia::Constante:
            emitir_declaracion(sentencia);
            return;

        case TipoSentencia::Si:
            emitir_si(sentencia);
            return;

        case TipoSentencia::Mientras:
            salida_ << indentacion() << "while (" << condicion(sentencia.expresion.get())
                    << ") {\n";
            bucles_.push_back(ambitos_.size());
            emitir_cuerpo(sentencia.cuerpo);
            bucles_.pop_back();
            salida_ << indentacion() << "}\n";
            return;

        case TipoSentencia::Para:
            emitir_para(sentencia);
            return;

        case TipoSentencia::Hacer:
            salida_ << indentacion() << "do {\n";
            bucles_.push_back(ambitos_.size());
            emitir_cuerpo(sentencia.cuerpo);
            bucles_.pop_back();
            salida_ << indentacion() << "} while (" << condicion(sentencia.paso.get()) << ");\n";
            return;

        case TipoSentencia::Bucle:
            salida_ << indentacion() << "for (;;) {\n";
            emitir_cuerpo(sentencia.cuerpo);
            salida_ << indentacion() << "}\n";
            return;

        case TipoSentencia::Romper:
            if (!bucles_.empty()) {
                liberar_desde(bucles_.back());
            }
            salida_ << indentacion() << "break;\n";
            return;

        case TipoSentencia::Continuar:
            if (!bucles_.empty()) {
                liberar_desde(bucles_.back());
            }
            salida_ << indentacion() << "continue;\n";
            return;

        case TipoSentencia::Asm:
            // MSVC no admite ensamblador en línea en x64: se avisa al compilar.
            salida_ << "#if defined(_MSC_VER)\n";
            salida_ << indentacion() << "#error \"asm! necesita GCC, Clang o MinGW\"\n";
            salida_ << "#else\n";
            salida_ << indentacion() << "__asm__ __volatile__(\"" << escapar_c(sentencia.nombre)
                    << "\");\n";
            salida_ << "#endif\n";
            return;

        case TipoSentencia::Inseguro: {
            // Dentro de un bloque `inseguro` la aritmética no se comprueba.
            const bool anterior = sin_comprobacion_;
            sin_comprobacion_ = true;
            salida_ << indentacion() << "{\n";
            emitir_cuerpo(sentencia.cuerpo);
            salida_ << indentacion() << "}\n";
            sin_comprobacion_ = anterior;
            return;
        }

        case TipoSentencia::Bloque:
            salida_ << indentacion() << "{\n";
            emitir_cuerpo(sentencia.cuerpo);
            salida_ << indentacion() << "}\n";
            return;

        case TipoSentencia::Coincide:
            emitir_coincide(sentencia);
            return;
        }
    }

    void emitir_declaracion(const sintactico::Sentencia& sentencia) {
        if (!sentencia.expresion) {
            no_soportado("una declaración sin valor inicial");
            return;
        }

        // `variable x = f()?`: si la variante no es la del valor, se propaga.
        if (sentencia.expresion->tipo == sintactico::TipoExpresion::Propagacion &&
            sentencia.expresion->izquierda) {
            const std::string tipo_resultado = tipo_de(*sentencia.expresion->izquierda);
            const auto enumeracion =
                enumeraciones_.find(tipo_resultado.rfind("nooki_", 0) == 0
                                        ? tipo_resultado.substr(std::string("nooki_").size())
                                        : tipo_resultado);
            if (enumeracion == enumeraciones_.end() || enumeracion->second->variantes.size() < 2 ||
                enumeracion->second->variantes[0].campos.size() != 1) {
                no_soportado("«?» sobre un valor que no tiene forma de resultado");
                return;
            }
            const sintactico::Variante& exito = enumeracion->second->variantes[0];
            const std::string campo_exito = tipo_declarado_c(exito.campos[0].tipo);
            const std::string temporal = "__nooki_r" + std::to_string(contador_temporal_++);

            salida_ << indentacion() << tipo_resultado << " " << temporal << " = "
                    << expresion(*sentencia.expresion->izquierda) << ";\n";
            salida_ << indentacion() << "if (" << temporal << ".etiqueta != 0) {\n";
            salida_ << indentacion() << "    return " << temporal << ";\n";
            salida_ << indentacion() << "}\n";
            salida_ << indentacion() << campo_exito << " " << sentencia.nombre << " = " << temporal
                    << ".datos." << exito.nombre << "_" << exito.campos[0].nombre << ";\n";
            declarar(sentencia.nombre, campo_exito);
            return;
        }

        const std::string tipo = sentencia.tipo_declarado.empty()
                                     ? tipo_de(*sentencia.expresion)
                                     : tipo_declarado_c(sentencia.tipo_declarado);

        if (sentencia.tipo == sintactico::TipoSentencia::Constante) {
            salida_ << indentacion() << "const ";
        } else {
            salida_ << indentacion();
        }

        // Un puntero derivado puede guardarse en una variable de tipo base: se
        // convierte explícitamente, porque la estructura base está al principio.
        std::string valor = expresion(*sentencia.expresion);
        const std::string tipo_expresion = tipo_de(*sentencia.expresion);
        if (tipo != tipo_expresion && !tipo.empty() && tipo.back() == '*' &&
            !tipo_expresion.empty() && tipo_expresion.back() == '*') {
            valor = "(" + tipo + ")" + valor;
        }

        // `variable d: &Rasgo = &objeto`: se guarda el objeto y su tabla.
        if (tipo.rfind("nooki_ref_", 0) == 0 &&
            sentencia.expresion->tipo == sintactico::TipoExpresion::Unaria &&
            sentencia.expresion->lexema == "&" && sentencia.expresion->izquierda) {
            const std::string tipo_interno = tipo_de(*sentencia.expresion->izquierda);
            std::string tipo_objeto = tipo_interno;
            const bool ya_es_puntero = !tipo_objeto.empty() && tipo_objeto.back() == '*';
            if (ya_es_puntero) {
                tipo_objeto.pop_back();
            }
            // Una clase ya es una referencia: se guarda tal cual. Algo por valor
            // necesita que se tome su dirección.
            const std::string objeto = ya_es_puntero
                                           ? expresion(*sentencia.expresion->izquierda)
                                           : "&" + expresion(*sentencia.expresion->izquierda);
            const std::string rasgo = tipo.substr(std::string("nooki_ref_").size());
            valor = "(" + tipo + "){ .objeto = (void*)" + objeto + ", .tabla = &" + tipo_objeto +
                    "_" + rasgo + "_tabla }";
        }

        // `variable x = *prestamo` copia: compartir el búfer del prestamista haría
        // que los dos lo liberaran.
        std::string valor_inicial = valor;
        const bool copia_de_prestamo =
            tipo == "nooki_texto" &&
            sentencia.expresion->tipo == sintactico::TipoExpresion::Unaria &&
            sentencia.expresion->lexema == "*" && sentencia.expresion->izquierda &&
            tipo_de(*sentencia.expresion->izquierda) == "nooki_texto*";
        if (copia_de_prestamo) {
            valor_inicial = "nooki_texto_copiar(" + valor + ")";
        }
        salida_ << tipo << ' ' << sentencia.nombre << " = " << valor_inicial << ";\n";

        // Una construcción con variables con dueño se queda con ellas.
        if (sentencia.expresion->tipo == sintactico::TipoExpresion::Construccion) {
            std::set<std::string> nombres;
            recoger_identificadores(*sentencia.expresion, nombres);
            for (const std::string& nombre : nombres) {
                anular_variable(nombre);
            }
        }

        emitir_movimientos_de_llamada(*sentencia.expresion);

        const bool referencia_de_clase = !tipo.empty() && tipo.back() == '*';
        const bool compuesto_fresco =
            compuestos_con_dueno_.count(tipo) != 0 &&
            (sentencia.expresion->tipo == sintactico::TipoExpresion::Construccion ||
             sentencia.expresion->tipo == sintactico::TipoExpresion::Llamada);

        if (tipo == "nooki_caja" || tipo == "nooki_texto" || tipo.rfind("nooki_vector_", 0) == 0 ||
            tipo.rfind("nooki_mapa_", 0) == 0 || compuesto_fresco || referencia_de_clase) {
            // Asignar un valor con dueño transfiere la propiedad: el origen queda vacío.
            if (sentencia.expresion->tipo == sintactico::TipoExpresion::Identificador) {
                const std::string& origen = std::string(sentencia.expresion->lexema);
                if (referencia_de_clase) {
                    salida_ << indentacion() << origen << " = NULL;\n";
                } else {
                    salida_ << indentacion() << origen << ".datos = NULL;\n";
                    if (tipo == "nooki_texto") {
                        salida_ << indentacion() << origen << ".propio = 0;\n";
                    }
                }
            }
            std::string liberar = "nooki_texto_soltar";
            if (tipo == "nooki_caja") {
                liberar = "nooki_caja_soltar";
            } else if (referencia_de_clase) {
                liberar = tipo.substr(0, tipo.size() - 1) + "_soltar";
            } else if (tipo.rfind("nooki_vector_", 0) == 0 || tipo.rfind("nooki_mapa_", 0) == 0 ||
                       compuesto_fresco) {
                liberar = tipo + "_soltar";
            }
            registrar_liberable(sentencia.nombre, std::move(liberar), referencia_de_clase);
        }

        declarar(sentencia.nombre, tipo);
    }

    void emitir_si(const sintactico::Sentencia& sentencia) {
        salida_ << indentacion() << "if (" << condicion(sentencia.expresion.get()) << ") {\n";
        emitir_cuerpo(sentencia.cuerpo);
        salida_ << indentacion() << "}";
        if (!sentencia.sino.empty()) {
            salida_ << " else {\n";
            emitir_cuerpo(sentencia.sino);
            salida_ << indentacion() << "}\n";
        } else {
            salida_ << '\n';
        }
    }

    void emitir_para(const sintactico::Sentencia& sentencia) {
        const sintactico::Expresion* rango = sentencia.expresion.get();
        if (rango == nullptr) {
            no_soportado("un «para» sin nada que recorrer");
            return;
        }

        // ¿Recorre una colección? `para (n en numeros)`.
        if (rango->tipo == sintactico::TipoExpresion::Identificador) {
            const std::string tipo = tipo_de(*rango);
            if (tipo.rfind("nooki_vector_", 0) == 0) {
                emitir_para_coleccion(sentencia, tipo);
                return;
            }
        }

        if (rango->tipo != sintactico::TipoExpresion::Binaria ||
            (rango->lexema != ".." && rango->lexema != "..=")) {
            no_soportado("un «para» que no recorre un rango ni una colección");
            return;
        }

        const bool inclusivo = rango->lexema == "..=";
        const std::string paso = sentencia.paso ? expresion(*sentencia.paso) : "1";

        salida_ << indentacion() << "for (long long " << sentencia.nombre << " = "
                << expresion(*rango->izquierda) << "; " << sentencia.nombre
                << (inclusivo ? " <= " : " < ") << expresion(*rango->derecha) << "; "
                << sentencia.nombre << " += " << paso << ") {\n";

        abrir_ambito();
        declarar(sentencia.nombre, "long long");
        ++nivel_;
        bucles_.push_back(ambitos_.size());
        for (const sintactico::Sentencia& hija : sentencia.cuerpo) {
            emitir_sentencia(hija, false);
        }
        bucles_.pop_back();
        --nivel_;
        cerrar_ambito();
        salida_ << indentacion() << "}\n";
    }

    /// `para (n en numeros)` → recorrido por índice con una variable interna.
    void emitir_para_coleccion(const sintactico::Sentencia& sentencia, const std::string& tipo) {
        const std::string nombre(sentencia.nombre);
        const std::string elemento = elemento_de_vector(tipo);
        const std::string indice = "__nooki_i" + std::to_string(contador_temporal_++);
        const std::string coleccion = expresion(*sentencia.expresion);

        // Si la colección llega como préstamo (`&Vector<T>`), ya es un puntero.
        const bool es_puntero = tipo.back() == '*';
        const std::string tipo_base = es_puntero ? tipo.substr(0, tipo.size() - 1) : tipo;

        salida_ << indentacion() << "{\n";
        ++nivel_;
        if (es_puntero) {
            salida_ << indentacion() << tipo << " __nooki_col = " << coleccion << ";\n";
        } else {
            salida_ << indentacion() << tipo << "* __nooki_col = &" << coleccion << ";\n";
        }
        salida_ << indentacion() << "for (long long " << indice << " = 0; " << indice << " < "
                << tipo_base << "_largo(__nooki_col); ++" << indice << ") {\n";
        ++nivel_;

        // Si el bucle pide el índice, se declara con ese nombre.
        if (!sentencia.indice.empty()) {
            salida_ << indentacion() << "long long " << sentencia.indice << " = " << indice
                    << ";\n";
        }
        salida_ << indentacion() << elemento << " " << nombre << " = " << tipo_base
                << "_obtener(__nooki_col, " << indice << ");\n";

        abrir_ambito();
        declarar(sentencia.nombre, elemento);
        if (!sentencia.indice.empty()) {
            declarar(sentencia.indice, "long long");
        }
        for (const sintactico::Sentencia& hija : sentencia.cuerpo) {
            emitir_sentencia(hija, false);
        }
        cerrar_ambito();

        --nivel_;
        salida_ << indentacion() << "}\n";
        --nivel_;
        salida_ << indentacion() << "}\n";
    }

    std::string condicion(const sintactico::Expresion* expresion_condicion) {
        if (expresion_condicion == nullptr) {
            no_soportado("una condición vacía");
            return "0";
        }
        return expresion(*expresion_condicion);
    }

    // --- Tipos de las expresiones -----------------------------------------
    std::string tipo_de(const sintactico::Expresion& expresion) const {
        using sintactico::TipoExpresion;

        switch (expresion.tipo) {
        case TipoExpresion::Texto:
            return "nooki_texto";
        case TipoExpresion::Entero:
            return "long long";
        case TipoExpresion::Flotante:
            return "double";
        case TipoExpresion::Booleano:
            return "int";
        case TipoExpresion::Nulo:
            return "nooki_nulo";
        case TipoExpresion::Identificador:
            return buscar(expresion.lexema);
        case TipoExpresion::Construccion: {
            const auto conocido = tipo_c_de_.find(std::string(expresion.anotacion));
            const std::string tipo = conocido != tipo_c_de_.end()
                                         ? conocido->second
                                         : tipo_estructura(expresion.anotacion);
            return expresion.monticulo ? tipo + "*" : tipo;
        }
        case TipoExpresion::Propagacion:
            return "long long";
        case TipoExpresion::Miembro: {
            if (!expresion.izquierda) {
                return "long long";
            }
            if (expresion.izquierda->tipo == TipoExpresion::Identificador) {
                const auto enumeracion =
                    enumeraciones_.find(std::string(expresion.izquierda->lexema));
                if (enumeracion != enumeraciones_.end()) {
                    return "nooki_" + std::string(expresion.izquierda->lexema);
                }
            }
            std::string objeto = tipo_de(*expresion.izquierda);
            if (!objeto.empty() && objeto.back() == '*') {
                objeto.pop_back();  // una clase por referencia
            }
            constexpr std::string_view PREFIJO = "nooki_";
            if (objeto.rfind(PREFIJO, 0) != 0) {
                return "long long";
            }
            const std::string nombre_estructura = objeto;

            const auto tipo = campos_.find(nombre_estructura);
            if (tipo != campos_.end()) {
                const auto campo = tipo->second.find(expresion.anotacion);
                if (campo != tipo->second.end()) {
                    return campo->second;
                }
            }

            const auto metodos = metodos_.find(nombre_estructura);
            if (metodos != metodos_.end()) {
                const auto metodo = metodos->second.find(expresion.anotacion);
                if (metodo != metodos->second.end()) {
                    return tipo_declarado_c(metodo->second->tipo_retorno);
                }
            }
            return "long long";
        }
        case TipoExpresion::Este:
            return estructura_actual_c_.empty() ? "long long" : estructura_actual_c_;
        case TipoExpresion::Llamada: {
            if (!expresion.izquierda) {
                return "void";
            }
            if (expresion.izquierda->tipo == TipoExpresion::Identificador &&
                !expresion.tipos_aplicados.empty()) {
                const auto generico = genericos_.find(std::string(expresion.izquierda->lexema));
                if (generico != genericos_.end()) {
                    const sintactico::Funcion& funcion = *generico->second;
                    for (std::size_t i = 0; i < funcion.tipo_parametros.size(); ++i) {
                        if (i < expresion.tipos_aplicados.size() &&
                            funcion.tipo_retorno == funcion.tipo_parametros[i]) {
                            return tipo_declarado_c(expresion.tipos_aplicados[i]);
                        }
                    }
                    return tipo_declarado_c(funcion.tipo_retorno);
                }
            }
            if (expresion.izquierda->tipo == TipoExpresion::Miembro) {
                const sintactico::Expresion& acceso = *expresion.izquierda;

                // `&Rasgo`: el tipo del resultado lo da el rasgo.
                if (acceso.izquierda) {
                    const std::string tipo_ref = tipo_de(*acceso.izquierda);
                    if (tipo_ref.rfind("nooki_ref_", 0) == 0) {
                        const auto rasgo =
                            rasgos_def_.find(tipo_ref.substr(std::string("nooki_ref_").size()));
                        if (rasgo != rasgos_def_.end()) {
                            for (const sintactico::Funcion& metodo : rasgo->second->metodos) {
                                if (metodo.nombre == acceso.anotacion) {
                                    return tipo_declarado_c(metodo.tipo_retorno);
                                }
                            }
                        }
                        return "long long";
                    }
                }

                // `super.metodo(...)`: el tipo lo da la clase base.
                if (acceso.izquierda && acceso.izquierda->tipo == TipoExpresion::Super) {
                    std::string donde = estructura_actual_c_;
                    for (int guarda = 0; guarda < 32; ++guarda) {
                        const auto base = base_de_.find(donde);
                        if (base == base_de_.end()) {
                            break;
                        }
                        donde = base->second;
                        const auto metodos = metodos_.find(donde);
                        if (metodos != metodos_.end()) {
                            const auto metodo = metodos->second.find(acceso.anotacion);
                            if (metodo != metodos->second.end()) {
                                return tipo_declarado_c(metodo->second->tipo_retorno);
                            }
                        }
                    }
                }

                if (acceso.izquierda && (tipo_de(*acceso.izquierda) == "nooki_texto" ||
                                         tipo_de(*acceso.izquierda) == "nooki_texto*")) {
                    const std::string& metodo = acceso.anotacion;
                    if (metodo == "dividir") {
                        return "nooki_vector_texto";
                    }
                    if (metodo == "subtexto" || metodo == "mayusculas" || metodo == "minusculas" ||
                        metodo == "recortar" || metodo == "sin_ultimo" ||
                        metodo == "agregar_codigo") {
                        return "nooki_texto";
                    }
                    if (metodo == "contiene" || metodo == "empieza_con") {
                        return "int";
                    }
                    if (metodo == "a_flotante") {
                        return "double";
                    }
                    return "long long";
                }
                if (acceso.izquierda && tipo_de(*acceso.izquierda).rfind("nooki_vector_", 0) == 0) {
                    if (acceso.anotacion == "agregar" || acceso.anotacion == "modificar" ||
                        acceso.anotacion == "vaciar") {
                        return "void";
                    }
                    // El elemento depende del vector: texto, flotante o entero.
                    if (acceso.anotacion == "obtener") {
                        return elemento_de_vector(tipo_de(*acceso.izquierda));
                    }
                    return "long long";
                }
                // Una variable de tipo `Caja<entero>` o `Caja<texto>`.
                if (acceso.izquierda && tipo_de(*acceso.izquierda).rfind("nooki_caja", 0) == 0) {
                    if (acceso.anotacion == "poner") {
                        return "void";
                    }
                    if (acceso.anotacion == "sacar") {
                        return tipo_de(*acceso.izquierda).find("texto") != std::string::npos
                                   ? "nooki_texto"
                                   : "long long";
                    }
                    // Cualquier otro método sigue su camino: puede ser de una
                    // clase del usuario que también se llama Caja.
                }
                // Una variable de tipo mapa.
                if (acceso.izquierda && tipo_de(*acceso.izquierda).rfind("nooki_mapa_", 0) == 0) {
                    if (acceso.anotacion == "insertar") {
                        return "void";
                    }
                    // `obtener` devuelve el tipo del valor del mapa.
                    if (acceso.anotacion == "obtener") {
                        if (tipo_de(*acceso.izquierda).find("flotante") != std::string::npos) {
                            return "double";
                        }
                        if (tipo_de(*acceso.izquierda).rfind("nooki_mapa_texto_texto", 0) == 0) {
                            return "nooki_texto";
                        }
                    }
                    // `quitar` dice si quitó algo: 1 o 0.
                    if (acceso.anotacion == "quitar") {
                        return "int";
                    }
                    if (acceso.anotacion == "contiene") {
                        return "int";
                    }
                    if (acceso.anotacion == "obtener") {
                        return tipo_de(*acceso.izquierda).find("texto_entero") != std::string::npos
                                   ? "long long"
                                   : "nooki_texto";
                    }
                    // `claves()` y lo demás lo resuelve el código de más abajo.
                }
                if (acceso.izquierda && acceso.izquierda->tipo == TipoExpresion::Identificador &&
                    acceso.izquierda->lexema == "Caja") {
                    return "nooki_caja";
                }
                if (acceso.izquierda && acceso.izquierda->tipo == TipoExpresion::Identificador) {
                    const auto modulo = modulos_.find(acceso.izquierda->lexema);
                    if (modulo != modulos_.end()) {
                        const auto funcion = modulo->second.find(acceso.anotacion);
                        if (funcion != modulo->second.end()) {
                            return tipo_declarado_c(funcion->second->tipo_retorno);
                        }
                    }
                }
                if (acceso.izquierda && acceso.izquierda->tipo == TipoExpresion::Identificador &&
                    acceso.izquierda->lexema == "Mapa") {
                    // `Mapa<texto, T>.nuevo()`: el valor manda.
                    const std::string_view valor = acceso.izquierda->tipos_aplicados.size() == 2
                                                       ? acceso.izquierda->tipos_aplicados[1]
                                                       : std::string_view{};
                    if (valor == "flotante") {
                        return "nooki_mapa_texto_flotante";
                    }
                    if (valor == "texto") {
                        return "nooki_mapa_texto_texto";
                    }
                    return "nooki_mapa_texto_entero";
                }
                if (acceso.izquierda && acceso.izquierda->tipo == TipoExpresion::Identificador &&
                    acceso.izquierda->lexema == "Vector") {
                    // `Vector<T>.nuevo()`: el tipo del vector sale del tipo aplicado.
                    const std::string_view elemento =
                        acceso.izquierda->tipos_aplicados.empty()
                            ? std::string_view{}
                            : acceso.izquierda->tipos_aplicados.front();
                    if (elemento == "texto") {
                        return "nooki_vector_texto";
                    }
                    if (elemento == "flotante") {
                        return "nooki_vector_flotante";
                    }
                    if (!elemento.empty() &&
                        vectores_de_estructura_.count(std::string(elemento)) != 0) {
                        return "nooki_vector_" + std::string(elemento);
                    }
                    return "nooki_vector_entero";
                }
                if (acceso.izquierda && tipo_de(*acceso.izquierda).rfind("nooki_mapa_", 0) == 0) {
                    if (acceso.anotacion == "insertar") {
                        return "void";
                    }
                    // `obtener` devuelve el tipo del valor del mapa.
                    if (acceso.anotacion == "obtener") {
                        if (tipo_de(*acceso.izquierda).find("flotante") != std::string::npos) {
                            return "double";
                        }
                        if (tipo_de(*acceso.izquierda).rfind("nooki_mapa_texto_texto", 0) == 0) {
                            return "nooki_texto";
                        }
                    }
                    if (acceso.anotacion == "claves") {
                        return "nooki_vector_texto";
                    }
                    return "long long";
                }
                if (acceso.izquierda && tipo_de(*acceso.izquierda).rfind("nooki_vector_", 0) == 0) {
                    if (acceso.anotacion == "agregar" || acceso.anotacion == "modificar" ||
                        acceso.anotacion == "vaciar") {
                        return "void";
                    }
                    // El elemento depende del vector: texto, flotante o entero.
                    if (acceso.anotacion == "obtener") {
                        return elemento_de_vector(tipo_de(*acceso.izquierda));
                    }
                    return "long long";
                }
                std::string tipo_objeto =
                    acceso.izquierda ? tipo_de(*acceso.izquierda) : std::string{};
                if (!tipo_objeto.empty() && tipo_objeto.back() == '*') {
                    tipo_objeto.pop_back();  // una clase por referencia
                }
                constexpr std::string_view PREFIJO = "nooki_";
                if (tipo_objeto.rfind(PREFIJO, 0) == 0) {
                    // Un método puede venir de una clase base.
                    std::string donde = tipo_objeto;
                    for (int guarda = 0; guarda < 32; ++guarda) {
                        const auto metodos = metodos_.find(donde);
                        if (metodos != metodos_.end()) {
                            const auto metodo = metodos->second.find(acceso.anotacion);
                            if (metodo != metodos->second.end()) {
                                return tipo_declarado_c(metodo->second->tipo_retorno);
                            }
                        }
                        const auto base = base_de_.find(donde);
                        if (base == base_de_.end()) {
                            break;
                        }
                        donde = base->second;
                    }
                }
                return "void";
            }
            if (expresion.izquierda->tipo == TipoExpresion::Identificador) {

                if (const auto variante = variantes_.find(std::string(expresion.izquierda->lexema));
                    variante != variantes_.end()) {
                    return "nooki_" + std::string(variante->second.first->nombre);
                }
                if (expresion.izquierda->lexema == "leer_linea" ||
                    expresion.izquierda->lexema == "leer_archivo") {
                    return "nooki_texto";
                }
                if (expresion.izquierda->lexema == "escribir_archivo" ||
                    expresion.izquierda->lexema == "agregar_archivo" ||
                    expresion.izquierda->lexema == "existe_archivo") {
                    return "int";
                }
                if (expresion.izquierda->lexema == "leer_entero") {
                    return "long long";
                }
                // Punteros: cada operación devuelve lo suyo.
                {
                    const std::string_view nombre = expresion.izquierda->lexema;
                    if (nombre == "reservar" || nombre == "desde_direccion") {
                        return "nooki_puntero";
                    }
                    if (nombre == "direccion_de" || nombre == "leer_en") {
                        return "long long";
                    }
                    if (nombre == "leer_flotante_en") {
                        return "double";
                    }
                    if (nombre == "liberar" || nombre == "escribir_en" ||
                        nombre == "escribir_flotante_en") {
                        return "void";
                    }
                }
                // Integradas que no son de punteros.
                {
                    const std::string_view nombre = expresion.izquierda->lexema;
                    if (nombre == "cuantos_argumentos_programa" || nombre == "tabla_de_consulta" ||
                        nombre == "filas_de_tabla" || nombre == "columnas_de_tabla" ||
                        nombre == "entero_de_tabla") {
                        return "long long";
                    }
                    if (nombre == "argumento_de" || nombre == "argumentos_programa") {
                        return nombre == "argumento_de" ? "nooki_texto" : "nooki_vector_texto";
                    }
                    if (nombre == "valor_de_tabla" || nombre == "nombre_de_columna" ||
                        nombre == "consultar_orden" || nombre == "version_datos" ||
                        nombre == "error_datos") {
                        return "nooki_texto";
                    }
                    if (nombre == "numero_de_tabla") {
                        return "double";
                    }
                    if (nombre == "agregar_columna_a_tabla" ||
                        nombre == "indice_de_columna_en_tabla") {
                        return "long long";
                    }
                    if (nombre == "liberar_tabla") {
                        return "void";
                    }
                }
                const auto encontrada = retornos_.find(std::string(expresion.izquierda->lexema));
                if (encontrada != retornos_.end()) {
                    return encontrada->second;
                }
            }
            return "void";
        }
        case TipoExpresion::Unaria: {
            if (expresion.lexema == "!") {
                return "int";
            }
            // `&x` es un puntero a lo que sea x.
            if ((expresion.lexema == "&" || expresion.lexema == "&mutable") &&
                expresion.izquierda) {
                return tipo_de(*expresion.izquierda) + "*";
            }
            // `*prestamo` de texto devuelve el texto prestado.
            // `*prestamo` da el tipo apuntado, sea texto, vector o mapa.
            if (expresion.lexema == "*" && expresion.izquierda) {
                const std::string apuntado = tipo_de(*expresion.izquierda);
                if (!apuntado.empty() && apuntado.back() == '*') {
                    return apuntado.substr(0, apuntado.size() - 1);
                }
            }
            if (expresion.izquierda && tipo_de(*expresion.izquierda) == "double") {
                return "double";
            }
            return "long long";
        }
        case TipoExpresion::Binaria: {
            const std::string_view operador = expresion.lexema;
            const bool comparacion = operador == "==" || operador == "!=" || operador == "<" ||
                                     operador == ">" || operador == "<=" || operador == ">=" ||
                                     operador == "&&" || operador == "||";
            if (!comparacion && expresion.izquierda && expresion.derecha &&
                expresion.lexema == "+" && tipo_de(*expresion.izquierda) == "nooki_texto" &&
                tipo_de(*expresion.derecha) == "nooki_texto") {
                return "nooki_texto";
            }
            if (comparacion) {
                return "int";
            }
            // Si algún operando es flotante, el resultado también lo es.
            if ((expresion.izquierda && tipo_de(*expresion.izquierda) == "double") ||
                (expresion.derecha && tipo_de(*expresion.derecha) == "double")) {
                return "double";
            }
            return "long long";
        }
        case TipoExpresion::Como:
            return tipo_c(expresion.anotacion);
        case TipoExpresion::Asignacion:
            return expresion.izquierda ? tipo_de(*expresion.izquierda) : "long long";
        default:
            return "long long";
        }
    }

    // --- Expresiones -------------------------------------------------------
    std::string expresion(const sintactico::Expresion& nodo) {
        using sintactico::TipoExpresion;

        const GuardiaProfundidad guardia(*this);
        switch (nodo.tipo) {
        case TipoExpresion::Texto:
            return "nooki_texto_literal(\"" + escapar_c(desescapar(nodo.lexema)) + "\")";
        case TipoExpresion::Entero:
            return limpiar_separadores(nodo.lexema);
        case TipoExpresion::Flotante:
            return limpiar_separadores(nodo.lexema);
        case TipoExpresion::Como:
            return conversion(nodo);
        case TipoExpresion::Booleano:
            return nodo.lexema == "verdadero" ? "1" : "0";
        case TipoExpresion::Nulo:
            return "NULL";
        case TipoExpresion::Identificador:
            return std::string(nodo.lexema);
        case TipoExpresion::Construccion:
            return construccion(nodo);
        case TipoExpresion::Este:
            return "(*este)";
        case TipoExpresion::Miembro:
            return miembro(nodo);
        case TipoExpresion::Unaria:
            return unaria(nodo);
        case TipoExpresion::Binaria:
            return binaria(nodo);
        case TipoExpresion::Asignacion:
            return asignacion(nodo);
        case TipoExpresion::Llamada:
            return llamada(nodo);
        default:
            break;
        }

        no_soportado(descripcion_expresion(nodo.tipo));
        return "0";
    }

    /// «expr como tipo»: de entero a flotante y de flotante a entero.
    std::string conversion(const sintactico::Expresion& nodo) {
        if (!nodo.izquierda) {
            no_soportado("una conversión sin valor");
            return "0";
        }
        const std::string valor = expresion(*nodo.izquierda);
        const std::string destino = tipo_c(nodo.anotacion);
        return "((" + destino + ")(" + valor + "))";
    }

    std::string construccion(const sintactico::Expresion& nodo) {
        const auto conocido = tipo_c_de_.find(std::string(nodo.anotacion));
        const std::string tipo =
            conocido != tipo_c_de_.end() ? conocido->second : tipo_estructura(nodo.anotacion);

        // `nuevo persona { ... }`: se reserva y se rellenan los campos en orden de
        // declaración, porque la función de reserva recibe todos los valores.
        if (nodo.monticulo) {
            std::string salida = tipo + "_nuevo(";
            const auto planos = campos_planos_.find(tipo);
            if (planos != campos_planos_.end()) {
                bool primero = true;
                for (const auto& [campo, tipo_campo] : planos->second) {
                    if (!primero) {
                        salida += ", ";
                    }
                    primero = false;
                    std::string valor = "0";
                    for (std::size_t i = 0; i < nodo.nombres_campo.size(); ++i) {
                        if (nodo.nombres_campo[i] == campo && i < nodo.lista.size() &&
                            nodo.lista[i]) {
                            valor = expresion(*nodo.lista[i]);
                            break;
                        }
                    }
                    salida += valor;
                }
            }
            salida += ')';
            return salida;
        }

        std::string salida = "(" + tipo + "){ ";
        for (std::size_t i = 0; i < nodo.lista.size(); ++i) {
            if (i != 0) {
                salida += ", ";
            }
            salida += '.';
            salida += i < nodo.nombres_campo.size() ? nodo.nombres_campo[i] : "?";
            salida += " = ";
            salida += nodo.lista[i] ? expresion(*nodo.lista[i]) : "0";
        }
        salida += " }";
        return salida;
    }

    /// `coincide` sobre una enumeración con datos: se compara la etiqueta y se
    /// extraen los campos ligados por el patrón.
    void emitir_coincide_datos(const sintactico::Sentencia& sentencia,
                               const sintactico::Enumeracion& enumeracion) {
        const std::string tipo = "nooki_" + std::string(enumeracion.nombre);
        const std::string temporal = "__nooki_v" + std::to_string(contador_temporal_++);

        salida_ << indentacion() << "{\n";
        ++nivel_;
        salida_ << indentacion() << tipo << " " << temporal << " = "
                << expresion(*sentencia.expresion) << ";\n";

        bool primero = true;
        for (const sintactico::Rama& rama : sentencia.ramas) {
            const sintactico::Variante* variante = nullptr;
            std::size_t indice = 0;
            if (rama.patron.clase == sintactico::Patron::Clase::Comodin) {
                salida_ << indentacion() << (primero ? "{" : "else {") << "\n";
            } else {
                for (std::size_t i = 0; i < enumeracion.variantes.size(); ++i) {
                    if (enumeracion.variantes[i].nombre == rama.patron.valor) {
                        variante = &enumeracion.variantes[i];
                        indice = i;
                        break;
                    }
                }
                if (variante == nullptr) {
                    no_soportado("un patrón que no es variante de la enumeración");
                    break;
                }
                salida_ << indentacion() << (primero ? "if" : "else if") << " (" << temporal
                        << ".etiqueta == " << indice << ") {\n";
            }
            primero = false;

            ++nivel_;
            abrir_ambito();
            if (variante != nullptr) {
                for (std::size_t i = 0;
                     i < variante->campos.size() && i < rama.patron.ligaduras.size(); ++i) {
                    const std::string campo_c = tipo_declarado_c(variante->campos[i].tipo);
                    salida_ << indentacion() << campo_c << " " << rama.patron.ligaduras[i] << " = "
                            << temporal << ".datos." << variante->nombre << "_"
                            << variante->campos[i].nombre << ";\n";
                    declarar(rama.patron.ligaduras[i], campo_c);
                }
            }
            for (const sintactico::Sentencia& hija : rama.cuerpo) {
                emitir_sentencia(hija, false);
            }
            cerrar_ambito();
            --nivel_;
            salida_ << indentacion() << "}\n";
        }

        --nivel_;
        salida_ << indentacion() << "}\n";
    }

    void emitir_coincide(const sintactico::Sentencia& sentencia) {
        if (!sentencia.expresion) {
            no_soportado("un «coincide» sin valor que examinar");
            return;
        }

        // ¿Es una enumeración con datos?
        const std::string tipo_examinado = tipo_de(*sentencia.expresion);
        if (tipo_examinado.rfind("nooki_", 0) == 0) {
            const auto enumeracion =
                enumeraciones_.find(tipo_examinado.substr(std::string("nooki_").size()));
            if (enumeracion != enumeraciones_.end()) {
                bool con_datos = false;
                for (const sintactico::Variante& variante : enumeracion->second->variantes) {
                    if (!variante.campos.empty()) {
                        con_datos = true;
                    }
                }
                if (con_datos) {
                    emitir_coincide_datos(sentencia, *enumeracion->second);
                    return;
                }
            }
        }

        const std::string temporal = "__nooki_valor" + std::to_string(contador_temporal_++);

        salida_ << indentacion() << "{\n";
        ++nivel_;
        salida_ << indentacion() << "long long " << temporal << " = "
                << expresion(*sentencia.expresion) << ";\n";

        bool primero = true;
        for (const sintactico::Rama& rama : sentencia.ramas) {
            if (rama.patron.clase == sintactico::Patron::Clase::Comodin) {
                salida_ << indentacion() << (primero ? "{" : "else {") << "\n";
            } else {
                const std::string valor = patron_c(rama.patron);
                if (valor.empty()) {
                    no_soportado("un patrón que el backend no sabe comparar");
                    --nivel_;
                    salida_ << indentacion() << "}\n";
                    return;
                }
                salida_ << indentacion() << (primero ? "if" : "else if") << " (" << temporal
                        << " == " << valor << ") {\n";
            }
            primero = false;

            ++nivel_;
            for (const sintactico::Sentencia& hija : rama.cuerpo) {
                emitir_sentencia(hija, false);
            }
            --nivel_;
            salida_ << indentacion() << "}\n";
        }

        --nivel_;
        salida_ << indentacion() << "}\n";
    }

    std::string patron_c(const sintactico::Patron& patron) {
        using sintactico::Patron;

        switch (patron.clase) {
        case Patron::Clase::Variante:
            return constante_enum(patron.enumeracion, patron.valor);
        case Patron::Clase::Literal:
            if (patron.valor == "verdadero") {
                return "1";
            }
            if (patron.valor == "falso") {
                return "0";
            }
            if (!patron.valor.empty() &&
                (patron.valor.front() == '"' || patron.valor.front() == '\'')) {
                return {};
            }
            return limpiar_separadores(patron.valor);
        case Patron::Clase::Comodin:
            break;
        }
        return {};
    }

    std::string miembro(const sintactico::Expresion& nodo) {
        if (!nodo.izquierda) {
            no_soportado("un acceso a miembro sin objeto");
            return "0";
        }
        if (nodo.izquierda->tipo == sintactico::TipoExpresion::Identificador) {
            const auto enumeracion = enumeraciones_.find(std::string(nodo.izquierda->lexema));
            if (enumeracion != enumeraciones_.end()) {
                // Una variante sin datos de una enumeración con datos es una
                // estructura con su etiqueta, no un número suelto.
                for (std::size_t i = 0; i < enumeracion->second->variantes.size(); ++i) {
                    if (enumeracion->second->variantes[i].nombre == nodo.anotacion) {
                        if (!enumeracion->second->variantes[i].campos.empty()) {
                            break;
                        }
                        const bool tiene_datos = std::any_of(
                            enumeracion->second->variantes.begin(),
                            enumeracion->second->variantes.end(),
                            [](const sintactico::Variante& v) { return !v.campos.empty(); });
                        if (tiene_datos) {
                            return "(nooki_" + std::string(enumeracion->first) +
                                   "){ .etiqueta = " + std::to_string(i) + " }";
                        }
                        break;
                    }
                }
                return constante_enum(enumeracion->first, nodo.anotacion);
            }
        }
        if (nodo.izquierda->tipo == sintactico::TipoExpresion::Este) {
            return "este->" + nodo.anotacion;
        }
        const std::string objeto = tipo_de(*nodo.izquierda);
        if (!objeto.empty() && objeto.back() == '*') {
            return expresion(*nodo.izquierda) + "->" + nodo.anotacion;
        }
        return expresion(*nodo.izquierda) + "." + nodo.anotacion;
    }

    std::string unaria(const sintactico::Expresion& nodo) {
        if (nodo.lexema == "&" || nodo.lexema == "&mutable") {
            if (!nodo.izquierda) {
                return "0";
            }
            // Prestar algo que no es una variable (el resultado de una llamada)
            // necesita un sitio donde vivir. En C, un literal compuesto de una
            // matriz de un elemento da justo eso: memoria automática que dura lo
            // que el bloque, y una dirección válida que pasar.
            const bool es_sitio =
                nodo.izquierda->tipo == sintactico::TipoExpresion::Identificador ||
                nodo.izquierda->tipo == sintactico::TipoExpresion::Miembro ||
                nodo.izquierda->tipo == sintactico::TipoExpresion::Indice ||
                nodo.izquierda->tipo == sintactico::TipoExpresion::Este;
            if (es_sitio) {
                return "&" + expresion(*nodo.izquierda);
            }
            const std::string tipo = tipo_de(*nodo.izquierda);
            if (tipo.empty() || tipo == "void") {
                no_soportado("prestar el resultado de una llamada sin tipo");
                return "0";
            }
            return "(&((" + tipo + "[]){ " + expresion(*nodo.izquierda) + " })[0])";
        }
        if (nodo.lexema == "*") {
            if (!nodo.izquierda) {
                return "0";
            }
            const std::string tipo = tipo_de(*nodo.izquierda);
            if (tipo == "nooki_caja") {
                return "(*" + expresion(*nodo.izquierda) + ".datos)";
            }
            if (tipo == "nooki_caja*") {
                return "(*(*" + expresion(*nodo.izquierda) + ").datos)";
            }
            // Un préstamo de cualquier otra cosa se lee con `(*x)`.
            if (!tipo.empty() && tipo.back() == '*') {
                return "(*" + expresion(*nodo.izquierda) + ")";
            }
            // `*prestamo` de texto: el prestatario devuelve el valor prestado.
            if (tipo == "nooki_texto*") {
                return "(*" + expresion(*nodo.izquierda) + ")";
            }
            no_soportado("«*» sobre algo que no es una «Caja» ni un préstamo de texto");
            return "0";
        }
        if (nodo.lexema != "-" && nodo.lexema != "!") {
            no_soportado("un operador unario que no sea «-», «!» o «*»");
            return "0";
        }
        if (!nodo.izquierda) {
            return "0";
        }
        // Negar el mínimo de `entero` desborda: se comprueba (ADR-03). Un
        // flotante se niega directamente, sin comprobación.
        if (nodo.lexema == "-") {
            if (tipo_de(*nodo.izquierda) == "double") {
                return "(-(" + expresion(*nodo.izquierda) + "))";
            }
            return "nooki_ent_negativo(" + expresion(*nodo.izquierda) + ")";
        }
        return "(!" + expresion(*nodo.izquierda) + ")";
    }

    std::string binaria(const sintactico::Expresion& nodo) {
        if (!nodo.izquierda || !nodo.derecha) {
            no_soportado("un operador sin sus dos operandos");
            return "0";
        }

        const std::string_view operador = nodo.lexema;
        const std::string tipo_izquierda = tipo_de(*nodo.izquierda);
        const std::string tipo_derecha = tipo_de(*nodo.derecha);
        const bool ambos_texto = tipo_izquierda == "nooki_texto" && tipo_derecha == "nooki_texto";

        if (ambos_texto && operador == "+") {
            return "nooki_texto_concatenar(" + expresion(*nodo.izquierda) + ", " +
                   expresion(*nodo.derecha) + ")";
        }
        if (ambos_texto && es_comparacion(operador)) {
            return "(nooki_texto_comparar(" + expresion(*nodo.izquierda) + ", " +
                   expresion(*nodo.derecha) + ") " + std::string(operador) + " 0)";
        }

        if (operador == "**") {
            return "nooki_potencia(" + expresion(*nodo.izquierda) + ", " +
                   expresion(*nodo.derecha) + ")";
        }

        static constexpr std::string_view OPERADORES[] = {
            "+",
            "-",
            "*",
            "/",
            "%",
            "==",
            "!=",
            "<",
            ">",
            "<=",
            ">=",
            "&&",
            "||",
            // De bits y desplazamientos: el parser los acepta y las asignaciones
            // compuestas ya los usaban, pero en una expresión se rechazaban.
            "&",
            "|",
            "^",
            "<<",
            ">>",
        };
        bool conocido = false;
        for (const std::string_view conocido_operador : OPERADORES) {
            if (operador == conocido_operador) {
                conocido = true;
                break;
            }
        }
        if (!conocido) {
            no_soportado("el operador «" + std::string(operador) + "»");
            return "0";
        }

        // Aritmética entera comprobada (ADR-03): suma, resta, multiplicación,
        // división y módulo avisan de un desbordamiento o de una división por cero.
        const bool entero_izquierda = tipo_izquierda == "long long" || tipo_izquierda == "int";
        const bool entero_derecha = tipo_derecha == "long long" || tipo_derecha == "int";
        if (entero_izquierda && entero_derecha && !es_comparacion(operador) && !sin_comprobacion_) {
            static const std::map<std::string_view, std::string_view> COMPROBADAS = {
                {"+", "nooki_ent_sumar"},       {"-", "nooki_ent_restar"},
                {"*", "nooki_ent_multiplicar"}, {"/", "nooki_ent_dividir"},
                {"%", "nooki_ent_modulo"},
            };
            const auto comprobada = COMPROBADAS.find(operador);
            if (comprobada != COMPROBADAS.end()) {
                return std::string(comprobada->second) + "(" + expresion(*nodo.izquierda) + ", " +
                       expresion(*nodo.derecha) + ")";
            }
        }

        return "(" + expresion(*nodo.izquierda) + " " + std::string(operador) + " " +
               expresion(*nodo.derecha) + ")";
    }

    /// Vacía una colección para que su limpieza no libere lo que ya se movió.
    static std::string vaciar_coleccion(const std::string& nombre) {
        return "(" + nombre + ".datos = NULL, " + nombre + ".largo = 0, " + nombre +
               ".capacidad = 0)";
    }

    std::string asignacion(const sintactico::Expresion& nodo) {
        if (!nodo.izquierda || !nodo.derecha) {
            no_soportado("una asignación incompleta");
            return "0";
        }
        const bool destino_valido =
            nodo.izquierda->tipo == sintactico::TipoExpresion::Identificador ||
            nodo.izquierda->tipo == sintactico::TipoExpresion::Miembro ||
            (nodo.izquierda->tipo == sintactico::TipoExpresion::Unaria &&
             nodo.izquierda->lexema == "*");
        if (!destino_valido) {
            no_soportado("una asignación a algo que no es una variable ni un campo");
            return "0";
        }
        // Asignar un texto libera el búfer viejo del destino. Se hace con una
        // función porque el valor nuevo tiene que evaluarse antes de liberar.
        const std::string destino = expresion(*nodo.izquierda);
        const std::string origen = expresion(*nodo.derecha);
        if (nodo.lexema == "=" && tipo_de(*nodo.izquierda) == "nooki_texto") {
            const bool a_si_mismo =
                nodo.derecha->tipo == sintactico::TipoExpresion::Identificador &&
                nodo.izquierda->tipo == sintactico::TipoExpresion::Identificador &&
                nodo.izquierda->lexema == nodo.derecha->lexema;
            if (!a_si_mismo) {
                return "nooki_texto_asignar(&(" + destino + "), " + origen + ")";
            }
        }

        // Asignar una colección la mueve: el origen queda vacío para que su
        if (nodo.lexema == "=") {
            const std::string tipo = tipo_de(*nodo.izquierda);
            const bool coleccion =
                tipo.rfind("nooki_vector_", 0) == 0 || tipo.rfind("nooki_mapa_", 0) == 0;
            if (coleccion && (nodo.derecha->tipo == sintactico::TipoExpresion::Identificador ||
                              nodo.derecha->tipo == sintactico::TipoExpresion::Miembro)) {
                // Se libera lo que tuviera el destino, se mueve el origen y se
                // vacía el origen para que su limpieza no libere lo movido.
                return "(" + tipo + "_soltar(&(" + destino + ")), " + destino + " = " + origen +
                       ", " + vaciar_coleccion(origen) + ")";
            }
        }
        return destino + " " + std::string(nodo.lexema) + " " + origen;
    }

    std::string llamada(const sintactico::Expresion& nodo) {
        if (!nodo.izquierda) {
            no_soportado("una llamada sin destino");
            return "0";
        }

        if (nodo.izquierda->tipo == sintactico::TipoExpresion::Miembro) {
            const sintactico::Expresion& acceso = *nodo.izquierda;

            // Llamada a una función de otro módulo: `matematicas.cuadrado(7)`.
            if (acceso.izquierda &&
                acceso.izquierda->tipo == sintactico::TipoExpresion::Identificador &&
                !declarada(acceso.izquierda->lexema) &&
                modulos_.count(acceso.izquierda->lexema) != 0) {
                std::string salida = "nooki_" + std::string(acceso.izquierda->lexema) + "_" +
                                     std::string(acceso.anotacion) + "(";
                const auto generico = modulos_.find(acceso.izquierda->lexema);
                const sintactico::Funcion* modulo_funcion = nullptr;
                if (generico != modulos_.end()) {
                    const auto encontrada = generico->second.find(acceso.anotacion);
                    if (encontrada != generico->second.end()) {
                        modulo_funcion = encontrada->second;
                    }
                }
                for (std::size_t i = 0; i < nodo.lista.size(); ++i) {
                    if (i != 0) {
                        salida += ", ";
                    }
                    if (!nodo.lista[i]) {
                        salida += "0";
                        continue;
                    }
                    std::string valor = expresion(*nodo.lista[i]);

                    // Un literal que llega a un parámetro prestado necesita dirección propia.
                    if (modulo_funcion != nullptr && i < modulo_funcion->parametros.size()) {
                        const std::string_view declarado = modulo_funcion->parametros[i].tipo;
                        if (!declarado.empty() && declarado.front() == '&' &&
                            (nodo.lista[i]->tipo == sintactico::TipoExpresion::Texto ||
                             nodo.lista[i]->tipo == sintactico::TipoExpresion::Entero)) {
                            valor = argumento_prestado(*nodo.lista[i], declarado);
                        }
                    }
                    salida += valor;
                }
                salida += ')';
                return salida;
            }

            if (acceso.izquierda) {
                const std::string tipo = tipo_de(*acceso.izquierda);
                const bool prestado = tipo == "nooki_texto*";
                if (tipo == "nooki_texto" || prestado) {
                    const std::string objeto = prestado ? "(*" + expresion(*acceso.izquierda) + ")"
                                                        : expresion(*acceso.izquierda);
                    static const std::set<std::string> METODOS_TEXTO = {
                        "largo",       "subtexto", "caracter",       "buscar",    "contiene",
                        "empieza_con", "a_entero", "a_flotante",     "dividir",   "mayusculas",
                        "minusculas",  "recortar", "agregar_codigo", "sin_ultimo"};
                    if (METODOS_TEXTO.count(std::string(acceso.anotacion)) != 0) {
                        std::string llamada =
                            "nooki_texto_" + std::string(acceso.anotacion) + "(" + objeto;
                        for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                            if (!argumento) {
                                continue;
                            }
                            llamada += ", ";
                            // El runtime recibe el texto por valor: un préstamo se
                            // desreferencia, venga de `&x` o de un parámetro `&texto`.
                            if (argumento->tipo == sintactico::TipoExpresion::Unaria &&
                                (argumento->lexema == "&" || argumento->lexema == "&mutable")) {
                                llamada += "(*" + expresion(*argumento) + ")";
                            } else if (tipo_de(*argumento) == "nooki_texto*") {
                                llamada += "(*" + expresion(*argumento) + ")";
                            } else {
                                llamada += expresion(*argumento);
                            }
                        }
                        llamada += ')';
                        return llamada;
                    }
                }
            }
            if (acceso.izquierda &&
                acceso.izquierda->tipo == sintactico::TipoExpresion::Identificador &&
                acceso.izquierda->lexema == "Mapa" && acceso.anotacion == "nuevo") {
                const std::string_view valor = acceso.izquierda->tipos_aplicados.size() == 2
                                                   ? acceso.izquierda->tipos_aplicados[1]
                                                   : std::string_view{};
                if (valor == "flotante") {
                    return "nooki_mapa_texto_flotante_nuevo()";
                }
                if (valor == "texto") {
                    return "nooki_mapa_texto_texto_nuevo()";
                }
                return "nooki_mapa_texto_entero_nuevo()";
            }
            if (acceso.izquierda &&
                acceso.izquierda->tipo == sintactico::TipoExpresion::Identificador &&
                acceso.izquierda->lexema == "Vector" && acceso.anotacion == "nuevo") {
                const std::string_view elemento = acceso.izquierda->tipos_aplicados.empty()
                                                      ? std::string_view{}
                                                      : acceso.izquierda->tipos_aplicados.front();
                if (elemento == "texto") {
                    return "nooki_vector_texto_nuevo()";
                }
                if (elemento == "flotante") {
                    return "nooki_vector_flotante_nuevo()";
                }
                if (!elemento.empty() &&
                    vectores_de_estructura_.count(std::string(elemento)) != 0) {
                    return "nooki_vector_" + std::string(elemento) + "_nuevo()";
                }
                return "nooki_vector_entero_nuevo()";
            }
            if (acceso.izquierda &&
                acceso.izquierda->tipo == sintactico::TipoExpresion::Identificador &&
                acceso.izquierda->lexema == "Caja") {
                if (nodo.lista.size() != 1 || !nodo.lista.front()) {
                    no_soportado("«Caja.nuevo» con un número de argumentos distinto de uno");
                    return "0";
                }
                return "nooki_caja_nuevo(" + expresion(*nodo.lista.front()) + ")";
            }
            return llamada_metodo(nodo);
        }

        if (nodo.izquierda->tipo != sintactico::TipoExpresion::Identificador) {
            no_soportado("una llamada a algo que no es un nombre");
            return "0";
        }

        if (!nodo.tipos_aplicados.empty()) {
            const std::string nombre_mangleado =
                manglear(std::string(nodo.izquierda->lexema), nodo.tipos_aplicados);
            instancias_[nombre_mangleado] = {std::string(nodo.izquierda->lexema),
                                             nodo.tipos_aplicados};

            std::string salida = nombre_mangleado + "(";
            for (std::size_t i = 0; i < nodo.lista.size(); ++i) {
                if (i != 0) {
                    salida += ", ";
                }
                salida += nodo.lista[i] ? expresion(*nodo.lista[i]) : "0";
            }
            salida += ')';
            return salida;
        }

        const std::string_view nombre_llamado = nodo.izquierda->lexema;

        // `Bien(5)`: construcción de una variante con datos.
        if (const auto variante = variantes_.find(std::string(nombre_llamado));
            variante != variantes_.end()) {
            const sintactico::Enumeracion& enumeracion = *variante->second.first;
            const std::size_t indice = variante->second.second;
            const sintactico::Variante& datos = enumeracion.variantes[indice];
            if (!datos.campos.empty()) {
                std::string salida = "(nooki_" + std::string(enumeracion.nombre) +
                                     "){ .etiqueta = " + std::to_string(indice) + ", .datos = {";
                for (std::size_t i = 0; i < datos.campos.size() && i < nodo.lista.size(); ++i) {
                    salida += " .";
                    salida += datos.nombre;
                    salida += "_";
                    salida += datos.campos[i].nombre;
                    salida += " = ";
                    salida += nodo.lista[i] ? expresion(*nodo.lista[i]) : "0";
                    salida += ",";
                }
                salida += " } }";
                return salida;
            }
        }

        // Punteros: los argumentos van tal cual, sin conversión.
        if (const auto puntero = PUNTEROS_C.find(std::string(nombre_llamado));
            puntero != PUNTEROS_C.end()) {
            std::string salida = "nooki_puntero_" + std::string(puntero->second) + "(";
            bool primero = true;
            for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                if (!argumento) {
                    continue;
                }
                if (!primero) {
                    salida += ", ";
                }
                primero = false;
                salida += expresion(*argumento);
            }
            salida += ')';
            return salida;
        }

        // Tablas de datos: el nombre en Nooki es el de C con prefijo.
        if (nombre_llamado == "tabla_de_consulta" || nombre_llamado == "filas_de_tabla" ||
            nombre_llamado == "columnas_de_tabla" || nombre_llamado == "valor_de_tabla" ||
            nombre_llamado == "nombre_de_columna" || nombre_llamado == "liberar_tabla" ||
            nombre_llamado == "numero_de_tabla" || nombre_llamado == "entero_de_tabla" ||
            nombre_llamado == "agregar_columna_a_tabla" ||
            nombre_llamado == "indice_de_columna_en_tabla") {
            const std::string en_c =
                nombre_llamado == "tabla_de_consulta"            ? "nooki_dd_tabla"
                : nombre_llamado == "filas_de_tabla"             ? "nooki_dd_tabla_filas"
                : nombre_llamado == "columnas_de_tabla"          ? "nooki_dd_tabla_columnas"
                : nombre_llamado == "valor_de_tabla"             ? "nooki_dd_tabla_valor"
                : nombre_llamado == "nombre_de_columna"          ? "nooki_dd_tabla_nombre"
                : nombre_llamado == "numero_de_tabla"            ? "nooki_dd_tabla_numero"
                : nombre_llamado == "entero_de_tabla"            ? "nooki_dd_tabla_entero"
                : nombre_llamado == "agregar_columna_a_tabla"    ? "nooki_dd_tabla_agregar_columna"
                : nombre_llamado == "indice_de_columna_en_tabla" ? "nooki_dd_tabla_indice"
                                                                 : "nooki_dd_tabla_liberar";
            std::string salida = en_c + "(";
            bool primero = true;
            for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                if (!argumento) {
                    continue;
                }
                if (!primero) {
                    salida += ", ";
                }
                primero = false;
                std::string valor = expresion(*argumento);
                const std::string tipo = tipo_de(*argumento);
                if (tipo == "nooki_texto") {
                    valor = argumento->tipo == sintactico::TipoExpresion::Identificador
                                ? valor + ".datos"
                                : "(" + valor + ").datos";
                } else if (tipo == "nooki_texto*") {
                    valor = "(" + valor + ")->datos";
                }
                salida += valor;
            }
            salida += ')';
            return salida;
        }

        // Argumentos del programa.
        if (nombre_llamado == "cuantos_argumentos_programa" || nombre_llamado == "argumento_de" ||
            nombre_llamado == "argumentos_programa") {
            if (nombre_llamado == "cuantos_argumentos_programa") {
                return "nooki_cuantos_argumentos()";
            }
            if (nombre_llamado == "argumentos_programa") {
                return "nooki_argumentos()";
            }
            std::string salida = "nooki_argumento(";
            bool primero = true;
            for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                if (!argumento) {
                    continue;
                }
                if (!primero) {
                    salida += ", ";
                }
                primero = false;
                salida += expresion(*argumento);
            }
            salida += ')';
            return salida;
        }

        // Datos: nombre en Nooki -> nombre en C.
        {
            const std::string_view nombre = nombre_llamado;
            const char* en_c = nullptr;
            if (nombre == "consultar_datos") {
                en_c = "nooki_dd_consultar";
            } else if (nombre == "ejecutar_datos") {
                en_c = "nooki_dd_ejecutar";
            } else if (nombre == "disponible_datos") {
                en_c = "nooki_dd_disponible";
            } else if (nombre == "version_datos") {
                en_c = "nooki_dd_version";
            } else if (nombre == "error_datos") {
                en_c = "nooki_dd_error";
            }
            if (en_c != nullptr) {
                std::string salida = std::string(en_c) + "(";
                bool primero = true;
                for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                    if (!argumento) {
                        continue;
                    }
                    if (!primero) {
                        salida += ", ";
                    }
                    primero = false;
                    std::string valor = expresion(*argumento);
                    const std::string tipo = tipo_de(*argumento);
                    if (tipo == "nooki_texto") {
                        valor = argumento->tipo == sintactico::TipoExpresion::Identificador
                                    ? valor + ".datos"
                                    : "(" + valor + ").datos";
                    } else if (tipo == "nooki_texto*") {
                        valor = "(" + valor + ")->datos";
                    }
                    salida += valor;
                }
                salida += ')';
                return salida;
            }
        }

        if (nombre_llamado == "ejecutar_orden" || nombre_llamado == "capturar_orden") {
            std::string salida = std::string("nooki_orden_") +
                                 (nombre_llamado == "capturar_orden" ? "capturar" : "ejecutar") +
                                 "(";
            bool primero = true;
            for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                if (!argumento) {
                    continue;
                }
                if (!primero) {
                    salida += ", ";
                }
                primero = false;
                std::string valor = expresion(*argumento);
                const std::string tipo = tipo_de(*argumento);
                if (tipo == "nooki_texto") {
                    valor = argumento->tipo == sintactico::TipoExpresion::Identificador
                                ? valor + ".datos"
                                : "(" + valor + ").datos";
                } else if (tipo == "nooki_texto*") {
                    valor = "(" + valor + ")->datos";
                }
                salida += valor;
            }
            salida += ')';
            return salida;
        }

        if (nombre_llamado == "leer_archivo" || nombre_llamado == "escribir_archivo" ||
            nombre_llamado == "agregar_archivo" || nombre_llamado == "existe_archivo") {
            std::string salida = "nooki_" + std::string(nombre_llamado) + "(";
            bool primero = true;
            for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                if (!argumento) {
                    continue;
                }
                if (!primero) {
                    salida += ", ";
                }
                primero = false;
                std::string valor = expresion(*argumento);
                const std::string tipo = tipo_de(*argumento);
                // Un texto (o un préstamo de texto) llega como puntero a sus bytes.
                if (argumento->tipo == sintactico::TipoExpresion::Unaria &&
                    argumento->lexema == "&" && argumento->izquierda &&
                    tipo_de(*argumento->izquierda) == "nooki_texto") {
                    valor = expresion(*argumento->izquierda) + ".datos";
                } else if (tipo == "nooki_texto") {
                    valor = argumento->tipo == sintactico::TipoExpresion::Identificador
                                ? valor + ".datos"
                                : "(" + valor + ").datos";
                } else if (tipo == "nooki_texto*") {
                    valor = "(" + valor + ")->datos";
                }
                salida += valor;
            }
            salida += ')';
            return salida;
        }

        if (nombre_llamado == "leer_linea" || nombre_llamado == "leer_entero") {
            if (!nodo.lista.empty()) {
                no_soportado("«" + std::string(nombre_llamado) + "» sin argumentos");
                return "0";
            }
            return "nooki_" + std::string(nombre_llamado) + "()";
        }

        if (nombre_llamado == "imprimir") {
            if (nodo.lista.size() != 1 || !nodo.lista.front()) {
                no_soportado("«imprimir» con un número de argumentos distinto de uno");
                return "0";
            }
            const sintactico::Expresion& argumento = *nodo.lista.front();
            const std::string tipo = tipo_de(argumento);
            const char* funcion = tipo == "nooki_texto" || tipo == "nooki_texto*"
                                      ? "nooki_imprimir_texto"
                                  : (tipo == "long long" || tipo == "int") ? "nooki_imprimir_entero"
                                  : (tipo == "double") ? "nooki_imprimir_flotante"
                                                       : nullptr;
            if (funcion == nullptr) {
                no_soportado("«imprimir» con un valor que el backend no sabe imprimir");
                return "0";
            }
            // Un préstamo de texto se imprime por su valor.
            const std::string valor =
                tipo == "nooki_texto*" ? "(*" + expresion(argumento) + ")" : expresion(argumento);
            return std::string(funcion) + "(" + valor + ")";
        }

        // Dentro de un módulo, una llamada a otro símbolo suyo lleva su prefijo.
        std::string nombre_para_c = nombre_c(nombre_llamado);
        if (!modulo_actual_.empty()) {
            const auto modulo = modulos_.find(modulo_actual_);
            if (modulo != modulos_.end() && modulo->second.count(nombre_llamado) != 0) {
                nombre_para_c =
                    "nooki_" + std::string(modulo_actual_) + "_" + std::string(nombre_llamado);
            }
        }
        // Una llamada que mueve un valor con dueño no puede ir anidada en otra
        // expresión: no habría dónde emitir el vaciado del origen.
        if (profundidad_expresion_ > 1) {
            const auto declarados = parametros_.find(std::string(nombre_llamado));
            if (declarados != parametros_.end()) {
                for (std::size_t i = 0; i < nodo.lista.size() && i < declarados->second.size();
                     ++i) {
                    if (!nodo.lista[i] ||
                        nodo.lista[i]->tipo != sintactico::TipoExpresion::Identificador) {
                        continue;
                    }
                    const std::string& declarado = declarados->second[i];
                    if (!declarado.empty() &&
                        (declarado.front() == '&' || declarado.front() == '*')) {
                        continue;
                    }
                    const std::string tipo_arg = tipo_de(*nodo.lista[i]);
                    if (tipo_arg == "nooki_texto" || tipo_arg == "nooki_caja" ||
                        tipo_arg.rfind("nooki_vector_", 0) == 0 ||
                        tipo_arg.rfind("nooki_mapa_", 0) == 0 ||
                        (!tipo_arg.empty() && tipo_arg.back() == '*')) {
                        no_soportado("una llamada que mueve un valor con dueño debe usarse "
                                     "como sentencia o inicializador");
                        return "0";
                    }
                }
            }
        }

        // Una función externa conserva su nombre de C y sus textos viajan como
        // `const char*`: se pasa el búfer, no la estructura.
        const bool externa = externas_.count(std::string(nombre_llamado)) != 0;
        if (externa) {
            nombre_para_c = std::string(nombre_llamado);
        }

        std::string salida = nombre_para_c + "(";
        const auto declarados = parametros_.find(std::string(nombre_llamado));
        bool primero = true;
        for (std::size_t i = 0; i < nodo.lista.size(); ++i) {
            const sintactico::ExpresionPtr& argumento = nodo.lista[i];
            if (!argumento) {
                continue;
            }
            if (!primero) {
                salida += ", ";
            }
            primero = false;

            std::string valor = expresion(*argumento);

            // `&p` donde `p` ya es una referencia (una clase): se pasa tal cual.
            if (declarados != parametros_.end() && i < declarados->second.size() &&
                !declarados->second[i].empty() && declarados->second[i].front() == '&' &&
                argumento->tipo == sintactico::TipoExpresion::Unaria && argumento->lexema == "&" &&
                argumento->izquierda) {
                const std::string tipo_interno = tipo_de(*argumento->izquierda);
                if (!tipo_interno.empty() && tipo_interno.back() == '*') {
                    valor = expresion(*argumento->izquierda);
                }
            }

            // `f(&objeto)` donde el parámetro es `&Rasgo`: se envuelve objeto y tabla.
            if (declarados != parametros_.end() && i < declarados->second.size()) {
                const std::string& declarado = declarados->second[i];
                if (declarado.size() > 1 && declarado.front() == '&' &&
                    es_rasgo_c(std::string_view(declarado).substr(1)) &&
                    argumento->tipo == sintactico::TipoExpresion::Unaria &&
                    argumento->lexema == "&" && argumento->izquierda) {
                    const std::string tipo_interno = tipo_de(*argumento->izquierda);
                    std::string base = tipo_interno;
                    const bool puntero = !base.empty() && base.back() == '*';
                    if (puntero) {
                        base.pop_back();
                    }
                    const std::string objeto = puntero ? expresion(*argumento->izquierda)
                                                       : "&" + expresion(*argumento->izquierda);
                    valor = "(nooki_ref_" + std::string(declarado).substr(1) +
                            "){ .objeto = (void*)" + objeto + ", .tabla = &" + base + "_" +
                            std::string(declarado).substr(1) + "_tabla }";
                }
            }

            // Un literal que llega a un parámetro prestado necesita dirección propia.
            if (!externa && declarados != parametros_.end() && i < declarados->second.size()) {
                const std::string& declarado = declarados->second[i];
                if (!declarado.empty() && declarado.front() == '&' &&
                    (argumento->tipo == sintactico::TipoExpresion::Texto ||
                     argumento->tipo == sintactico::TipoExpresion::Entero)) {
                    valor = argumento_prestado(*argumento, declarado);
                }
            }

            // Un texto hacia una función de C viaja como puntero a sus bytes.
            if (externa) {
                const std::string tipo_argumento = tipo_de(*argumento);
                if (tipo_argumento == "nooki_texto") {
                    valor = argumento->tipo == sintactico::TipoExpresion::Identificador
                                ? valor + ".datos"
                                : "(" + valor + ").datos";
                } else if (tipo_argumento == "nooki_texto*") {
                    valor = "(" + valor + ")->datos";
                }
            }

            salida += valor;
        }
        salida += ')';

        // Una función de C que devuelve `const char*` produce un texto de Nooki:
        // se copia, porque el búfer puede ser de la biblioteca y no nuestro.
        if (externa && retornos_.count(std::string(nombre_llamado)) != 0 &&
            retornos_[std::string(nombre_llamado)] == "nooki_texto") {
            return "nooki_texto_desde_c(" + salida + ")";
        }

        return salida;
    }

    /// Pasar un valor con dueño por valor (a un parámetro que no es préstamo) lo mueve:
    /// el origen queda vacío para que nadie lo libere dos veces.
    void emitir_movimientos_de_llamada(const sintactico::Expresion& nodo) {
        if (nodo.tipo != sintactico::TipoExpresion::Llamada || !nodo.izquierda) {
            return;
        }
        if (nodo.izquierda->tipo == sintactico::TipoExpresion::Identificador) {
            const auto funcion = funciones_ast_.find(std::string(nodo.izquierda->lexema));
            if (funcion != funciones_ast_.end()) {
                emitir_movimientos(nodo, funcion->second->parametros);
            }
            return;
        }
        if (nodo.izquierda->tipo == sintactico::TipoExpresion::Miembro) {
            const sintactico::Expresion& acceso = *nodo.izquierda;
            if (acceso.izquierda &&
                acceso.izquierda->tipo == sintactico::TipoExpresion::Identificador) {
                const auto modulo = modulos_.find(acceso.izquierda->lexema);
                if (modulo != modulos_.end()) {
                    const auto funcion = modulo->second.find(acceso.anotacion);
                    if (funcion != modulo->second.end()) {
                        emitir_movimientos(nodo, funcion->second->parametros);
                    }
                }
            }
        }
    }

    /// Vacía una variable con dueño: su valor pasa a otro sitio.
    void anular_variable(const std::string& nombre) {
        const std::string tipo = buscar(nombre);
        if (tipo == "nooki_caja") {
            salida_ << indentacion() << nombre << ".datos = NULL;\n";
        } else if (tipo == "nooki_texto") {
            salida_ << indentacion() << nombre << ".datos = NULL;\n";
            salida_ << indentacion() << nombre << ".largo = 0;\n";
            salida_ << indentacion() << nombre << ".propio = 0;\n";
        } else if (tipo.rfind("nooki_vector_", 0) == 0 || tipo.rfind("nooki_mapa_", 0) == 0) {
            salida_ << indentacion() << nombre << ".datos = NULL;\n";
            salida_ << indentacion() << nombre << ".largo = 0;\n";
            salida_ << indentacion() << nombre << ".capacidad = 0;\n";
        }
    }

    void emitir_movimientos(const sintactico::Expresion& nodo,
                            const std::vector<sintactico::Parametro>& declarados) {
        for (std::size_t i = 0; i < nodo.lista.size() && i < declarados.size(); ++i) {
            const sintactico::ExpresionPtr& argumento = nodo.lista[i];
            if (!argumento || argumento->tipo != sintactico::TipoExpresion::Identificador) {
                continue;
            }
            const std::string_view declarado = declarados[i].tipo;
            if (!declarado.empty() && (declarado.front() == '&' || declarado.front() == '*')) {
                continue;  // préstamo: no mueve
            }

            const std::string tipo = tipo_de(*argumento);
            const std::string nombre(argumento->lexema);
            if (tipo == "nooki_caja") {
                salida_ << indentacion() << nombre << ".datos = NULL;\n";
            } else if (tipo == "nooki_texto") {
                salida_ << indentacion() << nombre << ".datos = NULL;\n";
                salida_ << indentacion() << nombre << ".propio = 0;\n";
            } else if (tipo.rfind("nooki_vector_", 0) == 0) {
                salida_ << indentacion() << nombre << ".datos = NULL;\n";
                salida_ << indentacion() << nombre << ".largo = 0;\n";
                salida_ << indentacion() << nombre << ".capacidad = 0;\n";
            }
        }
    }
    std::string llamada_metodo(const sintactico::Expresion& nodo) {
        const sintactico::Expresion& acceso = *nodo.izquierda;
        if (!acceso.izquierda) {
            no_soportado("una llamada a un método sin objeto");
            return "0";
        }

        // `&Rasgo`: la referencia lleva el objeto y su tabla.
        if (tipo_de(*acceso.izquierda).rfind("nooki_ref_", 0) == 0) {
            std::string salida = expresion(*acceso.izquierda) + ".tabla->" +
                                 std::string(acceso.anotacion) + "(" +
                                 expresion(*acceso.izquierda) + ".objeto";
            for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                if (!argumento) {
                    continue;
                }
                salida += ", ";
                salida += expresion(*argumento);
            }
            salida += ')';
            return salida;
        }

        // `super.metodo(...)`: la implementación de la clase base, sin despacho.
        if (acceso.izquierda->tipo == sintactico::TipoExpresion::Super) {
            const auto base = base_de_.find(estructura_actual_c_);
            if (base == base_de_.end()) {
                no_soportado("«super» en una clase sin base");
                return "0";
            }
            std::string salida = base->second + "_" + std::string(acceso.anotacion) + "(este";
            for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                if (!argumento) {
                    continue;
                }
                salida += ", ";
                salida += expresion(*argumento);
            }
            salida += ')';
            return salida;
        }

        std::string tipo_objeto = tipo_de(*acceso.izquierda);
        // `este` siempre es una referencia al objeto, aunque su tipo no lleve «*».
        const bool objeto_puntero = (!tipo_objeto.empty() && tipo_objeto.back() == '*') ||
                                    acceso.izquierda->tipo == sintactico::TipoExpresion::Este;
        if (!tipo_objeto.empty() && tipo_objeto.back() == '*') {
            tipo_objeto.pop_back();
        }

        if (tipo_objeto.rfind("nooki_vector_", 0) == 0 ||
            tipo_objeto.rfind("nooki_mapa_", 0) == 0) {
            // Si llega como préstamo (`&Vector<T>`), ya es un puntero.
            std::string salida = tipo_objeto + "_" + std::string(acceso.anotacion) + "(";
            salida += objeto_puntero ? expresion(*acceso.izquierda)
                                     : direccion_de_expresion(*acceso.izquierda, tipo_objeto,
                                                              expresion(*acceso.izquierda));
            for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                if (!argumento) {
                    continue;
                }
                salida += ", ";
                // El runtime recibe el texto por valor, aunque llegue prestado.
                if (tipo_de(*argumento) == "nooki_texto*") {
                    salida += "(*" + expresion(*argumento) + ")";
                } else {
                    salida += expresion(*argumento);
                }
            }
            salida += ')';

            // `agregar` e `insertar` toman el dueño del valor: el origen queda vacío.
            const bool toma_dueno = (acceso.anotacion == "agregar" && nodo.lista.size() == 1) ||
                                    (acceso.anotacion == "insertar" && nodo.lista.size() == 2);
            if (toma_dueno && nodo.lista.front() && tipo_de(*nodo.lista.front()) == "nooki_texto" &&
                (nodo.lista.front()->tipo == sintactico::TipoExpresion::Identificador ||
                 nodo.lista.front()->tipo == sintactico::TipoExpresion::Miembro)) {
                // Vale un nombre o un campo: en ambos casos el origen se vacía.
                const std::string origen = expresion(*nodo.lista.front());
                salida = "(" + salida + ", " + origen + ".datos = NULL, " + origen +
                         ".largo = 0, " + origen + ".propio = 0)";
            }
            return salida;
        }

        constexpr std::string_view PREFIJO = "nooki_";
        if (tipo_objeto.rfind(PREFIJO, 0) != 0) {
            no_soportado("una llamada a un método sobre algo que no es una estructura");
            return "0";
        }

        const auto tipo_ptr = por_c_.find(tipo_objeto);
        const std::vector<std::string> virtuales = virtuales_de_at(tipo_objeto);
        // Solo las clases tienen tabla virtual: una estructura se llama directo.
        const bool virtual_aqui =
            objeto_puntero && tipo_ptr != por_c_.end() && tipo_ptr->second->es_clase &&
            std::find(virtuales.begin(), virtuales.end(), std::string(acceso.anotacion)) !=
                virtuales.end();

        // Expresión de C que da un puntero al objeto, sea `este` o un valor.
        std::string objeto_ptr =
            acceso.izquierda->tipo == sintactico::TipoExpresion::Este
                ? "este"
                : (objeto_puntero ? expresion(*acceso.izquierda)
                                  : direccion_de_expresion(*acceso.izquierda, tipo_objeto,
                                                           expresion(*acceso.izquierda)));
        // `(*x).metodo()` sobre una referencia: el runtime quiere `x`.
        if (acceso.izquierda->tipo == sintactico::TipoExpresion::Unaria &&
            acceso.izquierda->lexema == "*" && acceso.izquierda->izquierda) {
            objeto_ptr = expresion(*acceso.izquierda->izquierda);
        }

        std::string salida;
        if (virtual_aqui) {
            salida = "((const " + raiz_c(tipo_objeto) + "_vtabla*)" + objeto_ptr + "->vtabla)->" +
                     std::string(acceso.anotacion) + "((void*)" + objeto_ptr;
        } else {
            salida = tipo_objeto + "_" + std::string(acceso.anotacion) + "(" + objeto_ptr;
        }

        for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
            if (!argumento) {
                continue;
            }
            salida += ", ";
            salida += expresion(*argumento);
        }

        salida += ')';
        return salida;
    }

    static std::string descripcion_expresion(sintactico::TipoExpresion tipo) {
        switch (tipo) {
        case sintactico::TipoExpresion::Caracter:
            return "un literal de carácter";
        case sintactico::TipoExpresion::Flotante:
            return "un número flotante";
        case sintactico::TipoExpresion::Nulo:
            return "el valor nulo";
        case sintactico::TipoExpresion::MiembroOpcional:
            return "el acceso opcional a un miembro";
        case sintactico::TipoExpresion::Indice:
            return "el acceso por índice";
        case sintactico::TipoExpresion::Propagacion:
            return "el operador «?»";
        case sintactico::TipoExpresion::Como:
            return "la conversión «como»";
        default:
            return "una expresión";
        }
    }

    std::ostringstream salida_;
    std::map<std::string, std::string> tipo_c_de_;
    std::map<std::string, const sintactico::DefinicionTipo*> tipo_ptr_;
    /// Nombre de C de la clase base de cada clase.
    std::map<std::string, std::string> base_de_;
    std::map<std::string, const sintactico::DefinicionTipo*> por_c_;
    /// Campos de una clase en orden de C: primero los heredados.
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> campos_planos_;
    /// Métodos virtuales (declarados en la raíz de la jerarquía).
    std::map<std::string, std::vector<std::string>> virtuales_de_;
    std::map<std::string, std::map<std::string, std::string>> campos_;
    std::map<std::string, std::string> retornos_;
    /// Tipos declarados de los parámetros, para saber cuáles son préstamos.
    std::map<std::string, std::vector<std::string>> parametros_;
    std::map<std::string_view, std::map<std::string_view, const sintactico::Funcion*>> modulos_;
    std::map<std::string, const sintactico::Funcion*> funciones_ast_;
    std::map<std::string, const sintactico::DefinicionTipo*> genericos_tipo_;
    std::set<std::string> instancias_tipo_;
    /// Estructuras usadas dentro de un `Vector<...>`.
    std::set<std::string> vectores_de_estructura_;
    /// Estructuras con algún campo de texto, para copiarlo y liberarlo.
    std::set<std::string> estructuras_con_texto_;
    std::set<std::string> compuestos_con_dueno_;
    std::set<std::string> rasgos_;
    std::set<std::string> externas_;
    /// Tipo de retorno de la función que se está generando.
    std::string retorno_actual_;
    /// Dentro de `inseguro { }` la aritmética se emite sin comprobar.
    bool sin_comprobacion_ = false;
    std::map<std::string, const sintactico::Rasgo*> rasgos_def_;
    std::set<std::string> clases_;
    std::vector<std::size_t> bucles_;
    int profundidad_expresion_ = 0;
    std::map<std::string, std::map<std::string, const sintactico::Funcion*>> metodos_;
    std::map<std::string, const sintactico::Enumeracion*> enumeraciones_;
    std::map<std::string, std::pair<const sintactico::Enumeracion*, std::size_t>> variantes_;
    std::map<std::string, const sintactico::Funcion*> genericos_;
    std::map<std::string, std::pair<std::string, std::vector<std::string_view>>> instancias_;
    std::map<std::string, std::string> sustitucion_;
    std::vector<Ambito> ambitos_;
    std::string estructura_actual_c_;
    std::string_view modulo_actual_;
    int contador_temporal_ = 0;
    int nivel_ = 0;
    bool soportado_ = true;
    std::string motivo_;
};

}  // namespace

ResultadoGeneracion generar_c(const sintactico::Programa& programa) {
    EmisorC emisor;
    ResultadoGeneracion resultado;
    resultado.codigo = emisor.generar(programa);
    resultado.soportado = emisor.soportado();
    resultado.motivo = emisor.motivo();
    return resultado;
}

}  // namespace nooki::generador
