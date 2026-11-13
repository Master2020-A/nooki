# DECISIONES — Registro de decisiones del proyecto NOOKI

> **Última actualización:** 2026-11-09
> Este archivo es un índice. El detalle normativo está en cada ADR (`docs/adr/`) y en la especificación (`spec/`).

---

## 1. Decisiones cerradas (aprobadas)

Las 13 ADR del Sprint 1 están **aprobadas**. No se editan: si algo cambia, se crea un ADR nuevo que cita al anterior.

| ADR | Decisión en una línea | Estado |
|---|---|---|
| [ADR-001](docs/adr/ADR-001-modelo-de-objetos.md) | `clase` = referencia (sin `nulo`), `estructura` = valor | ✅ Aprobada |
| [ADR-002](docs/adr/ADR-002-mutabilidad-y-nombres.md) | `variable` mutable / `constante` inmutable; primitivos en minúscula; `texto` | ✅ Aprobada |
| [ADR-003](docs/adr/ADR-003-aritmetica-entera.md) | Desbordamiento comprobado siempre; `envolver()` para *wrap* | ✅ Aprobada |
| [ADR-004](docs/adr/ADR-004-modelo-de-errores.md) | `resultado<T,E>` + `?`; `intentar` solo para pánico | ✅ Aprobada |
| [ADR-005](docs/adr/ADR-005-async-fuera-de-v1.md) | `async`/`await` fuera de la v1.0 (reservadas) | ✅ Aprobada |
| [ADR-006](docs/adr/ADR-006-atributos-y-macros.md) | Atributos con `@`; `///` para documentar; macros en v0.7 | ✅ Aprobada |
| [ADR-007](docs/adr/ADR-007-memoria-transitoria.md) | RC temporal de S44 a S52, con fecha de caducidad | ✅ Aprobada |
| [ADR-008](docs/adr/ADR-008-unicode-e-identificadores.md) | Keywords ASCII sin tildes; identificadores normalizados a NFC | ✅ Aprobada |
| [ADR-009](docs/adr/ADR-009-virtualidad-explicita.md) | No virtual por defecto; `@virtual` + `@sobrescribir` obligatorios | ✅ Aprobada |
| [ADR-010](docs/adr/ADR-010-abi-y-ffi.md) | `@repr(C)`, *mangling* propio estable, pánicos no cruzan FFI | ✅ Aprobada |
| [ADR-011](docs/adr/ADR-011-ensamblador-e-intrinsecos.md) | `asm!` en `inseguro`, operandos nombrados, sintaxis Intel | ✅ Aprobada |
| [ADR-012](docs/adr/ADR-012-licencia-y-gobernanza.md) | MIT OR Apache-2.0 + excepción de runtime; RFC, ediciones, semver | ✅ Aprobada |
| [ADR-013](docs/adr/ADR-013-decisiones-sintacticas.md) | `interfaz`=`rasgo`; `nuevo` invariable; NL termina sentencia; sin turbofish | ✅ Aprobada |
| [ADR-014](docs/adr/ADR-014-backend-c.md) | Backend C en la v0.1; LLVM más adelante detrás de la misma interfaz | ✅ Aprobada (S06) |
| [ADR-015](docs/adr/ADR-015-clases-por-valor-en-v01.md) | En la v0.1, `clase` y `estructura` se compilan igual, por valor | 🔁 Reemplazada por ADR-021 (S36) |
| [ADR-016](docs/adr/ADR-016-caja-y-movimiento.md) | `Caja<T>`: dueño único, movimiento y liberación determinista | ✅ Aprobada (S20) |
| [ADR-017](docs/adr/ADR-017-prestamos-de-expresion.md) | Préstamos `&` de alcance de expresión; no se pueden guardar | ✅ Aprobada (S21) |
| [ADR-018](docs/adr/ADR-018-texto-con-marca-de-propiedad.md) | `texto` con marca de propiedad (`propio`) en lugar de dos tipos | ✅ Aprobada (S23) |
| [ADR-019](docs/adr/ADR-019-genericos-por-instanciacion.md) | Genéricos por instanciación explícita; sin inferencia todavía | ✅ Aprobada (S24) |
| [ADR-020](docs/adr/ADR-020-regla-unica-de-propiedad.md) | Pasar un valor con dueño por valor lo mueve; prestar no mueve | ✅ Aprobada (S28) |
| [ADR-021](docs/adr/ADR-021-clases-por-referencia.md) | `clase` por referencia con `nuevo`; reemplaza a ADR-015 | ✅ Aprobada (S36) |
| [ADR-022](docs/adr/ADR-022-herencia-y-despacho.md) | Herencia simple con campos aplanados y despacho por tabla virtual | ✅ Aprobada (S37) |
| [ADR-023](docs/adr/ADR-023-errores-como-valores.md) | Errores como valores: `Resultado`, `Ok`/`Error` y `?` | ✅ Aprobada (S40) |
| [ADR-024](docs/adr/ADR-024-variantes-con-datos.md) | Variantes con datos y patrones que ligan | ✅ Aprobada (S42) |
| [ADR-025](docs/adr/ADR-025-biblioteca-en-nooki.md) | La biblioteca estándar se escribe en Nooki; `Resultado` deja de ser especial | ✅ Aprobada (S43) |
| [ADR-026](docs/adr/ADR-026-rasgos.md) | Rasgos como contrato verificado y `&Rasgo` con despacho por tabla | ✅ Aprobada (S54–S55) |

