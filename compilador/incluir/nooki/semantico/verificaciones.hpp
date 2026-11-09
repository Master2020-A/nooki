#pragma once

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/sintactico/ast.hpp"

namespace nooki::semantico {

/// Comprueba que toda llamada apunte a una función declarada o al preludio.
///
/// Es la primera verificación semántica del compilador: sin ella, un error de
/// tecleo acababa en un mensaje del compilador de C en inglés.
/// Devuelve el número de errores encontrados.
int verificar_llamadas(const sintactico::Programa& programa, diagnostico::Emisor& emisor);

}  // namespace nooki::semantico
