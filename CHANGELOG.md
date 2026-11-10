# Historial de cambios

El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/) y el proyecto usa
[versionado semántico](https://semver.org/lang/es/).

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
