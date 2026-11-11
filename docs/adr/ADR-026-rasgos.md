# ADR-026 — Rasgos y despacho por rasgo

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-10 |
| **Sprint** | S54–S55 |
| **Ámbito** | Lenguaje · tipos · polimorfismo |
| **Relación** | Complementa ADR-022 (herencia) y ADR-024 (variantes) |

## Contexto

La herencia resuelve el polimorfismo **dentro de una jerarquía**: dos clases que comparten un padre
pueden tratarse igual. Pero la mayoría de las veces dos tipos que deberían tratarse igual **no
comparten padre**: una `persona` y un `robot` saben saludar sin ser parientes.

Sin un mecanismo para eso, el programador duplica funciones o inventa una base común artificial que
arrastra campos que no necesita.

## Decisión

1. **`rasgo Nombre { funcion f(...) -> tipo }`** declara un contrato: un conjunto de firmas.
2. **`clase X implementa A, B`** se compromete a cumplirlo. El compilador exige que **estén todos**
   (`NOK3011`) y que la **firma coincida** (`NOK4012`).
3. **`&Rasgo` es un tipo**: una referencia que lleva el **objeto** y su **tabla de métodos**.
   ```aura
   funcion presentar(quien: &Saluda) -> vacio {
       imprimir(quien.saludar())   // despacha al tipo concreto
   }

   presentar(&p)   // persona
   presentar(&r)   // robot
   ```
4. **Una tabla por (tipo, rasgo)**, con un **adaptador por método** cuya firma es la del rasgo y que
   llama a la implementación del tipo.
5. **Un rasgo sin `&` no genera código**: es solo verificación en compilación. El coste de la
   indirección se paga únicamente cuando se usa como tipo.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Solo herencia | No cubre tipos sin relación; obliga a inventar bases artificiales |
| Rasgos solo declarativos (sin `&Rasgo`) | Útiles para documentar, insuficientes para polimorfismo |
| Punteros a función sueltos | Sin verificación ni adaptadores; el usuario gestiona los tipos a mano |
| Tabla global por rasgo con identificador de tipo | Pierde el despacho directo y añade una búsqueda por llamada |

## Consecuencias

**Positivas**
- **Polimorfismo entre tipos no emparentados**, que es lo que la herencia no puede dar.
- El contrato se comprueba antes de generar código; el binario solo paga si usa `&Rasgo`.
- Los adaptadores evitan castings inseguros en las llamadas.
- Dos formas de polimorfismo claras: jerarquía (`&Figura`) o contrato (`&Saluda`).

**Costes**
- Una tabla y un adaptador por (tipo, rasgo): código generado proporcional a las implementaciones.
- `&Rasgo` no es dueño del objeto: presta. Si el objeto muere, la referencia queda colgando; el
  verificador no lo detecta todavía (es el terreno de los tiempos de vida).
- Los métodos de un rasgo no pueden ser genéricos ni llevar `este` explícito.

## Impacto en la implementación

- S54: `rasgo`, `implementa`, verificación de métodos y firmas.
- S55: tipo `&Rasgo`, tabla por (tipo, rasgo), adaptadores, conversión en declaraciones y argumentos,
  despacho en las llamadas.
- Fase de tiempos de vida: la duración de la referencia se verificará como la de cualquier préstamo.
