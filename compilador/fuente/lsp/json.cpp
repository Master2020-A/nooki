#include "nooki/lsp/json.hpp"

#include <cstdio>
#include <cstdlib>

namespace nooki::lsp {
namespace {

void escribir_cadena(std::string& salida, std::string_view texto) {
    salida += '"';
    for (const char caracter : texto) {
        switch (caracter) {
        case '"':
            salida += "\\\"";
            break;
        case '\\':
            salida += "\\\\";
            break;
        case '\n':
            salida += "\\n";
            break;
        case '\r':
            salida += "\\r";
            break;
        case '\t':
            salida += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(caracter) < 0x20) {
                char escape[8];
                std::snprintf(escape, sizeof(escape), "\\u%04x", static_cast<unsigned>(caracter));
                salida += escape;
            } else {
                salida += caracter;
            }
        }
    }
    salida += '"';
}

/// Analizador recursivo descendente sobre una vista del texto.
class Lector {
public:
    explicit Lector(std::string_view texto) : texto_(texto) {}

    bool leer(Json& salida) {
        saltar();
        if (!valor(salida)) {
            return false;
        }
        saltar();
        return pos_ == texto_.size();
    }

private:
    void saltar() {
        while (pos_ < texto_.size() && (texto_[pos_] == ' ' || texto_[pos_] == '\t' ||
                                        texto_[pos_] == '\n' || texto_[pos_] == '\r')) {
            ++pos_;
        }
    }

    bool valor(Json& salida) {
        if (pos_ >= texto_.size()) {
            return false;
        }
        switch (texto_[pos_]) {
        case '{':
            return objeto(salida);
        case '[':
            return arreglo(salida);
        case '"': {
            std::string texto;
            if (!cadena(texto)) {
                return false;
            }
            salida = Json(std::move(texto));
            return true;
        }
        case 't':
            if (texto_.compare(pos_, 4, "true") == 0) {
                pos_ += 4;
                salida = Json(true);
                return true;
            }
            return false;
        case 'f':
            if (texto_.compare(pos_, 5, "false") == 0) {
                pos_ += 5;
                salida = Json(false);
                return true;
            }
            return false;
        case 'n':
            if (texto_.compare(pos_, 4, "null") == 0) {
                pos_ += 4;
                salida = Json();
                return true;
            }
            return false;
        default:
            return numero(salida);
        }
    }

    bool objeto(Json& salida) {
        salida = Json::objeto();
        ++pos_;  // '{'
        saltar();
        if (pos_ < texto_.size() && texto_[pos_] == '}') {
            ++pos_;
            return true;
        }
        for (;;) {
            saltar();
            std::string clave;
            if (!cadena(clave)) {
                return false;
            }
            saltar();
            if (pos_ >= texto_.size() || texto_[pos_] != ':') {
                return false;
            }
            ++pos_;
            saltar();
            Json contenido;
            if (!valor(contenido)) {
                return false;
            }
            salida.poner(std::move(clave), std::move(contenido));
            saltar();
            if (pos_ >= texto_.size()) {
                return false;
            }
            if (texto_[pos_] == ',') {
                ++pos_;
                continue;
            }
            if (texto_[pos_] == '}') {
                ++pos_;
                return true;
            }
            return false;
        }
    }

    bool arreglo(Json& salida) {
        salida = Json::arreglo();
        ++pos_;  // '['
        saltar();
        if (pos_ < texto_.size() && texto_[pos_] == ']') {
            ++pos_;
            return true;
        }
        for (;;) {
            saltar();
            Json elemento;
            if (!valor(elemento)) {
                return false;
            }
            salida.poner({}, std::move(elemento));
            saltar();
            if (pos_ >= texto_.size()) {
                return false;
            }
            if (texto_[pos_] == ',') {
                ++pos_;
                continue;
            }
            if (texto_[pos_] == ']') {
                ++pos_;
                return true;
            }
            return false;
        }
    }

