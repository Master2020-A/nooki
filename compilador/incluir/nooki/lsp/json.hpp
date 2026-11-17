#ifndef NOOKI_LSP_JSON_HPP
#define NOOKI_LSP_JSON_HPP

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace nooki::lsp {

/// Valor JSON mínimo: lo justo para leer y escribir mensajes de LSP.
class Json {
public:
    enum class Clase { Nulo, Booleano, Numero, Cadena, Arreglo, Objeto };

    Json() = default;
    Json(bool valor) : clase_(Clase::Booleano), booleano_(valor) {}
    Json(double valor) : clase_(Clase::Numero), numero_(valor) {}
    Json(int valor) : clase_(Clase::Numero), numero_(valor) {}
    Json(std::string valor) : clase_(Clase::Cadena), cadena_(std::move(valor)) {}
    Json(const char* valor) : clase_(Clase::Cadena), cadena_(valor) {}

    static Json arreglo();
    static Json objeto();

    [[nodiscard]] Clase clase() const noexcept {
        return clase_;
    }
    [[nodiscard]] bool es_objeto() const noexcept {
        return clase_ == Clase::Objeto;
    }
    [[nodiscard]] bool es_arreglo() const noexcept {
        return clase_ == Clase::Arreglo;
    }
    [[nodiscard]] bool es_cadena() const noexcept {
        return clase_ == Clase::Cadena;
    }

    [[nodiscard]] const std::string& cadena() const noexcept {
        return cadena_;
    }
    [[nodiscard]] double numero() const noexcept {
        return numero_;
    }
    [[nodiscard]] int entero() const noexcept {
        return static_cast<int>(numero_);
    }
    [[nodiscard]] bool booleano() const noexcept {
        return booleano_;
    }

    /// Acceso a un campo de un objeto; devuelve un valor nulo si no está.
    [[nodiscard]] const Json& campo(std::string_view nombre) const;

    /// Añade un campo a un objeto (o un elemento a un arreglo).
    Json& poner(std::string nombre, Json valor);

    [[nodiscard]] const std::vector<Json>& elementos() const noexcept {
        return arreglo_;
    }

    /// Texto JSON del valor.
    [[nodiscard]] std::string escribir() const;

    /// Lee un documento completo. Devuelve `false` si no es JSON válido.
    static bool leer(std::string_view texto, Json& salida);

private:
    void escribir_en(std::string& salida) const;

    Clase clase_ = Clase::Nulo;
    bool booleano_ = false;
    double numero_ = 0;
    std::string cadena_;
    std::vector<Json> arreglo_;
    std::vector<std::pair<std::string, Json>> objeto_;
};

}  // namespace nooki::lsp

#endif  // NOOKI_LSP_JSON_HPP
