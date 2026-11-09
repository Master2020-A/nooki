# ADR-011 — Ensamblador en línea e intrínsecos

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 (decisión) · S40 (implementación) |
| **Ámbito** | Lenguaje · generación de código · hardware |
| **Origen** | Ninguno de los dos planes menciona ensamblador: sin él, "controlar el hardware como C" es falso |

## Contexto

El plan promete "control de hardware como C y C++" y "acceso a registros de memoria" (`*registro = 0x01`). Pero eso, sin ensamblador ni intrínsecos, solo llega hasta donde llega el compilador:

- no se pueden escribir rutinas de arranque (*boot*), manejadores de interrupción ni *context switches*;
- no hay barreras de memoria ni instrucciones de sistema (`cli`, `sti`, `wfi`, `syscall`);
- no se puede acceder a registros de CPU (`cr3`, `msr`) ni usar instrucciones específicas (`rdtsc`, `cpuid`, SIMD concreto).

C y C++ sí pueden. Si Nooki no, la promesa es publicidad, no ingeniería.

## Decisión

1. **`asm!` existe desde la v0.5** y solo dentro de `inseguro { }`:
   ```nooki
   inseguro {
       asm!("hlt")
       asm!("mov {0}, {1}", salida(reg) destino, entrada(reg) valor)
       asm!("cpuid", salida("eax") a, salida("ebx") b, salida("ecx") c, salida("edx") d)
   }
   ```
2. **Sintaxis de ensamblador: Intel** por defecto (`opciones(intel)` implícita); se admite ATT con `opciones(att)`.
3. **Operandos nombrados obligatorios** (estilo Rust, no `%0`/`%1`): el objetivo es que el error de compilación sea legible en español.
4. **`opciones(...)`** admite: `intel`, `att`, `volatile` (implícita si hay efectos), `barrera_memoria`, `no_retorno`, `desnudo` (función sin prólogo ni epílogo).
5. **Intrínsecos** disponibles sin `inseguro`, en `core.intrinsecos`: `barrera_memoria()`, `volatil_leer/escribir`, `cpuid()`, `rdtsc()`, `contador_bits`, SIMD básico (`suma_vectorial`, `multiplicacion_vectorial`).
6. **Funciones desnudas:** `@desnudo` permite escribir manejadores de interrupción y puntos de entrada de arranque, sin prólogo ni epílogo generados.
7. **`inseguro` no se propaga:** cada bloque `inseguro` es explícito; una función no puede ser "entera insegura" salvo con `@insegura`, y el linter lo cuenta.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| No ofrecer ensamblador y delegar en `externo "C"` | Obliga a tener un compilador de C en la cadena, y aun así hay cosas imposibles (manejadores de interrupción) |
| Solo intrínsecos, sin `asm!` | Cubre el 80 %, pero deja fuera el arranque y las instrucciones sin intrínseco |
| `asm!` con `%0`, `%1` (estilo GCC) | Ilegible en los errores; contradice el objetivo "todo en español" |
| `asm!` disponible en código seguro | Rompería la garantía: el compilador no puede verificar lo que hace el ensamblador |
| Bloques `inseguro` implícitos por función | Difumina la auditoría de seguridad, que es un pilar |

## Consecuencias

**Positivas**
- El pilar de hardware es real: arranque, interrupciones, registros y SIMD.
- Los errores de ensamblador (operando mal escrito, registro inválido) se detectan al compilar, no en tiempo de ejecución.
- La auditoría de seguridad tiene una frontera nítida: `inseguro` cuenta y se reporta.

**Costes**
- Superficie de lenguaje y de documentación grande (cada arquitectura tiene sus registros).
- Riesgo de que la librería estándar se llene de `inseguro`. **Mitigación:** regla 7 del plan (lista blanca revisada) y el linter cuenta los bloques por módulo.
- El optimizador de LLVM debe respetar `volatile` y barreras; hay que probarlo (S40).

## Impacto en la implementación

- S40: parser de `asm!` como forma sintáctica especial (no como macro general), validación de operandos contra una tabla de registros por arquitectura, emisión de `llvm.asm`/`llvm.sideeffect`.
- S43: demo de *bare metal* que usa `asm!` para arrancar y para `wfi`.
- S91: la auditoría de seguridad revisa cada bloque `inseguro` de la stdlib.
