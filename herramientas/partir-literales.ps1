# Parte en trozos los literales gigantes del runtime, para que MSVC los acepte.
#
#   .\herramientas\partir-literales.ps1 -Ruta compilador\fuente\ia\runtime_ia.cpp
#   .\herramientas\partir-literales.ps1 -Comprobar     solo informa, no escribe
#
# MSVC (al menos hasta VS 17) rechaza los literales de cadena de más de 16 KB
# con «error C2026: cadena demasiado grande». Los runtimes de interfaz e IA
# viven en literales de decenas de KB y C++ los une al traducir, así que basta
# con partirlos en trozos de 7 KB: mismo contenido, sin el límite por token.
#
# Se apoya en el delimitador `nooki`: el contenido de un literal se escribe
#   R"nooki(
#   ...texto...
#   )nooki"
# y varios seguidos se concatenan al compilar.
[CmdletBinding()]
param(
    [string[]]$Ruta = @(
        "compilador/fuente/ia/runtime_ia.cpp",
        "compilador/fuente/ui/runtime_ui.cpp"
    ),
    [switch]$Comprobar,
    [int]$Tamano = 7000
)

$ErrorActionPreference = "Stop"
$raiz = Split-Path -Parent $PSScriptRoot
Set-Location $raiz

$INICIO = 'return R"nooki('
$SEPARADOR = ')nooki" R"nooki('
$FIN = ')nooki";'

foreach ($archivo in $Ruta) {
    if (-not (Test-Path $archivo)) { continue }
    $t = Get-Content $archivo -Raw
    if ($t -match "`r`n") { $t = $t.Replace("`r`n", "`n") }

    $posInicio = $t.IndexOf($INICIO)
    $posFin = $t.LastIndexOf($FIN)
    if ($posInicio -lt 0 -or $posFin -lt 0) {
        Write-Host "$archivo : no tiene literales con el delimitador nooki"
        continue
    }

    $desde = $posInicio + $INICIO.Length
    $cuerpo = $t.Substring($desde, $posFin - $desde)
    # Quitar los separadores y la indentación que añade cada trozo.
    $cuerpo = $cuerpo.Replace($SEPARADOR, "")
    $trozos = @()

    $lineas = $cuerpo -split "`n"
    $actual = New-Object System.Text.StringBuilder
    foreach ($linea in $lineas) {
        [void]$actual.Append($linea).Append("`n")
        if ($actual.Length -ge $Tamano) {
            $trozos += $actual.ToString()
            $actual = New-Object System.Text.StringBuilder
        }
    }
    if ($actual.Length -gt 0) { $trozos += $actual.ToString() }

    # Lo que hay ahora en el archivo, para poder compararlo con lo que saldría.
    $actuales = $t.Substring($posInicio, $posFin - $posInicio) -split [regex]::Escape($SEPARADOR)
    $maximoActual = ($actuales | ForEach-Object { $_.Length } | Measure-Object -Maximum).Maximum
    $maximo = ($trozos | ForEach-Object { $_.Length } | Measure-Object -Maximum).Maximum
    Write-Host ("{0}: ahora {1} trozos de hasta {2} caracteres" -f $archivo, $actuales.Count, $maximoActual)
    if ($maximoActual -gt 16380) {
        Write-Host ("  el mayor pasa del límite de MSVC (16380): hay que partirlo")
    }

    if ($Comprobar) { continue }
    if ($maximoActual -le 16380) {
        Write-Host "  no hace falta tocarlo"
        continue
    }

    $nuevo = New-Object System.Text.StringBuilder
    [void]$nuevo.Append($INICIO).Append("`n")
    for ($i = 0; $i -lt $trozos.Count; $i++) {
        if ($i -gt 0) { [void]$nuevo.Append($SEPARADOR).Append("`n") }
        [void]$nuevo.Append($trozos[$i])
    }
    [void]$nuevo.Append($FIN)

    $resultado = $t.Substring(0, $posInicio) + $nuevo.ToString() + $t.Substring($posFin + $FIN.Length)
    Set-Content $archivo $resultado -Encoding utf8 -NoNewline
    Write-Host "  reescrito"
}

if ($Comprobar) { Write-Host "solo comprobación: no se modificó nada" }
