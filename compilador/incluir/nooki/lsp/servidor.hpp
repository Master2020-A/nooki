#ifndef NOOKI_LSP_SERVIDOR_HPP
#define NOOKI_LSP_SERVIDOR_HPP

#include <iosfwd>

namespace nooki::lsp {

/// Atiende el protocolo LSP por un flujo de entrada y otro de salida.
/// Devuelve 0 cuando el cliente pide terminar.
int servir(std::istream& entrada, std::ostream& salida);

}  // namespace nooki::lsp

#endif  // NOOKI_LSP_SERVIDOR_HPP
