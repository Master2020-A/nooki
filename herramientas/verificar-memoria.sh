#!/usr/bin/env bash
# Mide la memoria del código **generado**, no la del compilador.
#
#   ./herramientas/verificar-memoria.sh programa.oki [otro.oki...]
#   ./herramientas/verificar-memoria.sh --estricto [programa.oki...]
#
# El preset `saneamiento` instrumenta el compilador; esto instrumenta lo que el
# compilador produce, que es donde están las fugas de verdad. Compila el C de
# cada archivo con AddressSanitizer, lo ejecuta y enseña el informe.
#
# Sin `--estricto` es informativo: enseña las fugas y no falla, porque todavía
# quedan clases conocidas (ver docs/guias/GUIA_DEL_DESARROLLADOR.md).
#
# Con `--estricto` compara con `herramientas/memoria-base.txt`, que apunta los
# casos que hoy fugan y cuántos bytes. Falla si aparece una fuga en un caso que
# estaba limpio, o si uno de la lista pierde más bytes que antes. Así el trabajo
# que queda está acotado y no crece por descuido.

set -u

estricto=0
if [ "${1:-}" = "--estricto" ]; then
    estricto=1
    shift
fi

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
linea_base="$raiz/herramientas/memoria-base.txt"

if [ "$#" -eq 0 ]; then
    if [ "$estricto" -eq 0 ]; then
        echo "uso: $0 [--estricto] <archivo.oki> [más archivos]"
        exit 64
    fi
    # Sin argumentos y en estricto: toda la batería de conformidad.
    set -- "$raiz"/pruebas/conformidad/valido/*/principal.oki
fi

nooki="$raiz/build/linux/compilador/nooki"
if [ ! -x "$nooki" ]; then
    echo "falta $nooki; compile antes con: cmake --build --preset linux"
    exit 69
fi

temporal="$(mktemp -d)"
trap 'rm -rf "$temporal"' EXIT

fallos=0
medidos=0

# Cuántos bytes apunta la línea base para ese caso (vacío si no está).
esperado_de() {
    [ -f "$linea_base" ] || return 1
    awk -F'|' -v caso="$1" '$1 == caso { print $2; encontrado = 1 } END { exit !encontrado }' "$linea_base"
}

for fuente in "$@"; do
    nombre="$(dirname "$fuente")"
    nombre="$(basename "$nombre")"
    [ "$nombre" = "valido" ] && nombre="$(basename "$fuente" .oki)"

    if [ "$estricto" -eq 1 ]; then
        printf '%-34s' "$nombre"
    else
        echo "== $nombre =="
    fi

    if ! "$nooki" compilar "$fuente" --emitir-c > "$temporal/$nombre.c" 2> "$temporal/$nombre.err"; then
        if [ "$estricto" -eq 1 ]; then
            # Un caso que no compila no es un caso de memoria: eso lo mira ctest.
            echo " (no compila, se salta)"
        else
            echo "  no compila:"
            sed -n '1,4p' "$temporal/$nombre.err"
        fi
        continue
    fi

    if ! gcc -fsanitize=address,undefined -g -O0 -o "$temporal/$nombre" "$temporal/$nombre.c" 2> "$temporal/$nombre-gcc.err"; then
        if [ "$estricto" -eq 1 ]; then
            echo " (el C no compila, se salta)"
        else
            echo "  el C generado no compila:"
            sed -n '1,4p' "$temporal/$nombre-gcc.err"
        fi
        continue
    fi

    ( cd "$temporal" && "./$nombre" > /dev/null 2> "$nombre-asan.txt" )
    codigo=$?
    medidos=$((medidos + 1))

    bytes=0
    if [ -s "$temporal/$nombre-asan.txt" ]; then
        bytes=$(grep -oE "[0-9]+ byte\(s\) leaked" "$temporal/$nombre-asan.txt" |
                grep -oE "^[0-9]+" | head -1)
        bytes=${bytes:-0}
    fi

    if [ "$estricto" -eq 1 ]; then
        esperado="$(esperado_de "$nombre" 2>/dev/null | tr -d "\r")" || esperado=""
        if [ -n "$esperado" ]; then
            if [ "$esperado" = "*" ] || [ "${bytes:-0}" -le "$esperado" ]; then
                echo " dentro de la base: $bytes bytes"
            else
                echo " FUGA NUEVA: $bytes bytes (la base dice $esperado)"
                fallos=$((fallos + 1))
            fi
        elif [ "${bytes:-0}" -gt 0 ]; then
            echo " FUGA NUEVA: $bytes bytes (no está en la base)"
            fallos=$((fallos + 1))
        elif [ "$codigo" -ne 0 ]; then
            # Sin fugas y termina mal: eso no es memoria, es un fallo de verdad.
            echo " LIMPIO PERO TERMINA CON $codigo"
            fallos=$((fallos + 1))
        else
            echo " limpio"
        fi
        continue
    fi

    if [ "$codigo" -ne 0 ]; then
        echo "  termina con $codigo (¿pánico? ¿asan?)"
    fi

    if [ "$bytes" -gt 0 ]; then
        grep -E "Direct leak|SUMMARY|ERROR: AddressSanitizer" "$temporal/$nombre-asan.txt" | head -8
        # De dónde viene cada fuga, por función.
        grep -oE "in nooki_[a-zA-Z0-9_]+" "$temporal/$nombre-asan.txt" | sort | uniq -c | sort -rn | head -6
    else
        echo "  limpio"
    fi
    echo
done

if [ "$estricto" -eq 1 ]; then
    echo
    echo "medidos: $medidos · fallos: $fallos"
    [ "$fallos" -eq 0 ] || exit 1
fi
