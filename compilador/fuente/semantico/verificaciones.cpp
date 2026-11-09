#include "nooki/semantico/verificaciones.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/codigos.hpp"
#include "nooki/semantico/tipos.hpp"

namespace nooki::semantico {
namespace {

using sintactico::Expresion;
using sintactico::Funcion;
using sintactico::Sentencia;
using sintactico::TipoExpresion;
using sintactico::TipoSentencia;

class Verificador {
public:
    Verificador(const sintactico::Programa& programa, diagnostico::Emisor& emisor)
        : programa_(programa), emisor_(emisor) {}

    int ejecutar() {
        for (const Funcion& funcion : programa_.funciones) {
            funciones_.emplace(funcion.nombre, &funcion);
        }
        for (const sintactico::DefinicionTipo& definicion : programa_.tipos) {
            tipos_.emplace(definicion.nombre, &definicion);
        }
        for (const Funcion& funcion : programa_.funciones) {
            revisar_funcion(funcion);
        }
        return errores_;
    }

private:
    // --- Utilidades --------------------------------------------------------
    diagnostico::Diagnostico& error(std::string_view codigo, diagnostico::Span span) {
        ++errores_;
        return emisor_.error(codigo, span);
    }

    static bool compatible(Tipo esperado, Tipo real) {
        return esperado == real || esperado == Tipo::Desconocido || real == Tipo::Desconocido;
    }

    void exigir(Tipo esperado, Tipo real, diagnostico::Span span) {
        if (compatible(esperado, real)) {
            return;
        }
        error(diagnostico::codigos::TIPO_INCOMPATIBLE, span)
            .parametro("esperado", nombre(esperado))
            .parametro("encontrado", nombre(real));
    }

    void exigir_booleano(Tipo real, diagnostico::Span span) {
        if (compatible(Tipo::Booleano, real)) {
            return;
        }
        error(diagnostico::codigos::CONDICION_NO_BOOLEANA, span).parametro("tipo", nombre(real));
    }

    // --- Ámbitos -----------------------------------------------------------
    void abrir_ambito() { ambitos_.emplace_back(); }
    void cerrar_ambito() {
        if (!ambitos_.empty()) {
            ambitos_.pop_back();
        }
    }
    void declarar(std::string_view nombre_variable, Tipo tipo) {
        if (!ambitos_.empty()) {
            ambitos_.back()[std::string(nombre_variable)] = tipo;
        }
    }
    Tipo buscar(std::string_view nombre_variable, diagnostico::Span span) {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            const auto encontrada = it->find(std::string(nombre_variable));
            if (encontrada != it->end()) {
                return encontrada->second;
            }
        }
        error(diagnostico::codigos::NOMBRE_NO_DECLARADO, span)
            .parametro("nombre", std::string(nombre_variable))
            .ayuda("declare la variable antes de usarla");
        return Tipo::Desconocido;
    }

    // --- Funciones ---------------------------------------------------------
    void revisar_funcion(const Funcion& funcion) {
        abrir_ambito();
        for (const sintactico::Parametro& parametro : funcion.parametros) {
            const Tipo tipo = desde_nombre(parametro.tipo);
            if (tipo == Tipo::Desconocido) {
                error(diagnostico::codigos::TIPO_INCOMPATIBLE, parametro.span)
                    .parametro("esperado", "un tipo conocido")
                    .parametro("encontrado", std::string(parametro.tipo));
            }
            declarar(parametro.nombre, tipo);
        }

        for (const Sentencia& sentencia : funcion.cuerpo) {
            revisar_sentencia(sentencia, funcion);
        }
        cerrar_ambito();
    }

    void revisar_cuerpo(const std::vector<Sentencia>& cuerpo, const Funcion& funcion) {
        abrir_ambito();
        for (const Sentencia& sentencia : cuerpo) {
            revisar_sentencia(sentencia, funcion);
        }
        cerrar_ambito();
    }

