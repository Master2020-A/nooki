# NOOKI — Lenguaje de programación en español

> **Estado: fase de diseño.** Todavía no hay código. Lo que existe es el plan de implementación, las decisiones de arquitectura y la especificación del lenguaje.

**Nooki** es un lenguaje compilado, escrito en C++20 con backend LLVM, multiplataforma (Windows, macOS, Linux), con:
poder de C/C++ · seguridad de memoria de Rust · sintaxis simple como Python · OOP completa · y todo —tipos, palabras clave, errores y documentación— **en español**.

| | |
|---|---|
| **Nombre** | Nooki |
| **Versión** | 1.0.0 |
| **Extensión de fuentes** | `.oki` |
| **CLI** | `nooki` |
| **Implementación** | C++20 + LLVM |
| **Licencia** | MIT OR Apache-2.0 + excepción de runtime |

---

## Mapa del proyecto

| Documento | Qué contiene |
|---|---|
| **[PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md](PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md)** | Plan de 96 sprints en 12 fases, con criterio de terminado por sprint, riesgos y calendario real |
| **[DECISIONES.md](DECISIONES.md)** | Las 13 ADR aprobadas + nomenclatura + decisiones de alcance + asuntos abiertos |
| **[spec/especificacion-v0.1.md](spec/especificacion-v0.1.md)** | Especificación normativa: léxico, tipos, OOP, memoria, errores, FFI |
| **[spec/gramatica.ebnf](spec/gramatica.ebnf)** | Gramática formal completa (EBNF), normativa |
| **[docs/adr/](docs/adr/)** | 13 Architecture Decision Records: el *porqué* de cada decisión |
| **[CHANGELOG.md](CHANGELOG.md)** | Qué cambió en cada sprint |
| **[CONTRIBUIR.md](CONTRIBUIR.md)** | Requisitos, cómo compilar, estilo y proceso de RFC |

### Documentos históricos

| Archivo | Estado |
|---|---|
| [Aurora1.txt](Aurora1.txt) | Plan original v1 (28–36 meses). **Superado**: se conserva solo como histórico |
| [Aurora2.txt](Aurora2.txt) | Plan original v2 (48 meses). **Superado por el plan de sprints**; sus ejemplos de sintaxis siguen siendo referencia de intención |

---

## Dónde estamos

