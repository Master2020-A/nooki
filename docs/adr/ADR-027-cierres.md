# ADR-027 — Cierres: funciones que llevan su entorno

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-20 |
| **Sprint** | S144–S146 |
| **Ámbito** | Lenguaje · tipos · propiedad |
| **Relación** | Extiende ADR-019 (funciones como valor) y reutiliza el modelo de propiedad de ADR-007 |

## Contexto

Desde la 1.0.13 una función se guarda en una variable y se pasa a otra. Es un **puntero**: no lleva nada
consigo. Por eso una lambda solo ve sus parámetros, y usar una variable de fuera es un error:

```nooki
variable base = 10
variable f = funcion(n: entero) -> entero => n + base   // NOK3001: no se ha declarado «base»
```

Ese límite corta justo los tres usos que más se piden: un `apply` sobre una tabla, un *callback* que
recuerde algo (un contador, una lista donde apuntar), y un comparador que ordene por un criterio calculado
antes. Sin cierres, cada uno obliga a subir el estado por parámetro hasta el sitio de la llamada, que es
exactamente lo que la función debía encapsular.

La pregunta que este ADR resuelve no es cómo capturar —eso es mecánico— sino **quién posee el entorno
capturado y cuándo se libera**. Una respuesta mala aquí no se puede cambiar después sin romper programas.

## Decisión

1. **Tipo nuevo: `cierre(A, B) -> R`.** No se toca `funcion(A, B) -> R`, que sigue siendo un puntero sin
   entorno. Una lambda que **no captura nada** sigue siendo del tipo `funcion`, y no paga nada. Una que
   captura es del tipo `cierre`. Se llaman igual, así que la sintaxis no cambia.

2. **El cierre es dueño de su entorno y se mueve, no se copia.** Es la misma regla que ya tienen `Vector` y
   `Mapa`: se pasan moviendo, el origen no se puede volver a usar (`NOK5001`, ya implementado) y al salir
   del ámbito se liberan. No hay recuento de referencias, ni recolector, ni una segunda forma de propiedad.

3. **Representación:**
   ```c
   typedef struct {
       void* codigo;              /* la función, con el entorno delante */
       void* entorno;             /* puede ser NULL si no captura nada */
       void (*soltar)(void*);     /* libera el entorno al final */
   } nooki_cierre;
   ```
   Llamar es `((R (*)(void*, A, B))f.codigo)(f.entorno, args...)`. El `codigo` de un cierre siempre recibe
   el entorno como primer argumento, aunque lo ignore.

4. **La captura es implícita y por copia.** El cuerpo de la lambda puede nombrar variables del ámbito donde
   se escribe; el generador las mete en una estructura del entorno, copiadas:
   - los tipos con valor (enteros, flotantes, booleanos) se copian tal cual;
   - `texto` se copia de verdad, con `nooki_texto_copiar`, y el entorno lo libera al final;
   - `Vector`, `Mapa` y `Caja` **no se pueden capturar** en v1: son valores que se mueven, y copiarlos no
     está implementado. El error lo dice y propone pasar los datos por parámetro.

5. **Un cierre se puede devolver.** Devolver un cierre es moverlo fuera, así que esto —que hoy es imposible—
   pasa a funcionar:
   ```nooki
   funcion contador() -> cierre() -> entero {
       variable n = 0
       retornar funcion() -> entero => n + 1
   }
   ```
   La variable `n` viaja **dentro** del entorno copiado. Ojo con lo que eso significa: cada llamada a
   `contador()` da un contador nuevo, y `n` no se comparte con el exterior. Un contador que de verdad
   acumule necesita una `Caja` compartida, que es otro trabajo.

6. **Lo que queda fuera de v1, a propósito:** capturar `&mutable` (dos dueños del mismo sitio), `cierre`
   dentro de un `Vector` o de una estructura (necesitaría copia del entorno), y capturar por movimiento
   (`mover base`).

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Entorno con recuento de referencias | Copiar un cierre exigiría incrementar y decrementar en cada asignación, parámetro y retorno. Es una **segunda** forma de propiedad conviviendo con la de `Vector`, y la que más se equivoca |
| Entorno prestado, con tiempos de vida | Es la más barata, pero **no permite devolver** un cierre: mata el uso principal (fábricas de callbacks) |
| Reservar el entorno y no liberarlo nunca | Fuga garantizada en un bucle |
| Un solo tipo (hacer que `funcion` sea un par) | Rompe todos los punteros a función actuales sin ganar nada cuando no hay captura; se puede unificar después, haciendo `cierre` con entorno nulo |
| Captura explícita `|base|` | Más ceremonia para el 90 % de los casos; el verificador ya sabe qué nombres son de fuera |

## Consecuencias

**Positivas**
- `apply`, *callbacks* con estado y comparadores con criterio propio pasan a ser posibles **dentro** del
  lenguaje, sin el puente de Python.
- No inventa un modelo de propiedad: un cierre es un valor con dueño que se mueve, como un `Vector`.
- Una lambda sin captura no cambia de tipo ni de coste: lo que ya funciona sigue igual.
- Devolver funciones deja de ser imposible.

**Costes**
- Un cierre **no se copia**: si hay que dárselo a dos sitios, hay que llamar dos veces a la fábrica.
- El generador tiene que emitir, por lambda con captura, la estructura del entorno, su copia y su
  `soltar`; y por cada uso, el movimiento.
- El verificador tiene que distinguir el tipo de una lambda según capture o no —lo que obliga a analizar
  los nombres libres del cuerpo— y aplicar las reglas de movimiento a `cierre`.
- Un cierre no puede vivir en un `Vector` ni en un campo de estructura en v1.

## Impacto en la implementación

- **S144**: tipo `cierre`, análisis de nombres libres en la lambda, estructura del entorno, copia de
  capturas, `soltar`. Un cierre se llama y se libera.
- **S145**: movimiento (asignación, parámetro, retorno), reglas del verificador y diagnóstico de captura
  imposible (`Vector`/`Mapa`/`Caja`).
- **S146**: pruebas — fábrica de contadores, `apply` con criterio capturado, comparador con orden fijado
  antes — y documentación.
- Fase posterior: `Caja` compartida entre cierres, captura por movimiento y cierres dentro de colecciones.
