# Historial de cambios

El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/) y el proyecto usa
[versionado semántico](https://semver.org/lang/es/).

## [Sin publicar]

### Sprint 77 — Biblioteca: archivos y fechas (2026-11-12)

Un lenguaje sin archivos no sirve para programas de verdad. Ahora los tiene, con la API de errores del
lenguaje:

```aura
usar archivo

coincide (archivo.leer(&ruta)) {
    caso Bien(contenido) => imprimir(contenido)
    caso Mal(motivo) => imprimir(motivo)
}
```

- **`archivo`**: `leer`, `escribir` y `existe`, con un enumerado `lectura` que lleva `Bien(contenido)` o
  `Mal(motivo)`; el `?` del lenguaje funciona sobre él.
- **Tres funciones integradas** en el runtime: `leer_archivo`, `escribir_archivo` y `existe_archivo`.
  Leen el archivo entero y son las que un módulo de Nooki envuelve — el lenguaje no gana magia, gana
  una API.
- **`fechas` con aritmética**: `a_dias`, `a_anio`, `a_mes`, `a_dia`, `sumar_dias` y `dia_de_la_semana`.
  Una fecha es un entero: días desde el 1 de enero de 1970. Verificado con valores conocidos
  (`2000-01-01` = 10957, y el 12 de noviembre de 2026 cae en jueves).
- Casos `0049-archivo` y `0050-fechas`. **79 casos de conformidad.**

**Y dos fallos que aparecieron al escribir los casos:**

1. **Los textos multilínea estaban rotos desde el principio**: `"""..."""` conservaba dos comillas a cada
   lado, porque el analizador quitaba solo una. Ahora se quitan las tres.
2. **El impresor del AST no conocía `inseguro` ni `asm!`** (del sprint anterior): `nooki ast` no las
   mostraba y la compilación con avisos como errores fallaba. Lo detectó la cadena de saneamiento.

### Sprint 76 — Servidor de lenguaje (2026-11-12)

`nooki lsp` habla el **Language Server Protocol** por la entrada y salida estándar: cualquier editor con
cliente LSP puede colorear errores mientras se escribe, sin escribir una extensión para Nooki.

```json
{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{...}}}
```
```json
{"jsonrpc":"2.0","method":"textDocument/publishDiagnostics","params":{"diagnostics":[
  {"range":{"start":{"line":2,"character":4},"end":{"line":2,"character":12}},
   "severity":1,"code":"NOK2002","source":"nooki",
   "message":"se esperaba una expresión y se encontró «retornar»"}]}}
```

- **Sincronización completa** de documentos (`didOpen`, `didChange`): cada cambio reanaliza el archivo.
- **Diagnósticos con rango exacto**: línea y columna del span, gravedad, código `NOKnnnn` y el mensaje en
  español con sus notas y ayudas.
- **Sin dependencias**: un lector y escritor de JSON de 300 líneas y el bucle del protocolo, en C++.
  Nada de bibliotecas externas.
- **`prueba_lsp`**: capacidades, un documento con error (comprueba código, severidad y línea) y un
  documento correcto sin diagnósticos. **15 suites.**

**Configuración en el editor**: cualquier cliente LSP sirve. En Neovim, con `nvim-lspconfig`, basta
registrar el comando `nooki lsp` para los archivos `.oki`. La extensión de VS Code sigue coloreando la
sintaxis; para diagnósticos necesita un cliente LSP (otro sprint).

**Lo que no incluye**: autocompletado, ir a la definición, formato y resaltado semántico. El protocolo
está montado para añadirlos sin tocar lo demás.

### Sprint 75 — Manifiesto y dependencias (2026-11-12)

Un proyecto deja de ser una carpeta suelta: declara qué es y de qué depende.

```toml
# agenda/nooki.toml
[proyecto]
nombre = "agenda"
version = "0.1.0"

[dependencias]
saludos = { ruta = "../saludos" }
```

```aura
usar saludos     // se resuelve en la dependencia, no solo en «biblioteca/»
```

- **`nooki.toml`**: nombre, versión y dependencias por ruta. Se admite un subconjunto de TOML —
  secciones, `clave = "valor"` y `clave = { ruta = "..." }` — y los comentarios con `#`.
- **`nooki construir` lee el manifiesto** y añade, a la búsqueda de módulos, la raíz y la `biblioteca/`
  de cada dependencia.
- **`nooki nuevo` escribe el manifiesto**, así que todo proyecto nuevo ya lo tiene.
- **`prueba_manifiesto`**: lectura, dependencias, rutas de búsqueda y escritura. **14 suites.**
- **Verificado de punta a punta**: dos proyectos (`agenda` y `saludos`), `nooki construir` desde
  `agenda`, binario que imprime «Hola, Nooki» usando el módulo del otro proyecto.

**Lo que no incluye**, y es deliberado: no hay versiones semánticas resueltas, ni descarga desde Git, ni
registro, ni bloqueo de dependencias. Eso es un gestor de paquetes, y necesita usuarios antes que
diseño. El manifiesto deja la puerta abierta: hoy `ruta`, mañana `version` y `git`.

### Sprint 74 — `inseguro` y `asm!` (2026-11-12)

El último pilar del lenguaje: **control de hardware**. Y la válvula de escape que el plan pedía, con
una regla que la hace explícita y localizable.

```aura
inseguro {
    variable envuelto = grande + 1   // se envuelve, no se detiene
    asm!("nop")
}
```

- **`inseguro { ... }`**: dentro, la aritmética se emite tal cual, sin las comprobaciones de ADR-03.
  Es lo que hace falta para escribir controladores, donde el desbordamiento es parte del hardware y no
  un error.
- **`asm!("...")`**: ensamblador en línea. **Solo dentro de `inseguro`** (`NOK3012`), para que cada uso
  se pueda encontrar con una búsqueda.
- **La propiedad no se desactiva**: dentro de `inseguro` se siguen liberando los valores y se siguen
  moviendo al pasar; lo único que se apaga es la comprobación aritmética. Un bloque no puede hacer que
  el compilador libere dos veces.
- Caso `0048-inseguro`, verde en MinGW, MSVC y Linux.

**Límite del `asm!`**: necesita un compilador de C que lo admita (GCC, Clang, MinGW). Si el backend es
`cl.exe`, el C generado avisa con un `#error` claro en lugar de fallar de forma incomprensible.

Con esto, **los cinco pilares del plan original tocan código**: sintaxis fácil, tipos, memoria segura,
OOP completa y control de hardware.

### Sprint 73 — FFI: llamar a C (2026-11-12)

El plan ponía el FFI **antes** de la biblioteca estándar, «porque sin libc no hay archivos, red ni
tiempo». Ha llegado con tres años de retraso, y es el desbloqueo que faltaba.

```aura
externo funcion puts(cadena: &texto) -> booleano
externo funcion time(zona: &entero) -> entero

funcion principal() -> entero {
    puts("hola desde C")
    variable ahora = time(nulo)
    si (ahora > 1000000000) {
        imprimir("el reloj responde")
    }
    retornar 0
}
```

- **`externo funcion`** declara una función de C: nombre, parámetros y tipo de retorno; sin cuerpo.
- **Tipos traducidos**: `entero` ↔ `long long`, `booleano` ↔ `int`, `caracter` ↔ `char`, `vacio` ↔
  `void`, `&texto` ↔ `const char*`, y `&escalar` ↔ puntero al escalar.
- **Los textos viajan como puntero a sus bytes**: el runtime ya los guarda terminados en cero, así que
  no hay copia ni asignación extra.
- **`nulo`** ya se traduce: es el `NULL` de C, y sirve para los parámetros que lo admiten.
- Caso de conformidad `0047-ffi`, verificado en MinGW, MSVC y Linux.

**Límites conocidos, escritos**: no se declaran estructuras, ni funciones que devuelvan `texto`, ni se
elige biblioteca (todo se enlaza contra la libc que el compilador de C use). Y si la firma declarada
no coincide con la de C, el compilador de C avisa del conflicto: `strlen` devuelve `size_t`, así que
declararlo `entero` no compila — hay que declararlo como `natural64`.

### Sprint 72 — Desbordamiento comprobado y rutas con tildes (2026-11-11)

**ADR-03 se cumple por fin.** Era una decisión cerrada y el código la contradecía: `a + b` se emitía
como un `+` de C y desbordaba en silencio. Ahora la aritmética entera pasa por funciones comprobadas:

```aura
variable grande = 9223372036854775807
variable otro = grande + 1
```
```
$ ./programa
error en ejecucion: desbordamiento al sumar
$ echo $?
70
```

- **Comprobadas**: suma, resta, multiplicación, división, módulo y negación. Un desbordamiento, una
  división por cero o negar el mínimo detienen el programa con código **70** y un mensaje en la salida
  de error. Nunca se continúa con un valor equivocado.
- **Sin coste apreciable**: con `-O2` cada comprobación es un salto predicho.
- **`prueba_panico`**: seis casos que compilan, ejecutan y verifican el código y el mensaje.

**Y el fuzzer del sprint anterior destapó un fallo de fondo: los nombres con tildes.** En un lenguaje
en español eso no es un caso raro.

- `nooki ejecutar archivo ñ.oki` **caía el compilador**: Windows entrega los argumentos en la página de
  códigos local y la conversión a ruta lanzaba `Illegal byte sequence`. Ahora la línea de comandos se
  lee en UTF-16 y se convierte a UTF-8 (`CommandLineToArgvW`).
- Todas las conversiones de ruta a texto pasan por UTF-8 (`ruta_a_utf8`), no por `.string()`.
- El archivo `.c` intermedio usa un nombre ASCII, y si el destino tiene tildes se enlaza en una ruta
  ASCII y el binario se mueve después: el enlazador de MinGW no abre rutas con acentos.
- Las órdenes con bytes no ASCII se entregan al intérprete como UTF-16 (`_wsystem`).
- **`prueba_rutas`**: compila y ejecuta `carpeta con tildes á/programa ñ.oki`.

**Y un tercer fallo, silencioso desde el principio**: en Linux y macOS `ejecutar_comando` devolvía el
estado de espera de `system()` en lugar del código de salida, así que un programa que salía con 70 se
leía como 17920. Corregido con `WEXITSTATUS`.

- **13 suites** en `ctest`, verdes en MinGW, MSVC, GCC y ASan+UBSan.

### Sprint 71 — Fuzzing y benchmarks (2026-11-11)

Dos guardarraíles del plan que nunca se habían cumplido: el fuzzing continuo (regla 8) y los
benchmarks (regla 9).

**El fuzzer encontró un fallo real en la primera sesión larga.** Un escape inválido al final del
archivo (`"abc\`) hacía caer el compilador con `std::out_of_range`: `avanzar(2)` dejaba la posición un
byte más allá del final y el `substr` de `crear` lanzaba. Corregido acotando la posición —ahora un
valor movido o un escape al final dan un diagnóstico, no una caída— y con un caso de regresión
(`invalido/0018-escape-al-final`).

- **`nooki_fuzz`**: 600 entradas en `ctest` (2 000 por defecto, semilla elegible). Mezcla mutaciones del
  corpus de conformidad, piezas del lenguaje y bytes aleatorios, y pasa cada entrada por léxico, parser,
  verificación y generación. Guarda la entrada que provoque una excepción.
- **Resultado**: **100 000 entradas con cinco semillas** sin caídas, en Windows y en Linux.
- **`nooki_rendimiento`**: línea base de las métricas del plan. Lexer **25,5 MB/s** (Windows) y **33,3
  MB/s** (Linux); compilación completa **0,0029 s por 1 000 líneas**, unas **setecientas veces** por
  debajo del objetivo de 2 s del plan. Bajo sanitizadores mide sin exigir umbrales.
- **11 suites** en `ctest` (eran 9), en las cinco cadenas.

### Sprint 70 — Verificación fuera de Windows (2026-11-11)

**El compilador compila y pasa las nueve suites en Linux, con tres cadenas distintas.** Era la regla 1
del plan («verde en los tres sistemas») y nunca se había comprobado.

| Cadena | Compilación | Suites |
|---|---|---|
| GCC 13.3 · Ubuntu 24.04 | ✅ sin avisos | **9/9** |
| Clang 18.1 · Ubuntu 24.04 | ✅ sin avisos | **9/9** |
| GCC 13.3 + **ASan + UBSan**, avisos como errores | ✅ | **9/9** sin errores de memoria ni comportamiento indefinido |

Los ejemplos también se ejecutan en Linux (`nooki ejecutar ejemplos/01-hola/principal.oki`).

- **Dos avisos eliminados** que solo veía Clang: un campo `fuentes_` sin usar en el lexer y una función
  `es_clase_c` huérfana en el generador.
- **Faltaba el preset de compilación `saneamiento`**: estaba documentado y solo existía el de
  configuración, así que `cmake --build --preset saneamiento` fallaba al instante. Añadido.
- **macOS sigue sin verificar** (no hay hardware): se anota como A-15. La cadena de Clang en Linux es
  la evidencia más cercana disponible.

### Sprint 69 — Ejemplos y resaltado de sintaxis (2026-11-11)

- **`ejemplos/`**: cuatro programas ejecutables (hola, notas con módulo y errores, inventario con
  herencia, saludos con rasgos), cada uno con su salida real en `ejemplos/LEEME.md`.
- **Resaltado de sintaxis**: extensión de **VS Code** (gramática TextMate + configuración de
  lenguaje), sintaxis de **Vim/Neovim**, e instrucciones para Sublime Text, Zed, TextMate y
  Notepad++. Todo en `editores/`.
- **Fallo corregido**: `nooki ejecutar` con rutas que usan `/` fallaba en Windows, porque el comando se
  entregaba a la consola tal cual y esta no entiende `/` como separador. Ahora las rutas se normalizan
  al separador nativo antes de invocar al sistema.

### Sprint 68 — Preparar la publicación (2026-11-11)

- **Licencias completas**: `LICENCIA-MIT.txt` y `LICENCIA-APACHE.txt` con los textos íntegros, y
  `LICENCIA.md` que enlaza ambos. Cierra A-06.
- **`CONTRIBUIR.md` reescrito**: reflejaba el estado del sprint 3 («casi nada implementado»). Ahora
  explica las reglas reales: C++20 sin dependencias, los tres niveles de prueba, los tres requisitos
  de un diagnóstico nuevo y el commit firmado.
- **`PUBLICAR.md`**: los pasos exactos para subir el repositorio, ejecutar la CI por primera vez y
  qué hacer con lo que falle.
- **CI revisada**: presets y rutas del binario comprobados contra `CMakePresets.json` y el árbol de
  `build/`.

## [1.0.0] — 2026-11-11

**Nooki 1.0: todo lo que la especificación describe está implementado y probado, sin defectos
conocidos.** Las notas completas de la versión están en [NOTAS_DE_VERSION.md](NOTAS_DE_VERSION.md).

### Sprint 67 — Cierre de la 1.0 (2026-11-11)

- **Notas de versión** con lo que incluye, lo que no, para quién es y cómo empezar.
- **Inventario de pendientes** en `DECISIONES.md`, cada uno con su tamaño.
- Versión 1.0.0 en el compilador, la especificación y la documentación.

### Sprint 66 — A-14: mover un campo (2026-11-11)

- **Mover un campo a una colección lo deja vacío, no roto**: al vaciar un `texto` no se ponía su
  longitud a cero, así que quedaba un texto con puntero nulo y longitud vieja — cualquier lectura
  posterior era corrupción. Ahora el valor movido es un texto vacío válido.
- **Un método heredado cuenta como miembro**: `h.f()` donde `f` está en la clase base se rechazaba con
  «no tiene ningún miembro». Se busca en la cadena de bases, en el acceso a campos y en la llamada.
- **Una variable construida recuerda su tipo concreto**: `variable p = nuevo persona {...}` no guardaba
  `persona`, así que `p.nombre + "fin"` no compilaba.
- **`imprimir` acepta booleanos** (1 o 0).
- Caso `0046-campo-a-coleccion`.

### Sprint 65 — Todas las vías del movimiento (2026-11-11)

- **La comprobación de préstamo vive en un solo sitio** (`comprobar_prestamo`), al que llaman todas
  las vías que mueven un valor: reasignación, declaración y argumento por valor. Antes solo la
  reasignación miraba la tabla de préstamos.
- **Reasignar ya no marca el valor como movido**: era un error del arreglo anterior y rompía
  programas válidos que reasignan un `texto`.
- Caso `0017-mover-por-declaracion`.
- **Dos límites encontrados y anotados**: prestar un parámetro que es una clase se rechaza con
  `NOK4008` (solo se prestan variables locales), y mover un *campo* a una colección
  (`vector.agregar(p.nombre)`) no se reconoce como movimiento (A-14).

## [0.9.0] — 2026-11-11

**La biblioteca estándar tiene cinco módulos, todos escritos en Nooki**, y la documentación ya no
puede desincronizarse sin que un `ctest` lo diga.

### Sprint 64 — Cierre de la v0.9 (2026-11-11)

- **Prueba de coherencia documental** (`prueba_documentos`): comprueba que cada sprint terminado del
  plan tiene entrada en el historial y en el README, y que la versión coincide en CMake, historial,
  LEEME y especificación. Está en `ctest` como el resto.
- **Al estrenarla encontró dos huecos**: los sprints 52 y 53 no estaban en el historial. Reparados, y
  también las entradas 0.7.0 y 0.8.0 que faltaban.
- **La guía gana un paso 12** sobre la biblioteca estándar, con su salida verificada.
- v0.9.0.

### Sprint 63 — La biblioteca crece (2026-11-11)

- **`listas`**: `suma`, `mayor`, `menor`, `promedio` y `contiene` sobre `&Vector<entero>`, escritos en
  Nooki.
- **`fechas`**: `bisiesto` (con la regla de los siglos: 2100 no, 2024 sí), `dias_del_mes` y
  `dias_del_anio`.
- Ambos viajan **dentro del compilador**: se usan con `usar` sin copiar archivos.
- Caso de conformidad `0045-biblioteca-listas`.

### Sprint 62 — A-13 no era un fallo (2026-11-11)

- **La biblioteca embebida funciona**: `usar textos` sin archivo resuelve el módulo que viaja dentro
  del compilador.
- **A-13 era un archivo viejo**: `build\textos.oki`, un programa de pruebas olvidado, tapaba al módulo
  embebido. *Diagnóstico anterior equivocado, corregido aquí.*
- **Un módulo no puede definir `principal`**: antes un módulo con punto de entrada producía dos `main`.

### Sprint 61 — Biblioteca en Nooki y tipos de colección (2026-11-10)

- **Módulo `textos`** escrito en Nooki: `unir`, `repetir` y `contar`.
- **El elemento de un `Vector<texto>` es un texto**: se tipaba como entero y la concatenación no
  compilaba.
- **`*prestamo` de texto**: desreferenciar un `&texto` devuelve un texto.
- **Un archivo ya no puede importarse a sí mismo**: `usar principal` dentro de `principal.oki` cargaba
  el propio archivo como módulo y generaba dos `main`.

## [0.8.0] — 2026-11-10

La propiedad queda cerrada por los dos extremos: el valor no se puede mover mientras alguien lo presta
(`NOK5007`), y no se puede devolver un préstamo a algo que muere (`NOK5006`).

### Sprint 60 — Cierre de la v0.8 (2026-11-10)

- **Auditoría de diagnóstico**: los **46 códigos** están definidos, todos se emiten y todos tienen
  mensaje en español e inglés. **36 tienen un caso que los provoca**.
- **Tres códigos ganaron caso**: `NOK4011`, `NOK4012` y `NOK4010`.
- **Los 10 restantes, identificados** como deuda de pruebas (A-12): rutas de integración (CLI, carga de
  módulos, símbolo privado, backend y avisos).

### Sprint 59 — La duración del préstamo (2026-11-10)

- **Reasignar o mover el dueño mientras otro lo presta es error** (`NOK5007`).
- **Prestar una clase presta el objeto**: `&persona` ya no genera un puntero doble, y pasar `&p` a un
  parámetro `&Clase` compila.

### Sprint 58 — Que las pruebas prueben lo que dicen (2026-11-10)

- **Los casos `invalido/` pasan por todas las fases**: léxico, parser y verificación.
- **Cuatro casos inválidos semánticos nuevos**: `NOK5006`, `NOK5001`, `NOK5004` y `NOK3011`.

### Sprint 57 — El préstamo que escapa (2026-11-10)

- **Devolver un préstamo a una variable local es error** (`NOK5006`).
- **Prestar un parámetro sí se puede devolver**; los argumentos de una llamada no escapan.

## [0.7.0] — 2026-11-10

**El sistema de tipos está completo.** Con rasgos y `&Rasgo`, el lenguaje tiene las dos formas de
polimorfismo: por jerarquía y por contrato.

### Sprint 56 — Cierre de la v0.7 (2026-11-10)

- **ADR-026** documenta los rasgos: contrato verificado, tabla por (tipo, rasgo), adaptadores y el
  coste de `&Rasgo`.
- **La guía gana un paso 11** sobre rasgos, con su salida verificada.

### Sprint 55 — `&Rasgo`: despacho por rasgo (2026-11-10)

- **`&Rasgo` como tipo**: una referencia que lleva el objeto y su tabla de métodos.
- **Tabla por (tipo, rasgo)** con un adaptador por método y despacho dinámico entre tipos **no
  emparentados**.

### Sprint 53 — La guía en español (2026-11-10)

- **`GUIA.md`**: recorrido de cero a un programa con módulos en once pasos, cada uno **ejecutado** para
  copiar su salida real.
- **`LEEME.md`**: qué es Nooki, qué pilares tiene y cuáles no, cómo compilarlo y cómo se prueba.

### Sprint 52 — Las herramientas del proyecto (2026-11-10)

- **`nooki nuevo`**, **`nooki probar`** y **`nooki limpiar`**: crear un proyecto que compila, ejecutar
  pruebas contra su `.esperado` y borrar artefactos.
- **La búsqueda de módulos parte de rutas absolutas**, así que `usar` funciona desde subcarpetas.

### Sprint 54 — Rasgos (2026-11-10)

- **`rasgo Nombre { funcion f(...) -> tipo }`** declara un contrato; `clase X implementa A, B` se
  compromete a cumplirlo.
- El compilador exige que **estén todos los métodos** (`NOK3011`) y que la **firma coincida**
  (`NOK4012`).

## [0.6.0] — 2026-11-10

**Sin defectos conocidos.** Por primera vez la lista de «cosas que están mal y se saben» está vacía:
los huecos de propiedad que quedaban abiertos se cerraron, y el único caso que el generador no puede
resolver —una llamada que mueve un valor con dueño dentro de otra expresión— es ahora un error de
compilación, no una fuga silenciosa.

### Sprint 51 — Cerrar A-11 y publicar la v0.6 (2026-11-10)

- **Llamada anidada que movería un valor con dueño**: `f(g(a))` donde `g` mueve `a` ya no compila. El
  generador no tiene dónde emitir el vaciado del origen dentro de una expresión, así que lo rechaza
  con un mensaje que explica la alternativa.
- **Caso `0020-propiedad-parametros` migrado** a la forma que sí funciona: mover en una declaración.
- Versión 0.6.0 y especificación actualizada.

### Sprint 50 — Cerrar los huecos de propiedad (2026-11-10)

- **Copiar un compuesto con campos con dueño es error** (`NOK5004`): compartiría las cadenas y se
  liberarían dos veces. El mensaje sugiere prestarlo o construir otro.
- **`romper` y `continuar` liberan lo que abandonan**: antes se saltaban la liberación de las
  variables con dueño declaradas dentro del bucle.
- **`para` y `mientras` registran su profundidad de ámbito**, de modo que la liberación al saltar
  sabe hasta dónde deshacer.
- **Hueco restante declarado**: una llamada que mueve un valor con dueño dentro de otra expresión
  (`f(g(a))`) sigue sin moverlo. Es una fuga, no corrupción, y está anotada.
- Programas de conformidad `0040-propiedad-saltos`, `0039-compuestos-dueno` y `0038-propiedad-retorno`.

### Sprint 49 — Los compuestos liberan sus campos (2026-11-10)

- **Destructor por instanciación**: un compuesto genérico cuyos campos tienen dueño genera su
  `soltar` y se libera al salir del ámbito. La fuga que quedaba abierta en A-10 se cierra.
- **Solo se libera el compuesto fresco**: el construido en el sitio o devuelto por una llamada. Uno
  copiado desde otra variable comparte los campos y no se libera, para no liberar dos veces.
- **Los campos también se mueven**: `libro.insertar(q.primero, ...)` vacía `q.primero` después de
  insertarlo, de modo que el mapa y el compuesto no comparten la misma cadena.
- Caso de conformidad `0039-compuestos-dueno`, que construye compuestos en un bucle e inserta sus
  campos en un mapa.

### Sprint 48 — Cerrar A-10: el retorno transfiere lo que contiene (2026-11-10)

- **Devolver un compuesto transfiere sus variables**: `retornar par<texto> { primero: nombre }` ya no
  libera `nombre`; la propiedad pasa al llamante.
- **Construir una estructura mueve lo que se le pasa**: `variable p = par<texto> { primero: nombre }`
  anula `nombre`, igual que una asignación. Se cierra el caso indirecto (construir y devolver la
  variable), que seguía dejando punteros colgando.
- **Caso de conformidad `0038-propiedad-retorno`** con las dos formas: la directa y la que construye
  en una variable antes de devolver.
- **Riesgo residual documentado** (A-10, reducido): un compuesto con campos con dueño no los libera al
  salir del ámbito, así que esas cadenas se pierden. Es una fuga, no corrupción.

## [0.5.0] — 2026-11-10

**El lenguaje del plan está implementado.** Con genéricos de tipos, proyectos y biblioteca escrita en
Nooki, todo lo que la especificación describe —salvo tiempos de vida y rasgos— funciona y está
probado: 59 programas que se compilan y ejecutan en cada `ctest`, en GCC y MSVC.

### Sprint 47 — Cierre de la v0.5 (2026-11-10)

- **Auditoría de diagnóstico**: los 41 códigos `NOK` están definidos y todos se emiten en algún
  camino del compilador; ninguno quedó huérfano.
- **Programa de cierre `0037-notas`**: módulos, genéricos (`par<texto>`), errores con variantes,
  `coincide` con ligaduras, mapa, préstamos y entrada estándar, en un solo programa de dos archivos.
- **Hallazgo documentado**: devolver una construcción que embebe variables locales con dueño las
  libera al retornar y deja punteros colgando. Se documenta como limitación conocida y se marca como
  el próximo trabajo de corrección; el ejemplo lo evita construyendo directamente en el `retornar`.
- **v0.5.0**.

### Sprint 46 — Genéricos de tipos (2026-11-10)

- **Tipos con parámetros**: `estructura par<T> { publico primero: T }`.
- **Instanciación explícita**: `par<entero> { primero: 1, segundo: 2 }` y `par<texto> { ... }`.
  Cada combinación genera su propia estructura en C (`nooki_par_entero`, `nooki_par_texto`),
  siguiendo el mismo criterio que las funciones genéricas.
- **Descubrimiento por recorrido**: el generador registra las instanciaciones que aparecen en
  declaraciones, parámetros, campos y construcciones antes de emitir nada.
- Programa de conformidad `0036-genericos-tipo`, compilado y ejecutado.

### Sprint 45 — Paquetes: proyectos (2026-11-10)

- **Búsqueda de módulos**: junto al archivo que importa, en su carpeta `biblioteca/` y subiendo por
  los directorios padre. Un proyecto deja de necesitar rutas ni copias de sus módulos.
- **`nooki construir`**: compila el proyecto del directorio actual, tomando `principal.oki` o
  `fuente/principal.oki` como punto de entrada.
- **La biblioteca integrada sigue disponible**: los módulos del compilador (`resultado`,
  `matematicas`) se usan sin archivo, y un proyecto puede sobrescribirlos con los suyos.
- Programa de conformidad `0035-proyecto`, con un módulo en `biblioteca/`.

### Sprint 44 — La biblioteca crece (2026-11-10)

- **`texto.dividir(separador)`**: parte un texto y devuelve un `Vector<texto>` cuyos elementos son
  copias con dueño. Con esto un programa analiza una línea con varios campos.
- **`texto.mayusculas()`, `minusculas()` y `recortar()`**: copias transformadas; `recortar` quita
  espacios y saltos de los extremos.
- **Módulo `matematicas` escrito en Nooki**: `maximo`, `minimo`, `absoluto` y `cuadrado`.
- Programa de conformidad `0034-biblioteca`, que analiza un CSV y usa la biblioteca matemática.

### Sprint 43 — La biblioteca estándar se escribe en Nooki (2026-11-10)

- **Módulo `resultado` escrito en Nooki**: `usar resultado` trae la enumeración con `Ok(valor)` y
  `Error(mensaje)`. El módulo viaja dentro del compilador, sin archivos ni rutas de búsqueda.
- **`?` es estructural**: funciona con cualquier enumeración que tenga forma de resultado (dos
  variantes y la primera con un campo), no con un tipo concreto del compilador.
- **El compilador adelgaza**: se retira `nooki_resultado` del runtime, los `Ok`/`Error` integrados y
  el caso especial de la verificación.
- **Migrado `0031-resultados`** a la biblioteca: el mismo programa, ahora sin magia en el compilador.
- **ADR-025** documenta la decisión y sus costes.

### Sprint 42 — Variantes con datos (2026-11-10)

- **Variantes con campos**: `enumeracion respuesta { Bien(valor: entero) Mal(mensaje: texto) }`.
- **Representación**: unión etiquetada en C cuando alguna variante lleva datos; entero con constantes
  cuando no, así que nada anterior cambia.
- **Construcción por variante**: `Bien(10)` y `Mal("mensaje")`.
- **Patrones que ligan**: `caso Bien(v)` declara `v` con el tipo del campo dentro de la rama.
- **Cobertura completa**: si están todas las variantes, no hace falta `_`.
- **Liberación**: las variantes con texto sueltan su campo al salir del ámbito.
- **ADR-024** documenta la decisión y sus costes.
- Programa de conformidad `0033-variantes`, compilado y ejecutado con las dos ramas.

### Sprint 41 — Operaciones de texto (2026-11-10)

- **`buscar(otro)`**: posición de la primera aparición, o `-1` si no está.
- **`subtexto(desde, cuantos)`**: copia con dueño, recortada al final del texto si se pide de más.
- **`contiene(otro)`** y **`empieza_con(prefijo)`**: comprobaciones sin extraer copia.
- **`caracter(indice)`**: byte en esa posición, o `-1` fuera de rango.
- **`a_entero()`**: interpreta los dígitos iniciales con signo; 0 si no hay.
- Con esto un programa puede **trocear y analizar** lo que lee de la entrada.
- Programa de conformidad `0032-texto-operaciones`, compilado y ejecutado.

### Sprint 40 — Errores como valores (2026-11-10)

- **`Resultado`**: tipo del lenguaje que representa un entero o un error de texto. `Ok(valor)` y
  `Error(mensaje)` lo construyen; se libera con su texto.
- **Operador `?`**: propaga el error sin comprobarlo a mano. Si el resultado es un error, la función
  devuelve ese mismo error.
- **Verificación de contexto**: `?` fuera de una función que devuelve `Resultado` es `NOK4011`.
- **`es_error()`, `valor()` y `mensaje()`** para tratar el resultado de forma explícita.
- **ADR-023** documenta la decisión, el límite (un solo tipo de resultado hasta los genéricos de
  tipos) y el coste (`?` solo en declaraciones).
- Programa de conformidad `0031-resultados`, compilado y ejecutado con las dos ramas.

## [0.4.0] — 2026-11-10

**La orientación a objetos está completa.** Clases por referencia, herencia simple, despacho por
tabla virtual, `super` y variables de tipo base con polimorfismo. Es el último pilar del lenguaje
tal como se concibió en los planes originales.

### Sprint 39 — Cierre de la v0.4 (2026-11-10)

- **Verificación de firma al sobrescribir**: una derivada que redefine un método virtual con otros
  parámetros o retorno recibe `NOK4010` en Nooki, en lugar de fallar en el C generado.
- **Ejemplo polimórfico entre módulos** (`0030-formas`): un módulo exporta la jerarquía y fábricas,
  y el programa principal guarda derivadas en variables de tipo base y las describe.
- **Especificación v0.4** y versión 0.4.0.

## [0.3.0] — 2026-11-10

**Biblioteca básica completa.** El lenguaje ya tiene estructuras con métodos, enumeraciones y
`coincide`, propiedad determinista, textos, colecciones (`Vector`, `Mapa`), genéricos y módulos con
visibilidad. Un programa puede repartirse en varios archivos.

### Sprint 38 — `super` y conversión a la base (2026-11-10)

- **`super.metodo(...)`**: una clase derivada invoca la implementación de su base sin despacho.
  `super.describe() + " cuadrada"` compone el comportamiento heredado con el propio.
- **Conversión a la base**: `variable f: figura = nuevo cuadrado { ... }` guarda un puntero derivado
  en una variable de tipo base, con conversión explícita en el C generado (la base está al
  principio de la estructura).
- **Polimorfismo real a través de la base**: `f.area()` despacha a la implementación de la clase
  concreta; `f.describe()` usa la heredada si la derivada no la redefine.
- **Un tipo de clase declarado siempre es una referencia**: el `*` lo pone el compilador.
- Programa de conformidad `0029-super`, compilado y ejecutado.

### Sprint 37 — Herencia y polimorfismo (2026-11-10)

- **`clase derivada hereda base`**: herencia simple con campos heredados primero y el puntero a la
  tabla virtual al principio, de modo que un puntero a derivada sirve como puntero a base.
- **Despacho por tabla virtual**: cada clase instala su tabla en `nuevo`, y una llamada a un método
  virtual va por la tabla. Dos derivadas de la misma base responden distinto al mismo mensaje.
- **Reutilización de implementaciones**: si una derivada no redefine un método, su tabla apunta a la
  de la base.
- **ADR-022** documenta la representación, el criterio de virtualidad y los límites (`super` y
  conversión a la base quedan para el sprint siguiente).
- Programa de conformidad `0028-herencia`, compilado y ejecutado.

### Sprint 36 — `clase` por referencia (2026-11-10)

- **`nuevo Tipo { ... }`**: una clase se reserva en el montículo y la variable es su dueña.
- **Misma regla de propiedad que el resto**: mover al asignar (`NOK5001`), prestar con `&` sin
  mover, y liberación determinista al cerrar el ámbito.
- **`estructura` sigue por valor**; ahora la diferencia es real y no una promesa.
- **ADR-021** reemplaza a ADR-015 y documenta la decisión, sus costes y lo que desbloquea.
- Programa de conformidad `0027-clases-referencia`, compilado y ejecutado.
- Corregido el paso de receptores prestados en colecciones tras generalizar los punteros.

### Sprint 35 — Espacios de nombres para tipos (2026-11-10)

- **`modulo.tipo`** en declaraciones, parámetros y campos: una estructura exportada por un módulo se
  usa desde fuera con su nombre cualificado (`figuras.punto`).
- **Nombres de C con prefijo**: `nooki_figuras_punto`, de modo que dos módulos pueden declarar
  `punto` sin chocar.
- **Visibilidad en tipos y enumeraciones**: `publico estructura`, `publico clase` y
  `publico enumeracion` en la cabecera de un archivo.
- **Estructuras devueltas por funciones**: una variable inicializada con una llamada cuyo tipo de
  retorno es una estructura recuerda ese tipo, así que sus métodos y campos se resuelven.
- **Comparación de tipos por definición**: `punto` y `figuras.punto` son la misma estructura aunque
  se escriban distinto.
- Programa de conformidad `0026-tipos-modulo`, con dos archivos, compilado y ejecutado.

### Sprint 34 — Programa interactivo y `claves()` (2026-11-10)

- **`mapa.claves() -> Vector<texto>`**: recorre las claves de un mapa. Las claves devueltas son
  vistas (el mapa sigue siendo el dueño), así que el vector se libera sin tocar los textos.
- **Ejemplo interactivo `0025-gastos`**: un módulo con un mapa, lectura de pares (concepto, importe)
  hasta el final de la entrada, recorrido de claves y total. Compila y ejecuta en cada `ctest`.
- **Corregido un error de orden en el C generado**: las liberaciones de un ámbito se emitían
  *después* de la llave de cierre de la función, es decir, fuera de la función. Solo se veía cuando
  una función declaraba una colección y no retornaba explícitamente.
- **Corregido el prefijo de las llamadas internas de un módulo**: `total(libro)` dentro del módulo
  `gastos` se emitía como `nooki_total` en lugar de `nooki_gastos_total`.

### Sprint 33 — Entrada estándar (2026-11-10)

- **`leer_linea() -> texto`**: lee una línea de la entrada estándar sin el salto y devuelve un texto
  con dueño. Al final de la entrada devuelve un texto vacío.
- **`leer_entero() -> entero`**: lee una línea y la interpreta como entero con signo; si no hay
  dígitos, devuelve 0.
- Ambas son integradas del lenguaje (no funciones de módulo) y respetan la propiedad: el texto que
  devuelven se libera al cerrar el ámbito.
- Un programa ya puede conversar: pide datos, los procesa y responde.
- Programa de conformidad `0024-entrada`, que compila y ejecuta en cada `ctest` con la entrada
  cerrada.

### Sprint 32 — `Mapa`: rehash y eliminación (2026-11-10)

- **`quitar(clave)`**: elimina la entrada, libera su clave y devuelve 1 si existía, 0 si no.
- **Rehash automático**: cuando el mapa pasa de tres cuartos de ocupación, duplica las cubetas y
  recoloca las entradas. `obtener` vuelve a ser O(1) medio con muchas claves.
- El caso de conformidad `0021-mapas` prueba inserción, reemplazo, consulta, eliminación y ausencia.

### Sprint 31 — Consolidación (2026-11-10)

- **Especificación v0.3** con las secciones 18.8 a 18.12: colecciones, mapas, módulos, propiedad en
  llamadas y los códigos nuevos. Ya no hay distancia entre lo que el compilador sabe y lo que está
  escrito.
- **Ejemplo real multifichero** (`0023-agenda`): un módulo con un mapa, préstamos mutables y textos,
  compilado y ejecutado en cada `ctest`.
- **Devuelto no es liberado**: devolver un valor con dueño de una función mueve la propiedad al
  llamante; antes se liberaba al salir del ámbito y quedaba colgando.
- **Un literal puede llegar a un parámetro prestado**: se le da dirección con un literal compuesto
  de C, en lugar de obligar a crear una variable.
- **Una variable local puede tapar el nombre de un módulo**, y se resuelve como método, no como
  módulo.
- v0.3.0.

### Sprint 30 — Módulos e importaciones (2026-11-10)

- **`usar modulo`** carga `modulo.oki` desde el directorio del archivo que lo importa, de forma
  recursiva. Los ciclos se deduplican en lugar de repetirse.
- **`modulo.simbolo`** para llamar a funciones de otro módulo; en C se traducen a
  `nooki_<modulo>_<funcion>`.
- **Visibilidad real**: solo `publico` es accesible desde fuera; usar un símbolo privado es
  `NOK3009`, y un módulo que no existe es `NOK3008` con la ruta buscada.
- Los módulos pueden importar a su vez otros módulos, y el orden no importa.
- Programa de conformidad `0022-modulos` con dos archivos, compilado y ejecutado.

### Sprint 29 — `Mapa<texto, entero>` (2026-11-10)

- **Tabla hash con encadenamiento**: `Mapa<texto, entero>` con `insertar`, `obtener`, `contiene` y
  `largo`.
- **Hash FNV-1a** sobre los bytes de la clave; inserción al principio de la cubeta, O(1) medio.
- **El mapa es dueño de las claves**: `insertar` transfiere el texto y anula el origen; al
  reemplazar una clave existente se libera la nueva y se conserva la que ya estaba.
- **Liberación determinista**: al cerrar el ámbito se sueltan todas las entradas, sus claves y el
  arreglo de cubetas.
- Programa de conformidad `0021-mapas`, compilado y ejecutado.

### Sprint 28 — Una sola regla de propiedad (2026-11-10)

- **Pasar un valor con dueño por valor lo mueve**: `consumir(a)` deja `a` vacío y usarlo después
  es `NOK5001`, con la sugerencia de prestarlo.
- **Prestar no mueve**: `prestado(&a)` deja `a` intacto.
- La regla vive **una sola vez** en el verificador y otra en el generador, mirando el tipo del
  parámetro; desaparece la lógica por tipo de dato que provocó el fallo de S27.
- **`&texto`** funciona como préstamo, también en `.largo()`.
- **ADR-020** documenta la decisión, el coste (la variable se invalida al pasarla) y el límite
  conocido (una llamada anidada todavía no mueve).
- Programa de conformidad `0020-propiedad-parametros`, compilado y ejecutado.

### Sprint 27 — `Vector<texto>` y dueño de los elementos (2026-11-10)

- **`Vector<texto>`**: colección de textos donde **el vector es el dueño** de cada elemento.
- **`agregar` transfiere la propiedad**: usar la variable de origen después es `NOK5001`, igual que
  con las cajas y los textos sueltos.
- **`obtener` devuelve una vista** (`propio = 0`): leer el elemento no lo libera ni lo duplica.
- `soltar` libera primero cada elemento y después el arreglo.
- Programa de conformidad `0019-vectores-texto`, compilado y ejecutado.
- **Corregido un fallo de corrupción de memoria**: la primera versión copiaba el texto al agregarlo
  y ambos lados lo liberaban al salir del ámbito. Lo detectó el propio programa con una excepción
  de montículo corrupto, no una prueba unitaria.

### Sprint 26 — Iteración sobre colecciones (2026-11-10)

- **`para (n en numeros)`**: recorre una colección por índice con una variable interna oculta; el
  cuerpo solo ve el elemento.
- **`Vector<T>` como parámetro prestado**: `&Vector<entero>` se traduce a puntero y el préstamo
  evita duplicar el dueño del arreglo.
- La verificación acepta `Vector<...>` como tipo válido y permite prestar variables que lo sean.
- Programa de conformidad `0018-iteracion`, compilado y ejecutado.

### Sprint 25 — `Vector<T>` (2026-11-10)

- **`Vector<entero>`**: arreglo dinámico en el montículo con `agregar`, `obtener`, `largo` y
  liberación determinista al cerrar el ámbito.
- **Crece multiplicando la capacidad** por dos, con `realloc`; `obtener` comprueba el índice y
  devuelve 0 fuera de rango en lugar de leer memoria ajena.
- **Misma propiedad que las cajas y los textos**: asignar mueve, y el liberador se elige por tipo.
- El parser admite tipos aplicados seguidos de `.` (`Vector<entero>.nuevo()`), no solo de `(`.
- Programa de conformidad `0017-vectores`, compilado y ejecutado.

### Sprint 24 — Genéricos por instanciación (2026-11-10)

- **Funciones genéricas**: `funcion identidad<T>(valor: T) -> T`, con instanciación explícita
  `identidad<entero>(42)` e `identidad<texto>("Nooki")`.
- **Una función de C por combinación de tipos**, con nombre mangleado (`nooki_identidad_texto`) y
  emitida una sola vez. Sin despacho en ejecución.
- **Descubrimiento diferido**: los cuerpos se generan primero en un búfer; las instanciaciones se
  emiten después, hasta que no aparezcan nuevas.
- **ADR-019** documenta el mecanismo, sus límites (sin inferencia) y el coste (una instancia por
  combinación).
- Programa de conformidad `0016-genericos`, compilado y ejecutado.

### Sprint 23 — `texto` de verdad (2026-11-10)
- **Tipo `texto`** con longitud: `datos`, `largo` y marca `propio` (ADR-018). Los literales no se
  liberan; las concatenaciones sí.
- **Concatenación** con `+` y **comparación por contenido** con `==`, `!=`, `<`, `>`, `<=`, `>=`.
- **`.largo()`** en bytes, en tiempo constante.
- **Propiedad**: asignar un texto lo mueve y usar el origen después es `NOK5001`. Liberación
  determinista al cerrar ámbito y antes de cada `retornar`.
- **Mezclar texto y entero** en una operación sigue siendo error (`NOK4001`).
- `imprimir` acepta `texto` en cualquier posición: literal, variable, parámetro o resultado de una
  función.
- Programa de conformidad `0015-textos`, compilado y ejecutado.

### Sprint 22 — Préstamos mutables y exclusividad (2026-11-09)

**Primera versión con memoria determinista.** El compilador ya escribe programas con
estructuras, objetos con métodos, enumeraciones, emparejamiento de patrones, cajas con dueño
único y préstamos que no mueven. Compila a binario nativo en Windows, Linux y macOS con
GCC/Clang/MSVC como cadenas de compilación.

- **`&mutable caja`** y parámetros `&mutable Caja<entero>`: escribir a través del préstamo sin
  mover la caja.
- **Escritura a través de un préstamo inmutable** → `NOK4009`, con la alternativa en el mensaje.
- **Exclusividad comprobada**: en una misma llamada no puede haber `&mutable x` junto a otro
  préstamo de `x` (`NOK5003`). Es la regla de préstamos de Rust verificada donde hoy puede
  violarse.
- **Versión 0.2.0** y **especificación v0.2** con la sección 18, que documenta todo lo añadido
  desde el Sprint 17 y la tabla completa de códigos de diagnóstico.
- Ejemplo real de conformidad (`0014-amortizacion`): un préstamo con estructura, método, caja y
  préstamo mutable, compilado y ejecutado.

### Sprint 21 — Préstamos inmutables (2026-11-09)

- **`&caja`**: préstamo inmutable que **no mueve**. La variable prestada sigue siendo válida y se
  libera donde siempre.
- **Tipo `&Caja<entero>`** en parámetros, traducido a `nooki_caja*` en C. El parser de tipos admite
  ahora `&T`, `&mutable T`, `*T` y genéricos `Caja<T>`.
- **Regla**: un préstamo vive durante la expresión que lo crea. Guardarlo en una variable es error
  `NOK5002`, con la explicación y la alternativa.
- **Prestar algo que no es una caja** → `NOK4008`.
- **ADR-017** documenta la regla, el límite (no se pueden devolver préstamos) y que `&mutable` se
  acepta pero aún no se distingue.
- Programa de conformidad `0012-prestamos`, compilado y ejecutado.

### Sprint 20 — `Caja<T>`: propiedad única y movimiento (2026-11-09)
- **`Caja<T>`**: valor en el montículo con **un único dueño**. `Caja.nuevo(41)` asigna; `*c` lee y
  escribe; al salir del ámbito el dueño libera.
- **Movimiento**: `variable d = c` transfiere la propiedad y anula el origen. Usar `c` después es
  **error de compilación** (`NOK5001`), con span y explicación.
- **Liberación determinista**: el generador registra las cajas de cada ámbito y emite
  `nooki_caja_soltar` al cerrarlo y antes de cada `retornar`. `soltar` es nulo-seguro, así que un
  movimiento no produce doble liberación.
- **`*` sobre algo que no es una caja** → `NOK4007`.
- **ADR-016** documenta la decisión, los límites (solo `Caja<entero>` hasta los genéricos) y el
  riesgo conocido con `romper`/`continuar`.
- **Corregido un fallo grave del parser**: un operador al principio de una línea nueva continuaba la
  expresión anterior. `variable x = 1` seguido de `*p = 2` se interpretaba como `1 * p = 2`, es
  decir, dos sentencias se fusionaban en silencio.
- Programa de conformidad `0011-cajas`, compilado y ejecutado.

### Sprint 19 — Enumeraciones y `coincide` (2026-11-09)

- **Enumeraciones**: `enumeracion color { rojo, verde, azul }`, con variantes accesibles como
  `color.rojo`.
- **Emparejamiento de patrones**: `coincide (valor) { caso 1 => ... caso color.rojo => ... caso _ => ... }`
  con literales, variantes y comodín, en una sola línea o con bloque.
- **Generación**: enumeraciones como constantes enteras y `coincide` como cadena `if/else` sobre una
  temporal, para no evaluar la expresión más de una vez.
- **Verificación**: variante inexistente (`NOK3007`) y `coincide` sin comodín (`NOK4006`).
- El lexer acepta `_` como comodín sin romper los identificadores que empiezan por guion bajo.
- Programa de conformidad `0010-patrones`, compilado y ejecutado.
- **Decisión registrada**: la semántica de referencia para `clase` (y con ella la herencia) se
  pospone a la fase de memoria. Un conteo de referencias a medias produce dobles liberaciones, que
  son corrupción de memoria; hacerlo bien exige liberar en todas las salidas de ámbito e incrementar
  en cada copia. Se hará una sola vez, junto al ownership.

### Sprint 18 — Métodos y `este` (2026-11-09)

- **Métodos dentro de tipos**: `publico funcion suma(otro: punto) -> entero { ... }`, con parámetros
  y retorno como cualquier función.
- **`este`** para acceder a los campos propios: `este.x`, `este.x = ...`.
- **Llamadas a métodos**: `p.suma(q)` se traduce a `nooki_punto_suma(&p, q)`, el mismo modelo que
  necesitará la herencia con tablas virtuales.
- **Verificación de métodos**: miembro inexistente (`NOK3006`), aridad (`NOK4003`) y tipo de
  argumento (`NOK4002`), incluidos los parámetros cuyo tipo es otra estructura.
- Programa de conformidad `0009-metodos`, compilado y ejecutado.
- El parser acepta `este` como inicio de sentencia (`este.x = ...`).

### Sprint 17 — Estructuras y clases con campos (2026-11-09)

- **Declaraciones de tipo**: `estructura punto { publico x: entero }` y `clase persona { ... }`,
  con visibilidad por campo (`publico`, `privado`, …).
- **Construcción con campos nombrados**: `punto { x: 1, y: 2 }`, traducida a literal compuesto de C.
- **Acceso y asignación de campos**: `p.x`, `p.x = 5`, incluido dentro de expresiones.
- **Verificación de la construcción**: tipo inexistente (`NOK3003`), campo inexistente (`NOK3004`),
  campo que falta (`NOK3005`) y tipo del valor incorrecto (`NOK4001`).
- **ADR-015**: en la v0.1, `clase` y `estructura` comparten representación por valor porque la
  semántica de referencia necesita el modelo de memoria. ADR-001 sigue vigente como objetivo.
- Programa de conformidad `0007-estructuras`, compilado y ejecutado.
- 4 casos nuevos de sentencias y 6 de verificación.

### Sprint 16 — Verificación de tipos (2026-11-09)

- **Verificador semántico** con ámbitos: nombres declarados, aridad de llamadas, tipos de
  argumentos, tipos de operadores, condiciones y retornos.
- **Cinco diagnósticos nuevos**, todos en español y con span:
  `NOK4001` tipo incompatible · `NOK4002` argumento incompatible · `NOK4003` aridad incorrecta ·
  `NOK4004` retorno incompatible · `NOK4005` condición no booleana.
- **`si`, `mientras` y `hacer` exigen condición booleana**: `si (1)` ya no compila. Es la defensa
  contra `si (x = 1)`, el error clásico de C.
- **Un solo `Tipo` para todo el compilador** (`semantico/tipos.hpp`): el generador deja de tener su
  propia noción de tipo y usa la misma.
- 22 casos de verificación (10 válidos, 12 inválidos) que comparan el código exacto esperado.
- El hueco que motivó el sprint queda cerrado: `factorial("texto")` ya no llega al compilador de C.

### Sprint 15 — Funciones con parámetros y primera verificación semántica (2026-11-09)

- **Funciones con parámetros**: `funcion factorial(n: entero) -> entero`, con cualquier número de
  parámetros y tipos (`entero`, `texto`, `booleano`).
- **Llamadas con argumentos** traducidas a C, con declaraciones anticipadas: la recursión funciona
  (el factorial recursivo se compila y ejecuta).
- **Inferencia del tipo de una llamada** a partir del tipo de retorno declarado, para que
  `imprimir(factorial(i))` elija la función de impresión correcta.
- **Primera verificación semántica**: `NOK3002` cuando se llama a una función que no existe, con
  span y sugerencia. Antes, ese error aparecía como un mensaje del compilador de C en inglés.
- Dos programas nuevos de conformidad, compilados y ejecutados: factorial recursivo y saludo con
  parámetro de texto.
- 37 comprobaciones de sentencias (4 nuevas sobre parámetros).

### Sprint 14 — El backend C compila variables y control de flujo (2026-11-09)

- **El generador traduce ahora el lenguaje que el parser ya entendía**: variables y constantes con
  tipo explícito o inferido, `si`/`sino`, `mientras`, `para` sobre rangos con `paso`, `hacer`,
  `bucle`, `romper`, `continuar`, bloques y asignaciones.
- **Aritmética y comparaciones** en C, y `**` mediante la función `nooki_potencia` del preludio.
- **`imprimir` distingue enteros de textos**: `nooki_imprimir_entero` y `nooki_imprimir_texto`.
- **Ámbito de tipos mínimo** (pila de mapas) para inferir el tipo de C de cada variable; no es el
  sistema de tipos del lenguaje, que llega en S20.
- **Declaraciones anticipadas** de funciones: se puede llamar a una función definida después.
- Tres programas nuevos de conformidad, **compilados y ejecutados**: FizzBuzz (1..15), Fibonacci
  (10 términos) y tabla de multiplicar con bucles anidados.
- El backend sigue negándose con `NOK6003` ante lo que no sabe traducir (flotantes, `coincide`,
  índice, conversiones) en lugar de generar código incorrecto.

### Sprint 13 — Sentencias y control de flujo (2026-11-09)

- **Sentencias completas**: `variable`/`constante` con tipo opcional, `si`/`sinosi`/`sino`,
  `mientras`, `para` (con `paso`), `hacer`, `bucle`, `romper`, `continuar`, `retornar`, bloques y
  expresiones sueltas.
- **`sinosi` encadenado** sin límite: se representa como un `si` anidado en la rama `sino`.
- **Nueva línea significativa**: `retornar` sin valor se distingue de `retornar x` por el salto de
  línea previo, no por mirar el token siguiente.
- **Impresor de sentencias y programas**: `(para i en (.. 0 10) paso 2 (si ...))`.
- **Comando `nooki ast`**: muestra el árbol canónico de un archivo `.oki`.
- **33 comprobaciones**: 27 programas válidos y 6 inválidos.
- `coincide` produce `NOK2004` con la explicación de que llega en el Sprint 17, en lugar de un error
  de sintaxis críptico.

### Sprint 12 — Parser de expresiones con precedencia (2026-11-09)

- **Parser Pratt** con la tabla de precedencia completa de la especificación §7.1: 15 niveles, con
  asociatividad izquierda, derecha y no asociativa donde corresponde.
- **AST de expresiones** propio: literales, identificador, unaria, binaria, llamada, miembro,
  miembro opcional, índice, propagación, conversión y asignación.
- **Impresor canónico** (`(+ 1 (* 2 3))`) que fija la precedencia por escrito.
- **197 comprobaciones** de precedencia y asociatividad, más 7 expresiones que deben fallar.
- Los rangos encadenados (`a..b..c`) son error `NOK2005`, no asociación implícita.
- El backend avisa con `NOK6003` en lugar de generar C incorrecto cuando el árbol usa algo que aún
  no sabe traducir.
- Dos decisiones de la tabla documentadas y registradas: los operadores de bits se unen más fuerte
  que las comparaciones (como Python, no como C), y `-2 ** 2` = `(-2) ** 2` (asunto A-08 a revisar).

### Sprint 6 — Rebanada vertical: "Hola, Nooki" (2026-11-09)

- **El primer programa Nooki compila y se ejecuta**: `.oki` → parser → C → binario nativo PE/ELF/Mach-O.
- **Backend C** (ADR-014): se genera C y se invoca el compilador del sistema (`cc`, `gcc`, `clang`,
  o `NOOKI_CC`). Decisión motivada porque no hay LLVM en la máquina y porque MSVC y MinGW no pueden
  enlazar las librerías de LLVM entre sí.
- **`nooki compilar`** (con `-o`, `--emitir-c`, `--conservar`) y **`nooki ejecutar`**.
- Parser descendente recursivo mínimo: funciones, bloques, llamadas, literales y `retornar`.
- Generador de C: runtime `imprimir`, traducción de tipos, nombres y separadores numéricos.
- Caso de conformidad `0000-hola-mundo` que **compila y compara la salida real** del binario.
- 13 comprobaciones del compilador; la suite completa queda en 5 pruebas.
- Corrección de dos fallos de plataforma: `cmd` recortaba las comillas iniciales de los comandos, y
  la redirección de Windows convertía `\n` en `\r\n` y hacía fallar toda comparación.

### Sprint 5 — Arnés de conformidad y analizador léxico (2026-11-09)

- **Arnés de conformidad** (`nooki_conformidad`): recorre `pruebas/conformidad/`, ejecuta cada caso
  y compara los códigos obtenidos con los esperados; informa del rendimiento del lexer.
- **22 casos de conformidad**: 12 válidos (léxico completo) y 10 inválidos con su código esperado.
- **Analizador léxico** de la v0.1: 84 palabras reservadas con búsqueda binaria verificada en
  compilación (`static_assert`), identificadores con tildes y eñe, enteros en cuatro bases,
  flotantes, textos con escapes y multilínea, caracteres y tiempos de vida, comentarios anidados y
  de documentación.
- Diagnóstico `NOK1001` con sugerencia cuando una palabra clave se escribe con tilde.
- 33 comprobaciones unitarias del lexer.
- Enlazado **totalmente estático** en MinGW: elimina la dependencia de `libwinpthread-1.dll`, que
  provocaba un fallo de arranque cuando el sistema tenía otra versión en el `PATH`.
- Línea base de rendimiento: **26,7 MB/s · 5,6 M tokens/s** (corpus de 128 KB).
- Re-corte del plan: el lexer pasa de S06 a S05 (el arnés necesitaba un consumidor real); S06 queda
  como parser mínimo + LLVM + enlace.

### Sprint 4 — Sistema de diagnósticos (2026-11-09)

- `nooki/utf8`: decodificador propio que detecta secuencias inválidas sin abortar.
- `GestorFuentes`: registro de archivos y conversión de desplazamientos en bytes a línea y columna
  contadas en puntos de código (ADR-008).
- Códigos `NOKnnnn` con el esquema por fases de la especificación §11.1.
- Catálogo bilingüe español/inglés: los mensajes son código + parámetros, no texto ya montado.
- Render en texto con canaleta, caretos, etiquetas, notas y ayudas; color ANSI opcional con
  detección de terminal y respeto a `NO_COLOR`.
- Render JSON para herramientas y el futuro LSP.
- 5 casos dorados de diagnóstico y 1 dorado JSON, más 11 comprobaciones de catálogo, color y
  ubicaciones.
- Corrección de C4996 en MSVC mediante `_dupenv_s` en lugar de silenciar avisos globalmente.

### Sprint 3 — Andamiaje del repositorio (2026-11-09)

- CMake 3.25+ con presets para MSVC 2022/2026, MinGW, GCC y Clang.
- Objetivo `nooki_nucleo` (biblioteca) + ejecutable `nooki`.
- Cabecera `nooki/version.hpp` generada desde CMake: única fuente de verdad de la versión.
- CLI con `version`, `ayuda`, códigos de salida documentados y sugerencia ante errores de tecleo.
- Pruebas de la CLI con `ctest` (9 casos, sin dependencias externas).
- Integración continua en Windows, Linux y macOS; trabajos informativos de formato y análisis.
- Enlace estático de la biblioteca estándar en MinGW: el binario es autocontenido.
- Licencia dual MIT/Apache-2.0, guía de contribución y plantilla de RFC.

### Sprint 2 — Especificación (2026-11-09)

- `spec/gramatica.ebnf`: gramática normativa completa (125 reglas).
- `spec/especificacion-v0.1.md`: léxico, tipos, OOP, memoria, errores, FFI y conformidad.
- 63 palabras clave + 21 nombres de tipo, sin tildes; identificadores normalizados a NFC.

### Sprint 1 — Decisiones (2026-11-09)

- 13 ADR aprobadas en `docs/adr/`.
- Renombrado del proyecto a **Nooki**, extensión `.oki`.
- Plan recalculado para un solo desarrollador; UI e IA pasan a post-1.0.
