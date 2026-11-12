// Rendimiento: línea base de las métricas del plan.
//   · velocidad del lexer en MB/s
//   · tiempo de compilación por cada 1 000 líneas (sin contar el compilador de C)
//
// Los umbrales son holgados a propósito: buscan regresiones de orden de magnitud,
// no medir la máquina. Para comparar consigo mismo, mira las cifras impresas.
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "nooki/diagnostico/emisor.hpp"
#include "nooki/diagnostico/gestor_fuentes.hpp"
#include "nooki/generador/generador_c.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/semantico/verificaciones.hpp"
#include "nooki/sintactico/parser.hpp"

namespace fs = std::filesystem;
using Reloj = std::chrono::steady_clock;

namespace {

double segundos_desde(Reloj::time_point inicio) {
    return std::chrono::duration<double>(Reloj::now() - inicio).count();
}

/// Un programa representativo: funciones, texto, colecciones, OOP y errores.
std::string programa_de(std::size_t funciones) {
    std::ostringstream codigo;
    codigo << "clase registro {\n"
              "    publico nombre: texto\n"
              "    publico valor: entero\n"
              "    publico funcion doble() -> entero {\n"
              "        retornar este.valor + este.valor\n"
              "    }\n"
              "}\n\n";
    for (std::size_t i = 0; i < funciones; ++i) {
        codigo << "funcion calcular_" << i << "(a: entero, b: entero) -> entero {\n"
                  "    variable total = a + b\n"
                  "    variable i = 0\n"
                  "    mientras (i < 10) {\n"
                  "        total = total + i\n"
                  "        i = i + 1\n"
                  "    }\n"
                  "    si (total > 100) {\n"
                  "        retornar total - 100\n"
                  "    }\n"
                  "    retornar total\n"
                  "}\n\n";
    }
    codigo << "funcion principal() -> entero {\n"
              "    variable r = nuevo registro { nombre: \"n\", valor: 1 }\n"
              "    variable numeros = Vector<entero>.nuevo()\n"
              "    numeros.agregar(r.doble())\n"
              "    variable suma = 0\n"
              "    para (n en numeros) {\n"
              "        suma = suma + n\n"
              "    }\n"
              "    imprimir(suma)\n"
              "    retornar 0\n"
              "}\n";
    return codigo.str();
}

struct Medida {
    std::string nombre;
    double segundos{};
    std::size_t bytes{};
    bool correcto{};
};

// Bajo sanitizadores la velocidad no es representativa (ASan frena el lexer
// unas cuarenta veces): se miden las cifras, pero no se exigen umbrales.
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
constexpr bool CON_SANITIZADOR = true;
#elif defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
constexpr bool CON_SANITIZADOR = true;
#else
constexpr bool CON_SANITIZADOR = false;
#endif
#else
constexpr bool CON_SANITIZADOR = false;
#endif

// --- Umbrales (holgados: buscan regresiones, no récords) --------------------
constexpr double LEXER_MINIMO_MB_S = 10.0;      // línea base medida: ~26
constexpr double COMPILAR_1000_LINEAS_MAX = 3.0;  // objetivo del plan: 2 s
constexpr double GENERAR_1000_LINEAS_MAX = 3.0;

}  // namespace

int main() {
    std::vector<Medida> medidas;
    bool todo_bien = true;
    auto informar = [&](const std::string& nombre, double segundos, std::size_t bytes,
                        bool correcto) {
        medidas.push_back({nombre, segundos, bytes, correcto});
        if (!correcto) {
            todo_bien = false;
        }
    };

    // --- Velocidad del lexer ------------------------------------------------
    {
        const std::string codigo = programa_de(400);  // ~40 000 líneas de texto
        const double megabytes = static_cast<double>(codigo.size()) / (1024.0 * 1024.0);

        double mejor = 1e9;
        for (int intento = 0; intento < 3; ++intento) {
            nooki::diagnostico::GestorFuentes fuentes;
            nooki::diagnostico::Emisor emisor;
            const auto archivo = fuentes.anadir("bench.oki", codigo);
            const auto inicio = Reloj::now();
            nooki::lexico::Lexer lexer(fuentes, archivo, emisor);
            const auto tokens = lexer.todos();
            const double segundos = segundos_desde(inicio);
            const double mb_s = megabytes / (segundos > 0 ? segundos : 1e-9);
            std::cout << "lexer: " << mb_s << " MB/s (" << tokens.size() << " tokens, "
                      << megabytes << " MB)\n";
            mejor = std::min(mejor, segundos);
            (void)tokens;
        }
        const double mb_s = megabytes / (mejor > 0 ? mejor : 1e-9);
        informar("lexer", mejor, codigo.size(),
                 CON_SANITIZADOR || mb_s >= LEXER_MINIMO_MB_S);
        std::cout << "  → " << mb_s << " MB/s (mínimo exigido " << LEXER_MINIMO_MB_S << ")\n";
    }

    // --- Compilación completa por cada 1 000 líneas -------------------------
    for (const std::size_t funciones : {std::size_t{45}, std::size_t{225}}) {
        const std::string codigo = programa_de(funciones);
        const std::size_t lineas = static_cast<std::size_t>(
            std::count(codigo.begin(), codigo.end(), '\n'));

        double mejor = 1e9;
        bool correcto = true;
        for (int intento = 0; intento < 3; ++intento) {
            nooki::diagnostico::GestorFuentes fuentes;
            nooki::diagnostico::Emisor emisor;
            const auto archivo = fuentes.anadir("bench.oki", codigo);
            const auto inicio = Reloj::now();

            nooki::lexico::Lexer lexer(fuentes, archivo, emisor);
            const auto tokens = lexer.todos();
            nooki::sintactico::Parser parser(fuentes, archivo, emisor, tokens);
            const nooki::sintactico::Programa programa = parser.analizar();
            nooki::semantico::verificar(programa, emisor);
            const auto generado = nooki::generador::generar_c(programa);

            const double segundos = segundos_desde(inicio);
            mejor = std::min(mejor, segundos);
            correcto = correcto && !emisor.hay_errores() && generado.soportado;
        }

        const double por_mil = mejor * 1000.0 / static_cast<double>(lineas);
        const std::string nombre = "compilar " + std::to_string(lineas) + " líneas";
        std::cout << nombre << ": " << mejor << " s (" << por_mil << " s por 1 000 líneas)\n";
        informar(nombre, mejor, codigo.size(),
                 correcto && (CON_SANITIZADOR || por_mil <= COMPILAR_1000_LINEAS_MAX));
    }

    // --- Resultado ----------------------------------------------------------
    std::cout << "\n";
    if (todo_bien) {
        std::cout << (CON_SANITIZADOR ? "rendimiento medido (umbrales no exigidos con "
                                        "sanitizadores)\n"
                                      : "rendimiento dentro de los umbrales\n");
        return 0;
    }
    for (const Medida& medida : medidas) {
        if (!medida.correcto) {
            std::cout << "FUERA DE UMBRAL: " << medida.nombre << " (" << medida.segundos
                      << " s)\n";
        }
    }
    return 1;
}
