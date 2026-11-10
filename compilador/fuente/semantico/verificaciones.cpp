#include "nooki/semantico/verificaciones.hpp"

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/codigos.hpp"
#include "nooki/semantico/tipos.hpp"

namespace nooki::semantico {
namespace {

using sintactico::Expresion;
using sintactico::Funcion;
using sintactico::Patron;
using sintactico::Rama;
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
        for (const sintactico::Enumeracion& enumeracion : programa_.enumeraciones) {
            enumeraciones_.emplace(enumeracion.nombre, &enumeracion);
        }
        for (const sintactico::Funcion& funcion : programa_.funciones) {
            if (!funcion.modulo.empty()) {
                modulos_[funcion.modulo].emplace(funcion.nombre, &funcion);
            }
        }
        for (const Funcion& funcion : programa_.funciones) {
            revisar_funcion(funcion);
        }
        for (const sintactico::DefinicionTipo& definicion : programa_.tipos) {
            const std::string_view anterior = estructura_actual_;
            estructura_actual_ = definicion.nombre;
            for (const Funcion& metodo : definicion.metodos) {
                revisar_funcion(metodo);
            }
            estructura_actual_ = anterior;
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
    struct Ambito {
        std::map<std::string, Tipo> tipos;
        std::map<std::string, std::string_view> estructuras;
        std::set<std::string> cajas;
        std::set<std::string> textos;
        std::set<std::string> prestables;
        std::set<std::string> vectores_texto;
        std::set<std::string> movidas;
        std::map<std::string, bool> prestamos;  // nombre -> ¿mutable?
    };

    void registrar_prestamo(std::string_view nombre, bool mutable_) {
        if (!ambitos_.empty()) {
            ambitos_.back().prestamos[std::string(nombre)] = mutable_;
        }
    }
    bool declarada(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->tipos.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }
    bool es_prestamo_declarado(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->prestamos.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }
    bool es_prestamo_mutable(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            const auto encontrado = it->prestamos.find(std::string(nombre));
            if (encontrado != it->prestamos.end()) {
                return encontrado->second;
            }
        }
        return false;
    }

    static bool es_prestamo(std::string_view tipo) {
        return !tipo.empty() && (tipo.front() == '&' || tipo.front() == '*');
    }

    /// ¿Es un tipo que el compilador entiende en esta etapa?
    bool es_tipo_valido(std::string_view tipo) const {
        if (tipo.empty()) {
            return false;
        }
        if (desde_nombre(tipo) != Tipo::Desconocido) {
            return true;
        }
        if (tipo_por_nombre(tipo) != nullptr || enumeraciones_.count(tipo) != 0) {
            return true;
        }
        if (tipo.rfind("Caja<", 0) == 0) {
            return true;
        }
        if (tipo.rfind("Vector<", 0) == 0) {
            return true;
        }
        if (tipo.rfind("Mapa<", 0) == 0) {
            return true;
        }
        if (parametros_tipo_.count(std::string(tipo)) != 0) {
            return true;
        }
        if (es_prestamo(tipo)) {
            std::string_view resto = tipo.substr(1);
            if (resto.rfind("mutable ", 0) == 0) {
                resto = resto.substr(8);
            }
            return es_tipo_valido(resto);
        }
        return false;
    }

    void declarar_caja(std::string_view nombre) {
        if (!ambitos_.empty()) {
            ambitos_.back().cajas.insert(std::string(nombre));
        }
    }
    bool es_caja(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->cajas.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }
    void declarar_vector_texto(std::string_view nombre) {
        if (!ambitos_.empty()) {
            ambitos_.back().vectores_texto.insert(std::string(nombre));
        }
    }
    bool es_vector_texto(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->vectores_texto.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }

    void declarar_prestable(std::string_view nombre) {
        if (!ambitos_.empty()) {
            ambitos_.back().prestables.insert(std::string(nombre));
        }
    }
    bool es_prestable(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->prestables.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }

    void declarar_texto(std::string_view nombre) {
        if (!ambitos_.empty()) {
            ambitos_.back().textos.insert(std::string(nombre));
        }
    }
    bool es_texto(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->textos.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }

