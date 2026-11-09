# Cómo contribuir a Nooki

Gracias por asomarte. Nooki está en fase de diseño activo: el compilador existe, pero casi nada
está implementado todavía. Es un buen momento para entrar.

## Antes de escribir código

1. Lee [README.md](README.md) para entender el estado del proyecto.
2. Lee [DECISIONES.md](DECISIONES.md): las decisiones ya están cerradas y no se rediscuten sin RFC.
3. Lee el sprint correspondiente en [PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md](PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md).
   Cada sprint tiene un criterio de terminado claro: si no lo cumples, no está terminado.

## Requisitos

| Herramienta | Versión mínima |
|---|---|
| CMake | 3.25 |
| Compilador C++ | MSVC 2022, GCC 12, Clang 15 (C++20) |
| Git | cualquiera reciente |

Opcionales: `clang-format` y `clang-tidy` (se usan en CI).

## Compilar y probar

```bash
# Windows (elige uno)
cmake --preset windows-msvc-2026     # Visual Studio 2026
cmake --preset windows-msvc          # Visual Studio 2022 (el que usa CI)
cmake --preset windows-mingw         # MinGW GCC

# Linux / macOS
cmake --preset linux                 # o macos

cmake --build --preset <preset>
ctest --preset <preset>
```

Con saneamiento (solo Linux y macOS):

```bash
cmake --preset saneamiento
cmake --build --preset saneamiento 2>/dev/null || cmake --build build/saneamiento
ctest --preset saneamiento
```

## Estilo

- Código, comentarios, mensajes y **nombres de identificadores en español**.
- Formato: `.clang-format` (4 espacios, 100 columnas). Ejecuta `clang-format -i` antes de enviar.
- Análisis: `.clang-tidy`.
- Comentarios solo cuando expliquen el *porqué*, no el *qué*.

## Pruebas

- Toda funcionalidad nueva necesita pruebas. En esta etapa, en `compilador/pruebas/`.
- A partir del Sprint 5, además, un caso de conformidad en `pruebas/conformidad/` (formato descrito
  en [pruebas/LEEME.md](pruebas/LEEME.md)).
- Un cambio que no compila y no pasa las pruebas en Windows, Linux y macOS no se acepta.

## Commits y firma

- Mensajes de commit en español, en imperativo: `añade el verificador de préstamos`.
- Firma tus commits con `git commit -s` (DCO). Al hacerlo certificas que puedes aportar ese código
  bajo la licencia del proyecto.

## Cambios que afectan al lenguaje

Si tu idea cambia la sintaxis, la semántica o la librería estándar, **no abras un PR**: abre un
**RFC** con la plantilla de [docs/rfcs/0000-plantilla.md](docs/rfcs/0000-plantilla.md). Si se
aprueba, se crea un ADR y después se implementa.

## Dónde empezar

Busca los sprints abiertos en el plan. Un buen primer aporte suele ser:

- un caso de conformidad que hoy falla,
- un mensaje de error que se entiende mal,
- una prueba que falta.

Gracias.
