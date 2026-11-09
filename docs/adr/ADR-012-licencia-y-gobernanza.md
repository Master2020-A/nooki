# ADR-012 — Licencia, gobernanza y versionado

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 |
| **Ámbito** | Proyecto · legal · comunidad |
| **Origen** | `Nooki2` menciona `licencia = "MIT"` en un ejemplo de TOML, nada más |

## Contexto

Un lenguaje necesita reglas de juego antes de tener usuarios: quién decide, cómo se cambia el lenguaje sin romper el mundo y con qué licencia se puede adoptar en empresas. Los planes no dicen nada, y la licencia aparece de refilón en un archivo de configuración de ejemplo.

## Decisión

**Licencia**
1. **Licencia dual `MIT OR Apache-2.0`** para el compilador, las herramientas y la librería estándar.
2. **Excepción de runtime** (como la de LLVM o Rust): el código generado por Nooki y las bibliotecas enlazadas **no** heredan la licencia del compilador. Esto es imprescindible para que una empresa pueda usar Nooki en software propietario.
3. La documentación se publica bajo `CC BY-SA 4.0`; los ejemplos, bajo `MIT`.

**Versionado**
4. **Semver** para el compilador (`MAYOR.MENOR.PARCHE`) y para la librería estándar.
5. **Ediciones** (como las de Rust): cada ~3 años, una edición puede introducir sintaxis nueva sin romper el código existente. El archivo de proyecto declara la edición (`edicion = "2026"`).
6. **Política de compatibilidad:** a partir de v1.0, ninguna versión menor rompe código compilado previamente. Las deprecaciones duran al menos dos versiones menores y son advertidas.
7. **`version_minima` (MSRV)** declarada por cada paquete en su manifiesto.

**Gobernanza**
8. **RFC público** en el repositorio para cualquier cambio de lenguaje o de la stdlib. Plantilla obligatoria: motivación, guía, referencia, desventajas, alternativas, impacto en compatibilidad.
9. **Código de conducta** (Contributor Covenant) y **DCO** (no CLA) para contribuciones.
10. **Decisiones arquitectónicas** en `docs/adr/`, numeradas y **append-only**: un ADR no se edita, se reemplaza con otro que lo cita.
11. **Etiquetas de contribución:** `bueno-para-empezar`, `necesita-diseño`, `bloqueado`, `documentación`.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Solo MIT | Sin cláusula de patentes; algunas empresas lo prefieren con Apache-2.0 |
| Solo Apache-2.0 | Incompatible con proyectos GPLv2, que es una parte importante del ecosistema C |
| GPL | Impide la adopción comercial y la integración con bibliotecas propietarias: mata el ecosistema |
| Dominio público | Sin protección de patentes ni garantías |
| Sin proceso de RFC (decide el autor) | Genera rupturas arbitrarias; los lenguajes que sobreviven tienen proceso |

## Consecuencias

**Positivas**
- Adopción empresarial viable gracias a `MIT OR Apache-2.0` + excepción de runtime.
- El sistema de ediciones permite corregir errores de diseño sin fragmentar el ecosistema.
- Los ADR como registro histórico evitan repetir discusiones.

**Costes**
- Dos licencias y una excepción de runtime exigen atención legal al empaquetar (S94).
- El proceso de RFC añade latencia a los cambios. **Mitigación:** los cambios internos del compilador no requieren RFC, solo los del lenguaje y la stdlib.
- El DCO no protege tanto como un CLA. **Aceptado:** el CLA es una barrera de entrada grande para un proyecto comunitario.

## Impacto en la implementación

- S03: archivos `LICENSE-MIT`, `LICENSE-APACHE`, `LICENCIA-RUNTIME.md`, `CODIGO_DE_CONDUCTA.md`, `CONTRIBUIR.md`, `docs/rfcs/0000-plantilla.md`.
- S55: el registro de paquetes guarda la licencia y la versión mínima de cada paquete.
- S94: revisión legal de la excepción de runtime antes de publicar instaladores.
