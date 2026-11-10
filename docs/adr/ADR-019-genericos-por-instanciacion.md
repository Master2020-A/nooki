# ADR-019 — Genéricos por instanciación explícita

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-10 |
| **Sprint** | S24 |
| **Ámbito** | Lenguaje · generación de código |
| **Relación** | Precede a las colecciones y a la generalización de `Caja<T>` |

## Contexto

`Caja<T>` solo funciona con enteros, y no hay forma de escribir una función que valga para varios
tipos sin duplicarla. Sin genéricos no puede haber `Vector<T>`, ni `Mapa<K, V>`, ni una biblioteca
estándar.

Hay dos caminos: borrado de tipos (como Java) o instanciación (como C++/Rust). El backend es C, y
en C no existe borrado sin punteros genéricos y despacho en ejecución.

## Decisión

1. **Instanciación en tiempo de compilación** con **argumentos de tipo explícitos**:
   ```aura
   funcion identidad<T>(valor: T) -> T {
       retornar valor
   }

   imprimir(identidad<entero>(42))       // nooki_identidad_entero
   imprimir(identidad<texto>("Nooki"))   // nooki_identidad_texto
   ```
2. **Sin inferencia todavía**: el llamante escribe los tipos. Inferirlos exige unificación y
   resolución de restricciones; se pospone hasta que haya más tipos que unificar.
3. **Cada instanciación es una función de C propia**, con nombre mangleado
   `nooki_<función>_<tipos>`, y se emite una sola vez por combinación.
4. **Las instanciaciones se descubren generando**: los cuerpos se emiten primero en un búfer, se
   recogen las llamadas genéricas y se emiten las instanciaciones hasta que no aparezcan nuevas.
5. **Solo funciones genéricas** en esta versión. Los tipos genéricos (`Vector<T>`) llegan con las
   colecciones, que es donde se necesitan de verdad.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Borrado de tipos con punteros `void*` | Pierde el tipado y mete despacho en ejecución: contrario al pilar de seguridad |
| Inferencia de tipos desde el primer día | Requiere unificación completa; retrasa el valor visible sin necesidad |
| Macroinstanciación textual (`#define`) | Los errores se reportan en C, no en Nooki, y se pierde el span |
| Genéricos solo en tipos, no en funciones | Las funciones genéricas son el caso más simple y el que valida el mecanismo |

## Consecuencias

**Positivas**
- Una función sirve para todos los tipos sin duplicar código fuente.
- El binario es tan rápido como el C escrito a mano: no hay despacho dinámico.
- El mecanismo es el mismo que necesitarán `Vector<T>` y `Caja<T>` general.

**Costes**
- El llamante debe escribir los tipos; no hay inferencia.
- El código generado crece con cada combinación de tipos (explosión de instanciaciones).
- Un tipo genérico usado en un parámetro que la función no admite falla al compilar el C, no en
  Nooki: la verificación de sustituciones es superficial todavía.

## Impacto en la implementación

- S24: `Funcion::tipo_parametros`, `Expresion::tipos_aplicados`, sustitución de tipos en el
  generador y emisión diferida de instanciaciones.
- S25: `Vector<T>` sobre este mecanismo, con `Caja<T>` generalizada de paso.
- Fase 7: inferencia y rasgos (`donde T: ...`) sobre la misma base.
