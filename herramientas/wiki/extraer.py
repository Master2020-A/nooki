"""Junta las páginas de DeepWiki en un solo HTML legible y sin conexión.

DeepWiki no usa contenedores semánticos: el contenido vive en un `div` con la
clase `prose-custom`. Se extrae ese bloque de cada página, se le quitan los
botones de la interfaz, y se arma un documento con una sección por página, sus
enlaces internos convertidos en anclas y el CSS del sitio incrustado.
"""

from __future__ import annotations

import argparse
import html
import re
import sys
import urllib.request
from pathlib import Path

from lxml import etree, html as lhtml

# Lo que no tiene sentido fuera de la web: botones de copiar, menús, iconos.
FUERA = (
    ".//button",
    ".//nav",
    ".//svg",
    ".//script",
    ".//style",
    ".//*[contains(@class,'md:hidden')]",
)


def limpiar(nodo: lhtml.HtmlElement) -> None:
    for patron in FUERA:
        for encontrado in nodo.xpath(patron):
            padre = encontrado.getparent()
            if padre is not None:
                padre.remove(encontrado)


def contenido(archivo: Path) -> tuple[str, str] | None:
    """Devuelve (título, html del contenido) de una página volcada."""
    try:
        documento = lhtml.parse(str(archivo))
    except Exception as error:  # noqa: BLE001 - un archivo roto no debe cortar todo
        print(f"  aviso: no se pudo leer {archivo.name}: {error}", file=sys.stderr)
        return None

    raiz = documento.getroot()
    bloques = raiz.xpath("//div[contains(@class,'prose-custom')]")
    if not bloques:
        return None

    bloque = bloques[-1]
    limpiar(bloque)

    titulo = ""
    for encabezado in bloque.xpath(".//h1|.//h2"):
        titulo = " ".join(encabezado.text_content().split())
        if titulo:
            break

    cuerpo = "".join(
        etree.tostring(hijo, encoding="unicode", method="html") for hijo in bloque
    )
    return titulo, cuerpo


def css_del_sitio(archivo: Path, base: str) -> list[str]:
    """Las hojas de estilo que referenciaba la página, para incrustarlas."""
    try:
        documento = lhtml.parse(str(archivo))
    except Exception:  # noqa: BLE001
        return []
    enlaces = documento.getroot().xpath("//link[contains(@rel,'stylesheet')]/@href")
    return [href if href.startswith("http") else base + href for href in enlaces]


def descargar(url: str) -> str:
    peticion = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    with urllib.request.urlopen(peticion, timeout=30) as respuesta:  # noqa: S310
        return respuesta.read().decode("utf-8", errors="replace")


