#include "nooki/conductor/compilar.hpp"
#include "nooki/ia/runtime_ia.hpp"
#include "nooki/rutas.hpp"
#include "nooki/ui/runtime_ui.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "nooki/diagnostico/codigos.hpp"
#include "nooki/entorno.hpp"
#include "nooki/generador/generador_c.hpp"
#include "nooki/lexico/lexer.hpp"
#include "nooki/proceso.hpp"
#include "nooki/semantico/verificaciones.hpp"
#include "nooki/sintactico/parser.hpp"

namespace nooki::conductor {
namespace {

#ifdef _WIN32
constexpr const char* NULO = "nul";
constexpr const char* EXTENSION_BINARIO = ".exe";
#else
constexpr const char* NULO = "/dev/null";
constexpr const char* EXTENSION_BINARIO = "";
#endif

std::string leer_archivo(const std::filesystem::path& ruta);

/// Biblioteca estándar mínima, escrita en Nooki y empaquetada con el compilador.
/// Archivo que se está compilando: un módulo no puede ser él mismo.
std::filesystem::path archivo_de_entrada;

/// Carpetas de dependencias donde buscar módulos, del manifiesto.
std::vector<std::filesystem::path> rutas_de_modulos;

/// ¿El programa usa el módulo de ventanas? Entonces hay que compilar su capa C.
bool hace_falta_ui = false;

/// ¿El programa usa tensores? Entonces hay que compilar su capa C.
bool hace_falta_ia = false;

/// Cabeceras que se anteponen a todo archivo de runtime. Cada runtime se compila
/// como una unidad aparte, así que tiene que bastarse a sí mismo; anteponerlas
/// aquí evita el error de olvidar una (pasó tres veces).
constexpr std::string_view CABECERAS_RUNTIME =
    "#include <stddef.h>\n#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n"
    "#include <string.h>\n";

std::string_view fuente_biblioteca(std::string_view modulo) {
    if (modulo == "resultado") {
        return R"(// Biblioteca estándar: el resultado de una operación que puede fallar.
publico enumeracion resultado {
    Ok(valor: entero)
    Error(mensaje: texto)
}
)";
    }
    if (modulo == "textos") {
        return R"(// Biblioteca estándar: utilidades sobre listas de textos.
// Convierte un número en texto, para poder componer mensajes: 42 -> "42".
publico funcion de_entero(valor: entero) -> texto {
    si (valor == 0) {
        retornar "0" + ""
    }
    variable negativo = valor < 0
    variable resto = valor
    si (negativo) {
        resto = 0 - resto
    }

    // Los dígitos salen del revés; se guardan y luego se componen al derecho.
    variable digitos = Vector<entero>.nuevo()
    mientras (resto > 0) {
        digitos.agregar(resto % 10)
        resto = resto / 10
    }

    variable salida = "" + ""
    si (negativo) {
        salida = "-" + ""
    }
    variable i = digitos.largo() - 1
    mientras (i >= 0) {
        salida = salida.agregar_codigo(48 + digitos.obtener(i))
        i = i - 1
    }
    retornar salida
}

// Un flotante sale con coma decimal y sin ceros de más: 2,5 y 3.
publico funcion de_flotante(valor: flotante) -> texto {
    variable negativo = valor < 0.0
    variable magnitud = valor
    si (negativo) {
        magnitud = 0.0 - magnitud
    }

    variable entera = magnitud como entero
    variable fraccion = magnitud - (entera como flotante)

    variable salida = de_entero(entera)
    si (negativo) {
        salida = "-" + salida
    }

    // Seis decimales redondeados; si no queda fracción, no se pone coma.
    variable milesimas = (fraccion * 1000000.0 + 0.5) como entero
    si (milesimas >= 1000000) {
        milesimas = 999999
    }
    si (milesimas == 0) {
        retornar salida
    }

    variable cuantos = 6
    mientras (cuantos > 0 && milesimas % 10 == 0) {
        milesimas = milesimas / 10
        cuantos = cuantos - 1
    }

    // Los dígitos de la fracción, con ceros a la izquierda hasta `cuantos`.
    variable cifras = Vector<entero>.nuevo()
    variable resto = milesimas
    mientras (resto > 0) {
        cifras.agregar(resto % 10)
        resto = resto / 10
    }
    variable relleno = cuantos - cifras.largo()

    variable decimales = "" + ""
    variable rellenos = 0
    mientras (rellenos < relleno) {
        decimales = decimales.agregar_codigo(48)
        rellenos = rellenos + 1
    }
    variable i = cifras.largo() - 1
    mientras (i >= 0) {
        decimales = decimales.agregar_codigo(48 + cifras.obtener(i))
        i = i - 1
    }

    variable coma = "," + ""
    retornar salida + coma + decimales
}

publico funcion unir(partes: &Vector<texto>, separador: texto) -> texto {
    variable junto = ""
    variable i = 0
    mientras (i < partes.largo()) {
        si (i > 0) {
            junto = junto + separador
        }
        junto = junto + partes.obtener(i)
        i = i + 1
    }
    retornar junto
}

publico funcion repetir(pieza: &texto, veces: entero) -> texto {
    variable junto = ""
    variable i = 0
    mientras (i < veces) {
        junto = junto + *pieza
        i = i + 1
    }
    retornar junto
}

publico funcion contar(pieza: &texto, aguja: &texto) -> entero {
    variable total = 0
    variable resto = *pieza
    mientras (resto.largo() > 0) {
        variable posicion = resto.buscar(*aguja)
        si (posicion < 0) {
            romper
        }
        total = total + 1
        resto = resto.subtexto(posicion + 1, resto.largo())
    }
    retornar total
}
)";
    }
    if (modulo == "redes") {
        return R"(// Biblioteca estándar: redes neuronales de una capa oculta.
// Se entrenan con descenso de gradiente sobre el grafo de autograd.
externo funcion nooki_ia_red_nueva(entradas: entero, ocultas: entero) -> entero
externo funcion nooki_ia_red_ejemplo(red: entero, x1: flotante, x2: flotante,
                                     x3: flotante, x4: flotante,
                                     esperado: flotante) -> vacio
externo funcion nooki_ia_red_ejemplos(red: entero) -> entero
externo funcion nooki_ia_red_entrenar(red: entero, vueltas: entero, tasa: flotante) -> vacio
externo funcion nooki_ia_red_predecir(red: entero, x1: flotante, x2: flotante,
                                      x3: flotante, x4: flotante) -> flotante
externo funcion nooki_ia_red_error(red: entero) -> flotante
externo funcion nooki_ia_red_liberar(red: entero) -> vacio
externo funcion nooki_ia_red_guardar(red: entero, ruta: &texto) -> entero
externo funcion nooki_ia_red_cargar(ruta: &texto) -> entero

externo funcion nooki_ia_clases_nueva(entradas: entero, ocultas: entero, clases: entero) -> entero
externo funcion nooki_ia_clases_ejemplo(red: entero, x1: flotante, x2: flotante,
                                         x3: flotante, x4: flotante, cual: entero) -> vacio
externo funcion nooki_ia_clases_entrenar(red: entero, vueltas: entero, tasa: flotante) -> vacio
externo funcion nooki_ia_clases_predecir(red: entero, x1: flotante, x2: flotante,
                                         x3: flotante, x4: flotante) -> entero
externo funcion nooki_ia_clases_probabilidad(red: entero, x1: flotante, x2: flotante,
                                             x3: flotante, x4: flotante, cual: entero) -> flotante
externo funcion nooki_ia_clases_aciertos(red: entero) -> entero
externo funcion nooki_ia_clases_total(red: entero) -> entero
externo funcion nooki_ia_clases_liberar(red: entero) -> vacio
externo funcion nooki_ia_clases_guardar(red: entero, ruta: &texto) -> entero
externo funcion nooki_ia_clases_cargar(ruta: &texto) -> entero
externo funcion nooki_ia_clases_ejemplo_datos(red: entero, x: &Vector<flotante>,
                                               cual: entero) -> entero
externo funcion nooki_ia_clases_predecir_datos(red: entero, x: &Vector<flotante>) -> entero
externo funcion nooki_ia_clases_probabilidad_datos(red: entero, x: &Vector<flotante>,
                                                    cual: entero) -> flotante

externo funcion nooki_ia_profunda_nueva(entradas: entero, ocultas: entero,
                                        clases: entero) -> entero
externo funcion nooki_ia_profunda_ejemplo(red: entero, x: &Vector<flotante>,
                                          cual: entero) -> entero
externo funcion nooki_ia_profunda_entrenar(red: entero, vueltas: entero, tasa: flotante) -> vacio
externo funcion nooki_ia_profunda_predecir(red: entero, x: &Vector<flotante>) -> entero
externo funcion nooki_ia_profunda_confianza(red: entero, x: &Vector<flotante>) -> flotante
externo funcion nooki_ia_profunda_aciertos(red: entero) -> entero
externo funcion nooki_ia_profunda_total(red: entero) -> entero
externo funcion nooki_ia_profunda_liberar(red: entero) -> vacio
externo funcion nooki_ia_profunda_evaluar(red: entero, x: &Vector<flotante>,
                                          cual: entero) -> entero
externo funcion nooki_ia_profunda_guardar(red: entero, ruta: &texto) -> entero
externo funcion nooki_ia_profunda_cargar(ruta: &texto) -> entero
externo funcion nooki_ia_profunda_entrenar_lote(red: entero, vueltas: entero, tasa: flotante,
                                                 lote: entero, momento: flotante) -> vacio

// Crea una red con esas entradas (1 a 4) y neuronas ocultas.
publico funcion nueva(entradas: entero, ocultas: entero) -> entero {
    retornar nooki_ia_red_nueva(entradas, ocultas)
}

// Añade un ejemplo. Las entradas que no se usen se pasan a 0.0.
publico funcion ejemplo(red: entero, x1: flotante, x2: flotante, x3: flotante, x4: flotante,
                        esperado: flotante) -> vacio {
    nooki_ia_red_ejemplo(red, x1, x2, x3, x4, esperado)
}

publico funcion ejemplos(red: entero) -> entero {
    retornar nooki_ia_red_ejemplos(red)
}

// Guarda la red, con sus pesos y sus ejemplos, en un archivo.
publico funcion guardar(red: entero, ruta: &texto) -> booleano {
    retornar nooki_ia_red_guardar(red, ruta) != 0
}

// Carga una red guardada. Devuelve 0 si el archivo no sirve.
publico funcion cargar(ruta: &texto) -> entero {
    retornar nooki_ia_red_cargar(ruta)
}

/* --- Red de varias clases ---
 * Igual que la anterior, pero la salida es una probabilidad por clase y se
 * predice la más probable. Sirve para clasificar en 2 a 16 clases. */

// Crea una red de esas clases. Devuelve 0 si los tamaños no valen.
publico funcion nueva_clases(entradas: entero, ocultas: entero, clases: entero) -> entero {
    retornar nooki_ia_clases_nueva(entradas, ocultas, clases)
}

// Añade una muestra; la clase empieza en 0.
publico funcion ejemplo_clase(red: entero, x1: flotante, x2: flotante, x3: flotante,
                              x4: flotante, cual: entero) -> vacio {
    nooki_ia_clases_ejemplo(red, x1, x2, x3, x4, cual)
}

publico funcion entrenar_clases(red: entero, vueltas: entero, tasa: flotante) -> vacio {
    nooki_ia_clases_entrenar(red, vueltas, tasa)
}

// La clase más probable para esa entrada.
publico funcion predecir_clase(red: entero, x1: flotante, x2: flotante, x3: flotante,
                               x4: flotante) -> entero {
    retornar nooki_ia_clases_predecir(red, x1, x2, x3, x4)
}

// La probabilidad de una clase concreta, entre 0 y 1.
publico funcion probabilidad(red: entero, x1: flotante, x2: flotante, x3: flotante,
                             x4: flotante, cual: entero) -> flotante {
    retornar nooki_ia_clases_probabilidad(red, x1, x2, x3, x4, cual)
}

publico funcion aciertos_clase(red: entero) -> entero {
    retornar nooki_ia_clases_aciertos(red)
}

publico funcion total_clase(red: entero) -> entero {
    retornar nooki_ia_clases_total(red)
}

publico funcion liberar_clase(red: entero) -> vacio {
    nooki_ia_clases_liberar(red)
}

// Guarda la red de clases, con sus pesos y sus muestras, en un archivo.
publico funcion guardar_clase(red: entero, ruta: &texto) -> booleano {
    retornar nooki_ia_clases_guardar(red, ruta) != 0
}

// Carga una red de clases guardada. Devuelve 0 si el archivo no sirve.
publico funcion cargar_clase(ruta: &texto) -> entero {
    retornar nooki_ia_clases_cargar(ruta)
}

/* Entradas de cualquier tamaño: en vez de x1..x4 se pasa un `Vector<flotante>`
 * con tantos valores como entradas tenga la red. */

// Añade una muestra con su clase. El vector tiene que tener `entradas` valores.
publico funcion ejemplo_datos(red: entero, x: &Vector<flotante>, cual: entero) -> booleano {
    retornar nooki_ia_clases_ejemplo_datos(red, x, cual) != 0
}

// La clase más probable para esos valores.
publico funcion predecir_datos(red: entero, x: &Vector<flotante>) -> entero {
    retornar nooki_ia_clases_predecir_datos(red, x)
}

publico funcion probabilidad_datos(red: entero, x: &Vector<flotante>, cual: entero) -> flotante {
    retornar nooki_ia_clases_probabilidad_datos(red, x, cual)
}

/* --- Red de dos capas ocultas ---
 * Para problemas que no se resuelven con una sola: la primera capa combina las
 * entradas y la segunda combina lo que sacó la primera. */

publico funcion nueva_profunda(entradas: entero, ocultas: entero, clases: entero) -> entero {
    retornar nooki_ia_profunda_nueva(entradas, ocultas, clases)
}

publico funcion ejemplo_profunda(red: entero, x: &Vector<flotante>, cual: entero) -> booleano {
    retornar nooki_ia_profunda_ejemplo(red, x, cual) != 0
}

publico funcion entrenar_profunda(red: entero, vueltas: entero, tasa: flotante) -> vacio {
    nooki_ia_profunda_entrenar(red, vueltas, tasa)
}

publico funcion predecir_profunda(red: entero, x: &Vector<flotante>) -> entero {
    retornar nooki_ia_profunda_predecir(red, x)
}

publico funcion confianza_profunda(red: entero, x: &Vector<flotante>) -> flotante {
    retornar nooki_ia_profunda_confianza(red, x)
}

publico funcion aciertos_profunda(red: entero) -> entero {
    retornar nooki_ia_profunda_aciertos(red)
}

publico funcion total_profunda(red: entero) -> entero {
    retornar nooki_ia_profunda_total(red)
}

publico funcion liberar_profunda(red: entero) -> vacio {
    nooki_ia_profunda_liberar(red)
}

// Entrena por lotes y con momento. `lote` muestras por paso; `momento` entre 0
// y 1 (0,9 va bien). Aprende más rápido y aguanta tasas más altas.
publico funcion entrenar_profunda_lote(red: entero, vueltas: entero, tasa: flotante,
                                       lote: entero, momento: flotante) -> vacio {
    nooki_ia_profunda_entrenar_lote(red, vueltas, tasa, lote, momento)
}

// ¿Acierta esta muestra? Para medir con datos que no se usaron al entrenar.
publico funcion evaluar_profunda(red: entero, x: &Vector<flotante>, cual: entero) -> booleano {
    retornar nooki_ia_profunda_evaluar(red, x, cual) != 0
}

// Guarda la red profunda, con sus pesos y sus muestras.
publico funcion guardar_profunda(red: entero, ruta: &texto) -> booleano {
    retornar nooki_ia_profunda_guardar(red, ruta) != 0
}

// Carga una red profunda guardada. Devuelve 0 si el archivo no sirve.
publico funcion cargar_profunda(ruta: &texto) -> entero {
    retornar nooki_ia_profunda_cargar(ruta)
}

// Entrena: vueltas sobre los ejemplos con esa tasa de aprendizaje.
publico funcion entrenar(red: entero, vueltas: entero, tasa: flotante) -> vacio {
    nooki_ia_red_entrenar(red, vueltas, tasa)
}

publico funcion predecir(red: entero, x1: flotante, x2: flotante, x3: flotante,
                         x4: flotante) -> flotante {
    retornar nooki_ia_red_predecir(red, x1, x2, x3, x4)
}

publico funcion error(red: entero) -> flotante {
    retornar nooki_ia_red_error(red)
}

publico funcion liberar(red: entero) -> vacio {
    nooki_ia_red_liberar(red)
}
)";
    }
    if (modulo == "autograd") {
        return R"(// Biblioteca estándar: derivadas automáticas sobre un grafo de operaciones.
// Se construye la expresión, se llama a `atras` y cada nodo tiene su gradiente.
externo funcion nooki_ia_auto_dato(valor: flotante) -> entero
externo funcion nooki_ia_auto_valor(nodo: entero) -> flotante
externo funcion nooki_ia_auto_gradiente(nodo: entero) -> flotante
externo funcion nooki_ia_auto_suma(a: entero, b: entero) -> entero
externo funcion nooki_ia_auto_resta(a: entero, b: entero) -> entero
externo funcion nooki_ia_auto_producto(a: entero, b: entero) -> entero
externo funcion nooki_ia_auto_division(a: entero, b: entero) -> entero
externo funcion nooki_ia_auto_relu(a: entero) -> entero
externo funcion nooki_ia_auto_sigmoides(a: entero) -> entero
externo funcion nooki_ia_auto_atras(nodo: entero) -> vacio
externo funcion nooki_ia_auto_liberar(nodo: entero) -> vacio

// Un número del que se puede derivar.
publico funcion dato(valor: flotante) -> entero {
    retornar nooki_ia_auto_dato(valor)
}

publico funcion valor(nodo: entero) -> flotante {
    retornar nooki_ia_auto_valor(nodo)
}

publico funcion gradiente(nodo: entero) -> flotante {
    retornar nooki_ia_auto_gradiente(nodo)
}

publico funcion suma(a: entero, b: entero) -> entero {
    retornar nooki_ia_auto_suma(a, b)
}

publico funcion resta(a: entero, b: entero) -> entero {
    retornar nooki_ia_auto_resta(a, b)
}

publico funcion producto(a: entero, b: entero) -> entero {
    retornar nooki_ia_auto_producto(a, b)
}

publico funcion division(a: entero, b: entero) -> entero {
    retornar nooki_ia_auto_division(a, b)
}

publico funcion relu(a: entero) -> entero {
    retornar nooki_ia_auto_relu(a)
}

publico funcion sigmoides(a: entero) -> entero {
    retornar nooki_ia_auto_sigmoides(a)
}

// Calcula los gradientes de todo el grafo desde este nodo.
publico funcion atras(nodo: entero) -> vacio {
    nooki_ia_auto_atras(nodo)
}

publico funcion liberar(nodo: entero) -> vacio {
    nooki_ia_auto_liberar(nodo)
}
)";
    }
    if (modulo == "tabla") {
        return R"(// Biblioteca estándar: tablas con columnas con nombre, como un DataFrame.
// Las columnas son numéricas o de texto. Los números con decimales van por mil.
externo funcion nooki_ia_tabla_crear(filas: entero) -> entero
externo funcion nooki_ia_tabla_columna_entero(t: entero, nombre: &texto) -> entero
externo funcion nooki_ia_tabla_columna_texto(t: entero, nombre: &texto) -> entero
externo funcion nooki_ia_tabla_filas(t: entero) -> entero
externo funcion nooki_ia_tabla_columnas(t: entero) -> entero
externo funcion nooki_ia_tabla_nombre(t: entero, columna: entero) -> texto
externo funcion nooki_ia_tabla_poner_entero(t: entero, fila: entero, columna: entero,
                                            valor: entero) -> vacio
externo funcion nooki_ia_tabla_poner_texto(t: entero, fila: entero, columna: entero,
                                           valor: &texto) -> vacio
externo funcion nooki_ia_tabla_valor_entero(t: entero, fila: entero, columna: entero) -> entero
externo funcion nooki_ia_tabla_valor_texto(t: entero, fila: entero, columna: entero) -> texto
externo funcion nooki_ia_tabla_suma(t: entero, columna: entero) -> entero
externo funcion nooki_ia_tabla_media(t: entero, columna: entero) -> entero
externo funcion nooki_ia_tabla_minimo(t: entero, columna: entero) -> entero
externo funcion nooki_ia_tabla_maximo(t: entero, columna: entero) -> entero
externo funcion nooki_ia_tabla_filtrar_texto(t: entero, columna: entero, valor: &texto) -> entero
externo funcion nooki_ia_tabla_ordenar(t: entero, columna: entero) -> entero
externo funcion nooki_ia_tabla_contar_por(t: entero, columna: entero) -> entero
externo funcion nooki_ia_tabla_liberar(t: entero) -> vacio

publico funcion crear(filas: entero) -> entero {
    retornar nooki_ia_tabla_crear(filas)
}

publico funcion columna_entero(t: entero, nombre: &texto) -> entero {
    retornar nooki_ia_tabla_columna_entero(t, nombre)
}

publico funcion columna_texto(t: entero, nombre: &texto) -> entero {
    retornar nooki_ia_tabla_columna_texto(t, nombre)
}

publico funcion filas(t: entero) -> entero {
    retornar nooki_ia_tabla_filas(t)
}

publico funcion columnas(t: entero) -> entero {
    retornar nooki_ia_tabla_columnas(t)
}

publico funcion nombre_columna(t: entero, columna: entero) -> texto {
    retornar nooki_ia_tabla_nombre(t, columna)
}

publico funcion poner_entero(t: entero, fila: entero, columna: entero, valor: entero) -> vacio {
    nooki_ia_tabla_poner_entero(t, fila, columna, valor)
}

publico funcion poner_texto(t: entero, fila: entero, columna: entero, valor: &texto) -> vacio {
    nooki_ia_tabla_poner_texto(t, fila, columna, valor)
}

publico funcion valor_entero(t: entero, fila: entero, columna: entero) -> entero {
    retornar nooki_ia_tabla_valor_entero(t, fila, columna)
}

publico funcion valor_texto(t: entero, fila: entero, columna: entero) -> texto {
    retornar nooki_ia_tabla_valor_texto(t, fila, columna)
}

publico funcion suma(t: entero, columna: entero) -> entero {
    retornar nooki_ia_tabla_suma(t, columna)
}

publico funcion media(t: entero, columna: entero) -> entero {
    retornar nooki_ia_tabla_media(t, columna)
}

publico funcion minimo(t: entero, columna: entero) -> entero {
    retornar nooki_ia_tabla_minimo(t, columna)
}

publico funcion maximo(t: entero, columna: entero) -> entero {
    retornar nooki_ia_tabla_maximo(t, columna)
}

// Filas cuyo valor de la columna de texto coincide.
publico funcion filtrar(t: entero, columna: entero, valor: &texto) -> entero {
    retornar nooki_ia_tabla_filtrar_texto(t, columna, valor)
}

// Filas ordenadas de menor a mayor por una columna numérica.
publico funcion ordenar(t: entero, columna: entero) -> entero {
    retornar nooki_ia_tabla_ordenar(t, columna)
}

// Cuántas filas hay de cada valor. Devuelve una tabla con «valor» y «cuenta».
publico funcion contar_por(t: entero, columna: entero) -> entero {
    retornar nooki_ia_tabla_contar_por(t, columna)
}

publico funcion liberar(t: entero) -> vacio {
    nooki_ia_tabla_liberar(t)
}
)";
    }
    if (modulo == "pln") {
        return R"(// Biblioteca estándar: procesamiento de lenguaje natural.
// Todo con textos, vectores de textos y mapas: nada de C.

// Pasa a minúsculas y deja un solo espacio entre palabras.
publico funcion normalizar(frase: &texto) -> texto {
    // En dos pasos: encadenar dos llamadas deja el texto intermedio sin dueño.
    variable en_minusculas: texto = (*frase).minusculas()
    variable resto: texto = en_minusculas.recortar()
    variable resultado: texto = "" + ""
    variable separador: texto = " " + ""

    mientras (resto.largo() > 0) {
        variable espacio: entero = resto.buscar(&separador)
        si (espacio < 0) {
            // La última palabra: también lleva separador delante.
            si (resultado.largo() > 0) {
                resultado = resultado + " " + ""
            }
            resultado = resultado + resto
            romper
        }
        variable trozo: texto = resto.subtexto(0, espacio)
        si (trozo.largo() > 0) {
            si (resultado.largo() > 0) {
                resultado = resultado + " " + ""
            }
            resultado = resultado + trozo
        }
        resto = resto.subtexto(espacio + 1, resto.largo() - espacio - 1)
    }
    retornar resultado
}

// Separa por palabras. No devuelve piezas vacías.
publico funcion tokenizar(frase: &texto) -> Vector<texto> {
    variable limpio: texto = normalizar(frase)
    variable palabras: Vector<texto> = Vector<texto>.nuevo()
    variable resto: texto = limpio
    variable separador: texto = " " + ""

    mientras (resto.largo() > 0) {
        variable espacio: entero = resto.buscar(&separador)
        si (espacio < 0) {
            palabras.agregar(resto)
            romper
        }
        variable trozo: texto = resto.subtexto(0, espacio)
        si (trozo.largo() > 0) {
            palabras.agregar(trozo)
        }
        resto = resto.subtexto(espacio + 1, resto.largo() - espacio - 1)
    }
    retornar palabras
}

// Cuenta cuántas veces aparece cada palabra.
publico funcion contar(palabras: &Vector<texto>) -> Mapa<texto, entero> {
    variable cuentas: Mapa<texto, entero> = Mapa<texto, entero>.nuevo()
    variable i: entero = 0
    mientras (i < (*palabras).largo()) {
        variable palabra: texto = (*palabras).obtener(i)
        variable veces: entero = 0
        si (cuentas.contiene(&palabra)) {
            veces = cuentas.obtener(&palabra)
        }
        cuentas.insertar(&palabra, veces + 1)
        i = i + 1
    }
    retornar cuentas
}

// Palabras distintas, en el orden en que aparecen.
publico funcion unicas(palabras: &Vector<texto>) -> Vector<texto> {
    variable vistas: Mapa<texto, entero> = Mapa<texto, entero>.nuevo()
    variable salida: Vector<texto> = Vector<texto>.nuevo()
    variable i: entero = 0
    mientras (i < (*palabras).largo()) {
        variable palabra: texto = (*palabras).obtener(i)
        si (!vistas.contiene(&palabra)) {
            vistas.insertar(&palabra, 1)
            salida.agregar(palabra)
        }
        i = i + 1
    }
    retornar salida
}

// Distancia de edición: cuántos cambios hacen falta para pasar de una a otra.
publico funcion distancia(a: &texto, b: &texto) -> entero {
    variable n: entero = (*a).largo()
    variable m: entero = (*b).largo()
    si (n == 0) {
        retornar m
    }
    si (m == 0) {
        retornar n
    }

    variable previa: Vector<entero> = Vector<entero>.nuevo()
    variable j: entero = 0
    mientras (j <= m) {
        previa.agregar(j)
        j = j + 1
    }

    variable i: entero = 1
    mientras (i <= n) {
        variable actual: Vector<entero> = Vector<entero>.nuevo()
        actual.agregar(i)

        variable jota: entero = 1
        mientras (jota <= m) {
            variable costo: entero = 1
            si ((*a).caracter(i - 1) == (*b).caracter(jota - 1)) {
                costo = 0
            }
            variable borrar: entero = previa.obtener(jota) + 1
            variable insertar: entero = actual.obtener(jota - 1) + 1
            variable cambiar: entero = previa.obtener(jota - 1) + costo
            variable menor: entero = borrar
            si (insertar < menor) {
                menor = insertar
            }
            si (cambiar < menor) {
                menor = cambiar
            }
            actual.agregar(menor)
            jota = jota + 1
        }
        previa = actual
        i = i + 1
    }
    retornar previa.obtener(m)
}
// ¿Está esta palabra en la lista?
publico funcion contiene_texto(palabras: &Vector<texto>, buscada: &texto) -> booleano {
    variable i: entero = 0
    mientras (i < (*palabras).largo()) {
        variable palabra: texto = (*palabras).obtener(i)
        si (palabra == *buscada) {
            retornar verdadero
        }
        i = i + 1
    }
    retornar falso
}
// ¿Esta palabra ya había salido antes de esa posición?
funcion repetida(palabras: &Vector<texto>, posicion: entero) -> booleano {
    variable i: entero = 0
    mientras (i < posicion) {
        variable anterior: texto = (*palabras).obtener(i)
        variable actual: texto = (*palabras).obtener(posicion)
        si (anterior == actual) {
            retornar verdadero
        }
        i = i + 1
    }
    retornar falso
}

// Cuántas palabras distintas hay.
publico funcion distintas(palabras: &Vector<texto>) -> entero {
    variable total: entero = 0
    variable i: entero = 0
    mientras (i < (*palabras).largo()) {
        si (!repetida(palabras, i)) {
            total = total + 1
        }
        i = i + 1
    }
    retornar total
}

// Palabras distintas de la primera lista que también están en la segunda.
funcion comunes(a: &Vector<texto>, b: &Vector<texto>) -> entero {
    variable total: entero = 0
    variable i: entero = 0
    mientras (i < (*a).largo()) {
        variable palabra: texto = (*a).obtener(i)
        si (!repetida(a, i)) {
            variable j: entero = 0
            variable encontrada: booleano = falso
            mientras (j < (*b).largo()) {
                variable otra: texto = (*b).obtener(j)
                si (otra == palabra) {
                    encontrada = verdadero
                }
                j = j + 1
            }
            si (encontrada) {
                total = total + 1
            }
        }
        i = i + 1
    }
    retornar total
}

// Parecido entre dos frases, por mil: intersección sobre unión de palabras.
publico funcion jaccard(a: &texto, b: &texto) -> entero {
    variable trozos_a: Vector<texto> = tokenizar(a)
    variable trozos_b: Vector<texto> = tokenizar(b)

    variable distintas_a: entero = distintas(&trozos_a)
    variable distintas_b: entero = distintas(&trozos_b)
    variable en_comun: entero = comunes(&trozos_a, &trozos_b)

    variable union_total: entero = distintas_a + distintas_b - en_comun
    si (union_total == 0) {
        retornar 1000
    }
    retornar en_comun * 1000 / union_total
}

// Cuántas palabras hay.
publico funcion cuantas(palabras: &Vector<texto>) -> entero {
    retornar (*palabras).largo()
}
)";
    }
    if (modulo == "tensor") {
        return R"(// Biblioteca estándar: tensores (matrices de enteros de 64 bits).
externo funcion nooki_ia_tensor_ceros(filas: entero, columnas: entero) -> entero
externo funcion nooki_ia_tensor_lleno(filas: entero, columnas: entero, valor: entero) -> entero
externo funcion nooki_ia_tensor_identidad(lado: entero) -> entero
externo funcion nooki_ia_tensor_filas(t: entero) -> entero
externo funcion nooki_ia_tensor_columnas(t: entero) -> entero
externo funcion nooki_ia_tensor_leer(t: entero, fila: entero, columna: entero) -> entero
externo funcion nooki_ia_tensor_poner(t: entero, fila: entero, columna: entero,
                                      valor: entero) -> entero
externo funcion nooki_ia_tensor_suma(a: entero, b: entero) -> entero
externo funcion nooki_ia_tensor_resta(a: entero, b: entero) -> entero
externo funcion nooki_ia_tensor_escalar(a: entero, factor: entero) -> entero
externo funcion nooki_ia_tensor_producto(a: entero, b: entero) -> entero
externo funcion nooki_ia_tensor_transpuesta(a: entero) -> entero
externo funcion nooki_ia_tensor_determinante(a: entero) -> entero
externo funcion nooki_ia_tensor_suma_total(a: entero) -> entero
externo funcion nooki_ia_tensor_iguales(a: entero, b: entero) -> entero
externo funcion nooki_ia_tensor_liberar(a: entero) -> vacio
externo funcion nooki_ia_tensor_minimo(t: entero) -> entero
externo funcion nooki_ia_tensor_maximo(t: entero) -> entero
externo funcion nooki_ia_tensor_media(t: entero) -> entero
externo funcion nooki_ia_tensor_varianza(t: entero) -> entero
externo funcion nooki_ia_tensor_desviacion(t: entero) -> entero
externo funcion nooki_ia_tensor_normalizar(t: entero) -> entero
externo funcion nooki_ia_tensor_relu(t: entero) -> entero
externo funcion nooki_ia_tensor_sigmoides(t: entero) -> entero
externo funcion nooki_ia_tensor_softmax(t: entero) -> entero

publico funcion ceros(filas: entero, columnas: entero) -> entero {
    retornar nooki_ia_tensor_ceros(filas, columnas)
}

publico funcion lleno(filas: entero, columnas: entero, valor: entero) -> entero {
    retornar nooki_ia_tensor_lleno(filas, columnas, valor)
}

publico funcion identidad(lado: entero) -> entero {
    retornar nooki_ia_tensor_identidad(lado)
}

publico funcion filas(t: entero) -> entero {
    retornar nooki_ia_tensor_filas(t)
}

publico funcion columnas(t: entero) -> entero {
    retornar nooki_ia_tensor_columnas(t)
}

publico funcion leer(t: entero, fila: entero, columna: entero) -> entero {
    retornar nooki_ia_tensor_leer(t, fila, columna)
}

publico funcion poner(t: entero, fila: entero, columna: entero, valor: entero) -> vacio {
    nooki_ia_tensor_poner(t, fila, columna, valor)
}

publico funcion suma(a: entero, b: entero) -> entero {
    retornar nooki_ia_tensor_suma(a, b)
}

publico funcion resta(a: entero, b: entero) -> entero {
    retornar nooki_ia_tensor_resta(a, b)
}

publico funcion escalar(a: entero, factor: entero) -> entero {
    retornar nooki_ia_tensor_escalar(a, factor)
}

publico funcion producto(a: entero, b: entero) -> entero {
    retornar nooki_ia_tensor_producto(a, b)
}

publico funcion transpuesta(a: entero) -> entero {
    retornar nooki_ia_tensor_transpuesta(a)
}

publico funcion determinante(a: entero) -> entero {
    retornar nooki_ia_tensor_determinante(a)
}

publico funcion suma_total(a: entero) -> entero {
    retornar nooki_ia_tensor_suma_total(a)
}

publico funcion iguales(a: entero, b: entero) -> booleano {
    retornar nooki_ia_tensor_iguales(a, b) != 0
}

publico funcion liberar(a: entero) -> vacio {
    nooki_ia_tensor_liberar(a)
}

// --- Estadística. Lo que tiene decimales se da por mil: 2500 es 2.5. ---

publico funcion minimo(t: entero) -> entero {
    retornar nooki_ia_tensor_minimo(t)
}

publico funcion maximo(t: entero) -> entero {
    retornar nooki_ia_tensor_maximo(t)
}

publico funcion media(t: entero) -> entero {
    retornar nooki_ia_tensor_media(t)
}

publico funcion varianza(t: entero) -> entero {
    retornar nooki_ia_tensor_varianza(t)
}

publico funcion desviacion(t: entero) -> entero {
    retornar nooki_ia_tensor_desviacion(t)
}

publico funcion normalizar(t: entero) -> entero {
    retornar nooki_ia_tensor_normalizar(t)
}

// --- Activaciones ---

publico funcion relu(t: entero) -> entero {
    retornar nooki_ia_tensor_relu(t)
}

publico funcion sigmoides(t: entero) -> entero {
    retornar nooki_ia_tensor_sigmoides(t)
}

publico funcion softmax(t: entero) -> entero {
    retornar nooki_ia_tensor_softmax(t)
}
)";
    }
    if (modulo == "imagenes") {
        return R"(// Biblioteca estándar: imágenes BMP (24 o 32 bits, sin comprimir).
externo funcion nooki_ui_imagen_cargar(ruta: &texto) -> entero
externo funcion nooki_ui_imagen_ancho(imagen: entero) -> entero
externo funcion nooki_ui_imagen_alto(imagen: entero) -> entero
externo funcion nooki_ui_imagen_color_en(imagen: entero, x: entero, y: entero) -> entero
externo funcion nooki_ui_imagen_soltar(imagen: entero) -> vacio
externo funcion nooki_ui_dibujo_imagen(pantalla: entero, imagen: entero, x: entero,
                                       y: entero) -> vacio

// Carga una imagen. Devuelve 0 si no existe o no es un BMP válido.
publico funcion cargar(ruta: &texto) -> entero {
    retornar nooki_ui_imagen_cargar(*ruta)
}

publico funcion cargada(imagen: entero) -> booleano {
    retornar imagen != 0
}

publico funcion ancho(imagen: entero) -> entero {
    retornar nooki_ui_imagen_ancho(imagen)
}

publico funcion alto(imagen: entero) -> entero {
    retornar nooki_ui_imagen_alto(imagen)
}

publico funcion color_en(imagen: entero, x: entero, y: entero) -> entero {
    retornar nooki_ui_imagen_color_en(imagen, x, y)
}

publico funcion soltar(imagen: entero) -> vacio {
    nooki_ui_imagen_soltar(imagen)
}

publico funcion dibujar(pantalla: entero, imagen: entero, x: entero, y: entero) -> vacio {
    nooki_ui_dibujo_imagen(pantalla, imagen, x, y)
}
)";
    }
    if (modulo == "accesibilidad") {
        return R"(// Biblioteca estándar: legibilidad y navegación por teclado.
// Los colores son enteros 0xRRGGBB.

// Luminancia percibida (YIQ): 0 negro, 255 blanco.
publico funcion luminancia(color: entero) -> entero {
    variable rojo = (color / 65536) % 256
    variable verde = (color / 256) % 256
    variable azul = color % 256
    retornar (299 * rojo + 587 * verde + 114 * azul) / 1000
}

// Diferencia de luminancia entre dos colores, en la misma escala.
publico funcion diferencia(a: entero, b: entero) -> entero {
    variable la = luminancia(a)
    variable lb = luminancia(b)
    si (la > lb) {
        retornar la - lb
    }
    retornar lb - la
}

// Criterio práctico: una diferencia de 128 sobre 255 es legible.
// No es el contraste WCAG exacto (que exige corrección gamma), pero es una
// heurística que se calcula con enteros y acierta en los casos normales.
publico funcion legible(texto_color: entero, fondo: entero) -> booleano {
    retornar diferencia(texto_color, fondo) >= 128
}

// Texto negro o blanco, el que más contraste dé sobre ese fondo.
publico funcion texto_sobre(fondo: entero) -> entero {
    si (luminancia(fondo) > 127) {
        retornar 0x000000
    }
    retornar 0xFFFFFF
}

// --- Navegación por teclado ---

// Lleva el control que tiene el foco entre `total` controles, con Tab y
// Mayús+Tab. El índice siempre está en rango.
publico clase foco {
    publico indice: entero
    publico total: entero

    publico funcion siguiente() -> vacio {
        si (este.total <= 0) {
            este.indice = 0
            retornar
        }
        este.indice = (este.indice + 1) % este.total
    }

    publico funcion anterior() -> vacio {
        si (este.total <= 0) {
            este.indice = 0
            retornar
        }
        este.indice = (este.indice + este.total - 1) % este.total
    }

    publico funcion es_actual(cual: entero) -> booleano {
        retornar cual == este.indice
    }

    publico funcion primera() -> vacio {
        este.indice = 0
    }
}
)";
    }
    if (modulo == "controles") {
        return R"(// Biblioteca estándar: controles de interfaz (widgets).
// La geometría se puede probar sin ventana; el dibujo necesita una.
usar ventana

publico clase control {
    publico x: entero
    publico y: entero
    publico ancho: entero
    publico alto: entero

    publico funcion contiene(px: entero, py: entero) -> booleano {
        si (px < este.x) {
            retornar falso
        }
        si (py < este.y) {
            retornar falso
        }
        si (px >= este.x + este.ancho) {
            retornar falso
        }
        si (py >= este.y + este.alto) {
            retornar falso
        }
        retornar verdadero
    }

    publico funcion dibujar(pantalla: entero) -> vacio {
    }
}

publico clase etiqueta hereda control {
    publico contenido: texto
    publico color: entero

    publico funcion dibujar(pantalla: entero) -> vacio {
        ventana.escribir_texto(pantalla, este.x, este.y + este.alto - 6, &este.contenido,
                               este.color)
    }
}

publico clase boton hereda control {
    publico contenido: texto
    publico fondo: entero
    publico color: entero

    publico funcion dibujar(pantalla: entero) -> vacio {
        ventana.rectangulo(pantalla, este.x, este.y, este.ancho, este.alto, este.fondo)
        ventana.escribir_texto(pantalla, este.x + 12, este.y + este.alto / 2 + 5,
                               &este.contenido, este.color)
    }

    publico funcion pulsado(px: entero, py: entero) -> booleano {
        retornar este.contiene(px, py)
    }
}

// Campo de texto: guarda lo que se escribe y responde al teclado.
publico clase campo hereda control {
    publico contenido: texto
    publico fondo: entero
    publico color: entero
    publico marcador: texto

    publico funcion dibujar(pantalla: entero) -> vacio {
        ventana.rectangulo(pantalla, este.x, este.y, este.ancho, este.alto, este.fondo)
        variable escrito = este.contenido
        si (escrito.largo() == 0) {
            escrito = este.marcador
        }
        ventana.escribir_texto(pantalla, este.x + 8, este.y + este.alto / 2 + 5, &escrito,
                               este.color)
    }

    // Añade el carácter tecleado (código de punto).
    publico funcion escribir(codigo: entero) -> vacio {
        este.contenido = este.contenido.agregar_codigo(codigo)
    }

    // Borra el último carácter.
    publico funcion borrar() -> vacio {
        este.contenido = este.contenido.sin_ultimo()
    }

    publico funcion limpiar_campo() -> vacio {
        este.contenido = "" + ""
    }
}

publico clase barra hereda control {
    publico fondo: entero
    publico relleno: entero
    // Parte llena, de 0 a 100.
    publico tanto: entero

    publico funcion dibujar(pantalla: entero) -> vacio {
        ventana.rectangulo(pantalla, este.x, este.y, este.ancho, este.alto, este.fondo)
        variable lleno = este.ancho * este.tanto / 100
        ventana.rectangulo(pantalla, este.x, este.y, lleno, este.alto, este.relleno)
    }
}
)";
    }
    if (modulo == "diseno") {
        return R"(// Biblioteca estándar: colocación de cajas (layout) sin dibujar nada.
// Es computación pura: se puede probar sin abrir una ventana.

publico funcion centrar(disponible: entero, medida: entero) -> entero {
    si (medida >= disponible) {
        retornar 0
    }
    retornar (disponible - medida) / 2
}

publico funcion repartir(disponible: entero, partes: entero) -> entero {
    si (partes <= 0) {
        retornar 0
    }
    retornar disponible / partes
}

// Reparte hijos en vertical dentro de un marco.
publico clase columna {
    publico x: entero
    publico y: entero
    publico ancho: entero
    publico alto: entero
    publico usado: entero
    publico separacion: entero

    // Devuelve el desplazamiento vertical del siguiente hijo y avanza el cursor.
    publico funcion hueco(alto_hijo: entero) -> entero {
        variable posicion = este.usado
        este.usado = este.usado + alto_hijo + este.separacion
        retornar posicion
    }

    publico funcion cabe(alto_hijo: entero) -> booleano {
        retornar este.usado + alto_hijo <= este.alto
    }

    publico funcion reiniciar() -> vacio {
        este.usado = 0
    }
}

// Reparte hijos en horizontal dentro de un marco.
publico clase fila {
    publico x: entero
    publico y: entero
    publico ancho: entero
    publico alto: entero
    publico usado: entero
    publico separacion: entero

    publico funcion hueco(ancho_hijo: entero) -> entero {
        variable posicion = este.usado
        este.usado = este.usado + ancho_hijo + este.separacion
        retornar posicion
    }

    publico funcion cabe(ancho_hijo: entero) -> booleano {
        retornar este.usado + ancho_hijo <= este.ancho
    }

    publico funcion reiniciar() -> vacio {
        este.usado = 0
    }
}
)";
    }
    if (modulo == "ventana") {
        return R"(// Biblioteca estándar: ventanas y bucle de eventos.
// Un manejador es un entero; 0 significa que no hay entorno gráfico.
externo funcion nooki_ui_ventana_crear(titulo: &texto, ancho: entero, alto: entero) -> entero
externo funcion nooki_ui_ventana_viva(manejador: entero) -> booleano
externo funcion nooki_ui_ventana_evento(manejador: entero) -> entero
externo funcion nooki_ui_ventana_cerrar(manejador: entero) -> vacio
externo funcion nooki_ui_ventana_esperar(milisegundos: entero) -> vacio
externo funcion nooki_ui_ventana_repintar(manejador: entero) -> entero

publico funcion abrir(titulo: &texto, ancho: entero, alto: entero) -> entero {
    retornar nooki_ui_ventana_crear(*titulo, ancho, alto)
}

publico funcion viva(manejador: entero) -> booleano {
    retornar nooki_ui_ventana_viva(manejador)
}

publico funcion siguiente_evento(manejador: entero) -> entero {
    retornar nooki_ui_ventana_evento(manejador)
}

publico funcion cerrar(manejador: entero) -> vacio {
    nooki_ui_ventana_cerrar(manejador)
}

publico funcion pausa(milisegundos: entero) -> vacio {
    nooki_ui_ventana_esperar(milisegundos)
}

// Verdadero cuando el sistema pide repintar (la ventana quedó al descubierto,
// se redimensionó o se restauró). Un programa con bucle debe volver a dibujar.
publico funcion repintar(manejador: entero) -> booleano {
    retornar nooki_ui_ventana_repintar(manejador) != 0
}

// --- Eventos. Las clases son: 0 nada, 1 cerrar, 2 movimiento, 3 pulsacion,
// 4 liberacion, 5 tecla. ---
externo funcion nooki_ui_evento_siguiente(manejador: entero) -> entero
externo funcion nooki_ui_evento_x(manejador: entero) -> entero
externo funcion nooki_ui_evento_y(manejador: entero) -> entero
externo funcion nooki_ui_evento_tecla(manejador: entero) -> entero

publico funcion evento(manejador: entero) -> entero {
    retornar nooki_ui_evento_siguiente(manejador)
}

publico funcion evento_x(manejador: entero) -> entero {
    retornar nooki_ui_evento_x(manejador)
}

publico funcion evento_y(manejador: entero) -> entero {
    retornar nooki_ui_evento_y(manejador)
}

publico funcion evento_tecla(manejador: entero) -> entero {
    retornar nooki_ui_evento_tecla(manejador)
}

publico funcion es_cerrar(cual: entero) -> booleano { retornar cual == 1 }
publico funcion es_movimiento(cual: entero) -> booleano { retornar cual == 2 }
publico funcion es_pulsacion(cual: entero) -> booleano { retornar cual == 3 }
publico funcion es_liberacion(cual: entero) -> booleano { retornar cual == 4 }
publico funcion es_tecla(cual: entero) -> booleano { retornar cual == 5 }
publico funcion es_letra(cual: entero) -> booleano { retornar cual == 6 }

// Código del carácter tecleado (UTF-8, ya traducido por el sistema).
publico funcion evento_letra(manejador: entero) -> entero {
    retornar nooki_ui_evento_tecla(manejador)
}

// --- Dibujo. Los colores son 0xRRGGBB. ---
externo funcion nooki_ui_dibujo_limpiar(manejador: entero, color: entero) -> vacio
externo funcion nooki_ui_dibujo_rectangulo(manejador: entero, x: entero, y: entero,
                                           ancho: entero, alto: entero, color: entero) -> vacio
externo funcion nooki_ui_dibujo_linea(manejador: entero, x1: entero, y1: entero,
                                      x2: entero, y2: entero, color: entero) -> vacio
externo funcion nooki_ui_dibujo_texto(manejador: entero, x: entero, y: entero,
                                      contenido: &texto, color: entero) -> vacio
externo funcion nooki_ui_dibujo_presentar(manejador: entero) -> vacio
externo funcion nooki_ui_dibujo_color_en(manejador: entero, x: entero, y: entero) -> entero

publico funcion limpiar(manejador: entero, color: entero) -> vacio {
    nooki_ui_dibujo_limpiar(manejador, color)
}

publico funcion rectangulo(manejador: entero, x: entero, y: entero, ancho: entero,
                           alto: entero, color: entero) -> vacio {
    nooki_ui_dibujo_rectangulo(manejador, x, y, ancho, alto, color)
}

publico funcion linea(manejador: entero, x1: entero, y1: entero, x2: entero, y2: entero,
                      color: entero) -> vacio {
    nooki_ui_dibujo_linea(manejador, x1, y1, x2, y2, color)
}

publico funcion escribir_texto(manejador: entero, x: entero, y: entero, contenido: &texto,
                      color: entero) -> vacio {
    nooki_ui_dibujo_texto(manejador, x, y, *contenido, color)
}

publico funcion presentar(manejador: entero) -> vacio {
    nooki_ui_dibujo_presentar(manejador)
}

publico funcion color_en(manejador: entero, x: entero, y: entero) -> entero {
    retornar nooki_ui_dibujo_color_en(manejador, x, y)
}
)";
    }
    if (modulo == "csv") {
        return R"(// Biblioteca estándar: leer archivos CSV, una fila por línea.
usar archivo

// Las filas del archivo, sin las líneas vacías. Si no se puede leer, ninguna.
publico funcion filas(ruta: &texto) -> Vector<texto> {
    variable salida = Vector<texto>.nuevo()

    variable leido = archivo.leer(ruta)
    coincide (leido) {
        caso Mal(motivo) => {
            retornar salida
        }
        caso Bien(contenido) => {
            variable salto = "\n" + ""
            variable lineas = contenido.dividir(&salto)
            variable i = 0
            mientras (i < lineas.largo()) {
                variable linea = lineas.obtener(i)
                variable limpia = linea.recortar()
                si (limpia.largo() > 0) {
                    salida.agregar(limpia)
                }
                i = i + 1
            }
            retornar salida
        }
    }
}

// Los campos de una fila, separados por comas.
publico funcion campos(linea: &texto) -> Vector<texto> {
    variable coma = "," + ""
    retornar campos_con(linea, &coma)
}

// Los campos de una fila, con el separador que se quiera. Con punto y coma
// caben números con coma decimal: `7,5`.
publico funcion campos_con(linea: &texto, separador: &texto) -> Vector<texto> {
    variable partes = (*linea).dividir(separador)
    variable salida = Vector<texto>.nuevo()
    variable i = 0
    mientras (i < partes.largo()) {
        salida.agregar(partes.obtener(i).recortar())
        i = i + 1
    }
    retornar salida
}

// El campo como número. Un campo que no es número vale 0.
publico funcion a_numero(campo: &texto) -> flotante {
    retornar (*campo).a_flotante()
}

// El campo como entero.
publico funcion a_entero(campo: &texto) -> entero {
    retornar (*campo).a_entero()
}

// Cuántas columnas tiene la primera fila.
publico funcion columnas(ruta: &texto) -> entero {
    variable todas = filas(ruta)
    si (todas.largo() == 0) {
        retornar 0
    }
    variable primera = todas.obtener(0)
    variable partes = campos(&primera)
    retornar partes.largo()
}
)";
    }
    if (modulo == "datos") {
        return R"nooki(usar archivo

// Biblioteca estándar: análisis de datos con SQL.
//
// Detrás está DuckDB, que se carga cuando el programa lo usa: lee CSV y Parquet,
// agrupa, une, pivota y calcula ventanas, que es lo que se hace con pandas.
// La consulta se escribe en SQL y el resultado vuelve como CSV, con cabecera.
//
// Una tabla: el resultado de una consulta, ya en memoria. Se lee por fila y
// columna y se libera con `liberar`.
publico estructura Tabla {
    publico manejador: entero
}

// La biblioteca se busca en NOOKI_DUCKDB, junto al programa o en el PATH.


// ¿Está la biblioteca? Útil para avisar en vez de fallar.
publico funcion disponible() -> booleano {
    retornar disponible_datos() != 0
}

// El motivo del último fallo (vacío si no hubo).
publico funcion error() -> texto {
    retornar error_datos()
}

// Ejecuta la consulta y devuelve la tabla. Con manejador 0, no se pudo.
publico funcion tabla(consulta: &texto) -> Tabla {
    variable h = tabla_de_consulta(*consulta)
    retornar Tabla { manejador: h }
}

// ¿Se pudo construir? Con un manejador a cero, no.
publico funcion hay(t: Tabla) -> booleano {
    retornar t.manejador != 0
}

// Cuántas filas tiene.
publico funcion filas(t: Tabla) -> entero {
    retornar filas_de_tabla(t.manejador)
}

// Cuántas columnas tiene.
publico funcion columnas(t: Tabla) -> entero {
    retornar columnas_de_tabla(t.manejador)
}

// El nombre de una columna.
publico funcion nombre_columna(t: Tabla, columna: entero) -> texto {
    retornar nombre_de_columna(t.manejador, columna)
}

// Un valor, como texto. Vacío si la posición no existe.
publico funcion valor(t: Tabla, fila: entero, columna: entero) -> texto {
    retornar valor_de_tabla(t.manejador, fila, columna)
}

// Un valor como número, para sumar o comparar.
publico funcion numero(t: Tabla, fila: entero, columna: entero) -> flotante {
    retornar numero_de_tabla(t.manejador, fila, columna)
}

// Un valor como entero.
publico funcion entero_de(t: Tabla, fila: entero, columna: entero) -> entero {
    retornar entero_de_tabla(t.manejador, fila, columna)
}

// Todos los valores de una columna, como texto.
publico funcion columna(t: Tabla, indice: entero) -> Vector<texto> {
    variable valores = Vector<texto>.nuevo()
    variable f = 0
    mientras (f < filas_de_tabla(t.manejador)) {
        variable v = valor_de_tabla(t.manejador, f, indice)
        valores.agregar(v + "")
        f = f + 1
    }
    retornar valores
}

// La tabla como CSV, para imprimir o guardar.
publico funcion a_texto(t: Tabla) -> texto {
    variable salida = "" + ""
    variable c = 0
    mientras (c < columnas_de_tabla(t.manejador)) {
        si (c > 0) {
            salida = salida + ","
        }
        salida = salida + nombre_de_columna(t.manejador, c)
        c = c + 1
    }
    salida = salida + "\n"

    variable f = 0
    mientras (f < filas_de_tabla(t.manejador)) {
        variable k = 0
        mientras (k < columnas_de_tabla(t.manejador)) {
            si (k > 0) {
                salida = salida + ","
            }
            salida = salida + valor_de_tabla(t.manejador, f, k)
            k = k + 1
        }
        salida = salida + "\n"
        f = f + 1
    }
    retornar salida
}

// Añade una columna con los valores dados, que tienen que ser tantos como
// filas. Es la forma de calcular una columna a medida: se construye el vector
// con un bucle y se añade.
publico funcion agregar_columna(t: Tabla, nombre: &texto, valores: Vector<texto>) -> booleano {
    retornar agregar_columna_a_tabla(t.manejador, nombre, valores) != 0
}

// El índice de una columna por su nombre, o -1 si no está.
publico funcion indice_columna(t: Tabla, nombre: &texto) -> entero {
    retornar indice_de_columna_en_tabla(t.manejador, nombre)
}

// ¿Tiene esa columna?
publico funcion hay_columna(t: Tabla, nombre: &texto) -> booleano {
    retornar indice_de_columna_en_tabla(t.manejador, nombre) >= 0
}

// Los valores de una columna, por su nombre.
publico funcion columna_por_nombre(t: Tabla, nombre: &texto) -> Vector<texto> {
    variable indice = indice_de_columna_en_tabla(t.manejador, nombre)
    retornar columna(t, indice)
}

// Un valor por el nombre de su columna. Vacío si la columna no está.
publico funcion valor_por_nombre(t: Tabla, fila: entero, nombre: &texto) -> texto {
    variable indice = indice_de_columna_en_tabla(t.manejador, nombre)
    si (indice < 0) {
        retornar "" + ""
    }
    retornar valor_de_tabla(t.manejador, fila, indice)
}

// Un número por el nombre de su columna.
publico funcion numero_por_nombre(t: Tabla, fila: entero, nombre: &texto) -> flotante {
    variable indice = indice_de_columna_en_tabla(t.manejador, nombre)
    si (indice < 0) {
        retornar 0.0
    }
    retornar numero_de_tabla(t.manejador, fila, indice)
}

// Un entero por el nombre de su columna.
publico funcion entero_por_nombre(t: Tabla, fila: entero, nombre: &texto) -> entero {
    variable indice = indice_de_columna_en_tabla(t.manejador, nombre)
    si (indice < 0) {
        retornar 0
    }
    retornar entero_de_tabla(t.manejador, fila, indice)
}

// Guarda la tabla en un archivo CSV.
publico funcion guardar(t: Tabla, ruta: &texto) -> booleano {
    variable contenido = a_texto(t)
    variable escrito = archivo.escribir(ruta, &contenido)
    coincide (escrito) {
        caso Mal(m) => {
            retornar falso
        }
        caso Bien(contenido) => {
            retornar verdadero
        }
    }
}

// Libera la memoria de la tabla.
publico funcion liberar(t: Tabla) -> vacio {
    liberar_tabla(t.manejador)
}

// La versión del motor, o vacío si no está.
publico funcion version() -> texto {
    retornar version_datos()
}

// Ejecuta la consulta y devuelve las filas en CSV, con cabecera.
publico funcion consultar(consulta: &texto) -> texto {
    retornar consultar_datos(*consulta)
}

// Ejecuta una orden que no devuelve filas (crear, copiar, borrar).
publico funcion ejecutar(consulta: &texto) -> booleano {
    retornar ejecutar_datos(*consulta) == 0
}

// Guarda el resultado de la consulta en un CSV.
publico funcion a_csv(consulta: &texto, destino: &texto) -> booleano {
    variable orden = "COPY (" + *consulta + ") TO '" + *destino + "' (HEADER)" + ""
    retornar ejecutar_datos(orden) == 0
}

// Guarda el resultado en Parquet, que es más rápido y pesa menos.
publico funcion a_parquet(consulta: &texto, destino: &texto) -> booleano {
    variable orden = "COPY (" + *consulta + ") TO '" + *destino + "' (FORMAT PARQUET)" + ""
    retornar ejecutar_datos(orden) == 0
}
)nooki";
    }
    if (modulo == "sistema") {
        return R"(// Biblioteca estándar: hablar con otros programas.
//
// Sirve para usar una herramienta que ya existe —un guion de Python, un
// programa del sistema— sin reescribirla. La orden es una línea de comandos, y
// las comillas de las rutas con espacios son responsabilidad de quien la escribe.
// Lanza la orden y devuelve su código de salida (0 si fue bien).
publico funcion ejecutar(comando: &texto) -> entero {
    retornar ejecutar_orden(*comando)
}

// Lanza la orden y devuelve lo que escribió por la salida estándar.
publico funcion capturar(comando: &texto) -> texto {
    retornar capturar_orden(*comando)
}

// Los argumentos con los que se lanzó el programa, sin su nombre.
publico funcion argumentos() -> Vector<texto> {
    retornar argumentos_programa()
}

// Cuántos argumentos hay.
publico funcion cuantos_argumentos() -> entero {
    retornar cuantos_argumentos_programa()
}

// Un argumento por su posición, empezando en 0. Vacío si no existe.
publico funcion argumento(indice: entero) -> texto {
    retornar argumento_de(indice)
}
)";
    }
    if (modulo == "archivo") {
        return R"(// Biblioteca estándar: leer y escribir archivos.
publico enumeracion lectura {
    Bien(contenido: texto)
    Mal(motivo: texto)
}

publico funcion existe(ruta: &texto) -> booleano {
    retornar existe_archivo(*ruta)
}

publico funcion leer(ruta: &texto) -> lectura {
    si (!existe_archivo(*ruta)) {
        retornar Mal("no existe el archivo")
    }
    variable contenido = leer_archivo(*ruta)
    retornar Bien(contenido)
}

publico funcion escribir(ruta: &texto, contenido: &texto) -> lectura {
    si (!escribir_archivo(*ruta, *contenido)) {
        retornar Mal("no se pudo escribir")
    }
    retornar Bien("")
}

// Añade al final, sin borrar lo que hubiera. Útil para registros.
publico funcion agregar(ruta: &texto, contenido: &texto) -> lectura {
    si (!agregar_archivo(*ruta, *contenido)) {
        retornar Mal("no se pudo añadir")
    }
    retornar Bien("")
}
)";
    }
    if (modulo == "listas") {
        return R"(// Biblioteca estándar: operaciones sobre listas de enteros.
publico funcion suma(numeros: &Vector<entero>) -> entero {
    variable total = 0
    variable i = 0
    mientras (i < numeros.largo()) {
        total = total + numeros.obtener(i)
        i = i + 1
    }
    retornar total
}

publico funcion mayor(numeros: &Vector<entero>) -> entero {
    variable mejor = numeros.obtener(0)
    variable i = 1
    mientras (i < numeros.largo()) {
        si (numeros.obtener(i) > mejor) {
            mejor = numeros.obtener(i)
        }
        i = i + 1
    }
    retornar mejor
}

publico funcion menor(numeros: &Vector<entero>) -> entero {
    variable peor = numeros.obtener(0)
    variable i = 1
    mientras (i < numeros.largo()) {
        si (numeros.obtener(i) < peor) {
            peor = numeros.obtener(i)
        }
        i = i + 1
    }
    retornar peor
}

publico funcion contiene(numeros: &Vector<entero>, buscado: entero) -> booleano {
    variable i = 0
    mientras (i < numeros.largo()) {
        si (numeros.obtener(i) == buscado) {
            retornar verdadero
        }
        i = i + 1
    }
    retornar falso
}

publico funcion promedio(numeros: &Vector<entero>) -> entero {
    si (numeros.largo() == 0) {
        retornar 0
    }
    variable total = 0
    variable i = 0
    mientras (i < numeros.largo()) {
        total = total + numeros.obtener(i)
        i = i + 1
    }
    retornar total / numeros.largo()
}

)";
    }
    if (modulo == "fechas") {
        return R"(// Biblioteca estándar: aritmética de calendario.
publico funcion bisiesto(anio: entero) -> booleano {
    si (anio % 400 == 0) {
        retornar verdadero
    }
    si (anio % 100 == 0) {
        retornar falso
    }
    retornar anio % 4 == 0
}

publico funcion dias_del_mes(mes: entero, anio: entero) -> entero {
    si (mes == 2) {
        si (bisiesto(anio)) {
            retornar 29
        }
        retornar 28
    }
    si (mes == 4 || mes == 6 || mes == 9 || mes == 11) {
        retornar 30
    }
    retornar 31
}

publico funcion dias_del_anio(anio: entero) -> entero {
    si (bisiesto(anio)) {
        retornar 366
    }
    retornar 365
}

// Una fecha se representa como días desde el 1 de enero de 1970.
publico funcion a_dias(anio: entero, mes: entero, dia: entero) -> entero {
    variable total = 0
    variable a = 1970
    mientras (a < anio) {
        total = total + dias_del_anio(a)
        a = a + 1
    }
    variable m = 1
    mientras (m < mes) {
        total = total + dias_del_mes(m, anio)
        m = m + 1
    }
    retornar total + dia - 1
}

publico funcion a_anio(dias: entero) -> entero {
    variable anio = 1970
    variable resto = dias
    mientras (resto >= dias_del_anio(anio)) {
        resto = resto - dias_del_anio(anio)
        anio = anio + 1
    }
    retornar anio
}

publico funcion a_mes(dias: entero) -> entero {
    variable anio = a_anio(dias)
    variable resto = dias - a_dias(anio, 1, 1)
    variable mes = 1
    mientras (resto >= dias_del_mes(mes, anio)) {
        resto = resto - dias_del_mes(mes, anio)
        mes = mes + 1
    }
    retornar mes
}

publico funcion a_dia(dias: entero) -> entero {
    retornar dias - a_dias(a_anio(dias), a_mes(dias), 1) + 1
}

publico funcion sumar_dias(dias: entero, cantidad: entero) -> entero {
    retornar dias + cantidad
}

// 0 = lunes … 6 = domingo. El 1 de enero de 1970 fue jueves.
publico funcion dia_de_la_semana(dias: entero) -> entero {
    retornar (dias + 3) % 7
}
)";
    }
    if (modulo == "matematicas") {
        return R"(// Biblioteca estándar: operaciones matemáticas básicas.
publico funcion maximo(a: entero, b: entero) -> entero {
    si (a > b) {
        retornar a
    }
    retornar b
}

publico funcion minimo(a: entero, b: entero) -> entero {
    si (a < b) {
        retornar a
    }
    retornar b
}

publico funcion absoluto(n: entero) -> entero {
    si (n < 0) {
        retornar 0 - n
    }
    retornar n
}

publico funcion cuadrado(n: entero) -> entero {
    retornar n * n
}
)";
    }
    return {};
}

