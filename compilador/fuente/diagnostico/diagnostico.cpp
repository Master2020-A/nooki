#include "nooki/diagnostico/diagnostico.hpp"

#include <utility>

#include "nooki/diagnostico/catalogo.hpp"

namespace nooki::diagnostico {

Diagnostico::Diagnostico(std::string_view codigo, Gravedad gravedad, Span span)
    : codigo_(codigo), gravedad_(gravedad), span_(span) {}

Diagnostico& Diagnostico::parametro(std::string nombre, std::string valor) {
    parametros_.emplace_back(std::move(nombre), std::move(valor));
    return *this;
}

Diagnostico& Diagnostico::etiqueta(Span span, std::string mensaje) {
    etiquetas_.push_back(Etiqueta{span, std::move(mensaje)});
    return *this;
}

Diagnostico& Diagnostico::nota(std::string texto) {
    notas_.push_back(std::move(texto));
    return *this;
}

Diagnostico& Diagnostico::ayuda(std::string texto) {
    ayudas_.push_back(std::move(texto));
    return *this;
}

const char* nombre_gravedad(Gravedad gravedad, Idioma idioma) noexcept {
    switch (gravedad) {
        case Gravedad::Error:
            return texto_fijo("error", idioma).data();
        case Gravedad::Advertencia:
            return texto_fijo("advertencia", idioma).data();
        case Gravedad::Nota:
            return texto_fijo("nota", idioma).data();
        case Gravedad::Ayuda:
            return texto_fijo("ayuda", idioma).data();
    }
    return texto_fijo("error", idioma).data();
}

}  // namespace nooki::diagnostico
