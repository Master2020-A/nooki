# Guía del administrador de Nooki

**Instalar, configurar, operar y mantener Nooki en un equipo, un aula o una organización.**

| | |
|---|---|
| Versión documentada | **Nooki 1.0.0** (edición 2026) |
| Ámbito | compilador `nooki`, proyectos `.oki`, backend de C, editores y CI |
| Audiencia | administradores de sistemas, responsables de equipo, docentes y soporte |
| Documento hermano | [Guía del desarrollador](GUIA_DEL_DESARROLLADOR.md) |

> Esta guía describe el comportamiento real de la versión 1.0.0. Cuando una operación tiene
> un efecto destructivo o una limitación, se indica de forma explícita. Las rutas de ejemplo
> usan `C:\...` y `./`; adapta los separadores a tu sistema.

---

## Índice

1. [Qué administra Nooki](#1-qué-administra-nooki)
2. [Matriz de requisitos](#2-matriz-de-requisitos)
3. [Instalación](#3-instalación)
4. [Configuración del entorno](#4-configuración-del-entorno)
5. [Operación: comandos y efectos](#5-operación-comandos-y-efectos)
6. [Proyectos, manifiesto y dependencias](#6-proyectos-manifiesto-y-dependencias)
7. [Pruebas, formato y documentación](#7-pruebas-formato-y-documentación)
8. [Editores y servidor de lenguaje](#8-editores-y-servidor-de-lenguaje)
9. [Integración continua](#9-integración-continua)
10. [Seguridad](#10-seguridad)
11. [Solución de problemas](#11-solución-de-problemas)
12. [Mantenimiento y ciclo de vida](#12-mantenimiento-y-ciclo-de-vida)
13. [Listas de verificación](#13-listas-de-verificación)

---

## 1. Qué administra Nooki

Nooki es un compilador de línea de comandos. No tiene servidor, base de datos ni servicio de
red. Administrar Nooki consiste en:

- mantener el binario `nooki` y un compilador de C disponibles;
- organizar proyectos con `nooki.toml`, `biblioteca/` y `pruebas/`;
- integrar el compilador y el servidor de lenguaje en los editores;
- ejecutar la compilación y las pruebas en integración continua;
- controlar el riesgo de ejecutar código nativo de terceros.

El compilador **no descarga nada de internet**. Las dependencias son carpetas locales
declaradas en `nooki.toml`. El binario invoca al compilador de C del sistema y ejecuta el
programa solo cuando se usa `nooki ejecutar` (o cuando el usuario lanza el binario
compilado).

### Piezas que conviene conocer

| Pieza | Qué es |
|---|---|
| `nooki` | el compilador y sus herramientas |
| `nooki lsp` | servidor de lenguaje por entrada/salida estándar |
| `nooki.toml` | manifiesto del proyecto |
| `.oki` | fuente Nooki |
| `.oki.esperado` | salida esperada de una prueba |
| `biblioteca/` | módulos propios del proyecto |
| `pruebas/` | programas de prueba del proyecto |
| `API.md` | documentación generada con `nooki documentar` |
| `build/`, `artefactos/` | artefactos que borra `nooki limpiar` |
| `dist/` | distribuciones generadas por los scripts de empaquetado |

---

## 2. Matriz de requisitos

### Para usar el compilador

| Requisito | Windows | Linux | macOS |
|---|---|---|---|
| Binario `nooki` | sí | sí | sí |
| Compilador de C | MinGW `cc`/`gcc`, o Clang | `gcc`/`cc` o Clang | `clang` (Xcode Command Line Tools) |
| Backend de ventanas | GDI (`user32`, `gdi32`) | X11 (`libx11-dev`) | no hay backend Cocoa; los stubs devuelven 0 |
| CMake para compilar Nooki | 3.25+ | 3.25+ | 3.25+ |
| Compilador de C++ para Nooki | MSVC 2022+ o MinGW | GCC 12+ o Clang 15+ | Clang 15+ |

> **Importante.** Compilar el compilador con MSVC no significa que el backend sea MSVC. En
> tiempo de ejecución, `nooki` busca un compilador de C en este orden: `NOOKI_CC`, `cc`,
> `gcc`, `clang`. No sondea `cl.exe`, y `cl` no entiende las opciones que Nooki pasa
> (`--version`, `-O2`, `-o`, `-lX11`/`-luser32`). En Windows, instala MinGW o Clang.

### Para programas con ventana

| Sistema | Compilar | Ejecutar |
|---|---|---|
| Windows | nada adicional | nada adicional |
| Linux | `sudo apt install libx11-dev` | X11 o `xvfb-run` |
| macOS | no hay backend Cocoa; el enlace con UI no está validado | `ventana.abrir` devuelve `0` si el binario se enlaza |

### Para el servidor de lenguaje

Cualquier editor que hable LSP y sepa lanzar un proceso por stdio. No hace falta un runtime
adicional.

---

## 3. Instalación

### 3.1 Desde una distribución

La distribución contiene el binario, documentación, ejemplos, editores, especificación y un
`INSTALAR.txt`. No contiene el código fuente del compilador.

#### Windows

```powershell
# Descomprime el zip y entra en la carpeta
cd nooki-1.0.0-windows-x86_64
.\herramientas\instalar.ps1
```

El script copia `nooki.exe` a `%LOCALAPPDATA%\Nooki\bin` y añade esa carpeta al `PATH` del
usuario. No toca el `PATH` del sistema ni pide permisos de administrador. Cierra y vuelve a
abrir la terminal.

Para comprobar:

```powershell
nooki version
nooki ejecutar ejemplos\01-hola\principal.oki
```

Si no quieres usar el script, añade la carpeta de la distribución al `PATH` manualmente.

#### Linux y macOS

```bash
tar -xzf nooki-1.0.0-linux-x86_64.tar.gz
cd nooki-1.0.0-linux-x86_64
export PATH="$PWD:$PATH"
nooki version
```

Para dejarlo permanente:

```bash
echo 'export PATH="'"$PWD"'":$PATH"' >> ~/.bashrc
source ~/.bashrc
```

En macOS, sustituye `~/.bashrc` por `~/.zshrc` si usas Zsh.

#### Verificación de la distribución

```bash
cd dist
unzip nooki-1.0.0-windows-x86_64.zip
./nooki-1.0.0-windows-x86_64/nooki.exe version
./nooki-1.0.0-windows-x86_64/nooki.exe ejecutar \
  nooki-1.0.0-windows-x86_64/ejemplos/01-hola/principal.oki
```

Debe imprimir `¡Hola, Nooki!`.

### 3.2 Desde el código fuente

Requisitos: CMake 3.25 o superior y un compilador de C++20.

```bash
git clone <repositorio>
cd nooki
cmake --preset windows-mingw      # o linux, macos, windows-msvc, windows-msvc-2026
cmake --build --preset windows-mingw
ctest --preset windows-mingw
```

Presets disponibles:

| Preset | Sistema | Generador | Binario |
|---|---|---|---|
| `windows-msvc` | Windows | Visual Studio 17 2022 | `build/windows-msvc/compilador/Release/nooki.exe` |
| `windows-msvc-2026` | Windows | Visual Studio 18 2026 | `build/windows-msvc-2026/compilador/Release/nooki.exe` |
| `windows-mingw` | Windows | MinGW Makefiles | `build/windows-mingw/compilador/nooki.exe` |
| `windows-mingw-depurar` | Windows | MinGW Makefiles (Debug) | `build/windows-mingw-depurar/compilador/nooki.exe` |
| `linux` | Linux | Unix Makefiles | `build/linux/compilador/nooki` |
| `linux-depurar` | Linux | Unix Makefiles (Debug) | `build/linux-depurar/compilador/nooki` |
| `macos` | macOS | Unix Makefiles | `build/macos/compilador/nooki` |
| `macos-depurar` | macOS | Unix Makefiles (Debug) | `build/macos-depurar/compilador/nooki` |
| `saneamiento` | Linux/macOS | Unix Makefiles + ASan/UBSan | `build/saneamiento/compilador/nooki` |

Opciones de CMake útiles:

| Opción | Por defecto | Qué hace |
|---|---|---|
| `NOOKI_PRUEBAS` | `ON` | construye y registra las pruebas |
| `NOOKI_SANEAMIENTO` | `OFF` | activa AddressSanitizer y UBSan (no MSVC) |
| `NOOKI_AVISOS_COMO_ERRORES` | `OFF` | convierte avisos en errores |
| `NOOKI_EDICION` | `2026` | edición del lenguaje que anuncia el binario |

Para instalar el binario compilado en Windows:

```powershell
.\herramientas\instalar.ps1 -Compilar
```

Para desinstalarlo:

```powershell
.\herramientas\instalar.ps1 -Desinstalar
```

### 3.3 Actualizar y desinstalar

| Tarea | Windows | Linux/macOS |
|---|---|---|
| Actualizar | reemplaza `%LOCALAPPDATA%\Nooki\bin\nooki.exe` | reemplaza el binario del `PATH` |
| Desinstalar | `.\herramientas\instalar.ps1 -Desinstalar` | quita la carpeta del `PATH` y bórrala |
| Conservar proyectos | los proyectos no viven en la carpeta del compilador | igual |

Antes de actualizar, ejecuta `nooki version` y guarda la versión. Los proyectos no tienen
archivo de bloqueo: la actualización puede cambiar el comportamiento del compilador.

---

## 4. Configuración del entorno

### 4.1 Variables de entorno

| Variable | Efecto | Valores |
|---|---|---|
| `PATH` | localiza `nooki`, `cc`, `gcc`, `clang` | rutas del sistema |
| `NOOKI_CC` | fuerza el compilador de C | ruta o nombre de un ejecutable que acepte `--version` |
| `NO_COLOR` | desactiva el color de los diagnósticos | cualquier valor no vacío |
| `TMPDIR` / `TEMP` / `TMP` | carpeta de archivos intermedios | ruta escribible |

`NOOKI_CC` solo se usa si el candidato responde a `--version`; si no, Nooki prueba `cc`,
`gcc` y `clang`. Si ninguno responde, el diagnóstico es `NOK6001`.

Ejemplo con un wrapper que añade bibliotecas:

```bash
# guarda como /usr/local/bin/nooki-cc y dale permiso de ejecución
#!/bin/sh
exec cc "$@" -lm
```

```bash
export NOOKI_CC=/usr/local/bin/nooki-cc
```

El wrapper debe aceptar las opciones que Nooki pasa: `-O2 -o <salida> <fuentes.c>` y, según
el caso, `-luser32 -lgdi32` (Windows) o `-lX11` (Linux).

### 4.2 Codificación y rutas

- Los fuentes deben ser **UTF-8 sin BOM**. Un BOM produce `NOK1004`.
- En PowerShell 5, `Set-Content -Encoding utf8` añade BOM. Usa PowerShell 7 con
  `-Encoding utf8NoBOM`, o guarda desde VS Code con “UTF-8”.
- Los identificadores pueden llevar tildes y `ñ`; las palabras clave no.
- Nooki maneja rutas con tildes en la mayoría de los casos (enlaza en una ruta ASCII y mueve
  el binario), pero algunas herramientas del sistema y enlazadores son más frágiles con
  Unicode. Para equipos y CI, prefiere rutas ASCII.
- Los flotantes se imprimen con **coma decimal** y hasta seis decimales. No dependas del
  locale para cambiar eso: es el formato del runtime.

### 4.3 Archivos intermedios

Nooki genera C en la carpeta temporal del sistema:

| Archivo | Cuándo |
|---|---|
| `nooki_<stem>.c` | siempre que se compila |
| `nooki_ia_<stem>.c` | si el programa usa `tensor`, `tabla`, `autograd` o `redes` |
| `nooki_ui_<stem>.c` | si el programa usa `ventana` o `imagenes` |
| `nooki_salida_<stem>.exe` | si la ruta de salida tiene caracteres no ASCII (Windows) |

`--conservar` conserva el `.c` principal y dice dónde está. Los archivos de UI/IA no se
borran automáticamente. Si compilas mucho, limpia la carpeta temporal o usa un `TMPDIR`
dedicado.

**Colisiones en paralelo.** El nombre del temporal depende solo del nombre del fuente
(`<stem>`). Dos compilaciones simultáneas de archivos con el mismo nombre (`a/principal.oki`
y `b/principal.oki`) pueden pisarse. En CI, usa un `TMPDIR` por trabajo o serializa esos
casos.

---

## 5. Operación: comandos y efectos

### 5.1 Tabla de comandos

| Comando | Sintaxis | Efecto |
|---|---|---|
| `ayuda` | `nooki ayuda` | muestra la ayuda; código 0 |
| `version` | `nooki version` | muestra versión, objetivo y edición |
| `compilar` | `nooki compilar <archivo.oki> [-o <salida>] [--emitir-c] [--conservar]` | genera un ejecutable |
| `ejecutar` | `nooki ejecutar <archivo.oki> [-o <salida>] [--conservar]` | compila y ejecuta; devuelve el código del programa |
| `ast` | `nooki ast <archivo.oki>` | imprime el árbol de sintaxis |
| `construir` | `nooki construir [carpeta] [-o <salida>]` | compila el proyecto; entrada `principal.oki` o `fuente/principal.oki` |
| `lsp` | `nooki lsp` | servidor LSP por stdio |
| `nuevo` | `nooki nuevo <nombre>` | crea un proyecto |
| `probar` | `nooki probar` | ejecuta `pruebas/*.oki` y compara con `.esperado` |
| `limpiar` | `nooki limpiar` | borra artefactos (ver aviso) |
| `verificar` | `nooki verificar <archivo.oki>` | léxico, parser y semántica; no genera C |
| `formatear` | `nooki formatear <archivo.oki>... [--comprobar]` | normaliza sangría y espacios finales |
| `documentar` | `nooki documentar [carpeta\|archivo] [--salida]` | genera `API.md` |

Notas de sintaxis:

- En `construir`, el primer argumento es la carpeta. Para pasar opciones, escribe primero la
  carpeta: `nooki construir . -o dist/app`. `nooki construir -o dist/app` interpreta `-o`
  como carpeta y falla.
- `--emitir-c` imprime el C generado por la salida estándar y **no** compila. No crea
  binario.
- `--conservar` deja el `.c` intermedio y lo anuncia por la salida de error.
- `nooki ejecutar` acepta las mismas opciones de compilación; si usas `--emitir-c`, no llega
  a ejecutar.

### 5.2 Códigos de salida

Códigos documentados en `nooki ayuda`:

| Código | Significado |
|---|---|
| `0` | éxito |
| `64` | uso incorrecto (argumentos, opciones) |
| `65` | error en los datos de entrada (código Nooki con errores) |
| `69` | funcionalidad no disponible (reservado; hoy todos los comandos están disponibles) |
| `70` | error interno del compilador o pánico en ejecución |

Matices reales de la 1.0.0:

| Comando | Comportamiento |
|---|---|
| `compilar`, `construir`, `verificar` | `65` si hay errores de compilación |
| `formatear --comprobar` | `65` si algún archivo necesita cambios |
| `documentar` | `65` si no hay nada público que documentar |
| `nuevo` | `2` si falta el nombre o ya existe; `1` si falla la creación |
| `probar` | `2` si no hay `pruebas/` o no hay `.oki`; `1` si alguna prueba falla |
| `limpiar` | `0` siempre que pueda recorrer el árbol |
| `ejecutar` | el código de salida del **programa** si compila bien; `65` si no compila |
| `lsp` | `0` al cerrar la entrada estándar |

Para scripts, no asumas que todo fallo es `1`: distingue `64`, `65` y `70`.

### 5.3 Efectos sobre el sistema de archivos

| Comando | Escribe | Borra |
|---|---|---|
| `compilar` | binario de salida; temporal `.c` | temporal `.c` salvo `--conservar` |
| `ejecutar` | binario temporal o el indicado con `-o` | nada más |
| `construir` | binario del proyecto | temporal `.c` |
| `nuevo` | carpeta, `principal.oki`, `nooki.toml`, `biblioteca/herramientas.oki` | nada |
| `probar` | `.bin` y `.salida` temporales en `pruebas/` | los borra al terminar cada prueba |
| `limpiar` | — | `build/`, `artefactos/`, y **todos** los `.exe`, `.bin`, `.salida` del árbol |
| `formatear` | reescribe los `.oki` indicados | — |
| `documentar` | `API.md` en el directorio actual | sobrescribe el anterior |

> **Aviso sobre `nooki limpiar`.** Recorre el directorio actual de forma recursiva y borra
> cualquier archivo con extensión `.exe`, `.bin` o `.salida`, además de las carpetas `build/`
> y `artefactos/`. No pide confirmación. No lo ejecutes en la raíz de un monorepo con
> binarios ajenos a Nooki. Ejecútalo dentro de la carpeta del proyecto.

### 5.4 Ejemplos de operación

```bash
# Un archivo suelto
nooki compilar hola.oki -o hola
./hola

# Un proyecto
cd agenda
nooki construir
./principal

# Verificar sin generar C
nooki verificar principal.oki

# Ver el C generado, sin compilar
nooki compilar principal.oki --emitir-c > principal.c

# Conservar el intermedio para depurar un fallo del backend
nooki compilar principal.oki --conservar

# Pruebas del proyecto
nooki probar

# Formato en CI
nooki formatear principal.oki biblioteca/*.oki --comprobar

# Documentación
nooki documentar --salida > API.md
```

---

## 6. Proyectos, manifiesto y dependencias

### 6.1 Estructura recomendada

```text
mi-proyecto/
├── nooki.toml
├── principal.oki
├── biblioteca/
│   ├── utilidades.oki
│   └── modelo.oki
├── pruebas/
│   ├── saludo.oki
│   └── saludo.oki.esperado
└── API.md                 (generado)
```

- `principal.oki` es la entrada por defecto. También se acepta
  `fuente/principal.oki`; en ese caso el binario por defecto queda en `fuente/` (por ejemplo,
  `fuente/principal.exe`), no en la raíz. Usa `-o principal` o `-o dist/app` si quieres otra
  ubicación.
- `biblioteca/` contiene módulos del proyecto. Se importan con `usar nombre`.
- `pruebas/` contiene programas completos con `principal`, no módulos.
- `nooki.toml` solo es necesario para declarar dependencias. `nooki construir` funciona sin
  él si hay `principal.oki`; `nooki probar` no lo lee en la 1.0.0. Un archivo suelto se
  compila sin manifiesto.

### 6.2 Manifiesto

```toml
# nooki.toml
[proyecto]
nombre = "agenda"
version = "0.1.0"

[dependencias]
utilidades = { ruta = "../utilidades" }
graficos   = { ruta = "libs/graficos" }
```

Reglas reales del parser:

- Secciones entre corchetes: `[proyecto]`, `[dependencias]`.
- `clave = valor`; `#` inicia un comentario.
- En `[proyecto]`, `nombre` y `version` son texto; las comillas son opcionales.
- En `[dependencias]`, solo se admite `nombre = { ruta = "..." }`; la ruta **debe** ir
  entre comillas y es relativa a la carpeta del manifiesto.
- No hay listas, tablas anidadas, versiones, descarga ni bloqueo.
- Las dependencias no son transitivas: si `utilidades` depende de `comun`, el proyecto debe
  declarar también `comun`.

### 6.3 Resolución de módulos

Para `usar nombre`, el compilador busca en este orden:

1. `nombre.oki` en la carpeta del archivo que hace el `usar`.
2. La raíz y la `biblioteca/` de cada dependencia del manifiesto.
3. `biblioteca/nombre.oki` en la carpeta del archivo y en las carpetas padre, subiendo.
4. La biblioteca estándar embebida en el compilador.

Consecuencias operativas:

- Una dependencia puede tapar un módulo local con el mismo nombre porque se consulta antes
  que la `biblioteca/` local. Usa nombres únicos por proyecto.
- Un archivo llamado `textos.oki` en una carpeta padre puede tapar el módulo estándar
  `textos`. Es la causa del antiguo asunto A-13; revisa los nombres.
- `nooki verificar` y `nooki probar` **no cargan las dependencias del manifiesto** en la
  1.0.0. Solo `nooki construir` aplica las rutas del `nooki.toml`. Si un módulo solo existe
  en una dependencia, `verificar` puede dar `NOK3008`; compila con `construir`.
- `nooki lsp` analiza el documento abierto y no resuelve módulos de otras carpetas.

### 6.4 Visibilidad

- `publico` en una función, estructura, clase o enumeración la expone fuera del módulo.
- Sin `publico`, es privada del módulo.
- `privado`, `protegido` e `interna` se aceptan en campos; en la práctica, solo `publico` es
  visible desde fuera. No diseñes una API basada en `protegido`/`interna` en la 1.0.0.

### 6.5 Pruebas del proyecto

Cada `pruebas/<nombre>.oki` se compila y se ejecuta. Su salida se compara con
`pruebas/<nombre>.oki.esperado`:

```text
pruebas/
├── suma.oki
└── suma.oki.esperado
```

```bash
nooki probar
```

- La comparación normaliza `\r\n` a `\n`, pero no ignora espacios ni líneas en blanco.
- El `.esperado` debe incluir el salto de línea final si el programa lo imprime.
- Los binarios y capturas temporales (`<prueba>.oki.bin`, `<prueba>.oki.salida`) se borran al
  terminar cada prueba.
- Las pruebas se ejecutan con el directorio de trabajo actual; las rutas relativas dentro del
  programa se resuelven desde ahí, no desde `pruebas/`.

---

## 7. Pruebas, formato y documentación

### 7.1 `nooki probar`

| Situación | Resultado |
|---|---|
| Todo pasa | `todas las pruebas pasaron`, código 0 |
| Una prueba falla | `FALLO ...`, código 1 |
| No hay `pruebas/` | error, código 2 |
| No hay `.oki` en `pruebas/` | error, código 2 |

### 7.2 `nooki formatear`

- Normaliza la sangría a **4 espacios** por nivel.
- Quita espacios al final de línea.
- Añade el salto de línea final si falta.
- No toca el contenido de textos, comentarios ni textos multilínea.
- No reordena código, no parte líneas largas y no aplica un estilo completo tipo `gofmt`.

En CI:

```bash
nooki formatear principal.oki biblioteca/*.oki --comprobar
```

`--comprobar` no reescribe; devuelve `65` si algo cambiaría.

> El `principal.oki` que genera `nooki nuevo` puede no tener salto de línea final. Pasa
> `nooki formatear principal.oki` una vez antes de activar `--comprobar` en CI.

### 7.3 `nooki documentar`

- Recorre la carpeta (o archivo) indicada y escribe `API.md` en el directorio actual.
- Incluye solo declaraciones `publico`: funciones, tipos y enumeraciones.
- De los tipos incluye campos; **no incluye métodos de clase**.
- Omite archivos con errores de léxico o sintaxis.
- `--salida` escribe el Markdown por la salida estándar en vez de crear `API.md`.
- Si no hay nada público, devuelve `65`.

### 7.4 `nooki verificar`

- Ejecuta léxico, parser y verificación semántica.
- No genera C ni invoca al compilador de C.
- No aplica las rutas de dependencias del manifiesto (ver §6.3).
- Es útil en pre-commit y en editores; no sustituye a `construir` para una verificación
  completa de enlace.

### 7.5 `nooki ast`

Imprime un árbol S-expression canónico. Ejemplo:

```text
(programa
  (funcion sumar (a entero, b entero) -> entero (retornar (+ a b)))
  (funcion principal -> entero (variable x (llamada sumar 1 2)) (llamada imprimir x) (retornar 0)))
```

Es una herramienta de diagnóstico, no un formato estable para consumo externo.

---

## 8. Editores y servidor de lenguaje

### 8.1 Resaltado de sintaxis

Las definiciones están en `editores/`. Instrucciones completas en
[`editores/LEEME.md`](../../editores/LEEME.md).

| Editor | Instalación |
|---|---|
| VS Code | copia `editores/vscode` a `%USERPROFILE%\.vscode\extensions\nooki-1.0.0` (Windows) o `~/.vscode/extensions/nooki-1.0.0`; reinicia |
| Vim/Neovim | copia `editores/vim/syntax/nooki.vim` y `ftdetect/nooki.vim` a tu configuración |
| Sublime/TextMate/Zed | usa `editores/vscode/syntaxes/nooki.tmLanguage.json` |
| JetBrains | usa la carpeta `editores/jetbrains/` (ver su LEEME) |

El `scopeName` es `source.nooki`; la extensión es `.oki`.

### 8.2 Servidor de lenguaje

`nooki lsp` habla JSON-RPC por **entrada/salida estándar**. No abre un puerto. El editor
debe lanzar el proceso en la raíz del proyecto (o en cualquier carpeta) y enviar
`Content-Length` correctamente.

Capacidades anunciadas:

| Capacidad | Detalle |
|---|---|
| Sincronización | texto completo en `didOpen` y `didChange` |
| Diagnósticos | `publishDiagnostics` con código `NOK`, rango y mensaje |
| Autocompletado | palabras clave, tipos, funciones y declaraciones del documento |
| Ir a la definición | funciones, tipos, enumeraciones, variables y parámetros del documento |
| Formato | documento completo, con el mismo formateador de `nooki formatear` |

Limitaciones:

- No resuelve módulos de otras carpetas ni el `nooki.toml`.
- No hay `hover`, `rename`, referencias, símbolos de workspace ni code actions.
- No hay análisis incremental: cada cambio recalcula el documento.
- El servidor no debe escribir logs en la salida estándar; rompería el protocolo.

#### Ejemplo para Neovim

```lua
vim.filetype.add({ extension = { oki = "nooki" } })

vim.api.nvim_create_autocmd("FileType", {
  pattern = "nooki",
  callback = function()
    vim.lsp.start({
      name = "nooki",
      cmd = { "nooki", "lsp" },
      root_dir = vim.fn.getcwd(),
    })
  end,
})
```

#### Ejemplo para Helix

```toml
# languages.toml
[language-server.nooki]
command = "nooki"
args = ["lsp"]

[[language]]
name = "nooki"
scope = "source.nooki"
file-types = ["oki"]
language-servers = ["nooki"]
```

#### VS Code y JetBrains

El repositorio **no incluye un cliente LSP** para VS Code ni para JetBrains; solo el
resaltado. Usa una extensión LSP genérica o el plugin LSP de JetBrains y configura el
comando `nooki` con el argumento `lsp`, transporte stdio. Si la extensión pide un
`languageId`, usa `nooki`.

### 8.3 Comprobación rápida del servidor

```bash
printf 'Content-Length: 58\r\n\r\n{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}' \
  | nooki lsp
```

Debe responder un JSON con `capabilities`. Si no responde, comprueba que `nooki` está en el
`PATH` del editor (los editores gráficos a veces no heredan el `PATH` del shell).

---

## 9. Integración continua

### 9.1 Flujo mínimo

```bash
set -e

nooki version
nooki formatear principal.oki biblioteca/*.oki --comprobar
nooki verificar principal.oki
nooki construir
nooki probar
```

Para una matriz real, añade:

```bash
ctest --preset linux --output-on-failure
```

### 9.2 Ejemplo de GitHub Actions

```yaml
name: Nooki

on:
  push:
    branches: [principal, main]
  pull_request:

jobs:
  compilar:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: windows-2022
            preset: windows-msvc
          - os: ubuntu-24.04
            preset: linux
          - os: macos-14
            preset: macos
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4

      - name: Instalar MinGW (Windows)
        if: runner.os == 'Windows'
        shell: pwsh
        run: |
          choco install mingw -y --no-progress
          "C:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\bin" |
            Out-File -FilePath $env:GITHUB_PATH -Append -Encoding utf8

      - name: Instalar X11 (Linux)
        if: runner.os == 'Linux'
        run: sudo apt-get update && sudo apt-get install -y libx11-dev

      - name: Configurar
        run: cmake --preset ${{ matrix.preset }}

      - name: Compilar
        run: cmake --build --preset ${{ matrix.preset }}

      - name: Probar
        run: ctest --preset ${{ matrix.preset }} --output-on-failure

      - name: Comprobar el binario
        shell: bash
        run: |
          if [ "$RUNNER_OS" = "Windows" ]; then
            bin="build/${{ matrix.preset }}/compilador/Release/nooki.exe"
          else
            bin="build/${{ matrix.preset }}/compilador/nooki"
          fi
          "$bin" version
          "$bin" ayuda > /dev/null
```

### 9.3 Matriz de pruebas del proyecto

| Trabajo | Qué valida |
|---|---|
| Windows · MSVC 2022 | compila el compilador con MSVC; backend MinGW instalado aparte |
| Linux · GCC | compilación y pruebas en Linux; X11 instalado |
| macOS · Clang | compilación y pruebas; UI sin backend |
| Linux · ASan + UBSan | memoria y comportamiento indefinido; avisos como errores |
| Formato (clang-format) | informativo; no bloquea |
| Análisis (clang-tidy) | informativo; no bloquea |

### 9.4 Consejos de CI

- Fija la versión del binario `nooki` y del compilador de C. No hay `nooki.lock`.
- Ejecuta `nooki verificar` antes de `construir` para obtener errores rápidos.
- Usa un `TMPDIR` por trabajo para evitar colisiones de archivos intermedios.
- En Linux sin pantalla, ejecuta las pruebas de UI con `xvfb-run -a`.
- Guarda `API.md` como artefacto si lo generas.
- No ejecutes `nooki limpiar` en la raíz del repositorio si hay binarios que quieras
  conservar.

---

## 10. Seguridad

### 10.1 Modelo de riesgo

Un programa Nooki se compila a código nativo y, al ejecutarse, tiene los mismos permisos que
el usuario que lo lanza. Además:

- `externo funcion` permite llamar a funciones de C enlazadas (libc y bibliotecas que añada
  el wrapper de `NOOKI_CC`).
- `asm!("...")` inserta ensamblador en línea; solo funciona con GCC/Clang.
- `inseguro { }` desactiva las comprobaciones aritméticas; no es un sandbox.
- `archivo` lee y escribe archivos con los permisos del proceso.
- `ventana` abre una interfaz gráfica; en un servidor no hay pantalla.

No hay sandbox, capabilities ni permisos declarativos. **Trata un `.oki` como tratarías un
`.c`.**

### 10.2 Recomendaciones

| Escenario | Medida |
|---|---|
| Compilar código de terceros | usuario sin privilegios, contenedor, sistema de archivos de solo lectura |
| Ejecutar código de terceros | contenedor sin red, límites de CPU/memoria, `TMPDIR` temporal |
| Dependencias | revisa las carpetas declaradas en `nooki.toml`; no hay firmas ni registro |
| Secretos | no los pases por variables de entorno si el programa no los necesita |
| Servidor de compilación | no ejecutes como `root`; aísla el backend de C |
| LSP en un workspace ajeno | el servidor solo analiza el documento, pero puede consumir CPU con archivos enormes |
| `asm!` | revísalo; puede saltarse cualquier garantía del lenguaje |
| `externo` | revisa qué símbolos y bibliotecas se enlazan; el wrapper puede añadir `-l...` |

### 10.3 Suministro y procedencia

- No hay registro de paquetes ni descarga automática: una dependencia es una carpeta local.
- No hay archivo de bloqueo ni verificación de integridad.
- La distribución incluye binarios; verifica su origen y, si es posible, compílalos desde el
  código fuente y compara `nooki version`.
- Los diagnósticos y el C generado pueden contener rutas y fragmentos de código fuente.
  Trátalos como datos del proyecto.

---

## 11. Solución de problemas

### 11.1 Tabla rápida

| Síntoma | Causa probable | Solución |
|---|---|---|
| `NOK6001`: no se encontró un compilador de C | no hay `cc`/`gcc`/`clang` en el `PATH` | instala uno o define `NOOKI_CC`; comprueba `cc --version` |
| `NOK6002`: el compilador de C terminó con código != 0 | C generado inválido, biblioteca ausente, combinación no soportada | usa `--conservar` o `--emitir-c`, revisa el error del backend; evita `Caja<texto>` |
| `NOK3008`: no se encontró el módulo | módulo fuera de las rutas | colócalo en `biblioteca/`, revisa `nooki.toml` o ejecuta desde la raíz |
| `NOK3009`: símbolo privado | falta `publico` | marca la función/tipo como `publico` |
| `NOK0001`: aquí no hay un proyecto | no hay `principal.oki` ni `fuente/principal.oki` | entra en la carpeta correcta o usa `nooki construir <carpeta>` |
| `NOK2003`: falta `principal` | el archivo es un módulo | añade `funcion principal() -> entero` |
| `NOK1004` en la primera línea | BOM UTF-8 | guarda UTF-8 sin BOM |
| `cannot find -lX11` / `X11/Xlib.h` | faltan cabeceras X11 | `sudo apt install libx11-dev` |
| El programa con ventana imprime `sin entorno grafico` | no hay pantalla o macOS | es el comportamiento esperado; usa `xvfb-run` en Linux |
| `ventana.abrir` devuelve 0 en macOS | no hay backend Cocoa | no hay UI nativa en macOS en la 1.0.0 |
| `nooki probar` falla por salida distinta | `.esperado` no coincide exactamente | compara con `diff`; revisa saltos finales y formato de flotantes |
| `formatear --comprobar` falla tras `nuevo` | falta salto de línea final | ejecuta `nooki formatear principal.oki` una vez |
| `documentar` dice que no hay nada público | no hay `publico` | marca la API con `publico` |
| El editor no ve diagnósticos | LSP no configurado o `PATH` distinto | lanza `nooki lsp` manualmente; revisa la configuración del editor |
| `nooki ejecutar` “falla” con código 3 | el programa devolvió 3 | es el código del programa, no del compilador |
| `asm!` falla al compilar | backend MSVC | usa GCC/Clang (MinGW) o quita `asm!` |
| `cl.exe` no se usa como backend | no se sondea | instala MinGW/Clang o apunta `NOOKI_CC` a un `cc` compatible |
| Compilaciones paralelas se pisan | mismo `<stem>` en la carpeta temporal | usa `TMPDIR` distinto o serializa |
| `nooki limpiar` borró binarios ajenos | borra todos los `.exe`/`.bin`/`.salida` | restaura desde el control de versiones; no lo ejecutes en la raíz |

### 11.2 Cómo recoger evidencia

```bash
nooki version
nooki verificar archivo.oki
nooki ast archivo.oki
nooki compilar archivo.oki --emitir-c > generado.c
nooki compilar archivo.oki --conservar
```

- `--emitir-c` permite ver el C exacto que se entrega al backend.
- `--conservar` deja el `.c` y muestra su ruta.
- `nooki ast` ayuda a distinguir un problema de sintaxis de uno semántico.
- En CI, guarda la salida de `ctest --output-on-failure` y la versión del compilador de C.

### 11.3 Errores del backend que no son de Nooki

Si el C generado es correcto pero el backend falla:

- comprueba que `cc` sea el que esperas (`cc --version`, `which cc`);
- revisa si `NOOKI_CC` apunta a un wrapper que no acepta `-O2`/`-o`;
- en Windows, confirma que el `PATH` del proceso incluye MinGW;
- en Linux, confirma `libx11-dev` solo si el programa usa UI;
- en macOS, la UI no existe; si el enlace falla por `-lX11`, es una limitación conocida de la
  1.0.0, no un problema de tu programa.

---

## 12. Mantenimiento y ciclo de vida

### 12.1 Política de versiones

- El proyecto usa versión semántica (`1.0.0`) y una **edición** del lenguaje (`2026`).
- `nooki version` muestra `nooki <versión> (<objetivo>, edición <edición>)`.
- La versión se define en `CMakeLists.txt`; no se edita a mano en el código.
- No hay promesa de compatibilidad binaria entre ediciones; los fuentes `.oki` son texto y
  se recompilan.

### 12.2 Actualización

1. Anota la versión actual: `nooki version`.
2. Ejecuta `ctest --preset <preset>` en una copia del proyecto.
3. Reemplaza el binario.
4. Ejecuta `nooki verificar` y `nooki probar` en los proyectos.
5. Revisa diferencias de salida (especialmente flotantes y diagnósticos).
6. Actualiza el resaltado y la configuración LSP si cambió el protocolo.

### 12.3 Copias de seguridad

- El código fuente y `nooki.toml` son texto: están en el control de versiones.
- `API.md` es generado; no es una copia de seguridad.
- Los artefactos (`build/`, binarios) son desechables.
- Si depends de una versión concreta del compilador, conserva el binario y el compilador de
  C usado; no hay lockfile.

### 12.4 Publicar una distribución

```powershell
# Windows
.\herramientas\empaquetar.ps1
```

```bash
# Linux y macOS
./herramientas/empaquetar.sh
```

El resultado queda en `dist/nooki-1.0.0-<plataforma>.zip` o `.tar.gz`. Contiene el binario,
documentación, ejemplos, editores, especificación y `INSTALAR.txt`; no incluye el código
fuente del compilador.

Antes de publicar:

1. `git status --short` limpio.
2. `ctest --preset <preset>` en verde.
3. `ctest --preset <preset> -R documentos`.
4. Comprobar que el binario de `dist/` ejecuta `ejemplos/01-hola`.
5. Etiquetar y adjuntar la distribución a la publicación.

### 12.5 Licencia

`MIT OR Apache-2.0` con excepción de runtime. Los textos están en `LICENCIA-MIT.txt` y
`LICENCIA-APACHE.txt`; el resumen, en [`LICENCIA.md`](../../LICENCIA.md). La distribución
incluye ambos.

---

## 13. Listas de verificación

### 13.1 Instalación

- [ ] `nooki version` responde y muestra la versión esperada.
- [ ] `cc --version` (o `gcc`/`clang`) responde.
- [ ] `nooki ejecutar ejemplos/01-hola/principal.oki` imprime `¡Hola, Nooki!`.
- [ ] En Linux con UI: `libx11-dev` instalado.
- [ ] `NOOKI_CC` documentado si se usa un wrapper.
- [ ] `NO_COLOR` configurado si los logs no admiten ANSI.

### 13.2 Proyecto nuevo

- [ ] `nooki nuevo <nombre>` crea `principal.oki`, `nooki.toml` y `biblioteca/`.
- [ ] `nooki construir` compila sin errores.
- [ ] `nooki probar` funciona tras crear `pruebas/`.
- [ ] `nooki formatear ... --comprobar` pasa.
- [ ] `nooki documentar` genera `API.md` si hay `publico`.
- [ ] El proyecto está en control de versiones.

### 13.3 CI

- [ ] Matriz de sistemas y presets.
- [ ] Compilador de C instalado en cada runner.
- [ ] X11 instalado en Linux si hay UI.
- [ ] `TMPDIR` por trabajo o compilaciones serializadas.
- [ ] `ctest` con `--output-on-failure`.
- [ ] `nooki formatear --comprobar` en un paso separado.
- [ ] Artefactos de diagnóstico guardados si falla.

### 13.4 Seguridad

- [ ] El código de terceros se compila/ejecuta sin privilegios.
- [ ] Las dependencias locales están revisadas.
- [ ] No hay secretos en el entorno de compilación.
- [ ] El backend de C y el binario `nooki` proceden de una fuente fiable.
- [ ] `asm!` y `externo` revisados en código ajeno.
- [ ] El servidor LSP no se expone a redes no confiables.

### 13.5 Publicación

- [ ] Versión y edición correctas.
- [ ] `ctest` en verde en los sistemas soportados.
- [ ] Documentación coherente (`-R documentos`).
- [ ] Distribución generada y probada.
- [ ] Licencia incluida.
- [ ] Notas de versión publicadas.
