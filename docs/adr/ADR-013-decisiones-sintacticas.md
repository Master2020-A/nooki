# ADR-013 — Decisiones sintácticas derivadas de la especificación v0.1

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01/S02 |
| **Ámbito** | Sintaxis · léxico |
| **Origen** | Ambigüedades detectadas al escribir la gramática EBNF |

## Contexto

Al redactar la gramática normativa aparecieron decisiones que ninguno de los dos planes resolvía y que no encajaban en las ADR anteriores. Se documentan juntas porque todas son de la misma naturaleza: **una sola forma canónica de escribir cada cosa**.

## Decisiones

### 1. `interfaz` y `rasgo` son sinónimos exactos

Ambas palabras producen el **mismo nodo del AST**; `rasgo` es el término canónico del libro de referencia. Se conservan las dos por familiaridad: quien viene de Java escribe `interfaz`, quien viene de Rust escribe `rasgo`.

### 2. `nuevo` es invariable

Existe una sola palabra, `nuevo`, sin concordancia de género (`nuevo Lista<entero>()`, no `nueva Lista`). Se documenta como operador, no como adjetivo. `nueva` **no** es palabra reservada y producirá error con sugerencia.

### 3. `caracter` con comillas simples, compatibles con *lifetimes*

- `'a'` → literal de carácter (comillas simples, exactamente un escalar).
- `'v` → *lifetime* (comilla simple + identificador).
- **Regla de desambiguación:** tras `'`, si el siguiente carácter es `\` o el que sigue al escalar es `'`, es carácter; si es un identificador, es *lifetime*. Es la misma regla que usa Rust y funciona sin retroceso.

### 4. Sentencias terminadas por salto de línea; `;` opcional

El salto de línea termina la sentencia, salvo que:
- la línea termine en un operador binario (`+`, `&&`, `=>`, `=`…);
- haya un delimitador abierto sin cerrar (`(`, `[`, `{`);
- la línea sea un comentario.

`;` se acepta como separador explícito. El formateador los omite.

### 5. `Este` es el tipo, `este` es el valor

`este` (minúscula) es la instancia actual. `Este` (mayúscula) es el tipo actual, válido en firmas de rasgos y en `implementar`. La diferencia es solo de capitalización, igual que en Rust (`self` / `Self`), y el linter avisa del uso confuso.

### 6. Sin argumentos de tipo explícitos en llamadas

Los genéricos se infieren siempre en posición de expresión. Lo explícito solo ocurre donde ya hay un tipo: `nuevo Caja<entero>(42)`, `variable x: vector<texto>`. Esto elimina la ambigüedad entre `a < b` y `a<b>(c)` **sin necesidad de turbofish**.

### 7. Arreglos con coma, no con punto y coma

`arreglo<entero, 10>` en lugar de `[entero; 10]`. Una sola forma de escribir genéricos.

### 8. Sin sufijos numéricos

`5`, `5.0`, `0xFF`, `0b1010`, `0o755`, con `_` como separador. El tipo lo da el contexto o la inferencia; el literal es polimórfico y por defecto `entero` o `flotante`.

### 9. `nulo` es un valor, no un tipo

`nulo` es la variante sin dato de `opcional<T>` y el patrón que la reconoce. No existe el tipo `nulo` ni la conversión implícita a él.

### 10. Visibilidad por defecto: `privado`

Los miembros son privados salvo que se declaren `publico`, `protegido` o `interna` (visible en el módulo). Es la opción segura; el linter sugiere `publico` cuando el miembro nunca se usa fuera del módulo.

### 11. Precedencia de `como` entre unario y `**`

`a como entero + 1` se lee `(a como entero) + 1`. La tabla completa está en la especificación §7.

### 12. `&&` y `||` con cortocircuito obligatorio

El cortocircuito es semántico, no una optimización: el operando derecho solo se evalúa si hace falta.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Obligar `;` | Rompe el objetivo "fácil como Python" y ensucia el código |
| `nuevo`/`nueva` como dos palabras | Duplica el espacio de palabras reservadas y obliga a conocer el género gramatical de cada tipo |
| Turbofish `::<>` | Sintaxis hostil para un lenguaje que quiere ser legible |
| Eliminar `interfaz` y dejar solo `rasgo` | Cierra la puerta a quien viene de Java, que es la mayoría del público objetivo inicial |
| `publico` por defecto | Cómodo, pero expone API accidentalmente; el coste aparece a largo plazo |

## Consecuencias

**Positivas**
- Cada construcción tiene una sola forma de escribirse: gramática más pequeña, errores más claros, formateador trivial.
- El parser no necesita retroceso en ningún caso.
- La migración mental desde Python, Java y Rust es directa.

**Costes**
- Dos palabras para un concepto (`interfaz`/`rasgo`) obliga a documentar ambas y a que el LSP las ofrezca juntas.
- La terminación por salto de línea exige reglas de continuación bien probadas (S14).
- Sin sufijos numéricos, algunos literales necesitan anotación de tipo explícita.

## Impacto en la implementación

- S07/S08: literales y comilla simple.
- S13/S14: continuación de línea y `;` opcional.
- S16: `interfaz` y `rasgo` al mismo nodo; `Este` como tipo.
- S24: inferencia de genéricos solo en expresiones.