    // --- Sentencias --------------------------------------------------------
    void revisar_sentencia(const Sentencia& sentencia, const Funcion& funcion) {
        switch (sentencia.tipo) {
            case TipoSentencia::Expresion:
                if (sentencia.expresion) {
                    tipo_de(*sentencia.expresion);
                }
                return;

            case TipoSentencia::Variable:
            case TipoSentencia::Constante: {
                if (!sentencia.expresion) {
                    return;
                }
                const Tipo real = tipo_de(*sentencia.expresion);
                if (sentencia.tipo_declarado.empty()) {
                    declarar(sentencia.nombre, real);
                    return;
                }
                const Tipo declarado = desde_nombre(sentencia.tipo_declarado);
                exigir(declarado, real, sentencia.expresion->span);
                declarar(sentencia.nombre, declarado);
                return;
            }

            case TipoSentencia::Si: {
                if (sentencia.expresion) {
                    exigir_booleano(tipo_de(*sentencia.expresion), sentencia.expresion->span);
                }
                revisar_cuerpo(sentencia.cuerpo, funcion);
                if (!sentencia.sino.empty()) {
                    revisar_cuerpo(sentencia.sino, funcion);
                }
                return;
            }

            case TipoSentencia::Mientras: {
                if (sentencia.expresion) {
                    exigir_booleano(tipo_de(*sentencia.expresion), sentencia.expresion->span);
                }
                revisar_cuerpo(sentencia.cuerpo, funcion);
                return;
            }

            case TipoSentencia::Para: {
                if (sentencia.expresion) {
                    tipo_de(*sentencia.expresion);
                }
                if (sentencia.paso) {
                    exigir(Tipo::Entero, tipo_de(*sentencia.paso), sentencia.paso->span);
                }
                abrir_ambito();
                declarar(sentencia.nombre, Tipo::Entero);
                for (const Sentencia& hija : sentencia.cuerpo) {
                    revisar_sentencia(hija, funcion);
                }
                cerrar_ambito();
                return;
            }

            case TipoSentencia::Hacer: {
                revisar_cuerpo(sentencia.cuerpo, funcion);
                if (sentencia.paso) {
                    exigir_booleano(tipo_de(*sentencia.paso), sentencia.paso->span);
                }
                return;
            }

            case TipoSentencia::Bucle:
                revisar_cuerpo(sentencia.cuerpo, funcion);
                return;

            case TipoSentencia::Romper:
            case TipoSentencia::Continuar:
                return;

            case TipoSentencia::Retornar: {
                revisar_retorno(sentencia, funcion);
                return;
            }

            case TipoSentencia::Bloque:
                revisar_cuerpo(sentencia.cuerpo, funcion);
                return;
        }
    }

    void revisar_retorno(const Sentencia& sentencia, const Funcion& funcion) {
        const Tipo declarado = desde_nombre(funcion.tipo_retorno);

        if (!sentencia.expresion) {
            if (declarado != Tipo::Vacio && declarado != Tipo::Desconocido) {
                error(diagnostico::codigos::RETORNO_INCOMPATIBLE, sentencia.span)
                    .parametro("funcion", std::string(funcion.nombre))
                    .parametro("esperado", nombre(declarado))
                    .parametro("encontrado", "nada");
            }
            return;
        }

        const Tipo real = tipo_de(*sentencia.expresion);
        if (declarado == Tipo::Vacio) {
            error(diagnostico::codigos::RETORNO_INCOMPATIBLE, sentencia.expresion->span)
                .parametro("funcion", std::string(funcion.nombre))
                .parametro("esperado", "nada")
                .parametro("encontrado", nombre(real));
            return;
        }
        if (!compatible(declarado, real)) {
            error(diagnostico::codigos::RETORNO_INCOMPATIBLE, sentencia.expresion->span)
                .parametro("funcion", std::string(funcion.nombre))
                .parametro("esperado", nombre(declarado))
                .parametro("encontrado", nombre(real));
        }
    }

