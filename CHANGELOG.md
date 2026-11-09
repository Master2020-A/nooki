# Historial de cambios

El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/) y el proyecto usa
[versionado semántico](https://semver.org/lang/es/).

## [Sin publicar]

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
