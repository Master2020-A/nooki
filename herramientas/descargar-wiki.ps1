<#
.SYNOPSIS
Descarga la wiki que DeepWiki genera para un repositorio y la convierte en HTML y PDF.

.DESCRIPTION
DeepWiki sirve el contenido con JavaScript y detrás de una comprobación anti-bots,
así que no basta con descargar el HTML. Este script usa Chrome en modo headless
—que sí pasa la comprobación— para volcar el DOM de cada página, extrae el
contenido con `herramientas/wiki/extraer.py`, y con el mismo Chrome imprime el
resultado a PDF.

.PARAMETER Repositorio
Repositorio en DeepWiki, con la forma `dueño/nombre`.

.PARAMETER Salida
Carpeta donde dejar `wiki-<nombre>.html` y `wiki-<nombre>.pdf`.

.EXAMPLE
./herramientas/descargar-wiki.ps1 -Repositorio Master2020-A/nooki
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $Repositorio,

    [string] $Salida = 'dist',

    [string] $Chrome = 'C:\Program Files\Google\Chrome\Application\chrome.exe',

    # Segundos que se le dan a cada página para dibujarse.
    [int] $Espera = 12
)

$ErrorActionPreference = 'Stop'
$raiz = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$python = 'C:\Users\Master\.dsh\dsh-runtimes\dsh-primary-runtime\dependencies\python\python.exe'

if (-not (Test-Path $Chrome)) {
    throw "no se encontró Chrome en «$Chrome»; pásalo con -Chrome"
}
if (-not (Test-Path $python)) {
    throw "no se encontró el Python con lxml en «$python»"
}

$base = "https://deepwiki.com/$Repositorio"
$nombre = ($Repositorio -split '/')[-1]
$trabajo = Join-Path $raiz "build/wiki/$nombre"
$destino = Join-Path $raiz $Salida
New-Item -ItemType Directory -Force -Path $trabajo, $destino | Out-Null

# Un perfil propio: conserva la cookie que resuelve la comprobación de Vercel
# entre una página y la siguiente, así no se repite en cada una.
$perfil = Join-Path $trabajo 'perfil'

function Volcar-Dom {
    param([string] $Url, [string] $Archivo)
    $temporal = "$Archivo.tmp"
    $milisegundos = $Espera * 1000
    # Chrome escribe avisos suyos por la salida de error (CRL, GPU…). No son
    # fallos, así que durante la llamada no se corta por ellos.
    $anterior = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    & $Chrome --headless=new --disable-gpu --no-first-run --no-default-browser-check `
        --hide-scrollbars "--virtual-time-budget=$milisegundos" `
        "--user-data-dir=$perfil" --dump-dom $Url 2>$null |
        Out-File -FilePath $temporal -Encoding utf8
    $ErrorActionPreference = $anterior
    if (-not (Test-Path $temporal)) {
        throw "no se pudo descargar $Url"
    }
    Move-Item -Force $temporal $Archivo
    return (Get-Item $Archivo).Length
}

Write-Host "índice: $base"
$indiceHtml = Join-Path $trabajo 'indice.html'
$tamano = Volcar-Dom -Url $base -Archivo $indiceHtml
Write-Host ("  " + [math]::Round($tamano / 1KB) + " KB")

$texto = Get-Content $indiceHtml -Raw -Encoding utf8
if ($texto -match 'Vercel Security Checkpoint' -and $texto -notmatch 'prose-custom') {
    throw 'la comprobación de Vercel no dejó pasar; abre la web una vez en Chrome y vuelve a intentarlo'
}

# Las páginas salen de los enlaces del índice.
$patron = 'href="(?:https://deepwiki\.com)?(/' + [regex]::Escape($Repositorio) + '/[^"#]+)"'
$paginas = [regex]::Matches($texto, $patron) |
    ForEach-Object { $_.Groups[1].Value } |
    Select-Object -Unique
Write-Host "páginas encontradas: $($paginas.Count)"

$descargadas = 0
foreach ($ruta in $paginas) {
    # Hay títulos con `:` («5.2-collections:-vector-and-mapa»), que Windows no
    # admite en un nombre de archivo.
    $slug = ($ruta -split '/')[-1] -replace '[\\/:*?"<>|]', '-'
    $archivo = Join-Path $trabajo "$slug.html"
    if (Test-Path $archivo) {
        Write-Host "  ya está $slug"
        $descargadas += 1
        continue
    }
    $tamano = Volcar-Dom -Url "https://deepwiki.com$ruta" -Archivo $archivo
    Write-Host ("  " + $slug + " (" + [math]::Round($tamano / 1KB) + " KB)")
    $descargadas += 1
}

Write-Host "páginas descargadas: $descargadas"

# El HTML de una pieza, con el CSS incrustado para que se pueda abrir sin conexión.
$html = Join-Path $destino "wiki-$nombre.html"
& $python (Join-Path $raiz 'herramientas/wiki/extraer.py') `
    --entrada $trabajo --salida $html --base "https://deepwiki.com" --titulo "Nooki — wiki del repositorio"
if ($LASTEXITCODE -ne 0) {
    throw 'falló la extracción del contenido'
}
$kbHtml = [math]::Round((Get-Item $html).Length / 1KB)
Write-Host "html: $html ($kbHtml KB)"

# Y el PDF, imprimiendo ese mismo HTML.
$pdf = Join-Path $destino "wiki-$nombre.pdf"
$rutaHtml = (Resolve-Path $html).Path
& $Chrome --headless=new --disable-gpu --no-first-run --no-default-browser-check `
    --virtual-time-budget=30000 `
    --user-data-dir="$perfil" --print-to-pdf="$pdf" "file:///$($rutaHtml -replace '\\', '/')" 2>$null
if (-not (Test-Path $pdf)) {
    throw 'no se pudo generar el PDF'
}
$kbPdf = [math]::Round((Get-Item $pdf).Length / 1KB)
Write-Host "pdf:  $pdf ($kbPdf KB)"
