# ADR-002 — Mutabilidad por defecto y nombres de los tipos primitivos

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 |
| **Ámbito** | Lenguaje · sistema de tipos · sintaxis |
| **Origen** | Contradicción entre `Aurora1.txt` (L92) y `Aurora2.txt` (L449, L464) |

## Contexto

`Nooki1` declara que `variable` es el equivalente de `let` (inmutable por defecto) y añade las palabras `mutable` e `inmutable`. `Nooki2` llama "variables mutables" a `variable` (L449) y a la vez escribe `variable mut` (L464, L802). Son **tres modelos de mutabilidad incompatibles** en dos documentos.

Además, los nombres de tipo difieren: `entero`/`cadena`/`booleano` en minúscula frente a `Entero`/`Texto`/`Booleano` en mayúscula.

## Decisión

**Mutabilidad**
- `variable` declara un enlace **mutable**. No existe la palabra `mut`.
- `constante` declara un enlace **inmutable**, verificado en compilación.
- La mutabilidad a través de una referencia la controla el operador: `&` (solo lectura) y `&mut` (exclusiva).
- Prestar `&mutable` algo declarado `constante` es **error de compilación**.
- `mutable` **se conserva** con un significado distinto y preciso: modifica el **tipo de la referencia o del puntero** (`&mutable T`, `*mutable T`), nunca el enlace de la variable. `&mut`/`*mut` se aceptan como alias de escritura corta.
- Se **eliminan** de la gramática: `mut` (suelto), `inmutable`, `propio`, `prestamo`, `apuntador`, `referencia`, `liberar`, `seguro`. La propiedad es implícita; `&`, `&mutable` y `*` cubren préstamos y punteros.

**Nombres de tipos** (todo en minúscula, sin tildes, ASCII)

| Tipo | Significado | Equivalente |
|---|---|---|
| `entero` | entero con signo de 64 bits (por defecto) | `i64` |
| `entero8`, `entero16`, `entero32`, `entero64`, `entero128` | enteros con signo | `i8`…`i128` |
| `natural` | entero sin signo de 64 bits | `u64` |
| `natural8`, `natural16`, `natural32`, `natural64`, `natural128` | enteros sin signo | `u8`…`u128` |
| `flotante` | coma flotante de 64 bits (por defecto) | `f64` |
| `flotante32` | coma flotante de 32 bits | `f32` |
| `booleano` | `verdadero` / `falso` | `bool` |
| `caracter` | escalar Unicode (32 bits) | `char` |
| `texto` | cadena UTF-8 con propiedad, en montículo | `String` |
| `vacio` | ausencia de valor de retorno | `()` |
| `byte` | alias de `natural8` | `u8` |
| `tamano` | entero sin signo del tamaño de un puntero | `usize` |
| `tamano_signado` | entero con signo del tamaño de un puntero | `isize` |

- El literal de texto (`"hola"`) tiene tipo **`&texto`** (préstamo estático), no `texto`.
- `texto` se elige sobre `cadena`: es más corto, más intuitivo para principiantes y no colisiona con el uso coloquial de "cadena" (cadena de bloques, cadena de conexión).

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Inmutable por defecto estilo Rust (`let`) | Más seguro, pero añade ceremonia a cada declaración y choca con el pilar "fácil como Python" |
| Inmutable por defecto + `mut` explícito | Dos palabras para lo que el principiante espera que sea una |
| `var` / `let` | Ya se usan `variable` / `constante`, que además son autoexplicativos en español |
| `natural` → `entero_sin_signo` / `ess8` | Verboso o críptico; `natural` es una palabra que el hablante ya conoce |
| `texto` → `cadena` | Más largo y con sobrecarga semántica |

## Consecuencias

**Positivas**
- `variable x = 5` es exactamente lo que un principiante espera.
- Un único conjunto coherente de palabras reservadas: **63 palabras clave + 21 nombres de tipo**, en lugar de dos conjuntos contradictorios.
- Los tipos se leen como palabras, no como siglas.

**Costes**
- El verificador pierde la "inmutabilidad por defecto" como red de seguridad: habrá más préstamos mutables de los necesarios y, por tanto, más conflictos detectados.
  **Mitigación:** diagnóstico proactivo *«esta variable nunca se modifica; declararla `constante`»* y lint `@inmutable_sugerido` (S59).
- `natural` puede confundirse con precisión arbitraria. **Mitigación:** documentación explícita y error de compilación si se desborda.

## Impacto en la implementación

- El AST no necesita nodo de mutabilidad en el binding; sí lo necesita la referencia (`&` vs `&mut`).
- El verificador de préstamos (S47) razona sobre **accesos**, no sobre declaraciones: un `&mut` a una `variable` es válido, pero dos simultáneos no.
- El chequeo de `constante` es puramente estático y se resuelve en S22.
