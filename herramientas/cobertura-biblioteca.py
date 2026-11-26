#!/usr/bin/env python3
"""Qué funciones públicas de la biblioteca no llama ninguna prueba.

    python herramientas/cobertura-biblioteca.py

Lee las firmas de `docs/referencia/biblioteca.md` (que se genera del código) y
busca cada nombre en los programas de `pruebas/` y `ejemplos/`. Una función que
no aparece en ninguno es una firma que nadie ha ejecutado nunca: compila, pero
puede estar mal.

Sale con 1 si queda alguna sin cubrir, para poder usarlo como barrera.
"""

import re
import sys
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
REFERENCIA = RAIZ / "docs" / "referencia" / "biblioteca.md"


def firmas_por_modulo():
    if not REFERENCIA.is_file():
        print(f"falta {REFERENCIA}; genérelo con herramientas/generar-referencia.py")
        sys.exit(69)

    partes = re.split(r"^### `([^`]+)`", REFERENCIA.read_text(encoding="utf-8"), flags=re.M)
    firmas = {}
    for i in range(1, len(partes), 2):
        nombre, cuerpo = partes[i], partes[i + 1]
        firmas[nombre] = re.findall(r"publico funcion ([a-zA-Z_][a-zA-Z0-9_]*)\(", cuerpo)
    return firmas


def texto_de_pruebas():
    texto = []
    for patron in ("pruebas/**/*.oki", "ejemplos/**/*.oki"):
        for ruta in RAIZ.glob(patron):
            texto.append(ruta.read_text(encoding="utf-8", errors="replace"))
    return "\n".join(texto)


def main():
    firmas = firmas_por_modulo()
    usado = texto_de_pruebas()

    total = sum(len(f) for f in firmas.values())
    sin_usar = []
    for nombre in sorted(firmas):
        faltan = [f for f in firmas[nombre] if f + "(" not in usado]
        if faltan:
            sin_usar.append((nombre, len(firmas[nombre]), faltan))

    print(f"{len(firmas)} módulos, {total} firmas")
    for nombre, cuantas, faltan in sin_usar:
        print(f"  {nombre} ({len(faltan)}/{cuantas}): {' '.join(faltan)}")

    pendientes = sum(len(f) for _, _, f in sin_usar)
    print(f"sin llamar: {pendientes}")
    return 1 if pendientes else 0


if __name__ == "__main__":
    sys.exit(main())