```
Sprint 1  ✅  Decisiones: 13 ADR aprobadas
Sprint 2  ✅  Especificación v0.1 + gramática EBNF
Sprint 3  ✅  Andamiaje: repositorio, CMake, CLI, CI en 3 sistemas
Sprint 4  ✅  Sistema de diagnósticos: códigos NOK, catálogo es/en, texto y JSON
Sprint 5  ✅  Arnés de conformidad (22 casos) + analizador léxico completo
Sprint 6  ✅  Rebanada vertical: Nooki compila y ejecuta su primer programa
Fase 2    ⏭️  (S07–S11, léxico) absorbida por el Sprint 5
Sprint 12 ✅  Parser de expresiones con precedencia (Pratt) · 197 casos
Sprint 13 ✅  Sentencias y control de flujo · `nooki ast`
Sprint 14 ✅  El backend compila variables y control de flujo · FizzBuzz y Fibonacci ejecutándose
Sprint 15 ✅  Funciones con parámetros · factorial recursivo y primera verificación semántica
Sprint 16 ✅  Verificación de tipos · errores en español antes de llegar a C
Sprint 17 ✅  Estructuras y clases con campos · objetos por valor
Sprint 18 ✅  Métodos y `este` · objetos con comportamiento
Sprint 19 ✅  Enumeraciones y `coincide` · emparejamiento de patrones
Sprint 20 ✅  `Caja<T>`: propiedad única, movimiento y liberación determinista
Sprint 21 ✅  Préstamos inmutables: prestar no mueve
Sprint 22 ✅  Préstamos mutables y exclusividad · **v0.2.0**
Sprint 23 ✅  `texto` con longitud, concatenación y comparación
Sprint 24 ✅  Genéricos por instanciación · una función, varios tipos
Sprint 25 ✅  `Vector<T>`: colección dinámica con dueño
Sprint 26 ✅  Iteración sobre colecciones · `para (n en numeros)`
Sprint 27 ✅  `Vector<texto>`: el vector es dueño de los elementos
Sprint 28 ✅  Una sola regla de propiedad en las llamadas
Sprint 29 ✅  `Mapa<texto, entero>`: tabla hash con claves propias
Sprint 30 ✅  Módulos e importaciones · varios archivos por programa
Sprint 31 ✅  Consolidación documental · **v0.3.0**
Sprint 32 ✅  `Mapa`: rehash automático y `quitar`
Sprint 33 ✅  Entrada estándar · `leer_linea()` y `leer_entero()`
Sprint 34 ✅  `mapa.claves()` y programa interactivo completo
Sprint 35 ✅  Espacios de nombres para tipos · `modulo.tipo`
Sprint 36 ✅  `clase` por referencia · `nuevo Tipo { ... }`
Sprint 37 ✅  Herencia y polimorfismo · tablas virtuales
Sprint 38 ✅  `super` y polimorfismo a través de la base
Sprint 39 ✅  Cierre de la v0.4 · firma al sobrescribir y ejemplo polimórfico
Sprint 40 ✅  Errores como valores · `Resultado` y `?`
Sprint 41 ✅  Operaciones de texto · cortar, buscar y convertir
Sprint 42 ✅  Variantes con datos · `caso Bien(v)`
Sprint 43 ✅  `resultado` escrito en Nooki · la biblioteca crece
Sprint 44 ✅  `texto.dividir`, `matematicas` y más biblioteca
Sprint 45 ✅  Proyectos · `nooki construir` y búsqueda en `biblioteca/`
Sprint 46 ✅  Genéricos de tipos · `estructura par<T>`
Sprint 47 ✅  Cierre de la v0.5 · auditoría y programa de cierre
Sprint 48 ✅  Propiedad en el retorno y en la construcción
Sprint 49 ✅  Los compuestos liberan sus campos
Sprint 50 ✅  Copia de compuestos rechazada y saltos que liberan
Sprint 51 ✅  Cierre de la v0.6 · sin defectos conocidos
Sprint 52 ✅  Herramientas: `nuevo`, `probar` y `limpiar`
Sprint 53 ✅  Guía y presentación en español
Sprint 54 ✅  Rasgos · `rasgo` e `implementa`
Sprint 55 ✅  `&Rasgo` · despacho dinámico entre tipos no emparentados
Sprint 56 ✅  Cierre de la v0.7 · sistema de tipos completo
Sprint 57 ✅  Tiempos de vida: el préstamo que escapa
Sprint 58 ✅  Los casos inválidos pasan por todas las fases
Sprint 59 ✅  La duración del préstamo
Sprint 60 ✅  Cierre de la v0.8 · auditoría de diagnóstico
Sprint 61 ✅  Biblioteca `textos` en Nooki
Sprint 62 ✅  Biblioteca embebida: `textos` viaja en el compilador
Sprint 63 ✅  Biblioteca: `listas` y `fechas`
Sprint 64 ✅  Cierre de la v0.9 · prueba de coherencia documental
Sprint 65 ✅  Todas las vías del movimiento
Sprint 66 ✅  A-14 cerrado: mover un campo lo deja vacío y válido
Sprint 67 ✅  Cierre de la 1.0
Sprint 68 ✅  Preparar la publicación
Sprint 69 ✅  Ejemplos ejecutables y resaltado en editores
Sprint 70 ✅  Verificado en Linux: GCC, Clang y ASan+UBSan
Sprint 71 ✅  Fuzzing y benchmarks: 100 000 entradas y un fallo real cerrado
Sprint 72 ✅  Aritmética comprobada y rutas con tildes
Sprint 73 ✅  FFI: llamar a C desde Nooki
Sprint 74 ✅  `inseguro` y `asm!`: control de hardware
Sprint 75 ✅  Manifiesto `nooki.toml` y dependencias
Sprint 76 ✅  Servidor de lenguaje `nooki lsp`
Sprint 77 ✅  Biblioteca: archivos y fechas
Sprint 78 ✅  Estabilización: API congelada y distribución
Sprint 79 ✅  UI: ventanas nativas desde Nooki
```

