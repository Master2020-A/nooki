#!/usr/bin/env bash
# Verifica Nooki en macOS. Un solo comando, para quien tenga un Mac a mano.
#
#   ./herramientas/verificar-macos.sh
#
# Compila, pasa las pruebas y después comprueba a mano las cuatro cosas que en
# macOS no ejercita CI: lanzar un programa, los argumentos, la biblioteca de
# datos y la interfaz. Tarda unos diez minutos.
#
# Sale con 0 si todo va bien y con 1 si algo falla, para poder pegarlo en un
# informe sin interpretarlo.

set -u

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$raiz" || exit 1

fallos=0
bien() { echo "  bien: $*"; }
mal() { echo "  FALLO: $*"; fallos=$((fallos + 1)); }

echo "== Sistema =="
sw_vers 2>/dev/null || true
uname -m
clang --version | head -1
cmake --version | head -1

echo
echo "== Compilar =="
if cmake --preset macos > /tmp/nooki-config.log 2>&1 &&
   cmake --build --preset macos -j "$(sysctl -n hw.ncpu)" > /tmp/nooki-build.log 2>&1; then
    bien "compila con el preset macos"
else
    mal "no compila; vea /tmp/nooki-build.log"
    tail -30 /tmp/nooki-build.log
    exit 1
fi

nooki="build/macos/compilador/nooki"
if [ ! -x "$nooki" ]; then
    mal "no se generó $nooki"
    exit 1
fi

echo
echo "== Pruebas del proyecto =="
if ctest --preset macos > /tmp/nooki-ctest.log 2>&1; then
    grep "tests passed" /tmp/nooki-ctest.log || true
    bien "las 18 suites pasan"
else
    mal "algunas suites fallan; vea /tmp/nooki-ctest.log"
    grep -E "Failed|FALLO" /tmp/nooki-ctest.log | head -20
fi

echo
echo "== Lo que CI no toca en macOS =="
temporal="$(mktemp -d)"
cd "$temporal" || exit 1

# 1. Compilar y lanzar: en Unix el binario se lanza con «./», que es justo lo que
#    se arregló en la 1.0.5 y solo se ha probado en Linux.
cat > saluda.oki <<'OKI'
usar textos

funcion principal() -> entero {
    imprimir("hola desde macOS")
    retornar 0
}
OKI
if "$raiz/$nooki" ejecutar saluda.oki > salida.txt 2>&1 &&
   grep -q "hola desde macOS" salida.txt; then
    bien "«ejecutar» compila y lanza el binario"
else
    mal "«ejecutar» no lanzó el binario: $(head -3 salida.txt)"
fi

# 2. Argumentos del programa.
cat > args.oki <<'OKI'
usar sistema
usar textos

funcion principal() -> entero {
    imprimir("cuántos: " + textos.de_entero(sistema.cuantos_argumentos()))
    si (sistema.cuantos_argumentos() > 1) {
        imprimir(sistema.argumento(1))
    }
    retornar 0
}
OKI
if "$raiz/$nooki" ejecutar args.oki uno "dos palabras" > args.txt 2>&1 &&
   grep -q "cuántos: 2" args.txt && grep -q "dos palabras" args.txt; then
    bien "los argumentos llegan enteros, con espacios"
else
    mal "los argumentos no llegan: $(head -3 args.txt)"
fi

# 3. La biblioteca de datos: dlopen de libduckdb.dylib.
if [ -f "$raiz/build/duckdb/libs122/libduckdb.dylib" ]; then
    cat > datos.oki <<'OKI'
usar datos
usar textos

funcion principal() -> entero {
    si (!datos.disponible()) {
        imprimir("sin motor: " + datos.error())
        retornar 1
    }
    variable t = datos.tabla(&"SELECT 21 * 2 AS respuesta" + "")
    si (!datos.hay(t)) {
        imprimir("sin tabla: " + datos.error())
        retornar 1
    }
    imprimir(textos.de_entero(datos.entero_de(t, 0, 0)))
    datos.liberar(t)
    retornar 0
}
OKI
    if "$raiz/$nooki" ejecutar datos.oki > datos.txt 2>&1 && grep -q "42" datos.txt; then
        bien "DuckDB se carga con dlopen y consulta"
    else
        mal "DuckDB no funciona: $(head -3 datos.txt)"
    fi
else
    echo "  (salteado: no está libduckdb.dylib en build/duckdb/libs122)"
fi

# 4. La interfaz: en macOS el backend es un stub, así que debe decir que no hay.
if [ -f "$raiz/ejemplos/05-ventana/principal.oki" ]; then
    cat > ventana.oki <<'OKI'
usar ventana

funcion principal() -> entero {
    variable titulo = "prueba" + ""
    variable v = ventana.abrir(&titulo, 200, 100)
    si (v == 0) {
        imprimir("sin entorno grafico")
        retornar 0
    }
    ventana.cerrar(v)
    retornar 0
}
OKI
    if "$raiz/$nooki" ejecutar ventana.oki > ventana.txt 2>&1; then
        if grep -q "sin entorno grafico" ventana.txt; then
            bien "«ventana» compila y dice que no hay backend (lo esperado en macOS)"
        else
            bien "«ventana» compila y abre ventana (¿hay backend nuevo?)"
        fi
    else
        mal "«ventana» no compila: $(head -3 ventana.txt)"
    fi
fi

cd "$raiz" || exit 1
rm -rf "$temporal"

echo
if [ "$fallos" -eq 0 ]; then
    echo "TODO BIEN: macOS verificado."
    exit 0
fi
echo "FALLOS: $fallos. Copie esta salida en el informe."
exit 1
