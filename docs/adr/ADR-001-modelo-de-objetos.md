# ADR-001 — Modelo de objetos: `clase` por referencia, `estructura` por valor

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 |
| **Ámbito** | Lenguaje · sistema de tipos · memoria |
| **Origen** | Vacío en `Aurora1.txt` y `Aurora2.txt` |

## Contexto

Ninguno de los dos planes define si una instancia de `clase` vive en la pila o en el montículo, ni si asignar un objeto copia o comparte. Es la decisión que más condiciona el compilador: determina el *layout*, las tablas virtuales, la semántica de movimiento, la igualdad y la existencia misma de `nulo`.

La visión del proyecto exige dos cosas que tiran en direcciones opuestas: **OOP con herencia y polimorfismo** (favorece semántica de referencia) y **control de hardware con memoria determinista** (favorece semántica de valor).

## Decisión

Se adopta un **modelo híbrido de dos tipos de dato**:

1. **`clase` → semántica de referencia.** Se instancia con `nuevo`, vive en el montículo, tiene identidad y tabla virtual. Asignarla **comparte** la referencia; no copia. Es la base de la herencia y el polimorfismo.
2. **`estructura` → semántica de valor.** Vive donde se declara (pila, o embebida en otro tipo), se **copia** al asignarse si todos sus campos son copiables, y se **mueve** si no lo son. No tiene herencia ni tabla virtual.

Reglas derivadas, todas normativas:

- **Una referencia a `clase` nunca es `nulo`.** La ausencia de valor se expresa con `opcional<Clase>`. Esto elimina por diseño el error de desreferencia nula.
- **`==` significa identidad** en `clase` e **igualdad estructural** en `estructura`. Para igualdad estructural en clases se deriva con `@derivar(igualdad)`.
- **La herencia solo existe entre clases.** Una estructura puede implementar rasgos y tener métodos, pero no heredar.
- **`@repr(C)`** solo aplica a estructuras y a clases selladas sin métodos virtuales; es la única forma de cruzar la frontera FFI con *layout* garantizado.
- Anidamiento libre: una estructura puede contener una clase (referencia) y una clase puede contener estructuras (por valor, embebidas).

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Todo por referencia (Java, C#, Python) | Imposibilita el control de hardware y la memoria determinista; cada objeto paga cabecera e indirección |
| Todo por valor (C++, Rust) | La herencia con polimorfismo se vuelve una fuente inagotable de errores de *slicing*; contradice el pilar "OOP completa" |
| Clases por valor con herencia (C++) | Mezcla `move`, vtable y `nuevo` en el mismo tipo: máxima carga cognitiva para un lenguaje que se vende como "fácil" |
| Todo por referencia con punteros nulos | Reintroduce el error de mil millones de dólares que Rust demuestra que se puede eliminar |

## Consecuencias

**Positivas**
- Dos modelos mentales claros: "lo que se comparte" y "lo que se copia".
- Sin `nulo` en referencias desaparecen la mitad de los fallos clásicos de OOP.
- El usuario elige el coste: estructura para datos calientes, clase para jerarquías.

**Costes**
- Hay que explicar la diferencia desde el tutorial 1.
- `opcional<Clase>` añade fricción en código OOP cotidiano; se mitiga con `?.` y `??`.
- El sistema de tipos debe distinguir **copiable / movible / clonable** desde el primer día.

## Impacto en la implementación

- C++: `clase` → `struct` propia + vtable generada por nosotros; `estructura` → `struct` cuyo criterio de copia es `is_trivially_copyable`.
- El verificador de propiedad (S45) trata distinto ambos casos: **mover** una estructura la invalida; **copiar** una clase no invalida nada.
- El *drop glue* se genera por tipo, no por valor.
