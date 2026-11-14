#!/usr/bin/env python3
"""Genera las tablas de REFERENCIA.md desde el propio compilador.

Las palabras reservadas salen de `lexer.cpp`, los códigos de diagnóstico de
`codigos.hpp` y sus mensajes de `catalogo.cpp`, y las firmas de la biblioteca de
los módulos que van dentro del compilador (`compilar.cpp`). Así la referencia no
puede desincronizarse: si se añade un código, se regenera y aparece.

    python herramientas/generar-referencia.py

Escribe `docs/referencia/palabras.md`, `docs/referencia/diagnosticos.md` y
`docs/referencia/biblioteca.md`; REFERENCIA.md los incluye.
"""
from __future__ import annotations

import pathlib
import re
import sys

RAIZ = pathlib.Path(__file__).resolve().parent.parent


def palabras_reservadas() -> list[str]:
    fuente = (RAIZ / "compilador" / "fuente" / "lexico" / "lexer.cpp").read_text(encoding="utf-8")
    bloque = re.search(r"PALABRAS_RESERVADAS\{\{(.*?)\}\}", fuente, re.S)
    if not bloque:
        raise SystemExit("no se encontró PALABRAS_RESERVADAS en lexer.cpp")
    return re.findall(r'"([^"]+)"', bloque.group(1))


def codigos() -> list[tuple[str, str]]:
    """(constante, código) en el orden en que están declarados."""
    cabecera = (RAIZ / "compilador" / "incluir" / "nooki" / "diagnostico" / "codigos.hpp").read_text(
        encoding="utf-8")
    return [(m.group(1), m.group(2)) for m in re.finditer(
        r'inline constexpr std::string_view\s+(\w+)\s*=\s*"(NOK\d+)"', cabecera)]


def mensajes() -> dict[str, str]:
    catalogo = (RAIZ / "compilador" / "fuente" / "diagnostico" / "catalogo.cpp").read_text(
        encoding="utf-8")
    salida: dict[str, str] = {}
    for m in re.finditer(
            r'\{codigos::(\w+),\s*"([^"]*)"', catalogo, re.S):
        salida[m.group(1)] = m.group(2)
    return salida


def modulos() -> dict[str, list[str]]:
    """Módulo → firmas públicas, sacadas de los textos que van en el compilador."""
    fuente = (RAIZ / "compilador" / "fuente" / "conductor" / "compilar.cpp").read_text(
        encoding="utf-8")
    salida: dict[str, list[str]] = {}
    for m in re.finditer(r'if \(modulo == "(\w+)"\) \{\s*return R"\((.*?)\)";', fuente, re.S):
        nombre, cuerpo = m.group(1), m.group(2)
        firmas = []
        for linea in cuerpo.splitlines():
            limpia = linea.strip()
            if limpia.startswith("publico funcion ") or limpia.startswith("publico enumeracion ") \
                    or limpia.startswith("publico estructura "):
                firmas.append(limpia)
        salida[nombre] = firmas
    return salida


def grupo_de(palabra: str) -> str:
    """A qué grupo pertenece una palabra reservada, para agrupar la tabla."""
    grupos = {
        "Declaraciones": ["variable", "constante", "funcion", "clase", "estructura", "rasgo",
                          "interfaz", "enumeracion", "tipo", "modulo", "importar", "exportar",
                          "implementar", "externo", "usar"],
        "Control de flujo": ["si", "sino", "mientras", "para", "en", "hacer", "romper",
                             "continuar", "retornar", "coincide", "caso"],
        "Objetos": ["nuevo", "este", "Este", "super", "hereda", "implementa", "constructor",
                    "destructor", "propiedad", "obtener", "establecer"],
        "Acceso": ["publico", "privado", "protegido", "interna", "mutable", "estatico",
                   "abstracta"],
        "Tipos": ["entero", "entero8", "entero16", "entero32", "entero64", "entero128",
                  "natural", "natural8", "natural16", "natural32", "natural64", "natural128",
                  "flotante", "flotante32", "booleano", "texto", "caracter", "vacio", "byte",
                  "tamano", "tamano_signado", "lista"],
        "Literales": ["verdadero", "falso", "nulo"],
        "Bajo nivel": ["inseguro", "asm"],
        "Conversión": ["como"],
        "Reservadas sin uso todavía": ["abstracta", "asincrono", "atrapar", "bucle",
                                       "donde", "es", "esperar", "finalmente", "intentar",
                                       "lanzar", "macro", "mixin", "paso", "union", "sinosi"],
    }
    for nombre, palabras in grupos.items():
        if palabra in palabras:
            return nombre
    return "Otras"


