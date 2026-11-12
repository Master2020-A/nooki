# NOOKI — Plan de implementación por sprints

> **Estado:** documento de planificación. **No contiene código.**
> **Base adoptada:** `Aurora2.txt` como especificación canónica, con las correcciones del Anexo A.
> **Objetivo:** lenguaje compilado, 100 % en español, multiplataforma (Windows / macOS / Linux), escrito en C++20 con backend LLVM.
> **Nombre:** **Nooki** · extensión de fuentes: **`.oki`** · CLI: **`nooki`**
> **Unidad de planificación:** sprint de 2 semanas (alcance). **Escenario real: 1 persona → ~3,5–4 semanas por sprint.**

---

## 0. Veredicto: ¿se puede seguir con alguno de los dos archivos?

**Sí, se puede seguir — pero solo con uno: `Aurora2.txt`.**

| Documento | Acople con tu visión | Veredicto |
|---|---|---|
| `Aurora2.txt` (2 895 líneas) | **~85 %** | **Base canónica.** Cubre OOP completa, ownership estilo Rust, sintaxis estilo Python, UI, IA (pandas/PyTorch), NLP, IA generativa, multiplataforma, C++ + LLVM |
| `Aurora1.txt` (1 213 líneas) | ~55 % | **No sirve como base**: es un subconjunto del anterior con otra sintaxis. Se archiva como histórico y se rescatan **dos ideas** (ver abajo) |

### Lo que hay que rescatar de `Aurora1.txt`

1. **Nombres de tipos primitivos en minúscula** (`entero`, `cadena`, `booleano`) en lugar de `Entero`/`Texto`/`Booleano`. Es más coherente con el objetivo "fácil como Python": menos mayúsculas arbitrarias y menos ruido visual. *(Decisión pendiente en ADR-02.)*
2. **`prestamo` como palabra clave** en lugar de `&`/`&mut`. Más legible para principiantes hispanohablantes, aunque menos familiar para quien viene de C/Rust. *(Decisión pendiente en ADR-02.)*

### Lo que NO se puede seguir tal cual (ni de `Nooki1` ni de `Nooki2`)

Ninguno de los dos es implementable hoy, por cinco razones — todas ya corregidas en este plan:

| # | Problema en los planes originales | Corrección en este plan |
|---|---|---|
| 1 | **Dos especificaciones contradictorias** (extensión `.nooki`/`.oki`, constructor `crear`/`constructor`, `prestamo`/`&`, 28–36 vs 48 meses) | Sprint 1: congelar base única + 12 ADR |
| 2 | **FFI después de la stdlib** (mes 18–22 vs 14–18): la stdlib no se puede escribir sin libc | S37: FFI **antes** de toda la stdlib |
| 3 | **Compilación incremental en la fase final** (mes 42–48): condiciona el diseño del AST y del LSP | S57: arquitectura *query-based* desde S30 |
| 4 | **IA y UI como núcleo** (10 meses para "PyTorch + pandas + NLP + visión") | Fases 10–11 como **paquetes sobre FFI** (ONNX, BLAS, Skia), no reimplementaciones |
| 5 | **Sin decisiones semánticas** (¿clase por valor o referencia? ¿overflow? ¿excepciones o `Resultado`?) | Sprint 1: 12 ADR que hay que cerrar antes de escribir la primera línea |

---

## 1. Trazabilidad: tu visión → el plan

| Tu requisito | ¿Está en `Nooki2`? | Viabilidad | Dónde se implementa |
|---|---|---|---|
| Manejo de hardware como C/C++ | Parcial (falta `asm!` e `inseguro` operativo, falta `core`/`no_std`) | ✅ Viable | Fase 6 (S40, S43) |
| Seguridad de memoria como Rust | Parcial (falta `Send`/`Sync`, modelo de memoria) | ✅ Viable, es lo más caro | Fase 7 (S45–S52) |
| Facilidad como Python | Sí en sintaxis; el ownership lo complica | ⚠️ Viable con mitigaciones | Fase 4 + "modo didáctico" (S92) |
| OOP: clases, herencia, polimorfismo | Sí, completo | ✅ Viable | S16, S26, S33 |
| Lenguaje compilado | Sí (LLVM) | ✅ Viable | Fase 5 |
| Librerías UI fáciles | Sí en intención; **3 toolkits nativos es inviable** | ⚠️ Recortado a 1 stack | Fase 10 (Skia + capa de ventanas) |
| Librerías IA tipo pandas/PyTorch | Enumerado, pero **"desde cero" es inviable** | 🔴 Reframado a FFI | Fase 11 (ONNX, BLAS, Arrow) |
| NLP e IA generativa | Enumerado | ⚠️ Vía ONNX + APIs | S86, S87 |
| **Todo en español** | Sí, pero sin política Unicode ni i18n del tooling | ✅ Con ADR-08 | Fase 1–2 |
| Escrito en C++ | Sí (C++20) | ✅ Viable | Todo el proyecto |
| Windows + macOS + Linux | Sí, pero sin estrategia de enlace ni de firma | ✅ Viable | S03, S34, S94 |

**Nota honesta sobre "IA desde cero":** PyTorch equivale a cientos de ingenieros-año. Lo que sí es viable —y es lo que propone este plan— es una **API en español idiomática** sobre ONNX Runtime, BLAS/LAPACK y Arrow. El usuario final no nota la diferencia; el esfuerzo baja de 10 años a 10 sprints.

---

## 2. Las 12 decisiones a cerrar antes del Sprint 1

Sin esto, cualquier sprint posterior rehace trabajo. Cada una se documenta como ADR de una página (decisión + alternativas + consecuencias).

