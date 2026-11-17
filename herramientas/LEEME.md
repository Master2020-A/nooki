# Herramientas

Utilidades del proyecto que no forman parte del compilador.

## `descargar-wiki.ps1`

Descarga la wiki que **DeepWiki** genera para el repositorio y la convierte en un HTML de una pieza y su
PDF:

```powershell
.\herramientas\descargar-wiki.ps1 -Repositorio Master2020-A/nooki
```

Deja `dist/wiki-nooki.html` y `dist/wiki-nooki.pdf`. El HTML lleva el índice, las 34 páginas, sus enlaces
internos convertidos en anclas y el CSS incrustado, así que se abre sin conexión.

DeepWiki sirve el contenido con JavaScript y detrás de una comprobación anti-bots de Vercel: una descarga
normal recibe un 429. Por eso la herramienta usa **Chrome en modo headless**, que sí pasa la comprobación,
vuelca el DOM de cada página y extrae el bloque de contenido con `herramientas/wiki/extraer.py`. Después
imprime ese mismo HTML a PDF con Chrome. Necesita Chrome instalado (o `-Chrome <ruta>`) y el Python con
`lxml`.

Las páginas ya descargadas se conservan en `build/wiki/<nombre>/`, así que si algo falla se puede volver a
lanzar y solo baja lo que falte.

### Traducirla

DeepWiki genera la wiki en inglés. Cada página traducida va en `docs/wiki-es/<página>.html`, con el mismo
nombre que la original:

```powershell
.\herramientas\descargar-wiki.ps1 -Repositorio Master2020-A/nooki -Sufijo es
```

Produce `dist/wiki-nooki-es.html` y su PDF. Las páginas que aún no están traducidas salen en el original,
así que el resultado se puede usar desde la primera tanda. Para ver el texto de una página y traducirlo:

```powershell
python herramientas/wiki/a-markdown.py --pagina 3-compiler-architecture
```

Los diagramas de las páginas se conservan: se quedan los SVG que llevan texto dentro (los dibujos) y se
quitan los que son iconos de la interfaz.

## `empaquetar.ps1` / `empaquetar.sh`

Crea una distribución lista para entregar: el binario, la documentación, los ejemplos, los editores y
la especificación, con un `INSTALAR.txt` que dice cómo añadirla al `PATH`.

```powershell
.\herramientas\empaquetar.ps1              # Windows
```
```bash
./herramientas/empaquetar.sh               # Linux y macOS
```

El resultado queda en `dist/` como `.zip` o `.tar.gz`, con el nombre
`nooki-<versión>-<plataforma>`. Conviene comprobar que el binario que va dentro funciona:

```bash
$ cd dist && tar -xzf nooki-1.0.0-linux-x86_64.tar.gz
$ ./nooki-1.0.0-linux-x86_64/nooki --version
nooki 1.0.0 (linux-x86_64, edición 2026)
$ ./nooki-1.0.0-linux-x86_64/nooki ejecutar nooki-1.0.0-linux-x86_64/ejemplos/01-hola/principal.oki
¡Hola, Nooki!
```

Lo que incluye la distribución:

| Carpeta o archivo | Contenido |
|---|---|
| `nooki` | el compilador |
| `LEEME.md`, `GUIA.md`, `NOTAS_DE_VERSION.md` | para empezar |
| `CHANGELOG.md`, `DECISIONES.md` | el historial y las decisiones |
| `LICENCIA.md`, `LICENCIA-MIT.txt`, `LICENCIA-APACHE.txt` | la licencia |
| `CONTRIBUIR.md`, `PUBLICAR.md` | cómo participar y cómo se publica |
| `ejemplos/` | cuatro programas ejecutables |
| `editores/` | resaltado de sintaxis |
| `spec/` | la especificación del lenguaje |
| `INSTALAR.txt` | los pasos de instalación |

No incluye el código fuente del compilador: eso vive en el repositorio.
