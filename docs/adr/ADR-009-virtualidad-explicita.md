# ADR-009 — Virtualidad explícita y herencia

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 |
| **Ámbito** | OOP · ABI · generación de código |
| **Origen** | `Nooki2` usa `@sobrescribir` sin definir si los métodos son virtuales por defecto |

## Contexto

Toda decisión sobre herencia se reduce a una pregunta: **¿un método es polimórfico por defecto o hay que declararlo?** No es una cuestión de gusto; cambia el layout de los objetos, el coste de las llamadas y la compatibilidad binaria.

- **Virtual por defecto** (Java, Python, Swift): cómodo, pero cada objeto lleva vtable, cada llamada puede ser indirecta y es imposible añadir un método sin romper la ABI.
- **No virtual por defecto** (C++, C#): explícito y predecible, con la trampa clásica de olvidar la palabra y "perder" el polimorfismo en silencio.

## Decisión

1. **No virtual por defecto.** Un método solo puede sobrescribirse si se declara `@virtual`.
2. **`@sobrescribir` es obligatorio** al sobrescribir. Si falta, **error de compilación**, no advertencia. Si se aplica a un método que no sobrescribe nada, también es error.
3. **`@final`** prohíbe sobrescribir un método; **`@sellada`** prohíbe heredar de una clase.
4. **`abstracta`** (palabra reservada) marca clases y métodos sin implementación; una clase con métodos abstractos debe declararse `abstracta`.
5. **La tabla virtual se genera solo si la clase declara al menos un método `@virtual` o implementa un rasgo con métodos.** Las clases sin polimorfismo no pagan vtable.
6. **`super.metodo(...)`** llama a la implementación del padre, siempre explícito.
7. **La herencia es simple.** La composición múltiple se consigue con rasgos.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Virtual por defecto (Java) | Paga vtable siempre, impide eliminar la indirección y rompe la ABI con cada método nuevo |
| Virtual por defecto + `final` para optimizar | El usuario paga por defecto y debe recordar optimizar |
| No virtual sin obligar `@sobrescribir` (C++) | El error silencioso más caro de C++: se cree estar sobrescribiendo y no se sobrescribe |
| Herencia múltiple | Ambigüedad de diamante, *layout* imposible y desplazamiento de punteros; descartada |

## Consecuencias

**Positivas**
- Polimorfismo explícito: lo que se lee es lo que se ejecuta.
- Objetos sin vtable cuando no hace falta: menos memoria y llamadas directas.
- La ABI de una clase sin `@virtual` es estable ante métodos nuevos.

**Costes**
- Más ceremonia en el código OOP: cada método sobrescrito necesita dos anotaciones (`@virtual` en el padre, `@sobrescribir` en el hijo).
- Quien venga de Java olvidará `@virtual`. **Mitigación:** mensaje de error que explica exactamente qué falta y sugiere el arreglo.

## Impacto en la implementación

- S26 (semántica): construcción de la jerarquía, verificación de firmas y de `@sobrescribir`.
- S33 (codegen): layout con puntero a vtable solo si procede; llamadas directas cuando el método es `@final` o la clase está `@sellada`.
- La vtable se emite en el módulo LLVM de la clase base, con orden estable por declaración (necesario para `@repr(C)` y para el enlazado incremental).
