#include "nooki/utf8.hpp"

namespace nooki::utf8 {
namespace {

constexpr char32_t REEMPLAZO = 0xFFFD;

struct Decodificado {
    bool valido = false;
    Punto punto{};
};

Decodificado decodificar_impl(std::string_view texto, std::size_t indice) noexcept {
    if (indice >= texto.size()) {
        return {false, Punto{REEMPLAZO, 1}};
    }

    const auto primero = static_cast<unsigned char>(texto[indice]);
    if (primero < 0x80) {
        return {true, Punto{static_cast<char32_t>(primero), 1}};
    }

    int extra = 0;
    char32_t valor = 0;
    char32_t minimo = 0;

    if ((primero & 0xE0) == 0xC0) {
        extra = 1;
        valor = static_cast<char32_t>(primero & 0x1F);
        minimo = 0x80;
    } else if ((primero & 0xF0) == 0xE0) {
        extra = 2;
        valor = static_cast<char32_t>(primero & 0x0F);
        minimo = 0x800;
    } else if ((primero & 0xF8) == 0xF0) {
        extra = 3;
        valor = static_cast<char32_t>(primero & 0x07);
        minimo = 0x10000;
    } else {
        return {false, Punto{REEMPLAZO, 1}};
    }

    if (indice + static_cast<std::size_t>(extra) >= texto.size()) {
        return {false, Punto{REEMPLAZO, 1}};
    }

    for (int i = 1; i <= extra; ++i) {
        const auto siguiente =
            static_cast<unsigned char>(texto[indice + static_cast<std::size_t>(i)]);
        if ((siguiente & 0xC0) != 0x80) {
            return {false, Punto{REEMPLAZO, 1}};
        }
        valor = static_cast<char32_t>((valor << 6) | (siguiente & 0x3F));
    }

    // Rechaza sobreprolongaciones, subrogados y valores fuera del rango Unicode.
    if (valor < minimo || valor > 0x10FFFF || (valor >= 0xD800 && valor <= 0xDFFF)) {
        return {false, Punto{REEMPLAZO, 1}};
    }

    return {true, Punto{valor, static_cast<std::size_t>(extra) + 1}};
}

}  // namespace

Punto decodificar(std::string_view texto, std::size_t indice) noexcept {
    return decodificar_impl(texto, indice).punto;
}

bool valido_en(std::string_view texto, std::size_t indice) noexcept {
    return decodificar_impl(texto, indice).valido;
}

std::size_t contar_puntos(std::string_view texto) noexcept {
    std::size_t total = 0;
    std::size_t i = 0;
    while (i < texto.size()) {
        i += decodificar_impl(texto, i).punto.bytes;
        ++total;
    }
    return total;
}

}  // namespace nooki::utf8
