"""Convierte un archivo C en líneas de un literal de C++ para el preludio.

Cada línea del fuente se convierte en `"    ...\\n"`, escapando las comillas y
las barras invertidas. Así el runtime se escribe como C normal y se incrusta sin
tocar el archivo a mano.
"""

from __future__ import annotations

import argparse
from pathlib import Path


def escapar(linea: str) -> str:
    salida = linea.replace("\\", "\\\\").replace('"', '\\"')
    return f'    "{salida}\\n"'


def main() -> int:
    analizador = argparse.ArgumentParser(description=__doc__)
    analizador.add_argument("fuente")
    analizador.add_argument("--salida", required=True)
    argumentos = analizador.parse_args()

    texto = Path(argumentos.fuente).read_text(encoding="utf-8")
    lineas = [escapar(linea) for linea in texto.rstrip("\n").split("\n")]
    Path(argumentos.salida).write_text("\n".join(lineas) + "\n", encoding="utf-8")
    print(f"{len(lineas)} lineas -> {argumentos.salida}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
