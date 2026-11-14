#!/usr/bin/env python3
"""Genera las gramáticas de resaltado de Nooki para VS Code y para JetBrains.

Una sola definición, dos formatos:

  editores/vscode/syntaxes/nooki.tmLanguage.json     JSON (TextMate JSON)
  editores/jetbrains/Nooki.tmbundle/Syntaxes/Nooki.tmLanguage   plist XML
  editores/jetbrains/Nooki.tmbundle/info.plist

Así los dos editores resaltan lo mismo y no se desincronizan. Ejecutar desde la
raíz del repositorio:

    python herramientas/generar-gramaticas.py
"""
from __future__ import annotations

import json
import pathlib
import plistlib
import sys
import uuid

RAIZ = pathlib.Path(__file__).resolve().parent.parent

# Un identificador puede llevar tildes y eñes.
ID = r"[A-Za-zÁÉÍÓÚÜÑáéíóúüñ_][A-Za-zÁÉÍÓÚÜÑáéíóúüñ0-9_]*"

DECLARACIONES = [
    "funcion", "variable", "constante", "clase", "estructura", "rasgo", "interfaz",
    "enumeracion", "tipo", "modulo", "importar", "exportar", "implementar", "externo",
]
CONTROL = [
    "si", "sino", "mientras", "para", "en", "hacer", "romper", "continuar", "retornar",
    "coincide", "caso", "defecto",
]
OBJETO = ["nuevo", "este", "Este", "super", "hereda", "implementa"]
VISIBILIDAD = ["publico", "privado", "protegido", "interna"]
META = ["inseguro", "asm", "usar"]
LITERALES = ["verdadero", "falso", "nulo"]
TIPOS = [
    "entero", "flotante", "booleano", "texto", "caracter", "vacio", "lista",
    "tamano", "esperar",
]
GENERICOS = ["Vector", "Mapa", "Caja", "resultado"]


def alternativas(palabras: list[str]) -> str:
    return "|".join(palabras)


