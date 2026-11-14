# Captura la ventana de un IDE a un PNG.
#
#   .\capturar-ide.ps1 -Titulo "demo.oki" -Salida docs\captura-colores.png
#
# Sirve para documentar con la ventana real. Necesita que la ventana esté
# abierta: este script solo la busca por su título, la trae al frente y la
# fotografía.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Titulo,
    [Parameter(Mandatory = $true)][string]$Salida,
    [int]$EsperaSegundos = 3
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Ventana {
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
"@

$proceso = Get-Process | Where-Object { $_.MainWindowTitle -like "*$Titulo*" } | Select-Object -First 1
if (-not $proceso) { throw "no hay ninguna ventana con «$Titulo» en el título" }

$handle = $proceso.MainWindowHandle
[void][Ventana]::ShowWindow($handle, 9)      # SW_RESTORE
[void][Ventana]::SetForegroundWindow($handle)
Start-Sleep -Seconds $EsperaSegundos

$rect = New-Object Ventana+RECT
if (-not [Ventana]::GetWindowRect($handle, [ref]$rect)) { throw "no se pudo medir la ventana" }
$ancho = $rect.Right - $rect.Left
$alto = $rect.Bottom - $rect.Top
if ($ancho -le 0 -or $alto -le 0) { throw "la ventana no tiene tamaño" }

$imagen = New-Object System.Drawing.Bitmap $ancho, $alto
$lienzo = [System.Drawing.Graphics]::FromImage($imagen)
$lienzo.CopyFromScreen($rect.Left, $rect.Top, 0, 0, $imagen.Size)
$lienzo.Dispose()

$carpeta = Split-Path -Parent $Salida
if ($carpeta -and -not (Test-Path $carpeta)) { New-Item -ItemType Directory -Force -Path $carpeta | Out-Null }
$imagen.Save((Join-Path (Get-Location) $Salida), [System.Drawing.Imaging.ImageFormat]::Png)
$imagen.Dispose()

Write-Host "captura: $Salida ($ancho x $alto)"