---

## 2. Aprobaciones del Anexo B del plan

| Pregunta | Respuesta | Consecuencia |
|---|---|---|
| ¿13 ADR por defecto? | **Sí** | Cerradas en `docs/adr/` |
| ¿Sprints de 2 semanas? | **Sí** (2 semanas de alcance) | 96 sprints; en solitario cada uno ocupa ~3,5–4 semanas reales |
| ¿Cuántas personas? | **1 (solo el autor)** | **Lenguaje completo ~4 años · v1.0 ~5,5–6 años · todo ~7–7,5 años** |
| ¿Recorte de alcance (IA/UI por FFI)? | **Sí**, y además **post-1.0** (D-14) | v1.0 = 78 sprints, sin UI ni IA |
| ¿Nombre y extensión? | **`nooki`** y **`.oki`** (2026-11-09) | Renombrado aplicado a todo el repositorio |
| ¿Licencia MIT/Apache-2.0? | **Sí** | + excepción de runtime (ADR-012) |

### 2.1 Nomenclatura oficial

| Elemento | Nombre | Nota |
|---|---|---|
| Lenguaje | **Nooki** (en código y rutas: `nooki`) | Nombre propio en prosa, minúsculas en identificadores |
| Extensión de fuentes | **`.oki`** | p. ej. `principal.oki` |
| CLI / compilador | **`nooki`** | Un solo binario hasta v0.7 (D-16) |
| Manifiesto de proyecto | `nooki.toml` | |
| Lockfile | `nooki.lock` | |
| Configuración del formateador | `.nooki-formato.toml` | |
| Espacios de nombres | `nooki.nucleo`, `nooki.io`, `nooki.texto`… | |
| Paquetes oficiales | `nooki_std`, `nooki_ui`, `nooki_ia` | |
| Registro de paquetes | índice Git + estáticos (D-15); dominio por confirmar | |
| Gestor de paquetes | `nooki paquete …` | Se **descarta** el nombre "Constelación" (era temático de Aurora) |

---

## 3. Decisiones tomadas al escribir la especificación (revisables hasta S06)

Resueltas para poder escribir la gramática. **Tienen valor por defecto y se pueden vetar antes de S06 sin coste**, porque aún no hay código.

