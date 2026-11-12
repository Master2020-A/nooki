#ifndef NOOKI_ENTORNO_HPP
#define NOOKI_ENTORNO_HPP

#include <filesystem>
#include <string>
#include <string_view>

namespace nooki {

/// Convierte una ruta que llega como UTF-8 (de la línea de comandos) al tipo de
/// ruta del sistema. En Windows la conversión directa desde `std::string`
/// interpreta los bytes en la página de códigos local y falla con «ñ» o tildes;
/// pasar por `u8path` los decodifica como UTF-8, que es lo que emite la consola.
inline std::filesystem::path ruta_desde_utf8(std::string_view texto) {
    const std::u8string utf8(reinterpret_cast<const char8_t*>(texto.data()), texto.size());
    return std::filesystem::path(utf8);
}

/// Texto UTF-8 de una ruta, para mostrarla o pasarla a otro proceso.
inline std::string ruta_a_utf8(const std::filesystem::path& ruta) {
    const std::u8string texto = ruta.u8string();
    return std::string(texto.begin(), texto.end());
}

}  // namespace nooki

#endif  // NOOKI_ENTORNO_HPP
