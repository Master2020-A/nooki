# Cómo contribuir a Nooki

Gracias por asomarte. Nooki está en la **versión 1.0**: el lenguaje funciona, está probado en dos
compiladores y no tiene defectos conocidos. Lo que falta está listado, con su tamaño, en
[DECISIONES.md](DECISIONES.md).

Es un buen momento para entrar: hay huecos pequeños que cierran cosas grandes.

## Antes de escribir código

1. Lee el [LEEME](LEEME.md) para saber qué es y qué no es el proyecto.
2. Lee [DECISIONES.md](DECISIONES.md): las decisiones cerradas no se rediscuten sin un RFC, y las
   abiertas están esperando a alguien.
3. Mira el [plan de sprints](PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md): cada fila es un sprint con su
   criterio de terminado.

## Reglas del proyecto

### Código

- **C++20**, sin dependencias externas. Solo la biblioteca estándar.
- Estilo del repositorio: 100 columnas, nombres de dominio en español (`verificar`, `emitir`,
  `ambito`), miembros con guion bajo final (`campos_`).
- **Sin `#include` en medio de un archivo** ni código comentado: lo que no se usa se borra.
- Todo lo que el compilador genere debe compilar **sin avisos** en GCC y MSVC.

### Pruebas

Ninguna característica entra sin su prueba. Hay tres niveles:

| Nivel | Dónde | Qué prueba |
|---|---|---|
| Unitarias | `compilador/pruebas/` | una fase del compilador, en memoria |
| Conformidad válida | `pruebas/conformidad/valido/` | un programa que se compila, se ejecuta y su salida coincide con `esperado.txt` |
| Conformidad inválida | `pruebas/conformidad/invalido/` | un programa que debe emitir exactamente los códigos de `esperado.txt` |

```bash
$ cmake --preset windows-mingw      # o linux, macos, windows-msvc-2022
$ cmake --build --preset windows-mingw
$ ctest --preset windows-mingw
```

**Un cambio que rompe `ctest` no está terminado**, aunque el programa que lo motivó funcione.

### Diagnósticos

Cada código `NOK` necesita tres cosas:

1. La constante en `compilador/incluir/nooki/diagnostico/codigos.hpp`.
2. El mensaje en español e inglés en `compilador/fuente/diagnostico/catalogo.cpp`, con parámetros
   `{asi}`.
3. **Un caso que lo provoque**, en conformidad o en una suite.

Y el mensaje debe decir **qué hacer**, no solo qué pasó:

```
error[NOK5001]: no se puede usar «a»: el valor se movió antes
   = ayuda: se movió a otro sitio: use «&» para prestarlo sin transferirlo
```

### Documentación

Si el cambio afecta al lenguaje, actualiza en el mismo commit:

- `spec/especificacion-v0.1.md` (una sección nueva o la existente),
- `CHANGELOG.md`,
- `GUIA.md` si es algo que un usuario principiante necesita,
- `PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md` (la fila del sprint).

Hay una prueba, `prueba_documentos`, que comprueba que los sprints terminados figuran en el historial
y en el LEEME, y que las versiones coinciden. **Si se te olvida algo, `ctest` lo dice.**

### Commits

- Mensaje en español, en imperativo: `biblioteca: anade el modulo textos`.
- Firmados: `git commit -s`.
- Un commit por sprint cuando trabajes por sprints, o un commit por cambio coherente.
- El cuerpo del mensaje explica **qué** y **por qué**, no repite el título.

## Cómo empezar

```bash
$ git clone <repositorio>
$ cd nooki
$ cmake --preset windows-mingw     # o linux, macos
$ cmake --build --preset windows-mingw
$ ctest --preset windows-mingw
$ build/windows-mingw/compilador/nooki --version
```

Y después:

```bash
$ build/windows-mingw/compilador/nooki ejecutar pruebas/conformidad/valido/0001-*/principal.oki
```

## Dónde mirar primero

| Buscas | Empieza por |
|---|---|
| Entender el lenguaje | [GUIA.md](GUIA.md) |
| Entender una decisión | [docs/adr/](docs/adr/) |
| Ver qué falta | [DECISIONES.md](DECISIONES.md), sección 7 |
| Tocar el compilador | `compilador/fuente/generador/generador_c.cpp` es el corazón; `semantico/verificaciones.cpp` las reglas |
| Añadir a la biblioteca | `fuente_biblioteca` en `conductor/compilar.cpp`, y un caso en `pruebas/conformidad/valido/` |

## Lo que más falta

En orden de esfuerzo, y todas bien acotadas:

1. **Casos para los diez códigos de integración** (A-12): CLI, carga de módulos, backend.
2. **Biblioteca estándar**: `archivo`, `texto` avanzado, aritmética de fechas.
3. **Paquetes con manifiesto**.
4. **Tiempos de vida completos**.

## Código de conducta

Se espera respeto y crítica técnica. Los desacuerdos se resuelven con argumentos y con pruebas, no
con autoridad: la mitad de los arreglos de este proyecto empezaron con un caso de conformidad que
fallaba.
