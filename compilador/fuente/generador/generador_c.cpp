#include "nooki/generador/generador_c.hpp"

#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/semantico/tipos.hpp"

namespace nooki::generador {
namespace {

/// Tipo de C de un tipo escrito en Nooki.
std::string tipo_c(std::string_view escrito) {
    if (!escrito.empty() && (escrito.front() == '&' || escrito.front() == '*')) {
        std::string_view resto = escrito.substr(1);
        if (resto.rfind("mutable ", 0) == 0) {
            resto = resto.substr(8);
        }
        return tipo_c(resto) + "*";
    }
    if (escrito.rfind("Caja<", 0) == 0) {
        return "nooki_caja";
    }
    if (escrito.rfind("Mapa<", 0) == 0) {
        return "nooki_mapa_texto_entero";
    }

    if (escrito.rfind("Vector<", 0) == 0) {
        return escrito.find("texto") != std::string_view::npos ? "nooki_vector_texto"
                                                              : "nooki_vector_entero";
    }
    switch (nooki::semantico::desde_nombre(escrito)) {
        case nooki::semantico::Tipo::Texto:
            return "nooki_texto";
        case nooki::semantico::Tipo::Booleano:
            return "int";
        case nooki::semantico::Tipo::Vacio:
            return "void";
        case nooki::semantico::Tipo::Desconocido:
        case nooki::semantico::Tipo::Entero:
            return "long long";
    }
    return "long long";
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

std::string constante_enum(std::string_view enumeracion, std::string_view variante) {
    return "NOOKI_" + mayusculas(enumeracion) + "_" + mayusculas(variante);
}

std::string tipo_estructura(std::string_view nombre) { return "nooki_" + std::string(nombre); }

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
    "        nooki_texto* datos = (nooki_texto*)realloc(v->datos, sizeof(nooki_texto) * (size_t)nueva);\n"
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
    "    m.cubetas = (nooki_mapa_entrada**)calloc((size_t)m.capacidad, sizeof(nooki_mapa_entrada*));\n"
    "    return m;\n"
    "}\n"
    "\n"
    "static int nooki_texto_igual(nooki_texto a, nooki_texto b) {\n"
    "    return nooki_texto_comparar(a, b) == 0;\n"
    "}\n"
    "\n"
    "static void nooki_mapa_texto_entero_rehash(nooki_mapa_texto_entero* m);\n"
    "\n"
    "static void nooki_mapa_texto_entero_insertar(nooki_mapa_texto_entero* m, nooki_texto clave, long long valor) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long long)m->capacidad;\n"
    "    for (nooki_mapa_entrada* e = m->cubetas[indice]; e != NULL; e = e->siguiente) {\n"
    "        if (nooki_texto_igual(e->clave, clave)) {\n"
    "            e->valor = valor;\n"
    "            nooki_texto_soltar(&clave);  /* la clave nueva no se usa: el mapa ya la tiene */\n"
    "            return;\n"
    "        }\n"
    "    }\n"
    "    nooki_mapa_entrada* nueva = (nooki_mapa_entrada*)malloc(sizeof(nooki_mapa_entrada));\n"
    "    if (nueva == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    nueva->clave = clave;\n"
    "    nueva->valor = valor;\n"
    "    nueva->siguiente = m->cubetas[indice];\n"
    "    m->cubetas[indice] = nueva;\n"
    "    m->largo += 1;\n"
    "    if (m->largo * 4 > m->capacidad * 3) {\n"
    "        nooki_mapa_texto_entero_rehash(m);\n"
    "    }\n"
    "}\n"
    "\n"
    "static long long nooki_mapa_texto_entero_obtener(nooki_mapa_texto_entero* m, nooki_texto clave) {\n"
    "    if (m->cubetas == NULL) {\n"
    "        return 0;\n"
    "    }\n"
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long long)m->capacidad;\n"
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
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long long)m->capacidad;\n"
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
    "    nooki_mapa_entrada** cubetas = (nooki_mapa_entrada**)calloc((size_t)nueva, sizeof(nooki_mapa_entrada*));\n"
    "    if (cubetas == NULL) {\n"
    "        return;\n"
    "    }\n"
    "    for (long long i = 0; i < m->capacidad; ++i) {\n"
    "        nooki_mapa_entrada* e = m->cubetas[i];\n"
    "        while (e != NULL) {\n"
    "            nooki_mapa_entrada* siguiente = e->siguiente;\n"
    "            const unsigned long long indice = nooki_mapa_hash(e->clave) % (unsigned long long)nueva;\n"
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
    "    const unsigned long long indice = nooki_mapa_hash(clave) % (unsigned long long)m->capacidad;\n"
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
    "\n"    "/* Caja<T>: valor en el montículo con un único dueño (S20). */\n"
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
    "static void nooki_caja_soltar(nooki_caja* caja) {\n"
    "    if (caja->datos != NULL) {\n"
    "        free(caja->datos);\n"
    "        caja->datos = NULL;\n"
    "    }\n"
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
            std::vector<std::pair<std::string, std::pair<std::string, std::vector<std::string_view>>>>
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

        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            for (const sintactico::Funcion& metodo : definicion.metodos) {
                salida_ << '\n';
                emitir_metodo(definicion, metodo);
            }
        }
        for (const sintactico::Funcion& funcion : programa.funciones) {
            if (!funcion.tipo_parametros.empty()) {
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
        std::string salida = "static " + tipo_declarado_c(funcion.tipo_retorno) + " " + nombre + "(";
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

    [[nodiscard]] bool soportado() const noexcept { return soportado_; }
    [[nodiscard]] const std::string& motivo() const noexcept { return motivo_; }

private:
    // --- Registro ----------------------------------------------------------
    void registrar(const sintactico::Programa& programa) {
        // Tipos genéricos e instanciaciones que el programa usa.
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            if (!definicion.tipo_parametros.empty()) {
                genericos_tipo_[std::string(definicion.nombre)] = &definicion;
            }
        }
        recoger_instancias(programa);
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
                const std::string tipo_campo = tipo_declarado_c(
                    sustituir_tipo(campo.tipo, *generico->second, argumentos));
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
                tipo_ptr_[std::string(definicion.modulo) + "." +
                          std::string(definicion.nombre)] = &definicion;
            }
            tipo_c_de_[std::string(definicion.nombre)] = nombre_c;
            if (!definicion.modulo.empty()) {
                tipo_c_de_[std::string(definicion.modulo) + "." +
                          std::string(definicion.nombre)] = nombre_c;
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
            tipo_c_de_[std::string(enumeracion.nombre)] = "nooki_" + std::string(enumeracion.nombre);
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
                    salida_ << constante_enum(enumeracion.nombre,
                                              enumeracion.variantes[i].nombre)
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
                    salida_ << "        " << tipo_declarado_c(campo.tipo) << ' '
                            << variante.nombre << "_" << campo.nombre << ";\n";
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
                salida_ << "static void " << nombre_c << "_soltar(" << nombre_c
                        << "* valor) {\n";
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
                salida_ << "    " << tipo_declarado_c(tipo_campo) << ' ' << campo.nombre
                        << ";\n";
            }
            salida_ << "} " << nombre_c << ";\n\n";

            // Si algún campo tiene dueño, el compuesto lo libera al salir.
            bool con_dueno = false;
            salida_ << "static void " << nombre_c << "_soltar(" << nombre_c << "* valor) {\n";
            for (const sintactico::Campo& campo : generico->second->campos) {
                const std::string tipo_campo = tipo_declarado_c(
                    sustituir_tipo(campo.tipo, *generico->second, argumentos));
                if (tipo_campo == "nooki_texto") {
                    salida_ << "    nooki_texto_soltar(&valor->" << campo.nombre << ");\n";
                    con_dueno = true;
                } else if (tipo_campo == "nooki_caja") {
                    salida_ << "    nooki_caja_soltar(&valor->" << campo.nombre << ");\n";
                    con_dueno = true;
                } else if (tipo_campo.rfind("nooki_vector_", 0) == 0 ||
                           tipo_campo.rfind("nooki_mapa_", 0) == 0) {
                    salida_ << "    " << tipo_campo << "_soltar(&valor->" << campo.nombre
                            << ");\n";
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
        for (std::size_t i = 0; i < generico.tipo_parametros.size() && i < argumentos.size();
             ++i) {
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
            salida_ << "    " << nombre_c << "* objeto = (" << nombre_c
                    << "*)malloc(sizeof(" << nombre_c << "));\n";
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
    std::string implementacion_de(const std::string& clase,
                                  const std::string& metodo) const {
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
        return virtuales != virtuales_de_.end() ? virtuales->second
                                                : std::vector<std::string>{};
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

    void abrir_ambito() { ambitos_.emplace_back(); }
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
            salida_ << indentacion() << it->funcion << "("
                    << (it->por_puntero ? "" : "&") << it->nombre << ");\n";
        }
        ambito.liberables.clear();
    }

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
        const auto sustituido = sustitucion_.find(std::string(escrito));
        if (sustituido != sustitucion_.end()) {
            return tipo_declarado_c(sustituido->second);
        }
        const auto conocido = tipo_c_de_.find(std::string(escrito));
        if (conocido != tipo_c_de_.end()) {
            // Una clase es siempre una referencia: el tipo declarado lleva el puntero.
            const auto definicion = tipo_ptr_.find(std::string(escrito));
            const bool es_clase =
                definicion != tipo_ptr_.end() && definicion->second->es_clase;
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
            salida_ << "    " << estructura_actual_c_ << "* este = ("
                    << estructura_actual_c_ << "*)este_v;\n";
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
            salida_ << "int main(void) {\n";
        } else {
            salida_ << firma(funcion) << " {\n";
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
                std::set<std::string> conservar;
                if (sentencia.expresion) {
                    recoger_identificadores(*sentencia.expresion, conservar);
                }
                liberar_todo(conservar);
                if (sentencia.expresion) {
                    salida_ << indentacion() << "return " << expresion(*sentencia.expresion)
                            << ";\n";
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
                salida_ << indentacion() << "} while (" << condicion(sentencia.paso.get())
                        << ");\n";
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
            const auto enumeracion = enumeraciones_.find(
                tipo_resultado.rfind("nooki_", 0) == 0
                    ? tipo_resultado.substr(std::string("nooki_").size())
                    : tipo_resultado);
            if (enumeracion == enumeraciones_.end() ||
                enumeracion->second->variantes.size() < 2 ||
                enumeracion->second->variantes[0].campos.size() != 1) {
                no_soportado("«?» sobre un valor que no tiene forma de resultado");
                return;
            }
            const sintactico::Variante& exito = enumeracion->second->variantes[0];
            const std::string campo_exito = tipo_declarado_c(exito.campos[0].tipo);
            const std::string temporal =
                "__nooki_r" + std::to_string(contador_temporal_++);

            salida_ << indentacion() << tipo_resultado << " " << temporal << " = "
                    << expresion(*sentencia.expresion->izquierda) << ";\n";
            salida_ << indentacion() << "if (" << temporal << ".etiqueta != 0) {\n";
            salida_ << indentacion() << "    return " << temporal << ";\n";
            salida_ << indentacion() << "}\n";
            salida_ << indentacion() << campo_exito << " " << sentencia.nombre << " = "
                    << temporal << ".datos." << exito.nombre << "_" << exito.campos[0].nombre
                    << ";\n";
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

        salida_ << tipo << ' ' << sentencia.nombre << " = " << valor << ";\n";

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

        if (tipo == "nooki_caja" || tipo == "nooki_texto" ||
            tipo.rfind("nooki_vector_", 0) == 0 || tipo.rfind("nooki_mapa_", 0) == 0 ||
            compuesto_fresco || referencia_de_clase) {
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
            } else if (tipo.rfind("nooki_vector_", 0) == 0 ||
                       tipo.rfind("nooki_mapa_", 0) == 0 || compuesto_fresco) {
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
    void emitir_para_coleccion(const sintactico::Sentencia& sentencia,
                               const std::string& tipo) {
        const std::string nombre(sentencia.nombre);
        const std::string elemento =
            tipo.rfind("nooki_vector_texto", 0) == 0 ? "nooki_texto" : "long long";
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

        salida_ << indentacion() << elemento << " " << nombre << " = " << tipo_base
                << "_obtener(__nooki_col, " << indice << ");\n";

        abrir_ambito();
        declarar(sentencia.nombre, elemento);
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
            case TipoExpresion::Booleano:
                return "int";
            case TipoExpresion::Identificador:
                return buscar(expresion.lexema);
            case TipoExpresion::Construccion: {
                const auto conocido = tipo_c_de_.find(std::string(expresion.anotacion));
                const std::string tipo =
                    conocido != tipo_c_de_.end() ? conocido->second
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
                    const auto generico =
                        genericos_.find(std::string(expresion.izquierda->lexema));
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

                    // `super.metodo(...)`: el tipo lo da la clase base.
                    if (acceso.izquierda &&
                        acceso.izquierda->tipo == TipoExpresion::Super) {
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


                    if (acceso.izquierda &&
                        (tipo_de(*acceso.izquierda) == "nooki_texto" ||
                         tipo_de(*acceso.izquierda) == "nooki_texto*")) {
                        const std::string& metodo = acceso.anotacion;
                        if (metodo == "dividir") {
                            return "nooki_vector_texto";
                        }
                        if (metodo == "subtexto" || metodo == "mayusculas" ||
                            metodo == "minusculas" || metodo == "recortar") {
                            return "nooki_texto";
                        }
                        if (metodo == "contiene" || metodo == "empieza_con") {
                            return "int";
                        }
                        return "long long";
                    }
                    if (acceso.izquierda &&
                        tipo_de(*acceso.izquierda).rfind("nooki_vector_", 0) == 0) {
                        return acceso.anotacion == "agregar" ? "void" : "long long";
                    }
                    if (acceso.izquierda &&
                        acceso.izquierda->tipo == TipoExpresion::Identificador &&
                        acceso.izquierda->lexema == "Caja") {
                        return "nooki_caja";
                    }
                    if (acceso.izquierda &&
                        acceso.izquierda->tipo == TipoExpresion::Identificador) {
                        const auto modulo = modulos_.find(acceso.izquierda->lexema);
                        if (modulo != modulos_.end()) {
                            const auto funcion = modulo->second.find(acceso.anotacion);
                            if (funcion != modulo->second.end()) {
                                return tipo_declarado_c(funcion->second->tipo_retorno);
                            }
                        }
                    }
                    if (acceso.izquierda &&
                        acceso.izquierda->tipo == TipoExpresion::Identificador &&
                        acceso.izquierda->lexema == "Mapa") {
                        return "nooki_mapa_texto_entero";
                    }
                    if (acceso.izquierda &&
                        acceso.izquierda->tipo == TipoExpresion::Identificador &&
                        acceso.izquierda->lexema == "Vector") {
                        const bool de_texto =
                            !acceso.izquierda->tipos_aplicados.empty() &&
                            acceso.izquierda->tipos_aplicados.front() == "texto";
                        return de_texto ? "nooki_vector_texto" : "nooki_vector_entero";
                    }
                    if (acceso.izquierda &&
                        tipo_de(*acceso.izquierda).rfind("nooki_mapa_", 0) == 0) {
                        if (acceso.anotacion == "insertar") {
                            return "void";
                        }
                        if (acceso.anotacion == "claves") {
                            return "nooki_vector_texto";
                        }
                        return "long long";
                    }
                    if (acceso.izquierda &&
                        tipo_de(*acceso.izquierda).rfind("nooki_vector_", 0) == 0) {
                        return acceso.anotacion == "agregar" ? "void" : "long long";
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

                    if (const auto variante =
                            variantes_.find(std::string(expresion.izquierda->lexema));
                        variante != variantes_.end()) {
                        return "nooki_" + std::string(variante->second.first->nombre);
                    }
                    if (expresion.izquierda->lexema == "leer_linea") {
                        return "nooki_texto";
                    }
                    if (expresion.izquierda->lexema == "leer_entero") {
                        return "long long";
                    }
                    const auto encontrada =
                        retornos_.find(std::string(expresion.izquierda->lexema));
                    if (encontrada != retornos_.end()) {
                        return encontrada->second;
                    }
                }
                return "void";
            }
            case TipoExpresion::Unaria:
                return expresion.lexema == "!" ? "int" : "long long";
            case TipoExpresion::Binaria: {
                const std::string_view operador = expresion.lexema;
                const bool comparacion = operador == "==" || operador == "!=" ||
                                         operador == "<" || operador == ">" ||
                                         operador == "<=" || operador == ">=" ||
                                         operador == "&&" || operador == "||";
                if (!comparacion && expresion.izquierda && expresion.derecha &&
                    expresion.lexema == "+" &&
                    tipo_de(*expresion.izquierda) == "nooki_texto" &&
                    tipo_de(*expresion.derecha) == "nooki_texto") {
                    return "nooki_texto";
                }
                return comparacion ? "int" : "long long";
            }
            case TipoExpresion::Asignacion:
                return expresion.izquierda ? tipo_de(*expresion.izquierda) : "long long";
            default:
                return "long long";
        }
    }

    // --- Expresiones -------------------------------------------------------
    std::string expresion(const sintactico::Expresion& nodo) {
        using sintactico::TipoExpresion;

        switch (nodo.tipo) {
            case TipoExpresion::Texto:
                return "nooki_texto_literal(\"" + escapar_c(nodo.lexema) + "\")";
            case TipoExpresion::Entero:
                return limpiar_separadores(nodo.lexema);
            case TipoExpresion::Booleano:
                return nodo.lexema == "verdadero" ? "1" : "0";
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
                    salida_ << indentacion() << campo_c << " " << rama.patron.ligaduras[i]
                            << " = " << temporal << ".datos." << variante->nombre << "_"
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

        const std::string temporal =
            "__nooki_valor" + std::to_string(contador_temporal_++);

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
                if (!patron.valor.empty() && (patron.valor.front() == '"' ||
                                              patron.valor.front() == '\'')) {
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
            const auto enumeracion =
                enumeraciones_.find(std::string(nodo.izquierda->lexema));
            if (enumeracion != enumeraciones_.end()) {
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
            return "&" + expresion(*nodo.izquierda);
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
            no_soportado("«*» sobre algo que no es una «Caja»");
            return "0";
        }
        if (nodo.lexema != "-" && nodo.lexema != "!") {
            no_soportado("un operador unario que no sea «-», «!» o «*»");
            return "0";
        }
        if (!nodo.izquierda) {
            return "0";
        }
        return "(" + std::string(nodo.lexema) + expresion(*nodo.izquierda) + ")";
    }

    std::string binaria(const sintactico::Expresion& nodo) {
        if (!nodo.izquierda || !nodo.derecha) {
            no_soportado("un operador sin sus dos operandos");
            return "0";
        }

        const std::string_view operador = nodo.lexema;
        const std::string tipo_izquierda = tipo_de(*nodo.izquierda);
        const std::string tipo_derecha = tipo_de(*nodo.derecha);
        const bool ambos_texto =
            tipo_izquierda == "nooki_texto" && tipo_derecha == "nooki_texto";

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
            "+",  "-",  "*",  "/",  "%",  "==", "!=", "<", ">",  "<=", ">=", "&&", "||",
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

        return "(" + expresion(*nodo.izquierda) + " " + std::string(operador) + " " +
               expresion(*nodo.derecha) + ")";
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
        return expresion(*nodo.izquierda) + " " + std::string(nodo.lexema) + " " +
               expresion(*nodo.derecha);
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
                    const std::string objeto =
                        prestado ? "(*" + expresion(*acceso.izquierda) + ")"
                                 : expresion(*acceso.izquierda);
                    static const std::set<std::string> METODOS_TEXTO = {
                        "largo", "subtexto", "caracter", "buscar", "contiene",
                        "empieza_con", "a_entero", "dividir", "mayusculas",
                        "minusculas", "recortar"};
                    if (METODOS_TEXTO.count(std::string(acceso.anotacion)) != 0) {
                        std::string llamada =
                            "nooki_texto_" + std::string(acceso.anotacion) + "(" + objeto;
                        for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                            if (!argumento) {
                                continue;
                            }
                            llamada += ", ";
                            llamada += expresion(*argumento);
                        }
                        llamada += ')';
                        return llamada;
                    }
                }
            }
            if (acceso.izquierda &&
                acceso.izquierda->tipo == sintactico::TipoExpresion::Identificador &&
                acceso.izquierda->lexema == "Mapa" && acceso.anotacion == "nuevo") {
                return "nooki_mapa_texto_entero_nuevo()";
            }
            if (acceso.izquierda &&
                acceso.izquierda->tipo == sintactico::TipoExpresion::Identificador &&
                acceso.izquierda->lexema == "Vector" && acceso.anotacion == "nuevo") {
                const bool de_texto = !acceso.izquierda->tipos_aplicados.empty() &&
                                      acceso.izquierda->tipos_aplicados.front() == "texto";
                return de_texto ? "nooki_vector_texto_nuevo()"
                                : "nooki_vector_entero_nuevo()";
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
                                     "){ .etiqueta = " + std::to_string(indice) +
                                     ", .datos = {";
                for (std::size_t i = 0;
                     i < datos.campos.size() && i < nodo.lista.size(); ++i) {
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
            const char* funcion = tipo == "nooki_texto" ? "nooki_imprimir_texto"
                                  : (tipo == "long long" || tipo == "int")
                                      ? "nooki_imprimir_entero"
                                      : nullptr;
            if (funcion == nullptr) {
                no_soportado("«imprimir» con un valor que el backend no sabe imprimir");
                return "0";
            }
            return std::string(funcion) + "(" + expresion(argumento) + ")";
        }

        // Dentro de un módulo, una llamada a otro símbolo suyo lleva su prefijo.
        std::string nombre_para_c = nombre_c(nombre_llamado);
        if (!modulo_actual_.empty()) {
            const auto modulo = modulos_.find(modulo_actual_);
            if (modulo != modulos_.end() &&
                modulo->second.count(nombre_llamado) != 0) {
                nombre_para_c = "nooki_" + std::string(modulo_actual_) + "_" +
                                std::string(nombre_llamado);
            }
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
            // Un literal que llega a un parámetro prestado necesita dirección propia.
            if (declarados != parametros_.end() && i < declarados->second.size()) {
                const std::string& declarado = declarados->second[i];
                if (!declarado.empty() && declarado.front() == '&' &&
                    (argumento->tipo == sintactico::TipoExpresion::Texto ||
                     argumento->tipo == sintactico::TipoExpresion::Entero)) {
                    valor = argumento_prestado(*argumento, declarado);
                }
            }
            salida += valor;
        }
        salida += ')';

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
            salida_ << indentacion() << nombre << ".propio = 0;\n";
        } else if (tipo.rfind("nooki_vector_", 0) == 0 ||
                   tipo.rfind("nooki_mapa_", 0) == 0) {
            salida_ << indentacion() << nombre << ".datos = NULL;\n";
            salida_ << indentacion() << nombre << ".largo = 0;\n";
            salida_ << indentacion() << nombre << ".capacidad = 0;\n";
        }
    }

    void emitir_movimientos(const sintactico::Expresion& nodo,
                            const std::vector<sintactico::Parametro>& declarados) {
        for (std::size_t i = 0; i < nodo.lista.size() && i < declarados.size(); ++i) {
            const sintactico::ExpresionPtr& argumento = nodo.lista[i];
            if (!argumento ||
                argumento->tipo != sintactico::TipoExpresion::Identificador) {
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
        const bool objeto_puntero = !tipo_objeto.empty() && tipo_objeto.back() == '*';
        if (objeto_puntero) {
            tipo_objeto.pop_back();
        }


        if (tipo_objeto.rfind("nooki_vector_", 0) == 0 ||
            tipo_objeto.rfind("nooki_mapa_", 0) == 0) {
            // Si llega como préstamo (`&Vector<T>`), ya es un puntero.
            std::string salida = tipo_objeto + "_" + std::string(acceso.anotacion) + "(";
            salida += objeto_puntero ? expresion(*acceso.izquierda)
                                     : "&" + expresion(*acceso.izquierda);
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
            const bool toma_dueno =
                (acceso.anotacion == "agregar" && nodo.lista.size() == 1) ||
                (acceso.anotacion == "insertar" && nodo.lista.size() == 2);
            if (toma_dueno && nodo.lista.front() &&
                tipo_de(*nodo.lista.front()) == "nooki_texto" &&
                (nodo.lista.front()->tipo == sintactico::TipoExpresion::Identificador ||
                 nodo.lista.front()->tipo == sintactico::TipoExpresion::Miembro)) {
                // Vale un nombre o un campo: en ambos casos el origen se vacía.
                const std::string origen = expresion(*nodo.lista.front());
                salida = "(" + salida + ", " + origen + ".datos = NULL, " + origen +
                         ".propio = 0)";
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
        const bool virtual_aqui =
            objeto_puntero && tipo_ptr != por_c_.end() &&
            std::find(virtuales.begin(), virtuales.end(), std::string(acceso.anotacion)) !=
                virtuales.end();

        std::string salida;
        if (virtual_aqui) {
            salida = "((const " + raiz_c(tipo_objeto) + "_vtabla*)" +
                     expresion(*acceso.izquierda) + "->vtabla)->" + std::string(acceso.anotacion) +
                     "((void*)" + expresion(*acceso.izquierda);
        } else {
            salida = tipo_objeto + "_" + std::string(acceso.anotacion) + "(";
            salida += objeto_puntero ? expresion(*acceso.izquierda)
                                     : "&" + expresion(*acceso.izquierda);
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
    std::set<std::string> compuestos_con_dueno_;
    std::vector<std::size_t> bucles_;
    std::map<std::string, std::map<std::string, const sintactico::Funcion*>> metodos_;
    std::map<std::string, const sintactico::Enumeracion*> enumeraciones_;
    std::map<std::string,
             std::pair<const sintactico::Enumeracion*, std::size_t>> variantes_;
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
