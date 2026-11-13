# Empaqueta Nooki para distribuirlo: binario + documentación + ejemplos + editores.
#
#   .\herramientas\empaquetar.ps1                 # compila si hace falta y empaqueta
#   .\herramientas\empaquetar.ps1 -Preset linux   # con otro preset
#
# El resultado queda en dist/nooki-<versión>-<plataforma>.zip
[CmdletBinding()]
param(
    [string]$Preset = "windows-mingw",
    [switch]$SaltarCompilacion
)

$ErrorActionPreference = "Stop"
$raiz = Split-Path -Parent $PSScriptRoot
Set-Location $raiz

if (-not $SaltarCompilacion) {
    Write-Host "compilando con el preset $Preset…"
    cmake --preset $Preset | Out-Null
    cmake --build --preset $Preset | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "la compilación falló" }
}

# El binario y la plataforma según el preset.
$carpeta = "build/$Preset/compilador"
$binario = if (Test-Path "$carpeta/nooki.exe") { "$carpeta/nooki.exe" } else { "$carpeta/nooki" }
if (-not (Test-Path $binario)) { throw "no se encontró el binario en $carpeta" }

$plataforma = switch -Wildcard ($Preset) {
    "windows*" { "windows-x86_64" }
    "linux*"   { "linux-x86_64" }
    "macos*"   { "macos-x86_64" }
    default    { "desconocida" }
}

$version = (& $binario --version) -replace '^nooki (\S+).*$', '$1'
if (-not $version) { throw "no se pudo leer la versión del binario" }

$nombre = "nooki-$version-$plataforma"
$destino = "dist/$nombre"
Write-Host "empaquetando $nombre…"

if (Test-Path $destino) { Remove-Item -Recurse -Force $destino }
New-Item -ItemType Directory -Force -Path $destino | Out-Null

# Binario y documentación.
Copy-Item $binario "$destino/nooki$(if ($binario.EndsWith('.exe')) { '.exe' })"
$documentos = @(
    "LEEME.md", "GUIA.md", "NOTAS_DE_VERSION.md", "CHANGELOG.md", "DECISIONES.md",
    "LICENCIA.md", "LICENCIA-MIT.txt", "LICENCIA-APACHE.txt", "CONTRIBUIR.md", "PUBLICAR.md"
)
foreach ($documento in $documentos) {
    if (Test-Path $documento) { Copy-Item $documento $destino }
}

# Ejemplos, editores y especificación.
Copy-Item -Recurse "ejemplos" "$destino/ejemplos"
Copy-Item -Recurse "editores" "$destino/editores"
New-Item -ItemType Directory -Force -Path "$destino/spec" | Out-Null
Copy-Item "spec/especificacion-v0.1.md" "$destino/spec"

# Instrucciones de instalación.
@"
Nooki $version ($plataforma)
============================

1. Copia esta carpeta donde quieras.
2. Añade al PATH la ruta de esta carpeta:

   Windows (PowerShell):
     [Environment]::SetEnvironmentVariable(
         "Path", `$env:Path + ";`$(Resolve-Path .)", "User")

   Linux y macOS:
     echo "export PATH=`"`$PWD:`$PATH`"" >> ~/.bashrc

3. Comprueba la instalación:

     nooki --version
     nooki ejecutar ejemplos/01-hola/principal.oki

Necesita un compilador de C en el PATH (cc, gcc, clang o MSVC): Nooki
genera C y lo entrega a ese compilador.

La guía completa está en GUIA.md.
"@ | Set-Content "$destino/INSTALAR.txt" -Encoding utf8

# Comprimir.
$zip = "dist/$nombre.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path $destino -DestinationPath $zip -CompressionLevel Optimal

$tamano = [math]::Round((Get-Item $zip).Length / 1MB, 1)
Write-Host "listo: $zip ($tamano MB)"
Write-Host "  binario, $($documentos.Count) documentos, ejemplos, editores y especificación"