| # | Pregunta | Recomendación por defecto |
|---|---|---|
| ADR-01 | ¿Una `clase` es valor o referencia? | **Híbrido**: `clase` = referencia (heap, identidad, herencia); `estructura` = valor (stack, copia). Como C#/Swift |
| ADR-02 | ¿`variable` mutable por defecto? ¿Tipos en minúscula? | **Mutable por defecto** (estilo Python) + `constante` inmutable; primitivos en **minúscula** (`entero`, `cadena`); préstamos con `&`/`&mut` |
| ADR-03 | ¿Overflow de enteros? | **Comprobado por defecto** (panic), con `envolver()` explícito. Coherente con el pilar "seguridad" |
| ADR-04 | ¿Errores: `Resultado` o excepciones? | **`Resultado` + `?` como primario.** `intentar/atrapar` solo en fronteras (FFI/panic) |
| ADR-05 | ¿Async/await en v1.0? | **No.** Hilos + canales primero (S42); async diferido a v1.1 |
| ADR-06 | ¿Macros y atributos? | **Una sola sintaxis de atributos (`@`)**; doc comments `///`; macros declarativas en v0.7; sin macros procedurales |
| ADR-07 | ¿Cómo ejecutar programas antes de que exista el borrow checker? | **RC/GC temporal** (S44) con fecha de caducidad: se elimina en S52. Deuda técnica *declarada* |
| ADR-08 | ¿Acentos y `ñ` en identificadores? | **Keywords 100 % ASCII sin tildes** (`funcion`, `tamano`); tildes/`ñ` permitidas en identificadores de usuario, normalizados **NFC**; rutas de archivo ASCII |
| ADR-09 | ¿Herencia virtual por defecto? | **Explícita**: `@virtual` para permitir override, `@sobrescribir` obligatorio. Mejor ABI y rendimiento |
| ADR-10 | ¿ABI y *name mangling*? | `@repr(C)` para FFI; mangling propio documentado y estable; sin `extern` implícito |
| ADR-11 | ¿Ensamblador en línea? | **Sí**, `asm!` dentro de `inseguro { }`, desde S40. Sin esto no hay "control de hardware como C" |
| ADR-12 | ¿Licencia, gobernanza y versionado? | **MIT OR Apache-2.0** + excepción de runtime; semver; "ediciones" tipo Rust; RFC para cambios de lenguaje |

