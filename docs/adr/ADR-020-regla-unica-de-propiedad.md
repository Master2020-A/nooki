# ADR-020 — Una sola regla de propiedad en las llamadas

| Campo | Estado |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-10 |
| **Sprint** | S28 |
| **Ámbito** | Lenguaje · semántica de propiedad |
| **Relación** | Unifica ADR-016, ADR-018 y el caso particular de `Vector` (S27) |

## Contexto

Tres tipos tienen dueño —`Caja<T>`, `texto` y `Vector<T>`— y cada uno aprendió la misma lección por
separado: al pasarlo a otro sitio, **el origen tiene que quedar vacío** o hay doble liberación.

En S27 eso produjo un fallo real de corrupción de montículo: `agregar` copiaba el texto y tanto la
variable local como el vector lo liberaban. La corrección se escribió dentro del caso especial de
`Vector`, que es exactamente donde no debe vivir una regla del lenguaje.

## Decisión

**Pasar un valor con dueño a un parámetro que no es préstamo lo mueve.**

```aura
funcion consumir(t: texto) -> entero {
    retornar t.largo()
}

variable a = "uno" + "!"
variable n = consumir(a)     // 'a' se mueve aquí
imprimir(a)                  // error NOK5001
```

Y con préstamo, no se mueve:

```aura
funcion prestado(t: &texto) -> entero {
    retornar t.largo()
}

imprimir(prestado(&a))       // correcto: prestar no transfiere
imprimir(a)                  // correcto: 'a' sigue siendo del llamante
```

La regla se implementa **una vez** en el verificador (`comprobar_argumentos`) y una vez en el
generador (`emitir_movimientos`), mirando el tipo del parámetro y el del argumento. No hay lógica
por tipo de dato.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Copiar siempre al pasar | Es lo que causó el fallo de S27: dos dueños del mismo búfer |
| Prohibir pasar valores con dueño por valor | Rompe el uso más natural (`funcion f(t: texto)` para solo leer) y obliga a escribir `&` en todas partes |
| Dejar la decisión a cada tipo en el generador | Es donde estaba y ya falló dos veces |
| Mover solo si el parámetro no es préstamo *y* el tipo es "grande" | Reglas invisibles que el programador no puede predecir |

## Consecuencias

**Positivas**
- Una sola regla, comprobada en un solo sitio, que cubre los tres tipos con dueño y los futuros.
- El error aparece en Nooki con span y sugerencia, no como corrupción de montículo en ejecución.
- Hace predecible el coste: pasar por valor transfiere, prestar no.

**Costes**
- Pasar una variable con dueño a una función que solo la lee la **invalida** para el llamante. Es
  la semántica de Rust y sorprende al principio; el mensaje de error sugiere usar `&`.
- El generador emite sentencias extra después de la llamada (no una expresión), así que el
  movimiento solo se aplica cuando la llamada aparece como sentencia o como inicializador. Una
  llamada anidada dentro de otra expresión todavía no mueve su argumento.

## Impacto en la implementación

- S28: `parametros_` en el generador para conocer qué parámetros son préstamos;
  `emitir_movimientos` tras la sentencia; marcado diferido en el verificador.
- S29 (`Mapa<K, V>`): hereda la regla sin escribir nada nuevo.
- Fase 7: los préstamos con tiempos de vida sustituyen la regla, no la contradicen.
