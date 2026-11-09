#include "nooki/diagnostico/gestor_fuentes.hpp"

#include <algorithm>
#include <utility>

#include "nooki/utf8.hpp"

namespace nooki::diagnostico {

GestorFuentes::GestorFuentes() {
    // El índice 0 es el marcador de "ningún archivo": los identificadores empiezan en 1.
    archivos_.push_back(Archivo{});
}

ArchivoId GestorFuentes::anadir(std::string ruta, std::string contenido) {
    Archivo archivo;
    archivo.ruta = std::move(ruta);
    archivo.contenido = std::move(contenido);

    archivo.inicios.push_back(0);
    for (std::size_t i = 0; i < archivo.contenido.size(); ++i) {
        if (archivo.contenido[i] == '\n' && i + 1 < archivo.contenido.size()) {
            archivo.inicios.push_back(static_cast<std::uint32_t>(i + 1));
        }
    }

    archivos_.push_back(std::move(archivo));
    return static_cast<ArchivoId>(archivos_.size() - 1);
}

bool GestorFuentes::existe(ArchivoId id) const noexcept { return buscar(id) != nullptr; }

std::string_view GestorFuentes::ruta(ArchivoId id) const {
    const Archivo* archivo = buscar(id);
    return archivo == nullptr ? std::string_view{} : std::string_view{archivo->ruta};
}

std::string_view GestorFuentes::contenido(ArchivoId id) const {
    const Archivo* archivo = buscar(id);
    return archivo == nullptr ? std::string_view{} : std::string_view{archivo->contenido};
}

LineaColumna GestorFuentes::ubicar(ArchivoId id, std::uint32_t desplazamiento) const {
    const Archivo* archivo = buscar(id);
    if (archivo == nullptr) {
        return LineaColumna{};
    }

    const std::uint32_t limite = static_cast<std::uint32_t>(archivo->contenido.size());
    const std::uint32_t destino = std::min(desplazamiento, limite);

    const auto posicion =
        std::upper_bound(archivo->inicios.begin(), archivo->inicios.end(), destino);
    const auto indice = static_cast<std::size_t>(posicion - archivo->inicios.begin()) - 1;

    const std::uint32_t inicio = archivo->inicios[indice];
    const std::string_view desde_inicio(archivo->contenido.data() + inicio, destino - inicio);

    LineaColumna resultado;
    resultado.linea = static_cast<std::uint32_t>(indice) + 1;
    resultado.columna = static_cast<std::uint32_t>(utf8::contar_puntos(desde_inicio)) + 1;
    return resultado;
}

std::string_view GestorFuentes::linea(ArchivoId id, std::uint32_t numero) const {
    const Archivo* archivo = buscar(id);
    if (archivo == nullptr || numero == 0 || numero > archivo->inicios.size()) {
        return {};
    }

    const std::size_t indice = static_cast<std::size_t>(numero) - 1;
    const std::size_t inicio = archivo->inicios[indice];
    const std::size_t fin = (numero < archivo->inicios.size())
                                ? static_cast<std::size_t>(archivo->inicios[numero]) - 1
                                : archivo->contenido.size();

    std::string_view texto(archivo->contenido.data() + inicio, fin - inicio);
    if (!texto.empty() && texto.back() == '\r') {
        texto.remove_suffix(1);
    }
    return texto;
}

std::uint32_t GestorFuentes::numero_de_lineas(ArchivoId id) const noexcept {
    const Archivo* archivo = buscar(id);
    return archivo == nullptr ? 0 : static_cast<std::uint32_t>(archivo->inicios.size());
}

std::uint32_t GestorFuentes::inicio_de_linea(ArchivoId id, std::uint32_t numero) const {
    const Archivo* archivo = buscar(id);
    if (archivo == nullptr || numero == 0 || numero > archivo->inicios.size()) {
        return 0;
    }
    return archivo->inicios[static_cast<std::size_t>(numero) - 1];
}

const GestorFuentes::Archivo* GestorFuentes::buscar(ArchivoId id) const noexcept {
    if (id == ARCHIVO_NINGUNO || id >= archivos_.size()) {
        return nullptr;
    }
    return &archivos_[id];
}

}  // namespace nooki::diagnostico
