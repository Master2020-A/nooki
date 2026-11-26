#!/usr/bin/env bash
# Mide la memoria del código **generado**, no la del compilador.
#
#   ./herramientas/verificar-memoria.sh programa.oki [otro.oki...]
#
# El preset `saneamiento` instrumenta el compilador; esto instrumenta lo que el
# compilador produce, que es donde están las fugas de verdad. Compila el C de
# cada archivo con AddressSanitizer, lo ejecuta y enseña el informe.
#
# No falla a propósito: hoy hay dos clases de fuga conocidas (ver
# docs/guias/GUIA_DEL_DESARROLLADOR.md) y una herramienta que siempre sale en
# rojo no se usa. Cuando estén cerradas, se le pone `--estricto` y sirve de
# barrera en la CI.

set -u

if [ "$#" -eq 0 ]; then
    echo "uso: $0 <archivo.oki> [más archivos]"
    exit 64
fi

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
nooki="$raiz/build/linux/compilador/nooki"
if [ ! -x "$nooki" ]; then
    echo "falta $nooki; compile antes con: cmake --build --preset linux"
    exit 69
fi

temporal="$(mktemp -d)"
trap 'rm -rf "$temporal"' EXIT

for fuente in "$@"; do
    nombre="$(basename "$fuente" .oki)"
    echo "== $nombre =="

    if ! "$nooki" compilar "$fuente" --emitir-c > "$temporal/$nombre.c" 2> "$temporal/$nombre.err"; then
        echo "  no compila:"
        sed -n '1,4p' "$temporal/$nombre.err"
        continue
    fi

    if ! gcc -fsanitize=address,undefined -g -O0 -o "$temporal/$nombre" "$temporal/$nombre.c" 2> "$temporal/$nombre-gcc.err"; then
        echo "  el C generado no compila:"
        sed -n '1,4p' "$temporal/$nombre-gcc.err"
        continue
    fi

    ( cd "$temporal" && "./$nombre" > /dev/null 2> "$nombre-asan.txt" )
    codigo=$?

    if [ "$codigo" -ne 0 ]; then
        echo "  termina con $codigo (¿pánico? ¿asan?)"
    fi

    if [ -s "$temporal/$nombre-asan.txt" ]; then
        grep -E "Direct leak|SUMMARY|ERROR: AddressSanitizer" "$temporal/$nombre-asan.txt" | head -8
        # De dónde viene cada fuga, por función.
        grep -oE "in nooki_[a-zA-Z0-9_]+" "$temporal/$nombre-asan.txt" | sort | uniq -c | sort -rn | head -6
    else
        echo "  limpio"
    fi
    echo
done
