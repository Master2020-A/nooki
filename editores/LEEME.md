# Resaltado de sintaxis para Nooki

Nooki escribe archivos `.oki`. Aquí están las definiciones para que los editores coloreen el
lenguaje: palabras reservadas, tipos, textos, comentarios y nombres de funciones.

| Editor | Carpeta |
|---|---|
| Visual Studio Code | [`vscode/`](vscode/) |
| **JetBrains** (RustRover, IntelliJ, CLion…) | [`jetbrains/`](jetbrains/) |
| Vim y Neovim | [`vim/`](vim/) |
| Sublime Text, TextMate, Zed y cualquier editor con gramáticas TextMate | [`vscode/syntaxes/nooki.tmLanguage.json`](vscode/syntaxes/nooki.tmLanguage.json) |

Las gramáticas de los tres editores se generan de una sola definición, para que resalten lo mismo:

```bash
python herramientas/generar-gramaticas.py
```

---

## Visual Studio Code

### Opción rápida: copiar la extensión

1. Copia la carpeta `editores/vscode` dentro de tu carpeta de extensiones:

   ```powershell
   # Windows
   Copy-Item -Recurse editores\vscode "$env:USERPROFILE\.vscode\extensions\nooki-1.0.0"
   ```

   ```bash
   # Linux y macOS
   cp -r editores/vscode ~/.vscode/extensions/nooki-1.0.0
   ```

2. Reinicia VS Code (o ejecuta **Recargar ventana** desde la paleta de comandos).

3. Abre cualquier `.oki`: la barra de estado debe decir **Nooki**.

### Opción con paquete

```bash
npm install -g @vscode/vsce
cd editores/vscode
vsce package          # genera nooki-1.0.0.vsix
code --install-extension nooki-1.0.0.vsix
```

### Qué colorea

| Elemento | Ejemplo |
|---|---|
| Palabras de declaración | `funcion`, `variable`, `clase`, `rasgo`, `constante` |
| Modificadores | `publico`, `privado`, `interna`, `abstracta` |
| Control | `si`, `sino`, `mientras`, `para`, `coincide`, `caso`, `retornar` |
| Tipos | `entero`, `texto`, `booleano`, `Vector`, `Mapa`, `Caja` |
| Literales | `verdadero`, `falso`, números, textos, caracteres |
| Nombres | la función tras `funcion`, el tipo tras `clase`, las llamadas `nombre(` |
| Módulos | `usar`, y el prefijo de `modulo.funcion` |
| Objeto | `nuevo`, `este`, `super` |
| Comentarios | `// …` |

---

## Vim y Neovim

Copia las dos carpetas en tu configuración:

```bash
mkdir -p ~/.vim/syntax ~/.vim/ftdetect
cp editores/vim/syntax/nooki.vim ~/.vim/syntax/
cp editores/vim/ftdetect/nooki.vim ~/.vim/ftdetect/
```

Para Neovim, la misma ruta bajo `~/.config/nvim/`.

Si no usas `ftdetect`, añade esto a tu `vimrc`:

```vim
au BufRead,BufNewFile *.oki set filetype=nooki
```

Comprobación: abre un `.oki` y ejecuta `:set syntax?`. Debe responder `syntax=nooki`.

---

## Sublime Text, TextMate y Zed

Los tres aceptan gramáticas TextMate. El archivo es
[`vscode/syntaxes/nooki.tmLanguage.json`](vscode/syntaxes/nooki.tmLanguage.json), con `scopeName`
`source.nooki` y extensión `.oki`.

**Sublime Text**: copia el `.tmLanguage.json` en `Packages/User/` y añade la extensión a
`Preferences → Settings`:

```json
"file_extensions": ["oki"]
```

**Zed**: en `~/.config/zed/settings.json`, dentro de `languages`, se puede declarar la gramática
apuntando al archivo, con `"name": "Nooki"` y `"grammar": "nooki.tmLanguage.json"`.

**TextMate**: doble clic sobre el `.tmLanguage.json` y el editor lo instala como paquete.

---

## Notepad++

Notepad++ no lee gramáticas TextMate, pero acepta un *User Defined Language*. La lista de palabras
reservadas está en este mismo documento y en
[`../compilador/fuente/lexico/lexer.cpp`](../../compilador/fuente/lexico/lexer.cpp): son las cadenas
entre comillas de la tabla de palabras clave.

---

## Mantener el resaltado al día

Las palabras reservadas del lenguaje viven en una sola lista:
`PALABRAS_CLAVE` en `compilador/fuente/lexico/lexer.cpp`. Si añades una, actualiza:

1. `editores/vscode/syntaxes/nooki.tmLanguage.json` (grupo que le corresponda).
2. `editores/vim/syntax/nooki.vim` (`syntax keyword`).
3. La tabla de este documento.

No hay comprobación automática de que las tres listas coincidan; si quieres añadirla, el sitio es
`compilador/pruebas/prueba_documentos.cpp`, que ya lee los documentos del proyecto.
