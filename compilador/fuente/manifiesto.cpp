#include "nooki/manifiesto.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace nooki {
namespace {

constexpr const char* ARCHIVO = "nooki.toml";

std::string recortar(std::string texto) {
    const auto inicio = texto.find_first_not_of(" \t\r\n");
    if (inicio == std::string::npos) {
        return {};
    }
    const auto fin = texto.find_last_not_of(" \t\r\n");
    return texto.substr(inicio, fin - inicio + 1);
}

/// Quita las comillas de un valor si las lleva.
std::string sin_comillas(std::string valor) {
    valor = recortar(std::move(valor));
    if (valor.size() >= 2 && valor.front() == '"' && valor.back() == '"') {
        return valor.substr(1, valor.size() - 2);
    }
    return valor;
}

}  // namespace

std::vector<std::filesystem::path> Manifiesto::rutas_de_busqueda() const {
    std::vector<std::filesystem::path> rutas;
    rutas.push_back(directorio);
    for (const Dependencia& dependencia : dependencias) {
        // El módulo puede estar en la raíz del otro proyecto o en su `biblioteca`.
        rutas.push_back(dependencia.ruta);
        rutas.push_back(dependencia.ruta / "biblioteca");
    }
    return rutas;
}

bool leer_manifiesto(const std::filesystem::path& directorio, Manifiesto& manifiesto,
                     std::string& error) {
    const std::filesystem::path ruta = directorio / ARCHIVO;
    std::ifstream flujo(ruta, std::ios::binary);
    if (!flujo) {
        error = "no se encontró «" + ruta.string() + "»";
        return false;
    }

    manifiesto = Manifiesto{};
    manifiesto.directorio = directorio;

    std::string seccion;
    std::string linea;
    int numero = 0;
    while (std::getline(flujo, linea)) {
        ++numero;
        if (!linea.empty() && linea.back() == '\r') {
            linea.pop_back();
        }
        // Comentarios y líneas vacías.
        const auto almohadilla = linea.find('#');
        if (almohadilla != std::string::npos) {
            linea.erase(almohadilla);
        }
        linea = recortar(linea);
        if (linea.empty()) {
            continue;
        }

        if (linea.front() == '[' && linea.back() == ']') {
            seccion = recortar(linea.substr(1, linea.size() - 2));
            continue;
        }

        const auto igual = linea.find('=');
        if (igual == std::string::npos) {
            error = ARCHIVO + std::string(":") + std::to_string(numero) + ": falta «=» en «" +
                    linea + "»";
            return false;
        }

        const std::string clave = recortar(linea.substr(0, igual));
        const std::string valor = recortar(linea.substr(igual + 1));

        if (seccion == "proyecto") {
            if (clave == "nombre") {
                manifiesto.nombre = sin_comillas(valor);
            } else if (clave == "version") {
                manifiesto.version = sin_comillas(valor);
            }
            continue;
        }

        if (seccion == "dependencias") {
            // Formato admitido: `nombre = { ruta = "../otro" }`.
            Dependencia dependencia;
            dependencia.nombre = clave;
            const auto ruta_clave = valor.find("ruta");
            if (ruta_clave == std::string::npos) {
                error = ARCHIVO + std::string(":") + std::to_string(numero) + ": la dependencia «" +
                        clave + "» necesita «ruta»";
                return false;
            }
            const auto comillas = valor.find('"', ruta_clave);
            if (comillas == std::string::npos) {
                error = ARCHIVO + std::string(":") + std::to_string(numero) +
                        ": falta la ruta entre comillas en «" + clave + "»";
                return false;
            }
            const auto fin = valor.find('"', comillas + 1);
            if (fin == std::string::npos) {
                error = ARCHIVO + std::string(":") + std::to_string(numero) +
                        ": falta cerrar las comillas en «" + clave + "»";
                return false;
            }
            dependencia.ruta = directorio / valor.substr(comillas + 1, fin - comillas - 1);
            dependencia.ruta = dependencia.ruta.lexically_normal();
            manifiesto.dependencias.push_back(std::move(dependencia));
        }
    }

    if (manifiesto.nombre.empty()) {
        manifiesto.nombre = directorio.filename().string();
    }
    return true;
}

bool escribir_manifiesto(const std::filesystem::path& directorio, const std::string& nombre,
                         std::string& error) {
    const std::filesystem::path ruta = directorio / ARCHIVO;
    std::ofstream flujo(ruta, std::ios::binary | std::ios::trunc);
    if (!flujo) {
        error = "no se pudo crear «" + ruta.string() + "»";
        return false;
    }

    flujo << "# Manifiesto del proyecto. Lo lee «nooki construir».\n"
             "[proyecto]\n"
             "nombre = \""
          << nombre
          << "\"\n"
             "version = \"0.1.0\"\n"
             "\n"
             "[dependencias]\n"
             "# nombre = { ruta = \"../otro-proyecto\" }\n";
    return true;
}

}  // namespace nooki
