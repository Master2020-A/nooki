"""Imprime páginas de la wiki en texto limpio, para traducirlas.

Quita el ruido que mete el volcado (CSS de los diagramas, la lista de archivos
fuente) y deja el texto con sus encabezados.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path


def limpiar(texto: str) -> str:
    lineas = []
    dentro_codigo = False
    for linea in texto.splitlines():
        if linea.startswith("```"):
            dentro_codigo = not dentro_codigo
            if dentro_codigo:
                lineas.append("```")
            else:
                lineas.append("```")
            continue
        if dentro_codigo:
            # Los diagramas vienen como CSS enorme: no se traducen.
            if "@keyframes" in linea or linea.startswith("#mermaid") or "svg{" in linea:
                continue
            if len(linea) > 400:
                continue
            lineas.append(linea)
            continue
        if re.fullmatch(r"- [\w./()-]+\.(md|txt|json|hpp|cpp|oki|toml)", linea.strip()):
            continue
        if linea.strip() == "":
            if lineas and lineas[-1] == "":
                continue
        lineas.append(linea)
    return "\n".join(lineas).strip()


def main() -> int:
    analizador = argparse.ArgumentParser(description=__doc__)
    analizador.add_argument("paginas", nargs="+")
    analizador.add_argument("--entrada", default="build/wiki-en")
    argumentos = analizador.parse_args()

    for nombre in argumentos.paginas:
        archivo = Path(argumentos.entrada) / f"{nombre}.md"
        if not archivo.exists():
            print(f"!! no existe {archivo}")
            continue
        print(f"\n========== {nombre} ==========\n")
        print(limpiar(archivo.read_text(encoding="utf-8")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
