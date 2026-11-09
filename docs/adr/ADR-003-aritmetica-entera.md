# ADR-003 — Aritmética entera: desbordamiento comprobado por defecto

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 |
| **Ámbito** | Lenguaje · semántica · generación de código |
| **Origen** | No especificado en ninguno de los dos planes |

## Contexto

Ningún documento dice qué pasa cuando `entero8` llega a 127 y se le suma uno. Las tres opciones del mercado tienen consecuencias muy distintas:

- **C/C++**: comportamiento indefinido → el compilador puede eliminar el código de comprobación; origen de vulnerabilidades reales.
- **Rust**: *panic* en depuración, *wrap* en publicación → el programa cambia de comportamiento según el perfil de compilación, lo que rompe la reproducibilidad.
- **Java/C#**: *wrap* silencioso siempre → errores lógicos silenciosos.

El proyecto se vende como "seguridad de Rust" y "control de hardware como C". Ambas cosas exigen una regla explícita y estable.

## Decisión

1. **Aritmética comprobada por defecto, en todos los perfiles** (`depurar`, `publicar`): sumar, restar, multiplicar, negar y dividir comprueban desbordamiento y división por cero. Si falla, se produce un **pánico** con mensaje en español y ubicación.
2. **Las comprobaciones no se eliminan con la optimización.** `-O3` no las desactiva.
3. **Salidas explícitas**, cada una con su nombre:

| Función | Semántica |
|---|---|
| `envolver(a + b)` | *wrap* de complemento a dos |
| `saturar(a + b)` | satura al mínimo/máximo del tipo |
| `ampliar<entero64>(a) + b` | promoción explícita de ancho |
| `@envolver` (atributo de función o bloque) | desactiva las comprobaciones en ese ámbito |
| `inseguro { }` | permite aritmética cruda solo junto a `asm!` o punteros |

4. **`flotante` cumple IEEE-754 estrictamente.** Prohibido `fast-math` por defecto; se habilita por función con `@rapido` y la documentación advierte que rompe la reproducibilidad (NaN, `-0.0`, asociatividad).
5. **División entera por cero** → pánico, nunca UB.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| *Wrap* en publicación, *panic* en depuración (Rust) | El programa cambia de comportamiento según el perfil: inaceptable para firmware y para reproducir fallos |
| *Wrap* siempre (Java/C#) | Errores silenciosos; contradice el pilar de seguridad |
| UB (C/C++) | Descartado por definición |
| Solo comprobado en depuración | Deja los fallos de producción sin red |

## Consecuencias

**Positivas**
- Un desbordamiento es un fallo detectable, no un misterio.
- Mismo comportamiento en desarrollo y en producción: los *bugs* se reproducen.
- El modo didáctico funciona: el mensaje de pánico explica qué pasó y cómo arreglarlo.

**Costes**
- **Entre 5 % y 15 % de rendimiento** en bucles con aritmética entera intensiva. Se mitiga con `@envolver` en rutinas calientes medidas con *benchmark*.
- El código generado es más largo (ramas de comprobación) → peor localidad de instrucciones en rutas calientes.
- Quien venga de C esperará el *wrap*: hay que documentarlo en el tutorial y en el mensaje de error.

## Impacto en la implementación

- Cada operación entera genera `add` + `llvm.sadd.with.overflow` + rama a `aura_panico`.
- La comprobación se inserta en el MIR (S30) como nodo explícito para que sea visible en la depuración.
- `envolver()` y `@envolver` se resuelven en HIR (S29): no llegan al MIR como comprobación.
- Los *benchmarks* de S36 deben medir el coste real de estas comprobaciones contra C y Rust.