**Extra administrativo (no bloquea sprint):** verificar disponibilidad de la marca "Nooki" y del dominio antes de invertir en identidad visual — ya existe [Nooki (SNN)](https://core.ac.uk/download/613982078.pdf), [Latino](https://www.languajelatino.org/) y [Águila](https://aguila.compilandocode.com/) en el espacio de lenguajes en español.

---

## 3. Reglas de ejecución (guardarraíles)

1. **Todo sprint termina con `main` verde en Windows, Linux y macOS.** Si no compila en los tres, no está terminado.
2. **Ninguna funcionalidad sin caso de conformidad.** El test es el contrato; la documentación es derivada.
3. **Todo error nuevo es de primera clase:** código (`NOKxxxx`), span, y sugerencia de arreglo en español.
4. **Un sprint = una funcionalidad grande.** WIP = 1 por desarrollador.
5. **Demo de 5 minutos al final de cada sprint**, grabada. Sin demo, no hay sprint.
6. **Deuda técnica con fecha de caducidad.** Toda excepción se anota con el sprint en que se paga (ver ADR-07).
7. **`inseguro` prohibido en la stdlib** salvo lista blanca revisada en code review.
8. **Fuzzing continuo desde S10**, en nightly, con corpus acumulado.
9. **Benchmarks desde S05.** Toda regresión > 5 % bloquea el merge.
10. **Idioma:** documentación, mensajes y **nombres de identificadores en C++** en español (coherente con el proyecto). Commits en español.
11. **Cobertura mínima del compilador: 85 %.** No baja nunca.
12. **Cero funcionalidades nuevas durante el cierre de cada fase** (feature freeze).

### Definition of Done (plantilla)

- [ ] Código en `main`, revisado por al menos una persona (o auto-revisión documentada si estás solo).
- [ ] Tests de conformidad añadidos (válidos e inválidos).
- [ ] CI verde en 3 sistemas operativos.
- [ ] Diagnóstico de errores con span, código y sugerencia.
- [ ] Documentación actualizada (`docs/`).
- [ ] Benchmark sin regresión.
- [ ] Demo grabada de 5 minutos.

---

## 4. El plan: 96 sprints en 12 fases

Leyenda: **DoD** = condición verificable de "terminado". Cada fila es un sprint de 2 semanas.

---

### Fase 1 — Cimientos y especificación (S01–S06)

**Entrada:** dos planes en texto plano. **Salida:** repositorio, especificación v0.1, "Hola, Nooki" nativo en 3 sistemas.

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S01** | Cerrar las 12 ADR y congelar la base | `docs/adr/ADR-001..012.md` aprobadas; `Aurora1.txt` movido a `historico/`; tabla de decisiones cerrada |
| **S02** | Especificación v0.1 + gramática EBNF normativa | `spec/gramatica.ebnf` validada por un checker; `spec/especificacion-v0.1.md` con léxico, sintaxis, tipos y semántica; lista de **no-objetivos** |
| **S03** | Andamiaje del repositorio | ✅ Git + CMake presets (MSVC 2022/2026, MinGW, GCC, Clang) + estructura de carpetas + CI en 3 OS + `nooki --version` + clang-format/tidy + ASan/UBSan configurado. *vcpkg y LLVM se incorporan en S06, cuando se necesiten de verdad* |
| **S04** | Sistema de diagnósticos | ✅ `GestorFuentes` + `Span`, códigos `NOKxxxx`, catálogo es/en, render en texto con caretos y en JSON, color con `NO_COLOR`; 5 casos dorados + 11 comprobaciones |
| **S05** | Arnés de conformidad y primer consumidor | ✅ `nooki_conformidad` + 22 casos (12 válidos / 10 inválidos) + **lexer completo** (se adelanta de S06: el arnés necesitaba algo real que probar) + línea base 26,7 MB/s |
| **S06** | **Rebanada vertical: "Hola, Nooki" nativo** | ✅ `.oki` → parser → C → binario nativo en las dos cadenas de Windows. `nooki compilar` y `nooki ejecutar`. Backend C en lugar de LLVM (ADR-014) |

> **S06 es el sprint más importante del proyecto.** Demuestra que la cadena completa funciona antes de invertir años en semántica. Es también la demo que te permite conseguir colaboradores.

---

### Fase 2 — Análisis léxico (S07–S11) — ⏭️ ABSORBIDA POR EL SPRINT 5

> **Nota de re-corte (2026-11-09).** Esta fase se adelantó al Sprint 5: el arnés de conformidad
> necesitaba un consumidor real y el lexer es la base de todo lo que sigue. Lo que queda de la fase
> es el endurecimiento (`fuzzing` continuo, corpus de 500 casos, interning y rendimiento), que se
> reparte entre los sprints S12–S19 y la fase de rendimiento (S92). **El siguiente sprint es S12.**

**Entrada:** gramática v0.1. **Salida:** lexer completo, probado y fuzzeado.

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S07** | Palabras clave y operadores | Tabla definitiva (sin tildes, ADR-08); 100 % de operadores de la spec; test por token |
| **S08** | Literales numéricos y de texto | Dec/hex/bin/octal, separadores `_`, sufijos; escapes, raw strings, multilínea; 100 tests |
| **S09** | UTF-8 e identificadores | Decodificador propio; normalización NFC; tildes y `ñ` en identificadores; rechazo con sugerencia de keyword acentuada (`función` → `funcion`); tests de normalización |
| **S10** | Ubicaciones, errores y recuperación | Línea/columna/offset; errores `NOK1xxx` con span y sugerencia; 0 crashes con fuzz de 1 h |
| **S11** | Rendimiento y cierre | String interning + arena; lexer > 50 MB/s; fuzz 24 h en nightly; cobertura > 90 % |

---

### Fase 3 — Sintaxis y AST (S12–S19)

**Entrada:** tokens. **Salida:** parser completo del lenguaje; hito **v0.2**.

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S12** | AST base, visitor y arena | ✅ AST de expresiones completo + **parser Pratt** con la tabla de §7.1 + impresor canónico + 197 casos de precedencia + 7 inválidas. *La arena y el visitante se posponen hasta que el tamaño del AST los justifique* |
| **S13** | Sentencias y control de flujo | ✅ `variable`/`constante`, `si`/`sinosi`/`sino`, `mientras`, `para` con `paso`, `hacer`, `bucle`, `romper`, `continuar`, bloques; impresor de programas y `nooki ast`; 33 casos. *El Pratt de expresiones se adelantó al S12* |
| **S14** | Backend C: variables y control de flujo | ✅ Variables y constantes con tipo, `si`/`sino`, `mientras`, `para` sobre rangos, `hacer`, `bucle`, `romper`, `continuar`, aritmética, comparaciones y `imprimir` de enteros. FizzBuzz, Fibonacci y tabla de multiplicar **compilados y ejecutados**. *El parser de sentencias se adelantó al S13* |
| **S15** | Funciones con parámetros y verificaciones | ✅ Parámetros tipados, llamadas con argumentos, recursión, inferencia del tipo de retorno y **primera verificación semántica** (`NOK3002`). *El parser de tipos genéricos se pospone a S16, junto con OOP* |
| **S16** | Verificación de tipos básica | ✅ Nombres declarados, aridad, tipos de argumentos, operadores, condiciones booleanas y retornos compatibles; 22 casos. *El parser de OOP pasa a S17* |
| **S17** | Estructuras y clases con campos | ✅ `estructura`/`clase`, campos con visibilidad, construcción `punto { x: 1 }`, acceso y asignación, verificación de campos. *El emparejamiento de patrones pasa a S18* |
| **S18** | Métodos y `este` | ✅ Métodos en tipos, `este` para campos propios, llamadas `p.metodo(args)` traducidas a funciones con el objeto como puntero, y verificación de miembro/aridad/tipos. *La recuperación de errores multi-diagnóstico pasa a S19* |
| **S19** | Enumeraciones y emparejamiento | ✅ `enumeracion` con variantes, `coincide` con literales, variantes y comodín, verificación de variante y exhaustividad, generado como `if/else` sobre una temporal. *La semántica de referencia y la herencia se posponen a la fase de memoria (asunto A-09)* |

---

### Fase 4 — Semántica y sistema de tipos (S20–S28)

**Entrada:** AST. **Salida:** checo semántico completo con tipos, genéricos, traits y OOP; hito **v0.3**.

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S20** | `Caja<T>` y movimiento | ✅ Dueño único en el montículo, movimiento al asignar, uso tras mover como error (`NOK5001`), liberación determinista al cerrar ámbito y antes de `retornar`. ADR-016. *La tabla de símbolos ya existía desde S16* |
| **S21** | Préstamos de alcance de expresión | ✅ `&caja`, tipo `&Caja<entero>`, prestar sin mover, `&mutable` aceptado sin distinguirse todavía; guardar un préstamo es error (`NOK5002`), prestar algo que no es caja (`NOK4008`). ADR-017. *El `TypeId` internado ya existe en `semantico/tipos.hpp`* |
| **S22** | Chequeo de expresiones y sentencias | Tipo de toda expresión; conversiones explícitas; errores con span; 150 tests |
| **S23** | `texto` con operaciones | ✅ Tipo `texto` con longitud y marca de propiedad (ADR-018), concatenación con `+`, comparación por contenido, `.largo()` en O(1) y movimiento con `NOK5001`. *La iteración sobre texto y los caracteres UTF-8 llegan con el runtime completo* |
| **S24** | Genéricos por instanciación | ✅ Funciones genéricas `<T>` con argumentos de tipo explícitos, sustitución de tipos y una función de C por combinación (ADR-019). *La inferencia y los tipos genéricos llegan con las colecciones* |
| **S25** | `Vector<T>` | ✅ `Vector<entero>` con `agregar`, `obtener` y `largo`, crecimiento por duplicación, comprobación de índice y liberación determinista. *La iteración en `para` y los elementos que no son enteros llegan en el sprint siguiente* |
| **S26** | Iteración sobre colecciones | ✅ `para (n en vector)` como recorrido por índice con variable interna, y `&Vector<T>` como parámetro prestado. *`Vector<texto>` y `quitar`/`insertar` quedan pendientes* |
| **S27** | `Vector<texto>` | ✅ Colección de textos con dueño: `agregar` mueve el elemento (`NOK5001`), `obtener` devuelve una vista y la liberación suelta elementos y arreglo. *`Mapa<K, V>` queda pendiente* |
| **S28** | Regla única de propiedad en llamadas | ✅ Pasar un valor con dueño por valor lo mueve (`NOK5001`); prestar no mueve; una sola implementación en verificador y generador (ADR-020). *Las llamadas anidadas quedan pendientes* |

---

### Fase 5 — IR y generación de código (S29–S36)

**Entrada:** AST anotado. **Salida:** programas reales compilados a nativo en 3 sistemas; hito **v0.4**.

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S29** | `Mapa<texto, entero>` | ✅ Tabla hash con encadenamiento, FNV-1a, `insertar`/`obtener`/`contiene`/`largo`, claves propias y liberación determinista. *La eliminación de claves queda pendiente* |
| **S30** | Módulos e importaciones | ✅ `usar`, `modulo.simbolo`, visibilidad `publico`/`privado`, carga recursiva con deduplicación de ciclos y traducción a nombres con prefijo. *Los tipos y enumeraciones de un módulo se comparten en un espacio global; falta delimitarlos* |
| **S31** | Consolidación y v0.3 | ✅ Especificación §18.8–18.12, ejemplo multifichero real, devolución con transferencia, literales para préstamos, sombreado de módulos y versión 0.3.0 |
| **S32** | `Mapa`: rehash y `quitar` | ✅ Duplicación de cubetas al pasar de 3/4 de ocupación con recolocación de entradas, y `quitar` que libera la clave y devuelve si existía |
| **S33** | Entrada estándar | ✅ `leer_linea()` y `leer_entero()` como integradas, con texto propio y comportamiento definido al final de la entrada |
| **S34** | Programa interactivo y `claves()` | ✅ `mapa.claves()` como vistas, ejemplo `0025-gastos` con módulo y entrada, y dos correcciones de generación (liberaciones fuera de la función y prefijo de llamadas internas) |
| **S35** | Espacios de nombres para tipos | ✅ `modulo.tipo` en declaraciones, parámetros y campos, nombres de C con prefijo, visibilidad en tipos y enumeraciones, y comparación por definición |
| **S36** | `clase` por referencia | ✅ `nuevo Tipo { ... }` reserva en el montículo, la variable es dueña, mueve al asignar y libera al salir; accesos con puntero y liberación por tipo (ADR-021). *Reemplaza ADR-015* |

---

### Fase 6 — Runtime, FFI y núcleo (S37–S44)

**Entrada:** codegen estable. **Salida:** el lenguaje controla el hardware y el sistema operativo; hito **v0.5**.

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S37** | Herencia y polimorfismo | ✅ `hereda`, campos aplanados, tabla virtual por jerarquía, despacho en las llamadas y reutilización de implementaciones (ADR-022) |
| **S38** | `super` y conversión a la base | ✅ `super.metodo()`, puntero derivado en variable base con conversión, polimorfismo a través de la base y tipos de clase siempre por referencia. *La verificación de firmas al sobrescribir queda pendiente* |
| **S39** | Cierre de la v0.4 | ✅ Verificación de firma al sobrescribir (`NOK4010`), ejemplo polimórfico entre módulos, especificación v0.4 y versión 0.4.0 |
| **S40** | Errores como valores | ✅ `Resultado` con `Ok`/`Error`, operador `?` que propaga en declaraciones, comprobación explícita por métodos y verificación de contexto (ADR-023) |
| **S41** | Operaciones de texto | ✅ `buscar`, `subtexto`, `contiene`, `empieza_con`, `caracter` y `a_entero`, con copia propia en `subtexto` y comprobación de índices. *Las variantes con datos quedan para el sprint siguiente* |
| **S42** | Variantes con datos | ✅ Variantes con campos, unión etiquetada, construcción por nombre, patrones que ligan (`caso Bien(v)`), cobertura sin comodín y liberación de campos de texto (ADR-024) |
| **S43** | Biblioteca en Nooki | ✅ Módulo `resultado` escrito en Nooki e integrado en el compilador, `?` estructural sobre cualquier enumeración con forma de resultado, y retirada del tipo especial (ADR-025) |
| **S44** | La biblioteca crece | ✅ `texto.dividir` que devuelve un vector, `mayusculas`, `minusculas`, `recortar` y el módulo `matematicas` escrito en Nooki |

---

### Fase 7 — Ownership y préstamos (S45–S52)

**Entrada:** codegen + runtime. **Salida:** seguridad de memoria del nivel prometido; hito **v0.6 (lenguaje completo)**. *Es la fase más difícil: no se recorta.*

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S45** | Paquetes: proyectos | ✅ `usar` busca junto al importador y en `biblioteca/` subiendo por los padres, y `nooki construir` compila el proyecto tomando su punto de entrada |
| **S46** | Genéricos de tipos | ✅ `estructura par<T>` con instanciación explícita (`par<entero>`), monomorficación por combinación y registro previo de campos para resolver accesos |
| **S47** | Cierre de la v0.5 | ✅ Auditoría de los 41 códigos de diagnóstico (todos se emiten), programa de cierre con todo el lenguaje, limitación de devolución de compuestos documentada y versión 0.5.0 |
| **S48** | Cerrar A-10 | ✅ El retorno de un compuesto transfiere sus variables con dueño y la construcción las mueve; el caso indirecto también queda cerrado. *Queda una fuga: los compuestos no liberan sus campos* |
| **S49** | Compuestos que liberan | ✅ `soltar` por instanciación para compuestos con campos con dueño, liberación solo de los frescos y movimiento de campos insertados en colecciones |
| **S50** | Cerrar los huecos de propiedad | ✅ Copia de compuestos con dueño rechazada (`NOK5004`), `romper`/`continuar` liberan lo que abandonan. *Queda la llamada anidada que mueve* |
| **S51** | Cerrar A-11 y v0.6 | ✅ La llamada anidada que movería un valor con dueño es error de compilación; caso `0020` migrado; versión 0.6.0 sin defectos conocidos |
| **S52** | Herramientas del proyecto | ✅ `nooki nuevo` crea un proyecto que compila, `nooki probar` ejecuta y compara con `.esperado`, y `nooki limpiar` borra artefactos. Búsqueda de módulos desde rutas absolutas |

---

### Fase 8 — Herramientas (S53–S61)

**Entrada:** lenguaje completo. **Salida:** toolchain usable por terceros; hito **v0.7**.

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S53** | Guía en español | ✅ `GUIA.md` en diez pasos con la salida real de cada ejemplo, `LEEME.md` de presentación e instalación, y verificación de todos los fragmentos ejecutándolos |
| **S54** | Rasgos | ✅ `rasgo` con firmas y `implementa` en tipos; verificación de método faltante (`NOK3011`) y firma incompatible (`NOK4012`), sin coste en ejecución |
| **S55** | `&Rasgo` | ✅ Referencia a rasgo con objeto y tabla, tabla por (tipo, rasgo) con adaptadores, despacho dinámico y conversión en variables y argumentos |
| **S56** | Cierre de la v0.7 | ✅ ADR-026 de rasgos, guía con paso 11, decisiones al día y versión 0.7.0 |
| **S57** | Tiempos de vida: escape | ✅ Devolver un préstamo a una variable local es error (`NOK5006`); devolver un préstamo recibido como parámetro está permitido. *La duración entre variables del mismo ámbito sigue sin verificarse* |
| **S58** | Arnés: casos inválidos en todas las fases | ✅ Los `invalido/` pasan por léxico, parser y verificación; cuatro casos semánticos nuevos (`NOK5001`, `NOK3011`, `NOK5004`, `NOK5006`) |
| **S59** | Duración del préstamo | ✅ Mover o reasignar el dueño con un préstamo vivo es error (`NOK5007`); prestar una clase conserva una sola referencia. *La duración temporal (`&x` vivo entre sentencias) sigue sin verificarse* |
| **S60** | Cierre de la v0.8 | ✅ Auditoría de los 46 códigos (todos se emiten y tienen mensaje; 36 con caso), tres códigos nuevos con prueba y versión 0.8.0 |
| **S61** | Biblioteca en Nooki | ✅ Módulo `textos` (`unir`, `repetir`, `contar`) con caso propio, tipos de `Vector<texto>` y de `*prestamo` corregidos, y auto-importación rechazada. *La biblioteca embebida no registra módulos nuevos (A-13)* |

---

### Fase 9 — Librería estándar (S62–S70)

**Entrada:** lenguaje + toolchain. **Salida:** stdlib completa y probada; hito **v0.8**.

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S62** | A-13: biblioteca embebida | ✅ No era un fallo: un archivo viejo tapaba el módulo. `textos` va embebido y un módulo con `principal` se rechaza |
| **S63** | Biblioteca: listas y fechas | ✅ Módulos `listas` (`suma`, `mayor`, `menor`, `promedio`, `contiene`) y `fechas` (`bisiesto`, `dias_del_mes`, `dias_del_anio`) embebidos, con caso propio |
| **S64** | Cierre de la v0.9 | ✅ Prueba de coherencia documental en `ctest` (encuentra sprints sin entrada y versiones desalineadas), guía con paso 12 sobre la biblioteca y versión 0.9.0 |
| **S65** | Las vías del movimiento | ✅ La comprobación de préstamo se centraliza y cubre reasignación, declaración y argumento por valor; caso `0017`. *Quedan anotados A-14 (mover un campo a una colección) y el límite de prestar un parámetro de clase* |
| **S66** | A-14: mover un campo | ✅ Vaciar un texto pone su longitud a cero, los métodos heredados cuentan como miembros, la variable construida recuerda su tipo y `imprimir` acepta booleanos |
| **S67** | Cierre de la 1.0 | ✅ Notas de versión, inventario de pendientes con tamaños y versión 1.0.0 en todo el proyecto |
| **S68** | Preparar la publicación | ✅ Licencias completas (A-06), `CONTRIBUIR.md` al día, `PUBLICAR.md` con los pasos y CI revisada |
| **S69** | Ejemplos y resaltado | ✅ Cuatro ejemplos ejecutables, extensión de VS Code (TextMate), sintaxis de Vim e instrucciones para otros editores; corregida la ejecución con rutas `/` en Windows |
| **S70** | Verificación fuera de Windows | ✅ GCC 13.3, Clang 18.1 y ASan+UBSan en Linux: compilación sin avisos y 9/9 suites en las tres cadenas. Eliminados dos avisos y añadido el preset `saneamiento` que faltaba. *macOS sin verificar (A-15)* |

---

### Fase 10 — Framework de UI (S71–S78)

**Decisión de arquitectura:** **un solo stack de renderizado** (capa de ventanas multiplataforma + Skia), no tres toolkits nativos. Tres backends nativos triplican el esfuerzo y garantizan inconsistencias visuales.

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S71** | Fuzzing y benchmarks | ✅ `nooki_fuzz` (100 000 entradas, 5 semillas, sin caídas) encontró y cerró un fallo real: escape inválido al final del archivo. `nooki_rendimiento` da la línea base: 25–33 MB/s de lexer y 0,003 s por 1 000 líneas |
| **S72** | Render 2D y texto | Skia, HarfBuzz/FreeType; texto con tildes y `ñ` correctos; benchmark de *frame* |
| **S73** | Layout y estilos | Flex/Grid/Stack; temas claro/oscuro; 60 tests visuales |
| **S74** | Widgets básicos | Ventana, Botón, Etiqueta, CampoTexto, Casilla, Combo, Deslizador, Progreso; 60 tests |
| **S75** | Widgets avanzados | Tabla, Árbol, Pestañas, Diálogos, Selector de archivo, Canvas; 50 tests |
| **S76** | Eventos y animaciones | Callbacks, gestos, transiciones; 40 tests |
| **S77** | Accesibilidad e i18n de UI | Navegación por teclado, lectores de pantalla, textos traducibles; auditoría |
| **S78** | **Cierre v0.9: herramientas de UI** | Vista previa/designer; ejemplos: calculadora y gestor de tareas |

---

### Fase 11 — Ecosistema de IA (S79–S88)

**Estrategia:** FFI primero. Se envuelve lo que ya existe (OpenBLAS, ONNX Runtime, Arrow) y se expone una API en español. **No se reimplementa PyTorch.**

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S79** | Tensores | Tensor N-dimensional, *strides*, operaciones, broadcasting; backend BLAS; 100 tests |
| **S80** | Autograd | Grafo computacional, retropropagación, gradientes; 40 tests |
| **S81** | Redes neuronales | Capas (densa, convolucional, pooling, normalización, dropout, embedding), activaciones, pérdidas, optimizadores; 60 tests |
| **S82** | Entrenamiento y checkpoints | Entrenador, validación, callbacks, guardar/cargar; MNIST de punta a punta |
| **S83** | MarcoDatos (estilo pandas) | Columnar, CSV/JSON/Excel, filtrar/agrupar/unir/pivotar, describir; 80 tests + benchmark vs pandas |
| **S84** | ML clásico | Regresión, árboles, bosques, KMeans, PCA, métricas, validación cruzada; 80 tests |
| **S85** | ONNX Runtime | Carga e inferencia de modelos ONNX; GPU opcional; 30 tests |
| **S86** | PLN | Tokenizadores (BPE/SentencePiece), embeddings, sentimiento, entidades, resumen; 50 tests |
| **S87** | IA generativa | Cliente OpenAI/Anthropic/Ollama, generación local GGUF, RAG con base vectorial; 40 tests |
| **S88** | **Cierre v0.95: visión y GPU** | Imágenes, clasificación y detección vía ONNX; CUDA/Metal/Vulkan si hay recursos |

---

### Fase 12 — Estabilización y v1.0 (S89–S96)

| Sprint | Objetivo | Entregable verificable (DoD) |
|---|---|---|
| **S89** | Estabilidad de API | Congelar la API de la std; MSRV; ediciones; política de deprecación |
| **S90** | Conformidad y regresión | 100 % del corpus; 0 regresiones; golden tests en 3 sistemas |
| **S91** | Auditoría de seguridad | Revisión de `inseguro`, fuzzing de 72 h, sanitizers, modelo de amenazas del gestor de paquetes |
| **S92** | Rendimiento final y "modo didáctico" | Optimización del compilador; benchmarks vs C/Rust/Go; mensajes de error pedagógicos para principiantes |
| **S93** | Documentación completa | Especificación 1.0, libro en español, tutoriales, referencia de API, guía de contribución |
| **S94** | Instaladores y distribución | MSI, DEB/RPM, Homebrew, WinGet; **PKG y notarización de macOS condicionados a un Mac o financiación** (D-17); firma de código; actualizador |
| **S95** | *Release candidate* | RC congelado; pruebas en 3 sistemas; migración de proyectos piloto |
| **S96** | **NOOKI v1.0** | Release pública, sitio, registro público; *spike* de autohospedaje (compilador escrito en Nooki) |

---

## 5. Duración real — escenario confirmado: 1 persona

**Decisión tomada (2026-11-09): el proyecto lo desarrolla una sola persona.** Eso **no cambia el orden** de los sprints —la secuencia sigue siendo la correcta— pero fija el calendario:

| Escenario | Personas | Semanas por sprint | 96 sprints | Lenguaje completo (S52) |
|---|---|---|---|---|
| **Real: desarrollador solo** | **1** | **3,5–4** | **~7–7,5 años** | **~4 años** |
| Referencia: pareja | 2 | 3 | ~5,5 años | ~3 años |
| Referencia: equipo pequeño | 3–4 | 2,5 | ~4,5–5 años | ~2,5 años |

### 5.1 Modo solitario: qué se recorta

El trabajo total no cambia; con una persona el único ajuste posible es **el alcance**. Se aplican cinco decisiones (registradas en [DECISIONES.md](DECISIONES.md) como D-13 a D-17):

1. **La v1.0 no incluye UI ni IA.** Las fases 10 (UI, S71–S78) y 11 (IA, S79–S88) pasan a *post-1.0*. La v1.0 queda en **78 sprints** (S01–S70 + los 8 de estabilización) ≈ **5,5–6 años**. Razón: una UI multiplataforma y un stack de IA son dos carreras separadas; el lenguaje, su tooling y su stdlib no lo son.
2. **Sprint nominal de 2 semanas, sprint real de 3,5–4.** No se renumeran los 96 sprints: se conserva la unidad de alcance y se ajusta el calendario.
3. **Registro de paquetes por Git, no servidor propio.** S55 se reduce a un índice en un repositorio + archivos estáticos. Un servidor con autenticación, firma y moderación solo se justifica cuando haya usuarios.
4. **Un solo binario `nooki`** (compilador + CLI + gestor de paquetes) hasta la v0.7. Separarlo en `nookic`/`nooki` es trabajo de equipo, no de una persona.
5. **macOS se valida en CI** (runners gratuitos de GitHub Actions). La firma y notarización de S94 quedan condicionadas a disponer de un Mac o de financiación.

**Calendario realista con una persona** (3,75 semanas por sprint):

| Hito | Sprint | Acumulado |
|---|---|---|
| "Hola, Nooki" nativo en 3 sistemas (demo para conseguir ayuda) | S06 | **~5–6 meses** |
| Parser completo del lenguaje | S19 | ~1,4 años |
| Semántica y tipos completos | S28 | ~2 años |
| Programas reales compilados a nativo | S36 | ~2,6 años |
| Hardware, FFI y `core` | S44 | ~3,2 años |
| **Ownership completo — lenguaje terminado** | **S52** | **~3,75–4 años** |
| Tooling, LSP y paquetes | S61 | ~4,4 años |
| Librería estándar completa | S70 | ~5 años |
| **v1.0 (sin UI ni IA)** | 78 sprints | **~5,5–6 años** |
| UI + IA (post-1.0) | +18 sprints | ~7–7,5 años |

> **La consecuencia práctica:** la decisión "solo yo" convierte el proyecto en un compromiso de **cinco a siete años**. La demo de S06 en ~6 meses es lo que decide si esos años se recorren solo o con compañía.

---

## 6. Paralelización: mapa de dependencias

> **Con una persona no hay paralelización**: las vías se recorren en serie. Esta tabla se conserva como **mapa de dependencias** (qué desbloquea qué) y como plan de incorporación el día que se sume alguien: la primera persona adicional entra por la vía C (tooling) o D (stdlib).

Después de S36 el proyecto se abre en seis vías simultáneas:

| Vía | Sprints | Depende de | Puede avanzar a la vez que |
|---|---|---|---|
| **A — Front-end** | S07–S28 | S06 | — |
| **B — Backend / IR** | S29–S36 | S28 | A (parcial), C |
| **C — Tooling** | S53–S61 | S36 | D, E, F |
| **D — Stdlib** | S62–S70 | S44 | C, E, F |
| **E — UI** | S71–S78 | S44 | C, D, F |
| **F — IA** | S79–S88 | S70 | C, D, E |
| **G — Ownership** | S45–S52 | S44 | **nada**: es crítico, serializado |

> **Regla:** la vía G (ownership) nunca se paraleliza con otra cosa. Es donde los errores de diseño se pagan dos veces.

---

## 7. Riesgos por fase y mitigación

| Riesgo | Fase | Probabilidad | Impacto | Mitigación |
|---|---|---|---|---|
| El borrow checker se vuelve inmanejable | 7 | Alta | Muy alto | Subconjunto primero (S45–S48 sin *lifetimes* explícitos); corpus de Rust como oráculo |
| Integración con LLVM consume más de lo previsto | 5 | Media | Alto | Rebanada vertical en S06; versión de LLVM **fijada** exacta, no "17+" |
| La UI se descontrola | 10 | Alta | Alto | Un solo stack (Skia); widgets en sprints cerrados; sin "pixel-perfect" nativo |
| El ecosistema IA no arranca | 11 | Media | Medio | FFI desde el primer día; sin reimplementar motores |
| Se pierde el rumbo por falta de equipo | Todas | **Muy alta** | **Muy alto** | Demo en S06; opensource temprano; RFC público desde S19 |
| macOS requiere hardware Apple para firmar | 12 | Alta | Medio | CI en *runners* macOS; notarización en S94 con cuenta Apple Developer desde S03 |
| Deriva de especificación | Todas | Media | Alto | Una sola fuente de verdad; cambios por RFC; conformance como contrato |
| "Fácil como Python" no se cumple | 4, 7, 92 | Media | Alto | Modo didáctico (S92); mensajes con causa y arreglo; inferencia agresiva |

---

## 8. Métricas de seguimiento (tablero por sprint)

| Métrica | Objetivo | Dónde se mide |
|---|---|---|
| Casos de conformidad que pasan | 100 % en cada cierre de fase | CI |
| Cobertura del compilador (C++) | ≥ 85 % | CI |
| Tiempo de compilación (1 000 LOC) | < 2 s | `nooki bench` |
| Tiempo de compilación (10 000 LOC) | < 5 s | `nooki bench` |
| Rendimiento del código generado | ≤ 110 % del tiempo de C equivalente | Benchmarks fib/nbody/matmul |
| Horas de fuzzing sin crash | Acumulado creciente | Nightly |
| Bugs abiertos por severidad | 0 críticos al cierre de fase | Gestor de incidencias |
| Programas de ejemplo que compilan | 100 % | CI |

---

## 9. No-objetivos de la v1.0 (lo que **no** se hará)

Declararlos por escrito evita el 80 % del *scope creep*:

- ❌ Macros procedurales (solo declarativas).
- ❌ `async/await` en el núcleo (diferido a v1.1).
- ❌ Backend propio de CPU (solo LLVM).
- ❌ UI móvil nativa (solo escritorio; web vía WASM en v1.2).
- ❌ Reimplementar PyTorch, pandas o TensorFlow (solo FFI).
- ❌ Compilador autohospedado en v1.0 (solo *spike* en S96).
- ❌ Soporte de Windows 32 bits, ARM32 y sistemas exóticos.
- ❌ Verificación formal del *borrow checker* (RustBelt) — aspiracional, no compromiso.

---

## 10. Detalle semana a semana de los 3 primeros sprints

### S01 — Decisiones (2 semanas)

| Semana | Actividad | Salida |
|---|---|---|
| 1 (días 1–2) | Redactar ADR-01 a ADR-06 | 6 documentos de 1 página |
| 1 (días 3–4) | Redactar ADR-07 a ADR-12 | 6 documentos de 1 página |
| 1 (día 5) | Revisión cruzada de las 12 ADR | Lista de objeciones |
| 2 (días 1–2) | Resolver objeciones y firmar | ADR aprobadas |
| 2 (día 3) | Archivar `Aurora1.txt` y declarar `Aurora2.txt` canónico | Repo ordenado |
| 2 (días 4–5) | Demo + retrospectiva | Video de 5 min |

### S02 — Especificación (2 semanas)

| Semana | Actividad | Salida |
|---|---|---|
| 1 | Léxico: keywords definitivas, operadores, literales, comentarios | Sección de léxico |
| 1 | Gramática EBNF de expresiones y tipos | `gramatica.ebnf` (parte 1) |
| 2 | Gramática EBNF de declaraciones, OOP, patrones y módulos | `gramatica.ebnf` (parte 2) |
| 2 | Semántica v0.1 + lista de no-objetivos | `especificacion-v0.1.md` |
| 2 | Validación de la gramática con el 100 % de los ejemplos | Informe de validación |

### S03 — Andamiaje (2 semanas)

| Semana | Actividad | Salida |
|---|---|---|
| 1 (días 1–2) | Repositorio, licencia, `.gitignore`, estructura de carpetas | Repo inicial |
| 1 (días 3–5) | `CMakeLists.txt` raíz + presets + vcpkg con LLVM fijado | Build local en las 3 plataformas |
| 2 (días 1–2) | CI: matriz Windows/Linux/macOS, caché de LLVM, ASan/UBSan | Pipeline verde |
| 2 (día 3) | CLI `nooki` que imprime versión y ayuda | `nooki --version` |
| 2 (día 4) | clang-format, clang-tidy, hooks de pre-commit | Estilo aplicado en CI |
| 2 (día 5) | Demo + retrospectiva | Video de 5 min |

---

## Anexo A — Correcciones aplicadas al plan original

| # | Plan original (`Aurora2.txt`) | Este plan | Razón |
|---|---|---|---|
| A1 | FFI en Fase 5 (mes 18–22), después de la stdlib | FFI en S37, **antes** de toda la stdlib | Sin libc no hay archivos, red ni tiempo |
| A2 | Incremental y paralelismo en Fase 9 (mes 42–48) | Arquitectura *query-based* en S30; incremental funcional en S57 | Condiciona AST, símbolos y LSP |
| A3 | Programas ejecutables solo "cuando el ownership exista" | GC/RC temporal (S44) eliminado en S52 | Permite usar el lenguaje 2 años antes |
| A4 | UI con tres toolkits nativos (Win32 + GTK4 + Cocoa) | Una capa de ventanas + Skia | Tres backends triplican el esfuerzo |
| A5 | IA "desde cero" en 10 meses | FFI a ONNX/BLAS/Arrow con API en español | PyTorch son cientos de ingenieros-año |
| A6 | OOP completa en Fase 5, después de la stdlib | Semántica de OOP en S26 y codegen en S33, antes de la stdlib | La stdlib usa clases, genéricos y traits |
| A7 | 10 fases horizontales de 4 años | 12 fases y 96 sprints con DoD verificable | Sin criterio de terminado no hay control |
| A8 | Dos documentos contradictorios | Una base canónica + 12 ADR | No se puede implementar dos especificaciones |
| A9 | Sin `asm!` ni `no_std` | S40 y S43 | Sin esto no existe el "control de hardware como C" |
| A10 | Sin `Send`/`Sync` ni modelo de memoria | S51 y S42 | Sin ellos la promesa "0 data races" es falsa |

---

## Anexo B — Aprobaciones (cerrado el 2026-11-09)

| # | Pregunta | Respuesta | Consecuencia |
|---|---|---|---|
| 1 | ¿12 ADR por defecto? | ✅ **Sí** | Cerradas en `docs/adr/` (13 ADR con la derivada) |
| 2 | ¿Sprints de 2 semanas o 1? | ✅ **2 semanas** | 96 sprints |
| 3 | ¿Cuántas personas? | ✅ **3–4 devs** (línea base) | **4,5–5 años**; con 1 persona, 6,5–7,5 años |
| 4 | ¿Recorte de alcance (IA/UI por FFI)? | ✅ **Sí** | Fases 10 y 11 sobre ONNX/Skia/BLAS |
| 5 | ¿Nombre "Nooki" y extensión `.oki`? | ✅ **Sí** | Verificar marca y dominio antes de S03 (asunto A-01) |
| 6 | ¿Licencia MIT/Apache-2.0? | ✅ **Sí** | + excepción de runtime (ADR-012) |

**Artefactos producidos tras la aprobación:**

- `docs/adr/ADR-001` … `ADR-013` — 13 decisiones cerradas (Sprint 1).
- `DECISIONES.md` — índice, decisiones revisables y asuntos abiertos.
- `spec/gramatica.ebnf` — gramática normativa completa (Sprint 2).
- `spec/especificacion-v0.1.md` — especificación del lenguaje (Sprint 2).

---

*Documento de planificación — no contiene código. Versión 1.1 (aprobado).*
