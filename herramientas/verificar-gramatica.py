#!/usr/bin/env python3
"""Comprueba que la gramática de Nooki es válida y que acierta sobre código real.

Se ejecuta a mano o desde la generación:

    python herramientas/verificar-gramatica.py

Comprueba tres cosas:

1. Que el plist y el JSON se leen y tienen las claves que el editor espera.
2. Que **todas** las expresiones regulares compilan (una sola rota haría que el
   resaltado fallara en silencio).
3. Que sobre un fragmento real, cada construcción cae en la regla que le toca.
"""
from __future__ import annotations

import json
import plistlib
import pathlib
import re
import sys

RAIZ = pathlib.Path(__file__).resolve().parent.parent
PLIST = RAIZ / "editores" / "jetbrains" / "Nooki.tmbundle" / "Syntaxes" / "Nooki.tmLanguage"
JSON = RAIZ / "editores" / "vscode" / "syntaxes" / "nooki.tmLanguage.json"


def recorrer(nodo, salida: list[str]) -> None:
    """Recoge todas las claves `match` y los `begin` de una gramática."""
    if isinstance(nodo, dict):
        for clave, valor in nodo.items():
            if clave in ("match", "begin") and isinstance(valor, str):
                salida.append(valor)
            else:
                recorrer(valor, salida)
    elif isinstance(nodo, list):
        for elemento in nodo:
            recorrer(elemento, salida)


def reglas_de_repository(gramatica: dict) -> dict[str, list[str]]:
    """Nombre de regla → lista de patrones, aplanando `patterns`."""
    reglas: dict[str, list[str]] = {}
    for nombre, contenido in gramatica.get("repository", {}).items():
        patrones: list[str] = []
        recorrer(contenido, patrones)
        reglas[nombre] = patrones
    return reglas


def main() -> int:
    problemas = 0

    if not PLIST.exists() or not JSON.exists():
        print("faltan los archivos de gramática: ejecuta generar-gramaticas.py antes")
        return 1

    with PLIST.open("rb") as flujo:
        de_jetbrains = plistlib.load(flujo)
    with JSON.open(encoding="utf-8") as flujo:
        de_vscode = json.load(flujo)

    # 1. Claves que el editor necesita.
    for nombre, gramatica in (("jetbrains", de_jetbrains), ("vscode", de_vscode)):
        for clave in ("name", "scopeName", "patterns", "repository"):
            if clave not in gramatica:
                print(f"FALLO: a la gramática de {nombre} le falta «{clave}»")
                problemas += 1

    # 2. Las dos gramáticas tienen las mismas reglas: no se han desincronizado.
    de_jb = reglas_de_repository(de_jetbrains)
    de_vs = reglas_de_repository(de_vscode)
    if set(de_jb) != set(de_vs):
        print(f"FALLO: las reglas no coinciden: {set(de_jb) ^ set(de_vs)}")
        problemas += 1

    # 3. Todas las expresiones regulares compilan.
    patrones: list[str] = []
    recorrer(de_jetbrains, patrones)
    for patron in patrones:
        try:
            re.compile(patron)
        except re.error as error:
            print(f"FALLO: la expresión «{patron}» no compila: {error}")
            problemas += 1
    print(f"expresiones regulares revisadas: {len(patrones)}")

    # 4. Cada construcción cae en la regla que le toca.
    casos = [
        ("declaraciones", "funcion principal() -> entero {", "funcion"),
        ("declaraciones", "clase gato hereda animal {", "clase"),
        ("control", "mientras (n < 3) {", "mientras"),
        ("control", "coincide (f) {", "coincide"),
        ("tipos", "variable x: entero = 1", "entero"),
        ("literales", "variable ok = verdadero", "verdadero"),
        ("modulos", "usar matematicas", "usar"),
        ("objeto", "variable p = nuevo perro {", "nuevo"),
        ("comentarios", "// esto es un comentario", "//"),
        ("numeros", "variable n = 1_000", "1_000"),
        ("numeros", "variable d = 3.5", "3.5"),
        ("textos", 'variable t = "hola"', '"hola"'),
    ]
    for regla, linea, buscado in casos:
        acierta = False
        for patron in de_jb.get(regla, []):
            if re.search(patron, linea):
                acierta = True
                break
        if not acierta:
            print(f"FALLO: «{buscado}» no cae en la regla «{regla}»")
            problemas += 1

    # Las palabras del plan viejo no deben aparecer: confundirían al editor.
    prohibidas = ["sinosi", "intentar", "atrapar", "asincrono", "mixin", "macro"]
    texto = PLIST.read_text(encoding="utf-8")
    for prohibida in prohibidas:
        if re.search(rf"\b{prohibida}\b", texto):
            print(f"FALLO: «{prohibida}» sigue en la gramática y no es del lenguaje")
            problemas += 1

    if problemas:
        print(f"{problemas} problema(s) en la gramática")
        return 1
    print("gramática correcta: reglas, expresiones y coincidencias")
    return 0


if __name__ == "__main__":
    sys.exit(main())