/// Carga un módulo, marca sus funciones con su nombre y añade también los suyos.
bool cargar_modulo(const std::filesystem::path& directorio, std::string_view nombre,
                   diagnostico::GestorFuentes& fuentes, diagnostico::Emisor& emisor,
                   std::vector<std::string>& cargados, sintactico::Programa& destino) {
    const std::string clave(nombre);
    if (std::find(cargados.begin(), cargados.end(), clave) != cargados.end()) {
        return true;  // ya cargado: un ciclo no duplica el módulo
    }
    cargados.push_back(clave);
    // Los módulos que hablan con el sistema necesitan la capa C de UI.
    if (clave == "ventana" || clave == "imagenes") {
        hace_falta_ui = true;
    }
    if (clave == "tensor" || clave == "tabla" || clave == "autograd" || clave == "redes") {
        hace_falta_ia = true;
    }

    // Se busca junto al archivo que importa, en su carpeta «biblioteca» y
    // subiendo por los directorios padre hasta encontrar una.
    // Se parte de una ruta absoluta para poder subir por los directorios padre.
    std::error_code error_ruta;
    const std::filesystem::path base = directorio.empty()
                                           ? std::filesystem::current_path(error_ruta)
                                           : std::filesystem::absolute(directorio, error_ruta);
    std::filesystem::path ruta = base / (clave + ".oki");

    // Las dependencias del manifiesto se miran antes de subir por los padres.
    for (const std::filesystem::path& extra : rutas_de_modulos) {
        const std::filesystem::path candidato = extra / (clave + ".oki");
        if (std::filesystem::exists(candidato)) {
            ruta = candidato;
            break;
        }
    }
    for (std::filesystem::path actual = base; !std::filesystem::exists(ruta);) {
        const std::filesystem::path candidato = actual / "biblioteca" / (clave + ".oki");
        if (std::filesystem::exists(candidato)) {
            ruta = candidato;
            break;
        }
        if (!actual.has_parent_path() || actual.parent_path() == actual) {
            break;
        }
        actual = actual.parent_path();
    }

    // Un archivo no puede importarse a sí mismo: ese candidato se ignora y se
    // sigue buscando (puede haber un módulo de la biblioteca con ese nombre).
    if (!archivo_de_entrada.empty() && std::filesystem::exists(ruta)) {
        std::error_code error_canonico;
        const std::filesystem::path modulo_real =
            std::filesystem::weakly_canonical(ruta, error_canonico);
        if (!error_canonico && modulo_real == archivo_de_entrada) {
            for (const std::filesystem::path& extra : rutas_de_modulos) {
                const std::filesystem::path candidato = extra / (clave + ".oki");
                if (std::filesystem::exists(candidato)) {
                    ruta = candidato;
                    break;
                }
            }
            if (std::filesystem::weakly_canonical(ruta, error_canonico) == archivo_de_entrada) {
                // Se deja sin resolver para que se use el módulo embebido.
                ruta = std::filesystem::temp_directory_path() /
                       ("nooki_inexistente_" + clave + ".oki");
            }
        }
    }

    std::string contenido = leer_archivo(ruta);
    if (contenido.empty() && !std::filesystem::exists(ruta)) {
        // Los módulos de la biblioteca viajan dentro del compilador.
        const std::string_view integrado = fuente_biblioteca(clave);
        if (integrado.empty()) {
            emisor.error(diagnostico::codigos::MODULO_NO_ENCONTRADO, diagnostico::Span{})
                .parametro("modulo", clave)
                .ayuda("se buscó «" + ruta_a_utf8(ruta) + "»");
            return false;
        }
        contenido = std::string(integrado);
    }

    const auto archivo = fuentes.anadir(ruta_a_utf8(ruta), contenido);
    lexico::Lexer lexer(fuentes, archivo, emisor);
    const auto tokens = lexer.todos();
    sintactico::Parser parser(fuentes, archivo, emisor, tokens);
    sintactico::Programa sub = parser.analizar();
    if (emisor.hay_errores()) {
        return false;
    }

    // Un módulo es una biblioteca: no tiene punto de entrada.
    for (const sintactico::Funcion& funcion : sub.funciones) {
        if (funcion.nombre == "principal") {
            emisor.error(diagnostico::codigos::TOKEN_INESPERADO, funcion.span)
                .parametro("esperado", "un módulo sin «principal»")
                .parametro("encontrado", std::string(clave) + ".principal")
                .ayuda("un módulo es una biblioteca: quite «principal» de «" + std::string(clave) +
                       ".oki»");
            return false;
        }
    }

    for (const std::string_view uso : sub.usos) {
        if (!cargar_modulo(directorio, uso, fuentes, emisor, cargados, destino)) {
            return false;
        }
    }

    for (sintactico::Funcion& funcion : sub.funciones) {
        funcion.modulo = nombre;
        destino.funciones.push_back(std::move(funcion));
    }
    for (sintactico::DefinicionTipo& tipo : sub.tipos) {
        tipo.modulo = nombre;
        destino.tipos.push_back(std::move(tipo));
    }
    for (sintactico::Enumeracion& enumeracion : sub.enumeraciones) {
        enumeracion.modulo = nombre;
        destino.enumeraciones.push_back(std::move(enumeracion));
    }
    return true;
}

