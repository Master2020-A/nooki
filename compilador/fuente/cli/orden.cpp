#include "nooki/cli/orden.hpp"

#include "nooki/cli/ayuda.hpp"
#include "nooki/cli/comandos.hpp"

#include <algorithm>
#include <cstddef>
#include <ostream>
#include <string_view>
#include <vector>

namespace nooki::cli {
namespace {

/// Distancia de Levenshtein: número mínimo de ediciones para pasar de `origen` a `destino`.
/// Se usa para sugerir el comando más parecido cuando el usuario se equivoca.
std::size_t distancia_edicion(std::string_view origen, std::string_view destino) {
    std::vector<std::size_t> anterior(destino.size() + 1);
    std::vector<std::size_t> actual(destino.size() + 1);

    for (std::size_t j = 0; j <= destino.size(); ++j) {
        anterior[j] = j;
    }
    for (std::size_t i = 1; i <= origen.size(); ++i) {
        actual[0] = i;
        for (std::size_t j = 1; j <= destino.size(); ++j) {
            const std::size_t coste = (origen[i - 1] == destino[j - 1]) ? 0U : 1U;
            actual[j] = std::min({anterior[j] + 1U, actual[j - 1] + 1U, anterior[j - 1] + coste});
        }
        std::swap(anterior, actual);
    }
    return anterior[destino.size()];
}

const Comando* buscar(std::string_view nombre) {
    for (const Comando& comando : COMANDOS) {
        if (comando.nombre == nombre) {
            return &comando;
        }
    }
    return nullptr;
}

/// Devuelve el comando más parecido, o una vista vacía si no hay ninguno suficientemente cercano.
std::string_view sugerir(std::string_view nombre) {
    constexpr std::size_t DISTANCIA_MAXIMA = 3;
    std::string_view mejor;
    std::size_t mejor_distancia = DISTANCIA_MAXIMA;
    for (const Comando& comando : COMANDOS) {
        const std::size_t distancia = distancia_edicion(nombre, comando.nombre);
        if (distancia < mejor_distancia) {
            mejor_distancia = distancia;
            mejor = comando.nombre;
        }
    }
    return mejor;
}

int informar_comando_desconocido(std::string_view nombre, std::ostream& error) {
    error << "nooki: el comando «" << nombre << "» no existe.\n";
    const std::string_view parecido = sugerir(nombre);
    if (!parecido.empty()) {
        error << "¿Quisiste decir «" << parecido << "»?\n";
    }
    error << "Consulta todos los comandos con «nooki --ayuda».\n";
    return USO_INCORRECTO;
}

}  // namespace

int ejecutar(std::span<const std::string_view> argumentos, std::ostream& salida,
             std::ostream& error) {
    if (argumentos.empty()) {
        imprimir_ayuda(salida);
        return EXITO;
    }

    const std::string_view primero = argumentos.front();

    if (primero == "--version" || primero == "-v") {
        imprimir_version(salida);
        return EXITO;
    }
    if (primero == "--ayuda" || primero == "--help" || primero == "-h") {
        imprimir_ayuda(salida);
        return EXITO;
    }
    if (!primero.empty() && primero.front() == '-') {
        error << "nooki: la opción «" << primero << "» no existe.\n";
        error << "Consulta las opciones con «nooki --ayuda».\n";
        return USO_INCORRECTO;
    }

    const Comando* comando = buscar(primero);
    if (comando == nullptr) {
        return informar_comando_desconocido(primero, error);
    }

    if (!comando->disponible()) {
        error << "nooki: «" << comando->nombre << "» aún no está disponible en la v0.1.\n";
        error << "Llega en el Sprint " << comando->sprint << " del plan de implementación.\n";
        return NO_DISPONIBLE;
    }

    if (comando->nombre == "ayuda") {
        imprimir_ayuda(salida);
        return EXITO;
    }
    if (comando->nombre == "version") {
        imprimir_version(salida);
        return EXITO;
    }

    // Un comando marcado como disponible sin implementación es un fallo del propio compilador:
    // se avisa en vez de devolver éxito silenciosamente.
    error << "nooki: el comando «" << comando->nombre
          << "» figura como disponible pero no tiene implementación.\n";
    error << "Es un error interno: abre una incidencia en el repositorio.\n";
    return ERROR_INTERNO;
}

}  // namespace nooki::cli
