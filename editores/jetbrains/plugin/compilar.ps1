# Compila el plugin de Nooki para JetBrains y deja el .jar en la raíz de esta carpeta.
#
#   .\compilar.ps1
#   .\compilar.ps1 -Ide "C:\Program Files\JetBrains\IntelliJ IDEA 2025.3"
#
# No necesita Gradle ni descargar nada: compila contra las bibliotecas del IDE que
# ya tengas instalado.
[CmdletBinding()]
param(
    [string]$Ide = ""
)

$ErrorActionPreference = "Stop"
$aqui = Split-Path -Parent $MyInvocation.MyCommand.Path

# 1. Encontrar un IDE de JetBrains instalado.
if (-not $Ide) {
    $candidatos = @()
    foreach ($base in @("C:\Program Files\JetBrains", "$env:LOCALAPPDATA\Programs")) {
        if (Test-Path $base) {
            $candidatos += Get-ChildItem $base -Directory -ErrorAction SilentlyContinue |
                Where-Object {
                    Test-Path (Join-Path $_.FullName "lib") -PathType Container
                }
        }
    }
    # Preferimos RustRover y, entre varios, el más nuevo.
    $elegido = $candidatos | Where-Object { $_.Name -match 'RustRover' } |
        Sort-Object -Property { [version]($_.Name -replace '[^\d.]', '') } -Descending |
        Select-Object -First 1
    if (-not $elegido) {
        $elegido = $candidatos |
            Sort-Object -Property { [version]($_.Name -replace '[^\d.]', '') } -Descending |
            Select-Object -First 1
    }
    if (-not $elegido) { throw "no se encontró ningún IDE de JetBrains; pasa -Ide <carpeta>" }
    $Ide = $elegido.FullName
}
Write-Host "compilando contra: $Ide"

$lib = Join-Path $Ide "lib"
if (-not (Test-Path $lib)) { throw "no existe $lib" }

# 2. Las bibliotecas del IDE van en el classpath.
$jars = (Get-ChildItem $lib -Filter '*.jar' | ForEach-Object { '"' + $_.FullName + '"' }) -join ';'
# 2b. Un JDK que pueda leer las clases del IDE. Desde 2024 los IDE usan Java 21,
# así que el javac tiene que ser 21 o más nuevo; si no, no puede ni leer los jars.
function Buscar-Jdk {
    $candidatos = @()
    if ($env:JAVA_HOME) { $candidatos += $env:JAVA_HOME }
    foreach ($base in @("C:\Program Files\Java", "$env:LOCALAPPDATA\Programs\Eclipse Adoptium")) {
        if (Test-Path $base) {
            $candidatos += (Get-ChildItem $base -Directory -ErrorAction SilentlyContinue |
                Where-Object { Test-Path (Join-Path $_.FullName "bin\javac.exe") } |
                ForEach-Object { $_.FullName })
        }
    }
    $validos = @()
    foreach ($candidato in $candidatos) {
        $javac = Join-Path $candidato "bin\javac.exe"
        if (-not (Test-Path $javac)) { continue }
        $salida = & $javac -version 2>&1
        if ($salida -match 'javac (\d+)') {
            $mayor = [int]$Matches[1]
            if ($mayor -ge 21) { $validos += [pscustomobject]@{ Ruta = $candidato; Mayor = $mayor } }
        }
    }
    return $validos | Sort-Object -Property Mayor -Descending | Select-Object -First 1
}

$jdk = Buscar-Jdk
if (-not $jdk) {
    throw "hace falta un JDK 21 o más nuevo para compilar contra este IDE (found JAVA_HOME=$env:JAVA_HOME)"
}
$env:JAVA_HOME = $jdk.Ruta
$javac = Join-Path $jdk.Ruta "bin\javac.exe"
$jar_exe = Join-Path $jdk.Ruta "bin\jar.exe"
Write-Host "jdk:               $($jdk.Ruta) (Java $($jdk.Mayor))"

$salida = Join-Path $aqui "build"
if (Test-Path $salida) { Remove-Item -Recurse -Force $salida }
New-Item -ItemType Directory -Force -Path $salida | Out-Null

# 3. Compilar.
$fuentes = Get-ChildItem (Join-Path $aqui "src") -Recurse -Filter '*.java' |
    ForEach-Object { $_.FullName }
Write-Host "compilando $($fuentes.Count) archivos…"
$argumentos = @("-encoding", "UTF-8", "-cp", $jars, "-d", $salida) + $fuentes
& $javac @argumentos
if ($LASTEXITCODE -ne 0) { throw "javac falló con $LASTEXITCODE" }

# 4. Copiar el plugin.xml y empaquetar.
Copy-Item (Join-Path $aqui "resources\META-INF") $salida -Recurse -Force
# Que no viajen las firmas de IntelliJ: el IDE rechaza un jar con firma rota.
Get-ChildItem $salida -Recurse -Filter '*.SF' | Remove-Item -Force -ErrorAction SilentlyContinue
# El jar se deja al lado del LEEME, junto al bundle, para tenerlo a mano.
$jar = Join-Path (Split-Path -Parent $aqui) "Nooki-1.0.0.jar"
if (Test-Path $jar) { Remove-Item $jar -Force }
& $jar_exe --create --file $jar -C $salida .
if ($LASTEXITCODE -ne 0) { throw "jar falló con $LASTEXITCODE" }

$tamano = [math]::Round((Get-Item $jar).Length / 1KB, 0)
Write-Host "listo: Nooki-1.0.0.jar ($tamano KB)"
Write-Host "instálalo con Settings → Plugins → ⚙ → Install Plugin from Disk…"
