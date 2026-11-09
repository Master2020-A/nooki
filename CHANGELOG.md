# Historial de cambios

El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/) y el proyecto usa
[versionado semántico](https://semver.org/lang/es/).

## [Sin publicar]

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
