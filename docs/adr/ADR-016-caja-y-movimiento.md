# ADR-016 — `Caja<T>` y movimiento: el primer trozo del modelo de memoria

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S20 |
| **Ámbito** | Lenguaje · memoria · generación de código |
| **Relación** | Primer paso de ADR-001 (seguridad de memoria) y de ADR-007 |

## Contexto

Casi todo lo que falta en el lenguaje —`texto` con operaciones, colecciones, clases por
referencia— choca con lo mismo: no hay modelo de memoria. Sin él, Nooki es un lenguaje de valores
primitivos con estructuras, no un lenguaje de sistemas.

El modelo completo de Rust (propiedad + préstamos + tiempos de vida) es la fase 7 del plan. Pero su
**primer cimiento** —un dueño único, movimiento y liberación determinista— es independiente de los
préstamos y se puede construir y verificar por separado.

## Decisión

1. **`Caja<T>` es un valor en el montículo con un único dueño.**
   ```aura
   variable c = Caja.nuevo(41)
   *c = *c + 1
   variable d = c        // movimiento: `c` deja de ser válido
   imprimir(*d)          // 42
   ```
2. **Asignar una caja transfiere la propiedad.** Usar el origen después es **error de
   compilación** (`NOK5001`), no un fallo en ejecución.
3. **El dueño libera al salir del ámbito**, incluido antes de cada `retornar`. No hay recolector.
4. **En esta versión, solo `Caja<entero>`.** Los genéricos llegan en S24; generalizarla antes
   obligaría a inventar una representación que habría que rehacer.
5. **Todavía no hay préstamos ni tiempos de vida.** `&T`, `&mutable T` y las reglas de préstamo son
   el sprint siguiente.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Empezar por los préstamos sin propiedad | No hay nada que prestar: sin dueño no hay reglas que verificar |
| Recolector de basura | Contradice el pilar y arruina la liberación determinista |
| `Caja<T>` genérica desde el principio | Los genéricos no existen todavía (S24); sería una representación desechable |
| No liberar (dejar que el sistema lo haga al salir del proceso) | Enseña un modelo mental equivocado y filtra memoria en bucles |

## Consecuencias

**Positivas**
- El lenguaje deja de filtrar memoria **por diseño** en el código que usa cajas.
- El error de usar un valor movido se detecta al compilar, con span y explicación.
- Sienta la estructura que los préstamos necesitan: un dueño por valor y un punto de liberación.

**Costes**
- Solo enteros por ahora: `Caja<texto>` y `Caja<estructura>` esperan a los genéricos.
- La liberación ocurre en los cierres de ámbito y en los `retornar`; un `romper` o `continuar`
  dentro de un bloque con cajas **puede saltarse la liberación**. Se documenta y se corrige cuando
  entre el verificador de flujo (fase 7).
- `Caja` se copia como struct de C en algunos contextos internos; el generador anula el origen tras
  el movimiento para que liberar dos veces sea inocuo.

## Impacto en la implementación

- S20: runtime `nooki_caja_nuevo` / `nooki_caja_soltar`; el generador registra las cajas de cada
  ámbito y emite `soltar` al cerrarlo y antes de cada `retornar`.
- S21: préstamos `&`/`&mutable` sobre cajas; el verificador pasa de "movido sí/no" a regiones.
- S24: `Caja<T>` genérica.
- S45–S52: el modelo completo sustituye esta implementación, que ya no tendrá que cambiar de
  semántica, solo de mecanismo.
