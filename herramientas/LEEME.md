# Herramientas

Utilidades del proyecto que no forman parte del compilador.

## `empaquetar.ps1` / `empaquetar.sh`

Crea una distribución lista para entregar: el binario, la documentación, los ejemplos, los editores y
la especificación, con un `INSTALAR.txt` que dice cómo añadirla al `PATH`.

```powershell
.\herramientas\empaquetar.ps1              # Windows
```
```bash
./herramientas/empaquetar.sh               # Linux y macOS
```

El resultado queda en `dist/` como `.zip` o `.tar.gz`, con el nombre
`nooki-<versión>-<plataforma>`. Conviene comprobar que el binario que va dentro funciona:

```bash
$ cd dist && tar -xzf nooki-1.0.0-linux-x86_64.tar.gz
$ ./nooki-1.0.0-linux-x86_64/nooki --version
nooki 1.0.0 (linux-x86_64, edición 2026)
$ ./nooki-1.0.0-linux-x86_64/nooki ejecutar nooki-1.0.0-linux-x86_64/ejemplos/01-hola/principal.oki
¡Hola, Nooki!
```

Lo que incluye la distribución:

| Carpeta o archivo | Contenido |
|---|---|
| `nooki` | el compilador |
| `LEEME.md`, `GUIA.md`, `NOTAS_DE_VERSION.md` | para empezar |
| `CHANGELOG.md`, `DECISIONES.md` | el historial y las decisiones |
| `LICENCIA.md`, `LICENCIA-MIT.txt`, `LICENCIA-APACHE.txt` | la licencia |
| `CONTRIBUIR.md`, `PUBLICAR.md` | cómo participar y cómo se publica |
| `ejemplos/` | cuatro programas ejecutables |
| `editores/` | resaltado de sintaxis |
| `spec/` | la especificación del lenguaje |
| `INSTALAR.txt` | los pasos de instalación |

No incluye el código fuente del compilador: eso vive en el repositorio.