    // --- Expresiones -------------------------------------------------------
    Tipo tipo_de(const Expresion& expresion) {
        switch (expresion.tipo) {
            case TipoExpresion::Texto:
                return Tipo::Texto;
            case TipoExpresion::Entero:
                return Tipo::Entero;
            case TipoExpresion::Booleano:
                return Tipo::Booleano;
            case TipoExpresion::Identificador:
                return buscar(expresion.lexema, expresion.span);
            case TipoExpresion::Unaria:
                return tipo_unaria(expresion);
            case TipoExpresion::Binaria:
                return tipo_binaria(expresion);
            case TipoExpresion::Asignacion:
                return tipo_asignacion(expresion);
            case TipoExpresion::Llamada:
                return tipo_llamada(expresion);
            case TipoExpresion::Construccion:
                return tipo_construccion(expresion);
            default:
                // Caracteres, flotantes, índices y conversiones: el generador los
                // rechaza con NOK6003 cuando llegue el momento.
                return Tipo::Desconocido;
        }
    }

    Tipo tipo_unaria(const Expresion& expresion) {
        const Tipo operando = expresion.izquierda ? tipo_de(*expresion.izquierda)
                                                  : Tipo::Desconocido;
        if (expresion.lexema == "!") {
            exigir(Tipo::Booleano, operando, expresion.span);
            return Tipo::Booleano;
        }
        if (expresion.lexema == "-") {
            exigir(Tipo::Entero, operando, expresion.span);
            return Tipo::Entero;
        }
        return Tipo::Desconocido;
    }

    Tipo tipo_binaria(const Expresion& expresion) {
        const Tipo izquierda =
            expresion.izquierda ? tipo_de(*expresion.izquierda) : Tipo::Desconocido;
        const Tipo derecha = expresion.derecha ? tipo_de(*expresion.derecha) : Tipo::Desconocido;
        const std::string_view operador = expresion.lexema;

        if (operador == "+" || operador == "-" || operador == "*" || operador == "/" ||
            operador == "%" || operador == "**") {
            exigir(Tipo::Entero, izquierda, expresion.span);
            exigir(Tipo::Entero, derecha, expresion.span);
            return Tipo::Entero;
        }
        if (operador == "==" || operador == "!=" || operador == "<" || operador == ">" ||
            operador == "<=" || operador == ">=") {
            // Solo entre enteros: comparar textos necesita el tipo texto de verdad.
            exigir(Tipo::Entero, izquierda, expresion.span);
            exigir(Tipo::Entero, derecha, expresion.span);
            return Tipo::Booleano;
        }
        if (operador == "&&" || operador == "||") {
            exigir(Tipo::Booleano, izquierda, expresion.span);
            exigir(Tipo::Booleano, derecha, expresion.span);
            return Tipo::Booleano;
        }
        if (operador == ".." || operador == "..=") {
            exigir(Tipo::Entero, izquierda, expresion.span);
            exigir(Tipo::Entero, derecha, expresion.span);
            return Tipo::Desconocido;
        }
        return Tipo::Desconocido;
    }

    Tipo tipo_asignacion(const Expresion& expresion) {
        const Tipo destino =
            expresion.izquierda ? tipo_de(*expresion.izquierda) : Tipo::Desconocido;
        const Tipo valor = expresion.derecha ? tipo_de(*expresion.derecha) : Tipo::Desconocido;
        exigir(destino, valor, expresion.span);
        return destino;
    }

