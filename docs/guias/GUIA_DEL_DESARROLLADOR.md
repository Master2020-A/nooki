# Guía del desarrollador de Nooki

**Cómo está construido el compilador, cómo se prueba y cómo se extiende sin romperlo.**

| | |
|---|---|
| Versión documentada | **Nooki 1.0.0** (edición 2026) |
| Lenguaje de implementación | C++20, sin dependencias externas |
| Backend | generación de C y compilador de C del sistema (ADR-014) |
| Pruebas | 18 suites de `ctest` y 99 casos de conformidad en esta copia |
| Audiencia | quien va a leer, modificar o extender el compilador |

> El `README.md` de la raíz conserva texto de la fase de diseño y está desactualizado.
> **La fuente de verdad es `compilador/`, `spec/`, `pruebas/` y este documento.** Si algo
> discrepa, manda el código; si el código discrepa de la especificación, es un fallo o una
> característica pendiente, y se documenta como tal.

---

## Índice

1. [Requisitos y primer build](#1-requisitos-y-primer-build)
2. [Mapa del repositorio](#2-mapa-del-repositorio)
3. [Arquitectura](#3-arquitectura)
4. [Modelo de datos](#4-modelo-de-datos)
5. [Flujo de compilación](#5-flujo-de-compilación)
6. [Cómo extender el compilador](#6-cómo-extender-el-compilador)
7. [Pruebas](#7-pruebas)
8. [Estilo y revisión](#8-estilo-y-revisión)
9. [Depuración](#9-depuración)
10. [Publicación](#10-publicación)
11. [Deuda técnica y hoja de ruta](#11-deuda-técnica-y-hoja-de-ruta)
12. [Apéndices](#12-apéndices)

---

## 1. Requisitos y primer build

### 1.1 Herramientas

- **CMake 3.25+**.
- Un compilador de **C++20**: MSVC 2022+, GCC 12+ o Clang 15+.
- Un compilador de **C** para el backend en tiempo de ejecución: `cc`, `gcc` o `clang`.
  En Windows, MinGW o Clang; `cl.exe` no se sondea.
- En Linux, `libx11-dev` si vas a compilar o probar programas con ventana.

No hay dependencias de terceros: solo la biblioteca estándar de C++.

### 1.2 Build

```bash
cmake --preset windows-mingw      # o linux, macos, windows-msvc, windows-msvc-2026
cmake --build --preset windows-mingw
ctest --preset windows-mingw
```

El binario queda en:

| Preset | Ruta |
|---|---|
| `windows-mingw` | `build/windows-mingw/compilador/nooki.exe` |
| `windows-msvc` | `build/windows-msvc/compilador/Release/nooki.exe` |
| `windows-msvc-2026` | `build/windows-msvc-2026/compilador/Release/nooki.exe` |
| `linux` | `build/linux/compilador/nooki` |
| `macos` | `build/macos/compilador/nooki` |
| `saneamiento` | `build/saneamiento/compilador/nooki` |

```bash
build/windows-mingw/compilador/nooki.exe version
build/windows-mingw/compilador/nooki.exe ejecutar pruebas/conformidad/valido/0000-hola-mundo/principal.oki
```

### 1.3 Opciones de CMake

| Opción | Por defecto | Efecto |
|---|---|---|
| `NOOKI_PRUEBAS` | `ON` | construye las suites y registra `ctest` |
| `NOOKI_SANEAMIENTO` | `OFF` | ASan + UBSan (no MSVC) |
| `NOOKI_AVISOS_COMO_ERRORES` | `OFF` | `-Werror` / `/WX` |
| `NOOKI_EDICION` | `2026` | edición que anuncia `nooki version` |

La versión se define en `CMakeLists.txt` (`project(Nooki VERSION 1.0.0)`), no en el código.
`compilador/incluir/nooki/version.hpp.in` se configura a
`build/<preset>/generado/nooki/version.hpp`.

### 1.4 Pruebas rápidas

```bash
ctest --preset windows-mingw -N                 # lista las suites
ctest --preset windows-mingw -R cli             # una suite
ctest --preset windows-mingw --output-on-failure
```

Para el arnés de conformidad directamente:

```bash
build/windows-mingw/compilador/nooki_conformidad.exe --directorio=pruebas/conformidad --detalle
```

---

## 2. Mapa del repositorio

```text
compilador/
├── fuente/
│   ├── principal.cpp          punto de entrada; argumentos UTF-8 en Windows
│   ├── utf8.cpp               decodificación y utilidades UTF-8
│   ├── entorno.cpp            lectura de variables de entorno
│   ├── proceso.cpp            ejecución de comandos (UTF-16 en Windows)
│   ├── manifiesto.cpp         parser de nooki.toml
│   ├── lexico/
│   │   ├── token.cpp          nombres de tokens
│   │   └── lexer.cpp          análisis léxico, palabras reservadas, literales
│   ├── sintactico/
│   │   ├── parser.cpp         descenso recursivo + Pratt para expresiones
│   │   ├── ast.hpp            nodos del árbol
│   │   └── impresor.cpp       impresión canónica S-expression
│   ├── semantico/
│   │   ├── tipos.cpp          tipo básico y nombres
│   │   └── verificaciones.cpp nombres, tipos, aridad, propiedad, préstamos
│   ├── generador/
│   │   └── generador_c.cpp    traducción a C y runtime embebido
│   ├── conductor/
│   │   └── compilar.cpp       orquesta fases, módulos, backend y temporales
│   ├── diagnostico/           códigos, catálogo, emisor, render texto/JSON
│   ├── formato/
│   │   └── formateador.cpp    sangría canónica
│   ├── lsp/
│   │   ├── json.cpp           JSON mínimo
│   │   └── servidor.cpp       JSON-RPC LSP por stdio
│   ├── cli/                   comandos y sus implementaciones
│   ├── ui/runtime_ui.cpp      runtime C de ventanas (GDI/X11), en literales
│   └── ia/runtime_ia.cpp      runtime C de tensores/redes, en literales
├── incluir/nooki/             cabeceras públicas del núcleo
├── pruebas/                   suites unitarias y arneses
└── CMakeLists.txt

pruebas/
├── conformidad/valido/        programas que compilan y se ejecutan
├── conformidad/invalido/      programas que deben emitir códigos concretos
└── LEEME.md

spec/                          especificación y gramática EBNF
docs/
├── adr/                       26 decisiones de arquitectura
├── referencia/                tablas generadas (palabras, diagnósticos, biblioteca)
├── PROYECTO.md                nooki.toml y dependencias
├── INTERFAZ.md                guía de UI
└── guias/                     estas guías

ejemplos/                      ocho programas ejecutables
editores/                      resaltado para VS Code, Vim, JetBrains, TextMate
herramientas/                  empaquetado, instalación, generadores, capturas
```

### Dónde mirar primero

| Buscas | Archivo |
|---|---|
| Cómo se lexa un token | `compilador/fuente/lexico/lexer.cpp` |
| Cómo se parsea una expresión | `compilador/fuente/sintactico/parser.cpp` |
| Qué forma tiene el AST | `compilador/incluir/nooki/sintactico/ast.hpp` |
| Dónde se comprueban tipos y propiedad | `compilador/fuente/semantico/verificaciones.cpp` |
| Cómo se genera C | `compilador/fuente/generador/generador_c.cpp` |
| Cómo se cargan módulos y se llama al backend | `compilador/fuente/conductor/compilar.cpp` |
| Cómo se define un diagnóstico | `compilador/incluir/nooki/diagnostico/codigos.hpp` y `catalogo.cpp` |
| Cómo se añade un comando | `compilador/incluir/nooki/cli/comandos.hpp` y `cli/orden.cpp` |
| Cómo se sirve LSP | `compilador/fuente/lsp/servidor.cpp` |
| Cómo se formatea | `compilador/fuente/formato/formateador.cpp` |

---

## 3. Arquitectura

### 3.1 Vista general

```text
              ┌────────────┐
  argumentos →│    CLI     │→ comandos (compilar, construir, ejecutar, lsp, …)
              └─────┬──────┘
                    ↓
              ┌────────────┐
  .oki ──────→│  conductor │→ carga módulos, orquesta fases
              └─────┬──────┘
                    ↓
   ┌──────────┬─────┴─────┬──────────────┬──────────────┐
   ↓          ↓           ↓              ↓              ↓
 lexer → parser → semántica → generador C → compilador C → binario
   │          │           │              │
   └──────────┴───────────┴──────────────┘
                    ↓
              diagnósticos (texto/JSON)
```

El compilador es una **biblioteca estática** (`nooki_nucleo`) más un ejecutable delgado
(`nooki`). Las suites de pruebas y el LSP enlazan la misma biblioteca.

### 3.2 Fases

| Fase | Archivo | Responsabilidad |
|---|---|---|
| Léxico | `lexico/lexer.cpp` | tokens, palabras reservadas, literales, comentarios, errores NOK1xxx |
| Sintáctico | `sintactico/parser.cpp` | AST, precedencia Pratt, sentencias, declaraciones, errores NOK2xxx |
| Impresión | `sintactico/impresor.cpp` | forma canónica del AST para pruebas y `nooki ast` |
| Semántica | `semantico/verificaciones.cpp` | nombres, tipos, aridad, retornos, `coincide`, propiedad y préstamos |
| Generación | `generador/generador_c.cpp` | C autónomo con runtime embebido; errores NOK6xxx |
| Conducción | `conductor/compilar.cpp` | módulos, manifiesto, backend, temporales, binario |
| Diagnóstico | `diagnostico/*` | códigos, catálogo es/en, spans, render texto/JSON |
| CLI | `cli/*` | despacho, ayuda, comandos de proyecto |
| LSP | `lsp/servidor.cpp` | JSON-RPC, diagnósticos, completion, definition, formatting |
| Formato | `formato/formateador.cpp` | sangría y espacios finales |
| Runtime UI | `ui/runtime_ui.cpp` | C para GDI (Windows) y X11 (Linux); no macOS |
| Runtime IA | `ia/runtime_ia.cpp` | C para tensores, tablas, autograd y redes |

### 3.3 Decisiones de arquitectura que explican el código

| ADR | Decisión | Consecuencia práctica |
|---|---|---|
| ADR-014 | Backend C en lugar de LLVM | el compilador invoca `cc`/`gcc`/`clang`; el C es el IR |
| ADR-015/021 | estructuras por valor, clases por referencia | dos representaciones distintas en C |
| ADR-016/018/020 | caja, texto con dueño, regla única de propiedad | el generador inserta liberaciones y comprobaciones |
| ADR-019 | genéricos por instanciación | `manglear` genera una versión por combinación |
| ADR-022 | herencia y despacho | tablas virtuales y `nooki_ref_<rasgo>` |
| ADR-023/024 | errores como valores y variantes con datos | `resultado`, `coincide`, `?` |
| ADR-025 | biblioteca en Nooki | los 16 módulos son texto embebido, no C |

## Memoria del código generado

El preset `saneamiento` instrumenta el **compilador**. Las fugas que importan están en el **C que el
compilador produce**, y para eso está `herramientas/verificar-memoria.sh`:

```bash
./herramientas/verificar-memoria.sh programa.oki
```

Compila el C con AddressSanitizer, lo ejecuta y dice de qué función viene cada fuga. Hoy hay **dos clases
conocidas**, y las dos están medidas:

### 1. Arreglada: `imprimir` de un temporal

`imprimir(f())` o `imprimir("a" + b)` dejaba el texto sin liberar. Ahora esas llamadas usan
`nooki_imprimir_texto_temporal`, que imprime y suelta. Los literales no pasan por ahí: no tienen dueño.

### 2. Arreglada: los parámetros con dueño no se liberaban

Una función que recibe un `texto` por valor **no lo liberaba al salir**: una fuga por llamada.

La corrección no era solo registrar el parámetro para liberarlo —eso, por sí solo, producía un **doble
libre**, porque el llamante también liberaba la variable que había pasado—. Hacía falta la otra mitad:
**quien recibe el valor pasa a ser su dueño**, y el llamante deja de liberarlo.

Esa mitad existía, pero solo para las llamadas que eran **sentencia**. Una llamada anidada
(`imprimir(unir(&p, separador))`) no movía nada, así que el mismo valor tenía dos dueños. En vez de emitir el
vaciado dentro de la expresión —que necesitaría temporales y un operador coma—, el generador ahora **quita el
nombre de la lista de lo que hay que liberar** en cuanto se pasa a un parámetro que no es préstamo. Sin código
emitido, sin fugas y sin dobles libres.

Comprobado con el caso que antes rompía (`0051-api-biblioteca`, `attempting double-free` en
`nooki_probar_textos`) y con `verificar-memoria.sh`: el programa de prueba pasó de **4 fugas** a **ninguna**.

### 2 bis. Arreglado: aportar un campo con dueño a un compuesto

`ficha { nombre: ana }` **mueve** `ana` dentro del compuesto: el generador vacía la variable. Pero el
verificador no la marcaba como movida, así que usarla después compilaba y leía memoria que ya no era suya.

Ahora se marca, en un pase único después de tipar (tipar dentro del tipo del compuesto daba un falso positivo,
porque un nodo se tipa más de una vez). El diagnóstico señala la **segunda** línea, que es la que sobra:

```nooki
variable ana = "Ana" + ""
variable f = ficha { nombre: ana, edad: 30 }
imprimir(ana)   // NOK5001: «ana» se movió antes
```

Esto destapó cuatro programas **míos** que hacían justo eso —dos casos de conformidad y dos ejemplos del
manual— y que "funcionaban" porque el compuesto temporal nunca se liberaba. Corregidos: ahora cada compuesto se
construye con su propio valor, y lo que se quiere mirar se mira antes de moverlo.

De paso salió un fallo del generador: `lista.obtener(1).miembros.obtener(0)` emitía `&llamada().miembros`, y un
campo de una llamada no es un sitio. Ahora la comprobación de «¿es un sitio?» es recursiva: un campo lo es si
lo es su base.

### 2 ter. Arreglado: el compuesto temporal pasado a `agregar`

El misterio del doble intento fallido: la construcción **copiaba** el campo, no lo movía. El compuesto y la
variable apuntaban al mismo búfer, y liberar los dos reventaba. Se veía en el C generado:

```c
nooki_vector_equipo_agregar(&lista, (nooki_equipo){ .nombre = ..., .miembros = e1 });
/* y ningún `e1.datos = NULL` después */
```

Ahora el generador emite el vaciado del origen (`emitir_movimientos_de_compuesto`), que es lo que el
verificador ya daba por hecho desde la 1.0.32. Con eso, la variante `_agregar_temporal` —que agrega y libera el
temporal— **funciona**: el caso `0105` pasa de fugas a limpio.

De paso salieron dos fallos más: `anular_variable` vaciaba un mapa con los campos del vector (`datos` en vez de
`cubetas`), y la copia de un mapa copiaba las claves dos veces —`insertar` ya guarda copias—, y la de más se
perdía.

### 3 bis. Arreglado: los textos temporales de `insertar`

`mapa.insertar("clave" + "", valor)` dejaba el texto del literal sin liberar. Ahora, cuando la clave (o el
valor, en el mapa de texto) es una **concatenación o una llamada** —lo único que produce un texto con dueño—,
se llama a una variante que inserta y suelta. El filtro es estricto a propósito: una variable llega como
`(*&nombre)` y un préstamo no se toca, que es justo lo que confundía el primer intento.

La batería pasó de **535 a 509 bytes** y el caso `0113` queda limpio.

### 3 ter. Pendiente: los locales de una estructura con campos con dueño

`variable f = c.sacar()` con `c` una celda de `ficha` deja sin liberar la copia: los locales de tipo estructura
**no entran en la lista de lo que hay que liberar**. Ponerlos es tentador, pero `Vector.obtener` devuelve una
**vista** y su `_vista` solo neutraliza los campos `texto` (`propio = 0`), no los `Vector`, `Mapa` ni `Caja`:
liberar una vista de esos liberaría lo que es del vector.

Lo que falta: que `_vista` neutralice **todos** los campos con dueño (`datos = NULL` en el vector,
`cubetas = NULL` en el mapa, `datos = NULL` en la caja) y después registrar los locales de estructura como
liberables. Son **16 bytes** en el caso `0115`.

### 3. Arreglado: el `resultado` no se liberaba

`variable malo: resultado = Error("algo fallo" + "")` dejaba el texto del error sin liberar: un `resultado` no
entraba en la lista de lo que hay que liberar. Ahora los `resultado` **frescos** —los que vienen de una llamada
o de una construcción— se sueltan con `nooki_resultado_soltar`, que ya existía en el preludio.

Comprobado con ASan en un programa mínimo y en el caso `0116-resultado-libera`.

### 3 bis. Pendiente: los `resultado` temporales de un `coincide`

`coincide (archivo.leer(&ruta)) { ... }` no asigna el `resultado` a ninguna variable, así que no hay dónde
liberarlo: queda su texto. Es la misma forma que la de los compuestos temporales, pero para `resultado`. Se ve
en `0049-archivo` (20 bytes) y `0072-csv` (94 bytes tras `csv_filas`).

### 3 ter. Arreglado (casi): `algo().largo()` con un vector

`csv.campos(&sola).largo()` construía un vector **temporal** que nunca se liberaba: el patrón de la
comparación con literales compuestos. Ahora cada tipo de vector —los tres genéricos y uno por estructura— tiene
su `<vec>_largo_temporal`, que mide y suelta, y el generador lo usa cuando el receptor de `largo` es una
llamada.

`0107-csv` pasó de **203 bytes a 6**. Los 6 que quedan son otra cosa: los textos de los campos que devuelve
`csv.campos` (tres concatenaciones de 2 bytes).

### 3 quater. Pendiente: lo que lee `csv.filas`

`0072-csv`, 94 bytes: dentro del módulo se lee el archivo y se copian sus líneas al vector, y el texto
intermedio no se libera. El `lectura` local ya se suelta; la fuga está en el módulo, no en el llamante.

### 3 quater. Pendiente: `sacar()` de una celda de texto

`0104-compartido-texto`, 44 bytes: cada `sacar()` devuelve una copia del texto y esa copia no se libera.

### Arreglado: el `retornar` final no liberaba

`liberar_cajas` **vaciaba** la lista de lo que hay que liberar en cuanto emitía las liberaciones. Así que un
`retornar` dentro de un `si` se llevaba por delante la lista y el `retornar` final del cuerpo ya no liberaba
nada:

```nooki
si (union_total == 0) {
    retornar 1000          // aquí sí liberaba: trozos_a y trozos_b
}
retornar en_comun * 1000 / union_total   // aquí no: se perdían los dos vectores
```

Ahora no se vacía: cada camino de salida emite sus propias liberaciones, y el código que queda detrás de un
`return` es inalcanzable.

`0058-pln` pasó de **427 a 107 bytes**. De paso desaparecieron las de `0072-csv` (94) y bajaron varias más: la
batería, sumando lo que queda, está en **346 bytes** en 21 casos.

### El otro arreglo: encadenar llamadas de texto

En el módulo `pln`, `(*frase).minusculas().recortar()` dejaba el texto intermedio sin dueño: es un temporal
pasado por valor a una función de texto, que lo copia y no lo libera. Se arregló en el **módulo**, en dos pasos:
un local para cada llamada.

### Arreglado: el cierre no liberaba sus parámetros

Un cierre con un parámetro con dueño (`funcion(v: texto) -> texto => "hola " + v`) recibía el texto y no lo
liberaba nunca. Ahora el cuerpo se emite con un temporal, se sueltan los parámetros y se devuelve el temporal.

Dos tropiezos por el camino, los dos por mirar donde no había que mirar: `tipos_lambda` viene **vacío** (hubo
que resolver el tipo por el nombre ya declarado en C), y `recoger_devueltos` recoge **todos** los identificadores
del cuerpo, no el devuelto, así que `"hola " + v` conservaba `v` y no se liberaba nada.

`0104-compartido-texto` pasó de **44 a 5 bytes**.

### Arreglado: `poner` de una celda de texto

`celda.poner(<llamada>)` recibía el texto por valor y guardaba una copia sin soltar el original. Ahora existe
`nooki_compartido_texto_poner_temporal`, que pone y suelta, y el generador la usa cuando el valor es una
llamada.

### Arreglado: el valor de un `coincide` no se liberaba

`coincide (dividir(1, 0)) { caso Error(m) => ... }` construye un `nooki_resultado` temporal con el texto del
error y **no lo liberaba**: el texto se quedaba sin dueño.

Ahora se suelta al terminar… pero solo cuando el valor examinado es un **temporal** (una llamada o una
construcción). Si viene de una variable(`coincide (malo)`), el generador trabaja sobre una **copia superficial**
de la estructura: liberarla soltaría el texto que sigue siendo de la variable. Ese fue el primer intento, y
rompió dos casos con un doble libre.

`0116-resultado-libera` y `0049-archivo` pasan a **limpio**. La batería queda en **250 bytes** en 21 casos.

### Arreglado: las cadenas de `+` de textos

`a + " " + b` se emitía como `concatenar(concatenar(a, " "), b)`: el intermedio se quedaba sin dueño. Y no era
un caso raro —está en todas partes—, así que era la fuga más repartida de la batería.

Ahora, cuando la cadena tiene tres o más piezas, se emite **una** llamada a `nooki_texto_concatenar_n`, que
reserva una vez y copia todas. Dos piezas siguen yendo por `concatenar`.

**`0058-pln` pasó de 107 bytes a limpio**, y la batería de **250 a 87 bytes** en 21 casos.

### Arreglado: qué se conserva al `retornar`

`recoger_devueltos` conservaba **cualquier** identificador que apareciera en la expresión devuelta. Con
`retornar letra == "m"`, `letra` se «conservaba» —o sea, no se liberaba— y la función la perdía.

Ahora solo se conserva lo que **viaja dentro** del valor devuelto: la variable misma (`retornar x`), las que
aportan un campo con dueño a un compuesto (`retornar ficha { nombre: ana }`) y los argumentos de un constructor
de variante (`retornar Ok(x)`). Los operandos de una operación no: `retornar a + b` copia.

El constructor de variante se reconoce por la forma del nombre (empieza por mayúscula), porque el ayudante que
recorre la expresión es estático y no ve la tabla de variantes. Conservar de más solo deja una fuga pequeña;
conservar de menos sería un uso después de liberar, así que el error cae del lado seguro.

**Batería: 87 → 71 bytes.** Y de paso destapó otro programa mío con uso tras mover: `0100-pasar-cierres` pasaba
el mismo cierre dos veces.

## Cobertura de los diagnósticos

Cada código `NOK` debería tener un programa que lo provoque, para que el mensaje no se rompa sin que nadie lo
note. Hoy **31 de 47** tienen caso (eran 14 antes de la 1.0.44); los 37 casos están en
`pruebas/conformidad/invalido/`.

El arnés compara el **conjunto exacto** de códigos, con repeticiones: si un programa emite `NOK2002` cuatro veces
y `NOK2005` una, el `esperado.txt` tiene que decirlo así.

### Los que no lo tienen, y por qué

- **No se pueden probar desde el arnés**: `NOK0001` (archivo ilegible), `NOK1006` (UTF-8 no válido), `NOK6001` (no
  hay compilador de C), `NOK6002` (el compilador de C falla). Son de entorno o de la CLI.
- **No se emiten nunca**, y eso sí es un hallazgo:
  - `NOK2001` (falta cerrar un paréntesis): el parser informa `NOK2002` en su lugar.
  - `NOK2003` (sin `principal`): un programa sin `principal` compila; el error sale del enlazador de C.
  - `NOK2005` (rango encadenado): se emite, pero siempre acompañado de varios `NOK2002`, así que no se puede
    aislar sin fijar el orden de recuperación del parser.
  - `NOK3004` (campo desconocido): los campos se informan con `NOK3006`.
  - `NOK3008` (módulo no encontrado): **`usar` un módulo que no existe no dice nada** — un nombre mal escrito
    pasa en silencio. Es un fallo, no una decisión.
  - `NOK3009` (símbolo privado), `NOK3013` (puntero fuera de `inseguro`), `NOK4009` (escritura en préstamo
    inmutable), `NOK4010` y `NOK4012` (firmas de métodos y rasgos), `NOK5002` (préstamo guardado): no conseguí
    provocarlos con un programa; puede que estén sin conectar o que la sintaxis que probé no sea la suya.

Lo siguiente con esto es decidir, código a código, si se conecta (y se prueba) o si se retira. Un código que
nunca se emite es una promesa que hace `DECISIONES.md` y que el compilador no cumple.

### La barrera: línea base de fugas conocidas

`herramientas/verificar-memoria.sh --estricto` compara cada caso con
`herramientas/memoria-base.txt`, que apunta los que hoy fugan y cuántos bytes. **Falla** si aparece una fuga en
un caso que estaba limpio, si uno de la lista pierde más que antes, o si un caso limpio termina con un código
distinto de cero (eso sí sería un fallo de verdad).

```
$ ./herramientas/verificar-memoria.sh --estricto
0015-textos                        dentro de la base: 12 bytes
...
medidos: 95 · fallos: 0
```

Los casos que no compilan **se saltan**: eso lo mira `ctest`, no esta herramienta (hay casos de conformidad que
son listas de palabras clave y no programas).

Así el trabajo que queda está acotado y no crece por descuido, que es lo que se pedía: las fugas que quedan
están **justificadas por escrito y vigiladas**.


### Estado final de las fugas conocidas

Las **21** que quedan, con sus bytes, tal como están en herramientas/memoria-base.txt (que es lo que vigila
--estricto). Todas están entre 4 y 12 bytes: ninguna es un caso de uso corriente que pierda memoria de forma
apreciable.

| Caso | Fuga |
|---|---|
| `0015-textos` | 0 B |
| `0023-agenda` | 4 B |
| `0029-super` | 9 B |
| `0034-biblioteca` | 6 B |
| `0038-propiedad-retorno` | 9 B |
| `0039-compuestos-dueno` | 6 B |
| `0040-propiedad-saltos` | 0 B |
| `0049-archivo` | 0 B |
| `0051-api-biblioteca` | 0 B |
| `0058-pln` | 0 B |
| `0063-mapa` | 0 B |
| `0072-csv` | 0 B |
| `0076-formato-numeros` | 9 B |
| `0081-prestamo-llamada` | 4 B |
| `0096-textos` | 0 B |
| `0097-metodos-de-texto` | 7 B |
| `0100-pasar-cierres` | 0 B |
| `0104-compartido-texto` | 5 B |
| `0107-csv` | 0 B |
| `0115-celda-ficha` | 12 B |
| `0116-resultado-libera` | 0 B |

**Total: 71 bytes.** Al empezar esta serie eran ~900.

Los patrones que se cerraron por el camino, todos en el generador salvo donde se dice:

1. imprimir(<temporal>) (1.0.27) y los parámetros con dueño que el llamante no vaciaba (1.0.28).
2. Compuestos que se copiaban en vez de moverse, y su temporal pasado a gregar (1.0.32, 1.0.33).
3. insertar/poner con textos temporales (1.0.32, 1.0.39).
4. El etornar final, que no liberaba porque la lista se vaciaba al emitir el primero (1.0.38).
5. Los esultado/lectura y el valor examinado de un coincide (1.0.35, 1.0.40).
6. Los parámetros con dueño de un cierre (1.0.39).
7. Las cadenas de ` + ` de textos, con un intermedio por cada suma (1.0.41).
8. Qué se conserva al etornar: solo lo que viaja dentro del valor (1.0.42).
### 3 quinquies. Nota sobre las medidas

Los totales de la batería **bailan** entre ejecuciones (509, 525, 554 con el mismo código: algún caso
nondeterminista, probablemente por el orden de un mapa). Los datos que sirven son **por caso**, que sí son
estables (`0107`, 203 bytes, dos veces seguidas). El script debería dejar de sumar y pasar a comparar por caso.

### 3. Pendiente: el texto de un `Bien(...)` que liga un patrón

Al desestructurar `coincide (archivo.leer(&ruta)) { caso Bien(leido) => imprimir(leido) }`, el texto que
lleva el `Bien` no se libera. Es una fuga pequeña y localizada en el emparejamiento con datos.

### Cómo medirlo

`verificar-memoria.sh` no falla a propósito: una herramienta que siempre sale en rojo no se usa. Cuando las
dos clases estén cerradas, se le pone el modo estricto y sirve de barrera en la CI.
| ADR-028 | estado compartido | capturarlo prohibido; después `Compartido<T>` con recuento |
| ADR-027 | cierres | función que lleva su entorno, dueña de él y de movimiento |
| ADR-026 | rasgos | contratos verificados y despacho dinámico |

El índice completo está en [`docs/adr/`](../adr/).

### 3.4 Estado global y reentrancia

`conductor/compilar.cpp` usa variables de ámbito de archivo:

- `archivo_de_entrada`
- `rutas_de_modulos`
- `hace_falta_ui`
- `hace_falta_ia`

`compilar()` reinicia las dos primeras, pero **no** reinicia `hace_falta_ui` ni
`hace_falta_ia`. En el CLI cada proceso compila una vez, así que no importa; si embebes el
compilador en un proceso de larga vida (servidor, LSP, herramienta que compila varios
archivos), tenlo en cuenta: una compilación con UI puede hacer que las siguientes enlacen
X11 aunque no lo necesiten. Aísla las compilaciones en procesos separados o añade un
reinicio explícito si vas a reutilizar la API.

El compilador no es thread-safe: no lances dos `compilar()` en paralelo en el mismo proceso.

---

## 4. Modelo de datos

### 4.1 Tokens

`lexico/token.hpp` define `TipoToken` (identificador, entero, flotante, texto, operador,
palabra clave, nueva línea, fin de archivo…) y `Token` con:

- `tipo`
- `lexema` (vista al texto original)
- `span` (inicio y fin en bytes)

Los diagnósticos usan `Span` y `GestorFuentes` para traducir a línea/columna.

### 4.2 AST

`incluir/nooki/sintactico/ast.hpp`:

| Nodo | Campos principales |
|---|---|
| `Programa` | `rasgos`, `funciones`, `tipos`, `enumeraciones`, `usos` |
| `Funcion` | `nombre`, `modulo`, `publico`, `es_externa`, `tipo_parametros`, `parametros`, `tipo_retorno`, `cuerpo` |
| `Parametro` | `nombre`, `tipo` (texto, puede incluir `&`, `&mutable`) |
| `DefinicionTipo` | `nombre`, `tipo_parametros`, `modulo`, `es_clase`, `publico`, `base`, `rasgos`, `campos`, `metodos` |
| `Campo` | `nombre`, `tipo`, `publico` |
| `Rasgo` | `nombre`, `metodos` |
| `Enumeracion` | `nombre`, `modulo`, `publico`, `variantes` |
| `Variante` | `nombre`, `campos` |
| `Expresion` | `tipo`, `lexema`, `anotacion`, `izquierda`, `derecha`, `lista`, `nombres_campo`, `tipos_aplicados`, `monticulo`, `span` |
| `Sentencia` | `tipo`, `expresion`, `paso`, `nombre`, `tipo_declarado`, `cuerpo`, `sino`, `ramas`, `span` |
| `Patron` | `clase` (comodín/literal/variante), `valor`, `enumeracion`, `ligaduras`, `span` |

`TipoExpresion` distingue texto, carácter, entero, flotante, booleano, nulo, identificador,
unaria, binaria, llamada, miembro, miembro opcional, índice, propagación, `como`,
asignación, construcción, `este` y `super`.

`TipoSentencia` distingue expresión, retorno, variable, constante, `si`, `mientras`, `para`,
`hacer`, `bucle`, `romper`, `continuar`, bloque, `coincide`, `inseguro` y `asm`.

### 4.3 Tipos semánticos

`semantico/tipos.hpp` define un enum pequeño:

```cpp
enum class Tipo : unsigned char {
    Desconocido, Vacio, Entero, Texto, Booleano, Flotante
};
```

Los tipos compuestos no se representan con un árbol de tipos: el AST guarda el **texto** del
tipo (`&texto`, `Vector<entero>`, `Caja<entero>`, `figura`, `&Saluda`) y el generador lo
traduce con `tipo_c()` y reglas ad hoc. Esto explica por qué algunas combinaciones
(`Caja<texto>`, `Vector<flotante>`) no están cerradas: el sistema de tipos completo nunca se
implementó; la compatibilidad se comprueba por nombre y por forma.

### 4.4 Diagnósticos

`diagnostico/diagnostico.hpp`:

- `Diagnostico` guarda **código + parámetros**, no el texto final.
- `Gravedad`: error, advertencia, nota, ayuda.
- `Etiqueta`: span secundario.
- `.parametro()`, `.etiqueta()`, `.nota()`, `.ayuda()`.

`diagnostico/emisor.hpp` acumula diagnósticos; `gestor_fuentes.hpp` convierte offsets en
línea/columna. `render_texto.cpp` y `render_json.cpp` producen la salida. El catálogo está en
`diagnostico/catalogo.cpp`, con español e inglés.

### 4.5 Reglas de oro del modelo

1. **Los códigos NOK son estables para siempre.** No se renumeran.
2. **El mensaje se compone desde el catálogo**, no se escribe en el punto de emisión.
3. **Todo diagnóstico lleva span** (salvo errores globales como `NOK0001`/`NOK6001`).
4. **La ayuda dice qué hacer**, no solo qué pasó.
5. **Los tests comparan códigos ordenados**, no el texto completo, para no acoplar la suite a
   la redacción.

---

## 5. Flujo de compilación

### 5.1 `compilar()` paso a paso

`conductor::compilar(opciones, fuentes, emisor, salida, error)`:

1. Lee el archivo de entrada. Si no existe, `NOK0001`.
2. Guarda `rutas_de_modulos` y `archivo_de_entrada` (canónico).
3. Lexa y parsea el archivo principal.
4. Si hay errores, termina.
5. Carga los módulos de `usar` recursivamente:
   - busca `nombre.oki` junto al archivo, dependencias del manifiesto, `biblioteca/` y
     padres;
   - evita autoimportarse;
   - aplana funciones, tipos y enumeraciones en el `Programa`, marcando su `modulo`;
   - activa `hace_falta_ui`/`hace_falta_ia` si el módulo lo requiere.
6. Verifica semánticamente. Si hay errores, termina.
7. Si `solo_verificar`, termina con éxito.
8. Genera C. Si el backend no soporta algo, `NOK6003`.
9. Si `solo_c`, escribe el C en `salida` y termina.
10. Busca compilador de C (`NOOKI_CC`, `cc`, `gcc`, `clang`). Si no hay, `NOK6001`.
11. Escribe el C en `%TEMP%/nooki_<stem>.c`.
12. Escribe runtimes UI/IA si hacen falta.
13. Invoca el backend con `-O2 -o <binario> <c> [<ia.c>] [<ui.c>] [-l...]`.
14. Si falla, `NOK6002` y deja la ruta del C en una nota.
15. Si el destino tiene caracteres no ASCII, enlaza en temporal y mueve.
16. Borra el C principal salvo `--conservar`; deja los de UI/IA.
17. Devuelve el binario.

### 5.2 Convenciones del C generado

- Un solo archivo C principal, autónomo, con runtime embebido.
- `nooki_texto` es `{ char* datos; long long largo; int propio; }`.
- `nooki_vector_entero`, `nooki_vector_texto`, `nooki_mapa_texto_entero`, `nooki_caja`.
- Aritmética entera con `__builtin_*_overflow` o comprobaciones manuales; pánico con
  `exit(70)`.
- Funciones de impresión: `nooki_imprimir_entero`, `_flotante` (coma decimal, 6 decimales),
  `_texto`, `_booleano`.
- Clases: struct + tabla de funciones virtuales; `nooki_ref_<rasgo>` para `&Rasgo`.
- Genéricos: `manglear(nombre, tipos)` produce nombres únicos por instanciación.
- Módulos: prefijo `nooki_<modulo>_<funcion>`.

### 5.3 Backend

`compilador_c()`:

```text
NOOKI_CC (si --version funciona)
luego cc, gcc, clang
```

Comando típico:

```bash
cc -O2 -o salida.exe nooki_principal.c nooki_ia_principal.c nooki_ui_principal.c \
   -luser32 -lgdi32      # Windows, si hay UI
   -lX11                 # Linux, si hay UI
```

No hay opción para pasar flags adicionales; usa un wrapper en `NOOKI_CC`.

### 5.4 Salida y códigos

- `nooki compilar`/`construir` devuelven `65` si hay errores de compilación.
- `nooki ejecutar` devuelve el código del programa si compila.
- Un pánico de aritmética sale con `70`.
- `nooki` no captura la salida del programa: se hereda la consola.

---

## 6. Cómo extender el compilador

### 6.1 Añadir o cambiar sintaxis

Orden recomendado:

1. **Diseño.** Si cambia el lenguaje, escribe o actualiza un ADR/RFC y la especificación.
   No empieces por el parser.
2. **Léxico.** Añade el token o la palabra en `lexico/lexer.cpp` (tabla
   `PALABRAS_RESERVADAS` ordenada) y en `token.cpp` si hace falta.
3. **AST.** Añade el nodo o el campo en `sintactico/ast.hpp`.
4. **Parser.** Impleméntalo en `parser.cpp`; respeta la precedencia Pratt y los saltos de
   línea.
5. **Impresor.** Actualiza `impresor.cpp` para que `nooki ast` y las pruebas tengan una forma
   canónica.
6. **Semántica.** Comprueba nombres, tipos, aridad, retornos, propiedad y préstamos en
   `semantico/verificaciones.cpp`.
7. **Generación.** Traduce a C en `generador/generador_c.cpp`; añade runtime si hace falta.
8. **Diagnósticos.** Añade códigos y mensajes; un caso que los provoque.
9. **Conformidad.** Un caso válido con `esperado.txt` y, si aplica, uno inválido con los
   códigos.
10. **Documentación.** Especificación, manual, referencia, guía, CHANGELOG y resaltado de
    editores.

Prueba cada fase por separado: `prueba_lexico`, `prueba_expresiones`,
`prueba_sentencias`, `prueba_semantico`, `prueba_compilador` y `conformidad`.

### 6.2 Añadir un diagnóstico

1. Añade la constante en `incluir/nooki/diagnostico/codigos.hpp`, en el rango correcto:
   - `NOK1xxx` léxico
   - `NOK2xxx` sintáctico
   - `NOK3xxx` nombres/tipos/módulos
   - `NOK4xxx` tipos y semántica
   - `NOK5xxx` propiedad/préstamos
   - `NOK6xxx` backend/ejecución
   - `NOK0xxx` compilador
2. Añade el mensaje en español e inglés en `diagnostico/catalogo.cpp`, con parámetros
   `{asi}`.
3. Emite el diagnóstico con `emisor.error(codigo, span).parametro(...).ayuda(...)`.
4. Añade un caso:
   - `pruebas/conformidad/invalido/<caso>/principal.oki` + `esperado.txt` (un código por
     línea), o
   - una comprobación en `compilador/pruebas/prueba_diagnosticos.cpp`.
5. Regenera `docs/referencia/diagnosticos.md` con
   `python herramientas/generar-referencia.py`.
6. Actualiza la tabla de la guía de referencia si procede.

Reglas:

- El mensaje debe decir **qué hacer**. Ejemplo:
  `no se puede usar «a»: el valor se movió antes` + `use «&» para prestarlo`.
- No reutilices un código para otra cosa.
- No cambies el significado de un código existente; añade uno nuevo.
- Un código sin caso es deuda (A-12).

### 6.3 Añadir una función o un módulo a la biblioteca

La biblioteca estándar vive **en Nooki**, dentro de `fuente_biblioteca()` en
`conductor/compilar.cpp`, como literales de cadena `R"(...)"`.

1. Añade el `if (modulo == "nombre")` con el código Nooki.
2. Marca la API con `publico`.
3. Si el módulo necesita runtime C:
   - crea/usa `runtime_ui.cpp` o `runtime_ia.cpp`;
   - añade las funciones `externo` en el módulo;
   - marca `hace_falta_ui`/`hace_falta_ia` en `cargar_modulo`;
   - añade las bibliotecas de enlace en `compilar()`.
4. Añade un caso válido en `pruebas/conformidad/valido/`.
5. Regenera `docs/referencia/biblioteca.md`.
6. Actualiza la tabla de módulos de `LEEME.md`, `GUIA.md`, `MANUAL.md` y
   `REFERENCIA.md` si procede.
7. Si el módulo tiene tipos/clases, recuerda que los tipos se importan sin prefijo y las
   funciones con prefijo.

**Límite de MSVC.** Los literales de cadena de más de 16 KB hacen que MSVC falle con
`C2026`. `runtime_ui.cpp` y `runtime_ia.cpp` están partidos en trozos. Si añades código,
ejecuta:

```powershell
.\herramientas\partir-literales.ps1 -Comprobar
.\herramientas\partir-literales.ps1
```

El preset `windows-msvc` lo detecta.

### 6.4 Añadir un comando de CLI

1. Añade la entrada en `incluir/nooki/cli/comandos.hpp`:
   `{ "nombre", "resumen", 0 }` (0 = disponible).
2. Despacha en `cli/orden.cpp`.
3. Implementa en `cli/<comando>.cpp` con su cabecera en `incluir/nooki/cli/`.
4. Añade el archivo a `compilador/CMakeLists.txt`.
5. Escribe pruebas en `compilador/pruebas/prueba_cli.cpp`.
6. Documenta en `docs/guias/REFERENCIA_DEL_LENGUAJE.md`, `GUIA_DEL_ADMINISTRADOR.md` y
   `nooki ayuda`.
7. Respeta los códigos de salida (`EXITO`, `USO_INCORRECTO`, `DATOS_INCORRECTOS`,
   `NO_DISPONIBLE`, `ERROR_INTERNO`).

### 6.5 Añadir una capacidad al LSP

`lsp/servidor.cpp` implementa JSON-RPC a mano:

1. Añade la capacidad en `initialize` (`capabilities`).
2. Implementa el método en el bucle `servir`.
3. Usa `Json` (`lsp/json.cpp`) para construir respuestas.
4. Añade una prueba en `compilador/pruebas/lsp.cpp`.
5. Recuerda: nada de logs por `stdout`; el protocolo ocupa ese canal.

Capacidades actuales: sincronización completa, diagnósticos, completion, definition y
formatting. No hay hover, rename, referencias ni workspace symbols.

### 6.6 Añadir o cambiar el formateador

`formato/formateador.cpp`:

- Cuenta llaves fuera de textos y comentarios.
- Ignora el interior de textos multilínea.
- Aplica 4 espacios por nivel y quita espacios finales.
- No reordena ni parte líneas.

Si cambias el estilo, actualiza `SANGRIA`, las pruebas y la guía del usuario. No conviertas
el formateador en un pretty-printer completo sin un ADR: cambiaría el código de los usuarios.

### 6.7 Añadir resaltado de sintaxis

Las gramáticas se generan de una sola definición:

```bash
python herramientas/generar-gramaticas.py
```

Actualiza:

1. `editores/vscode/syntaxes/nooki.tmLanguage.json`
2. `editores/vim/syntax/nooki.vim`
3. La tabla de `editores/LEEME.md`

No hay comprobación automática de que las tres listas coincidan; el sitio para añadirla es
`compilador/pruebas/prueba_documentos.cpp`.

### 6.8 Añadir una variante de runtime C

Si añades una función `externo` nueva:

- declárala en el módulo Nooki con los tipos correctos;
- impleméntala en el runtime C;
- respeta la ABI de `texto` (`const char*` en ida, copia al volver);
- añade un caso de conformidad que la ejecute;
- prueba en GCC y MSVC (si el runtime es C estándar).

---

## 7. Pruebas

### 7.1 Las 18 suites

| Suite | Qué prueba |
|---|---|
| `cli` | despacho, ayuda, `verificar`, `documentar` |
| `diagnosticos` | catálogo y casos de diagnóstico |
| `lexico` | tokens, palabras, literales, errores léxicos |
| `compilador` | compilación y ejecución de programas |
| `expresiones` | precedencia y asociatividad Pratt |
| `sentencias` | AST de control de flujo |
| `semantico` | nombres, tipos, aridad, retornos, propiedad |
| `documentos` | coherencia de versiones, sprints y documentación |
| `fuzz` | entradas aleatorias contra todas las fases |
| `panico` | desbordamiento/división por cero con código 70 |
| `rutas` | rutas con tildes |
| `lsp` | mensajes JSON-RPC y diagnósticos |
| `ventana` | runtime de UI enlazado y respuesta |
| `manual` | los bloques `nooki` de `MANUAL.md` compilan |
| `imagenes` | BMP y lectura de píxeles |
| `manifiesto` | parser de `nooki.toml` y dependencias |
| `rendimiento` | línea base del lexer |
| `conformidad` | 99 casos válidos/inválidos |

### 7.2 Conformidad

```text
pruebas/conformidad/
├── valido/<caso>/principal.oki
│   └── esperado.txt            (opcional)
└── invalido/<caso>/principal.oki
    └── esperado.txt            (obligatorio)
```

- **Válido con `esperado.txt`:** se compila, se ejecuta y su salida se compara exactamente.
- **Válido sin `esperado.txt`:** solo se comprueba que el léxico no dé errores.
- **Inválido:** pasa por léxico, parser y semántica; debe emitir exactamente los códigos
  listados en `esperado.txt`. Las líneas vacías y las que empiezan por `#` se ignoran; los
  códigos se comparan ordenados.
- Si no hay compilador de C, los válidos con salida se omiten (no fallan).

Para añadir un caso:

```bash
mkdir -p pruebas/conformidad/valido/0070-mi-caso
# escribe principal.oki y esperado.txt
ctest --preset windows-mingw -R conformidad --output-on-failure
```

### 7.3 Fuzzing

`compilador/pruebas/fuzz.cpp` genera entradas aleatorias y mutadas. `ctest` lo ejecuta con
600 iteraciones y 120 segundos de límite. Si falla, guarda la entrada que lo provocó y
añádela como caso de regresión.

### 7.4 Saneamiento

```bash
cmake --preset saneamiento
cmake --build --preset saneamiento
ctest --preset saneamiento
```

ASan + UBSan, avisos como errores. No se aplica con MSVC. Es la red de seguridad para el
generador y los runtimes.

### 7.5 Documentación verificada

- `prueba_manual` extrae los bloques `nooki` de `MANUAL.md` y los compila.
- `prueba_documentos` comprueba que los sprints terminados figuran en el historial y el
  LEEME, y que las versiones coinciden.
- `herramientas/generar-referencia.py` regenera las tablas de palabras, diagnósticos y
  biblioteca.

Si cambias el lenguaje, actualiza la documentación en el mismo commit. `ctest` lo detecta.

### 7.6 Buenas prácticas de prueba

- Una prueba debe fallar por la razón correcta. No compares mensajes completos si un código
  basta.
- Para un diagnóstico nuevo, prueba el código exacto y la línea/columna si es estable.
- Para una función de biblioteca, prueba el caso normal y el borde (vacío, fuera de rango,
  UTF-8).
- Para una optimización, añade una prueba funcional antes de medir.
- No dejes `TODO` sin un asunto en `DECISIONES.md`.

---

## 8. Estilo y revisión

### 8.1 C++

- **C++20**, sin dependencias externas.
- 4 espacios, 100 columnas, llaves pegadas (`Attach`), `#pragma once` o guardas según el
  archivo.
- Nombres de dominio en español: `verificar`, `emitir`, `ambito`, `siguiente`.
- Miembros con guion bajo final: `campos_`, `indice_`.
- Tipos y clases en `CamelCase`; funciones y variables en `snake_case`; constantes en
  `MAYUSCULAS`.
- `#include` solo al principio; nada de código comentado.
- Avisos como errores en CI de saneamiento.

Configuración: `.clang-format`, `.clang-tidy`, `.editorconfig`.

### 8.2 Commits

- Mensaje en español, imperativo: `biblioteca: anade el modulo textos`.
- Firmados: `git commit -s`.
- Un commit por cambio coherente o por sprint.
- El cuerpo explica **qué** y **por qué**, no repite el título.
- Ninguna característica sin prueba y sin documentación.

### 8.3 Revisión: lista de comprobación

- [ ] ¿El cambio tiene un caso de conformidad o una prueba unitaria?
- [ ] ¿Los diagnósticos nuevos tienen código, mensaje es/en y ayuda?
- [ ] ¿Se actualizó la especificación si cambia el lenguaje?
- [ ] ¿Se actualizaron `MANUAL.md`, `GUIA.md`, `REFERENCIA.md` y el CHANGELOG?
- [ ] ¿Compila sin avisos en GCC y MSVC?
- [ ] ¿Pasa `ctest` en el preset correspondiente?
- [ ] ¿Pasa ASan/UBSan si toca memoria?
- [ ] ¿El C generado sigue compilando sin avisos?
- [ ] ¿Los resaltados de editor siguen al día?
- [ ] ¿El cambio no rompe la compatibilidad de códigos NOK?

---

## 9. Depuración

### 9.1 Herramientas del compilador

```bash
# Ver el AST canónico
nooki ast archivo.oki

# Verificar sin generar C
nooki verificar archivo.oki

# Ver el C generado
nooki compilar archivo.oki --emitir-c > generado.c

# Conservar el C intermedio y su ruta
nooki compilar archivo.oki --conservar
```

### 9.2 Fallos típicos

| Síntoma | Dónde mirar |
|---|---|
| El parser se come una línea | saltos de línea, precedencia, `aceptar_lexema` |
| Un tipo no se reconoce | `tipo_c()`, `desde_nombre()`, `analizar_tipo_texto()` |
| Una llamada no encuentra la función | carga de módulos, prefijo `nooki_<modulo>_`, `funciones_` |
| `NOK6002` con C inválido | `--conservar`, compilar el `.c` a mano con `-Wall` |
| Fuga de memoria | ASan, `nooki_texto_soltar`, liberaciones de campos |
| Pánico inesperado | `nooki_panico`, aritmética comprobada, `inseguro` |
| El LSP no responde | stdio, `Content-Length`, logs en stdout |
| El formateador rompe un texto | llaves dentro de `"""`, comentarios |

### 9.3 Windows y MSVC

- El compilador se puede construir con MSVC, pero el backend debe ser MinGW/Clang.
- `runtime_ui.cpp` y `runtime_ia.cpp` usan literales largos; MSVC falla con `C2026` si
  superan 16 KB. Usa `herramientas/partir-literales.ps1`.
- Los argumentos de `nooki` se leen en UTF-16 y se convierten a UTF-8 en
  `principal.cpp`, para rutas con tildes.
- El enlazador de MinGW no acepta rutas no ASCII; el conductor enlaza en temporal y mueve.

### 9.4 Linux

- Sin pantalla, usa `xvfb-run -a` para pruebas de UI.
- `libx11-dev` solo hace falta para programas que usan UI.
- ASan/UBSan es el preset `saneamiento`.

### 9.5 Embebido / integración

Si usas `nooki_nucleo` como biblioteca:

- `conductor::compilar` no es reentrante ni thread-safe;
- `hace_falta_ui`/`hace_falta_ia` persisten entre llamadas;
- los diagnósticos se acumulan en un `Emisor` que tú proporcionas;
- `GestorFuentes` debe vivir lo suficiente para renderizar los spans.

---

## 10. Publicación

### 10.1 Antes de publicar

1. `git status --short` limpio.
2. `ctest --preset <preset>` en verde.
3. `ctest --preset <preset> -R documentos`.
4. `cmake --build --preset <preset>` sin avisos.
5. `nooki version` coincide con la versión de `CMakeLists.txt`.

### 10.2 Empaquetar

```powershell
.\herramientas\empaquetar.ps1
```

```bash
./herramientas/empaquetar.sh
```

Genera `dist/nooki-1.0.0-<plataforma>.zip` o `.tar.gz` con binario, documentación,
ejemplos, editores, especificación e `INSTALAR.txt`. No incluye el código fuente del
compilador.

Comprueba el binario empaquetado:

```bash
cd dist
unzip nooki-1.0.0-windows-x86_64.zip
./nooki-1.0.0-windows-x86_64/nooki.exe ejecutar \
  nooki-1.0.0-windows-x86_64/ejemplos/01-hola/principal.oki
```

### 10.3 Etiquetar

```bash
git tag -a v1.0.0 -m "Nooki 1.0: el lenguaje que describe la especificación"
git push origen v1.0.0
```

Publica las notas de [`NOTAS_DE_VERSION.md`](../../NOTAS_DE_VERSION.md) y adjunta la
distribución.

### 10.4 Compatibilidad

- Los códigos `NOK` no cambian.
- La sintaxis existente no se rompe sin una edición nueva o una versión mayor.
- Los módulos de la biblioteca añaden funciones; no cambian las firmas existentes sin
  documentarlo.
- Un ADR no se edita: se reemplaza.

---

## 11. Deuda técnica y hoja de ruta

### 11.1 Huecos observados en la 1.0.0

Estos no son «fallos del lenguaje» en el sentido de la especificación, pero son límites
reales que un desarrollador debe conocer:

| Hueco | Estado | Dónde |
|---|---|---|
| `Caja<texto>` genera C inválido | no soportado | `generador_c.cpp`, runtime `nooki_caja` |
| Métodos/herencia en estructuras | soporte parcial; llamar a un método heredado desde la hija puede generar `NOK6002` | `parser.cpp`, `generador_c.cpp` |
| `Vector<flotante>` se trata como vector de enteros | no soportado | `tipo_c()` elige `vector_entero` si no contiene `texto` |
| Escapes `\n`, `\t`, `\\`, `\"` se conservan literales | comportamiento actual | `escapar_c()` duplica la barra |
| `??` y `?.` se parsean pero el backend no los traduce | `NOK6003` | `generador_c.cpp` |
| `caracter` y literales `'a'` | lexados, backend no | `NOK6003` |
| `nulo` | literal, sin tipo opcional | semántica |
| `tipo`, `interfaz`, `modulo`, `importar`, `exportar`, `implementar` | reservadas, sin parser | `parser.cpp` |
| `constructor`, `destructor`, `propiedad`, `estatico` | reservadas, sin parser | `parser.cpp` |
| Macros, `async`, `await` | reservadas | lexer |
| Tiempos de vida completos | pendiente | asunto abierto |
| Temporales de expresión no se liberan | fuga menor A-18 | generador |
| `nooki probar`/`verificar` no cargan dependencias del manifiesto | limitación | `cli/proyecto.cpp`, `cli/verificar.cpp` |
| `nooki documentar` no incluye métodos de clase | limitación | `cli/documentar.cpp` |
| LSP sin módulos, hover, rename ni workspace symbols | limitación | `lsp/servidor.cpp` |
| UI macOS (sin Cocoa; el enlace añade `-lX11`), PNG, recolocación, cursor | pendiente | `runtime_ui.cpp`, `conductor/compilar.cpp` |
| 6 códigos NOK sin caso | deuda A-12 | `DECISIONES.md` |

### 11.2 Dónde empezar

Por orden de esfuerzo/beneficio:

1. Casos para los códigos sin prueba (A-12).
2. Cerrar `Caja<texto>` y `Vector<flotante>` con diagnóstico claro o implementación.
3. Arreglar la decodificación de escapes y añadir pruebas.
4. Cargar el manifiesto en `verificar` y `probar`.
5. Incluir métodos en `nooki documentar`.
6. LSP: módulos, hover y rename.
7. Tiempos de vida completos.
8. UI macOS y PNG.
9. Paquetes con manifiesto y registro.

### 11.3 Documentos de referencia

- [`DECISIONES.md`](../../DECISIONES.md): ADR y asuntos abiertos.
- [`PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md`](../../PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md):
  historial de sprints.
- [`CHANGELOG.md`](../../CHANGELOG.md): qué entró en cada versión.
- [`CONTRIBUIR.md`](../../CONTRIBUIR.md): reglas de contribución.
- [`spec/especificacion-v0.1.md`](../../spec/especificacion-v0.1.md): norma (incluye
  características aún no implementadas).
- [`docs/referencia/`](../referencia/): tablas generadas.

---

## 12. Apéndices

### 12.1 Comandos de desarrollo

```bash
cmake --preset linux
cmake --build --preset linux -j 4
ctest --preset linux --output-on-failure
ctest --preset linux -R conformidad
ctest --preset linux -R documentos
cmake --preset saneamiento && cmake --build --preset saneamiento && ctest --preset saneamiento
```

### 12.2 Estructura de un diagnóstico

```cpp
emisor.error(codigos::USO_TRAS_MOVER, span)
    .parametro("nombre", "letras")
    .ayuda("use «&» para prestarlo sin transferirlo");
```

### 12.3 Glosario

| Término | Significado |
|---|---|
| AST | árbol de sintaxis abstracta |
| backend | compilador de C que recibe el C generado |
| conformidad | caso que compila/ejecuta o que debe emitir códigos |
| dueño | variable responsable de liberar un valor |
| préstamo | referencia temporal `&` o `&mutable` |
| monomorfización | generar una versión por instanciación genérica |
| pánico | parada en ejecución con código 70 |
| Nooki | nombre del lenguaje y del compilador |
| `.oki` | extensión de fuente |
| `nooki.toml` | manifiesto del proyecto |

### 12.4 Índice de ADR

| ADR | Tema |
|---|---|
| ADR-001 | modelo de objetos |
| ADR-002 | mutabilidad y nombres |
| ADR-003 | aritmética entera |
| ADR-004 | modelo de errores |
| ADR-005 | `async` fuera de la v1 |
| ADR-006 | atributos y macros |
| ADR-007 | memoria transitoria |
| ADR-008 | Unicode e identificadores |
| ADR-009 | virtualidad explícita |
| ADR-010 | ABI y FFI |
| ADR-011 | ensamblador e intrínsecos |
| ADR-012 | licencia y gobernanza |
| ADR-013 | decisiones sintácticas |
| ADR-014 | backend C |
| ADR-015 | clases por valor en v0.1 |
| ADR-016 | caja y movimiento |
| ADR-017 | préstamos de expresión |
| ADR-018 | texto con marca de propiedad |
| ADR-019 | genéricos por instanciación |
| ADR-020 | regla única de propiedad |
| ADR-021 | clases por referencia |
| ADR-022 | herencia y despacho |
| ADR-023 | errores como valores |
| ADR-024 | variantes con datos |
| ADR-025 | biblioteca en Nooki |

