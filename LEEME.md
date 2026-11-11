# Nooki

**Un lenguaje compilado, con sintaxis en español, tipado seguro y memoria determinista.**

Nooki se escribe en archivos `.oki` y se compila a un binario nativo. Usa un compilador de C
(`cc`, `gcc`, `clang` o MSVC) como backend, así que funciona en Windows, macOS y Linux sin runtime
propio.

```aura
funcion principal() -> entero {
    imprimir("¡Hola, Nooki!")
    retornar 0
}
```

```bash
$ nooki ejecutar hola.oki
¡Hola, Nooki!
```

## Qué tiene

| Pilar | Estado |
|---|---|
| Sintaxis sencilla, todo en español | sí |
| Tipos comprobados antes de ejecutar | sí |
| Memoria segura sin recolector: propiedad, movimiento y préstamos | sí |
| OOP completa: clases, herencia, polimorfismo, `super` | sí |
| Genéricos de funciones y de tipos | sí |
| Enumeraciones con datos y emparejamiento | sí |
| Rasgos y despacho por rasgo (`&Rasgo`) | sí |
| Errores como valores (`resultado` y `?`), sin excepciones | sí |
| Colecciones: `Vector<entero>`, `Vector<texto>`, `Mapa<texto, entero>` | sí |
| Módulos con visibilidad y proyectos con `biblioteca/` | sí |
| Biblioteca estándar escrita en Nooki | `resultado`, `matematicas` |
| Control de hardware (`inseguro`, `asm!`) | pendiente (fase posterior) |

**Versión actual: 0.7.0.** No hay defectos conocidos: todos los huecos de propiedad están cerrados o
son error de compilación. Lo que falta son características planificadas, no fallos.

## Empezar en tres comandos

```bash
$ nooki nuevo demo
$ cd demo
$ nooki construir
$ ./principal
Hola, Nooki
```

Después:

```bash
$ nooki probar      # ejecuta pruebas/*.oki y compara con su .esperado
$ nooki limpiar     # borra binarios y artefactos
```

El recorrido completo del lenguaje está en la **[guía](GUIA.md)**: diez pasos, cada uno con su
programa y su salida real.

## Compilar el compilador

Requisitos: CMake 3.25 o superior y un compilador de C++20 (MSVC 2022+, GCC 12+ o Clang 15+).

```bash
$ cmake --preset windows-mingw      # o linux, macos, windows-msvc-2026
$ cmake --build --preset windows-mingw
$ ctest --preset windows-mingw
```

El binario queda en `build/<preset>/compilador/nooki.exe`. Para generar un ejecutable y usarlo desde
cualquier sitio, añade esa carpeta al `PATH`.

Presets disponibles: `windows-msvc-2022`, `windows-msvc-2026`, `windows-mingw`,
`windows-mingw-depurar`, `linux`, `macos` y `saneamiento` (AddressSanitizer y UBSan).

## Estructura del repositorio

```
compilador/            código del compilador (C++20)
  fuente/lexico/         análisis léxico
  fuente/sintactico/     parser y AST
  fuente/semantico/      verificación de tipos y propiedad
  fuente/generador/      generación de C
  fuente/conductor/      carga de módulos y cadena de compilación
  fuente/cli/            interfaz de línea de comandos
  pruebas/               pruebas unitarias del compilador
pruebas/conformidad/   programas que se compilan y ejecutan en cada `ctest`
spec/                  especificación del lenguaje
docs/adr/              decisiones de arquitectura
```

## Documentación

- **[Guía](GUIA.md)** — aprender el lenguaje desde cero.
- **[Especificación](spec/especificacion-v0.1.md)** — el documento normativo: gramática, tipos,
  sentencias, códigos de diagnóstico.
- **[Decisiones](DECISIONES.md)** — índice de los 25 ADR y de los asuntos abiertos.
- **[Plan de sprints](PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md)** — qué está hecho y qué falta.
- **[Cambios](CHANGELOG.md)** — qué entró en cada versión.

## Cómo se prueba

Cada cambio pasa por tres filtros:

1. **Ocho suites unitarias** del compilador (léxico, parser, semántica, diagnósticos, CLI, expresión).
2. **62 programas de conformidad** en `pruebas/conformidad/valido/`: cada uno se compila, se ejecuta y
   su salida se compara con la esperada. Si un cambio rompe el código generado, se ve aquí.
3. **Ambos compiladores de C**: GCC y MSVC, con `ctest` en los dos.

Los diagnósticos también se prueban: cada código `NOK` tiene un caso que lo provoca.

## Licencia

MIT o Apache-2.0, a elegir. Ver [LICENCIA](LICENCIA.md).

## Contribuir

Lee [CONTRIBUIR](CONTRIBUIR.md). En resumen: código en C++20 con el estilo del repositorio, mensajes
de commit en español e imperativo, y ninguna característica nueva sin su caso de conformidad.