**Ya compila y ejecuta:**

```bash
$ nooki ejecutar hola.oki
¡Hola, Nooki!
```

```aura
funcion principal() -> entero {
    imprimir("¡Hola, Nooki!")
    retornar 0
}
```

Un ejemplo completo (estructura, método, caja y préstamo mutable) está en
[`pruebas/conformidad/valido/0014-amortizacion`](pruebas/conformidad/valido/0014-amortizacion/principal.oki).

El binario `nooki` ya existe y responde (`--version`, `ayuda`), pero **todavía no compila `.oki`**:
eso llega en el Sprint 6.

### Compilar y probar

```bash
cmake --preset windows-msvc      # o: windows-msvc-2026, windows-mingw, linux, macos
cmake --build --preset windows-msvc
ctest --preset windows-msvc
```

```bash
build/windows-msvc/compilador/Release/nooki.exe --version
#  nooki 0.1.0 (windows-x86_64, edición 2026)
```

Requisitos y estilo en [CONTRIBUIR.md](CONTRIBUIR.md). Historial en [CHANGELOG.md](CHANGELOG.md).

---

## Realidad del proyecto

El proyecto lo desarrolla **una sola persona**. Eso no cambia el orden de los sprints, pero sí el calendario y el alcance:

| Hito | Cuándo |
|---|---|
| Demo "Hola, Nooki" nativo en Windows, Linux y macOS | **~5–6 meses** |
| Lenguaje completo (ownership incluido) | ~3,75–4 años |
| **v1.0** (lenguaje + tooling + librería estándar, **sin UI ni IA**) | **~5,5–6 años** |
| UI e IA (post-1.0) | ~7–7,5 años |

- **UI e IA no son parte de la v1.0**: se entregan después como paquetes sobre FFI (Skia, ONNX, BLAS), no reimplementadas.
- **Registro de paquetes por Git** en lugar de servidor propio, hasta que haya usuarios.
- **macOS se valida en CI**; la notarización queda condicionada a un Mac o a financiación.

Detalle completo en [PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md](PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md) §5.

---

## Principios innegociables

1. **Todo en español**: palabras clave, mensajes de error, documentación y nombres de identificadores en el compilador.
2. **La seguridad no se negocia**: sin `nulo` en referencias, desbordamiento comprobado, préstamos verificados en compilación.
3. **El hardware es de primera clase**: `inseguro`, `asm!`, `@repr(C)`, `core` sin sistema operativo.
4. **Los errores se explican**: todo diagnóstico lleva código, ubicación y sugerencia.
5. **Nada se promete sin diseño**: si no hay ADR y caso de conformidad, no existe.

---

## Cómo revisar este repositorio

Si vienes a **revisar el diseño**:

1. Lee [DECISIONES.md](DECISIONES.md) y objeta cualquier decisión de la sección 3 (revisables hasta el Sprint 6).
2. Lee [spec/especificacion-v0.1.md](spec/especificacion-v0.1.md) con la gramática al lado: un lenguaje se valida escribiendo programas, no leyendo documentos.
3. Si algo no se puede expresar, abre un RFC.

Si vienes a **implementar**: empieza por el Sprint 3 del plan. Cada sprint tiene su criterio de terminado.

---

## Licencia

`MIT OR Apache-2.0` con excepción de runtime (ver [ADR-012](docs/adr/ADR-012-licencia-y-gobernanza.md)).
