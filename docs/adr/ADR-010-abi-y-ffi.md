# ADR-010 — ABI, *name mangling* y frontera FFI

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 (decisión) · S37 (implementación) |
| **Ámbito** | Generación de código · enlazado · interoperabilidad |
| **Origen** | `Nooki2` promete FFI con C y C++ sin definir *layout* ni símbolos |

## Contexto

La interoperabilidad con C/C++ es un pilar ("control de hardware como C/C++") y a la vez el punto donde más lenguajes fracasan. Los detalles que hay que fijar **antes** de generar código:

- qué *layout* tiene una estructura cuando cruza la frontera;
- cómo se llaman los símbolos (si el nombre del símbolo depende de la firma, no hay enlace estable);
- quién libera la memoria asignada por cada lado;
- qué pasa si una función de Nooki entra en pánico dentro de una llamada de C.

## Decisión

1. **`@repr(C)`** garantiza *layout* compatible con C para estructuras y para clases selladas sin métodos virtuales. Sin él, el *layout* es **opaco y no especificado** (el compilador puede reordenar campos).
2. **Esquema de *mangling* propio y estable**, documentado en `docs/especificacion/abi.md`:
   - Funciones normales: `_A<longitud_modulo>_<modulo>_<longitud_nombre>_<nombre>` (longitud prefijada para evitar colisiones con `_`).
   - Sobrecargas: se añade un sufijo hash de 8 caracteres calculado sobre la firma normalizada.
   - El *mangling* **no cambia entre versiones menores**; un cambio exige una "edición" nueva (ADR-012).
3. **`externo "C"`** declara símbolos externos con ABI de C; **`@exportar("nombre")`** publica una función de Nooki con nombre C explícito y sin *mangling*.
4. **Variádicas:** se admiten solo en declaraciones `externo "C"` (`...`), con la regla de promociones de C. No se pueden escribir funciones variádicas en Nooki en v0.1.
5. **Propiedad de la memoria en la frontera:** una función `externo "C"` **no toma propiedad** de sus argumentos; los punteros son prestados y solo válidos durante la llamada. La asignación y liberación cruzadas se documentan por API (`malloc`/`free` del lado C, `asignar`/`liberar` del lado Nooki).
6. **Los pánicos no cruzan la frontera.** El compilador envuelve la exportación en una barrera que aborta con mensaje en español si hay pánico.
7. **C++:** se admite llamar a funciones `extern "C"` de C++. La interoperabilidad con clases y *templates* de C++ **queda fuera de la v1.0** (requiere un *frontend* de C++, como libclang, y es un proyecto en sí mismo).

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| *Mangling* basado en el nombre de C++ (Itanium/MSVC) | Acopla el lenguaje a un compilador concreto y a versiones de ABI ajenas |
| Sin *mangling* (todo como C) | Imposible con módulos y sobrecargas |
| Todo tipo con *layout* de C por defecto | Impide optimizaciones (reordenar campos para empaquetar) y rompe la promesa de rendimiento |
| Soporte de C++ desde el día uno | Meses de trabajo solo para nombres y excepciones; se pospone y se declara fuera de alcance |
| Dejar la propiedad de la memoria implícita | Es la fuente número uno de *use-after-free* en FFI |

## Consecuencias

**Positivas**
- La frontera con C es predecible y auditable.
- El *mangling* estable permite bibliotecas compiladas por separado y actualizaciones sin recompilar todo.
- La regla de "no se toma propiedad" elimina una clase entera de errores.

**Costes**
- El usuario debe recordar `@repr(C)`; sin él, los datos no cruzan. **Mitigación:** error específico cuando un tipo no `@repr(C)` se pasa a `externo "C"`.
- Sin clases de C++ en la v1.0, la interoperabilidad con bibliotecas como Qt o libtorch se hace por su API de C o por *wrappers* en C.
- El *mangling* propio complica el enlazado con herramientas de terceros (`nm`, `c++filt`). **Mitigación:** se documenta y `nooki símbolos` lo desazucara.

## Impacto en la implementación

- S32: `@repr(C)` y verificación de tipos compatibles.
- S37: `externo`, `@exportar`, barrera de pánico, variádicas, enlace con libc.
- S55/S94: el *mangling* estable es requisito para publicar paquetes precompilados y para la firma de binarios.
