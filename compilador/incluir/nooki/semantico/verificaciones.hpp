#pragma once

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/sintactico/ast.hpp"

namespace nooki::semantico {

/// Verificación semántica de la v0.1.
///
/// Comprueba, en este orden:
///   - que toda llamada apunte a una función declarada o al preludio (NOK3002);
///   - que los identificadores usados estén declarados (NOK3001);
///   - que las llamadas tengan la aridad y los tipos correctos (NOK4002, NOK4003);
///   - que los operadores reciban los tipos que esperan (NOK4001);
///   - que las condiciones sean booleanas (NOK4005);
///   - que el tipo devuelto coincida con el declarado (NOK4004).
///
/// Devuelve el número de errores encontrados.
int verificar(const sintactico::Programa& programa, diagnostico::Emisor& emisor);

}  // namespace nooki::semantico
