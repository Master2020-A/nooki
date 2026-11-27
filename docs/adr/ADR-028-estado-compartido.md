# ADR-028 — Estado compartido entre cierres

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-23 |
| **Sprint** | S149 |
| **Ámbito** | Lenguaje · propiedad · biblioteca |
| **Relación** | Depende de ADR-007 (memoria transitoria) y ADR-027 (cierres) |

## Contexto

Un cierre copia lo que captura (ADR-027). Eso está bien para leer un valor, pero no deja **acumular**: un
contador así no cuenta.

```nooki
funcion contador() -> cierre() -> entero {
    variable n = 0
    retornar funcion() -> entero => n + 1     // n es una copia; no se acumula
}
```

La pieza que falta es un sitio **compartido** al que el cierre pueda escribir y el resto leer. Nooki tiene
`Caja<T>` desde S20, pero es de **un solo dueño** (ADR-007): `{ long long* datos; }`, sin recuento y sin
copia. Capturarla no es seguro, y hasta hoy el compilador **no lo impedía**:

- el entorno del cierre copiaba la estructura, así que el cierre y el ámbito compartían `datos` — de hecho un
  contador así funcionaba;
- pero si el cierre **escapaba** (devolviéndolo de una fábrica), el ámbito liberaba `datos` y el cierre se
  quedaba apuntando a memoria libre. Al probarlo, esa ruta ni siquiera compila hoy por otro motivo, pero el
  agujero estaba.

Se cerró el agujero: **capturar una `Caja`, un `Vector` o un `Mapa` es ahora `NOK6003`**, con un mensaje que
lo explica. Es la decisión segura mientras no haya una forma de compartir que el compilador pueda comprobar.

## Decisión

1. **Hoy: no se comparte estado mutable.** Capturar una colección o una `Caja` es un error de compilación.
   El patrón que sí funciona es **pasar el estado como valor**: el cierre recibe el estado y devuelve el
   nuevo.

   ```nooki
   variable estado = 0
   variable siguiente = funcion(n: entero) -> entero => n + 1
   estado = siguiente(estado)     // 1
   estado = siguiente(estado)     // 2
   ```

   Es explícito y no cuesta nada en ejecución, pero obliga a llevar el estado a mano.

2. **Después: un tipo aparte, `Compartido<T>`.** Una celda con **recuento de referencias**:
   ```c
   typedef struct { long long* datos; long long* referencias; } nooki_compartido;
   ```
   Copiarla suma una referencia, soltarla resta, y a cero se libera. Capturarla en un cierre es entonces
   seguro: el cierre se queda una referencia y el entorno la suelta al morir.

3. **`Caja` no cambia.** Sigue siendo la celda de un solo dueño, la barata, la que se mueve. Hacer que
   `Caja` fuera copiable rompería ADR-007 y todas las reglas de movimiento que ya usan los programas: dos
   formas de propiedad en el mismo tipo es justo lo que el ADR-027 decidió no hacer.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Recuento de referencias en `Caja` | Cambia el modelo de propiedad de un tipo que ya está en los programas; un valor que a veces se mueve y a veces se copia según su contenido es imposible de razonar |
| Tiempos de vida: el cierre no puede sobrevivir a lo que presta | Es la solución «de manual», pero exige el verificador de préstamos completo: mucho más grande que los cierres |
| Caja global del programa (estilo estático) | Sin dueño, sin liberación, y no se puede tener dos contadores |
| Estado mutable sin comprobar (dejarlo pasar) | Un contador funciona y el siguiente programa tiene un *use-after-free*. En un lenguaje que vende seguridad de memoria, no |

## Consecuencias

**Positivas**
- No hay forma de compartir estado mutable **sin que el compilador lo sepa**: el agujero está cerrado.
- El patrón de pasar el estado como valor funciona hoy, no cuesta nada y encaja con la propiedad existente.
- `Compartido<T>` se puede añadir después sin tocar `Caja` ni los cierres: es un tipo más con su copia y su
  liberación.

**Costes**
- Los contadores «de verdad» (con estado escondido en el cierre) no se pueden escribir todavía.
- `Compartido<T>` pide un recuento por copia: hay que emitir `sumar` y `soltar` en los mismos sitios donde
  hoy se emite `soltar` para los tipos con dueño, y hay que decidir si es atómico (hilos) o no.
- Documentar el patrón de estado como valor es menos atractivo que un contador que «simplemente funciona».

## Impacto en la implementación

- **S149 (hecho)**: capturar `Caja`, `Vector` o `Mapa` es `NOK6003`, con el mensaje que dice por qué.
- **S150 (hecho, 1.0.22) y S159 (hecho, 1.0.31)**: `Compartido<T>` de entero, de texto y de una estructura sin campos con dueño — tipo, `nuevo`, `poner`, `sacar`, copia con recuento y liberación;
  captura permitida; una prueba con dos cierres que comparten el mismo contador.
- Docencia: en el manual, el patrón de estado como valor, y por qué compartir una `Caja` todavía no se puede.
