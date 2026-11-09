# ADR-006 — Atributos, documentación y macros

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 |
| **Ámbito** | Lenguaje · sintaxis · herramientas |
| **Origen** | `Nooki2` usa **dos** sintaxis a la vez: `@sobrescribir` (L988) y `#[prueba]` (L1036) |

## Contexto

`Nooki2` introduce atributos con dos notaciones incompatibles dentro del mismo documento:

```nooki
@sobrescribir                          // L988
#[prueba]                              // L1036
#[si(plataforma = "windows")]          // L2094
#[exportar_c]                          // L1078
```

Además nunca especifica **cómo se documenta el código**: no existe `///` ni `//!` en 2 895 líneas, en un lenguaje cuyo pilar es la documentación.

## Decisión

**Atributos**
1. **Una sola sintaxis: `@`**, con argumentos entre paréntesis.
   ```nooki
   @prueba
   @si(plataforma = "windows")
   @exportar("mi_funcion")
   @derivar(igualdad, orden, clonar)
   ```
2. **Conjunto cerrado en v0.1.** El compilador conoce la lista completa; un atributo desconocido es error con sugerencia del más parecido. Como el sistema de macros llega después, el comportamiento sigue siendo predecible.
3. **Atributos normativos de la v0.1:** `@virtual`, `@sobrescribir`, `@final`, `@sellada`, `@estatico` (alternativa a la palabra), `@repr`, `@derivar`, `@prueba`, `@rendimiento`, `@si`, `@exportar`, `@envolver`, `@rapido`, `@deprecado`, `@permitir`, `@avisar`, `@inmutable_sugerido`.

**Documentación**
4. **`///`** documenta el elemento siguiente; **`//!`** documenta el módulo que lo contiene.
5. `nooki doc` (S60) extrae doc comments a HTML; los ejemplos dentro de un doc comment se ejecutan como pruebas de documentación.

**Macros**
6. **Macros declarativas** (`macro nombre!(...)`) a partir de la **v0.7**; en v0.1 solo existen `asm!` y las macros intrínsecas de la librería estándar.
7. **Sin macros procedurales** hasta la v1.x. Son la principal causa de que el soporte de IDE en Rust sea complejo y lento.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| `#[...]` estilo Rust | Obliga a reservar `#`, choca visualmente con el comentario `#` de otros lenguajes y es más ruidosa |
| `@` + `#[...]` (lo que hacen los planes) | Ambos documentos no coinciden entre sí; un lenguaje no puede tener dos sintaxis de atributo |
| Anotaciones sin símbolo (`virtual funcion`) | Vuelve a llenar el espacio de palabras reservadas y compite con la visibilidad |
| Macros procedurales desde el inicio | Compromete el parser, el LSP y la compilación incremental antes de tener el lenguaje estable |
| Documentación solo con `/** */` | `///` por línea es más legible en revisiones de código y es lo que esperan los editores |

## Consecuencias

**Positivas**
- Una sola forma de escribir metadatos: los editores, el formateador y el LSP razonan igual.
- `@derivar` reduce el código repetitivo (sin él, cada estructura necesitaría tres `implementar`).
- `///` convierte la documentación en parte del contrato del lenguaje, no en un apéndice.

**Costes**
- Un conjunto cerrado de atributos no permite extensiones de terceros hasta v0.7.
- `asm!` es la única macro en v0.1 y debe tratarse como caso especial del parser.

## Impacto en la implementación

- El lexer produce el token `@` y `///` como **token de documentación** (conserva el texto, no lo descarta).
- El parser adjunta atributos a la declaración siguiente; en S16 se validan por posición.
- El formateador (S56) tiene reglas fijas para atributos: uno por línea, encima del elemento.
- El LSP (S58) usa los doc comments como texto de *hover*.
