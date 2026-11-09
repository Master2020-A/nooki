#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/diagnostico.hpp"
#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/lexico/token.hpp"

namespace nooki::lexico {

/// Analizador léxico de Nooki.
///
/// Produce los tokens de un archivo y reporta los errores léxicos al `Emisor`;
/// no lanza excepciones ni aborta: ante una secuencia inválida avanza un carácter
/// y sigue, para que el usuario vea todos los errores de una vez.
class Lexer {
public:
    Lexer(const diagnostico::GestorFuentes& fuentes, diagnostico::ArchivoId archivo,
          diagnostico::Emisor& emisor);

    /// Devuelve el siguiente token. Al llegar al final devuelve siempre FinDeArchivo.
    Token siguiente();

    /// Consume el archivo entero (incluye el token de FinDeArchivo).
    std::vector<Token> todos();

private:
    [[nodiscard]] bool fin() const noexcept { return pos_ >= contenido_.size(); }
    [[nodiscard]] char actual() const noexcept;
    [[nodiscard]] char mirar(std::size_t distancia) const noexcept;
    void avanzar(std::size_t cantidad = 1) noexcept;

    [[nodiscard]] Token crear(TipoToken tipo, std::size_t inicio) const;
    diagnostico::Diagnostico& error(std::string_view codigo, std::size_t inicio, std::size_t fin);

    Token leer_identificador();
    Token leer_numero();
    Token leer_texto();
    Token leer_caracter_o_vida();
    Token leer_operador();

    const diagnostico::GestorFuentes& fuentes_;
    diagnostico::ArchivoId archivo_;
    diagnostico::Emisor& emisor_;
    std::string_view contenido_;
    std::size_t pos_ = 0;
};

}  // namespace nooki::lexico