bool existe_compilador(const std::string& candidato) {
    const std::string comando =
        entrecomillar_si_hace_falta(candidato) + " --version > " + NULO + " 2>&1";
    return ejecutar_comando(comando) == 0;
}

std::string leer_archivo(const std::filesystem::path& ruta) {
    std::ifstream entrada(ruta, std::ios::binary);
    if (!entrada) {
        return {};
    }
    std::ostringstream contenido;
    contenido << entrada.rdbuf();
    return contenido.str();
}

/// Sustituye por «_» todo byte que no sea ASCII imprimible.
std::string nombre_ascii(const std::string& nombre) {
    std::string salida;
    salida.reserve(nombre.size());
    for (const unsigned char caracter : nombre) {
        salida += (caracter >= 0x20 && caracter < 0x7F) ? static_cast<char>(caracter) : '_';
    }
    if (salida.empty()) {
        salida = "programa";
    }
    return salida;
}

/// ¿Todos los bytes de la ruta son ASCII imprimible?
bool ruta_es_ascii(const std::filesystem::path& ruta) {
    for (const unsigned char caracter : ruta_a_utf8(ruta)) {
        if (caracter >= 0x80) {
            return false;
        }
    }
    return true;
}

std::filesystem::path ruta_de_salida(const Opciones& opciones) {
    if (!opciones.salida.empty()) {
        return opciones.salida;
    }
    std::filesystem::path salida = opciones.fuente;
    salida.replace_extension(EXTENSION_BINARIO);
    return salida;
}

}  // namespace

