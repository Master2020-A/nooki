#include "nooki/generador/generador_c.hpp"

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
        // Primera pasada: nombres, para que un tipo pueda referirse a otro declarado después.
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            campos_[std::string(definicion.nombre)];
            for (const sintactico::Funcion& metodo : definicion.metodos) {
                metodos_[std::string(definicion.nombre)][std::string(metodo.nombre)] = &metodo;
            }
        }
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            for (const sintactico::Campo& campo : definicion.campos) {
                campos_[std::string(definicion.nombre)][std::string(campo.nombre)] =
                    tipo_declarado_c(campo.tipo);
            }
        }
        for (const sintactico::Funcion& funcion : programa.funciones) {
            retornos_[std::string(funcion.nombre)] = tipo_c(funcion.tipo_retorno);

            std::vector<std::string> tipos;
            tipos.reserve(funcion.parametros.size());
            for (const sintactico::Parametro& parametro : funcion.parametros) {
                tipos.emplace_back(parametro.tipo);
            }
            parametros_[std::string(funcion.nombre)] = std::move(tipos);
        }
        for (const sintactico::Enumeracion& enumeracion : programa.enumeraciones) {
            enumeraciones_[std::string(enumeracion.nombre)] = &enumeracion;
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
            salida_ << "typedef int nooki_" << enumeracion.nombre << ";\n";
            salida_ << "enum {";
            for (std::size_t i = 0; i < enumeracion.variantes.size(); ++i) {
                if (i != 0) {
                    salida_ << ", ";
                }
                salida_ << constante_enum(enumeracion.nombre, enumeracion.variantes[i]) << " = "
                        << i;
            }
            salida_ << "};\n\n";
        }
    }

    void emitir_tipos(const sintactico::Programa& programa) {
        if (programa.tipos.empty()) {
            return;
        }
        salida_ << '\n';
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            salida_ << "typedef struct " << tipo_estructura(definicion.nombre) << " {\n";
            for (const sintactico::Campo& campo : definicion.campos) {
                salida_ << "    " << tipo_declarado_c(campo.tipo) << ' ' << campo.nombre << ";\n";
            }
            salida_ << "} " << tipo_estructura(definicion.nombre) << ";\n\n";
        }
    }

    // --- Ámbito ------------------------------------------------------------
    struct Ambito {
        std::map<std::string, std::string> tipos;
        // nombre + función de liberación, en orden de declaración
        std::vector<std::pair<std::string, std::string>> liberables;
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
    void liberar_cajas(Ambito& ambito) {
        for (auto it = ambito.liberables.rbegin(); it != ambito.liberables.rend(); ++it) {
            salida_ << indentacion() << it->second << "(&" << it->first << ");\n";
        }
        ambito.liberables.clear();
    }

    /// Antes de un `retornar` hay que liberar todas las cajas vivas.
    void liberar_todo() {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            liberar_cajas(*it);
        }
    }

    void declarar(std::string_view nombre, std::string tipo) {
        if (ambitos_.empty()) {
            return;
        }
        Ambito& ambito = ambitos_.back();
        ambito.tipos[std::string(nombre)] = std::move(tipo);
    }

    void registrar_liberable(std::string_view nombre, std::string funcion) {
        if (!ambitos_.empty()) {
            ambitos_.back().liberables.emplace_back(std::string(nombre), std::move(funcion));
        }
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
        if (campos_.find(std::string(escrito)) != campos_.end()) {
            return tipo_estructura(escrito);
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
        salida += nombre_c(funcion.nombre);
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
        const std::string estructura = tipo_estructura(definicion.nombre);
        std::string salida = "static ";
        salida += tipo_declarado_c(metodo.tipo_retorno);
        salida += ' ';
        salida += estructura + "_" + std::string(metodo.nombre);
        salida += '(';
        salida += estructura + "* este";

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
        estructura_actual_c_ = tipo_estructura(definicion.nombre);

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
        salida_ << "}\n";
        cerrar_ambito();
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

            case TipoSentencia::Retornar:
                liberar_todo();
                if (sentencia.expresion) {
                    salida_ << indentacion() << "return " << expresion(*sentencia.expresion)
                            << ";\n";
                } else {
                    salida_ << indentacion() << (principal ? "return 0;\n" : "return;\n");
                }
                return;

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
                emitir_cuerpo(sentencia.cuerpo);
                salida_ << indentacion() << "}\n";
                return;

            case TipoSentencia::Para:
                emitir_para(sentencia);
                return;

            case TipoSentencia::Hacer:
                salida_ << indentacion() << "do {\n";
                emitir_cuerpo(sentencia.cuerpo);
                salida_ << indentacion() << "} while (" << condicion(sentencia.paso.get())
                        << ");\n";
                return;

            case TipoSentencia::Bucle:
                salida_ << indentacion() << "for (;;) {\n";
                emitir_cuerpo(sentencia.cuerpo);
                salida_ << indentacion() << "}\n";
                return;

            case TipoSentencia::Romper:
                salida_ << indentacion() << "break;\n";
                return;

            case TipoSentencia::Continuar:
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

        const std::string tipo = sentencia.tipo_declarado.empty()
                                     ? tipo_de(*sentencia.expresion)
                                     : tipo_declarado_c(sentencia.tipo_declarado);

        if (sentencia.tipo == sintactico::TipoSentencia::Constante) {
            salida_ << indentacion() << "const ";
        } else {
            salida_ << indentacion();
        }

        salida_ << tipo << ' ' << sentencia.nombre << " = " << expresion(*sentencia.expresion)
                << ";\n";

        emitir_movimientos_de_llamada(*sentencia.expresion);

        if (tipo == "nooki_caja" || tipo == "nooki_texto" ||
            tipo.rfind("nooki_vector_", 0) == 0) {
            // Asignar un valor con dueño transfiere la propiedad: el origen queda vacío.
            if (sentencia.expresion->tipo == sintactico::TipoExpresion::Identificador) {
                const std::string& origen = std::string(sentencia.expresion->lexema);
                salida_ << indentacion() << origen << ".datos = NULL;\n";
                if (tipo == "nooki_texto") {
                    salida_ << indentacion() << origen << ".propio = 0;\n";
                }
            }
            std::string liberar = "nooki_texto_soltar";
            if (tipo == "nooki_caja") {
                liberar = "nooki_caja_soltar";
            } else if (tipo.rfind("nooki_vector_", 0) == 0) {
                liberar = tipo + "_soltar";
            }
            registrar_liberable(sentencia.nombre, std::move(liberar));
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
        for (const sintactico::Sentencia& hija : sentencia.cuerpo) {
            emitir_sentencia(hija, false);
        }
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
            case TipoExpresion::Construccion:
                return tipo_estructura(expresion.anotacion);
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
                const std::string objeto = tipo_de(*expresion.izquierda);
                constexpr std::string_view PREFIJO = "nooki_";
                if (objeto.rfind(PREFIJO, 0) != 0) {
                    return "long long";
                }
                const std::string nombre_estructura = objeto.substr(PREFIJO.size());

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
                    if (acceso.izquierda && acceso.anotacion == "largo" &&
                        (tipo_de(*acceso.izquierda) == "nooki_texto" ||
                         tipo_de(*acceso.izquierda) == "nooki_texto*")) {
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
                        acceso.izquierda->tipo == TipoExpresion::Identificador &&
                        acceso.izquierda->lexema == "Vector") {
                        const bool de_texto =
                            !acceso.izquierda->tipos_aplicados.empty() &&
                            acceso.izquierda->tipos_aplicados.front() == "texto";
                        return de_texto ? "nooki_vector_texto" : "nooki_vector_entero";
                    }
                    const std::string tipo_objeto =
                        acceso.izquierda ? tipo_de(*acceso.izquierda) : std::string{};
                    constexpr std::string_view PREFIJO = "nooki_";
                    if (tipo_objeto.rfind(PREFIJO, 0) == 0) {
                        const auto metodos = metodos_.find(tipo_objeto.substr(PREFIJO.size()));
                        if (metodos != metodos_.end()) {
                            const auto metodo = metodos->second.find(acceso.anotacion);
                            if (metodo != metodos->second.end()) {
                                return tipo_declarado_c(metodo->second->tipo_retorno);
                            }
                        }
                    }
                    return "void";
                }
                if (expresion.izquierda->tipo == TipoExpresion::Identificador) {
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
        const std::string tipo = tipo_estructura(nodo.anotacion);
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

    void emitir_coincide(const sintactico::Sentencia& sentencia) {
        if (!sentencia.expresion) {
            no_soportado("un «coincide» sin valor que examinar");
            return;
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
            if (acceso.izquierda && acceso.anotacion == "largo") {
                const std::string tipo = tipo_de(*acceso.izquierda);
                if (tipo == "nooki_texto") {
                    return "nooki_texto_largo(" + expresion(*acceso.izquierda) + ")";
                }
                if (tipo == "nooki_texto*") {
                    return "nooki_texto_largo(*" + expresion(*acceso.izquierda) + ")";
                }
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

        std::string salida = nombre_c(nombre_llamado) + "(";
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

    /// Pasar un valor con dueño por valor (a un parámetro que no es préstamo) lo mueve:
    /// el origen queda vacío para que nadie lo libere dos veces.
    void emitir_movimientos_de_llamada(const sintactico::Expresion& nodo) {
        if (nodo.tipo != sintactico::TipoExpresion::Llamada || !nodo.izquierda) {
            return;
        }
        if (nodo.izquierda->tipo != sintactico::TipoExpresion::Identificador) {
            return;
        }
        emitir_movimientos(nodo, nodo.izquierda->lexema);
    }

    void emitir_movimientos(const sintactico::Expresion& nodo,
                            std::string_view nombre_funcion) {
        const auto declarados = parametros_.find(std::string(nombre_funcion));
        if (declarados == parametros_.end()) {
            return;
        }

        std::string extras;
        for (std::size_t i = 0; i < nodo.lista.size() && i < declarados->second.size(); ++i) {
            const sintactico::ExpresionPtr& argumento = nodo.lista[i];
            if (!argumento ||
                argumento->tipo != sintactico::TipoExpresion::Identificador) {
                continue;
            }
            const std::string& declarado = declarados->second[i];
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

        const std::string tipo_objeto = tipo_de(*acceso.izquierda);

        if (tipo_objeto.rfind("nooki_vector_", 0) == 0) {
            std::string salida = tipo_objeto + "_" + std::string(acceso.anotacion) + "(&" +
                                 expresion(*acceso.izquierda);
            for (const sintactico::ExpresionPtr& argumento : nodo.lista) {
                if (!argumento) {
                    continue;
                }
                salida += ", ";
                salida += expresion(*argumento);
            }
            salida += ')';

            // `agregar` toma el dueño del elemento: el origen queda vacío.
            if (acceso.anotacion == "agregar" && nodo.lista.size() == 1 && nodo.lista.front() &&
                tipo_de(*nodo.lista.front()) == "nooki_texto" &&
                nodo.lista.front()->tipo == sintactico::TipoExpresion::Identificador) {
                const std::string origen(nodo.lista.front()->lexema);
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

        std::string salida = tipo_objeto + "_" + std::string(acceso.anotacion) + "(&" +
                             expresion(*acceso.izquierda);

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
    std::map<std::string, std::map<std::string, std::string>> campos_;
    std::map<std::string, std::string> retornos_;
    /// Tipos declarados de los parámetros, para saber cuáles son préstamos.
    std::map<std::string, std::vector<std::string>> parametros_;
    std::map<std::string, std::map<std::string, const sintactico::Funcion*>> metodos_;
    std::map<std::string, const sintactico::Enumeracion*> enumeraciones_;
    std::map<std::string, const sintactico::Funcion*> genericos_;
    std::map<std::string, std::pair<std::string, std::vector<std::string_view>>> instancias_;
    std::map<std::string, std::string> sustitucion_;
    std::vector<Ambito> ambitos_;
    std::string estructura_actual_c_;
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
