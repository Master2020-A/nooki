# ADR-014 — Backend de la v0.1: C como lenguaje intermedio

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S06 |
| **Ámbito** | Generación de código · dependencias · portabilidad |
| **Origen** | En la máquina de desarrollo no hay LLVM ni clang; MinGW y MSVC no pueden enlazar las librerías de LLVM entre sí |

## Contexto

El plan original fijaba LLVM como backend (ADR-010 y `Aurora2.txt`: «¿Por qué LLVM? Optimizaciones de clase mundial…»). Al llegar al Sprint 6 aparecieron tres hechos:

1. **No hay LLVM instalado** en la máquina de desarrollo: ni `llvm-config`, ni `clang`, ni `LLVMConfig.cmake`. El paquete de Visual Studio solo trae `clang-format` y `clang-tidy`.
2. **No hay `winget`** para instalarlo, y una descarga manual de ~500 MB en un proyecto de una sola persona es una dependencia frágil.
3. **EL problema real: ABI.** LLVM para Windows se distribuye compilado con MSVC. El proyecto ya se compila con **dos** cadenas (MSVC y MinGW GCC). Enlazar `libLLVM` desde MinGW es imposible (ABI de C++ incompatible) y desde MSVC obliga a fijar la versión exacta y a renunciar a MinGW.

Un backend que solo funciona con una de las dos cadenas de compilación no es aceptable: el CI compila con las dos.

## Decisión

1. **La v0.1 genera C y delega la compilación final en un compilador de C del sistema** (`cc`, `gcc`, `clang` o `cl`, en ese orden).
2. **El generador es una pieza aislada detrás de una interfaz**: `GeneradorC` produce texto; un futuro `GeneradorLLVM` producirá IR. El AST y el futuro HIR no saben qué backend los consume.
3. **El binario final es nativo** (PE/ELF/Mach-O), producido por el compilador de C. La promesa de «lenguaje compilado» se cumple.
4. **`--emitir-c`** muestra el C generado: es documentación viva y la principal herramienta de depuración del backend.
5. **LLVM sigue siendo el objetivo** para la v0.5+ (optimizaciones, objetivos múltiples, información de depuración fina). La migración no toca ni el léxico, ni el parser, ni la semántica: solo el generador.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Emitir IR textual de LLVM e invocar `clang` | Requiere `clang` instalado; hoy no lo hay y no se puede verificar la demo |
| Enlazar `libLLVM` desde MSVC | Rompe MinGW, ata el proyecto a una versión exacta y añade ~500 MB al entorno |
| Escribir un backend propio de máquina (COFF/ELF/Mach-O) | Meses de trabajo antes de imprimir «hola» |
| Diferir la rebanada vertical hasta tener LLVM | Bloquea el hito que decide el proyecto: el primer binario |

## Consecuencias

**Positivas**
- La rebanada vertical es verificable **hoy**, en las dos cadenas de Windows y en CI.
- El compilador de C hace por nosotros registro, enlazado, bibliotecas estándar y, más adelante, información de depuración.
- No hay dependencia de una versión concreta de LLVM: el proyecto compila con lo que ya tiene cualquier desarrollador.
- El C generado es legible, lo que acelera la depuración de errores de codegen.

**Costes**
- **Dependencia en tiempo de compilación**: el usuario necesita un compilador de C. En la práctica, todo el que compila C++20 ya lo tiene.
- **Menos control**: no hay intrínsecos propios ni control fino de ABI hasta que llegue LLVM.
- **Rendimiento**: heredamos el del compilador de C, que es bueno, pero perdemos optimizaciones específicas del lenguaje.
- El C intermedio es visible: un usuario puede editarlo, y eso crea expectativas de compatibilidad. Se documenta como artefacto, no como API.

## Impacto en la implementación

- S06: `GeneradorC` + conductor que invoca el compilador de C; `nooki compilar` y `nooki ejecutar`.
- S29–S33: el HIR y la verificación semántica se añaden **por delante** del generador, sin cambios en su interfaz.
- S34: se reevalúa LLVM con la experiencia de un año de uso; si se adopta, `GeneradorLLVM` convive con `GeneradorC` para depurar diferencias.
- S94: los instaladores deberán advertir de la dependencia del compilador de C, o incluir uno.
