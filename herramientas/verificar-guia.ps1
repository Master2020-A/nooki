# Comprueba los ejemplos de una guía, envolviendo los fragmentos.
#
#   .\herramientas\verificar-guia.ps1 -Documento docs\guias\GUIA_DEL_USUARIO.md
#
# Una guía no está hecha solo de programas completos: hay fragmentos (`variable x = 1`,
# una función sin `principal`, una estructura suelta). Este script prueba cada
# bloque de tres formas y solo canta cuando **ninguna** compila, que es cuando hay
# un error de verdad: una función que no existe, un argumento de más, un tipo mal.
#
#   completa   el bloque ya es un programa
#   envuelta   se mete dentro de `funcion principal`
#   con base   se le añade un `principal` vacío al final
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string[]]$Documento,
    [string]$Nooki = "",
    [switch]$SoloFallos
)

$ErrorActionPreference = "Stop"
$raiz = Split-Path -Parent $PSScriptRoot
Set-Location $raiz

if (-not $Nooki) {
    $Nooki = Get-ChildItem -Path "build" -Filter "nooki.exe" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $Nooki) { throw "no se encontró nooki.exe" }

$destino = Join-Path ([System.IO.Path]::GetTempPath()) "nooki-guia"
New-Item -ItemType Directory -Force -Path $destino | Out-Null

function Bloques {
    param([string]$Ruta)
    $salida = @()
    $enBloque = $false
    $inicio = 0
    $lineas = @()
    $numero = 0
    foreach ($linea in (Get-Content $Ruta)) {
        $numero++
        if (-not $enBloque -and $linea -match '^```nooki\s*$') {
            $enBloque = $true
            $inicio = $numero + 1
            $lineas = @()
            continue
        }
        if ($enBloque -and $linea -match '^```\s*$') {
            $enBloque = $false
            $salida += [pscustomobject]@{ Linea = $inicio; Codigo = ($lineas -join "`n") }
            continue
        }
        if ($enBloque) { $lineas += $linea }
    }
    return $salida
}

function Compila {
    param([string]$Codigo, [string]$Nombre)
    $archivo = Join-Path $destino "$Nombre.oki"
    [System.IO.File]::WriteAllText($archivo, ($Codigo + "`n"), (New-Object System.Text.UTF8Encoding $false))
    # El compilador escribe los errores por la salida de error, y con
    # `ErrorActionPreference = Stop` eso abortaría el script: se baja mientras dura.
    $previo = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $salida = & $Nooki verificar $archivo 2>&1
    $codigo = $LASTEXITCODE
    $ErrorActionPreference = $previo
    if ($codigo -eq 0) { return $null }
    return (($salida | Where-Object { $_ -match '^\s*error\[' } | Select-Object -First 1) -join '')
}

$total = 0
$fallos = 0
foreach ($doc in $Documento) {
    Write-Host "== $doc"
    foreach ($bloque in (Bloques -Ruta $doc)) {
        $codigo = $bloque.Codigo
        # Bloques que enseñan un error a propósito: su fallo es el mensaje.
        if ($codigo -match '//\s*error\[NOK') {
            if (-not $SoloFallos) { Write-Host "  (error a propósito) línea $($bloque.Linea)" }
            continue
        }
        if ($codigo -match '//\s*\.\.\.') {
            if (-not $SoloFallos) { Write-Host "  (ilustrativo)   línea $($bloque.Linea)" }
            continue
        }
        $total++
        $nombre = "b$total"

        $fallo = Compila -Codigo $codigo -Nombre $nombre
        if (-not $fallo) {
            if (-not $SoloFallos) { Write-Host "  ok (completa)   línea $($bloque.Linea)" }
            continue
        }

        $envuelto = "funcion principal() -> entero {`n" + (($codigo -split "`n" | ForEach-Object { "    $_" }) -join "`n") + "`n    retornar 0`n}"
        $fallo2 = Compila -Codigo $envuelto -Nombre "${nombre}w"
        if (-not $fallo2) {
            if (-not $SoloFallos) { Write-Host "  ok (envuelta)   línea $($bloque.Linea)" }
            continue
        }

        $conBase = $codigo + "`n`nfuncion principal() -> entero {`n    retornar 0`n}"
        $fallo3 = Compila -Codigo $conBase -Nombre "${nombre}b"
        if (-not $fallo3) {
            if (-not $SoloFallos) { Write-Host "  ok (con base)   línea $($bloque.Linea)" }
            continue
        }

        # Se informa del error del bloque tal cual, que es el más informativo.
        $fallos++
        Write-Host "  FALLA           línea $($bloque.Linea)" -ForegroundColor Yellow
        Write-Host "     $fallo"
        Write-Host "     --- el bloque ---"
        ($codigo -split "`n" | Select-Object -First 6) | ForEach-Object { Write-Host "     $_" }
    }
}

Write-Host ""
Write-Host "$total bloques, $fallos sin compilar de ninguna forma"
if ($fallos -gt 0) { exit 1 }
