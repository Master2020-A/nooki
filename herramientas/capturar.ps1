# Captura la ventana de un programa de Nooki a un PNG.
#
#   .\herramientas\capturar.ps1 -Fuente pruebas\aplicaciones\tareas\principal.oki `
#                               -Titulo "Nooki - Tareas" -Salida docs\captura-tareas.png
#
# Abre el programa, espera a que aparezca la ventana por su título, la captura y
# cierra el programa. Sirve para documentar ejemplos con una imagen real.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Fuente,
    [Parameter(Mandatory = $true)][string]$Titulo,
    [Parameter(Mandatory = $true)][string]$Salida,
    [string]$Nooki = "",
    [int]$EsperaSegundos = 5
)

$ErrorActionPreference = "Stop"
$raiz = Split-Path -Parent $PSScriptRoot
Set-Location $raiz

if (-not $Nooki) {
    $Nooki = Get-ChildItem -Path "build" -Filter "nooki.exe" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $Nooki) { throw "no se encontró nooki.exe; compila antes o pasa -Nooki" }

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Ventana {
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindow(string clase, string titulo);
    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr ventana, out RECT rect);
    [DllImport("user32.dll")]
    public static extern bool PrintWindow(IntPtr ventana, IntPtr lienzo, uint opciones);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int izquierda, arriba, derecha, abajo; }
}
"@
Add-Type -AssemblyName System.Drawing

Write-Host "abriendo $Fuente…"
$proceso = Start-Process -FilePath $Nooki -ArgumentList "ejecutar", "`"$Fuente`"" -PassThru `
    -WindowStyle Hidden -RedirectStandardOutput "$env:TEMP\nooki-captura-out.txt" `
    -RedirectStandardError "$env:TEMP\nooki-captura-err.txt"

try {
    # Se busca la ventana por el título entre las ventanas visibles.
    $ventana = [IntPtr]::Zero
    $duenio = $null
    for ($intento = 0; $intento -lt ($EsperaSegundos * 10); $intento++) {
        Start-Sleep -Milliseconds 100
        $duenio = Get-Process | Where-Object { $_.MainWindowTitle -eq $Titulo } |
            Select-Object -First 1
        if ($duenio) {
            $ventana = $duenio.MainWindowHandle
            break
        }
    }
    if ($ventana -eq [IntPtr]::Zero) {
        throw "no apareció una ventana con el título «$Titulo»"
    }

    Start-Sleep -Milliseconds 500
    $rect = New-Object Ventana+RECT
    [void][Ventana]::GetClientRect($ventana, [ref]$rect)
    $ancho = $rect.derecha - $rect.izquierda
    $alto = $rect.abajo - $rect.arriba
    if ($ancho -le 0 -or $alto -le 0) { throw "la ventana no tiene tamaño" }

    $imagen = New-Object System.Drawing.Bitmap $ancho, $alto
    $lienzo = [System.Drawing.Graphics]::FromImage($imagen)
    $dc = $lienzo.GetHdc()
    # 2 = PW_RENDERFULLCONTENT: captura también lo dibujado con GDI.
    [void][Ventana]::PrintWindow($ventana, $dc, 2)
    $lienzo.ReleaseHdc($dc)
    $lienzo.Dispose()

    $carpeta = Split-Path -Parent $Salida
    if ($carpeta -and -not (Test-Path $carpeta)) { New-Item -ItemType Directory -Force -Path $carpeta | Out-Null }
    $imagen.Save($Salida, [System.Drawing.Imaging.ImageFormat]::Png)
    $imagen.Dispose()

    $tamano = [math]::Round((Get-Item $Salida).Length / 1KB, 1)
    Write-Host "captura guardada: $Salida ($ancho x $alto, $tamano KB)"
}
finally {
    if ($duenio) { Stop-Process -Id $duenio.Id -Force -ErrorAction SilentlyContinue }
    if (-not $proceso.HasExited) { Stop-Process -Id $proceso.Id -Force }
    Remove-Item "$env:TEMP\nooki-captura-out.txt", "$env:TEMP\nooki-captura-err.txt" -ErrorAction SilentlyContinue
}
