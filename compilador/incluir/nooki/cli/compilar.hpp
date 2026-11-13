#pragma once

#include <iosfwd>
#include <span>
#include <string_view>
#include <vector>
#include <filesystem>

namespace nooki::cli {

/// Implementa `nooki compilar`.
int comando_nuevo(std::span<const std::string_view> argumentos, std::ostream& salida,
                  std::ostream& error);
int comando_probar(std::span<const std::string_view> argumentos, std::ostream& salida,
                   std::ostream& error);
int comando_limpiar(std::span<const std::string_view> argumentos, std::ostream& salida,
                    std::ostream& error);

int comando_construir(std::span<const std::string_view> argumentos, std::ostream& salida,
                      std::ostream& error);

int comando_lsp(std::span<const std::string_view> argumentos, std::ostream& salida,
                std::ostream& error);

int compilar_con_rutas(std::span<const std::string_view> argumentos,
                       const std::vector<std::filesystem::path>& rutas, std::ostream& salida,
                       std::ostream& error);

int comando_compilar(std::span<const std::string_view> argumentos, std::ostream& salida,
                     std::ostream& error);

/// Implementa `nooki ejecutar`: compila y ejecuta el resultado.
int comando_ejecutar(std::span<const std::string_view> argumentos, std::ostream& salida,
                     std::ostream& error);

/// `nooki verificar <archivo.oki>`: léxico, parser y verificación, sin generar C.
int comando_verificar(std::span<const std::string_view> argumentos, std::ostream& salida,
                      std::ostream& error);

/// `nooki formatear <archivo.oki>...`: normaliza la sangría y los espacios.
int comando_formatear(std::span<const std::string_view> argumentos, std::ostream& salida,
                      std::ostream& error);

}  // namespace nooki::cli
