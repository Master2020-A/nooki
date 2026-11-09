# ADR-017 — Préstamos de alcance de expresión

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S21 |
| **Ámbito** | Lenguaje · memoria |
| **Relación** | Continúa ADR-016; paso previo a los tiempos de vida |

## Contexto

Con `Caja<T>` (ADR-016) ya existe algo que prestar. El paso natural es permitir prestar sin mover,
que es el 90 % del uso real de los préstamos: pasar un valor a una función sin transferir la
propiedad.

El modelo completo de Rust exige tiempos de vida, regiones y análisis de flujo. Implementarlo entero
de golpe es la fase 7 del plan y varios sprints. Pero hay un subconjunto que cubre el uso más común y
se puede verificar **sin** análisis de flujo.

## Decisión

1. **Se puede prestar una caja**: `&caja` produce un préstamo inmutable, de tipo `&Caja<entero>`.
   ```aura
   funcion suma(a: &Caja<entero>, b: &Caja<entero>) -> entero {
       retornar *a + *b
   }

   variable c = Caja.nuevo(20)
   imprimir(suma(&c, &c))     // prestar no mueve: `c` sigue siendo válida
   ```
2. **El préstamo vive durante la expresión que lo crea.** No se puede guardar:
   `variable r = &c` es error `NOK5002`. Su uso es pasarlo como argumento.
3. **Prestar no transfiere la propiedad.** La variable prestada sigue siendo del mismo dueño y se
   libera donde siempre.
4. **Solo se puede prestar una variable que sea una `Caja`** (`NOK4008`); prestar un entero suelto
   todavía no significa nada.
5. **`&mutable` se analiza pero no se distingue de `&`** en esta versión: la exclusividad de los
   préstamos mutables requiere el análisis de regiones del sprint siguiente.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Implementar ya tiempos de vida explícitos | Requiere inferencia de regiones completa; varios sprints sin valor visible |
| Permitir guardar préstamos y comprobarlos con una regla léxica | Da falsos positivos y falsos negativos: peor que no comprobarlo |
| Copiar la caja al pasarla | Pierde el objetivo: pasar por referencia es lo que se busca |
| No distinguir `&` de `&mutable` (lo elegido) | Es una limitación consciente y documentada, no un accidente |

## Consecuencias

**Positivas**
- El caso de uso dominante (pasar sin mover) funciona y está verificado.
- El tipo `&Caja<T>` entra en el sistema de tipos sin necesidad de regiones.
- Deja el terreno preparado: cuando lleguen los tiempos de vida, solo hay que **ampliar** el alcance
  del préstamo, no cambiar su semántica.

**Costes**
- No se pueden devolver préstamos ni guardarlos para usarlos después. Muchos patrones idiomáticos de
  Rust quedan fuera hasta la fase 7.
- `&mutable` no aporta nada todavía: se acepta para no romper la sintaxis futura.

## Impacto en la implementación

- S21: `analizar_tipo_texto` admite `&T`, `&mutable T`, `*T` y `Caja<T>`; el generador traduce un
  préstamo a un puntero de C (`nooki_caja*`).
- S22: préstamos mutables y la regla de exclusividad, con un primer análisis de regiones.
- S45–S52: tiempos de vida explícitos y el verificador de préstamos completo sustituyen la regla de
  alcance de expresión.
