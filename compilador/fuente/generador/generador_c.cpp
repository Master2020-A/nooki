#include "nooki/generador/generador_c.hpp"

#include <map>
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

bool es_comparacion(std::string_view operador) {
    return operador == "==" || operador == "!=" || operador == "<" || operador == ">" ||
           operador == "<=" || operador == ">=";
}

class EmisorC {
public:
    std::string generar(const sintactico::Programa& programa) {
        registrar(programa);

        salida_ << PRELUDIO;
        emitir_enumeraciones(programa);
        emitir_tipos(programa);

        for (const sintactico::Funcion& funcion : programa.funciones) {
            if (funcion.nombre == "principal") {
                continue;
            }
            salida_ << firma(funcion) << ";\n";
        }
        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            for (const sintactico::Funcion& metodo : definicion.metodos) {
                salida_ << firma_metodo(definicion, metodo) << ";\n";
            }
        }

        for (const sintactico::DefinicionTipo& definicion : programa.tipos) {
            for (const sintactico::Funcion& metodo : definicion.metodos) {
                salida_ << '\n';
                emitir_metodo(definicion, metodo);
            }
        }

        for (const sintactico::Funcion& funcion : programa.funciones) {
            salida_ << '\n';
            emitir_funcion(funcion);
        }
        return salida_.str();
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
        }
        for (const sintactico::Enumeracion& enumeracion : programa.enumeraciones) {
            enumeraciones_[std::string(enumeracion.nombre)] = &enumeracion;
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

        if (tipo == "nooki_caja" || tipo == "nooki_texto") {
            // Asignar un valor con dueño transfiere la propiedad: el origen queda vacío.
            if (sentencia.expresion->tipo == sintactico::TipoExpresion::Identificador) {
                const std::string& origen = std::string(sentencia.expresion->lexema);
                salida_ << indentacion() << origen << ".datos = NULL;\n";
                if (tipo == "nooki_texto") {
                    salida_ << indentacion() << origen << ".propio = 0;\n";
                }
            }
            registrar_liberable(sentencia.nombre,
                                tipo == "nooki_caja" ? "nooki_caja_soltar" : "nooki_texto_soltar");
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
        if (rango == nullptr || rango->tipo != sintactico::TipoExpresion::Binaria ||
            (rango->lexema != ".." && rango->lexema != "..=")) {
            no_soportado("un «para» que no recorre un rango");
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
                if (expresion.izquierda->tipo == TipoExpresion::Miembro) {
                    const sintactico::Expresion& acceso = *expresion.izquierda;
                    if (acceso.izquierda && acceso.anotacion == "largo" &&
                        tipo_de(*acceso.izquierda) == "nooki_texto") {
                        return "long long";
                    }
                    if (acceso.izquierda &&
                        acceso.izquierda->tipo == TipoExpresion::Identificador &&
                        acceso.izquierda->lexema == "Caja") {
                        return "nooki_caja";
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
            if (acceso.izquierda && acceso.anotacion == "largo" &&
                tipo_de(*acceso.izquierda) == "nooki_texto") {
                return "nooki_texto_largo(" + expresion(*acceso.izquierda) + ")";
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

    /// `p.metodo(a, b)` → `nooki_punto_metodo(&p, a, b)`.
    std::string llamada_metodo(const sintactico::Expresion& nodo) {
        const sintactico::Expresion& acceso = *nodo.izquierda;
        if (!acceso.izquierda) {
            no_soportado("una llamada a un método sin objeto");
            return "0";
        }

        const std::string tipo_objeto = tipo_de(*acceso.izquierda);
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
    std::map<std::string, std::map<std::string, const sintactico::Funcion*>> metodos_;
    std::map<std::string, const sintactico::Enumeracion*> enumeraciones_;
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