GRAMATICA = {
    "name": "Nooki",
    "scopeName": "source.nooki",
    "fileTypes": ["oki"],
    "patterns": [
        {"include": "#comentarios"},
        {"include": "#texto_multilinea"},
        {"include": "#textos"},
        {"include": "#numeros"},
        {"include": "#declaraciones"},
        {"include": "#control"},
        {"include": "#tipos"},
        {"include": "#literales"},
        {"include": "#modulos"},
        {"include": "#objeto"},
        {"include": "#llamadas"},
        {"include": "#referencias"},
        {"include": "#operadores"},
    ],
    "repository": {
        "comentarios": {
            "patterns": [
                {"name": "comment.line.double-slash.nooki", "match": "//.*$"},
                {
                    "name": "comment.block.nooki",
                    "begin": r"/\*",
                    "end": r"\*/",
                },
            ]
        },
        # Los textos de tres comillas admiten varias líneas y no escapan nada.
        "texto_multilinea": {
            "patterns": [
                {
                    "name": "string.quoted.triple.nooki",
                    "begin": r'"""',
                    "end": r'"""',
                }
            ]
        },
        "textos": {
            "patterns": [
                {
                    "name": "string.quoted.double.nooki",
                    "begin": r'"',
                    "end": r'"',
                    "patterns": [
                        {"name": "constant.character.escape.nooki", "match": r"\\."}
                    ],
                }
            ]
        },
        "numeros": {
            "patterns": [
                {
                    "name": "constant.numeric.float.nooki",
                    "match": r"\b[0-9][0-9_]*\.[0-9][0-9_]*\b",
                },
                {
                    "name": "constant.numeric.hex.nooki",
                    "match": r"\b0x[0-9A-Fa-f][0-9A-Fa-f_]*\b",
                },
                {
                    "name": "constant.numeric.integer.nooki",
                    "match": r"\b[0-9][0-9_]*\b",
                },
            ]
        },
        "declaraciones": {
            "patterns": [
                {
                    "match": rf"\b(funcion)\s+({ID})",
                    "captures": {
                        "1": {"name": "storage.type.function.nooki"},
                        "2": {"name": "entity.name.function.nooki"},
                    },
                },
                {
                    "match": rf"\b(clase|estructura|rasgo|interfaz|enumeracion|tipo)\s+({ID})",
                    "captures": {
                        "1": {"name": "storage.type.class.nooki"},
                        "2": {"name": "entity.name.type.nooki"},
                    },
                },
                {
                    "match": rf"\b({alternativas(DECLARACIONES)})\b",
                    "name": "keyword.declaration.nooki",
                },
                {
                    "match": rf"\b({alternativas(VISIBILIDAD)})\b",
                    "name": "storage.modifier.nooki",
                },
                {"match": r"\b&mutable\b", "name": "storage.modifier.mutable.nooki"},
            ]
        },
        "control": {
            "patterns": [
                {
                    "match": rf"\b({alternativas(CONTROL)})\b",
                    "name": "keyword.control.nooki",
                }
            ]
        },
        "tipos": {
            "patterns": [
                {
                    "match": rf"\b({alternativas(TIPOS)})\b",
                    "name": "storage.type.primitivo.nooki",
                },
                {
                    "match": rf"\b({alternativas(GENERICOS)})\b",
                    "name": "support.type.nooki",
                },
            ]
        },
        "literales": {
            "patterns": [
                {
                    "match": rf"\b({alternativas(LITERALES)})\b",
                    "name": "constant.language.nooki",
                }
            ]
        },
        "modulos": {
            "patterns": [
                {
                    "match": rf"\b({alternativas(META)})\b",
                    "name": "keyword.other.nooki",
                }
            ]
        },
        "objeto": {
            "patterns": [
                {
                    "match": rf"\b({alternativas(OBJETO)})\b",
                    "name": "variable.language.nooki",
                }
            ]
        },
        # `modulo.funcion` en las llamadas a la biblioteca.
        "llamadas": {
            "patterns": [
                {
                    "match": rf"\b({ID})(\.)({ID})(\s*)(?=\()",
                    "captures": {
                        "1": {"name": "entity.name.namespace.nooki"},
                        "2": {"name": "punctuation.accessor.nooki"},
                        "3": {"name": "entity.name.function.nooki"},
                    },
                },
                {
                    "match": rf"\b({ID})\s*(?=\()",
                    "name": "entity.name.function.llamada.nooki",
                },
            ]
        },
        # `&x`, `&mutable x`, `*x`, y el `?` de propagación.
        "referencias": {
            "patterns": [
                {"match": r"&(mutable\b)?", "name": "keyword.operator.referencia.nooki"},
                {"match": r"\*(?=\s*[A-Za-zÁÉÍÓÚÜÑáéíóúüñ_])", "name": "keyword.operator.desreferencia.nooki"},
                {"match": r"\?", "name": "keyword.operator.propagacion.nooki"},
            ]
        },
        "operadores": {
            "patterns": [
                {
                    "match": r"\*\*|==|!=|<=|>=|&&|\|\||\.\.=|\.\.|->|[+\-*/%<>=!]",
                    "name": "keyword.operator.nooki",
                },
                {"match": r"[{}()\[\],;:]", "name": "punctuation.nooki"},
            ]
        },
    },
}

INFO = {
    "name": "Nooki",
    "contactName": "Nooki",
    "description": "Resaltado de sintaxis para el lenguaje Nooki (.oki)",
    "uuid": str(uuid.uuid5(uuid.NAMESPACE_DNS, "nooki.grammar")),
}


def main() -> int:
    vscode = RAIZ / "editores" / "vscode" / "syntaxes" / "nooki.tmLanguage.json"
    vscode.parent.mkdir(parents=True, exist_ok=True)
    gramatica = {"$schema": "https://raw.githubusercontent.com/martinring/tmlanguage/master/tmlanguage.json", **GRAMATICA}
    vscode.write_text(json.dumps(gramatica, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"escrito: {vscode.relative_to(RAIZ)}")

    bundle = RAIZ / "editores" / "jetbrains" / "Nooki.tmbundle"
    (bundle / "Syntaxes").mkdir(parents=True, exist_ok=True)
    con_plist = dict(GRAMATICA)
    con_plist["uuid"] = INFO["uuid"]
    objetivo = bundle / "Syntaxes" / "Nooki.tmLanguage"
    with objetivo.open("wb") as flujo:
        plistlib.dump(con_plist, flujo)
    print(f"escrito: {objetivo.relative_to(RAIZ)}")

    info = bundle / "info.plist"
    with info.open("wb") as flujo:
        plistlib.dump(INFO, flujo)
    print(f"escrito: {info.relative_to(RAIZ)}")

    # Se comprueba lo generado: una expresión mal escrita fallaría en silencio
    # dentro del editor, así que se revisa aquí.
    import subprocess

    return subprocess.call([sys.executable, str(RAIZ / "herramientas" / "verificar-gramatica.py")])


if __name__ == "__main__":
    raise SystemExit(main())
