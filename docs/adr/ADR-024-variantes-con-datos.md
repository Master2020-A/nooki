# ADR-024 — Variantes con datos y patrones que ligan

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-10 |
| **Sprint** | S42 |
| **Ámbito** | Lenguaje · tipos · emparejamiento |
| **Relación** | Generaliza ADR-023 (`Resultado`) y completa la sección de enumeraciones |

## Contexto

Las enumeraciones eran listas de nombres sin datos, y `Resultado` era un tipo especial del
compilador con un entero o un texto. Ninguna de las dos cosas es un tipo de primera clase: no se
puede escribir una enumeración propia que lleve datos, ni emparejarla sacando el valor.

## Decisión

1. **Las variantes pueden llevar campos**:
   ```aura
   enumeracion respuesta {
       Bien(valor: entero)
       Mal(mensaje: texto)
   }
   ```
2. **Representación**: si ninguna variante lleva datos, la enumeración sigue siendo un entero con
   constantes (compatible con lo anterior). Si alguna lleva datos, se genera una **unión etiquetada**:
   ```c
   typedef struct nooki_respuesta nooki_respuesta;
   struct nooki_respuesta {
       int etiqueta;
       union {
           long long Bien_valor;
           nooki_texto Mal_mensaje;
       } datos;
   };
   ```
3. **Construcción** con el nombre de la variante: `Bien(10)` → literal compuesto con etiqueta y datos.
4. **Patrones que ligan**: `caso Bien(v)` declara `v` con el tipo del campo y la liga al valor.
5. **Cobertura**: si están todas las variantes, el `coincide` es exhaustivo sin `_`.
6. **Las variantes con texto se liberan**: se genera `nooki_<enum>_soltar` que suelta el campo de
   texto de la variante activa.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Dejar `Resultado` como tipo especial del compilador | No se pueden escribir enumeraciones propias con datos |
| Una estructura con todos los campos a la vez | Desperdicia memoria y hace imposible el emparejamiento seguro |
| `union` sin etiqueta | No se puede saber qué variante está activa |
| Exigir siempre `_` en el `coincide` | Molesto cuando ya están todas las variantes |

## Consecuencias

**Positivas**
- `Resultado` puede pasar a escribirse en Nooki: es una enumeración más.
- El emparejamiento extrae el dato con su tipo, sin conversiones.
- Las enumeraciones sin datos no cambian de representación: nada de lo anterior se rompe.

**Costes**
- Las variantes con texto necesitan liberación explícita; el tipo no la arrastra consigo.
- El `coincide` no comprueba que las ligaduras cubran todos los campos de una variante en tiempo de
  ejecución: la comprobación de aridad es en compilación, sin más.
- Un tipo de enumeración con datos no se puede copiar a la ligera sin decidir quién es el dueño del
  texto; hoy el compilador lo libera solo al salir del ámbito, como el resto.

## Impacto en la implementación

- S42: `Variante` con campos en el AST, parser de variantes y de ligaduras, unión etiquetada,
  constructores por nombre y `coincide` por etiqueta con ligaduras.
- S43: `Resultado` escrito en Nooki sobre este mecanismo, y entonces sí `Resultado<T, E>`.
