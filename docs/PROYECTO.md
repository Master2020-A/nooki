# El proyecto de Nooki

Qué es `nooki.toml`, cuándo hace falta y qué hace el compilador con él.

---

## Qué es

`nooki.toml` es el **manifiesto** del proyecto: un archivo de texto que dice cómo se llama el proyecto y
de qué otros proyectos depende. Es lo que convierte una carpeta con archivos `.oki` en un proyecto.

**No hace falta para un programa suelto.** `nooki ejecutar hola.oki` funciona sin manifiesto; el
manifiesto existe para cuando un proyecto crece y quiere usar módulos de fuera.

---

## Cuándo hace falta

| Situación | ¿Manifiesto? |
|---|---|
| Un archivo, `nooki ejecutar hola.oki` | no |
| Varios archivos en la misma carpeta, con `usar` | no: se buscan a los lados |
| Un proyecto con `nooki construir` y `nooki probar` | **sí** |
| Un proyecto que usa módulos de otra carpeta | **sí** |

`nooki nuevo` lo crea por ti.

---

## Lo que crea `nooki nuevo`

```
demo/
├── nooki.toml            el manifiesto
├── principal.oki         el programa, con su funcion principal
├── biblioteca/           los módulos propios del proyecto
│   └── herramientas.oki
└── pruebas/              aquí van las pruebas (se crea al añadir la primera)
```

`nooki construir` busca la entrada en `principal.oki` y, si no está, en `fuente/principal.oki`.

---

## El contenido

```toml
# Manifiesto de un proyecto de Nooki.

[proyecto]
nombre = "demo"
version = "0.1.0"

[dependencias]
utilidades = { ruta = "../utilidades" }
```

### `[proyecto]`

| Clave | Qué es | Por defecto |
|---|---|---|
| `nombre` | el nombre del proyecto | el de la carpeta |
| `version` | la versión | `0.1.0` |

### `[dependencias]`

Cada línea declara otro proyecto y **dónde está su carpeta**, con ruta relativa a este manifiesto:

```toml
[dependencias]
utilidades = { ruta = "../utilidades" }
graficos   = { ruta = "libs/graficos" }
```

La única forma admitida es `nombre = { ruta = "..." }`. La ruta tiene que ir **entre comillas**.

No hay versiones ni descarga: Nooki **no** va a buscarlas a internet. Una dependencia es una carpeta que
ya está en el disco.

### Reglas del formato

- Las secciones van entre corchetes: `[proyecto]`.
- Cada valor es `clave = valor`.
- `#` empieza un comentario, hasta el final de la línea.
- Las comillas de los valores se quitan (las de la ruta son obligatorias).
- Las líneas vacías se ignoran.

Es un subconjunto de TOML, no TOML completo: no hay listas, ni tablas anidadas, ni tipos.

---

## Cómo se usan las dependencias

Cuando un archivo hace `usar utilidades`, el compilador busca el módulo en este orden:

1. La carpeta del archivo que hace el `usar`.
2. La `biblioteca/` del proyecto.
3. Las carpetas padre, subiendo.
4. **La raíz y la `biblioteca/` de cada dependencia** del manifiesto.
5. La biblioteca estándar, que va dentro del compilador.

Ese punto 4 es lo único que aporta el manifiesto: sin él, `usar utilidades` no encontraría
`../utilidades/biblioteca/utilidades.oki`.

---

## Ejemplo completo

Dos proyectos, uno que usa al otro:

```text
taller/
├── nooki.toml
├── principal.oki
└── biblioteca/
    └── saludos.oki

utilidades/
├── nooki.toml
└── biblioteca/
    └── textos.oki
```

`taller/nooki.toml`:

```toml
[proyecto]
nombre = "taller"
version = "0.1.0"

[dependencias]
utilidades = { ruta = "../utilidades" }
```

`taller/principal.oki`:

```nooki
usar saludos
usar textos

funcion principal() -> entero {
    variable nombre = "Ana"
    imprimir(saludos.saludo(&nombre))
    variable trozo = "-"
    imprimir(textos.repetir(&trozo, 10))
    retornar 0
}
```

`saludos.oki` viene de la `biblioteca/` propia; `textos` se encuentra porque está en la `biblioteca/` de
la dependencia declarada.

---

## Errores frecuentes

| Síntoma | Causa |
|---|---|
| `error[NOK0001]: aquí no hay un proyecto de Nooki` | no hay `principal.oki` ni `nooki.toml` en esta carpeta |
| `nooki: no se pudo leer nooki.toml: …: falta «=» en «…»` | una línea sin `=` |
| `la dependencia «x» necesita «ruta»` | falta `{ ruta = "..." }` |
| `falta la ruta entre comillas en «x»` | la ruta va entre comillas |
| `NOK3008: no se encontró el módulo` | el módulo no está en ninguna ruta de búsqueda |

---

## Lo que el manifiesto todavía no hace

- **No hay versiones**: no se puede pedir `utilidades = "1.2"` ni resolver conflictos.
- **No hay descarga**: no hay registro de paquetes ni `nooki fetch`.
- **No hay archivo de bloqueo** (`nooki.lock`).
- **No hay dependencias transitivas**: solo las declaradas aquí; si `utilidades` depende de otra cosa,
  también hay que declararla.

Está anotado en [DECISIONES.md](../DECISIONES.md) como pendiente.
