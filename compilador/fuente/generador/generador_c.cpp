#include "nooki/generador/generador_c.hpp"

#include <sstream>
#include <string>
#include <string_view>

namespace nooki::generador {
namespace {

/// Escapa un texto de Nooki para que sea un literal de C válido.
/// Los bytes UTF-8 se copian tal cual: el compilador de C los conserva.
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

/// Traduce un tipo de Nooki a su equivalente en C.
std::string tipo_c(std::string_view tipo) {
    if (tipo == "vacio") {
        return "void";
    }
    if (tipo == "booleano") {
        return "int";
    }
    if (tipo == "texto") {
        return "const char*";
    }
    if (tipo == "flotante" || tipo == "flotante32") {
        return "double";
    }
    if (tipo == "byte" || tipo == "natural8") {
        return "unsigned char";
    }
    if (tipo == "natural" || tipo == "tamano") {
        return "unsigned long long";
    }
    return "long long";
}

/// `1_000` en Nooki es `1000` en C: los separadores son solo legibilidad.
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

const char* INDENTACION = "    ";

class EmisorC {
public:
    std::string generar(const sintactico::Programa& programa) {
        salida_ << "/* Generado por Nooki. No editar: se regenera en cada compilación. */\n";
        salida_ << "#include <stdio.h>\n\n";
        salida_ << "static void nooki_imprimir(const char* texto) {\n";
        salida_ << INDENTACION << "fputs(texto, stdout);\n";
        salida_ << INDENTACION << "fputc('\\n', stdout);\n";
        salida_ << "}\n";

        for (const sintactico::Funcion& funcion : programa.funciones) {
            salida_ << '\n';
            emitir_funcion(funcion);
        }
        return salida_.str();
    }

private:
    void emitir_funcion(const sintactico::Funcion& funcion) {
        const bool principal = funcion.nombre == "principal";

        if (principal) {
            salida_ << "int main(void) {\n";
        } else {
            salida_ << "static " << tipo_c(funcion.tipo_retorno) << ' ' << nombre_c(funcion.nombre)
                    << "(void) {\n";
        }

        for (const sintactico::Sentencia& sentencia : funcion.cuerpo) {
            emitir_sentencia(sentencia, principal);
        }

        // `main` devuelve int: si el cuerpo no termina en `retornar`, se añade.
        const bool termina_en_retorno =
            !funcion.cuerpo.empty() &&
            funcion.cuerpo.back().tipo == sintactico::TipoSentencia::Retornar;
        if (principal && !termina_en_retorno) {
            salida_ << INDENTACION << "return 0;\n";
        }
        salida_ << "}\n";
    }

    void emitir_sentencia(const sintactico::Sentencia& sentencia, bool principal) {
        if (sentencia.tipo == sintactico::TipoSentencia::Retornar) {
            if (sentencia.expresion) {
                salida_ << INDENTACION << "return " << expresion(*sentencia.expresion) << ";\n";
            } else {
                salida_ << INDENTACION << (principal ? "return 0;\n" : "return;\n");
            }
            return;
        }
        if (sentencia.expresion) {
            salida_ << INDENTACION << expresion(*sentencia.expresion) << ";\n";
        }
    }

    std::string expresion(const sintactico::Expresion& expresion) {
        switch (expresion.tipo) {
            case sintactico::TipoExpresion::Texto:
                return "\"" + escapar_c(expresion.lexema) + "\"";
            case sintactico::TipoExpresion::Entero:
                return limpiar_separadores(expresion.lexema);
            case sintactico::TipoExpresion::Identificador:
                return nombre_c(expresion.lexema);
            case sintactico::TipoExpresion::Llamada:
                return llamada(expresion);
        }
        return "0";
    }

    std::string llamada(const sintactico::Expresion& llamada_expresion) {
        std::string destino = "nooki_desconocido";
        if (llamada_expresion.destino &&
            llamada_expresion.destino->tipo == sintactico::TipoExpresion::Identificador) {
            destino = nombre_c(llamada_expresion.destino->lexema);
        }

        std::string salida = destino + "(";
        bool primero = true;
        for (const sintactico::ExpresionPtr& argumento : llamada_expresion.argumentos) {
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

    std::ostringstream salida_;
};

}  // namespace

std::string generar_c(const sintactico::Programa& programa) {
    EmisorC emisor;
    return emisor.generar(programa);
}

}  // namespace nooki::generador
