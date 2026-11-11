# ADR-025 — La biblioteca estándar se escribe en Nooki

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-10 |
| **Sprint** | S43 |
| **Ámbito** | Lenguaje · biblioteca · compilador |
| **Relación** | Completa ADR-024 y retira el tipo especial de ADR-023 |

## Contexto

`Resultado` nació en S40 como un tipo **del compilador**: una estructura `nooki_resultado` en el
runtime, con `Ok`/`Error` reconocidos por el verificador y el generador como si fueran funciones
integradas, y un operador `?` que conocía sus campos por su nombre.

Era la decisión correcta cuando no había variantes con datos. Pero desde S42 **sí las hay**, y eso
significa que un tipo que puede expresarse en el lenguaje no debería estar escrito en C++ dentro del
compilador.

## Decisión

1. **`resultado` es un módulo de la biblioteca estándar escrito en Nooki**:
   ```aura
   publico enumeracion resultado {
       Ok(valor: entero)
       Error(mensaje: texto)
   }
   ```
2. **El módulo viaja dentro del compilador**: `usar resultado` lo carga de un texto integrado, sin
   necesidad de archivo ni de ruta de búsqueda. Es la semilla de un sistema de paquetes.
3. **`?` deja de conocer un tipo concreto**: funciona con **cualquier enumeración que tenga forma de
   resultado**, es decir, al menos dos variantes donde la primera lleva un único campo.
4. **Se retira el tipo especial**: fuera `nooki_resultado` del runtime, fuera los `Ok`/`Error`
   integrados, fuera el caso especial en la verificación. El compilador adelgaza.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Dejar `Resultado` como tipo del compilador | Un tipo que se puede escribir en el lenguaje no debe vivir en C++ |
| Un archivo `resultado.oki` junto a cada programa | Obliga a copiar la biblioteca en cada proyecto |
| Una ruta de búsqueda de paquetes | Es el sprint de paquetes, no este; el texto integrado da lo mismo hoy con menos maquinaria |
| `?` específico de `resultado` | Una enumeración propia con la misma forma debería servir igual |

## Consecuencias

**Positivas**
- **El compilador adelgaza**: menos código especial, menos superficie que mantener.
- La biblioteca crece en Nooki, que es la señal de que el lenguaje se puede usar para escribir el
  lenguaje.
- `?` es estructural: no depende de un nombre, sino de una forma.
- El módulo integrado es la base del futuro sistema de paquetes.

**Costes**
- El texto de la biblioteca vive dentro del binario del compilador; para crecer necesitará archivos
  y una ruta de búsqueda.
- `?` reconoce la forma por convención (primera variante con un campo), no por un rasgo declarado.
  Cuando existan los rasgos, esto se expresará con uno.

## Impacto en la implementación

- S43: `fuente_biblioteca` en el conductor, `?` sobre enumeraciones, retirada del tipo especial y
  migración del caso de conformidad `0031-resultados`.
- S44: más módulos de biblioteca (`texto`, `matemáticas`) y el sistema de paquetes.
