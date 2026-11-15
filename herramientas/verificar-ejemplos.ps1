# Extrae los bloques ```nooki de un documento y compila cada uno.
#
#   .\herramientas\verificar-ejemplos.ps1 -Documento MANUAL.md
#
# Cada bloque debe ser un programa completo. Sirve para que la documentación no
# se desincronice del lenguaje.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string[]]$Documento,
    [string]$Nooki = ""
)

$ErrorActionPreference = "Stop"
$raiz = Split-Path -Parent $PSScriptRoot
Set-Location $raiz

if (-not $Nooki) {
    $Nooki = Get-ChildItem -Path "build" -Filter "nooki.exe" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $Nooki) { throw "no se encontró nooki.exe" }

$lineas = @()
foreach ($uno in $Documento) { $lineas += Get-Content $uno }
$bloques = @()
$actual = $null
$enBloque = $false
$numero = 0
$inicio = 0

for ($i = 0; $i -lt $lineas.Count; $i++) {
    $linea = $lineas[$i]
    if (-not $enBloque -and $linea -match '^```nooki\s*$') {
        $actual = New-Object System.Collections.Generic.List[string]
        $enBloque = $true
        $inicio = $i + 1
        continue
    }
    if ($enBloque -and $linea -match '^```\s*$') {
        $enBloque = $false
        $numero++
        $bloques += [pscustomobject]@{ Numero = $numero; Linea = $inicio; Codigo = ($actual -join "`n") }
        $actual = $null
        continue
    }
    if ($enBloque) { $actual.Add($linea) }
}

$destino = Join-Path ([System.IO.Path]::GetTempPath()) "nooki-ejemplos"
New-Item -ItemType Directory -Force -Path $destino | Out-Null

$fallos = 0
foreach ($bloque in $bloques) {
    $archivo = Join-Path $destino "ejemplo-$($bloque.Numero).oki"
    # Sin BOM: Nooki rechaza el U+FEFF al principio del archivo.
    [System.IO.File]::WriteAllText($archivo, $bloque.Codigo + "`n", (New-Object System.Text.UTF8Encoding $false))
    $salida = & $Nooki verificar $archivo 2>&1
    if ($LASTEXITCODE -ne 0) {
        $fallos++
        Write-Host "FALLO el ejemplo $($bloque.Numero) (línea $($bloque.Linea)):" -ForegroundColor Red
        $salida | Select-Object -First 4 | ForEach-Object { Write-Host "  $_" }
    }
}

Write-Host "$($bloques.Count) ejemplos revisados, $fallos con problemas"
if ($fallos -gt 0) { exit 1 }
