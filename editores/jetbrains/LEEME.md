# Nooki para JetBrains

Resaltado de sintaxis para RustRover, IntelliJ IDEA, CLion, PyCharm, WebStorm, GoLand, PhpStorm,
RubyMine, DataGrip y cualquier otro IDE de JetBrains. Funciona en todos porque usa el mecanismo
**TextMate**, que la plataforma soporta de serie.

## Instalar

### Opción rápida (recomendada)

1. Abre **Settings → Plugins → Marketplace** y busca **TextMate Bundles**. Instálalo y reinicia.
2. **Settings → Editor → TextMate Bundles** (o *Appearance & Behavior → TextMate Bundles*).
3. Pulsa **+** y elige la carpeta `Nooki.tmbundle` de este directorio.
4. Abre cualquier archivo `.oki`: ya sale coloreado.

Si prefieres no descargar nada de la tienda, también vale el plugin **TextMate** de terceros, o
cualquiera que cargue bundles `.tmbundle`.

### Opción con el zip

`Nooki.tmbundle.zip` contiene lo mismo, para moverlo a otra máquina:

```bash
unzip Nooki.tmbundle.zip -d ~/Libros/TextMate      # macOS y Linux
```

Y en Windows, descomprímelo donde quieras y apunta ahí el paso 3.

### Comprobar que carga

Abre `ejemplos/06-tareas/principal.oki`. Deberías ver:

- `funcion`, `variable`, `mientras` en un color de palabra clave;
- `entero`, `texto`, `booleano` en otro (son tipos);
- los textos entre comillas en verde, y los comentarios en gris;
- `nuevo`, `este` y `super` como palabras del lenguaje.

Si no aparece nada, el bundle no se cargó: revisa que apuntaste a la **carpeta** `Nooki.tmbundle` y no
al archivo de dentro.

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

## Errores y autocompletado, además del color

El resaltado es solo color. Para **subrayar errores** y tener **autocompletado** e **ir a la
definición**, hace falta conectar el servidor de lenguaje:

1. Instala el plugin **LSP4IJ** (o cualquier cliente LSP para JetBrains).
2. Añade un servidor nuevo:
   - **Comando**: `nooki lsp`
   - **Archivos**: `*.oki`
   - **Transporte**: stdio (el que usa por defecto)
3. Abre un `.oki`: los errores aparecerán subrayados mientras escribes, con su código `NOK` y su
   explicación en español.

## Regenerar el bundle

El bundle se genera desde una definición única —la misma que usa VS Code— para que los dos editores no
se desincronicen:

```bash
python herramientas/generar-gramaticas.py
```

Escribe el JSON de VS Code y los dos `.plist` de aquí. Después, vuelve a comprimir la carpeta si quieres
actualizar el zip.
