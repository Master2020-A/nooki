# Instala Nooki en el PATH del usuario, para poder escribir `nooki` desde cualquier carpeta.
#
#   .\herramientas\instalar.ps1              instala el binario ya compilado
#   .\herramientas\instalar.ps1 -Compilar    compila antes de instalar
#   .\herramientas\instalar.ps1 -Desinstalar lo quita del PATH y borra los archivos
#
# Deja el ejecutable en %LOCALAPPDATA%\Nooki\bin y añade esa carpeta al PATH del
# usuario. No toca el PATH del sistema ni pide permisos de administrador.
[CmdletBinding()]
param(
    [switch]$Compilar,
    [switch]$Desinstalar
)

$ErrorActionPreference = "Stop"
$raiz = Split-Path -Parent $PSScriptRoot
Push-Location $raiz

$destino = Join-Path $env:LOCALAPPDATA "Nooki\bin"
$clave = "HKCU:\Environment"

function Leer-PathUsuario {
    $item = Get-Item -Path $clave
    return [string]$item.GetValue("Path", "", "DoNotExpandEnvironmentNames")
}

function Escribir-PathUsuario {
    param([string]$Valor)
    $item = Get-Item -Path $clave
    $tipo = $item.GetValueKind("Path")
    if ($Valor.Length -gt 32000) {
        throw "el PATH del usuario se quedaría demasiado largo"
    }
    Set-ItemProperty -Path $clave -Name "Path" -Value $Valor -Type $tipo
}

function Anunciar {
    param([string]$Texto)
    Write-Host $Texto
}

# --- Desinstalar -------------------------------------------------------------
if ($Desinstalar) {
    $actual = Leer-PathUsuario
    $partes = $actual -split ';' | Where-Object { $_ -and $_ -ne $destino }
    Escribir-PathUsuario (($partes) -join ';')
    if (Test-Path (Join-Path $env:LOCALAPPDATA "Nooki")) {
        Remove-Item -Recurse -Force (Join-Path $env:LOCALAPPDATA "Nooki")
    }
    Anunciar "Nooki desinstalado: se quitó $destino del PATH y se borraron los archivos."
    Anunciar "Cierra y vuelve a abrir la terminal para que el cambio surta efecto."
    exit 0
}

# --- Compilar si se pide -----------------------------------------------------
if ($Compilar) {
    $preset = "windows-mingw"
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        throw "hace falta cmake para compilar; instálalo o usa el binario ya compilado"
    }
    Anunciar "compilando ($preset)…"
    & cmake --preset $preset | Out-Null
    & cmake --build --preset $preset
    if ($LASTEXITCODE -ne 0) { throw "la compilación falló" }
}

# --- Buscar el binario -------------------------------------------------------
# Vale tanto el repositorio (con su build) como la distribución, donde el
# binario está en `bin/nooki.exe` junto a este script.
$candidatos = @(
    (Join-Path $PSScriptRoot "nooki.exe"),
    (Join-Path $PSScriptRoot "..\bin\nooki.exe"),
    (Join-Path $PSScriptRoot "..\nooki.exe"),
    "build\windows-mingw\compilador\nooki.exe",
    "build\windows-msvc\compilador\Release\nooki.exe",
    "build\windows-msvc-2026\compilador\Release\nooki.exe"
)
$binario = $null
foreach ($candidato in $candidatos) {
    if (Test-Path $candidato) {
        $binario = (Resolve-Path $candidato).Path
        break
    }
}
if (-not $binario) {
    $binario = Get-ChildItem "build" -Recurse -Filter "nooki.exe" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName
}
if (-not $binario) {
    throw "no se encontró nooki.exe; compila antes (.\herramientas\instalar.ps1 -Compilar)"
}
Anunciar "binario: $binario"

# --- Copiar a una carpeta estable -------------------------------------------
New-Item -ItemType Directory -Force -Path $destino | Out-Null
Copy-Item $binario (Join-Path $destino "nooki.exe") -Force
Anunciar "instalado en: $destino"

# --- Añadir al PATH del usuario ---------------------------------------------
$actual = Leer-PathUsuario
$partes = @($actual -split ';' | Where-Object { $_ })
if ($partes -contains $destino) {
    Anunciar "ya estaba en el PATH"
} else {
    Escribir-PathUsuario (($partes + $destino) -join ';')
    Anunciar "añadido al PATH del usuario"
}

# --- Comprobar que el binario responde --------------------------------------
$version = & (Join-Path $destino "nooki.exe") version
Anunciar "comprobación: $version"

Anunciar ""
Anunciar "Listo. Cierra y abre la terminal (el PATH no cambia en la sesión abierta),"
Anunciar "y después:"
Anunciar ""
Anunciar "    nooki version"
Anunciar "    nooki nuevo demo"
Anunciar "    cd demo"
Anunciar "    nooki construir"