def main() -> int:
    destino = RAIZ / "docs" / "referencia"
    destino.mkdir(parents=True, exist_ok=True)

    # 1. Palabras reservadas, agrupadas.
    palabras = palabras_reservadas()
    por_grupo: dict[str, list[str]] = {}
    for palabra in sorted(palabras):
        por_grupo.setdefault(grupo_de(palabra), []).append(palabra)
    lineas = [
        "<!-- GENERADO por herramientas/generar-referencia.py: no editar a mano -->",
        "",
        f"El compilador reconoce **{len(palabras)} palabras reservadas**. Ninguna puede usarse como",
        "nombre de variable, función, campo o tipo.",
        "",
        "| Grupo | Palabras |",
        "|---|---|",
    ]
    orden = ["Declaraciones", "Control de flujo", "Objetos", "Acceso", "Tipos", "Literales",
             "Conversión", "Bajo nivel", "Reservadas sin uso todavía", "Otras"]
    for grupo in orden:
        if grupo in por_grupo:
            lista = ", ".join(f"`{p}`" for p in por_grupo[grupo])
            lineas.append(f"| {grupo} ({len(por_grupo[grupo])}) | {lista} |")
    (destino / "palabras.md").write_text("\n".join(lineas) + "\n", encoding="utf-8")
    print(f"escrito: docs/referencia/palabras.md ({len(palabras)} palabras)")

    # 2. Diagnósticos.
    textos = mensajes()
    lista_codigos = codigos()
    lineas = [
        "<!-- GENERADO por herramientas/generar-referencia.py: no editar a mano -->",
        "",
        f"Hay **{len(lista_codigos)} códigos**. El mensaje es el que imprime el compilador, con sus",
        "huecos entre llaves.",
        "",
        "| Código | Constante | Mensaje |",
        "|---|---|---|",
    ]
    for constante, codigo in lista_codigos:
        mensaje = textos.get(constante, "").replace("|", "\\|")
        lineas.append(f"| `{codigo}` | `{constante}` | {mensaje} |")
    (destino / "diagnosticos.md").write_text("\n".join(lineas) + "\n", encoding="utf-8")
    print(f"escrito: docs/referencia/diagnosticos.md ({len(lista_codigos)} códigos)")

    # 3. Biblioteca: firma de cada función pública por módulo.
    biblioteca = modulos()
    lineas = [
        "<!-- GENERADO por herramientas/generar-referencia.py: no editar a mano -->",
        "",
        f"**{len(biblioteca)} módulos**, todos escritos en Nooki. Estas son sus declaraciones públicas,",
        "copiadas de la fuente que va dentro del compilador.",
        "",
    ]
    for nombre in sorted(biblioteca):
        lineas.append(f"### `{nombre}`")
        lineas.append("")
        firmas = biblioteca[nombre]
        if not firmas:
            lineas.append("*(sin declaraciones públicas propias)*")
        for firma in firmas:
            lineas.append("```text")
            lineas.append(firma)
            lineas.append("```")
        lineas.append("")
    (destino / "biblioteca.md").write_text("\n".join(lineas) + "\n", encoding="utf-8")
    total = sum(len(v) for v in biblioteca.values())
    print(f"escrito: docs/referencia/biblioteca.md ({len(biblioteca)} módulos, {total} firmas)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
