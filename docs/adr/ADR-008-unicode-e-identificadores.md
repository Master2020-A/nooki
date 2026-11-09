# ADR-008 — Unicode, tildes y normalización de identificadores

| Campo | Valor |
|---|---|
| **Estado** | Aprobada |
| **Fecha** | 2026-11-09 |
| **Sprint** | S01 (decisión) · S09 (implementación) |
| **Ámbito** | Léxico · portabilidad · accesibilidad |
| **Origen** | Los planes usan `Tamaño`, `señales.oki`, `función` sin definir normalización ni portabilidad |

## Contexto

Es un lenguaje **en español**: las tildes y la `ñ` no son un caso raro, son el caso normal. Pero Unicode en identificadores trae problemas concretos y bien documentados:

- **Normalización:** macOS usa NFD (letra + acento combinado) y Linux/Windows NFC. Sin normalizar, `año` escrito en un Mac y en un Linux son **dos identificadores distintos**.
- **Palabras reservadas acentuadas:** `sí` (afirmación) vs `si` (condicional); `función` vs `funcion`; `está` vs `esta`.
- **Teclados:** en distribución US (la más común entre programadores) escribir `ñ` o `á` requiere combinaciones.
- **Homóglifos:** la `а` cirílica y la `a` latina son distintas; sin control, se pueden escribir identificadores visualmente idénticos.
- **Rutas de archivo:** nombres con acentos rompen herramientas, archivos ZIP y algunos sistemas de control de versiones.

## Decisión

1. **Palabras reservadas 100 % ASCII, sin tildes:** `funcion`, `tamano`, `mas`… Las palabras que en español llevan tilde se escriben sin ella.
2. **Si el usuario escribe una palabra reservada con tilde** (`función`, `tamaño`, `sí`), el compilador produce un error específico con sugerencia:
   `error[NOK1001]: «función» no es una palabra reservada; escriba «funcion» sin tilde`.
3. **Identificadores:** permiten `a-z A-Z 0-9 _`, más `á é í ó ú ü ñ` (mayúsculas incluidas). **Se normalizan a NFC** antes de comparar. No se permiten caracteres de control, espacios ni signos.
4. **Las palabras reservadas también se normalizan a NFC** antes de compararse, de modo que el error de la regla 2 sea determinista.
5. **Columna del compilador:** se cuenta en **puntos de código** (no bytes, no grafemas). La línea se cuenta en saltos de línea `\n`, `\r\n` y `\r`.
6. **Sensibilidad a mayúsculas:** los identificadores **distinguen mayúsculas** (`valor` ≠ `Valor`). La convención de estilo (recomendada por el formateador) es `serpiente_para_todo`.
7. **Homóglifos:** el lexer avisa (`NOK1002`) si un identificador mezcla alfabetos latino y no latino.
8. **Rutas de archivo del proyecto:** ASCII obligatorio; la herramienta `nooki` rechaza rutas con caracteres no ASCII en el árbol fuente y sugiere renombrar.
9. **El código fuente debe ser UTF-8 válido.** Se admite BOM (se ignora). Cualquier byte inválido produce error con la línea.

## Alternativas consideradas

| Alternativa | Por qué se descarta |
|---|---|
| Solo ASCII en todo (identificadores incluidos) | Renuncia a la `ñ`, que es la letra más representativa del español |
| Palabras reservadas con tilde (`función`, `mientras`) | Roza el objetivo, pero multiplica los errores en teclados US y complica el léxico sin aportar nada |
| Sin normalización | El mismo programa no compila igual en macOS y en Linux: inaceptable |
| Normalizar a NFD | Igual de válido técnicamente, pero NFC es lo que usan por defecto Windows, Linux y la web |
| Contar columnas en bytes | Los mensajes de error apuntan a la columna equivocada en cualquier línea con acentos |

## Consecuencias

**Positivas**
- Los programas son idénticos en Windows, macOS y Linux.
- `nombre_de_año` es válido y se ve bien.
- Los mensajes de error apuntan exactamente al carácter correcto.

**Costes**
- Escribir `funcion` sin tilde parece "mal escrito" al principio. **Mitigación:** el error sugiere la forma correcta y la documentación lo explica desde el tutorial 1.
- La normalización añade un paso en el lexer (coste despreciable: una pasada por identificador, con caché).
- `serpiente_para_todo` como convención choca con quien espera `PascalCase` para tipos; el formateador lo impone y el linter lo avisa.

## Impacto en la implementación

- S09: decodificador UTF-8 propio + normalizador NFC (tabla de descomposición incluida o dependencia de ICU/utf8proc; **decisión: tabla propia generada**, para no añadir ICU al compilador).
- S09: `string_interner` guarda el identificador ya normalizado; la comparación es por puntero.
- S10: cálculo de columna en puntos de código; los tests deben cubrir líneas con `ñ` y diacríticos combinados.