    void marcar_movida(std::string_view nombre) {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            const std::string clave(nombre);
            if (it->cajas.count(clave) != 0 || it->textos.count(clave) != 0) {
                it->movidas.insert(clave);
                return;
            }
        }
    }
    bool esta_movida(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->movidas.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }

    static bool es_llamada_mapa(const Expresion& expresion) {
        if (expresion.tipo != TipoExpresion::Llamada || !expresion.izquierda) {
            return false;
        }
        const Expresion& destino = *expresion.izquierda;
        return destino.tipo == TipoExpresion::Miembro && destino.izquierda &&
               destino.izquierda->tipo == TipoExpresion::Identificador &&
               destino.izquierda->lexema == "Mapa";
    }

    static bool es_llamada_vector(const Expresion& expresion) {
        if (expresion.tipo != TipoExpresion::Llamada || !expresion.izquierda) {
            return false;
        }
        const Expresion& destino = *expresion.izquierda;
        return destino.tipo == TipoExpresion::Miembro && destino.izquierda &&
               destino.izquierda->tipo == TipoExpresion::Identificador &&
               destino.izquierda->lexema == "Vector";
    }

    static bool es_llamada_caja(const Expresion& expresion) {
        if (expresion.tipo != TipoExpresion::Llamada || !expresion.izquierda) {
            return false;
        }
        const Expresion& destino = *expresion.izquierda;
        return destino.tipo == TipoExpresion::Miembro && destino.izquierda &&
               destino.izquierda->tipo == TipoExpresion::Identificador &&
               destino.izquierda->lexema == "Caja" && destino.anotacion == "nuevo";
    }

