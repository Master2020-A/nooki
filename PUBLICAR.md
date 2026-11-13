# Publicar Nooki

Pasos para llevar el proyecto de esta carpeta a un repositorio público, con integración continua
funcionando. Son diez minutos y no requieren tocar el código.

## Antes de empezar

Comprueba que el repositorio está limpio y verde:

```bash
$ cd nooki
$ git status --short              # no debe mostrar nada
$ ctest --preset windows-mingw    # 17/17 correctas
```

Y que la documentación es coherente:

```bash
$ ctest --preset windows-mingw -R documentos
```

## 1. Decidir la licencia

El proyecto es de doble licencia (`MIT OR Apache-2.0`) y los textos completos ya están en
`LICENCIA-MIT.txt` y `LICENCIA-APACHE.txt`. No hay nada que hacer salvo confirmarlo: quien publica
elige, y el que usa elige después.

Si quieres usar solo una, borra el otro archivo y ajusta `LICENCIA.md` y el `SPDX-License-Identifier`
de la cabecera de los fuentes.

## 2. Crear el repositorio

Con la CLI de GitHub (recomendado):

```bash
$ gh auth login
$ gh repo create nooki --public --source=. --remote=origen --push
```

Sin la CLI: crea el repositorio vacío en la web, **sin README ni licencia** (los dos ya existen aquí),
y después:

```bash
$ git remote add origen https://github.com/<usuario>/nooki.git
$ git push -u origen principal
```

> La rama se llama `principal`, y es la que espera la configuración de la CI
> (`.github/workflows/ci.yml`, eventos `push` y `pull_request` sobre `principal` y `main`).

## 3. Ver la primera ejecución de la CI

```bash
$ gh run list --limit 3
$ gh run watch
```

O en la web: **Actions → Integración continua**.

Debe salir verde en los tres sistemas. Es la primera vez que la CI se ejecuta: **es probable que algo
falle**, y eso es información, no un fracaso. Lo más habitual:

| Síntoma | Causa | Arreglo |
|---|---|---|
| `CMake Error: Could not find preset` | nombre de preset distinto | revisar `CMakePresets.json` |
| El backend no encuentra compilador de C (Linux/macOS) | falta `cc` | instalar `build-essential` / Xcode CLT en el paso previo |
| El backend no encuentra compilador de C (Windows) | el `cc` de MinGW no está en el `PATH` | el paso «Instalar compilador de C» ya lo añade; comprobar la ruta |
| Falla al compilar un programa con ventana (Linux) | falta `libx11-dev` | el paso «Instalar X11» ya lo instala |
| Falla solo en macOS | no hay backend de ventanas | el runtime responde «sin pantalla»; los programas compilan y las pruebas pasan |
| `formato` o `analisis` en rojo | son informativos (`continue-on-error`) | no bloquean; se endurecen cuando el estilo se estabilice |

Arregla lo que salga en un commit aparte, con su explicación:

```bash
$ git add -A && git commit -s -m "ci: corrige la ruta del preset en linux"
$ git push
```

## 4. Etiquetar la 1.0

```bash
$ git tag -a v1.0.0 -m "Nooki 1.0: el lenguaje que describe la especificación"
$ git push origen v1.0.0
```

Y crear la publicación en GitHub con el contenido de [NOTAS_DE_VERSION.md](NOTAS_DE_VERSION.md).

## 5. Publicar la distribución

Se genera con un comando y contiene el binario, la documentación, los ejemplos, los editores, la
especificación y un `INSTALAR.txt`:

```powershell
$ .\herramientas\empaquetar.ps1                    # Windows
$ ./herramientas/empaquetar.sh                     # Linux y macOS
```

Queda en `dist/nooki-1.0.0-<plataforma>.zip` (o `.tar.gz`). Se adjunta a la publicación de GitHub.
Conviene comprobar que el binario de dentro funciona antes de subirlo:

```bash
$ cd dist && unzip nooki-1.0.0-windows-x86_64.zip
$ ./nooki-1.0.0-windows-x86_64/nooki.exe ejecutar nooki-1.0.0-windows-x86_64/ejemplos/01-hola/principal.oki
¡Hola, Nooki!
```

## 6. Mantener la costumbre

Cada cambio, desde el primero:

1. Una rama: `git switch -c biblioteca/archivo`.
2. Los cambios con su prueba (ver [CONTRIBUIR.md](CONTRIBUIR.md)).
3. `ctest` en verde en los dos compiladores de C que tengas a mano.
4. Un commit firmado, en español e imperativo.
5. Una solicitud de cambios, y la CI como árbitro.

## Lo que falta después

Con el repositorio publicado, lo que queda es la lista de [DECISIONES.md](DECISIONES.md), sección 7.
La publicación es el paso que falta para que el proyecto exista fuera de esta máquina. Todo lo demás
—macOS de verdad, el resaltado semántico, un diseñador visual— se decide mejor con usuarios delante.
