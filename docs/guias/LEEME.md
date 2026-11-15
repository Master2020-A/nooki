# Guías de Nooki

Cuatro documentos para cuatro necesidades distintas. Todos describen la implementación
**Nooki 1.0.0 (edición 2026)** y se apoyan en el código y en programas ejecutados.

| Guía | Para quién | Qué responde |
|---|---|---|
| **[Guía del usuario](GUIA_DEL_USUARIO.md)** | quien va a programar en Nooki | cómo se escribe un programa, qué tipos hay, cómo se usan funciones, clases, módulos, memoria, UI e IA |
| **[Guía del administrador](GUIA_DEL_ADMINISTRADOR.md)** | quien instala y mantiene Nooki | requisitos, instalación, backend de C, proyectos, CI, editores/LSP, seguridad y diagnóstico de fallos |
| **[Guía del desarrollador](GUIA_DEL_DESARROLLADOR.md)** | quien va a leer o extender el compilador | arquitectura, mapa del código, pruebas, cómo añadir sintaxis, diagnósticos, biblioteca, CLI y LSP |
| **[Referencia del lenguaje](REFERENCIA_DEL_LENGUAJE.md)** | quien necesita consultar | léxico, tipos, operadores, sentencias, declaraciones, propiedad, biblioteca, diagnósticos, CLI y gramática |

## Versiones PDF

En la raíz del repositorio, junto a `MANUAL.pdf`, hay copias en PDF de las cuatro guías:

| Guía | PDF |
|---|---|
| Usuario | [`GUIA_DEL_USUARIO.pdf`](../../GUIA_DEL_USUARIO.pdf) |
| Administrador | [`GUIA_DEL_ADMINISTRADOR.pdf`](../../GUIA_DEL_ADMINISTRADOR.pdf) |
| Desarrollador | [`GUIA_DEL_DESARROLLADOR.pdf`](../../GUIA_DEL_DESARROLLADOR.pdf) |
| Referencia del lenguaje | [`REFERENCIA_DEL_LENGUAJE.pdf`](../../REFERENCIA_DEL_LENGUAJE.pdf) |

Los PDF se generan desde los Markdown; si cambias una guía, vuelve a generarlos.

## Por dónde empezar

### Nunca he escrito Nooki

1. [Guía del usuario](GUIA_DEL_USUARIO.md), capítulos 1 a 9.
2. [Guía del usuario](GUIA_DEL_USUARIO.md), capítulos 10 a 17.
3. [`ejemplos/`](../../ejemplos/) y [`GUIA.md`](../../GUIA.md).
4. [Referencia del lenguaje](REFERENCIA_DEL_LENGUAJE.md) para consultar dudas.

### Ya programo y quiero el lenguaje

1. [Referencia del lenguaje](REFERENCIA_DEL_LENGUAJE.md), capítulos 1 a 7.
2. [Guía del usuario](GUIA_DEL_USUARIO.md), capítulos 10 a 17 para los patrones de uso.
3. [`MANUAL.md`](../../MANUAL.md) para programas completos.

### Tengo que instalar Nooki en un equipo o en CI

1. [Guía del administrador](GUIA_DEL_ADMINISTRADOR.md), capítulos 2 a 5.
2. [Guía del administrador](GUIA_DEL_ADMINISTRADOR.md), capítulos 6 a 9.
3. [Guía del administrador](GUIA_DEL_ADMINISTRADOR.md), capítulos 10 a 13.

### Voy a tocar el compilador

1. [Guía del desarrollador](GUIA_DEL_DESARROLLADOR.md), capítulos 1 a 5.
2. [Guía del desarrollador](GUIA_DEL_DESARROLLADOR.md), capítulos 6 a 9.
3. [`CONTRIBUIR.md`](../../CONTRIBUIR.md), [`DECISIONES.md`](../../DECISIONES.md) y
   [`docs/adr/`](../adr/).

## Relación con la documentación existente

Estas guías no sustituyen a los documentos históricos del proyecto; los ordenan por
audiencia:

| Documento | Papel |
|---|---|
| [`GUIA.md`](../../GUIA.md) | recorrido didáctico de trece pasos |
| [`MANUAL.md`](../../MANUAL.md) | manual completo con programas verificados |
| [`REFERENCIA.md`](../../REFERENCIA.md) | referencia breve del lenguaje |
| [`spec/especificacion-v0.1.md`](../../spec/especificacion-v0.1.md) | especificación de diseño, incluye lo que aún no existe |
| [`docs/PROYECTO.md`](../PROYECTO.md) | `nooki.toml` y dependencias |
| [`docs/INTERFAZ.md`](../INTERFAZ.md) | interfaz gráfica |
| [`editores/LEEME.md`](../../editores/LEEME.md) | resaltado y editores |
| [`CONTRIBUIR.md`](../../CONTRIBUIR.md) | cómo contribuir |
| [`DECISIONES.md`](../../DECISIONES.md) | ADR y asuntos abiertos |

## Nota de verificación

Las guías se escribieron contra la copia del repositorio que declara `nooki version` como
`1.0.0 (windows-x86_64, edición 2026)` y se comprobaron con el binario de
`build/windows-mingw`. Algunas afirmaciones describen **limitaciones observadas** de esa
versión (por ejemplo, escapes que se conservan literales, `Caja<texto>` que no genera C
válido o `nooki probar` que no carga dependencias). Se indican como tales para que nadie las
confunda con características del lenguaje.

Si cambias el compilador, actualiza la guía correspondiente en el mismo commit y ejecuta:

```bash
ctest --preset <preset> -R documentos
```
