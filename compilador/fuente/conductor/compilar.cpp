#include "nooki/conductor/compilar.hpp"
#include "nooki/rutas.hpp"
#include "nooki/ui/runtime_ui.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <algorithm>
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
    if (clave == "ventana") {
        hace_falta_ui = true;
    }

    // Se busca junto al archivo que importa, en su carpeta «biblioteca» y
    // subiendo por los directorios padre hasta encontrar una.
    // Se parte de una ruta absoluta para poder subir por los directorios padre.
    std::error_code error_ruta;
    const std::filesystem::path base =
        directorio.empty() ? std::filesystem::current_path(error_ruta)
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
                .ayuda("un módulo es una biblioteca: quite «principal» de «" +
                       std::string(clave) + ".oki»");
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
        !ruta_es_ascii(salida_binario) &&
        std::filesystem::path(salida_binario).has_parent_path();
    std::filesystem::path destino_enlace = salida_binario;
    if (necesita_mover) {
        destino_enlace = std::filesystem::temp_directory_path() /
                         ("nooki_salida_" + nombre_ascii(ruta_a_utf8(opciones.fuente.stem())) +
                          EXTENSION_BINARIO);
    }

    // La capa de ventanas se compila aparte, solo si el programa usa el módulo.
    std::filesystem::path ui_nativo;
    std::string fuente_ui;
    if (hace_falta_ui) {
        fuente_ui = std::string(ui::fuente_runtime_ui());
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
                                   std::filesystem::copy_options::overwrite_existing,
                                   error_mover);
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