    bool cadena(std::string& salida) {
        if (pos_ >= texto_.size() || texto_[pos_] != '"') {
            return false;
        }
        ++pos_;
        while (pos_ < texto_.size()) {
            const char caracter = texto_[pos_++];
            if (caracter == '"') {
                return true;
            }
            if (caracter != '\\') {
                salida += caracter;
                continue;
            }
            if (pos_ >= texto_.size()) {
                return false;
            }
            const char escape = texto_[pos_++];
            switch (escape) {
            case '"':
                salida += '"';
                break;
            case '\\':
                salida += '\\';
                break;
            case '/':
                salida += '/';
                break;
            case 'b':
                salida += '\b';
                break;
            case 'f':
                salida += '\f';
                break;
            case 'n':
                salida += '\n';
                break;
            case 'r':
                salida += '\r';
                break;
            case 't':
                salida += '\t';
                break;
            case 'u': {
                // Se conserva el byte: los mensajes de LSP que usamos son ASCII
                // salvo las rutas, que llegan ya como UTF-8 en otros campos.
                if (pos_ + 4 > texto_.size()) {
                    return false;
                }
                unsigned codigo = 0;
                for (int i = 0; i < 4; ++i) {
                    const char digito = texto_[pos_ + static_cast<std::size_t>(i)];
                    codigo <<= 4;
                    if (digito >= '0' && digito <= '9') {
                        codigo += static_cast<unsigned>(digito - '0');
                    } else if (digito >= 'a' && digito <= 'f') {
                        codigo += static_cast<unsigned>(digito - 'a' + 10);
                    } else if (digito >= 'A' && digito <= 'F') {
                        codigo += static_cast<unsigned>(digito - 'A' + 10);
                    } else {
                        return false;
                    }
                }
                pos_ += 4;
                if (codigo < 0x80) {
                    salida += static_cast<char>(codigo);
                } else if (codigo < 0x800) {
                    salida += static_cast<char>(0xC0 | (codigo >> 6));
                    salida += static_cast<char>(0x80 | (codigo & 0x3F));
                } else {
                    salida += static_cast<char>(0xE0 | (codigo >> 12));
                    salida += static_cast<char>(0x80 | ((codigo >> 6) & 0x3F));
                    salida += static_cast<char>(0x80 | (codigo & 0x3F));
                }
                break;
            }
            default:
                return false;
            }
        }
        return false;
    }

    bool numero(Json& salida) {
        const std::size_t inicio = pos_;
        if (pos_ < texto_.size() && (texto_[pos_] == '-' || texto_[pos_] == '+')) {
            ++pos_;
        }
        while (pos_ < texto_.size() &&
               ((texto_[pos_] >= '0' && texto_[pos_] <= '9') || texto_[pos_] == '.' ||
                texto_[pos_] == 'e' || texto_[pos_] == 'E' || texto_[pos_] == '-' ||
                texto_[pos_] == '+')) {
            ++pos_;
        }
        if (pos_ == inicio) {
            return false;
        }
        salida =
            Json(std::strtod(std::string(texto_.substr(inicio, pos_ - inicio)).c_str(), nullptr));
        return true;
    }

    std::string_view texto_;
    std::size_t pos_ = 0;
};

}  // namespace

Json Json::arreglo() {
    Json valor;
    valor.clase_ = Clase::Arreglo;
    return valor;
}

Json Json::objeto() {
    Json valor;
    valor.clase_ = Clase::Objeto;
    return valor;
}

const Json& Json::campo(std::string_view nombre) const {
    static const Json nulo;
    for (const auto& [clave, valor] : objeto_) {
        if (clave == nombre) {
            return valor;
        }
    }
    return nulo;
}

Json& Json::poner(std::string nombre, Json valor) {
    if (clase_ == Clase::Arreglo) {
        arreglo_.push_back(std::move(valor));
        return arreglo_.back();
    }
    clase_ = Clase::Objeto;
    for (auto& [clave, existente] : objeto_) {
        if (clave == nombre) {
            existente = std::move(valor);
            return existente;
        }
    }
    objeto_.emplace_back(std::move(nombre), std::move(valor));
    return objeto_.back().second;
}

void Json::escribir_en(std::string& salida) const {
    switch (clase_) {
    case Clase::Nulo:
        salida += "null";
        break;
    case Clase::Booleano:
        salida += booleano_ ? "true" : "false";
        break;
    case Clase::Numero: {
        char texto[32];
        if (numero_ == static_cast<double>(static_cast<long long>(numero_))) {
            std::snprintf(texto, sizeof(texto), "%lld", static_cast<long long>(numero_));
        } else {
            std::snprintf(texto, sizeof(texto), "%g", numero_);
        }
        salida += texto;
        break;
    }
    case Clase::Cadena:
        escribir_cadena(salida, cadena_);
        break;
    case Clase::Arreglo: {
        salida += '[';
        bool primero = true;
        for (const Json& elemento : arreglo_) {
            if (!primero) {
                salida += ',';
            }
            primero = false;
            elemento.escribir_en(salida);
        }
        salida += ']';
        break;
    }
    case Clase::Objeto: {
        salida += '{';
        bool primero = true;
        for (const auto& [clave, valor] : objeto_) {
            if (!primero) {
                salida += ',';
            }
            primero = false;
            escribir_cadena(salida, clave);
            salida += ':';
            valor.escribir_en(salida);
        }
        salida += '}';
        break;
    }
    }
}

std::string Json::escribir() const {
    std::string salida;
    escribir_en(salida);
    return salida;
}

bool Json::leer(std::string_view texto, Json& salida) {
    Lector lector(texto);
    return lector.leer(salida);
}

}  // namespace nooki::lsp
