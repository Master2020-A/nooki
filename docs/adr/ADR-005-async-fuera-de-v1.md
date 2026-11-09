# ADR-005 — `async`/`await` queda fuera de la v1.0

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 |
| **Ámbito** | Lenguaje · runtime · librería estándar |
| **Origen** | `Nooki1` lo asigna a la Fase 3 (runtime) y `Nooki2` a la Fase 5; ninguno lo diseña |

## Contexto

Ambos planes prometen `asincrono` / `esperar` sin especificar nada: ni ejecutor, ni cancelación, ni qué ocurre con un préstamo que cruza un `await`. En Rust, el diseño de `async` estuvo **años** en desarrollo precisamente porque interactúa con todo:

- necesita `Pin` (direcciones estables) y por tanto un sistema de tipos que lo soporte;
- necesita `Send`/`Sync` para que los futuros puedan moverse entre hilos;
- multiplica los préstamos por cada punto de suspensión;
- contamina las firmas de funciones (funciones de color) y la inferencia de tipos.

Un lenguaje que promete seguridad de memoria y que aún no tiene el verificador de préstamos terminado no puede permitirse además el coste de `async`.

## Decisión

1. **La v1.0 no incluye `async` ni `await`.** Las palabras `asincrono` y `esperar` quedan **reservadas** para v1.1 y el compilador emite un error explícito: *«`asincrono` está reservado para NOOKI v1.1; use hilos y canales en la v1.0»*.
2. **Modelo de concurrencia de la v1.0:** hilos del sistema operativo, `mutex`, `rwlock`, canales e `hilos.trabajo(n)` (grupo de hilos). Es suficiente para servidores con un hilo por conexión o *thread pool*.
3. **La red de la v1.0 es bloqueante.** Documentado desde el tutorial de concurrencia.
4. **El diseño de v1.1 se documenta en S92**, no antes, y requiere su propio ADR.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Implementar `async` desde el principio | Duplica el coste del verificador de préstamos; es la causa nº 1 de complejidad en Rust moderno |
| `async` sin `Pin` (futuros en el montículo) | Implica asignaciones dinámicas en cada estado → rendimiento impredecible |
| Usar hilos con pilas grandes y llamarlo "async" | Engaña al usuario y no escala a decenas de miles de conexiones |
| Reservar las palabras pero documentar solo la intención | Es exactamente lo que hacen los planes actuales: una promesa sin diseño |

## Consecuencias

**Positivas**
- Un subsistema menos que retrasa la v1.0.
- El verificador de préstamos se diseña sin la restricción adicional de los puntos de suspensión.
- Los mensajes de error enseñan el camino correcto (hilos y canales).

**Costes**
- Frente competitivo: un servidor con 10 000 conexiones requerirá un grupo de hilos grande o un servidor escrito en otra tecnología.
- `nooki_red` y los ejemplos de servidor deben escribirse con el modelo bloqueante.
- Habrá presión de la comunidad para adelantarlo. **Mitigación:** ADR público, requisito de diseño y fecha orientativa v1.1.

## Impacto en la implementación

- El lexer reconoce `asincrono` y `esperar` como palabras reservadas desde S07 (para no romper compatibilidad futura).
- El parser emite un diagnóstico específico (`NOK2xxx`) en lugar de "se esperaba una expresión".
- La interfaz de `hilos` (S42) se diseña con un trait `iterador`/`ejecutor` que permita añadir un ejecutor asíncrono después sin romper API.
