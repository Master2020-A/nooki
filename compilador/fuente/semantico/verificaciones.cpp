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
            tipos_.emplace(std::string(definicion.nombre), &definicion);
            if (!definicion.modulo.empty()) {
                tipos_.emplace(std::string(definicion.modulo) + "." +
                                   std::string(definicion.nombre),
                               &definicion);
            }
        }
        for (const sintactico::Enumeracion& enumeracion : programa_.enumeraciones) {
            enumeraciones_.emplace(std::string(enumeracion.nombre), &enumeracion);
            if (!enumeracion.modulo.empty()) {
                enumeraciones_.emplace(std::string(enumeracion.modulo) + "." +
                                           std::string(enumeracion.nombre),
                                       &enumeracion);
            }
        }
        for (const sintactico::Rasgo& rasgo : programa_.rasgos) {
            rasgos_.emplace(std::string(rasgo.nombre), &rasgo);
        }
        for (const sintactico::Funcion& funcion : programa_.funciones) {
            if (!funcion.modulo.empty()) {
                modulos_[funcion.modulo].emplace(funcion.nombre, &funcion);
            }
        }
        for (const sintactico::Enumeracion& enumeracion : programa_.enumeraciones) {
            for (std::size_t i = 0; i < enumeracion.variantes.size(); ++i) {
                variantes_.emplace(enumeracion.variantes[i].nombre,
                                   std::make_pair(&enumeracion, i));
            }
        }
        for (const Funcion& funcion : programa_.funciones) {
            revisar_funcion(funcion);
        }
        revisar_sobrescrituras();
        revisar_rasgos();
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
        // El ámbito es dueño del nombre: así la vista que se devuelve no puede
        // quedar apuntando a una cadena local que ya murió.
        std::map<std::string, std::string> estructuras;
        std::set<std::string> cajas;
        std::set<std::string> textos;
        std::set<std::string> prestables;
        std::set<std::string> referencias;
        std::set<std::string> compuestos;
        std::set<std::string> vectores_texto;
        std::set<std::string> vectores_flotante;
        std::set<std::string> mapas_flotante;
        /// Variables de tipo función del ámbito: nombre → tipo.
        std::map<std::string, std::string> funciones;
        /// Variables que guardan un cierre, que se mueven al asignarlas.
        std::set<std::string> cierres;
        /// Celdas compartidas que guardan un texto.
        std::set<std::string> celdas_texto;
        /// Variables que son `Vector<MiEstructura>`: nombre → estructura.
        std::map<std::string, std::string> vector_de_estructura;
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
        if (tipo_por_nombre(tipo) != nullptr || enumeraciones_.count(std::string(tipo)) != 0) {
            return true;
        }
        if (tipo.rfind("Caja<", 0) == 0 || tipo.rfind("Compartido<", 0) == 0) {
            return true;
        }
        if (!tipo.empty() && tipo.front() == '&' &&
            rasgos_.count(std::string(tipo.substr(1))) != 0) {
            return true;
        }
        if (tipo.rfind("Vector<", 0) == 0) {
            return true;
        }
        // Un tipo función o un cierre.
        if (tipo.rfind("funcion(", 0) == 0 || tipo.rfind("cierre(", 0) == 0) {
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
    /// ¿Esa variable es una celda compartida de texto?
    bool es_celda_texto(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->celdas_texto.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
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
    /// La estructura que guarda un vector, o vacío si no es de estructuras.
    std::string estructura_de_vector(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            const auto encontrada = it->vector_de_estructura.find(std::string(nombre));
            if (encontrada != it->vector_de_estructura.end()) {
                return encontrada->second;
            }
        }
        return {};
    }
    void declarar_vector_de_estructura(std::string_view nombre, std::string estructura) {
        if (!ambitos_.empty()) {
            ambitos_.back().vector_de_estructura[std::string(nombre)] = std::move(estructura);
        }
    }
    void declarar_mapa_flotante(std::string_view nombre) {
        if (!ambitos_.empty()) {
            ambitos_.back().mapas_flotante.insert(std::string(nombre));
        }
    }
    bool es_mapa_flotante(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->mapas_flotante.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }
    void declarar_vector_flotante(std::string_view nombre) {
        if (!ambitos_.empty()) {
            ambitos_.back().vectores_flotante.insert(std::string(nombre));
        }
    }
    bool es_vector_flotante(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->vectores_flotante.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }
    bool es_vector_texto(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->vectores_texto.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }

    void declarar_compuesto(std::string_view nombre) {
        if (!ambitos_.empty()) {
            ambitos_.back().compuestos.insert(std::string(nombre));
        }
    }
    bool es_compuesto(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->compuestos.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }

    void declarar_referencia(std::string_view nombre) {
        if (!ambitos_.empty()) {
            ambitos_.back().referencias.insert(std::string(nombre));
        }
    }
    bool es_referencia(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->referencias.count(std::string(nombre)) != 0) {
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

    /// Avisa si el valor está prestado: moverlo o reasignarlo dejaría la
    /// referencia colgando.
    void comprobar_prestamo(std::string_view nombre, diagnostico::Span span) {
        const auto prestamo = prestamos_.find(std::string(nombre));
        if (prestamo != prestamos_.end()) {
            error(diagnostico::codigos::MOVER_CON_PRESTAMO, span)
                .parametro("nombre", std::string(nombre))
                .parametro("prestamo", prestamo->second);
        }
    }

    /// Marca un valor como movido. Si estaba prestado, el movimiento lo dejaría
    /// colgando y se informa.
    void marcar_movida(std::string_view nombre, diagnostico::Span span = {}) {
        const std::string clave(nombre);
        comprobar_prestamo(nombre, span);
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->cajas.count(clave) != 0 || it->textos.count(clave) != 0 ||
                it->referencias.count(clave) != 0 || it->cierres.count(clave) != 0 ||
                it->celdas_texto.count(clave) != 0 || it->vectores_texto.count(clave) != 0 ||
                it->vectores_flotante.count(clave) != 0 || it->mapas_flotante.count(clave) != 0) {
                it->movidas.insert(clave);
                return;
            }
        }
    }
    /// Recorre una expresión y marca como movidas las variables que aportan un
    /// campo con dueño a un compuesto: `ficha { nombre: ana }` vacía `ana`, así
    /// que usarla después tiene que ser un error. Se hace en un pase, después de
    /// tipar, porque el tipado de un nodo puede ocurrir más de una vez.
    void marcar_campos_movidos(const Expresion& expresion) {
        if (expresion.tipo == TipoExpresion::Construccion) {
            const sintactico::DefinicionTipo* definicion = tipo_por_nombre(expresion.anotacion);
            if (definicion != nullptr) {
                const std::vector<const sintactico::Campo*> todos =
                    campos_con_heredados(*definicion);
                for (std::size_t i = 0; i < expresion.lista.size(); ++i) {
                    const std::string_view nombre_campo =
                        i < expresion.nombres_campo.size() ? expresion.nombres_campo[i] : "";
                    const sintactico::ExpresionPtr& valor = expresion.lista[i];
                    if (!valor || valor->tipo != TipoExpresion::Identificador) {
                        continue;
                    }
                    for (const sintactico::Campo* declarado : todos) {
                        if (declarado->nombre != nombre_campo) {
                            continue;
                        }
                        const std::string_view tipo_campo = declarado->tipo;
                        const bool con_dueno = tipo_campo == "texto" ||
                                               tipo_campo.rfind("Vector<", 0) == 0 ||
                                               tipo_campo.rfind("Mapa<", 0) == 0 ||
                                               tipo_campo.rfind("Caja<", 0) == 0 ||
                                               tipo_campo.rfind("Compartido<", 0) == 0 ||
                                               tipo_campo.rfind("cierre(", 0) == 0;
                        if (con_dueno) {
                            marcar_movida_siempre(valor->lexema);
                        }
                        break;
                    }
                }
            }
        }
        if (expresion.izquierda) {
            marcar_campos_movidos(*expresion.izquierda);
        }
        if (expresion.derecha) {
            marcar_campos_movidos(*expresion.derecha);
        }
        for (const sintactico::ExpresionPtr& hijo : expresion.lista) {
            if (hijo) {
                marcar_campos_movidos(*hijo);
            }
        }
    }

    /// Marca una variable como movida sin mirar de qué tipo es: el que llama ya
    /// lo sabe (un campo con dueño de un compuesto, por ejemplo).
    void marcar_movida_siempre(std::string_view nombre) {
        comprobar_prestamo(nombre, {});
        if (!ambitos_.empty()) {
            ambitos_.back().movidas.insert(std::string(nombre));
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

    /// Tipo de retorno declarado de una llamada, si se puede saber.
    std::string_view retorno_de_llamada(const Expresion& expresion) const {
        if (expresion.tipo != TipoExpresion::Llamada || !expresion.izquierda) {
            return {};
        }
        const Expresion& destino = *expresion.izquierda;
        if (destino.tipo == TipoExpresion::Identificador) {
            const auto funcion = funciones_.find(destino.lexema);
            return funcion != funciones_.end() ? funcion->second->tipo_retorno : std::string_view{};
        }
        if (destino.tipo == TipoExpresion::Miembro && destino.izquierda &&
            destino.izquierda->tipo == TipoExpresion::Identificador) {
            const auto modulo = modulos_.find(destino.izquierda->lexema);
            if (modulo != modulos_.end()) {
                const auto funcion = modulo->second.find(destino.anotacion);
                if (funcion != modulo->second.end()) {
                    return funcion->second->tipo_retorno;
                }
            }
        }
        return {};
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

    void abrir_ambito() {
        ambitos_.emplace_back();
    }
    void cerrar_ambito() {
        if (!ambitos_.empty()) {
            ambitos_.pop_back();
        }
    }
    void declarar(std::string_view nombre_variable, Tipo tipo, std::string_view estructura = {}) {
        if (ambitos_.empty()) {
            return;
        }
        Ambito& ambito = ambitos_.back();
        ambito.tipos[std::string(nombre_variable)] = tipo;
        if (!estructura.empty()) {
            ambito.estructuras[std::string(nombre_variable)] = std::string(estructura);
        }
    }
    Tipo buscar(std::string_view nombre_variable, diagnostico::Span span) {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            const auto encontrada = it->tipos.find(std::string(nombre_variable));
            if (encontrada != it->tipos.end()) {
                return encontrada->second;
            }
        }
        // Un nombre que es una función, sin llamarla, es una función como valor.
        if (funciones_.count(std::string(nombre_variable)) != 0) {
            return Tipo::Desconocido;
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
        const auto encontrada = tipos_.find(std::string(nombre));
        return encontrada == tipos_.end() ? nullptr : encontrada->second;
    }

    /// Nombre del tipo de estructura de una expresión, si lo tiene.
    std::string_view estructura_de(const Expresion& expresion) const {
        switch (expresion.tipo) {
        case TipoExpresion::Identificador:
            return buscar_estructura(expresion.lexema);
        case TipoExpresion::Este:
        case TipoExpresion::Super:
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
                    // El `string_view` se construye explícito: si no, el
                    // ternario crea un `std::string` temporal y la vista
                    // queda apuntando a memoria muerta.
                    return tipo_por_nombre(campo.tipo) != nullptr ? std::string_view(campo.tipo)
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
    /// Busca un método por nombre en la jerarquía de una clase.
    const sintactico::Funcion* metodo_en_jerarquia(const sintactico::DefinicionTipo& definicion,
                                                   std::string_view nombre) const {
        const sintactico::DefinicionTipo* actual = &definicion;
        for (int guarda = 0; guarda < 32 && actual != nullptr; ++guarda) {
            for (const sintactico::Funcion& metodo : actual->metodos) {
                if (metodo.nombre == nombre) {
                    return &metodo;
                }
            }
            const auto base = tipos_.find(std::string(actual->base));
            actual = base != tipos_.end() ? base->second : nullptr;
        }
        return nullptr;
    }

    /// ¿Respeta `sobrescrito` la firma de `base`?
    static bool firma_compatible(const Funcion& sobrescrito, const Funcion& base) {
        if (sobrescrito.parametros.size() != base.parametros.size()) {
            return false;
        }
        if (sobrescrito.tipo_retorno != base.tipo_retorno) {
            return false;
        }
        for (std::size_t i = 0; i < base.parametros.size(); ++i) {
            if (sobrescrito.parametros[i].tipo != base.parametros[i].tipo) {
                return false;
            }
        }
        return true;
    }

    /// Comprueba que un tipo cumple los rasgos que declara implementar.
    void revisar_rasgos() {
        for (const sintactico::DefinicionTipo& definicion : programa_.tipos) {
            for (const std::string_view nombre_rasgo : definicion.rasgos) {
                const auto rasgo = rasgos_.find(std::string(nombre_rasgo));
                if (rasgo == rasgos_.end()) {
                    error(diagnostico::codigos::TIPO_DESCONOCIDO, definicion.span)
                        .parametro("nombre", std::string(nombre_rasgo));
                    continue;
                }
                for (const sintactico::Funcion& exigido : rasgo->second->metodos) {
                    const sintactico::Funcion* propio = nullptr;
                    for (const sintactico::Funcion& metodo : definicion.metodos) {
                        if (metodo.nombre == exigido.nombre) {
                            propio = &metodo;
                            break;
                        }
                    }
                    if (propio == nullptr) {
                        error(diagnostico::codigos::METODO_DE_RASGO_FALTANTE, definicion.span)
                            .parametro("tipo", std::string(definicion.nombre))
                            .parametro("metodo", std::string(exigido.nombre))
                            .parametro("rasgo", std::string(nombre_rasgo));
                        continue;
                    }
                    if (!firma_compatible(*propio, exigido)) {
                        error(diagnostico::codigos::FIRMA_DE_RASGO, propio->span)
                            .parametro("metodo", std::string(exigido.nombre))
                            .parametro("rasgo", std::string(nombre_rasgo));
                    }
                }
            }
        }
    }

    void revisar_sobrescrituras() {
        for (const sintactico::DefinicionTipo& definicion : programa_.tipos) {
            if (definicion.base.empty()) {
                continue;
            }
            const auto padre = tipos_.find(std::string(definicion.base));
            if (padre == tipos_.end()) {
                continue;
            }
            for (const sintactico::Funcion& metodo : definicion.metodos) {
                const sintactico::Funcion* heredado =
                    metodo_en_jerarquia(*padre->second, metodo.nombre);
                if (heredado == nullptr) {
                    continue;
                }
                if (!firma_compatible(metodo, *heredado)) {
                    error(diagnostico::codigos::FIRMA_INCOMPATIBLE, metodo.span)
                        .parametro("metodo", std::string(metodo.nombre))
                        .parametro("base", std::string(definicion.base))
                        .ayuda("la firma debe coincidir con la del método heredado");
                }
            }
        }
    }

    void revisar_funcion(const Funcion& funcion) {
        funcion_actual_ = &funcion;
        const std::size_t previos = parametros_tipo_.size();
        for (const std::string_view parametro : funcion.tipo_parametros) {
            parametros_tipo_.insert(std::string(parametro));
        }

        parametros_actuales_.clear();
        for (const sintactico::Parametro& parametro : funcion.parametros) {
            parametros_actuales_.insert(std::string(parametro.nombre));
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
            if (parametro.tipo == "&texto" || parametro.tipo == "&mutable texto") {
                prestamos_texto_.insert(std::string(parametro.nombre));
            }
            // Un parámetro `&Vector<texto>` también es un vector de textos.
            if (parametro.tipo.find("Vector<texto>") != std::string_view::npos) {
                declarar_vector_texto(parametro.nombre);
            }
            // Un parámetro que es una función o un cierre se recuerda para poder
            // llamarlo.
            if (parametro.tipo.rfind("funcion(", 0) == 0 ||
                parametro.tipo.rfind("cierre(", 0) == 0) {
                funciones_parametro[std::string(parametro.nombre)] = std::string(parametro.tipo);
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

    /// Comprueba que un tipo escrito existe: primitivo, declarado o genérico.
    void revisar_tipo_declarado(std::string_view tipo, diagnostico::Span span) {
        if (tipo.empty()) {
            return;
        }
        std::string_view base = tipo;
        if (base.rfind("&mutable ", 0) == 0) {
            base.remove_prefix(9);
        } else if (base.rfind("&", 0) == 0) {
            base.remove_prefix(1);
        }
        const std::size_t angulo = base.find('<');
        if (angulo != std::string_view::npos) {
            base = base.substr(0, angulo);
        }
        if (base.empty()) {
            return;
        }

        // Primitivos que el lenguaje implementa de verdad.
        if (base == "entero" || base == "flotante" || base == "booleano" || base == "texto" ||
            base == "vacio" || base == "nulo") {
            return;
        }
        // `Puntero` es un tipo del lenguaje, aunque su uso esté limitado a
        // `inseguro`: representa una dirección de memoria.
        if (base == "Puntero") {
            return;
        }
        // Un tipo función o un cierre: `funcion(entero) -> entero`.
        if (base.rfind("funcion(", 0) == 0 || base.rfind("cierre(", 0) == 0) {
            return;
        }
        // `Caja` y `Compartido` guardan un entero en el montículo. Con otro tipo
        // el valor se convertiría en silencio —`Caja<flotante>` truncaba 2,5 a 2
        // sin decir nada— así que se rechaza mientras no haya celda por tipo.
        if (base == "Caja" || base == "Compartido") {
            const std::size_t abre = tipo.find('<');
            const std::size_t cierra = tipo.find('>');
            std::string dentro = abre == std::string_view::npos || cierra == std::string_view::npos
                                     ? std::string("entero")
                                     : std::string(tipo.substr(abre + 1, cierra - abre - 1));
            while (!dentro.empty() && dentro.front() == ' ') {
                dentro.erase(dentro.begin());
            }
            while (!dentro.empty() && dentro.back() == ' ') {
                dentro.pop_back();
            }
            const bool celda_de_texto = std::string(base) == "Compartido" && dentro == "texto";
            const bool celda_de_estructura =
                std::string(base) == "Compartido" && es_estructura_simple(dentro);
            if (dentro != "entero" && !celda_de_texto && !celda_de_estructura) {
                error(diagnostico::codigos::NO_SOPORTADO, span)
                    .parametro("construccion", std::string(base) + "<" + dentro + ">")
                    .ayuda(std::string(base) == "Caja"
                               ? "Caja guarda un entero; para otro tipo, todavía no"
                               : "Compartido guarda un entero; para otro tipo, todavía no");
                return;
            }
            return;
        }
        // Genéricos de la biblioteca.
        if (base == "Vector" || base == "Mapa" || base == "resultado") {
            return;
        }
        // Tipos que declara el programa.
        const std::string nombre(base);
        if (tipos_.count(nombre) != 0 || enumeraciones_.count(nombre) != 0 ||
            rasgos_.count(nombre) != 0) {
            return;
        }
        error(diagnostico::codigos::TIPO_DESCONOCIDO, span).parametro("nombre", std::string(base));
    }

    void revisar_sentencia(const Sentencia& sentencia, const Funcion& funcion) {
        switch (sentencia.tipo) {
        case TipoSentencia::Expresion:
            if (sentencia.expresion) {
                // Reasignar es mover: el dueño no puede estar prestado.
                if (sentencia.expresion->tipo == TipoExpresion::Asignacion &&
                    sentencia.expresion->izquierda &&
                    sentencia.expresion->izquierda->tipo == TipoExpresion::Identificador) {
                    comprobar_prestamo(sentencia.expresion->izquierda->lexema,
                                       sentencia.expresion->span);
                }
                tipo_de(*sentencia.expresion);
                marcar_campos_movidos(*sentencia.expresion);
            }
            return;

        case TipoSentencia::Variable:
        case TipoSentencia::Constante: {
            if (!sentencia.expresion) {
                return;
            }
            // El tipo que se escribe tiene que existir. Sin esto, `variable n:
            // natural = 1` se aceptaba y acababa siendo un `entero` en C.
            revisar_tipo_declarado(sentencia.tipo_declarado, sentencia.span);
            if (sentencia.tipo_declarado.rfind("funcion(", 0) == 0 ||
                sentencia.tipo_declarado.rfind("cierre(", 0) == 0) {
                ambitos_.back().funciones[std::string(sentencia.nombre)] =
                    std::string(sentencia.tipo_declarado);
                if (sentencia.tipo_declarado.rfind("cierre(", 0) == 0) {
                    ambitos_.back().cierres.insert(std::string(sentencia.nombre));
                }
            } else if (sentencia.expresion && sentencia.expresion->tipo == TipoExpresion::Lambda) {
                // Una lambda guardada en una variable: su tipo se escribe con la
                // forma del lenguaje, para poder llamarla después.
                std::string firma = "funcion(";
                const Expresion& lambda = *sentencia.expresion;
                for (std::size_t i = 0; i < lambda.tipos_lambda.size(); ++i) {
                    if (i > 0) {
                        firma += ", ";
                    }
                    firma += std::string(lambda.tipos_lambda[i]);
                }
                firma += ") -> " + std::string(lambda.vuelta_lambda);
                ambitos_.back().funciones[std::string(sentencia.nombre)] = firma;
                ambitos_.back().cierres.insert(std::string(sentencia.nombre));
            }
            // `variable eco = Compartido<texto>.nuevo("...")`: su `sacar` da texto.
            if (sentencia.expresion && sentencia.expresion->tipo == TipoExpresion::Llamada &&
                sentencia.expresion->izquierda &&
                sentencia.expresion->izquierda->tipo == TipoExpresion::Miembro &&
                sentencia.expresion->izquierda->izquierda &&
                sentencia.expresion->izquierda->izquierda->tipo == TipoExpresion::Identificador &&
                sentencia.expresion->izquierda->izquierda->lexema == "Compartido" &&
                !sentencia.expresion->lista.empty() && sentencia.expresion->lista.front() &&
                tipo_de(*sentencia.expresion->lista.front()) == Tipo::Texto) {
                ambitos_.back().celdas_texto.insert(std::string(sentencia.nombre));
            }
            if (sentencia.tipo_declarado.rfind("Compartido<texto>", 0) == 0) {
                ambitos_.back().celdas_texto.insert(std::string(sentencia.nombre));
            }
            const Tipo real = tipo_de(*sentencia.expresion);
            marcar_campos_movidos(*sentencia.expresion);

            // Un préstamo no se puede guardar, salvo como referencia a un rasgo.
            const bool referencia_a_rasgo =
                sentencia.tipo_declarado.rfind("&", 0) == 0 &&
                rasgos_.count(std::string(sentencia.tipo_declarado.substr(1))) != 0;
            if (referencia_a_rasgo && sentencia.expresion->tipo == TipoExpresion::Unaria &&
                sentencia.expresion->izquierda &&
                sentencia.expresion->izquierda->tipo == TipoExpresion::Identificador) {
                prestamos_[std::string(sentencia.expresion->izquierda->lexema)] =
                    std::string(sentencia.nombre);
            }
            if (!referencia_a_rasgo && sentencia.expresion->tipo == TipoExpresion::Unaria &&
                (sentencia.expresion->lexema == "&" || sentencia.expresion->lexema == "&mutable")) {
                error(diagnostico::codigos::PRESTAMO_GUARDADO, sentencia.expresion->span)
                    .ayuda("pase el préstamo directamente como argumento");
                return;
            }

            if (sentencia.tipo_declarado.rfind("Vector<", 0) == 0 ||
                sentencia.tipo_declarado.rfind("Mapa<", 0) == 0 ||
                es_llamada_vector(*sentencia.expresion) || es_llamada_mapa(*sentencia.expresion)) {
                declarar_prestable(sentencia.nombre);
            }

            // `Vector<MiEstructura>`: se recuerda qué estructura guarda,
            // para que `obtener` devuelva una y se puedan tocar sus campos.
            {
                std::string dentro;
                const std::string_view declarado = sentencia.tipo_declarado;
                const std::size_t abre = declarado.find("Vector<");
                if (abre != std::string_view::npos) {
                    const std::size_t cierra = declarado.find('>', abre);
                    if (cierra != std::string_view::npos) {
                        dentro = std::string(declarado.substr(abre + 7, cierra - abre - 7));
                    }
                } else if (sentencia.expresion->tipo == TipoExpresion::Llamada &&
                           sentencia.expresion->izquierda &&
                           sentencia.expresion->izquierda->tipo == TipoExpresion::Miembro &&
                           sentencia.expresion->izquierda->izquierda &&
                           sentencia.expresion->izquierda->izquierda->tipo ==
                               TipoExpresion::Identificador &&
                           sentencia.expresion->izquierda->izquierda->lexema == "Vector" &&
                           sentencia.expresion->izquierda->izquierda->tipos_aplicados.size() == 1) {
                    dentro = std::string(
                        sentencia.expresion->izquierda->izquierda->tipos_aplicados.front());
                }
                if (!dentro.empty() && tipos_.count(dentro) != 0) {
                    declarar_vector_de_estructura(sentencia.nombre, dentro);
                }
            }

            // Y si la variable es una estructura sacada de un vector, se
            // declara como estructura para poder prestarla y tocar campos.
            if (sentencia.expresion->tipo == TipoExpresion::Llamada &&
                sentencia.expresion->izquierda &&
                sentencia.expresion->izquierda->tipo == TipoExpresion::Miembro &&
                sentencia.expresion->izquierda->anotacion == "obtener" &&
                sentencia.expresion->izquierda->izquierda &&
                sentencia.expresion->izquierda->izquierda->tipo == TipoExpresion::Identificador) {
                const std::string guarda =
                    estructura_de_vector(sentencia.expresion->izquierda->izquierda->lexema);
                if (!guarda.empty()) {
                    declarar(sentencia.nombre, Tipo::Desconocido, guarda);
                }
            }
            if (!sentencia.tipo_declarado.empty() &&
                tipos_.count(std::string(sentencia.tipo_declarado)) != 0) {
                declarar(sentencia.nombre, Tipo::Desconocido,
                         std::string(sentencia.tipo_declarado));
            }
            // Un `Vector<flotante>` se recuerda para saber que su elemento es
            // un flotante, tanto al obtener como al recorrerlo.
            if (sentencia.tipo_declarado.find("Mapa<") != std::string_view::npos &&
                sentencia.tipo_declarado.find("flotante") != std::string_view::npos) {
                declarar_mapa_flotante(sentencia.nombre);
            }
            // `variable m = Mapa<texto, flotante>.nuevo()`: el tipo está en
            // los tipos aplicados, no en una anotación.
            if (sentencia.expresion->tipo == TipoExpresion::Llamada &&
                sentencia.expresion->izquierda &&
                sentencia.expresion->izquierda->tipo == TipoExpresion::Miembro &&
                sentencia.expresion->izquierda->izquierda &&
                sentencia.expresion->izquierda->izquierda->tipo == TipoExpresion::Identificador &&
                sentencia.expresion->izquierda->izquierda->lexema == "Mapa" &&
                sentencia.expresion->izquierda->izquierda->tipos_aplicados.size() == 2 &&
                sentencia.expresion->izquierda->izquierda->tipos_aplicados[1] == "flotante") {
                declarar_mapa_flotante(sentencia.nombre);
            }
            if (sentencia.tipo_declarado.find("Vector<flotante>") != std::string_view::npos ||
                (sentencia.expresion->tipo == TipoExpresion::Llamada &&
                 sentencia.expresion->izquierda &&
                 sentencia.expresion->izquierda->tipo == TipoExpresion::Miembro &&
                 sentencia.expresion->izquierda->izquierda &&
                 sentencia.expresion->izquierda->izquierda->tipo == TipoExpresion::Identificador &&
                 sentencia.expresion->izquierda->izquierda->lexema == "Vector" &&
                 sentencia.expresion->izquierda->izquierda->tipos_aplicados.size() == 1 &&
                 sentencia.expresion->izquierda->izquierda->tipos_aplicados.front() ==
                     "flotante")) {
                declarar_vector_flotante(sentencia.nombre);
            }
            // Una función propia que devuelve una colección deja un valor
            // prestable, igual que si se hubiera creado aquí.
            if (sentencia.expresion->tipo == TipoExpresion::Llamada &&
                sentencia.expresion->izquierda &&
                sentencia.expresion->izquierda->tipo == TipoExpresion::Identificador) {
                const auto propia =
                    funciones_.find(std::string(sentencia.expresion->izquierda->lexema));
                if (propia != funciones_.end()) {
                    const std::string_view devuelto = propia->second->tipo_retorno;
                    if (devuelto.rfind("Vector<", 0) == 0 || devuelto.rfind("Mapa<", 0) == 0 ||
                        devuelto.rfind("Caja<", 0) == 0) {
                        declarar_prestable(sentencia.nombre);
                    }
                    if (devuelto.find("Vector<flotante>") != std::string_view::npos) {
                        declarar_vector_flotante(sentencia.nombre);
                    }
                    if (devuelto.find("Vector<texto>") != std::string_view::npos) {
                        declarar_vector_texto(sentencia.nombre);
                    }
                }
            }
            // Una llamada a un módulo que devuelve una colección también se presta.
            if (sentencia.expresion->tipo == TipoExpresion::Llamada &&
                sentencia.expresion->izquierda &&
                sentencia.expresion->izquierda->tipo == TipoExpresion::Miembro) {
                const Expresion& acceso = *sentencia.expresion->izquierda;
                if (acceso.izquierda && acceso.izquierda->tipo == TipoExpresion::Identificador) {
                    const auto modulo = modulos_.find(acceso.izquierda->lexema);
                    if (modulo != modulos_.end()) {
                        const auto encontrada = modulo->second.find(acceso.anotacion);
                        if (encontrada != modulo->second.end()) {
                            const std::string_view devuelto = encontrada->second->tipo_retorno;
                            if (devuelto.rfind("Mapa<", 0) == 0 ||
                                devuelto.rfind("Vector<", 0) == 0 ||
                                devuelto.rfind("Caja<", 0) == 0) {
                                declarar_prestable(sentencia.nombre);
                            }
                            // Y se recuerda de qué es el vector, para que
                            // `obtener` devuelva el tipo correcto.
                            if (devuelto.find("Vector<texto>") != std::string_view::npos) {
                                declarar_vector_texto(sentencia.nombre);
                            }
                            if (devuelto.find("Vector<flotante>") != std::string_view::npos) {
                                declarar_vector_flotante(sentencia.nombre);
                            }
                        }
                    }
                }
            }
            if (es_llamada_vector(*sentencia.expresion)) {
                const Expresion& destino = *sentencia.expresion->izquierda;
                if (destino.izquierda && !destino.izquierda->tipos_aplicados.empty() &&
                    destino.izquierda->tipos_aplicados.front() == "texto") {
                    declarar_vector_texto(sentencia.nombre);
                }
            }
            if (real == Tipo::Texto && sentencia.tipo_declarado.empty()) {
                if (sentencia.expresion->tipo == TipoExpresion::Identificador &&
                    es_texto(sentencia.expresion->lexema)) {
                    marcar_movida(sentencia.expresion->lexema, sentencia.span);
                }
                declarar_texto(sentencia.nombre);
                declarar_prestable(sentencia.nombre);
                declarar(sentencia.nombre, real);
                return;
            }

            // Un compuesto genérico con campos con dueño no se puede copiar.
            if (sentencia.expresion->tipo == TipoExpresion::Identificador &&
                es_compuesto(sentencia.expresion->lexema)) {
                error(diagnostico::codigos::COPIA_COMPUESTO, sentencia.expresion->span)
                    .parametro("nombre", std::string(sentencia.expresion->lexema))
                    .ayuda("construya otro o páselo con «&»");
            }

            // Se construye un compuesto: puede tener campos con dueño.
            if (sentencia.expresion->tipo == TipoExpresion::Construccion &&
                sentencia.expresion->anotacion.find('<') != std::string::npos) {
                declarar_compuesto(sentencia.nombre);
            }

            // ¿Declara una clase por referencia?
            if (sentencia.expresion->tipo == TipoExpresion::Construccion &&
                sentencia.expresion->monticulo) {
                declarar_referencia(sentencia.nombre);
                declarar_prestable(sentencia.nombre);
                // Se recuerda el tipo concreto para leer sus campos, salvo que
                // se haya declarado un tipo base: ahí manda el declarado.
                declarar(sentencia.nombre, real,
                         sentencia.tipo_declarado.empty() ? sentencia.expresion->anotacion
                                                          : sentencia.tipo_declarado);
                return;
            }
            if (sentencia.expresion->tipo == TipoExpresion::Identificador &&
                es_referencia(sentencia.expresion->lexema)) {
                marcar_movida(sentencia.expresion->lexema, sentencia.span);
                declarar_referencia(sentencia.nombre);
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
                marcar_movida(sentencia.expresion->lexema, sentencia.span);
                declarar_caja(sentencia.nombre);
                declarar(sentencia.nombre, real);
                return;
            }

            const bool es_construccion = sentencia.expresion->tipo == TipoExpresion::Construccion;
            std::string_view estructura;
            if (es_construccion) {
                estructura = sentencia.expresion->anotacion;
            } else {
                const std::string_view devuelto = retorno_de_llamada(*sentencia.expresion);
                if (!devuelto.empty() && tipo_por_nombre(devuelto) != nullptr) {
                    estructura = devuelto;
                }
            }
            if (sentencia.tipo_declarado.empty()) {
                declarar(sentencia.nombre, real, estructura);
                return;
            }
            const Tipo declarado = desde_nombre(sentencia.tipo_declarado);
            exigir(declarado, real, sentencia.expresion->span);
            declarar(sentencia.nombre, declarado,
                     tipo_por_nombre(sentencia.tipo_declarado) != nullptr ? sentencia.tipo_declarado
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
                marcar_campos_movidos(*sentencia.expresion);
            }
            if (sentencia.paso) {
                exigir(Tipo::Entero, tipo_de(*sentencia.paso), sentencia.paso->span);
            }
            abrir_ambito();
            // La variable de un `para` sobre una colección toma el tipo del
            // elemento: texto si el vector es de textos, entero si no.
            const bool sobre_textos = sentencia.expresion &&
                                      sentencia.expresion->tipo == TipoExpresion::Identificador &&
                                      es_vector_texto(sentencia.expresion->lexema);
            const bool sobre_flotantes =
                sentencia.expresion && sentencia.expresion->tipo == TipoExpresion::Identificador &&
                es_vector_flotante(sentencia.expresion->lexema);
            if (!sentencia.indice.empty()) {
                declarar(sentencia.indice, Tipo::Entero);
            }
            declarar(sentencia.nombre, sobre_textos      ? Tipo::Texto
                                       : sobre_flotantes ? Tipo::Flotante
                                                         : Tipo::Entero);
            if (sobre_textos) {
                declarar_vector_texto(sentencia.nombre);
            }
            if (sobre_flotantes) {
                declarar_vector_flotante(sentencia.nombre);
            }
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

        case TipoSentencia::Retornar:
            if (sentencia.expresion) {
                revisar_escape(*sentencia.expresion, funcion);
            }
            {
                revisar_retorno(sentencia, funcion);
                return;
            }

        case TipoSentencia::Asm:
            if (profundidad_inseguro_ == 0) {
                error(diagnostico::codigos::ASM_FUERA_DE_INSEGURO, sentencia.span)
                    .ayuda("envuélvalo en «inseguro { }» y documente por qué");
            }
            return;

        case TipoSentencia::Inseguro:
            ++profundidad_inseguro_;
            revisar_cuerpo(sentencia.cuerpo, funcion);
            --profundidad_inseguro_;
            return;

        case TipoSentencia::Bloque:
            revisar_cuerpo(sentencia.cuerpo, funcion);
            return;

        case TipoSentencia::Coincide:
            revisar_coincide(sentencia, funcion);
            return;
        }
    }

    /// Un préstamo a una variable local no puede salir de la función.
    void revisar_escape(const Expresion& expresion, const Funcion& funcion) {
        if (expresion.tipo == TipoExpresion::Unaria &&
            (expresion.lexema == "&" || expresion.lexema == "&mutable") && expresion.izquierda &&
            expresion.izquierda->tipo == TipoExpresion::Identificador) {
            const std::string_view nombre = expresion.izquierda->lexema;
            if (parametros_actuales_.count(std::string(nombre)) == 0) {
                error(diagnostico::codigos::PRESTAMO_QUE_ESCAPA, expresion.span)
                    .parametro("nombre", std::string(nombre))
                    .ayuda("devuelva el valor por movimiento o reciba el préstamo como parámetro");
            }
        }
        // Los argumentos de una llamada solo viven durante la llamada: no escapan.
        if (expresion.tipo != TipoExpresion::Llamada) {
            for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                if (argumento) {
                    revisar_escape(*argumento, funcion);
                }
            }
        }
        if (expresion.izquierda) {
            revisar_escape(*expresion.izquierda, funcion);
        }
        if (expresion.derecha) {
            revisar_escape(*expresion.derecha, funcion);
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
                revisar_cuerpo(rama.cuerpo, funcion);
                continue;
            }

            if (rama.patron.clase == Patron::Clase::Variante &&
                rama.patron.enumeracion == rama.patron.valor) {
                // Variante con datos: `caso Bien(v)`. Las ligaduras viven en la rama.
                const auto variante = variantes_.find(std::string(rama.patron.valor));
                if (variante == variantes_.end()) {
                    error(diagnostico::codigos::TIPO_DESCONOCIDO, rama.patron.span)
                        .parametro("nombre", std::string(rama.patron.valor));
                    revisar_cuerpo(rama.cuerpo, funcion);
                    continue;
                }
                const sintactico::Variante& datos =
                    variante->second.first->variantes[variante->second.second];
                if (rama.patron.ligaduras.size() != datos.campos.size()) {
                    error(diagnostico::codigos::ARIDAD_INCORRECTA, rama.patron.span)
                        .parametro("funcion", std::string(rama.patron.valor))
                        .parametro("esperados", std::to_string(datos.campos.size()))
                        .parametro("dados", std::to_string(rama.patron.ligaduras.size()));
                }
                abrir_ambito();
                for (std::size_t i = 0; i < datos.campos.size() && i < rama.patron.ligaduras.size();
                     ++i) {
                    declarar(rama.patron.ligaduras[i], desde_nombre(datos.campos[i].tipo));
                }
                revisar_cuerpo(rama.cuerpo, funcion);
                cerrar_ambito();
                continue;
            }

            if (rama.patron.clase == Patron::Clase::Variante) {
                const auto enumeracion = enumeraciones_.find(std::string(rama.patron.enumeracion));
                if (enumeracion == enumeraciones_.end()) {
                    error(diagnostico::codigos::TIPO_DESCONOCIDO, rama.patron.span)
                        .parametro("nombre", std::string(rama.patron.enumeracion));
                } else {
                    bool existe = false;
                    for (const sintactico::Variante& variante : enumeracion->second->variantes) {
                        if (variante.nombre == rama.patron.valor) {
                            existe = true;
                            break;
                        }
                    }
                    if (!existe) {
                        error(diagnostico::codigos::VARIANTE_DESCONOCIDA, rama.patron.span)
                            .parametro("enumeracion", std::string(enumeracion->second->nombre))
                            .parametro("variante", std::string(rama.patron.valor));
                    }
                }
            }

            revisar_cuerpo(rama.cuerpo, funcion);
        }

        // Cubrir todas las variantes de una enumeración con datos también es exhaustivo.
        if (!hay_comodin) {
            const sintactico::Enumeracion* con_datos = nullptr;
            std::set<std::string> cubiertas;
            for (const Rama& rama : sentencia.ramas) {
                if (rama.patron.clase != Patron::Clase::Variante) {
                    continue;
                }
                // Dos formas de nombrar una variante: `Variante(datos)` —donde el
                // nombre hace de enumeración— o `Enumeracion.variante`, que es la
                // única para las variantes sin datos.
                const auto variante = variantes_.find(std::string(rama.patron.valor));
                if (variante == variantes_.end()) {
                    continue;
                }
                if (rama.patron.enumeracion != rama.patron.valor &&
                    variante->second.first->nombre != rama.patron.enumeracion) {
                    continue;
                }
                con_datos = variante->second.first;
                cubiertas.insert(std::string(rama.patron.valor));
            }
            const bool completo =
                con_datos != nullptr && cubiertas.size() >= con_datos->variantes.size();
            if (!completo) {
                error(diagnostico::codigos::COINCIDE_NO_EXHAUSTIVO, sentencia.span)
                    .ayuda("añada «caso _ => ...» para los casos que falten");
            }
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
                expresion.lexema == "Mapa" || expresion.lexema == "Compartido") {
                // La celda solo guarda enteros: con otro tipo el valor se
                // convertiría en silencio, así que se rechaza aquí también.
                const bool es_celda =
                    expresion.lexema == "Caja" || expresion.lexema == "Compartido";
                const bool texto_ok = expresion.lexema == "Compartido" &&
                                      !expresion.tipos_aplicados.empty() &&
                                      expresion.tipos_aplicados.front() == "texto";
                const bool estructura_ok = expresion.lexema == "Compartido" &&
                                           !expresion.tipos_aplicados.empty() &&
                                           es_estructura_simple(expresion.tipos_aplicados.front());
                if (es_celda && !expresion.tipos_aplicados.empty() &&
                    expresion.tipos_aplicados.front() != "entero" && !texto_ok && !estructura_ok) {
                    error(diagnostico::codigos::NO_SOPORTADO, expresion.span)
                        .parametro("construccion",
                                   std::string(expresion.lexema) + "<" +
                                       std::string(expresion.tipos_aplicados.front()) + ">")
                        .ayuda("la celda guarda un entero; para otro tipo, todavía no");
                }
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
        case TipoExpresion::Propagacion: {
            // `?` solo tiene sentido en una función que devuelve un resultado.
            bool devuelve_resultado = false;
            if (funcion_actual_ != nullptr) {
                const auto enumeracion =
                    enumeraciones_.find(std::string(funcion_actual_->tipo_retorno));
                if (enumeracion != enumeraciones_.end() &&
                    enumeracion->second->variantes.size() >= 2 &&
                    enumeracion->second->variantes[0].campos.size() == 1) {
                    devuelve_resultado = true;
                }
            }
            if (!devuelve_resultado) {
                error(diagnostico::codigos::PROPAGACION_SIN_RESULTADO, expresion.span)
                    .ayuda("devuelva un resultado desde esta función o trate el error");
            }
            return Tipo::Entero;
        }
        case TipoExpresion::Miembro:
            return tipo_miembro(expresion);
        case TipoExpresion::Este:
        case TipoExpresion::Super:
            return Tipo::Desconocido;
        case TipoExpresion::Lambda: {
            // Los parámetros son lo único que la lambda ve: una variable de
            // fuera no se puede usar, porque no hay cierres.
            abrir_ambito();
            for (std::size_t i = 0; i < expresion.nombres_lambda.size(); ++i) {
                const std::string_view tipo_parametro = i < expresion.tipos_lambda.size()
                                                            ? expresion.tipos_lambda[i]
                                                            : std::string_view("entero");
                declarar(expresion.nombres_lambda[i], desde_nombre(tipo_parametro), tipo_parametro);
            }
            if (expresion.cuerpo_lambda) {
                revisar_cuerpo_lambda(*expresion.cuerpo_lambda, expresion);
                const Tipo cuerpo = tipo_de(*expresion.cuerpo_lambda);
                const Tipo esperado = desde_nombre(expresion.vuelta_lambda);
                if (esperado != Tipo::Desconocido && cuerpo != Tipo::Desconocido &&
                    cuerpo != esperado) {
                    error(diagnostico::codigos::TIPO_INCOMPATIBLE, expresion.span)
                        .parametro("esperado", std::string(expresion.vuelta_lambda))
                        .parametro("encontrado", "otra cosa");
                }
            }
            cerrar_ambito();
            return Tipo::Desconocido;
        }
        case TipoExpresion::Flotante:
            return Tipo::Flotante;
        case TipoExpresion::Como: {
            // `x como tipo`: el resultado tiene el tipo pedido, si existe.
            const Tipo destino = desde_nombre(expresion.anotacion);
            if (destino == Tipo::Desconocido) {
                error(diagnostico::codigos::TIPO_DESCONOCIDO, expresion.span)
                    .parametro("nombre", std::string(expresion.anotacion));
            }
            return destino;
        }
        default:
            // Caracteres, índices y conversiones: el generador los rechaza con
            // NOK6003 cuando llegue el momento.
            return Tipo::Desconocido;
        }
    }

    Tipo tipo_unaria(const Expresion& expresion) {
        const Tipo operando =
            expresion.izquierda ? tipo_de(*expresion.izquierda) : Tipo::Desconocido;
        if (expresion.lexema == "!") {
            exigir(Tipo::Booleano, operando, expresion.span);
            return Tipo::Booleano;
        }
        if (expresion.lexema == "-") {
            if (operando == Tipo::Flotante) {
                return Tipo::Flotante;
            }
            exigir(Tipo::Entero, operando, expresion.span);
            return Tipo::Entero;
        }
        if (expresion.lexema == "*") {
            // Desreferenciar un `&texto` da un texto, no un entero.
            if (expresion.izquierda && expresion.izquierda->tipo == TipoExpresion::Identificador &&
                prestamos_texto_.count(std::string(expresion.izquierda->lexema)) != 0) {
                return Tipo::Texto;
            }
            const bool caja = expresion.izquierda &&
                              expresion.izquierda->tipo == TipoExpresion::Identificador &&
                              es_caja(expresion.izquierda->lexema);
            const bool prestamo_de_caja =
                expresion.izquierda && expresion.izquierda->tipo == TipoExpresion::Identificador &&
                es_prestamo_declarado(expresion.izquierda->lexema);
            if (!caja && !prestamo_de_caja) {
                error(diagnostico::codigos::DEREFERENCIA_INVALIDA, expresion.span);
            }
            return Tipo::Entero;
        }
        if (expresion.lexema == "&" || expresion.lexema == "&mutable") {
            // Un campo es un sitio válido: `&objeto.campo` presta el campo.
            const bool campo =
                expresion.izquierda && expresion.izquierda->tipo == TipoExpresion::Miembro;
            // Se puede prestar una caja, una colección o un texto.
            const bool caja = expresion.izquierda &&
                              expresion.izquierda->tipo == TipoExpresion::Identificador &&
                              (es_caja(expresion.izquierda->lexema) ||
                               es_prestable(expresion.izquierda->lexema) ||
                               es_texto(expresion.izquierda->lexema) ||
                               tipo_de(*expresion.izquierda) == Tipo::Texto);
            if (campo) {
                tipo_de(*expresion.izquierda);
                return Tipo::Texto;
            }
            // También se presta el resultado de una llamada (el compilador le da
            // un sitio temporal) y una variable cualquiera, incluidas las de un
            // tipo propio, que es como se pasa una estructura sin copiarla.
            const bool llamada =
                expresion.izquierda && expresion.izquierda->tipo == TipoExpresion::Llamada;
            // Una variable suelta también: es como se pasa una estructura sin
            // copiarla. Lo que no vale es prestar algo que ya es un préstamo:
            // eso sería un puntero a puntero.
            const bool variable =
                expresion.izquierda &&
                (expresion.izquierda->tipo == TipoExpresion::Identificador ||
                 expresion.izquierda->tipo == TipoExpresion::Miembro ||
                 expresion.izquierda->tipo == TipoExpresion::Indice) &&
                !es_prestamo_declarado(std::string_view(expresion.izquierda->lexema)) &&
                (expresion.izquierda->tipo != TipoExpresion::Identificador ||
                 !buscar_estructura(expresion.izquierda->lexema).empty() ||
                 es_caja(expresion.izquierda->lexema) ||
                 es_prestable(expresion.izquierda->lexema) ||
                 es_texto(expresion.izquierda->lexema) ||
                 tipo_de(*expresion.izquierda) == Tipo::Texto);
            if (!caja && !llamada && !variable) {
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

        // Flotantes: las cuatro operaciones y la comparación, siempre entre
        // flotantes. No hay conversión implícita: se pide con «como».
        if (izquierda == Tipo::Flotante && derecha == Tipo::Flotante) {
            if (operador == "+" || operador == "-" || operador == "*" || operador == "/") {
                return Tipo::Flotante;
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
            // Dos booleanos se comparan con `==` y `!=`; el orden no tiene sentido.
            if (izquierda == Tipo::Booleano && derecha == Tipo::Booleano) {
                if (operador != "==" && operador != "!=") {
                    error(diagnostico::codigos::TIPO_INCOMPATIBLE, expresion.span)
                        .parametro("esperado", "un valor comparable por orden")
                        .parametro("encontrado", "booleano");
                }
                return Tipo::Booleano;
            }
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
        // `g = f` con dos cierres es mover el entorno de `f` a `g`: `f` deja de
        // poder usarse, como cualquier valor con dueño.
        if (expresion.izquierda && expresion.derecha &&
            expresion.izquierda->tipo == TipoExpresion::Identificador &&
            expresion.derecha->tipo == TipoExpresion::Identificador &&
            es_cierre(expresion.izquierda->lexema) && es_cierre(expresion.derecha->lexema)) {
            const Tipo resultado = tipo_de(*expresion.izquierda);
            marcar_movida(expresion.derecha->lexema, expresion.span);
            return resultado;
        }
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
            marcar_movida(expresion.derecha->lexema, expresion.span);
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

    /// Una estructura que se puede meter en una celda compartida: existe y no
    /// tiene campos con dueño, así que copiarla es copiar sus valores.
    bool es_estructura_simple(std::string_view nombre) const {
        const sintactico::DefinicionTipo* definicion = tipo_por_nombre(nombre);
        if (definicion == nullptr) {
            return false;
        }
        for (const sintactico::Campo& campo : definicion->campos) {
            const std::string_view tipo = campo.tipo;
            if (tipo == "texto" || tipo.rfind("Vector<", 0) == 0 || tipo.rfind("Mapa<", 0) == 0 ||
                tipo.rfind("Caja<", 0) == 0 || tipo.rfind("Compartido<", 0) == 0 ||
                tipo.rfind("cierre(", 0) == 0) {
                return false;
            }
        }
        return true;
    }

    /// ¿Esa variable guarda un cierre?
    bool es_cierre(std::string_view nombre) const {
        for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
            if (it->cierres.count(std::string(nombre)) != 0) {
                return true;
            }
        }
        return false;
    }

    /// Dentro de una lambda solo valen sus parámetros y las funciones: una
    /// variable de fuera no existe, porque no hay cierres. Se revisa aquí para
    /// que el error sea del lenguaje y no del compilador de C.
    void revisar_cuerpo_lambda(const Expresion& expresion, const Expresion& lambda,
                               bool raiz = true) {
        if (!raiz && expresion.tipo == TipoExpresion::Identificador) {
            const std::string_view nombre = expresion.lexema;
            bool es_parametro = false;
            for (const std::string_view suyo : lambda.nombres_lambda) {
                if (suyo == nombre) {
                    es_parametro = true;
                    break;
                }
            }
            const bool es_funcion = funciones_.count(std::string(nombre)) != 0;
            const bool es_tipo = tipo_por_nombre(nombre) != nullptr ||
                                 enumeraciones_.count(std::string(nombre)) != 0;
            if (!es_parametro && !es_funcion && !es_tipo) {
                // Una variable de fuera se captura copiándola, y solo si se puede
                // copiar: las colecciones se mueven, no se copian.
                const std::string_view tipo = buscar_estructura(nombre);
                const Tipo declarado = buscar(nombre, expresion.span);
                (void)tipo;
                if (declarado == Tipo::Desconocido && !declarada(nombre)) {
                    error(diagnostico::codigos::NOMBRE_NO_DECLARADO, expresion.span)
                        .parametro("nombre", std::string(nombre));
                }
            }
            return;
        }
        if (expresion.izquierda) {
            revisar_cuerpo_lambda(*expresion.izquierda, lambda, false);
        }
        if (expresion.derecha) {
            revisar_cuerpo_lambda(*expresion.derecha, lambda, false);
        }
        for (const sintactico::ExpresionPtr& hijo : expresion.lista) {
            if (hijo) {
                revisar_cuerpo_lambda(*hijo, lambda, false);
            }
        }
        if (expresion.cuerpo_lambda) {
            revisar_cuerpo_lambda(*expresion.cuerpo_lambda, lambda, false);
        }
    }

    Tipo tipo_llamada(const Expresion& expresion) {
        if (!expresion.izquierda) {
            return Tipo::Desconocido;
        }
        // Llamar a algo que se movió es un error: su valor ya no está aquí.
        if (expresion.izquierda->tipo == TipoExpresion::Identificador &&
            esta_movida(expresion.izquierda->lexema)) {
            error(diagnostico::codigos::USO_TRAS_MOVER, expresion.span)
                .parametro("nombre", std::string(expresion.izquierda->lexema));
            return Tipo::Desconocido;
        }
        comprobar_exclusividad(expresion);

        if (expresion.izquierda->tipo == TipoExpresion::Miembro) {
            const Expresion& acceso = *expresion.izquierda;
            if (acceso.izquierda && acceso.izquierda->tipo == TipoExpresion::Identificador) {
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
                    return comprobar_argumentos(*funcion->second, expresion,
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

        // Constructor de una variante con datos: `Bien(5)`.
        if (const auto variante = variantes_.find(std::string(nombre_llamado));
            variante != variantes_.end()) {
            const sintactico::Variante& datos =
                variante->second.first->variantes[variante->second.second];
            if (!datos.campos.empty()) {
                if (expresion.lista.size() != datos.campos.size()) {
                    error(diagnostico::codigos::ARIDAD_INCORRECTA, expresion.span)
                        .parametro("funcion", std::string(nombre_llamado))
                        .parametro("esperados", std::to_string(datos.campos.size()))
                        .parametro("dados", std::to_string(expresion.lista.size()));
                }
                for (std::size_t i = 0; i < expresion.lista.size() && i < datos.campos.size();
                     ++i) {
                    if (expresion.lista[i]) {
                        const Tipo real = tipo_de(*expresion.lista[i]);
                        exigir(desde_nombre(datos.campos[i].tipo), real, expresion.lista[i]->span);
                    }
                }
                return Tipo::Desconocido;
            }
        }

        // Tablas de datos.
        if (nombre_llamado == "tabla_de_consulta") {
            for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                if (argumento) {
                    tipo_de(*argumento);
                }
            }
            return Tipo::Entero;
        }
        if (nombre_llamado == "indice_de_columna_en_tabla") {
            for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                if (argumento) {
                    tipo_de(*argumento);
                }
            }
            return Tipo::Entero;
        }
        if (nombre_llamado == "agregar_columna_a_tabla") {
            for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                if (argumento) {
                    tipo_de(*argumento);
                }
            }
            return Tipo::Entero;
        }
        if (nombre_llamado == "numero_de_tabla" || nombre_llamado == "entero_de_tabla") {
            for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                if (argumento) {
                    tipo_de(*argumento);
                }
            }
            return nombre_llamado == "numero_de_tabla" ? Tipo::Flotante : Tipo::Entero;
        }
        if (nombre_llamado == "filas_de_tabla" || nombre_llamado == "columnas_de_tabla" ||
            nombre_llamado == "valor_de_tabla" || nombre_llamado == "nombre_de_columna" ||
            nombre_llamado == "liberar_tabla") {
            for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                if (argumento) {
                    tipo_de(*argumento);
                }
            }
            if (nombre_llamado == "valor_de_tabla" || nombre_llamado == "nombre_de_columna") {
                return Tipo::Texto;
            }
            return nombre_llamado == "liberar_tabla" ? Tipo::Vacio : Tipo::Entero;
        }

        // Argumentos del programa.
        if (nombre_llamado == "cuantos_argumentos_programa" || nombre_llamado == "argumento_de" ||
            nombre_llamado == "argumentos_programa") {
            const std::size_t esperados = (nombre_llamado == "cuantos_argumentos_programa" ||
                                           nombre_llamado == "argumentos_programa")
                                              ? 0
                                              : 1;
            if (expresion.lista.size() != esperados) {
                error(diagnostico::codigos::ARIDAD_INCORRECTA, expresion.span)
                    .parametro("funcion", std::string(nombre_llamado))
                    .parametro("esperados", std::to_string(esperados))
                    .parametro("dados", std::to_string(expresion.lista.size()));
            }
            for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                if (argumento) {
                    tipo_de(*argumento);
                }
            }
            if (nombre_llamado == "argumentos_programa") {
                return Tipo::Desconocido;
            }
            return nombre_llamado == "argumento_de" ? Tipo::Texto : Tipo::Entero;
        }

        // Funciones de datos: hablan con la biblioteca de análisis.
        if (nombre_llamado == "consultar_datos" || nombre_llamado == "ejecutar_datos" ||
            nombre_llamado == "disponible_datos" || nombre_llamado == "version_datos" ||
            nombre_llamado == "error_datos") {
            const std::size_t esperados =
                (nombre_llamado == "disponible_datos" || nombre_llamado == "version_datos" ||
                 nombre_llamado == "error_datos")
                    ? 0
                    : 1;
            if (expresion.lista.size() != esperados) {
                error(diagnostico::codigos::ARIDAD_INCORRECTA, expresion.span)
                    .parametro("funcion", std::string(nombre_llamado))
                    .parametro("esperados", std::to_string(esperados))
                    .parametro("dados", std::to_string(expresion.lista.size()));
            }
            for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                if (argumento) {
                    tipo_de(*argumento);
                }
            }
            if (nombre_llamado == "consultar_datos" || nombre_llamado == "version_datos" ||
                nombre_llamado == "error_datos") {
                return Tipo::Texto;
            }
            return Tipo::Entero;
        }

        if (nombre_llamado == "ejecutar_orden" || nombre_llamado == "capturar_orden") {
            if (expresion.lista.size() != 1) {
                error(diagnostico::codigos::ARIDAD_INCORRECTA, expresion.span)
                    .parametro("funcion", std::string(nombre_llamado))
                    .parametro("esperados", "1")
                    .parametro("dados", std::to_string(expresion.lista.size()));
            }
            for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                if (argumento) {
                    tipo_de(*argumento);
                }
            }
            return nombre_llamado == "capturar_orden" ? Tipo::Texto : Tipo::Entero;
        }

        if (nombre_llamado == "leer_archivo" || nombre_llamado == "existe_archivo" ||
            nombre_llamado == "escribir_archivo" || nombre_llamado == "agregar_archivo") {
            const std::size_t esperados =
                (nombre_llamado == "escribir_archivo" || nombre_llamado == "agregar_archivo") ? 2
                                                                                              : 1;
            if (expresion.lista.size() != esperados) {
                error(diagnostico::codigos::ARIDAD_INCORRECTA, expresion.span)
                    .parametro("funcion", std::string(nombre_llamado))
                    .parametro("esperados", std::to_string(esperados))
                    .parametro("dados", std::to_string(expresion.lista.size()));
            }
            for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                if (argumento) {
                    tipo_de(*argumento);
                }
            }
            return nombre_llamado == "leer_archivo" ? Tipo::Texto : Tipo::Booleano;
        }

        // Punteros y memoria manual: solo dentro de `inseguro`.
        {
            const std::string_view nombre = nombre_llamado;
            const bool es_puntero = nombre == "reservar" || nombre == "liberar" ||
                                    nombre == "leer_en" || nombre == "escribir_en" ||
                                    nombre == "leer_flotante_en" ||
                                    nombre == "escribir_flotante_en" ||
                                    nombre == "desde_direccion" || nombre == "direccion_de";
            if (es_puntero) {
                if (profundidad_inseguro_ == 0) {
                    error(diagnostico::codigos::PUNTERO_FUERA_DE_INSEGURO, expresion.span)
                        .ayuda("envuélvalo en «inseguro { }» y documente por qué");
                }
                std::size_t esperados = 1;
                if (nombre == "escribir_en" || nombre == "escribir_flotante_en" ||
                    nombre == "leer_flotante_en" || nombre == "leer_en") {
                    esperados = 2;
                }
                if (nombre == "escribir_en" || nombre == "escribir_flotante_en") {
                    esperados = 3;
                }
                if (expresion.lista.size() != esperados) {
                    error(diagnostico::codigos::ARIDAD_INCORRECTA, expresion.span)
                        .parametro("funcion", std::string(nombre))
                        .parametro("esperados", std::to_string(esperados))
                        .parametro("dados", std::to_string(expresion.lista.size()));
                }
                for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                    if (argumento) {
                        tipo_de(*argumento);
                    }
                }
                if (nombre == "reservar" || nombre == "desde_direccion") {
                    return Tipo::Desconocido;
                }
                if (nombre == "direccion_de" || nombre == "leer_en") {
                    return Tipo::Entero;
                }
                if (nombre == "leer_flotante_en") {
                    return Tipo::Flotante;
                }
                return Tipo::Vacio;
            }
        }

        if (nombre_llamado == "leer_linea" || nombre_llamado == "leer_entero") {
            if (!expresion.lista.empty()) {
                error(diagnostico::codigos::ARIDAD_INCORRECTA, expresion.span)
                    .parametro("funcion", std::string(nombre_llamado))
                    .parametro("esperados", "0")
                    .parametro("dados", std::to_string(expresion.lista.size()));
            }
            return nombre_llamado == "leer_linea" ? Tipo::Texto : Tipo::Entero;
        }

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
                argumento != Tipo::Booleano && argumento != Tipo::Flotante &&
                argumento != Tipo::Desconocido) {
                error(diagnostico::codigos::TIPO_INCOMPATIBLE, expresion.lista.front()->span)
                    .parametro("esperado", "entero, texto, booleano o flotante")
                    .parametro("encontrado", nombre(argumento));
            }
            return Tipo::Vacio;
        }

        const auto encontrada = funciones_.find(nombre_llamado);
        if (encontrada == funciones_.end()) {
            // Una variable de tipo función se llama igual que una función.
            for (auto it = ambitos_.rbegin(); it != ambitos_.rend(); ++it) {
                const auto variable = it->funciones.find(std::string(nombre_llamado));
                if (variable == it->funciones.end()) {
                    continue;
                }
                for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                    if (argumento) {
                        tipo_de(*argumento);
                    }
                }
                const std::size_t flecha = variable->second.find("->");
                if (flecha != std::string::npos) {
                    return desde_nombre(std::string_view(variable->second).substr(flecha + 2));
                }
                return Tipo::Desconocido;
            }
            // Un parámetro de tipo función se llama igual que una función.
            if (const auto parametro = funciones_parametro.find(std::string(nombre_llamado));
                parametro != funciones_parametro.end()) {
                for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                    if (argumento) {
                        tipo_de(*argumento);
                    }
                }
                const std::size_t flecha = parametro->second.find("->");
                if (flecha != std::string::npos) {
                    return desde_nombre(std::string_view(parametro->second).substr(flecha + 2));
                }
                return Tipo::Desconocido;
            }
            error(diagnostico::codigos::FUNCION_NO_DECLARADA, expresion.span)
                .parametro("nombre", std::string(nombre_llamado))
                .ayuda("compruebe el nombre o declare la función");
            return Tipo::Desconocido;
        }

        return comprobar_argumentos(*encontrada->second, expresion,
                                    std::string(encontrada->second->nombre));
    }

    Tipo tipo_llamada_metodo(const Expresion& expresion) {
        // `sacar` de una celda de texto da texto, no un entero.
        if (expresion.izquierda && expresion.izquierda->tipo == TipoExpresion::Miembro &&
            expresion.izquierda->izquierda &&
            expresion.izquierda->izquierda->tipo == TipoExpresion::Identificador &&
            es_celda_texto(expresion.izquierda->izquierda->lexema)) {
            if (expresion.izquierda->anotacion == "sacar") {
                return Tipo::Texto;
            }
            if (expresion.izquierda->anotacion == "poner") {
                for (const sintactico::ExpresionPtr& argumento : expresion.lista) {
                    if (argumento) {
                        tipo_de(*argumento);
                    }
                }
                return Tipo::Vacio;
            }
        }
        const Expresion& acceso = *expresion.izquierda;

        if (acceso.izquierda) {
            Tipo objeto = tipo_de(*acceso.izquierda);
            // Un texto prestado (`&texto`) también es un texto para esto.
            if (objeto != Tipo::Texto && acceso.izquierda->tipo == TipoExpresion::Identificador &&
                prestamos_texto_.count(std::string(acceso.izquierda->lexema)) != 0) {
                objeto = Tipo::Texto;
            }
            if (objeto == Tipo::Texto) {
                // Los métodos de `texto` devuelven un tipo conocido: sin esto,
                // `valor.subtexto(0, 1)` se quedaba sin tipo y compararlo con un
                // texto daba «se esperaba entero».
                const std::string_view metodo = acceso.anotacion;
                if (metodo == "largo" || metodo == "buscar" || metodo == "a_entero") {
                    return Tipo::Entero;
                }
                if (metodo == "a_flotante") {
                    return Tipo::Flotante;
                }
                if (metodo == "contiene" || metodo == "empieza_con") {
                    return Tipo::Booleano;
                }
                if (metodo == "subtexto" || metodo == "mayusculas" || metodo == "minusculas" ||
                    metodo == "recortar" || metodo == "sin_ultimo" || metodo == "agregar_codigo") {
                    return Tipo::Texto;
                }
            }
        }

        // `Vector<texto>.obtener(i)` devuelve un texto, no un entero.
        if (acceso.anotacion == "obtener" && acceso.izquierda &&
            acceso.izquierda->tipo == TipoExpresion::Identificador &&
            es_vector_texto(acceso.izquierda->lexema)) {
            return Tipo::Texto;
        }
        // `texto.a_flotante()` devuelve un flotante.
        if (acceso.anotacion == "a_flotante" && acceso.izquierda &&
            acceso.izquierda->tipo == TipoExpresion::Identificador &&
            (es_texto(acceso.izquierda->lexema) || tipo_de(*acceso.izquierda) == Tipo::Texto)) {
            return Tipo::Flotante;
        }
        // `Vector<flotante>.obtener(i)` devuelve un flotante.
        if (acceso.anotacion == "obtener" && acceso.izquierda &&
            acceso.izquierda->tipo == TipoExpresion::Identificador &&
            es_vector_flotante(acceso.izquierda->lexema)) {
            return Tipo::Flotante;
        }
        // `Mapa<texto, flotante>.obtener(clave)` también.
        if (acceso.anotacion == "obtener" && acceso.izquierda &&
            acceso.izquierda->tipo == TipoExpresion::Identificador &&
            es_mapa_flotante(acceso.izquierda->lexema)) {
            return Tipo::Flotante;
        }
        if ((acceso.anotacion == "obtener" || acceso.anotacion == "largo") && acceso.izquierda &&
            acceso.izquierda->tipo == TipoExpresion::Identificador &&
            declarada(acceso.izquierda->lexema) && !es_vector_texto(acceso.izquierda->lexema) &&
            estructura_de(*acceso.izquierda).empty()) {
            return Tipo::Entero;
        }

        // `vector.agregar(texto)` toma el dueño del elemento.
        if (acceso.anotacion == "agregar" && acceso.izquierda &&
            acceso.izquierda->tipo == TipoExpresion::Identificador &&
            es_vector_texto(acceso.izquierda->lexema) && expresion.lista.size() == 1 &&
            expresion.lista.front() &&
            expresion.lista.front()->tipo == TipoExpresion::Identificador) {
            marcar_movida(expresion.lista.front()->lexema, expresion.span);
        }

        const sintactico::DefinicionTipo* definicion = tipo_por_nombre(
            acceso.izquierda ? estructura_de(*acceso.izquierda) : std::string_view{});
        if (definicion == nullptr) {
            return Tipo::Desconocido;
        }

        const Funcion* metodo = metodo_de(*definicion, acceso.anotacion);
        // Un método heredado se busca en la cadena de bases.
        for (const sintactico::DefinicionTipo* base = definicion; metodo == nullptr;
             base = base_de(*base)) {
            if (base == nullptr) {
                break;
            }
            metodo = metodo_de(*base, acceso.anotacion);
        }
        if (metodo == nullptr) {
            error(diagnostico::codigos::MIEMBRO_DESCONOCIDO, acceso.span)
                .parametro("tipo", std::string(definicion->nombre))
                .parametro("nombre", acceso.anotacion);
            return Tipo::Desconocido;
        }

        return comprobar_argumentos(*metodo, expresion,
                                    std::string(definicion->nombre) + "." +
                                        std::string(metodo->nombre));
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
                // `punto` y `figuras.punto` pueden ser la misma estructura: se compara
                // por definición, no por el texto con que se escribió.
                const bool mismo_tipo =
                    real == funcion.parametros[i].tipo ||
                    (tipo_por_nombre(real) != nullptr &&
                     tipo_por_nombre(real) == tipo_por_nombre(funcion.parametros[i].tipo));
                if (!mismo_tipo) {
                    error(diagnostico::codigos::ARGUMENTO_INCOMPATIBLE, llamada.lista[i]->span)
                        .parametro("parametro", std::string(funcion.parametros[i].nombre))
                        .parametro("funcion", nombre_visible)
                        .parametro("esperado", std::string(funcion.parametros[i].tipo))
                        .parametro("encontrado",
                                   real.empty() ? std::string("otro tipo") : std::string(real));
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
            const auto enumeracion = enumeraciones_.find(std::string(expresion.izquierda->lexema));
            if (enumeracion != enumeraciones_.end()) {
                for (const sintactico::Variante& variante : enumeracion->second->variantes) {
                    if (variante.nombre == expresion.anotacion) {
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
        // Un método heredado también es un miembro del tipo.
        for (const sintactico::DefinicionTipo* base = base_de(*definicion); base != nullptr;
             base = base_de(*base)) {
            if (const Funcion* heredado = metodo_de(*base, expresion.anotacion);
                heredado != nullptr) {
                return desde_nombre(heredado->tipo_retorno);
            }
            for (const sintactico::Campo& campo : base->campos) {
                if (campo.nombre == expresion.anotacion) {
                    return desde_nombre(campo.tipo);
                }
            }
        }

        error(diagnostico::codigos::MIEMBRO_DESCONOCIDO, expresion.span)
            .parametro("tipo", std::string(definicion->nombre))
            .parametro("nombre", expresion.anotacion);
        return Tipo::Desconocido;
    }

    /// Clase base de una definición, si la tiene.
    const sintactico::DefinicionTipo* base_de(const sintactico::DefinicionTipo& definicion) const {
        if (definicion.base.empty()) {
            return nullptr;
        }
        const sintactico::DefinicionTipo* base = tipo_por_nombre(definicion.base);
        return base == &definicion ? nullptr : base;
    }

    /// Campos de una clase, incluidos los heredados (base primero).
    std::vector<const sintactico::Campo*>
    campos_con_heredados(const sintactico::DefinicionTipo& definicion) const {
        std::vector<const sintactico::DefinicionTipo*> cadena;
        const sintactico::DefinicionTipo* actual = &definicion;
        for (int guarda = 0; guarda < 32 && actual != nullptr; ++guarda) {
            cadena.insert(cadena.begin(), actual);
            const auto base = tipos_.find(std::string(actual->base));
            actual = base != tipos_.end() ? base->second : nullptr;
        }

        std::vector<const sintactico::Campo*> campos;
        for (const sintactico::DefinicionTipo* tipo : cadena) {
            for (const sintactico::Campo& campo : tipo->campos) {
                campos.push_back(&campo);
            }
        }
        return campos;
    }

    Tipo tipo_construccion(const Expresion& expresion) {
        auto encontrado = tipos_.find(std::string(expresion.anotacion));
        if (encontrado == tipos_.end()) {
            // `par<entero>` se valida con la definición de `par`.
            const std::size_t abre = expresion.anotacion.find('<');
            if (abre != std::string_view::npos) {
                encontrado = tipos_.find(std::string(expresion.anotacion.substr(0, abre)));
            }
        }
        if (encontrado == tipos_.end()) {
            error(diagnostico::codigos::TIPO_DESCONOCIDO, expresion.span)
                .parametro("nombre", expresion.anotacion);
            return Tipo::Desconocido;
        }

        const sintactico::DefinicionTipo& definicion = *encontrado->second;

        const std::vector<const sintactico::Campo*> todos = campos_con_heredados(definicion);

        for (std::size_t i = 0; i < expresion.lista.size(); ++i) {
            const std::string_view nombre_campo =
                i < expresion.nombres_campo.size() ? expresion.nombres_campo[i] : "";

            const sintactico::Campo* campo = nullptr;
            for (const sintactico::Campo* declarado : todos) {
                if (declarado->nombre == nombre_campo) {
                    campo = declarado;
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

        for (const sintactico::Campo* declarado : todos) {
            bool presente = false;
            for (const std::string_view nombre_campo : expresion.nombres_campo) {
                if (nombre_campo == declarado->nombre) {
                    presente = true;
                    break;
                }
            }
            if (!presente) {
                error(diagnostico::codigos::CAMPO_FALTANTE, expresion.span)
                    .parametro("tipo", std::string(definicion.nombre))
                    .parametro("campo", std::string(declarado->nombre));
            }
        }

        // La v0.1 no distingue tipos de estructura en el verificador: el generador
        // los conoce por la construcción. Los campos se comprueban igualmente.
        return Tipo::Desconocido;
    }

    const sintactico::Programa& programa_;
    diagnostico::Emisor& emisor_;
    std::map<std::string_view, const Funcion*> funciones_;
    std::map<std::string, const sintactico::DefinicionTipo*> tipos_;
    std::map<std::string, const sintactico::Enumeracion*> enumeraciones_;
    std::map<std::string, const sintactico::Rasgo*> rasgos_;
    std::map<std::string, std::pair<const sintactico::Enumeracion*, std::size_t>> variantes_;
    std::map<std::string_view, std::map<std::string_view, const sintactico::Funcion*>> modulos_;
    std::set<std::string> parametros_tipo_;
    std::set<std::string> parametros_actuales_;
    /// Dentro de cuántos bloques `inseguro` estamos.
    int profundidad_inseguro_ = 0;
    /// Parámetros de tipo función en la función actual: nombre → tipo.
    std::map<std::string, std::string> funciones_parametro;
    /// Parámetros que son un préstamo de texto: `*parametro` es un texto.
    std::set<std::string> prestamos_texto_;
    /// Dueño prestado → variable que guarda el préstamo.
    std::map<std::string, std::string> prestamos_;
    const Funcion* funcion_actual_ = nullptr;
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