    void abrir_ambito() { ambitos_.emplace_back(); }
    void cerrar_ambito() {
        if (!ambitos_.empty()) {
            ambitos_.pop_back();
        }
    }
    void declarar(std::string_view nombre_variable, Tipo tipo,
                  std::string_view estructura = {}) {
        if (ambitos_.empty()) {
            return;
        }
        Ambito& ambito = ambitos_.back();
        ambito.tipos[std::string(nombre_variable)] = tipo;
        if (!estructura.empty()) {
            ambito.estructuras[std::string(nombre_variable)] = estructura;
        }
    }
    Tipo buscar(std::string_view nombre_variable, diagnostico::Span span) {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            const auto encontrada = it->tipos.find(std::string(nombre_variable));
            if (encontrada != it->tipos.end()) {
                return encontrada->second;
            }
        }
        error(diagnostico::codigos::NOMBRE_NO_DECLARADO, span)
            .parametro("nombre", std::string(nombre_variable))
            .ayuda("declare la variable antes de usarla");
        return Tipo::Desconocido;
    }
    std::string_view buscar_estructura(std::string_view nombre_variable) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            const auto encontrada = it->estructuras.find(std::string(nombre_variable));
            if (encontrada != it->estructuras.end()) {
                return encontrada->second;
            }
        }
        return {};
    }

    const sintactico::DefinicionTipo* tipo_por_nombre(std::string_view nombre) const {
        const auto encontrada = tipos_.find(nombre);
        return encontrada == tipos_.end() ? nullptr : encontrada->second;
    }

    /// Nombre del tipo de estructura de una expresión, si lo tiene.
    std::string_view estructura_de(const Expresion& expresion) const {
        switch (expresion.tipo) {
            case TipoExpresion::Identificador:
                return buscar_estructura(expresion.lexema);
            case TipoExpresion::Este:
                return estructura_actual_;
            case TipoExpresion::Miembro: {
                if (!expresion.izquierda) {
                    return {};
                }
                const sintactico::DefinicionTipo* definicion =
                    tipo_por_nombre(estructura_de(*expresion.izquierda));
                if (definicion == nullptr) {
                    return {};
                }
                for (const sintactico::Campo& campo : definicion->campos) {
                    if (campo.nombre == expresion.anotacion) {
                        return tipo_por_nombre(campo.tipo) != nullptr ? campo.tipo
                                                                     : std::string_view{};
                    }
                }
                return {};
            }
            default:
                return {};
        }
    }

    const Funcion* metodo_de(const sintactico::DefinicionTipo& definicion,
                             std::string_view nombre_metodo) const {
        for (const Funcion& metodo : definicion.metodos) {
            if (metodo.nombre == nombre_metodo) {
                return &metodo;
            }
        }
        return nullptr;
    }

    // --- Funciones ---------------------------------------------------------
    void revisar_funcion(const Funcion& funcion) {
        const std::size_t previos = parametros_tipo_.size();
        for (const std::string_view parametro : funcion.tipo_parametros) {
            parametros_tipo_.insert(std::string(parametro));
        }

        abrir_ambito();
        for (const sintactico::Parametro& parametro : funcion.parametros) {
            const Tipo tipo = desde_nombre(parametro.tipo);
            const sintactico::DefinicionTipo* estructura = tipo_por_nombre(parametro.tipo);
            if (!es_tipo_valido(parametro.tipo)) {
                error(diagnostico::codigos::TIPO_INCOMPATIBLE, parametro.span)
                    .parametro("esperado", "un tipo conocido")
                    .parametro("encontrado", std::string(parametro.tipo));
            }
            declarar(parametro.nombre, tipo,
                     estructura != nullptr ? parametro.tipo : std::string_view{});
            if (es_prestamo(parametro.tipo)) {
                registrar_prestamo(parametro.nombre, parametro.tipo.rfind("&mutable", 0) == 0);
            }
        }

        for (const Sentencia& sentencia : funcion.cuerpo) {
            revisar_sentencia(sentencia, funcion);
        }
        cerrar_ambito();

        while (parametros_tipo_.size() > previos) {
            parametros_tipo_.erase(*parametros_tipo_.begin());
        }
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

                // Un préstamo no se puede guardar: solo vive durante la llamada.
                if (sentencia.expresion->tipo == TipoExpresion::Unaria &&
                    (sentencia.expresion->lexema == "&" ||
                     sentencia.expresion->lexema == "&mutable")) {
                    error(diagnostico::codigos::PRESTAMO_GUARDADO, sentencia.expresion->span)
                        .ayuda("pase el préstamo directamente como argumento");
                    return;
                }

                if (sentencia.tipo_declarado.rfind("Vector<", 0) == 0 ||
                    sentencia.tipo_declarado.rfind("Mapa<", 0) == 0 ||
                    es_llamada_vector(*sentencia.expresion) ||
                    es_llamada_mapa(*sentencia.expresion)) {
                    declarar_prestable(sentencia.nombre);
                }
                // Una llamada a un módulo que devuelve una colección también se presta.
                if (sentencia.expresion->tipo == TipoExpresion::Llamada &&
                    sentencia.expresion->izquierda &&
                    sentencia.expresion->izquierda->tipo == TipoExpresion::Miembro) {
                    const Expresion& acceso = *sentencia.expresion->izquierda;
                    if (acceso.izquierda &&
                        acceso.izquierda->tipo == TipoExpresion::Identificador) {
                        const auto modulo = modulos_.find(acceso.izquierda->lexema);
                        if (modulo != modulos_.end()) {
                            const auto encontrada = modulo->second.find(acceso.anotacion);
                            if (encontrada != modulo->second.end()) {
                                const std::string_view devuelto =
                                    encontrada->second->tipo_retorno;
                                if (devuelto.rfind("Mapa<", 0) == 0 ||
                                    devuelto.rfind("Vector<", 0) == 0 ||
                                    devuelto.rfind("Caja<", 0) == 0) {
                                    declarar_prestable(sentencia.nombre);
                                }
                            }
                        }
                    }
                }
                if (es_llamada_vector(*sentencia.expresion)) {
                    const Expresion& destino = *sentencia.expresion->izquierda;
                    if (destino.izquierda &&
                        !destino.izquierda->tipos_aplicados.empty() &&
                        destino.izquierda->tipos_aplicados.front() == "texto") {
                        declarar_vector_texto(sentencia.nombre);
                    }
                }
                if (real == Tipo::Texto && sentencia.tipo_declarado.empty()) {
                    if (sentencia.expresion->tipo == TipoExpresion::Identificador &&
                        es_texto(sentencia.expresion->lexema)) {
                        marcar_movida(sentencia.expresion->lexema);
                    }
                    declarar_texto(sentencia.nombre);
                    declarar_prestable(sentencia.nombre);
                    declarar(sentencia.nombre, real);
                    return;
                }

                // ¿Declara una caja nueva, o mueve una que ya existía?
                if (es_llamada_caja(*sentencia.expresion) && sentencia.tipo_declarado.empty()) {
                    declarar_caja(sentencia.nombre);
                    declarar(sentencia.nombre, real);
                    return;
                }
                if (sentencia.expresion->tipo == TipoExpresion::Identificador &&
                    es_caja(sentencia.expresion->lexema) && sentencia.tipo_declarado.empty()) {
                    marcar_movida(sentencia.expresion->lexema);
                    declarar_caja(sentencia.nombre);
                    declarar(sentencia.nombre, real);
                    return;
                }

                const bool es_construccion =
                    sentencia.expresion->tipo == TipoExpresion::Construccion;
                if (sentencia.tipo_declarado.empty()) {
                    declarar(sentencia.nombre, real,
                             es_construccion ? sentencia.expresion->anotacion : std::string_view{});
                    return;
                }
                const Tipo declarado = desde_nombre(sentencia.tipo_declarado);
                exigir(declarado, real, sentencia.expresion->span);
                declarar(sentencia.nombre, declarado,
                         tipo_por_nombre(sentencia.tipo_declarado) != nullptr
                             ? sentencia.tipo_declarado
                             : std::string_view{});
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

            case TipoSentencia::Coincide:
                revisar_coincide(sentencia, funcion);
                return;
        }
    }

    void revisar_coincide(const Sentencia& sentencia, const Funcion& funcion) {
        if (sentencia.expresion) {
            tipo_de(*sentencia.expresion);
        }

        bool hay_comodin = false;
        for (const Rama& rama : sentencia.ramas) {
            if (rama.patron.clase == Patron::Clase::Comodin) {
                hay_comodin = true;
            } else if (rama.patron.clase == Patron::Clase::Variante) {
                const auto enumeracion = enumeraciones_.find(rama.patron.enumeracion);
                if (enumeracion == enumeraciones_.end()) {
                    error(diagnostico::codigos::TIPO_DESCONOCIDO, rama.patron.span)
                        .parametro("nombre", std::string(rama.patron.enumeracion));
                } else {
                    bool existe = false;
                    for (const std::string_view variante : enumeracion->second->variantes) {
                        if (variante == rama.patron.valor) {
                            existe = true;
                            break;
                        }
                    }
                    if (!existe) {
                        error(diagnostico::codigos::VARIANTE_DESCONOCIDA, rama.patron.span)
                            .parametro("enumeracion",
                                       std::string(enumeracion->second->nombre))
                            .parametro("variante", std::string(rama.patron.valor));
                    }
                }
            }

            revisar_cuerpo(rama.cuerpo, funcion);
        }

        if (!hay_comodin) {
            error(diagnostico::codigos::COINCIDE_NO_EXHAUSTIVO, sentencia.span)
                .ayuda("añada «caso _ => ...» para los casos que falten");
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
            case TipoExpresion::Identificador: {
                if (expresion.lexema == "Caja" || expresion.lexema == "Vector" ||
                    expresion.lexema == "Mapa") {
                    return Tipo::Desconocido;
                }
                if (esta_movida(expresion.lexema)) {
                    error(diagnostico::codigos::USO_TRAS_MOVER, expresion.span)
                        .parametro("nombre", std::string(expresion.lexema))
                        .ayuda("se movió a otro sitio: use «&» para prestarlo sin transferirlo");
                    return Tipo::Desconocido;
                }
                return buscar(expresion.lexema, expresion.span);
            }
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
            case TipoExpresion::Miembro:
                return tipo_miembro(expresion);
            case TipoExpresion::Este:
                return Tipo::Desconocido;
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
        if (expresion.lexema == "*") {
            const bool caja = expresion.izquierda &&
                              expresion.izquierda->tipo == TipoExpresion::Identificador &&
                              es_caja(expresion.izquierda->lexema);
            const bool prestamo_de_caja =
                expresion.izquierda &&
                expresion.izquierda->tipo == TipoExpresion::Identificador &&
                es_prestamo_declarado(expresion.izquierda->lexema);
            if (!caja && !prestamo_de_caja) {
                error(diagnostico::codigos::DEREFERENCIA_INVALIDA, expresion.span);
            }
            return Tipo::Entero;
        }
        if (expresion.lexema == "&" || expresion.lexema == "&mutable") {
            const bool caja = expresion.izquierda &&
                              expresion.izquierda->tipo == TipoExpresion::Identificador &&
                              (es_caja(expresion.izquierda->lexema) ||
                               es_prestable(expresion.izquierda->lexema));
            if (!caja) {
                error(diagnostico::codigos::PRESTAMO_INVALIDO, expresion.span);
            }
            return Tipo::Desconocido;
        }
        return Tipo::Desconocido;
    }

    Tipo tipo_binaria(const Expresion& expresion) {
        const Tipo izquierda =
            expresion.izquierda ? tipo_de(*expresion.izquierda) : Tipo::Desconocido;
        const Tipo derecha = expresion.derecha ? tipo_de(*expresion.derecha) : Tipo::Desconocido;
        const std::string_view operador = expresion.lexema;

        // Textos: concatenación con «+» y comparación por contenido.
        if (izquierda == Tipo::Texto && derecha == Tipo::Texto) {
            if (operador == "+") {
                return Tipo::Texto;
            }
            if (operador == "==" || operador == "!=" || operador == "<" || operador == ">" ||
                operador == "<=" || operador == ">=") {
                return Tipo::Booleano;
            }
        }

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
        if (expresion.izquierda && expresion.izquierda->tipo == TipoExpresion::Unaria &&
            expresion.izquierda->lexema == "*" && expresion.izquierda->izquierda &&
            expresion.izquierda->izquierda->tipo == TipoExpresion::Identificador) {
            const std::string_view nombre = expresion.izquierda->izquierda->lexema;
            if (es_prestamo_declarado(nombre) && !es_prestamo_mutable(nombre)) {
                error(diagnostico::codigos::ESCRITURA_EN_PRESTAMO_INMUTABLE, expresion.span);
            }
        }

        const Tipo destino =
            expresion.izquierda ? tipo_de(*expresion.izquierda) : Tipo::Desconocido;
        const Tipo valor = expresion.derecha ? tipo_de(*expresion.derecha) : Tipo::Desconocido;
        if (expresion.derecha && expresion.derecha->tipo == TipoExpresion::Identificador &&
            es_caja(expresion.derecha->lexema)) {
            marcar_movida(expresion.derecha->lexema);
        }
        exigir(destino, valor, expresion.span);
        return destino;
    }

    /// Dentro de una misma llamada no puede haber un préstamo mutable y otro cualquiera
    /// de la misma variable.
    void comprobar_exclusividad(const Expresion& llamada) {
        std::map<std::string_view, int> mutables;
        std::map<std::string_view, int> compartidos;

        for (const sintactico::ExpresionPtr& argumento : llamada.lista) {
            if (!argumento || argumento->tipo != TipoExpresion::Unaria) {
                continue;
            }
            const bool es_mutable = argumento->lexema == "&mutable";
            if (!es_mutable && argumento->lexema != "&") {
                continue;
            }
            if (!argumento->izquierda ||
                argumento->izquierda->tipo != TipoExpresion::Identificador) {
                continue;
            }
            const std::string_view nombre = argumento->izquierda->lexema;
            if (es_mutable) {
                ++mutables[nombre];
            } else {
                ++compartidos[nombre];
            }
        }

        for (const auto& [nombre, cuenta] : mutables) {
            if (cuenta > 1 || compartidos[nombre] > 0) {
                error(diagnostico::codigos::PRESTAMO_EXCLUSIVO, llamada.span)
                    .parametro("nombre", std::string(nombre));
            }
        }
    }

    Tipo tipo_llamada(const Expresion& expresion) {
        if (!expresion.izquierda) {
            return Tipo::Desconocido;
        }
        comprobar_exclusividad(expresion);

        if (expresion.izquierda->tipo == TipoExpresion::Miembro) {
            const Expresion& acceso = *expresion.izquierda;
            if (acceso.izquierda &&
                acceso.izquierda->tipo == TipoExpresion::Identificador) {
                // Una variable local con el nombre del módulo tiene prioridad.
                const auto modulo = declarada(acceso.izquierda->lexema)
                                        ? modulos_.end()
                                        : modulos_.find(acceso.izquierda->lexema);
                if (modulo != modulos_.end()) {
                    const auto funcion = modulo->second.find(acceso.anotacion);
                    if (funcion == modulo->second.end()) {
                        error(diagnostico::codigos::FUNCION_NO_DECLARADA, expresion.span)
                            .parametro("nombre", std::string(acceso.izquierda->lexema) + "." +
                                                   std::string(acceso.anotacion));
                        return Tipo::Desconocido;
                    }
                    if (!funcion->second->publico) {
                        error(diagnostico::codigos::SIMBOLO_PRIVADO, expresion.span)
                            .parametro("simbolo", std::string(acceso.anotacion))
                            .parametro("modulo", std::string(acceso.izquierda->lexema));
                        return Tipo::Desconocido;
                    }
                    return comprobar_argumentos(
                        *funcion->second, expresion,
                        std::string(acceso.izquierda->lexema) + "." +
                            std::string(acceso.anotacion));
                }
            }
            return tipo_llamada_metodo(expresion);
        }

        if (expresion.izquierda->tipo != TipoExpresion::Identificador) {
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

        return comprobar_argumentos(*encontrada->second, expresion,
                                    std::string(encontrada->second->nombre));
    }

    Tipo tipo_llamada_metodo(const Expresion& expresion) {
        const Expresion& acceso = *expresion.izquierda;

        if (acceso.izquierda) {
            const Tipo objeto = tipo_de(*acceso.izquierda);
            if (objeto == Tipo::Texto && acceso.anotacion == "largo") {
                return Tipo::Entero;
            }
        }

        // `vector.agregar(texto)` toma el dueño del elemento.
        if (acceso.anotacion == "agregar" && acceso.izquierda &&
            acceso.izquierda->tipo == TipoExpresion::Identificador &&
            es_vector_texto(acceso.izquierda->lexema) && expresion.lista.size() == 1 &&
            expresion.lista.front() && expresion.lista.front()->tipo == TipoExpresion::Identificador) {
            marcar_movida(expresion.lista.front()->lexema);
        }

        const sintactico::DefinicionTipo* definicion = tipo_por_nombre(
            acceso.izquierda ? estructura_de(*acceso.izquierda) : std::string_view{});
        if (definicion == nullptr) {
            return Tipo::Desconocido;
        }

        const Funcion* metodo = metodo_de(*definicion, acceso.anotacion);
        if (metodo == nullptr) {
            error(diagnostico::codigos::MIEMBRO_DESCONOCIDO, acceso.span)
                .parametro("tipo", std::string(definicion->nombre))
                .parametro("nombre", acceso.anotacion);
            return Tipo::Desconocido;
        }

        return comprobar_argumentos(
            *metodo, expresion,
            std::string(definicion->nombre) + "." + std::string(metodo->nombre));
    }

    Tipo comprobar_argumentos(const Funcion& funcion, const Expresion& llamada,
                              const std::string& nombre_visible) {
        std::vector<std::string_view> a_mover;

        if (llamada.lista.size() != funcion.parametros.size()) {
            error(diagnostico::codigos::ARIDAD_INCORRECTA, llamada.span)
                .parametro("funcion", nombre_visible)
                .parametro("esperados", std::to_string(funcion.parametros.size()))
                .parametro("dados", std::to_string(llamada.lista.size()));
            return desde_nombre(funcion.tipo_retorno);
        }

        for (std::size_t i = 0; i < llamada.lista.size(); ++i) {
            if (!llamada.lista[i]) {
                continue;
            }

            // Pasar un valor con dueño por valor lo mueve: se marca al final, cuando ya
            // se comprobó el tipo de este argumento.
            if (!es_prestamo(funcion.parametros[i].tipo) &&
                llamada.lista[i]->tipo == TipoExpresion::Identificador) {
                const std::string_view origen = llamada.lista[i]->lexema;
                if (es_caja(origen) || es_texto(origen) || es_prestable(origen)) {
                    a_mover.push_back(origen);
                }
            }

            // Un parámetro cuyo tipo es una estructura se compara por nombre de tipo.
            if (tipo_por_nombre(funcion.parametros[i].tipo) != nullptr) {
                const std::string_view real = estructura_de(*llamada.lista[i]);
                if (real != funcion.parametros[i].tipo) {
                    error(diagnostico::codigos::ARGUMENTO_INCOMPATIBLE, llamada.lista[i]->span)
                        .parametro("parametro", std::string(funcion.parametros[i].nombre))
                        .parametro("funcion", nombre_visible)
                        .parametro("esperado", std::string(funcion.parametros[i].tipo))
                        .parametro("encontrado", real.empty() ? std::string("otro tipo")
                                                              : std::string(real));
                }
                continue;
            }

            const Tipo real = tipo_de(*llamada.lista[i]);
            const Tipo esperado = desde_nombre(funcion.parametros[i].tipo);
            if (compatible(esperado, real)) {
                continue;
            }
            error(diagnostico::codigos::ARGUMENTO_INCOMPATIBLE, llamada.lista[i]->span)
                .parametro("parametro", std::string(funcion.parametros[i].nombre))
                .parametro("funcion", nombre_visible)
                .parametro("esperado", nombre(esperado))
                .parametro("encontrado", nombre(real));
        }

        for (const std::string_view origen : a_mover) {
            marcar_movida(origen);
        }

        return desde_nombre(funcion.tipo_retorno);
    }

    Tipo tipo_miembro(const Expresion& expresion) {
        if (!expresion.izquierda) {
            return Tipo::Desconocido;
        }

        // ¿Es una variante de enumeración? `color.rojo`.
        if (expresion.izquierda->tipo == TipoExpresion::Identificador) {
            const auto enumeracion = enumeraciones_.find(expresion.izquierda->lexema);
            if (enumeracion != enumeraciones_.end()) {
                for (const std::string_view variante : enumeracion->second->variantes) {
                    if (variante == expresion.anotacion) {
                        return Tipo::Desconocido;
                    }
                }
                error(diagnostico::codigos::VARIANTE_DESCONOCIDA, expresion.span)
                    .parametro("enumeracion", std::string(enumeracion->second->nombre))
                    .parametro("variante", expresion.anotacion);
                return Tipo::Desconocido;
            }
        }

        const sintactico::DefinicionTipo* definicion =
            tipo_por_nombre(estructura_de(*expresion.izquierda));
        if (definicion == nullptr) {
            tipo_de(*expresion.izquierda);
            return Tipo::Desconocido;
        }

        for (const sintactico::Campo& campo : definicion->campos) {
            if (campo.nombre == expresion.anotacion) {
                return desde_nombre(campo.tipo);
            }
        }
        if (const Funcion* metodo = metodo_de(*definicion, expresion.anotacion);
            metodo != nullptr) {
            return desde_nombre(metodo->tipo_retorno);
        }

        error(diagnostico::codigos::MIEMBRO_DESCONOCIDO, expresion.span)
            .parametro("tipo", std::string(definicion->nombre))
            .parametro("nombre", expresion.anotacion);
        return Tipo::Desconocido;
    }

    Tipo tipo_construccion(const Expresion& expresion) {
        const auto encontrado = tipos_.find(expresion.anotacion);
        if (encontrado == tipos_.end()) {
            error(diagnostico::codigos::TIPO_DESCONOCIDO, expresion.span)
                .parametro("nombre", expresion.anotacion);
            return Tipo::Desconocido;
        }

        const sintactico::DefinicionTipo& definicion = *encontrado->second;

        for (std::size_t i = 0; i < expresion.lista.size(); ++i) {            const std::string_view nombre_campo =
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
    std::map<std::string_view, const sintactico::Enumeracion*> enumeraciones_;
    std::map<std::string_view, std::map<std::string_view, const sintactico::Funcion*>> modulos_;
    std::set<std::string> parametros_tipo_;
    std::string_view estructura_actual_;
    std::vector<Ambito> ambitos_;
    int errores_ = 0;
};

}  // namespace

int verificar(const sintactico::Programa& programa, diagnostico::Emisor& emisor) {
    Verificador verificador(programa, emisor);
    return verificador.ejecutar();
}

}  // namespace nooki::semantico
