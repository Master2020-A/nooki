# ADR-021 — `clase` por referencia y `nuevo`

| Campo | Valor |
|---|---|
| **Estado** | Aprobada — reemplaza a ADR-015 |
| **Fecha** | 2026-11-10 |
| **Sprint** | S36 |
| **Ámbito** | Lenguaje · objetos · memoria |
| **Relación** | Sustituye ADR-015; prepara ADR-001 (herencia y polimorfismo) |

## Contexto

ADR-015 dejó las clases **por valor** en la v0.1 con una promesa: cuando hubiera modelo de memoria,
volverían a ser referencias. El modelo ya está (S20–S28): dueño único, movimiento, préstamos y
liberación determinista.

Con clases por valor, la herencia no tiene sentido: `figura f = circulo(...)` **recorta** el objeto y
las tablas virtuales no pueden apuntar a nada estable. Es el último pilar que quedaba bloqueado.

## Decisión

**Una `clase` se reserva con `nuevo` y la variable es dueña de la referencia.**

```aura
clase contador {
    publico valor: entero

    publico funcion incrementar() -> vacio {
        este.valor = este.valor + 1
    }
}

variable c = nuevo contador { valor: 10 }
c.incrementar()
imprimir(c.valor)     // 11
```

1. **`nuevo Tipo { ... }` reserva en el montículo** y devuelve una referencia.
2. **La variable es la dueña**: se libera al cerrar el ámbito, con la misma maquinaria que `Caja`,
   `texto`, `Vector` y `Mapa`.
3. **Asignar mueve**: `variable q = p` transfiere; usar `p` después es `NOK5001`.
4. **Prestar no mueve**: `f(&p)` deja `p` intacta.
5. **`estructura` sigue siendo por valor.** La diferencia ya es real: `estructura` es un dato,
   `clase` es un objeto con identidad.
6. Los accesos usan `->` en el C generado (`p->valor`) y los métodos reciben el puntero.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Dejar las clases por valor y hacer herencia con incrustación | El recorte de objetos hace la herencia engañosa |
| Conteo de referencias | Exige liberar en todas las salidas e incrementar en cada copia; a medias produce dobles liberaciones (asunto A-09) |
| Recolector de basura | Contradice el pilar de liberación determinista |

## Consecuencias

**Positivas**
- Las clases tienen identidad y el objeto no se copia al pasarlo por una referencia.
- La herencia con tablas virtuales ya es posible (S37).
- Misma semántica de propiedad que el resto de tipos con dueño: una sola regla.

**Costes**
- Un `nuevo` por objeto y una indirección en cada acceso.
- No hay todavía constructores con lógica: `nuevo persona { ... }` solo rellena campos.
- Prestar una clase (`&persona`) y llamar a un método a través del préstamo funciona para los
  propios, pero la herencia traerá el despacho virtual.

## Impacto en la implementación

- S36: `nuevo` como prefijo; funciones `nooki_<clase>_nuevo` y `nooki_<clase>_soltar` por clase;
  accesos con `->`; liberación por puntero en el registro de ámbitos.
- S37: herencia y tablas virtuales sobre esta representación.
- Fase 7: el ownership sustituye la liberación simple por préstamos verificados.
