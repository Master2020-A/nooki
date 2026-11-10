# ADR-015 — En la v0.1, `clase` y `estructura` comparten representación

| Campo | Valor |
|---|---|
| **Estado** | Reemplazada por [ADR-021](ADR-021-clases-por-referencia.md) (S36) |
| **Fecha** | 2026-11-09 |
| **Sprint** | S17 |
| **Ámbito** | Generación de código · modelo de objetos |
| **Relación** | **Simplifica temporalmente** ADR-001 (clase por referencia, estructura por valor) |

## Contexto

ADR-001 decidió que `clase` tiene semántica de **referencia** (montículo, identidad, sin `nulo`) y `estructura` semántica de **valor**. Esa decisión es correcta y no se cambia.

Pero implementar la semántica de referencia exige lo que todavía no existe:

- asignación en el montículo y un propietario que libere (`nuevo` + destrucción),
- reglas de copia y de movimiento para no duplicar ni perder objetos,
- y, cuando llegue la herencia, tablas virtuales.

Nada de eso cabe en un sprint, y construir la mitad deja un lenguaje donde `nuevo` filtra memoria sin que nadie lo controle.

## Decisión

1. **En la v0.1, `clase` y `estructura` se compilan igual**: un `struct` de C, por valor, con copia al asignar.
2. **La palabra `clase` se acepta** y se documenta como «objeto por valor en esta versión».
3. **`nuevo` no se usa con clases en la v0.1**: la construcción es `persona { nombre: "Ana", edad: 30 }`.
4. **ADR-001 sigue vigente como objetivo.** Cuando llegue el modelo de memoria (S45 en adelante), `clase` pasará a representarse como puntero con propietario y esta ADR quedará reemplazada por otra que cite a ambas.
5. **El compilador avisa** en la documentación, no en cada compilación: sería ruido constante.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Implementar ya la semántica de referencia con `malloc` sin liberación | Filtra memoria en cada objeto y contradice el pilar de seguridad; además obliga a rehacerlo todo al llegar el ownership |
| No aceptar `clase` hasta S45 | Retrasa el requisito central del lenguaje varios años por un detalle de representación |
| Aceptar `clase` y tratarla como referencia con punteros sin verificar | Es exactamente la clase de atajo que produce *use-after-free* |

## Consecuencias

**Positivas**
- Los objetos con campos existen y funcionan hoy: se pueden escribir programas con registros.
- La sintaxis que aprenderá el usuario es la definitiva; solo cambia lo que ocurre por debajo.
- Cero deuda de memoria: nada que liberar, nada que filtrar.

**Costes**
- `persona a = b` **copia** en la v0.1, cuando la semántica final será compartir la referencia. Es una diferencia observable que habrá que documentar en el tutorial y que **romperá programas** al cambiarla. Aceptado: la estabilidad se promete a partir de la v1.0.
- La identidad de objetos (`a == b` por referencia) no existe todavía.

## Impacto en la implementación

- S17: `typedef struct { … } nooki_persona;` y construcción con literal compuesto de C.
- S45–S50: al introducir el ownership, `clase` pasa a `nooki_persona*` con `nuevo`/destructor; el acceso a campos cambia de `.` a `->` **solo en el generador**.
- El tutorial de S93 debe explicar la diferencia entre la v0.1 y la v1.0.
