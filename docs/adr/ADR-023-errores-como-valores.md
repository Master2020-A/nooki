# ADR-023 — Errores como valores: `Resultado` y `?`

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-10 |
| **Sprint** | S40 |
| **Ámbito** | Lenguaje · tipos · control de flujo |
| **Relación** | Cumple la sección 9 de la especificación (sin excepciones) |

## Contexto

La especificación dice desde el primer día: **no hay excepciones**. Un fallo se representa con un
valor, no se lanza. Hasta ahora el lenguaje no tenía forma de expresarlo: las funciones solo podían
devolver un valor cualquiera o nada.

Sin esto, un programa real no puede fallar bien: no hay manera de decir "esto puede salir mal" y
obligar a quien llama a considerarlo.

## Decisión

1. **`Resultado`** es un tipo del lenguaje que representa **o un valor entero o un error de texto**:
   ```c
   typedef struct {
       int es_error;
       long long valor;
       nooki_texto error;
   } nooki_resultado;
   ```
2. **Constructores `Ok(valor)` y `Error(mensaje)`** crean resultados; el texto del error es del
   resultado y se libera con él.
3. **El operador `?`** propaga: si el resultado es un error, la función devuelve ese mismo error; si
   no, la expresión vale el entero guardado.
   ```aura
   funcion repartir(a: entero, divisor: entero) -> Resultado {
       variable cociente = dividir(a, divisor)?
       retornar Ok(cociente + 1)
   }
   ```
4. **`?` solo se permite en funciones que devuelven `Resultado`** (`NOK4011`).
5. **Comprobación explícita** mediante `es_error()`, `valor()` y `mensaje()`.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Excepciones | Contradicen el pilar de seguridad y la especificación; cuestan *unwinding* en C |
| `Resultado<T, E>` genérico desde el principio | Exige variantes con datos y monomorficación de tipos; es un sprint entero por sí solo |
| Códigos de error enteros | Pierden el mensaje y se ignoran con facilidad |
| `?` en cualquier expresión | Obligaría a emitir sentencias dentro de expresiones; se limita a declaraciones, que es el 90 % del uso |

## Consecuencias

**Positivas**
- Un fallo se ve en el tipo: quien llama sabe que puede salir mal.
- `?` elimina el ruido de comprobar y devolver en cada paso.
- Sin *unwinding*: el error es un valor y la liberación sigue siendo determinista.

**Costes**
- **Un solo tipo de resultado**: el éxito lleva un entero y el error un texto. Devolver un texto
  como éxito exigirá `Resultado<T, E>` genérico.
- `?` solo funciona en declaraciones (`variable x = f()?`), no dentro de una expresión más grande.
- Sin `coincide` sobre resultados: la comprobación es por métodos.

## Impacto en la implementación

- S40: `nooki_resultado` en el runtime, `Ok`/`Error` como integradas, `?` en declaraciones y
  verificación de contexto (`NOK4011`).
- S41: variantes con datos y `Resultado<T, E>` genérico; entonces `coincide` podrá desestructurar.
