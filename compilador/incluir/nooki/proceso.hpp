#pragma once

#include <string>

namespace nooki {

/// Ejecuta un comando del sistema y devuelve su código de salida.
///
/// En Windows, `cmd /c "..."` recorta la primera y la última comilla cuando el
/// comando empieza por comilla, lo que rompe cualquier ruta entrecomillada
/// («El nombre de archivo, el nombre de directorio o la sintaxis de la etiqueta
/// del volumen no son correctos»). Esta función añade un par de comillas externo
/// para que `cmd` recorte las correctas.
int ejecutar_comando(const std::string& comando);

/// Entrecomilla una ruta solo si contiene espacios, para no introducir comillas
/// innecesarias al principio del comando.
[[nodiscard]] std::string entrecomillar_si_hace_falta(const std::string& valor);

/// Convierte los finales de línea CRLF en LF.
///
/// En Windows, redirigir la salida de un programa a un archivo traduce cada `\n`
/// en `\r\n`; comparar esa salida con un archivo escrito en LF fallaría siempre.
[[nodiscard]] std::string normalizar_saltos(std::string texto);

}  // namespace nooki