std::string compilador_c() {
    const std::string preferido = leer_entorno("NOOKI_CC");
    if (!preferido.empty() && existe_compilador(preferido)) {
        return preferido;
    }
    for (const char* candidato : {"cc", "gcc", "clang"}) {
        if (existe_compilador(candidato)) {
            return candidato;
        }
    }
    return {};
}

Resultado compilar(const Opciones& opciones, diagnostico::GestorFuentes& fuentes,
                   diagnostico::Emisor& emisor, std::ostream& salida, std::ostream& error) {
    Resultado resultado;

    const std::string contenido = leer_archivo(opciones.fuente);
    if (contenido.empty() && !std::filesystem::exists(opciones.fuente)) {
        emisor.error(diagnostico::codigos::ARCHIVO_NO_ENCONTRADO, diagnostico::Span{})
            .parametro("archivo", ruta_a_utf8(opciones.fuente));
        return Resultado{1, {}, {}};
    }

    rutas_de_modulos = opciones.rutas_de_modulos;
    std::error_code error_entrada;
    archivo_de_entrada = std::filesystem::weakly_canonical(opciones.fuente, error_entrada);
    if (error_entrada) {
        archivo_de_entrada = opciones.fuente;
    }

    const auto archivo = fuentes.anadir(ruta_a_utf8(opciones.fuente), contenido);

    lexico::Lexer lexer(fuentes, archivo, emisor);
    const auto tokens = lexer.todos();

    sintactico::Parser parser(fuentes, archivo, emisor, tokens);
    sintactico::Programa programa = parser.analizar();

    if (emisor.hay_errores()) {
        return Resultado{1, {}, {}};
    }

    // Módulos: se cargan los que el archivo principal importa, y los suyos.
    std::vector<std::string> cargados;
    const std::filesystem::path directorio = opciones.fuente.parent_path();
    for (const std::string_view uso : programa.usos) {
        if (!cargar_modulo(directorio, uso, fuentes, emisor, cargados, programa)) {
            return Resultado{1, {}, {}};
        }
    }

    // Verificación semántica: nombres, aridad, tipos, condiciones y retornos.
    if (semantico::verificar(programa, emisor) > 0) {
        return Resultado{1, {}, {}};
    }

    // `verificar` se detiene aquí: ya se cargaron los módulos y se comprobó todo,
    // pero no hace falta generar C ni llamar al compilador.
    if (opciones.solo_verificar) {
        return resultado;
    }

    const generador::ResultadoGeneracion generado = generador::generar_c(programa);
    if (!generado.soportado) {
        emisor.error(diagnostico::codigos::BACKEND_NO_SOPORTA, diagnostico::Span{})
            .parametro("construccion", generado.motivo)
            .ayuda("el backend C solo traduce llamadas, textos, enteros e identificadores");
        return Resultado{1, generado.codigo, {}};
    }
    resultado.c_generado = generado.codigo;

    if (opciones.solo_c) {
        salida << resultado.c_generado;
        return resultado;
    }

    const std::string compilador = compilador_c();
    if (compilador.empty()) {
        emisor.error(diagnostico::codigos::SIN_COMPILADOR_C, diagnostico::Span{});
        return Resultado{1, resultado.c_generado, {}};
    }

    // El archivo intermedio lleva un nombre ASCII: la consola y el compilador de C
    // no tratan igual los nombres con tildes en todas las plataformas, y el
    // contenido ya identifica el programa por su ruta en los diagnósticos.
    const std::filesystem::path intermedio =
        std::filesystem::temp_directory_path() /
        ("nooki_" + nombre_ascii(ruta_a_utf8(opciones.fuente.stem())) + ".c");

    {
        std::ofstream archivo_c(intermedio, std::ios::binary | std::ios::trunc);
        archivo_c << resultado.c_generado;
    }

    const std::filesystem::path salida_binario = ruta_de_salida(opciones);
    // El enlazador de MinGW no abre rutas con tildes: se enlaza en una ruta ASCII
    // y el binario se mueve después, que sí entiende Unicode.
    std::error_code error_mover;
    const bool necesita_mover =
        !ruta_es_ascii(salida_binario) && std::filesystem::path(salida_binario).has_parent_path();
    std::filesystem::path destino_enlace = salida_binario;
    if (necesita_mover) {
        destino_enlace = std::filesystem::temp_directory_path() /
                         ("nooki_salida_" + nombre_ascii(ruta_a_utf8(opciones.fuente.stem())) +
                          EXTENSION_BINARIO);
    }

    // La capa de tensores, igual: aparte y solo si se usa.
    std::filesystem::path ia_nativo;
    if (hace_falta_ia) {
        const std::string fuente_ia =
            std::string(CABECERAS_RUNTIME) + std::string(ia::fuente_runtime_ia());
        ia_nativo = std::filesystem::temp_directory_path() /
                    ("nooki_ia_" + nombre_ascii(ruta_a_utf8(opciones.fuente.stem())) + ".c");
        std::ofstream archivo_ia(ia_nativo, std::ios::binary | std::ios::trunc);
        archivo_ia << fuente_ia;
    }

    // La capa de ventanas se compila aparte, solo si el programa usa el módulo.
    std::filesystem::path ui_nativo;
    std::string fuente_ui;
    if (hace_falta_ui) {
        fuente_ui = std::string(CABECERAS_RUNTIME) + std::string(ui::fuente_runtime_ui());
        ui_nativo = std::filesystem::temp_directory_path() /
                    ("nooki_ui_" + nombre_ascii(ruta_a_utf8(opciones.fuente.stem())) + ".c");
        std::ofstream archivo_ui(ui_nativo, std::ios::binary | std::ios::trunc);
        archivo_ui << fuente_ui;
    }

    // La consola del sistema no entiende «/» como separador: se usa el nativo.
    std::filesystem::path binario_nativo = destino_enlace;
    std::filesystem::path intermedio_nativo = intermedio;
    std::string comando =
        entrecomillar_si_hace_falta(compilador) + " -O2 -o " +
        entrecomillar_si_hace_falta(ruta_a_utf8(binario_nativo.make_preferred())) + " " +
        entrecomillar_si_hace_falta(ruta_a_utf8(intermedio_nativo.make_preferred()));
    if (hace_falta_ia) {
        comando += " " + entrecomillar_si_hace_falta(ruta_a_utf8(ia_nativo.make_preferred()));
    }
    if (hace_falta_ui) {
        comando += " " + entrecomillar_si_hace_falta(ruta_a_utf8(ui_nativo.make_preferred()));
        // Las bibliotecas del sistema que necesita la capa de ventanas.
#if defined(_WIN32)
        comando += " -luser32 -lgdi32";
#else
        comando += " -lX11";
#endif
    }

    const int codigo = ejecutar_comando(comando);
    if (codigo != 0) {
        emisor.error(diagnostico::codigos::FALLO_COMPILADOR_C, diagnostico::Span{})
            .parametro("compilador", compilador)
            .parametro("codigo", std::to_string(codigo))
            .nota("C generado en " + ruta_a_utf8(intermedio));
        return Resultado{1, resultado.c_generado, {}};
    }

    // El binario se enlazó en una ruta ASCII si el destino tenía tildes: se mueve.
    if (necesita_mover) {
        std::filesystem::copy_file(destino_enlace, salida_binario,
                                   std::filesystem::copy_options::overwrite_existing, error_mover);
        if (error_mover) {
            emisor.error(diagnostico::codigos::FALLO_COMPILADOR_C, diagnostico::Span{})
                .parametro("compilador", compilador)
                .parametro("codigo", std::to_string(error_mover.value()))
                .nota("no se pudo mover el binario a «" + ruta_a_utf8(salida_binario) + "»");
            return Resultado{1, resultado.c_generado, {}};
        }
        std::error_code ignorado;
        std::filesystem::remove(destino_enlace, ignorado);
    }

    if (opciones.conservar_intermedios) {
        error << "nooki: C intermedio conservado en " << ruta_a_utf8(intermedio) << '\n';
    } else {
        std::error_code ignorado;
        std::filesystem::remove(intermedio, ignorado);
    }

    resultado.binario = salida_binario;
    return resultado;
}

}  // namespace nooki::conductor
