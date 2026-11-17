"""Saca el texto de una página de la wiki, para traducirla.

Escribe un Markdown sencillo: encabezados, párrafos, listas y bloques de código
tal cual (el código no se traduce). Con `--todas` las procesa todas.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from lxml import html

BLOQUES = ("p", "li", "h1", "h2", "h3", "h4", "pre", "table", "blockquote")


def texto_de(nodo) -> str:
    return " ".join(nodo.text_content().split())


def a_markdown(bloque, salida: list[str]) -> None:
    for hijo in bloque.iterchildren():
        etiqueta = hijo.tag if isinstance(hijo.tag, str) else ""
        if etiqueta in ("h1", "h2", "h3", "h4"):
            nivel = int(etiqueta[1])
            salida.append(f"{'#' * nivel} {texto_de(hijo)}\n")
        elif etiqueta == "p":
            texto = texto_de(hijo)
            if texto:
                salida.append(f"{texto}\n")
        elif etiqueta in ("ul", "ol"):
            for elemento in hijo.iterchildren("li"):
                salida.append(f"- {texto_de(elemento)}\n")
            salida.append("")
        elif etiqueta == "pre":
            salida.append("```\n" + hijo.text_content().rstrip() + "\n```\n")
        elif etiqueta == "table":
            for fila in hijo.iterchildren("tr"):
                celdas = [texto_de(celda) for celda in fila.iterchildren("td", "th")]
                if celdas:
                    salida.append("| " + " | ".join(celdas) + " |\n")
            salida.append("")
        elif etiqueta == "blockquote":
            salida.append(f"> {texto_de(hijo)}\n")
        elif etiqueta in ("div", "section", "details", "summary"):
            a_markdown(hijo, salida)


def extraer(archivo: Path) -> str:
    documento = html.parse(str(archivo)).getroot()
    bloques = documento.xpath("//div[contains(@class,'prose-custom')]")
    if not bloques:
        return ""
    salida: list[str] = []
    a_markdown(bloques[-1], salida)
    texto = "\n".join(salida)
    return re.sub(r"\n{3,}", "\n\n", texto).strip() + "\n"


def main() -> int:
    analizador = argparse.ArgumentParser(description=__doc__)
    analizador.add_argument("--entrada", default="build/wiki/nooki")
    analizador.add_argument("--salida", default="build/wiki-en")
    analizador.add_argument("--pagina", help="solo una página, por su nombre sin .html")
    argumentos = analizador.parse_args()

    carpeta = Path(argumentos.entrada)
    destino = Path(argumentos.salida)
    destino.mkdir(parents=True, exist_ok=True)

    if argumentos.pagina:
        archivos = [carpeta / f"{argumentos.pagina}.html"]
    else:
        archivos = sorted(
            archivo for archivo in carpeta.glob("*.html") if archivo.name != "indice.html"
        )

    for archivo in archivos:
        texto = extraer(archivo)
        if not texto:
            print(f"  aviso: {archivo.name} sin contenido", file=sys.stderr)
            continue
        (destino / f"{archivo.stem}.md").write_text(texto, encoding="utf-8")
        print(f"  {archivo.stem}: {len(texto.split())} palabras")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
