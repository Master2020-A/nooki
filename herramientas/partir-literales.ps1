# Parte en trozos los literales gigantes del runtime, para que MSVC los acepte.
#
#   .\herramientas\partir-literales.ps1 -Comprobar   dice cómo están, no toca nada
#   .\herramientas\partir-literales.ps1              los vuelve a partir en trozos de 7 KB
#
# Por qué: MSVC (VS 17 y anteriores) rechaza los literales de cadena de más de
# 16380 caracteres con «error C2026: cadena demasiado grande». El código C que
# va dentro del compilador (interfaz e IA) vive en literales de decenas de KB, y
# C++ une los literales contiguos al traducir, así que basta con partirlos:
# mismo contenido, sin el límite por token.
#
# El contenido se escribe entre R"nooki( y )nooki" y los trozos se concatenan:
#
#   return R"nooki(
#   ...texto...
#   )nooki" R"nooki(
#   ...más texto...
#   )nooki";
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
$LIMITE = 16380

# Corta el texto por líneas en trozos de, como mucho, `Tamano` caracteres.
function Trocear {
    param([string]$Texto, [int]$Limite)
    $trozos = New-Object System.Collections.Generic.List[string]
    $actual = New-Object System.Text.StringBuilder
    foreach ($linea in ($Texto -split "`n")) {
        [void]$actual.Append($linea).Append("`n")
        if ($actual.Length -ge $Limite) {
            $trozos.Add($actual.ToString())
            [void]$actual.Clear()
        }
    }
    if ($actual.Length -gt 0) { $trozos.Add($actual.ToString()) }
    return , $trozos
}

$fallos = 0
foreach ($archivo in $Ruta) {
    if (-not (Test-Path $archivo)) { continue }
    $texto = Get-Content $archivo -Raw
    if ($texto -match "`r`n") { $texto = $texto.Replace("`r`n", "`n") }

    $desde = $texto.IndexOf($INICIO)
    $hasta = $texto.LastIndexOf($FIN)
    if ($desde -lt 0 -or $hasta -lt 0) {
        Write-Host "$archivo : no tiene literales con el delimitador nooki"
        continue
    }

    # El contenido de todos los trozos, sin los separadores.
    $cuerpo = $texto.Substring($desde + $INICIO.Length, $hasta - ($desde + $INICIO.Length))
    $cuerpo = $cuerpo.Replace($SEPARADOR, "")

    # Lo que hay ahora mismo en el archivo, para informar de la diferencia.
    $actuales = $texto.Substring($desde, $hasta - $desde) -split [regex]::Escape($SEPARADOR)
    $mayorActual = ($actuales | ForEach-Object { $_.Length } | Measure-Object -Maximum).Maximum

    $trozos = Trocear -Texto $cuerpo -Limite $Tamano
    $mayor = ($trozos | ForEach-Object { $_.Length } | Measure-Object -Maximum).Maximum

    Write-Host ("{0}: {1} trozos, el mayor de {2} caracteres (límite {3})" -f `
        $archivo, $actuales.Count, $mayorActual, $LIMITE)

    if ($mayor -gt $LIMITE) {
        Write-Host "  FALLO: no se puede bajar del límite"
        $fallos++
        continue
    }
    if ($Comprobar) { continue }

    $nuevo = New-Object System.Text.StringBuilder
    [void]$nuevo.Append($INICIO).Append("`n")
    for ($i = 0; $i -lt $trozos.Count; $i++) {
        if ($i -gt 0) { [void]$nuevo.Append($SEPARADOR).Append("`n") }
        [void]$nuevo.Append($trozos[$i])
    }
    [void]$nuevo.Append($FIN)

    $resultado = $texto.Substring(0, $desde) + $nuevo.ToString() +
        $texto.Substring($hasta + $FIN.Length)

    # Antes de escribir: el contenido debe ser exactamente el mismo. Si el
    # troceado se comiera una línea, esto lo detecta y no se toca el archivo.
    $comprobacion = $resultado.Substring($desde + $INICIO.Length)
    $comprobacion = $comprobacion.Substring(0, $comprobacion.IndexOf($FIN))
    if ($comprobacion.Replace($SEPARADOR, "") -ne $cuerpo) {
        Write-Host "  FALLO: el troceado cambiaría el contenido; no se escribe nada"
        $fallos++
        continue
    }

    Copy-Item $archivo "$archivo.antes-del-troceado" -Force
    Set-Content $archivo $resultado -Encoding utf8 -NoNewline
    Remove-Item "$archivo.antes-del-troceado" -Force
    Write-Host ("  reescrito en {0} trozos" -f $trozos.Count)
}

exit $fallos
