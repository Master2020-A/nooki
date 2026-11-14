# Comprueba que el plugin carga en RustRover.
#
#   .\probar-carga.ps1
#
# Arranca un RustRover con configuración, sistema y plugins en una carpeta
# temporal —no toca la instalación del usuario—, espera a que arranque, mira el
# registro y cierra. Sirve para saber si el plugin se carga o si el IDE lo
# rechaza, sin abrir el IDE de verdad.
[CmdletBinding()]
param(
    [string]$Ide = "",
    [int]$EsperaSegundos = 90
)

$ErrorActionPreference = "Stop"
$aqui = Split-Path -Parent $MyInvocation.MyCommand.Path
$jar = Join-Path (Split-Path -Parent $aqui) "Nooki-1.0.0.jar"
if (-not (Test-Path $jar)) { throw "falta $jar; ejecuta compilar.ps1 antes" }

if (-not $Ide) {
    $Ide = (Get-ChildItem "C:\Program Files\JetBrains" -Directory |
        Where-Object { $_.Name -match 'RustRover' } |
        Sort-Object -Property { [version]($_.Name -replace '[^\d.]', '') } -Descending |
        Select-Object -First 1).FullName
}
if (-not $Ide) { throw "no se encontró RustRover" }
Write-Host "ide: $Ide"

$ejecutable = Join-Path $Ide "bin\rustrover64.exe"
if (-not (Test-Path $ejecutable)) {
    $ejecutable = Join-Path $Ide "bin\rustrover.exe"
}
if (-not (Test-Path $ejecutable)) { throw "no se encontró el ejecutable del IDE" }

# Un entorno aparte: nada de lo que pase aquí afecta a la configuración real.
$base = Join-Path ([System.IO.Path]::GetTempPath()) "nooki-prueba-plugin"
if (Test-Path $base) { Remove-Item -Recurse -Force $base }
foreach ($carpeta in @("config", "system", "plugins", "log")) {
    New-Item -ItemType Directory -Force -Path (Join-Path $base $carpeta) | Out-Null
}
Copy-Item $jar (Join-Path $base "plugins")

$argumentos = @(
    "-Didea.config.path=$base\config",
    "-Didea.system.path=$base\system",
    "-Didea.plugins.path=$base\plugins",
    "-Didea.log.path=$base\log",
    "-Didea.is.internal=false",
    "-Dnosplash"
)
Write-Host "arrancando con el entorno aislado…"
$proceso = Start-Process -FilePath $ejecutable -ArgumentList $argumentos -PassThru
Start-Sleep -Seconds $EsperaSegundos
if (-not $proceso.HasExited) { Stop-Process -Id $proceso.Id -Force }
Get-Process -Name 'rustrover64','rustrover' -ErrorAction SilentlyContinue |
    Where-Object { $_.Id -ne $proceso.Id } | Stop-Process -Force -ErrorAction SilentlyContinue

$log = Join-Path $base "log\idea.log"
if (-not (Test-Path $log)) {
    Write-Host "no se generó el registro: el IDE no llegó a arrancar"
    exit 1
}

$contenido = Get-Content $log
$problemas = $contenido | Select-String -Pattern 'Nooki' | Select-String -Pattern 'ERROR|Error|error|failed|Failed|no se pudo'
$cargado = $contenido | Select-String -Pattern 'Nooki' | Select-Object -First 6

Write-Host ""
Write-Host "--- líneas del registro que mencionan Nooki ---"
if ($cargado) { $cargado | ForEach-Object { Write-Host "  $($_.Line.Trim())" } }
else { Write-Host "  (ninguna: el plugin no se cargó)" }

Write-Host ""
if ($problemas) {
    Write-Host "--- errores ---"
    $problemas | ForEach-Object { Write-Host "  $($_.Line.Trim())" }
    Write-Host ""
    Write-Host "el registro completo está en $log"
    exit 1
}
if (-not $cargado) {
    Write-Host "el registro no menciona el plugin; revisa $log"
    exit 1
}
Write-Host "el plugin se cargó sin errores (registro en $log)"
