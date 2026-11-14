# Nooki 1.0 — notas de la versión

**Un lenguaje compilado, con sintaxis en español, memoria determinista, interfaz gráfica y
herramientas de IA incluidas.**

---

## Qué es la 1.0

La 1.0 significa una cosa concreta: **lo que este documento describe está implementado y probado**.

No es «el lenguaje completo de la visión original» —quedan piezas grandes, como el control de hardware
a bajo nivel y un diseñador visual— sino un lenguaje que hoy sirve para escribir programas de verdad:
de consola, de datos y de escritorio.

Lo que hay, medido:

| | |
|---|---|
| Programas de conformidad | **92**, compilados y ejecutados en cada `ctest` |
| Suites de pruebas | **17** |
| Módulos de biblioteca | **16**, todos escritos en Nooki |
| Comandos | **13** |
| Cadenas verificadas | MinGW, MSVC, Linux GCC y Linux con ASan + UBSan |
| Defectos conocidos | **ninguno** |

## El lenguaje

Sintaxis en español, tipado estático comprobado antes de generar código, y memoria determinista: sin
recolector de basura y sin excepciones.

```aura
usar textos

funcion principal() -> entero {
    variable nombres = Vector<texto>.nuevo()
    nombres.agregar("Ana" + "")
    nombres.agregar("Luis" + "")

    para (nombre en nombres) {
        imprimir("Hola, " + nombre)
    }
    retornar 0
}
```

- **Valores**: `entero`, `flotante` (doble precisión), `booleano`, `texto`.
- **Colecciones**: `Vector<entero>`, `Vector<texto>`, `Mapa<texto, entero>`, `Caja<T>`.
- **Funciones** con tipos, genéricos por instanciación y parámetros prestados.
- **Estructuras** por valor y **clases** por referencia, con herencia, polimorfismo y `super`.
- **Rasgos**: contrato verificado y despacho dinámico con `&Rasgo` entre tipos no emparentados.
- **Enumeraciones con datos** y emparejamiento exhaustivo con `coincide`.
- **Errores como valores**: `resultado` y el operador `?`, sin excepciones.
- **Módulos** con visibilidad de funciones y tipos, y dependencias por `nooki.toml`.
- **FFI**: `externo funcion` para llamar a C, incluidas funciones que devuelven `texto`.
- **`inseguro` y `asm!`** para quien necesita bajar al ensamblador, con las comprobaciones de
  aritmética desactivadas solo dentro del bloque.

## La memoria

El rasgo que más distingue a Nooki:

| Regla | Efecto |
|---|---|
| Dueño único; pasar por valor mueve | no hay copias invisibles |
| `&` presta sin transferir | se lee sin mover |
| `&mutable` presta para escribir | se modifica sin duplicar |
| Asignar una colección mueve y libera el destino | no se pierde memoria |
| Devolver un compuesto transfiere lo que contiene | no queda memoria muerta detrás |
| Copiar un compuesto con dueño es error | no hay doble liberación |
| No se mueve el dueño mientras está prestado | no hay referencias colgando |
| No se devuelve un préstamo a algo local | no escapa lo que muere |
| `romper` y `continuar` liberan | no hay fugas al salir de un bucle |
| Salir de una función libera sus locales | no hay fugas al volver |

**Cuando el compilador no puede garantizarlo, lo rechaza.** Esa es la regla que explica casi todas las
decisiones del lenguaje.

## La biblioteca: 16 módulos

Todos escritos **en Nooki** (la capa de sistema, en C, solo existe donde hace falta: ventanas, tensores
y tablas).

### Datos y textos

| Módulo | Qué trae |
|---|---|
| `resultado` | `Ok(valor)`, `Error(mensaje)` |
| `matematicas` | `maximo`, `minimo`, `absoluto`, `cuadrado` |
| `textos` | `unir`, `repetir`, `contar` |
| `listas` | `suma`, `mayor`, `menor`, `promedio`, `contiene` |
| `fechas` | `bisiesto`, `dias_del_mes`, `a_dias`, `sumar_dias`, `dia_de_la_semana` |
| `archivo` | `existe`, `leer`, `escribir` |
| `pln` | `normalizar`, `tokenizar`, `contar`, `distancia`, `jaccard` |

### Interfaz gráfica

