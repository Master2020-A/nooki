# ADR-018 — `texto` con marca de propiedad

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-10 |
| **Sprint** | S23 |
| **Ámbito** | Lenguaje · memoria · runtime |
| **Relación** | Reutiliza el modelo de ADR-016 |

## Contexto

Hasta la v0.2 solo se podían imprimir literales de texto. Sin concatenación ni comparación por
contenido, el lenguaje no sirve para escribir nada que hable con el usuario.

`texto` es un valor con memoria: viene de un literal (estática) o de una concatenación (en el
montículo). Si un solo tipo tiene que representar ambas cosas, hay que saber **cuál hay que liberar**.

## Decisión

1. **`texto` es una estructura de tres campos**: `datos`, `largo` (en bytes) y `propio`.
   ```c
   typedef struct { char* datos; long long largo; int propio; } nooki_texto;
   ```
2. **`propio = 1`** significa que el búfer es del valor y debe liberarse; `propio = 0` significa que
   apunta a memoria estática (un literal) y no se libera.
3. **`soltar` consulta la marca**: libera solo si `propio`. Así, liberar un literal es inocuo y no
   hace falta distinguir tipos en el código generado.
4. **Movimiento**: asignar un `texto` transfiere; el origen queda con `datos = NULL` y `propio = 0`.
   Usarlo después es error de compilación (`NOK5001`), igual que con las cajas.
5. **`largo` es el número de bytes**, no de caracteres. Para UTF-8 coinciden salvo en los caracteres
   multibyte; se documenta y se corrige cuando llegue la iteración sobre texto.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Dos tipos, `&texto` y `texto` (como Rust) | Duplica el sistema de tipos y obliga a conversiones explícitas en cada llamada; llega con los préstamos completos |
| Literales siempre en el montículo | Cada literal reserva y hay que liberarlo: fugas por todas partes y un coste absurdo |
| `char*` terminado en cero como hasta ahora | No hay longitud, `largo()` cuesta un recorrido y no se puede comparar sin `strcmp` |
| Copiar siempre al asignar | Convierte el movimiento en copia y rompe la coherencia con `Caja` |

## Consecuencias

**Positivas**
- Un solo tipo cubre literales y textos construidos, sin conversiones visibles.
- La liberación es determinista y segura: un literal nunca se libera.
- `largo()` es O(1), y la comparación es por contenido y acotada por el mínimo de longitudes.

**Costes**
- Hay un campo extra (4 bytes con relleno) por cada texto, y una comprobación en cada liberación.
- `largo` cuenta bytes: un texto con tildes informa de más “caracteres” de los que ve el usuario.
- Los literales con `\0` embebido se truncan, porque `texto_literal` usa `strlen`.

## Impacto en la implementación

- S23: runtime `nooki_texto_*`; el generador registra los textos de cada ámbito junto a las cajas y
  los libera al cerrarlo.
- S24: `largo` pasará a contar caracteres cuando el recorrido de UTF-8 entre en el runtime.
- Fase 7: el modelo de propiedad sustituye la marca `propio` por tipos con préstamo, y `&texto`
  cubrirá lo que hoy hace `propio = 0`.