| # | Decisión tomada | Alternativa si la vetas | Impacto del cambio |
|---|---|---|---|
| D-01 | `texto` como nombre del tipo cadena | `cadena` | Renombrar en gramática, spec y ejemplos (1 hora) |
| D-02 | `natural` para enteros sin signo (`natural8`…`natural128`) | `entero_sin_signo`, `ess8` | Renombrar tabla de tipos (1 hora) |
| D-03 | Miembros `privado` por defecto | `publico` por defecto | Cambia ejemplos y linter (2 horas) |
| D-04 | `&mutable` (alias aceptado `&mut`) | Solo `&mut`, o `prestamo mutable` | Cambia el lexer (1 hora) |
| D-05 | Sin sufijos numéricos (`5`, no `5u`) | Sufijos tipo Rust | Añadir regla léxica (2 horas) |
| D-06 | `principal()` y `principal(argumentos: vector<texto>) -> entero` | Solo una firma | Cambia la resolución del punto de entrada (1 hora) |
| D-07 | `para i en 0..10 paso 2` con `paso` como palabra reservada | Método `.paso(2)` | Añadir/quitar palabra reservada |
| D-08 | `_` como comodín en `coincide` (sin `defecto`) | `defecto` como alias | Añadir palabra reservada |
| D-09 | Cierres con `|x| expr` | `funcion(x) { }` | Cambia el parser de expresiones |
| D-10 | Los módulos se derivan de la ruta del archivo; `modulo` es opcional | Declaración obligatoria | Cambia la resolución de módulos |
| D-11 | `interna` como cuarta visibilidad (visible en el módulo) | Solo tres niveles | Añadir palabra reservada |
| D-12 | Cadenas multilínea con `"""` | Solo `\n` escapado | Regla léxica extra |

---

## 4. Modo solitario — decisiones de alcance (2026-11-09)

| # | Decisión | Por qué |
|---|---|---|
| D-13 | **Sprint de alcance = 2 semanas; sprint real = 3,5–4 semanas.** No se renumeran los 96 sprints | La rejilla de sprints es la unidad de alcance, no el calendario |
| D-14 | **La v1.0 no incluye UI ni IA**: las fases 10 y 11 pasan a post-1.0. v1.0 = 78 sprints | UI e IA son dos carreras separadas; el lenguaje y su stdlib no |
| D-15 | **Registro de paquetes por Git + archivos estáticos** en lugar de un servidor propio | Ahorra semanas de desarrollo y toda la operación; un servidor solo se justifica con usuarios |
| D-16 | **Un solo binario `nooki`** (compilador + CLI + paquetes) hasta v0.7 | Menos superficie que mantener; se separa cuando haya equipo |
| D-17 | **macOS se valida en CI**; firma y notarización condicionadas a un Mac o financiación | Sin hardware Apple no hay notarización posible |

---

## 5. Asuntos abiertos

| # | Asunto | Estado | Cuándo |
|---|---|---|---|
| A-01 | **Marca y dominio de "Nooki"** | 🟡 Parcialmente resuelto | Ver §5.1 |
| A-02 | **Equipo** | ✅ **Cerrado:** 1 persona | — |
| A-03 | Infraestructura del registro de paquetes | 🟡 Parcial: búsqueda en `biblioteca/` y biblioteca integrada (S45, S52). Falta un registro público | Fase de paquetes |
| A-04 | Política de seguridad y divulgación de vulnerabilidades | ⬜ Abierto | S91 |
| A-05 | Financiación (becas, patrocinio, fundación) | ⬜ Abierto | Antes de S52 |
| A-06 | Incorporar el texto completo de Apache-2.0 (`LICENSE-APACHE`) | ⬜ Abierto | Antes de la primera publicación (S94) |
| A-07 | Conectar el repositorio a GitHub y comprobar la primera ejecución de CI | 🟡 Preparado (S68): pasos exactos en `PUBLICAR.md` y CI revisada. Falta ejecutarla | Antes de anunciar el proyecto |
| A-08 | **¿`-2 ** 2` debe ser `(-2)**2` o `-(2**2)`?** La especificación dice lo primero; Python y Ruby dicen lo segundo | ⬜ Abierto (RFC) | Antes de la v1.0 |
| A-16 | **Un programa con ventana lanzado desde `cmd /c` terminaba con `0xC0000005`** | ✅ Cerrado (S84): la cola de eventos no se inicializaba al crear la ventana; `cuenta` quedaba con basura del montículo. Encontrado con ASan + Xvfb | — |
| A-17 | **Fuga al devolver de una función con locales con dueño** | ✅ Cerrado (S95): no se liberaban los locales al retornar, y el objeto de un método se conservaba como si se devolviera. Ahora se evalúa en un temporal, se libera y se devuelve. ASan limpio | — |
| A-15 | **macOS sin verificar**: no hay hardware Apple. Clang 18 en Linux compila y pasa las suites, que es la evidencia más cercana | ⬜ Abierto | Cuando haya un Mac o un runner de CI |
| A-14 | **Mover un campo de un objeto a una colección** | ✅ Cerrado (S66): el campo se vacía de verdad (longitud a cero) y queda un texto vacío válido | — |
| A-13 | **Un módulo nuevo de la biblioteca embebida no registraba sus funciones** | ✅ Cerrado (S62): no era un fallo del compilador; un archivo `build/textos.oki` olvidado tapaba al módulo. La búsqueda prefiere archivo a biblioteca | — |
| A-12 | **Diez códigos de diagnóstico sin caso directo** (CLI, carga de módulos, símbolo privado, backend y avisos) | ⬜ Abierto (deuda de pruebas) | Sin fecha |
| A-11 | **Llamada que mueve un valor con dueño dentro de otra expresión** (`f(g(a))`) no lo mueve | ✅ Cerrado (S51): es error de compilación en lugar de fuga | — |
| A-10 | **Devolver una construcción con variables locales con dueño** libera esas variables al retornar y deja punteros colgando | ✅ Cerrado (S48–S49): el retorno y la construcción transfieren, y los compuestos frescos liberan sus campos | — |
| A-09 | **Semántica de referencia para `clase` y herencia** | ✅ Resuelto (S36): ADR-021, clases por referencia con `nuevo` | — |

