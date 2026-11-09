#include "nooki/entorno.hpp"

#include <cstdlib>

namespace nooki {

std::string leer_entorno(const char* nombre) {
#ifdef _WIN32
    char* valor = nullptr;
    std::size_t longitud = 0;
    if (_dupenv_s(&valor, &longitud, nombre) != 0 || valor == nullptr) {
        return {};
    }
    std::string resultado(valor);
    std::free(valor);
    return resultado;
#else
    const char* valor = std::getenv(nombre);
    return valor == nullptr ? std::string{} : std::string{valor};
#endif
}

}  // namespace nooki
