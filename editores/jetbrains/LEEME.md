# Nooki para JetBrains

Resaltado de sintaxis para **RustRover**, IntelliJ IDEA, CLion, PyCharm, WebStorm, GoLand y el resto de
IDEs de JetBrains.

Hay tres caminos. Cuál usar depende de una cosa que comprobé instalando RustRover 2026.1 y mirando
dentro: **RustRover no trae el soporte de TextMate** (`plugins/textmate` no existe en su distribución),
así que la página *Settings → Editor → TextMate Bundles* **no aparece** y el bundle no sirve ahí.

| Tu IDE | Camino |
|---|---|
| **RustRover** | **A** — el plugin `.jar` |
| IntelliJ IDEA, CLion, PyCharm… (traen TextMate) | B — el bundle |
| Cualquiera, sin instalar nada | C — tipos de archivo |

---

## Camino A — El plugin (para RustRover)

`Nooki-1.0.0.jar` es un plugin de verdad: lexer propio, colores, comentarios con Ctrl+/ y emparejado de
llaves.

1. **Settings** (Ctrl+Alt+S) → **Plugins**.
2. Pulsa el engranaje ⚙ → **Install Plugin from Disk…**
3. Elige `Nooki-1.0.0.jar` de este directorio.
4. **Apply** y reinicia el IDE cuando lo pida.
5. Abre cualquier `.oki`: ya sale coloreado.

Comprobado sobre **RustRover 2026.1** (build 261.22158.331): el registro del IDE dice
`Loaded custom plugins: Nooki (1.0.0)` y el lexer clasifica un programa de prueba con 59 tokens y
**cero desconocidos**.

### Si el IDE lo rechaza

El plugin está compilado para **Java 21 o más nuevo**, que es lo que usan los IDEs desde 2024. En
RustRover 2024.3 y anteriores (Java 17) hay que recompilarlo contra ese IDE:

```powershell
cd editores\jetbrains\plugin
.\compilar.ps1 -Ide "C:\Program Files\JetBrains\RustRover 2024.3"
```

Y volver a instalarlo.

### Recompilar el plugin

No hace falta Gradle ni descargar nada: se compila contra las bibliotecas del IDE que ya tienes.

```powershell
cd editores\jetbrains\plugin
.\compilar.ps1                    # elige RustRover y el JDK más nuevos que encuentre
.\probar-carga.ps1                # arranca un IDE aislado y comprueba que carga
```

`probar-carga.ps1` usa una configuración temporal, así que **no toca tu instalación**: arranca, lee el
registro, dice si cargó y cierra.

---

## Camino B — TextMate Bundles (IntelliJ, CLion, PyCharm…)

Estos IDEs traen el soporte de TextMate de fábrica; no hay que instalar ningún plugin de la tienda.

1. **Settings** → **Editor → TextMate Bundles**.
2. Pulsa **+** y elige la **carpeta** `Nooki.tmbundle` de este directorio.
3. **OK** y abre un `.oki`.

Si la página no aparece, el plugin **TextMate** está desactivado: *Settings → Plugins → Installed*,
búscalo y actívalo.

---

## Camino C — Tipos de archivo (sin instalar nada)

Funciona en todos los IDEs, pero solo da un color a las palabras clave; no distingue tipos, textos ni
comentarios.

1. **Settings → Editor → File Types** → **+**.
2. Nombre **Nooki**, descripción «Lenguaje Nooki».
3. **File name patterns**: `*.oki`
4. **Keywords**: pega esta lista:

```text
funcion variable constante clase estructura rasgo interfaz enumeracion tipo modulo
importar exportar implementar externo usar
si sino mientras para en hacer romper continuar retornar coincide caso defecto
nuevo este Este super hereda implementa
publico privado protegido interna
inseguro asm
verdadero falso nulo
entero flotante booleano texto caracter vacio lista tamano esperar
Vector Mapa Caja resultado
```

5. **OK**.

---

## Errores subrayados y autocompletado

El color es solo color. Para **diagnósticos**, **autocompletado**, **ir a la definición** y
**formato**, conecta el servidor de lenguaje del compilador:

1. **Settings → Plugins → Marketplace** → instala **LSP4IJ** y reinicia.
2. **Settings → Languages & Frameworks → LSP4IJ** → **+** → *User-defined*.
3. Rellena:
   - **Name**: `Nooki`
   - **Command**: `nooki lsp` (o la ruta completa si no está en el `PATH`)
   - **File patterns**: `*.oki`
   - **Transport**: `stdio`
4. Abre un `.oki`.

Con eso, un `NOK5001` aparece subrayado en su línea mientras escribes, con la explicación en español.

---

## Qué resalta

| Elemento | Ejemplo |
|---|---|
| Declaraciones | `funcion`, `clase`, `rasgo`, `variable`, `constante`, `externo` |
| Control | `si`, `sino`, `mientras`, `para`, `coincide`, `caso`, `romper` |
| Tipos | `entero`, `flotante`, `booleano`, `texto`, `caracter`, `vacio` |
| Genéricos | `Vector`, `Mapa`, `Caja`, `resultado` |
| Objeto | `nuevo`, `este`, `super`, `hereda`, `implementa` |
| Meta | `usar`, `inseguro`, `asm` |
| Literales | `verdadero`, `falso`, `nulo` |
| Textos | `"..."`, `"""..."""` de varias líneas, escapes |
| Números | enteros, con guiones bajos, hexadecimales y flotantes |
| Comentarios | `//` de línea y `/* */` de bloque |
| Operadores | `**`, `..=`, `->`, `&&`, `\|\|`, y el `?` de propagación |
| Referencias | `&`, `&mutable`, `*` de desreferencia |

Además, con el plugin: **Ctrl+/** comenta la línea, y las llaves, paréntesis y corchetes se emparejan.

## Contenido de esta carpeta

| Archivo | Qué es |
|---|---|
| `Nooki-1.0.0.jar` | El plugin, listo para instalar (RustRover y cualquier IDE con Java 21+) |
| `plugin/` | Su código fuente y los dos scripts: `compilar.ps1` y `probar-carga.ps1` |
| `Nooki.tmbundle/` | El bundle TextMate, para los IDEs que lo soportan |
| `Nooki.tmbundle.zip` | Lo mismo, para moverlo a otra máquina |

El bundle se genera desde una definición única que también escribe la gramática de VS Code, para que
los editores no se desincronicen:

```bash
python herramientas/generar-gramaticas.py
```
