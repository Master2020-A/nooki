#!/usr/bin/env bash
# Empaqueta Nooki para distribuirlo: binario + documentación + ejemplos + editores.
#
#   ./herramientas/empaquetar.sh              # con el preset del sistema
#   ./herramientas/empaquetar.sh linux        # con otro preset
#
# El resultado queda en dist/nooki-<versión>-<plataforma>.tar.gz
set -euo pipefail

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$raiz"

case "$(uname -s)" in
    Linux*)  preset_por_defecto="linux"; plataforma="linux-x86_64" ;;
    Darwin*) preset_por_defecto="macos"; plataforma="macos-x86_64" ;;
    *)       preset_por_defecto="linux"; plataforma="desconocida" ;;
esac

preset="${1:-$preset_por_defecto}"

echo "compilando con el preset $preset…"
cmake --preset "$preset" > /dev/null
cmake --build --preset "$preset" > /dev/null

binario="build/$preset/compilador/nooki"
if [ ! -x "$binario" ]; then
    echo "no se encontró el binario en build/$preset/compilador" >&2
    exit 1
fi

version="$("$binario" --version | sed -n 's/^nooki \([^ ]*\).*/\1/p')"
if [ -z "$version" ]; then
    echo "no se pudo leer la versión del binario" >&2
    exit 1
fi

nombre="nooki-$version-$plataforma"
destino="dist/$nombre"
echo "empaquetando $nombre…"

rm -rf "$destino"
mkdir -p "$destino/spec"

cp "$binario" "$destino/nooki"
for documento in LEEME.md GUIA.md MANUAL.md REFERENCIA.md NOTAS_DE_VERSION.md CHANGELOG.md \
                 DECISIONES.md LICENCIA.md LICENCIA-MIT.txt LICENCIA-APACHE.txt CONTRIBUIR.md \
                 PUBLICAR.md; do
    [ -f "$documento" ] && cp "$documento" "$destino/"
done

# Las guías por audiencia y sus PDF, si están.
if [ -d docs/guias ]; then
    mkdir -p "$destino/guias"
    cp docs/guias/*.md "$destino/guias/" 2>/dev/null || true
    cp docs/guias/*.pdf "$destino/guias/" 2>/dev/null || true
fi
for pdf in GUIA_DEL_USUARIO.pdf GUIA_DEL_ADMINISTRADOR.pdf GUIA_DEL_DESARROLLADOR.pdf \
           REFERENCIA_DEL_LENGUAJE.pdf; do
    [ -f "$pdf" ] && cp "$pdf" "$destino/guias/"
done

cp -r ejemplos editores "$destino/"
# El instalador, como en el paquete de Windows.
mkdir -p "$destino/herramientas"
cp herramientas/instalar.ps1 "$destino/herramientas/" 2>/dev/null || true
cp spec/especificacion-v0.1.md "$destino/spec/"

cat > "$destino/INSTALAR.txt" <<TEXTO
Nooki $version ($plataforma)
============================

1. Copia esta carpeta donde quieras.
2. Añade al PATH la ruta de esta carpeta:

     echo "export PATH=\"\$PWD:\$PATH\"" >> ~/.bashrc

3. Comprueba la instalación:

     nooki --version
     nooki ejecutar ejemplos/01-hola/principal.oki

Necesita un compilador de C en el PATH (cc, gcc o clang): Nooki genera C y lo
entrega a ese compilador.

La guía completa está en GUIA.md.
TEXTO

cd dist
tar -czf "$nombre.tar.gz" "$nombre"
cd "$raiz"

tamano="$(du -h "dist/$nombre.tar.gz" | cut -f1)"
echo "listo: dist/$nombre.tar.gz ($tamano)"
