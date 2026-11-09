#include "nooki/generador/generador_c.hpp"

#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/semantico/tipos.hpp"

namespace nooki::generador {
namespace {

/// El generador usa el mismo tipo que el verificador: una sola definición de
/// «qué es un entero» para todo el compilador.
using TipoValor = nooki::semantico::Tipo;

const char* tipo_c(TipoValor tipo) {
    switch (tipo) {
        case TipoValor::Texto:
            return "const char*";
        case TipoValor::Booleano:
            return "int";
        case TipoValor::Vacio:
            return "void";
        case TipoValor::Desconocido:
        case TipoValor::Entero:
            return "long long";
    }
    return "long long";
}

bool es_comparacion(std::string_view op) {
    return op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=";
}

bool es_logico(std::string_view op) { return op == "&&" || op == "||"; }

bool es_aritmetico(std::string_view op) {
    return op == "+" || op == "-" || op == "*" || op == "/" || op == "%" || op == "**";
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

TipoValor tipo_declarado(std::string_view tipo) {
    const TipoValor conocido = nooki::semantico::desde_nombre(tipo);
    // Un tipo desconocido no debería llegar aquí: el verificador ya lo rechazó.
    return conocido == TipoValor::Desconocido ? TipoValor::Entero : conocido;
}

const char* PRELUDIO =
    "/* Generado por Nooki. No editar: se regenera en cada compilación. */\n"
    "#include <stdio.h>\n"
    "\n"
    "static void nooki_imprimir_texto(const char* texto) {\n"
    "    fputs(texto, stdout);\n"
    "    fputc('\\n', stdout);\n"
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

class EmisorC {
public:
    std::string generar(const sintactico::Programa& programa) {
        // Tipos de retorno conocidos: permiten inferir el tipo de una llamada.
        for (const sintactico::Funcion& funcion : programa.funciones) {
            funciones_[std::string(funcion.nombre)] = tipo_declarado(funcion.tipo_retorno);
        }

        salida_ << PRELUDIO;

        // Declaraciones anticipadas: una función puede llamar a otra definida después.
        for (const sintactico::Funcion& funcion : programa.funciones) {
            if (funcion.nombre == "principal") {
                continue;
            }
            salida_ << firma(funcion) << ";\n";
        }

        for (const sintactico::Funcion& funcion : programa.funciones) {
            salida_ << '\n';
            emitir_funcion(funcion);
        }
        return salida_.str();
    }

    /// Firma de C de una función de Nooki: `static long long nooki_f(long long n)`.
    std::string firma(const sintactico::Funcion& funcion) const {
        std::string salida = "static ";
        salida += tipo_c(tipo_declarado(funcion.tipo_retorno));
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
                salida += tipo_c(tipo_declarado(parametro.tipo));
                salida += ' ';
                salida += parametro.nombre;
            }
        }

        salida += ')';
        return salida;
    }

    [[nodiscard]] bool soportado() const noexcept { return soportado_; }
    [[nodiscard]] const std::string& motivo() const noexcept { return motivo_; }

private:
    // --- Ámbito de tipos ---------------------------------------------------
    void abrir_ambito() { ambitos_.emplace_back(); }
    void cerrar_ambito() {
        if (!ambitos_.empty()) {
            ambitos_.pop_back();
        }
    }
    void declarar(std::string_view nombre, TipoValor tipo) {
        if (!ambitos_.empty()) {
            ambitos_.back()[std::string(nombre)] = tipo;
        }
    }
    TipoValor buscar(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            const auto encontrado = it->find(std::string(nombre));
            if (encontrado != it->end()) {
                return encontrado->second;
            }
        }
        return TipoValor::Desconocido;
    }

    // --- Utilidades --------------------------------------------------------
    std::string indentacion() const { return std::string(static_cast<std::size_t>(nivel_) * 4, ' '); }

    void no_soportado(std::string descripcion) {
        if (soportado_) {
            soportado_ = false;
            motivo_ = std::move(descripcion);
        }
    }

