#include "nooki/lexico/token.hpp"

namespace nooki::lexico {

const char* nombre_tipo(TipoToken tipo) noexcept {
    switch (tipo) {
    case TipoToken::FinDeArchivo:
        return "fin de archivo";
    case TipoToken::NuevaLinea:
        return "nueva línea";
    case TipoToken::Identificador:
        return "identificador";
    case TipoToken::PalabraClave:
        return "palabra clave";
    case TipoToken::Entero:
        return "entero";
    case TipoToken::Flotante:
        return "flotante";
    case TipoToken::Texto:
        return "texto";
    case TipoToken::Caracter:
        return "caracter";
    case TipoToken::Vida:
        return "tiempo de vida";
    case TipoToken::Operador:
        return "operador";
    case TipoToken::Documentacion:
        return "documentación";
    }
    return "desconocido";
}

}  // namespace nooki::lexico
