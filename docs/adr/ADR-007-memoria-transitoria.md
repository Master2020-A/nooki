# ADR-007 — Gestión de memoria transitoria (RC/GC) entre S44 y S52

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 (decisión) · S44 (implementación) · S52 (eliminación) |
| **Ámbito** | Runtime · estrategia de desarrollo |
| **Origen** | Ninguno de los dos planes resuelve cómo ejecutar programas antes de tener el verificador de préstamos |

## Contexto

El verificador de préstamos (Fase 7, S45–S52) llega **dos años después** del primer programa compilado (S36). Si el lenguaje no puede gestionar memoria hasta entonces:

- no hay programas reales que probar durante 16 sprints;
- la librería estándar (S62–S70) no puede empezar;
- la ergonomía no se valida con usuarios;
- todos los errores de diseño se descubren tarde y caros.

Rust pasó por esto: sus primeras versiones tenían punteros con recolector (`@`, `~`) antes de llegar a la propiedad y los préstamos. No fue un error de juventud: fue una estrategia de secuenciación.

## Decisión

1. **Entre S44 y S52, el lenguaje incluye una gestión automática temporal** basada en conteo de referencias (ARC) para los tipos que lo necesiten, activada por módulo:
   ```nooki
   @gestion_automatica
   modulo prototipo
   ```
2. **Marca visible y ruidosa.** Todo módulo con `@gestion_automatica` emite una advertencia en cada compilación: *«gestión automática temporal: se elimina en NOOKI v0.6»*.
3. **Fecha de caducidad contractual: S52.** En ese sprint se elimina el atributo, se migran los ejemplos y cualquier programa que lo use deja de compilar, con un mensaje que explica la alternativa (`&`, `&mut`, `contador_ref`).
4. **No contamina la semántica del lenguaje final:** el atributo no aparece en la especificación v1.0, solo en las versiones 0.4–0.5.
5. **La suite de conformidad etiqueta estos casos** con `@temporal` para poder retirarlos mecánicamente.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Esperar al verificador de préstamos para ejecutar cualquier programa | Cuatro años sin programas reales; la stdlib se bloquea; imposible validar la ergonomía |
| Recolector de basura definitivo | Contradice el pilar "sin GC" y arruina el control de hardware y los tiempos deterministas |
| Solo pila (sin montículo) hasta S52 | `texto`, `vector`, `mapa` y `clase` no existirían: el lenguaje no sirve para nada real |
| Reutilizar `contador_ref` desde el principio conscientemente | Es lo que se hace, pero **sin** atributo explícito el usuario creería que es permanente → deuda silenciosa |

## Consecuencias

**Positivas**
- Hay lenguaje usable 16 sprints antes.
- La stdlib y los ejemplos arrancan en S39 sin esperar a S52.
- Los usuarios tempranos descubren problemas de ergonomía mientras aún es barato corregirlos.

**Costes**
- **Dos caminos de generación de código** durante 8 sprints: hay que mantener ambos y probar el doble.
- Riesgo real de que alguien construya sobre el GC temporal. **Mitigación:** advertencia obligatoria, fecha pública y `@temporal` en los tests.
- Eliminar el atributo en S52 es un cambio incompatible. **Aceptado:** la estabilidad se promete a partir de v1.0, no antes.

## Impacto en la implementación

- S38: `contador_ref` y `contador_ref_atomico` en `core`.
- S44: atributo `@gestion_automatica` + inserción automática de *drops* de referencias contadas en MIR.
- S52: eliminación del atributo, actualización de la especificación y del libro.
- El código generado debe marcar los tipos gestionados en los metadatos de depuración para poder auditar su uso.