    // --- Funciones ---------------------------------------------------------
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
            declarar(parametro.nombre, tipo_declarado(parametro.tipo));
        }
        for (const sintactico::Sentencia& sentencia : funcion.cuerpo) {
            emitir_sentencia(sentencia, principal);
        }
        --nivel_;

        const bool termina_en_retorno =
            !funcion.cuerpo.empty() &&
            funcion.cuerpo.back().tipo == sintactico::TipoSentencia::Retornar;
        if (principal && !termina_en_retorno) {
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

            case TipoSentencia::Mientras: {
                salida_ << indentacion() << "while (" << condicion(sentencia.expresion.get())
                        << ") {\n";
                emitir_cuerpo(sentencia.cuerpo);
                salida_ << indentacion() << "}\n";
                return;
            }

            case TipoSentencia::Para:
                emitir_para(sentencia);
                return;

            case TipoSentencia::Hacer: {
                salida_ << indentacion() << "do {\n";
                emitir_cuerpo(sentencia.cuerpo);
                salida_ << indentacion() << "} while (" << condicion(sentencia.paso.get())
                        << ");\n";
                return;
            }

            case TipoSentencia::Bucle: {
                salida_ << indentacion() << "for (;;) {\n";
                emitir_cuerpo(sentencia.cuerpo);
                salida_ << indentacion() << "}\n";
                return;
            }

            case TipoSentencia::Romper:
                salida_ << indentacion() << "break;\n";
                return;

            case TipoSentencia::Continuar:
                salida_ << indentacion() << "continue;\n";
                return;

            case TipoSentencia::Bloque: {
                salida_ << indentacion() << "{\n";
                emitir_cuerpo(sentencia.cuerpo);
                salida_ << indentacion() << "}\n";
                return;
            }
        }
    }

    void emitir_declaracion(const sintactico::Sentencia& sentencia) {
        if (!sentencia.expresion) {
            no_soportado("una declaración sin valor inicial");
            return;
        }

        const TipoValor tipo = sentencia.tipo_declarado.empty()
                                   ? tipo_de(*sentencia.expresion)
                                   : tipo_declarado(sentencia.tipo_declarado);

        if (sentencia.tipo == sintactico::TipoSentencia::Constante) {
            salida_ << indentacion() << "const ";
        } else {
            salida_ << indentacion();
        }

        salida_ << tipo_c(tipo) << ' ' << sentencia.nombre << " = "
                << expresion(*sentencia.expresion) << ";\n";

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
        declarar(sentencia.nombre, TipoValor::Entero);
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

    // --- Expresiones -------------------------------------------------------
    TipoValor tipo_de(const sintactico::Expresion& expresion) const {
        using sintactico::TipoExpresion;

        switch (expresion.tipo) {
            case TipoExpresion::Entero:
                return TipoValor::Entero;
            case TipoExpresion::Texto:
                return TipoValor::Texto;
            case TipoExpresion::Booleano:
                return TipoValor::Booleano;
            case TipoExpresion::Identificador:
                return buscar(expresion.lexema);
            case TipoExpresion::Unaria:
                return expresion.lexema == "!" ? TipoValor::Booleano : TipoValor::Entero;
            case TipoExpresion::Binaria:
                return (es_comparacion(expresion.lexema) || es_logico(expresion.lexema))
                           ? TipoValor::Booleano
                           : TipoValor::Entero;
            case TipoExpresion::Asignacion:
                return expresion.izquierda ? tipo_de(*expresion.izquierda) : TipoValor::Desconocido;
            case TipoExpresion::Llamada: {
                if (expresion.izquierda &&
                    expresion.izquierda->tipo == TipoExpresion::Identificador) {
                    const auto encontrada =
                        funciones_.find(std::string(expresion.izquierda->lexema));
                    if (encontrada != funciones_.end()) {
                        return encontrada->second;
                    }
                }
                return TipoValor::Desconocido;
            }
            default:
                return TipoValor::Desconocido;
        }
    }

    std::string expresion(const sintactico::Expresion& nodo) {
        using sintactico::TipoExpresion;

        switch (nodo.tipo) {
            case TipoExpresion::Texto:
                return "\"" + escapar_c(nodo.lexema) + "\"";
            case TipoExpresion::Entero:
                return limpiar_separadores(nodo.lexema);
            case TipoExpresion::Booleano:
                return nodo.lexema == "verdadero" ? "1" : "0";
            case TipoExpresion::Identificador:
                return std::string(nodo.lexema);
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

    std::string unaria(const sintactico::Expresion& nodo) {
        if (nodo.lexema != "-" && nodo.lexema != "!") {
            no_soportado("un operador unario que no sea «-» o «!»");
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

        if (operador == "**") {
            return "nooki_potencia(" + expresion(*nodo.izquierda) + ", " +
                   expresion(*nodo.derecha) + ")";
        }
        if (!es_aritmetico(operador) && !es_comparacion(operador) && !es_logico(operador)) {
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
        if (nodo.izquierda->tipo != sintactico::TipoExpresion::Identificador) {
            no_soportado("una asignación a algo que no es una variable");
            return "0";
        }
        return std::string(nodo.izquierda->lexema) + " " + std::string(nodo.lexema) + " " +
               expresion(*nodo.derecha);
    }

    std::string llamada(const sintactico::Expresion& nodo) {
        if (!nodo.izquierda ||
            nodo.izquierda->tipo != sintactico::TipoExpresion::Identificador) {
            no_soportado("una llamada a algo que no es un nombre");
            return "0";
        }

        const std::string_view nombre = nodo.izquierda->lexema;

        if (nombre == "imprimir") {
            if (nodo.lista.size() != 1 || !nodo.lista.front()) {
                no_soportado("«imprimir» con un número de argumentos distinto de uno");
                return "0";
            }
            const sintactico::Expresion& argumento = *nodo.lista.front();
            const TipoValor tipo = tipo_de(argumento);
            if (tipo == TipoValor::Desconocido) {
                no_soportado("«imprimir» con un valor de tipo desconocido");
                return "0";
            }
            const char* funcion =
                tipo == TipoValor::Texto ? "nooki_imprimir_texto" : "nooki_imprimir_entero";
            return std::string(funcion) + "(" + expresion(argumento) + ")";
        }

        std::string salida = nombre_c(nombre) + "(";
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

    static std::string descripcion_expresion(sintactico::TipoExpresion tipo) {
        switch (tipo) {
            case sintactico::TipoExpresion::Caracter:
                return "un literal de carácter";
            case sintactico::TipoExpresion::Flotante:
                return "un número flotante";
            case sintactico::TipoExpresion::Nulo:
                return "el valor nulo";
            case sintactico::TipoExpresion::Miembro:
                return "el acceso a un miembro";
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
    std::vector<std::map<std::string, TipoValor>> ambitos_;
    std::map<std::string, TipoValor> funciones_;
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