| Módulo | Qué trae |
|---|---|
| `ventana` | abrir, bucle de eventos, dibujo, lectura de píxeles |
| `diseno` | columnas, filas, centrado, reparto |
| `controles` | `etiqueta`, `boton`, `barra`, `campo` de texto |
| `accesibilidad` | contraste legible y foco por teclado |
| `imagenes` | carga de BMP y dibujo |

### IA

| Módulo | Qué trae |
|---|---|
| `tensor` | matrices, álgebra, determinante, estadística, activaciones |
| `tabla` | columnas con nombre, filtros, orden, agrupación — tipo DataFrame |
| `autograd` | grafo de operaciones y gradientes |
| `redes` | red de una capa oculta que aprende |

La red aprende XOR, que es el problema que no se puede resolver sin capa oculta:

```
$ nooki ejecutar ejemplos/…  →  error de 0,261 a 0,0003, y las cuatro combinaciones acertadas
```

## Las herramientas

```bash
nooki nuevo demo        # crea un proyecto que compila
nooki construir         # compila el proyecto del directorio actual
nooki probar            # ejecuta pruebas/*.oki y compara con su .esperado
nooki verificar         # léxico, parser y verificación, sin generar C
nooki formatear         # sangría canónica; --comprobar para la CI
nooki documentar        # genera API.md con lo público del proyecto
nooki lsp               # servidor de lenguaje para el editor
nooki limpiar           # borra artefactos sin tocar el código
nooki compilar, ejecutar, ast, ayuda, version
```

### El servidor de lenguaje

`nooki lsp` habla el protocolo por la entrada y salida estándar:

- **diagnósticos** subrayados en su línea, con el código `NOK` y el mensaje en español;
- **autocompletado** de palabras, tipos y funciones del documento;
- **ir a la definición** de funciones, tipos, enumeraciones, variables y parámetros;
- **formato** del documento entero.

Funciona con cualquier editor que hable LSP; el repositorio trae el resaltado para VS Code y Vim.

## La documentación

- **[Guía](GUIA.md)**: trece capítulos, cada uno con su programa y su salida real.
- **[Interfaz](docs/INTERFAZ.md)**: cómo se hace una aplicación de escritorio, sus cinco trampas y lo
  que todavía no hay.
- **[Especificación](spec/especificacion-v0.1.md)**: 64 secciones, el documento normativo.
- **[Decisiones](DECISIONES.md)**: 26 ADR con el porqué de cada decisión, y los asuntos abiertos.
- **Diagnósticos**: 46 códigos `NOK` con mensaje en español e inglés. 40 tienen una prueba que los
  provoca; los seis restantes necesitan escenarios que no caben en una prueba automática y están
  anotados con su motivo.
- **Ejemplos**: siete programas, de «hola, mundo» a una lista de tareas con ratón y teclado.

## Qué no incluye

Dicho con claridad, para que nadie lo busque:

| Pendiente | Tamaño | Qué daría |
|---|---|---|
| Ventanas en macOS | medio | el backend Cocoa; hoy los programas compilan pero no abren ventana |
| Imágenes PNG | medio | un decodificador entero, o una dependencia |
| Cursor y selección en el campo de texto | pequeño | editar en medio de lo escrito |
| Formato de números en `texto` | pequeño | `"" + 42` |
| Diseñador visual de formularios | grande | arrastrar controles y generar código |
| Control de hardware a bajo nivel | grande | registros, interrupciones, dispositivos |
| Tiempos de vida completos | medio | cerrar el último rincón teórico de la memoria |

Ninguna es un defecto: son fases que no se han hecho.

## Para quién es

Para quien quiera **escribir en español** sin renunciar a un lenguaje compilado, tipado y con control
de memoria. Hoy sirve para programas de consola, procesamiento de texto y datos, aplicaciones de
escritorio sencillas y experimentos de aprendizaje automático.

Y para quien quiera **ver cómo se construye un lenguaje**: el compilador, sus 17 suites, sus 26
decisiones y sus 107 commits están a la vista.

## Cómo empezar

```bash
$ nooki nuevo demo
$ cd demo
$ nooki construir
$ ./principal
Hola, Nooki
```

Y después, la [guía](GUIA.md). Si quieres ver algo con ventana:

```bash
$ nooki ejecutar ejemplos/06-tareas/principal.oki
```

## Licencia

MIT o Apache-2.0, a elegir. Ver [LICENCIA](LICENCIA.md).