### 5.1 Resultado de la verificación del nombre (A-01)

**Lo que se comprobó el 2026-11-09:**

- **No hay ningún lenguaje de programación llamado "Nooki".** Los más cercanos son [Nooklang](https://github.com/tsklxiv/Nooklang) y el lenguaje [Nock](https://github.com/antlr/grammars-v4/blob/c306207ac870e7c5a95763b518ba3e3a1ca61409/nock/README.md) de Urbit: nombres distintos, sin conflicto de búsqueda.
- **Los dominios `nooki.online`, `nooki.club` y `nooki.capital` están registrados**; falta comprobar en el registrador la disponibilidad de **`nooki.dev`** y **`nooki.org`**.
- **La extensión `.oki` no tiene conflicto conocido** en lenguajes de programación. *Oki* es una marca de impresoras (Oki Electric), pero en otro sector y con otro uso.

**Pendiente (bloquea S03, no el diseño):** comprar el dominio definitivo y hacer una búsqueda formal en el registro de marcas del país de residencia. Si `nooki.dev` está tomado, alternativas: `nooki-lang.org`, `nooki-lang.dev`, `nooki.es`.

---

## 6. Cómo se cambia una decisión

1. Se abre un **RFC** en `docs/rfcs/` con la plantilla `0000-plantilla.md`.
2. Se discute en público (mínimo 7 días).
3. Si se aprueba, se crea un **ADR nuevo** que cita y reemplaza al anterior; el ADR viejo **no se modifica**.
4. Si el cambio afecta al lenguaje, sube la **edición** (ADR-012) o se marca como ruptura en la versión mayor.

> Regla de oro: *un ADR no se edita, se reemplaza.* El historial de decisiones es parte del activo del proyecto.

## 7. Inventario de pendientes para después de la 1.0

Lo que la 1.0 **no** incluye, con su tamaño y lo que desbloquea. Nada de esto es un defecto: son
características planificadas.

| Pendiente | Tamaño | Qué desbloquea |
|---|---|---|
| **Tiempos de vida completos**: duración de un préstamo entre sentencias del mismo ámbito | medio | cierra el último rincón teórico del modelo de memoria |
| **Paquetes con manifiesto** (`nooki.toml`) y dependencias | medio | compartir código entre proyectos |
| **Biblioteca estándar amplia**: `archivo`, `texto` avanzado, `fecha` con aritmética | medio | programas sin escribir lo básico |
| **Casos para los 10 códigos de integración** (A-12) | pequeño | confianza en las rutas de la CLI y del backend |
| **Interfaz gráfica nativa** | grande | aplicaciones de escritorio |
| **Bibliotecas de IA** (equivalente a pandas y PyTorch) | grande | análisis de datos y modelos |
| **`inseguro` y `asm!`** | grande | control de hardware y sistemas |
| **Conectar el repositorio a GitHub y ejecutar la CI** (A-07) | pequeño | que el proyecto exista fuera de una máquina |

El orden recomendado es el de la tabla: primero lo pequeño que cierra huecos, después lo grande.
