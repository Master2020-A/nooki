# ADR-022 — Herencia simple y despacho por tabla virtual

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-10 |
| **Sprint** | S37 |
| **Ámbito** | Lenguaje · objetos |
| **Relación** | Aplica ADR-021 (clases por referencia) y cumple ADR-001 |

## Contexto

Con las clases por referencia (ADR-021) el cuello de botella desapareció: un puntero a derivada
puede usarse donde se espera la base, y una tabla virtual tiene dónde apuntar. Antes, con clases por
valor, la herencia habría recortado objetos (*slicing*) y el despacho no habría tenido sentido.

## Decisión

1. **Herencia simple** con `hereda`:
   ```aura
   clase figura {
       publico nombre: texto
       publico funcion area() -> entero { retornar 0 }
   }

   clase cuadrado hereda figura {
       publico lado: entero
       publico funcion area() -> entero { retornar este.lado * este.lado }
   }
   ```
2. **Los campos se aplanan**: la estructura de C lleva primero los heredados y después los propios,
   con el puntero a la tabla virtual al principio. Así, un `nooki_cuadrado*` se puede usar como
   `nooki_figura*` sin ajuste de offset.
3. **Despacho por tabla virtual**: la raíz de la jerarquía define el tipo `nooki_figura_vtabla` con
   un hueco por método suyo; cada clase tiene su tabla y su `nuevo` la instala.
4. **Un método es virtual si la raíz de la jerarquía lo declara.** Los métodos que solo existen en
   una clase derivada se llaman directamente, sin índice en la tabla.
5. **Las implementaciones heredadas se reutilizan**: si una clase derivada no redefine un método, su
   tabla apunta a la implementación de la base.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Herencia múltiple | Complica el aplanado y el despacho; nadie la necesita todavía |
| Sin tablas virtuales (solo reutilización) | No habría polimorfismo: el objetivo del sprint |
| Incrustar la base como campo con nombre (`objeto.base`) | Rompe el uso de un puntero derivado como base sin conversión |
| Despacho por `switch` sobre un identificador de tipo | Funciona, pero cierra la jerarquía y ensucia cada llamada |

## Consecuencias

**Positivas**
- **Polimorfismo real**: dos clases derivadas responden distinto al mismo mensaje.
- La representación en C es la habitual: compatible con código C que use el mismo diseño.
- La reutilización de implementaciones es implícita y no cuesta código.

**Costes**
- Sin `super` todavía: una derivada no puede invocar la implementación de su base.
- Sin conversión explícita de un puntero derivado a una variable de tipo base (falta la
  comprobación de asignabilidad entre clases de la misma jerarquía).
- Una indirección por llamada virtual.
- Sin constructores con lógica: `nuevo` solo rellena campos, y los heredados hay que darlos por su
  nombre en la construcción de la derivada.

## Impacto en la implementación

- S37: `hereda` en el parser; `base_de_`, `campos_planos_`, `virtuales_de_` en el generador; tablas
  virtuales declaradas antes de las estructuras y definidas después; métodos virtuales con `void*`.
- S38: `super`, conversión a la base y verificación de firmas al sobrescribir.
