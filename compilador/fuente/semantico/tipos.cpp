#include "nooki/semantico/tipos.hpp"

namespace nooki::semantico {

const char* nombre(Tipo tipo) noexcept {
    switch (tipo) {
        case Tipo::Vacio:
            return "vacio";
        case Tipo::Entero:
            return "entero";
        case Tipo::Texto:
            return "texto";
        case Tipo::Booleano:
            return "booleano";
        case Tipo::Desconocido:
            return "desconocido";
    }
    return "desconocido";
}

Tipo desde_nombre(std::string_view escrito) noexcept {
    if (escrito == "texto") {
        return Tipo::Texto;
    }
    if (escrito == "booleano") {
        return Tipo::Booleano;
    }
    if (escrito == "vacio") {
        return Tipo::Vacio;
    }

    // Todos los enteros y naturales comparten representación en esta etapa.
    if (escrito == "entero" || escrito == "entero8" || escrito == "entero16" ||
        escrito == "entero32" || escrito == "entero64" || escrito == "entero128" ||
        escrito == "natural" || escrito == "natural8" || escrito == "natural16" ||
        escrito == "natural32" || escrito == "natural64" || escrito == "natural128" ||
        escrito == "byte" || escrito == "tamano" || escrito == "tamano_signado") {
        return Tipo::Entero;
    }

    return Tipo::Desconocido;
}

}  // namespace nooki::semantico