    Tipo tipo_llamada(const Expresion& expresion) {
        if (!expresion.izquierda ||
            expresion.izquierda->tipo != TipoExpresion::Identificador) {
            return Tipo::Desconocido;
        }

        const std::string_view nombre_llamado = expresion.izquierda->lexema;

        if (nombre_llamado == "imprimir") {
            if (expresion.lista.size() != 1) {
                error(diagnostico::codigos::ARIDAD_INCORRECTA, expresion.span)
                    .parametro("funcion", "imprimir")
                    .parametro("esperados", "1")
                    .parametro("dados", std::to_string(expresion.lista.size()));
                return Tipo::Vacio;
            }
            const Tipo argumento =
                expresion.lista.front() ? tipo_de(*expresion.lista.front()) : Tipo::Desconocido;
            if (argumento != Tipo::Entero && argumento != Tipo::Texto &&
                argumento != Tipo::Desconocido) {
                error(diagnostico::codigos::TIPO_INCOMPATIBLE, expresion.lista.front()->span)
                    .parametro("esperado", "entero o texto")
                    .parametro("encontrado", nombre(argumento));
            }
            return Tipo::Vacio;
        }

        const auto encontrada = funciones_.find(nombre_llamado);
        if (encontrada == funciones_.end()) {
            error(diagnostico::codigos::FUNCION_NO_DECLARADA, expresion.span)
                .parametro("nombre", std::string(nombre_llamado))
                .ayuda("compruebe el nombre o declare la función");
            return Tipo::Desconocido;
        }

        const Funcion& funcion = *encontrada->second;

        if (expresion.lista.size() != funcion.parametros.size()) {
            error(diagnostico::codigos::ARIDAD_INCORRECTA, expresion.span)
                .parametro("funcion", std::string(funcion.nombre))
                .parametro("esperados", std::to_string(funcion.parametros.size()))
                .parametro("dados", std::to_string(expresion.lista.size()));
            return desde_nombre(funcion.tipo_retorno);
        }

        for (std::size_t i = 0; i < expresion.lista.size(); ++i) {
            if (!expresion.lista[i]) {
                continue;
            }
            const Tipo real = tipo_de(*expresion.lista[i]);
            const Tipo esperado = desde_nombre(funcion.parametros[i].tipo);
            if (compatible(esperado, real)) {
                continue;
            }
            error(diagnostico::codigos::ARGUMENTO_INCOMPATIBLE, expresion.lista[i]->span)
                .parametro("parametro", std::string(funcion.parametros[i].nombre))
                .parametro("funcion", std::string(funcion.nombre))
                .parametro("esperado", nombre(esperado))
                .parametro("encontrado", nombre(real));
        }

        return desde_nombre(funcion.tipo_retorno);
    }

    Tipo tipo_construccion(const Expresion& expresion) {
        const auto encontrado = tipos_.find(expresion.anotacion);
        if (encontrado == tipos_.end()) {
            error(diagnostico::codigos::TIPO_DESCONOCIDO, expresion.span)
                .parametro("nombre", expresion.anotacion);
            return Tipo::Desconocido;
        }

        const sintactico::DefinicionTipo& definicion = *encontrado->second;

        for (std::size_t i = 0; i < expresion.lista.size(); ++i) {
            const std::string_view nombre_campo =
                i < expresion.nombres_campo.size() ? expresion.nombres_campo[i] : "";
            const sintactico::Campo* campo = nullptr;
            for (const sintactico::Campo& declarado : definicion.campos) {
                if (declarado.nombre == nombre_campo) {
                    campo = &declarado;
                    break;
                }
            }
            if (campo == nullptr) {
                error(diagnostico::codigos::CAMPO_DESCONOCIDO, expresion.span)
                    .parametro("tipo", std::string(definicion.nombre))
                    .parametro("campo", std::string(nombre_campo));
                continue;
            }
            if (expresion.lista[i]) {
                exigir(desde_nombre(campo->tipo), tipo_de(*expresion.lista[i]),
                       expresion.lista[i]->span);
            }
        }

        for (const sintactico::Campo& campo : definicion.campos) {
            bool presente = false;
            for (const std::string_view nombre_campo : expresion.nombres_campo) {
                if (nombre_campo == campo.nombre) {
                    presente = true;
                    break;
                }
            }
            if (!presente) {
                error(diagnostico::codigos::CAMPO_FALTANTE, expresion.span)
                    .parametro("tipo", std::string(definicion.nombre))
                    .parametro("campo", std::string(campo.nombre));
            }
        }

        // La v0.1 no distingue tipos de estructura en el verificador: el generador
        // los conoce por la construcción. Los campos se comprueban igualmente.
        return Tipo::Desconocido;
    }

    const sintactico::Programa& programa_;
    diagnostico::Emisor& emisor_;
    std::map<std::string_view, const Funcion*> funciones_;
    std::map<std::string_view, const sintactico::DefinicionTipo*> tipos_;
    std::vector<std::map<std::string, Tipo>> ambitos_;
    int errores_ = 0;
};

}  // namespace

int verificar(const sintactico::Programa& programa, diagnostico::Emisor& emisor) {
    Verificador verificador(programa, emisor);
    return verificador.ejecutar();
}

}  // namespace nooki::semantico