def main() -> int:
    analizador = argparse.ArgumentParser(description=__doc__)
    analizador.add_argument("--entrada", required=True, help="carpeta con los .html volcados")
    analizador.add_argument("--salida", required=True, help="HTML resultante")
    analizador.add_argument("--base", default="https://deepwiki.com")
    analizador.add_argument("--titulo", default="Wiki")
    argumentos = analizador.parse_args()

    carpeta = Path(argumentos.entrada)
    archivos = sorted(
        (ruta for ruta in carpeta.glob("*.html") if ruta.name != "indice.html"),
        key=lambda ruta: _orden(ruta.stem),
    )
    if not archivos:
        print("no hay páginas que juntar", file=sys.stderr)
        return 1

    # El CSS del sitio no se descarga: DeepWiki limita las peticiones que no
    # vienen de un navegador (HTTP 429) y su Tailwind pelearía con los estilos de
    # impresión. Se usa el CSS propio, que deja el archivo autónomo.
    estilos: list[str] = []

    secciones: list[str] = []
    indice: list[str] = []
    conocidas = {archivo.stem for archivo in archivos}
    saneados = {sanear(slug): slug for slug in conocidas}

    for archivo in archivos:
        extraido = contenido(archivo)
        if extraido is None:
            print(f"  aviso: {archivo.name} no tiene contenido", file=sys.stderr)
            continue
        titulo, cuerpo = extraido
        slug = archivo.stem

        # Los enlaces internos pasan a ser anclas dentro del documento.
        def a_ancla(coincidencia: re.Match[str]) -> str:
            destino = coincidencia.group(1)
            if sanear(destino) in saneados:
                return f'href="#{sanear(destino)}"'
            return coincidencia.group(0)

        cuerpo = re.sub(
            r'href="(?:https://deepwiki\.com)?/[^/"]+/[^/"]+/([^"#]+)"', a_ancla, cuerpo
        )

        secciones.append(
            f'<section class="pagina" id="{html.escape(slug)}">\n'
            f'<p class="procedencia">deepwiki.com/{slug}</p>\n'
            f"{cuerpo}\n</section>"
        )
        indice.append(
            f'<li><a href="#{html.escape(slug)}">{html.escape(titulo or slug)}</a></li>'
        )

    documento = f"""<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(argumentos.titulo)}</title>
<style>
{chr(10).join(estilos)}
</style>
<style>
  :root {{ color-scheme: dark; }}
  body {{ background: #0d1117; color: #e6edf3;
         font-family: "Segoe UI", system-ui, sans-serif; line-height: 1.65; }}
  .envoltorio {{ max-width: 52rem; margin: 0 auto; padding: 2rem 1.5rem 6rem; }}
  .portada h1 {{ font-size: 2.2rem; margin-bottom: .25rem; }}
  .portada p {{ color: #8b949e; margin-top: 0; }}
  .indice {{ columns: 2; gap: 2rem; padding-left: 1.2rem; }}
  .indice li {{ margin-bottom: .35rem; break-inside: avoid; }}
  .indice a {{ color: #58a6ff; text-decoration: none; }}
  .indice a:hover {{ text-decoration: underline; }}
  .pagina {{ break-before: page; page-break-before: always; padding-top: 1rem; }}
  .pagina:first-of-type {{ break-before: auto; page-break-before: auto; }}
  .procedencia {{ color: #6e7681; font-size: .78rem; margin-bottom: 1.5rem;
                  font-family: ui-monospace, monospace; }}
  .pagina h1 {{ font-size: 1.9rem; border-bottom: 1px solid #21262d; padding-bottom: .4rem; }}
  .pagina h2 {{ font-size: 1.35rem; margin-top: 2rem; }}
  .pagina h3 {{ font-size: 1.1rem; margin-top: 1.6rem; }}
  .pagina a {{ color: #58a6ff; }}
  .pagina code {{ background: #161b22; padding: .12rem .35rem; border-radius: 4px;
                  font-size: .9em; }}
  .pagina pre {{ background: #161b22; padding: 1rem; border-radius: 8px;
                 overflow-x: auto; border: 1px solid #21262d; }}
  .pagina pre code {{ background: none; padding: 0; }}
  .pagina table {{ border-collapse: collapse; width: 100%; margin: 1rem 0; }}
  .pagina th, .pagina td {{ border: 1px solid #21262d; padding: .45rem .7rem;
                            text-align: left; vertical-align: top; }}
  .pagina th {{ background: #161b22; }}
  .pagina img, .pagina svg {{ max-width: 100%; height: auto; }}
  @media print {{
    body {{ background: #fff; color: #111; }}
    .indice a, .pagina a {{ color: #0645ad; }}
    .pagina code, .pagina pre, .pagina th {{ background: #f3f4f6; }}
    .pagina pre, .pagina table {{ border-color: #d0d7de; }}
    .procedencia {{ color: #666; }}
    .pagina h1, .pagina h2, .pagina h3 {{ break-after: avoid; page-break-after: avoid; }}
    .pagina pre, .pagina table, .pagina blockquote {{ break-inside: avoid; }}
  }}
</style>
</head>
<body>
<div class="envoltorio">
<header class="portada">
<h1>{html.escape(argumentos.titulo)}</h1>
<p>Contenido generado por DeepWiki para el repositorio. Descargado el {__import__("datetime").date.today().isoformat()} · {len(secciones)} páginas.</p>
<h2>Índice</h2>
<ul class="indice">
{chr(10).join(indice)}
</ul>
</header>
{chr(10).join(secciones)}
</div>
</body>
</html>
"""

    Path(argumentos.salida).write_text(documento, encoding="utf-8")
    print(f"páginas incluidas: {len(secciones)}")
    return 0


def sanear(slug: str) -> str:
    """El mismo saneado que hace el script de PowerShell al nombrar archivos."""
    return re.sub(r'[\\/:*?"<>|]', "-", slug)


def _orden(nombre: str) -> tuple:
    """Ordena por la numeración de DeepWiki: 1, 1.1, 2, 3.1…"""
    partes = re.findall(r"\d+", nombre.split("-")[0])
    return tuple(int(parte) for parte in partes) if partes else (999,)


if __name__ == "__main__":
    raise SystemExit(main())
