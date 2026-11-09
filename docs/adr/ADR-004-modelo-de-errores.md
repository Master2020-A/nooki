# ADR-004 — Modelo de errores: `resultado<T, E>` como primario, pánico como excepción

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 |
| **Ámbito** | Lenguaje · runtime · FFI · código generado |
| **Origen** | `Aurora2.txt` mezcla `intentar/atrapar/lanzar` (L545) con `Resultado<T,E>` y `?` (L557) sin política |

## Contexto

`Nooki2` presenta dos mecanismos de error a la vez: excepciones (`intentar` / `atrapar` / `lanzar` / `finalmente`) y valores de error (`resultado` + `?`). Mantener ambos sin una regla clara tiene costes muy concretos:

- El *unwinding* de excepciones obliga a tablas de desenrollado, encarece cada llamada y complica la optimización.
- Las excepciones **no pueden cruzar la frontera FFI** sin comportamiento indefinido.
- Excepciones + `async` es la combinación más difícil de todas (por eso Rust las descartó).

## Decisión

1. **`resultado<T, E>` es el mecanismo primario y el único que forma parte del flujo normal.**

```nooki
funcion leer_config(ruta: texto) -> resultado<config, error_io> {
    variable archivo = abrir(ruta)?
    variable datos = archivo.leer_todo()?
    retornar ok(analizar(datos))
}
```

2. **`?`** propaga el error hacia arriba. Solo es válido dentro de una función que devuelve `resultado`. Requiere que el tipo de error del llamador sea el mismo o implemente el rasgo `desde_error<F>`; en caso contrario es **error de compilación** con sugerencia de conversión.

3. **`opcional<T>`** es un tipo distinto y **no** admite `?` en v0.1 (evita la ambigüedad de Rust entre `Option` y `Result`); se desempaqueta con `coincide` o con `.desempaquetar_o(valor)`.

4. **`intentar` / `atrapar` / `finalmente` / `lanzar` existen, pero solo para pánico:**
   - `lanzar "mensaje"` provoca un **pánico** (fallo interno, error de programación o estado irrecuperable), nunca un error de negocio.
   - `intentar { } atrapar (e) { }` **captura pánicos** en la frontera: servidores, plugins, FFI, hilos de trabajo.
   - El linter avisa si se usa `intentar` en código de negocio.
5. **Un pánico nunca cruza la frontera FFI.** El compilador inserta una barrera que aborta el proceso con mensaje, en lugar de permitir *unwinding* a través de `externo "C"`.
6. **No hay excepciones de usuario.** No existe `clase Error : Exception`.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Solo excepciones (Java, Python) | Incompatible con FFI limpio, encarece llamadas, imposible de razonar con ownership |
| Solo `Result` sin captura de pánico (Rust) | Un servidor no puede sobrevivir a un fallo en un manejador; se necesita una frontera |
| `Result` + excepciones completas | Es exactamente la ambigüedad del documento original: dos formas de fallar, ninguna canónica |
| `Result` + `Option` unificados con `?` | Complica la inferencia y los mensajes de error |

## Consecuencias

**Positivas**
- Un solo camino feliz de errores: tipos explícitos, visibles en la firma.
- Sin *unwinding* en el camino caliente: cero coste cuando no hay error (representación optimizada, igual que Rust).
- La frontera FFI es segura por construcción.
- `intentar` se reserva para donde de verdad hace falta: aislamiento de fallos.

**Costes**
- Verbo explícito: `?` en cada llamada que puede fallar. **Mitigación:** el operador `?` es de un solo carácter y encadenable.
- Ausencia de excepciones sorprende a quien viene de Python/Java. **Mitigación:** el mensaje de error del compilador sugiere `retornar error(...)` o `?`.
- El compilador debe implementar `desde_error` y su resolución (S25).

## Impacto en la implementación

- `resultado<T, E>` se representa como una unión etiquetada con nicho cuando `E` lo permite (puntero nulo imposible) → mismo tamaño que `T` en el caso común.
- `?` se desazucara en HIR (S29) a `coincide`: `ok(v) => v`, `error(e) => retornar error(convertir(e))`.
- La barrera de pánico en FFI se genera en S37.
- El runtime (S38) incluye `aura_panico` y el gancho de *backtrace*.
