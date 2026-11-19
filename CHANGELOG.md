# Historial de cambios

El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/) y el proyecto usa
[versionado semántico](https://semver.org/lang/es/).

## [1.0.14] — 2026-11-18

### Sprint 142 — Lambdas (2026-11-18)

Pasar una función a otra ya se podía, pero había que declararla aparte y con nombre. Ahora se escribe donde
hace falta:

```nooki
imprimir(aplicar(21, funcion(n: entero) -> entero => n * 2))

variable triple = funcion(n: entero) -> entero => n * 3
imprimir(triple(5))
```

La forma es `funcion(parámetros) -> tipo => expresión`; el cuerpo es una sola expresión, que es lo que cabe
sin llaves ni `retornar`.

Por debajo es azúcar: el generador emite cada lambda como una función `static` aparte y por donde aparecía
deja su nombre. Se emiten **antes** de los cuerpos, con sus parámetros en un ámbito propio.

Una lambda **no captura**: solo ve sus parámetros. Usar una variable de fuera es `NOK3001` en la línea
exacta — lo comprueba el verificador, para que el error sea del lenguaje y no del compilador de C.

Caso `0092-lambdas`, con lambda directa, en variable, de dos parámetros y de texto.

## [1.0.13] — 2026-11-18

### Sprint 141 — Una función en una variable (2026-11-18)

La 1.0.12 dejaba pasar funciones como argumento, pero no guardarlas. Ahora sí, escribiendo el tipo:

```nooki
variable f: funcion(entero) -> entero = doblar
imprimir(f(21))     // 42

f = sumar_uno       // otra con la misma forma
imprimir(f(21))     // 22

imprimir(aplicar(10, f))   // y se pasa como cualquier valor
```

El verificador apunta las variables de tipo función en su ámbito para poder llamarlas; el generador emite la
declaración con el nombre dentro del puntero (`long long (*f)(long long) = nooki_doblar;`) y resuelve el tipo
de vuelta de la llamada.

**Lo que sigue sin poder hacerse es devolver una función**, y con eso no hay cierres ni `lambda`: un puntero
no lleva memoria consigo. Eso es el siguiente escalón, y es grande.

Caso `0091-funcion-en-variable`.

## [1.0.12] — 2026-11-18

### Sprint 140 — Funciones como argumento (2026-11-18)

Hasta hoy una función no se podía pasar a otra: `filtrar(numeros, es_par)` era imposible, y con eso no había
manera de escribir un `apply`, un `map` ni un `filtrar` propios.

Ahora sí, con un tipo `funcion(...) -> tipo` que es un **puntero a función**:

```nooki
funcion filtrar(numeros: &Vector<entero>, cumple: funcion(entero) -> booleano) -> Vector<entero> {
    variable salida = Vector<entero>.nuevo()
    variable i = 0
    mientras (i < numeros.largo()) {
        si (cumple(numeros.obtener(i))) {
            salida.agregar(numeros.obtener(i))
        }
        i = i + 1
    }
    retornar salida
}

variable pares = filtrar(&numeros, es_par)
```

Dentro de la función que la recibe, el parámetro se llama como cualquier función. El nombre de una función
sin paréntesis es su valor.

Por debajo: el tipo se traduce a `R (*)(A, B)`; el parámetro se emite con el nombre dentro
(`R (*cumple)(A, B)`); la llamada va sin el prefijo `nooki_`; y un nombre de función pasado como argumento se
emite con su nombre en C.

**Lo que no entra, y es a propósito:** no es un valor de primera clase. No se puede guardar en una variable
ni devolver desde una función, porque no hay cierres: es un puntero, sin memoria propia. Eso —funciones que
capturan su entorno— es otro trabajo, y bastante más grande.

Caso `0090-funciones-valor`, con `filtrar` y `mapear` sobre un vector.

## [1.0.11] — 2026-11-17

### Sprint 139 — Pedir una columna por su nombre (2026-11-17)

La tabla se leía por índice de columna. Ahora también por nombre, que es lo que se escribe sin pensar:

```nooki
si (datos.hay_columna(t, "unidades")) {
    variable unidades = datos.entero_por_nombre(t, f, "unidades")
    variable precio = datos.numero_por_nombre(t, f, "precio")
}
```

`indice_columna(t, &nombre)` da la posición (o `-1`), `hay_columna` dice si está, y
`columna_por_nombre`, `valor_por_nombre`, `numero_por_nombre` y `entero_por_nombre` leen sin llevar el índice
a mano. Además `guardar(t, &ruta)` escribe la tabla en un CSV.

De paso, el módulo `datos` no importaba `archivo`, y al añadir `guardar` eso salió como `NOK3001` al usar el
módulo. Corregido: el módulo declara sus importaciones.

**220 firmas** en la biblioteca.

## [1.0.10] — 2026-11-17

### Sprint 138 — Columnas calculadas en la tabla (2026-11-17)

La `Tabla` de la 1.0.9 sabía leer, pero no crecer: no había forma de añadir una columna con un valor
calculado. En pandas eso es `df["importe"] = df.unidades * df.precio`.

Ahora se puede, y el cálculo se escribe **en Nooki**, que es lo que hacía falta para no depender de que el
SQL supiera hacerlo:

```nooki
variable importes = Vector<texto>.nuevo()
variable f = 0
mientras (f < datos.filas(t)) {
    variable unidades = datos.entero_de(t, f, 1)
    variable precio = datos.numero(t, f, 2)
    importes.agregar(textos.de_flotante((unidades como flotante) * precio) + "")
    f = f + 1
}

variable nombre = "importe" + ""
datos.agregar_columna(t, &nombre, importes)
```

`agregar_columna` comprueba que haya tantos valores como filas, copia los textos y libera la estructura
anterior. El ejemplo `13-tabla` añade una columna `precio_medio` calculada fila a fila.

Esto cierra la diferencia que quedaba con un `apply` de pandas para el caso normal: recorrer y calcular con
código propio. Lo que sigue sin existir es pasarle **una función** a la tabla, y la indexación por etiqueta.

## [1.0.9] — 2026-11-17

### Sprint 137 — Una tabla en memoria, como un DataFrame (2026-11-17)

Hasta ahora `datos.consultar(sql)` devolvía **texto CSV**: para recorrer un resultado había que partirlo con
el módulo `csv` y convertir cada campo a mano. Faltaba el objeto.

Ahora `datos.tabla(sql)` devuelve una **`Tabla`** —el resultado ya en memoria, con sus columnas— y se
trabaja con ella:

```nooki
variable t = datos.tabla(&consulta)

mientras (f < datos.filas(t)) {
    variable ciudad = datos.valor(t, f, 0)        // texto
    variable unidades = datos.entero_de(t, f, 1)  // ya convertido
    total = total + datos.numero(t, f, 2)
    f = f + 1
}
datos.liberar(t)
```

`filas`, `columnas`, `nombre_columna`, `valor` (texto), `numero` y `entero_de` (convertidos),
`columna(t, c)` que devuelve la columna entera como `Vector<texto>`, y `a_texto` para el CSV. La tabla se
libera con `datos.liberar`.

DuckDB rellena la estructura en C directamente, sin pasar por el CSV intermedio: los nombres de columna y
los valores se guardan ya en memoria y los números se convierten con `strtod` y `strtoll`.

Ejemplo nuevo `ejemplos/13-tabla`, que consulta, recorre, suma y guarda el resultado. **212 firmas** en la
biblioteca.

## [1.0.8] — 2026-11-17

### Sprint 136 — Un vector de fichas, con su texto (2026-11-17)

Un `Vector` de estructuras solo admitía estructuras **sin campos con dueño**: meter una `ficha { nombre:
texto, edad: entero }` era `NOK6003`. Y un registro con un nombre es justo lo que se guarda en una lista.

Ahora se admite. El vector guarda **su propia copia** del texto y la libera, así que la variable de la que
salió sigue siendo de quien la escribió:

```nooki
variable lista = Vector<ficha>.nuevo()
lista.agregar(ficha { nombre: ana, edad: 30 })
variable primera = lista.obtener(0)
imprimir(primera.nombre)
```

El generador emite, para cada una de esas estructuras, tres ayudantes: copiar de verdad, dar una **vista**
(para leer sin liberar) y liberar. Con eso:

- `agregar` y `modificar` copian el texto; `modificar` libera el que había.
- `obtener` devuelve una vista: el texto es del vector, como en `Vector<texto>`.
- `vaciar` y `soltar` liberan los textos de todos los elementos.

Comprobado con ASan **sobre el código C generado** —no solo sobre el compilador—: sin fugas ni dobles
liberaciones. Caso `0089-vector-fichas`.

Lo que sigue sin admitirse es una estructura con una **colección o una caja** dentro, que exigiría copiarlas
en profundidad; el mensaje ahora lo dice así.

## [1.0.7] — 2026-11-17

### Sprint 135 — `.largo()` sobre el resultado de una llamada (2026-11-17)

La 1.0.6 traía los argumentos del programa, pero `sistema.argumentos().largo()` **no compilaba**: el
generador emitía `&llamada()`, y en C no se puede tomar la dirección de un valor temporal.

Ahora, cuando el receptor de un método de colección no es un sitio (una variable, un campo, un índice), se
le da uno con un literal compuesto —la misma idea que ya se usaba al prestar el resultado de una llamada—.

```nooki
variable lista = sistema.argumentos()
imprimir(lista.largo())                  // ya funcionaba
imprimir(sistema.argumentos().largo())   // y ahora también
```

El caso `0088-argumentos` comprueba las dos formas.

De paso, el integrado que cuenta los argumentos pasa a llamarse `cuantos_argumentos_programa`, para que no
comparta nombre con la función del módulo `sistema`. No era un fallo —`cuantos_argumentos()` funcionaba—,
sino dos cosas con el mismo nombre, que es una trampa para el siguiente que lo lea.

**Una nota sobre cómo se detectó esto, porque importa:** el fallo de `.largo()` salió del compilador, no de
una medición. Y una supuesta avería de `cuantos_argumentos()` que creí ver era un artefacto de medir el
código de salida a través de WSL, donde `$?` llegaba siempre a cero: comprobado después con el código de
salida real, la 1.0.6 lo hacía bien. Queda escrito aquí porque la primera versión de esta entrada decía lo
contrario.
## [1.0.6] — 2026-11-17

### Sprint 134 — Argumentos para el programa (2026-11-17)

El `main` que generaba el compilador era `int main(void)`: un programa de Nooki **no podía recibir
argumentos**. Para un lenguaje compilado eso deja fuera media vida real —un programa que recibe un archivo,
un modo, un número de iteraciones—.

Ahora `main` recibe `argc` y `argv`, y el módulo `sistema` los expone:

```nooki
usar sistema

funcion principal() -> entero {
    variable lista = sistema.argumentos()
    variable i = 0
    mientras (i < lista.largo()) {
        imprimir(lista.obtener(i))
        i = i + 1
    }
    retornar 0
}
```

`sistema.argumentos()` da la lista sin el nombre del programa, `cuantos_argumentos()` la cuenta y
`argumento(i)` una posición concreta (vacío si no existe).

Y `nooki ejecutar` se los pasa: **todo lo que va detrás del fuente** es para el programa, y las opciones
del compilador van antes. Cada argumento se entrecomilla por separado, así que uno con espacios llega
entero.

```bash
nooki ejecutar programa.oki uno "dos palabras"
```

Caso nuevo `0088-argumentos` y dos pruebas de CLI que comprueban que el programa recibe lo que se le pasa
(devuelve cuántos, y el código de salida lo dice).

## [1.0.5] — 2026-11-17

### Sprint 133 — `nooki ejecutar` en Linux (2026-11-17)

Un fallo que la 1.0.4 llevaba dentro: en Linux y macOS, `nooki ejecutar hola.oki` compilaba y después
fallaba con «sh: hola: not found». La causa es que el binario se lanzaba por su nombre relativo, y la
consola de Unix **no busca en el directorio actual**: hay que escribir `./hola`. En Windows no se veía
porque `cmd` sí lo busca.

Arreglado en el lanzamiento, y con una prueba que lo cubre de verdad: el caso cambia a la carpeta del
programa y lo invoca por su nombre a secas, que es la forma en que el fallo aparece; con la corrección
pasa y sin ella falla.

## [1.0.4] — 2026-11-17

Cuarta versión de la serie 1.0. Trae el análisis de datos dentro del lenguaje y deja el proyecto con la
biblioteca que hace falta, sin que el usuario tenga que instalar nada.

### Sprint 132 — `datos` sin proceso y con la biblioteca dentro (2026-11-17)

El módulo `datos` lanzaba el ejecutable de DuckDB para cada consulta. Ahora habla con su **API de C**, que se
abre en tiempo de ejecución: el programa no se enlaza contra DuckDB al compilar y, si no analiza datos, ni
siquiera la carga. La interfaz del módulo no ha cambiado.

Y la biblioteca viaja con la distribución: `duckdb.dll` junto al ejecutable en Windows, `libduckdb.so` en
Linux (la descarga la integración continua al empaquetar). Se puede usar otra con `NOOKI_DUCKDB`.

Dos cosas que costó aprender y que quedan escritas en el código:

- **El `duckdb_result` real es mayor que el que declara su cabecera**: con la estructura replicada (48
  bytes) el programa corrompía el montón. Ahora se reserva un colchón alineado de 4 KB, que es lo único que
  la API necesita, porque solo usa el puntero.
- **El nombre de una columna es de la biblioteca**, no del llamante: liberarlo con `duckdb_free` corrompe
  el montón y el programa revienta al destruir el resultado. Ese `free` sobraba.

Probado con DuckDB **1.2.2** (la que se empaqueta) y con la **2.0.0**.

### Sprint 131 — Análisis de datos con SQL (2026-11-16)

Las operaciones que se hacen con pandas —agrupar, pivotar, unir, ventanas deslizantes, describir— se pueden
hacer desde Nooki con el módulo `datos`, sin pasar por Python: `consultar`, `ejecutar`, `a_csv`, `a_parquet`
y `version`. Detrás está DuckDB, elegida después de comprobar contra datos reales que cubre `groupby`,
`pivot_table`, `rolling`, `merge`, `describe`, filtrado, ordenación y lectura y escritura de CSV y Parquet.
Lo que no cubre —`apply` con una función propia de Python, gráficas, Excel— sigue teniendo su camino con el
módulo `sistema`. El ejemplo `ejemplos/12-datos` hace las siete operaciones con su equivalente en pandas al
lado.

**19 módulos y 197 firmas** en la biblioteca.
## [1.0.3] — 2026-11-16

### Sprint 131 — Análisis de datos con SQL nativo (2026-11-16)

Las operaciones que se hacen con pandas —agrupar, pivotar, unir, ventanas deslizantes, describir— ya se
pueden hacer desde Nooki con un módulo **`datos`**, sin pasar por Python.

```nooki
variable consulta = "SELECT ciudad, SUM(unidades) FROM read_csv('ventas.csv') GROUP BY ciudad" + ""
imprimir(datos.consultar(&consulta))
```

`consultar` devuelve CSV; `a_csv` y `a_parquet` guardan el resultado. Detrás está **DuckDB**, que se eligió
después de comprobar contra datos reales que cubre lo que se usa de pandas: `groupby`, `pivot_table`
(`PIVOT`), `rolling` (ventanas), `merge` (join), `describe`, filtrado, ordenación y lectura y escritura de
CSV y Parquet. Lo que no cubre —`apply` con una función propia de Python, gráficas, Excel— sigue teniendo
su camino: el módulo `sistema` con un guion de Python.

El ejemplo `ejemplos/12-datos` hace las siete operaciones y las imprime, con su equivalente en pandas
comentado al lado. **19 módulos** y **196 firmas** en la biblioteca.

## [1.0.3] — 2026-11-16

### Sprint 130 — Hablar con otros programas (2026-11-16)

Hasta ahora un programa de Nooki no podía lanzar otro. Eso dejaba fuera todo lo que ya existe hecho: un
guion de Python con pandas, una herramienta del sistema, un conversor.

Ahora hay un módulo **`sistema`** con dos funciones:

- `sistema.ejecutar(&comando) -> entero`: lanza la orden y devuelve su código de salida.
- `sistema.capturar(&comando) -> texto`: lanza la orden y devuelve su salida estándar.

Con eso, usar pandas desde Nooki es un puente de tres pasos: el programa escribe el CSV, llama a
`python analisis.py entrada.csv salida.csv` y lee el resultado. Está en
`ejemplos/11-pandas`, y funciona: pandas agrupa y suma, y Nooki lee el resumen.

La orden se entrega al intérprete del sistema (`cmd` en Windows, `sh` en Linux), así que las comillas de
las rutas son responsabilidad de quien la escribe.

Caso nuevo `0087-sistema`. **117 casos**, **18 módulos** y **190 firmas** en la biblioteca.

## [1.0.3] — 2026-11-16

### Sprint 129 — Punteros en `inseguro` y la wiki en español (2026-11-16)

**Control del hardware.** Dentro de `inseguro { }` hay un tipo `Puntero` y ocho operaciones: `reservar`,
`liberar`, `leer_en`, `escribir_en`, `leer_flotante_en`, `escribir_flotante_en`, `desde_direccion` y
`direccion_de`. Permiten pedir memoria, leerla y escribirla con desplazamientos y construir un puntero a una
dirección concreta, que es como se habla con los registros de un dispositivo. Fuera de `inseguro` no existen
(`NOK3013`). Las lecturas van con `memcpy`, así que valen en direcciones alineadas o no.

**Formato del código.** Se pasó `clang-format` a todo el compilador (2 321 desviaciones a cero) y el trabajo
de formato de la integración continua quedó en verde. Hubo que excluir la plantilla `version.hpp.in`, que
`clang-format` rompe, y quitar un `#include <shellapi.h>` que el orden alfabético ponía antes de
`windows.h`.

**La wiki en español.** Las 34 páginas de la wiki que DeepWiki genera para el repositorio, traducidas y
versionadas en `docs/wiki-es/`, con una herramienta que las descarga y las ensambla en HTML y PDF
(`herramientas/descargar-wiki.ps1`). Se corrigieron además los datos desfasados del original y se recuperaron
los diagramas, que la primera versión tiraba.

**113 → 116 casos** de conformidad y **47 diagnósticos**.

## [1.0.2] — 2026-11-16

### Sprint 128 — Punteros y memoria manual (2026-11-16)

El pilar que faltaba de la visión original: controlar el hardware como en C, pero sin renunciar a las
comprobaciones del resto del lenguaje.

Dentro de `inseguro { }` hay ya un tipo **`Puntero`** y ocho operaciones: `reservar`, `liberar`,
`leer_en`, `escribir_en`, `leer_flotante_en`, `escribir_flotante_en`, `desde_direccion` y `direccion_de`.
Con eso se pide memoria, se lee y se escribe en ella con desplazamientos, y se construye un puntero a una
dirección concreta —que es como se habla con los registros de un dispositivo.

Es un tipo crudo a propósito: se pasa por valor, no tiene dueño, no se libera solo y no comprueba límites.
Fuera de `inseguro` no existe: `NOK3013`. Las lecturas y escrituras van con `memcpy`, así que funcionan en
cualquier dirección, alineada o no, y un puntero nulo no lee ni escribe.

Caso nuevo `0086-punteros`. **116 casos** y **47 diagnósticos.**

## [1.0.2] — 2026-11-16

Segunda versión de la serie 1.0. Cierra la API de inteligencia artificial, añade colecciones que faltaban y
deja el repositorio presentable.

**Datos y colecciones.** `Vector<flotante>`, `Vector` de estructuras propias, `Mapa<texto, flotante>` y
`Mapa<texto, texto>`. Módulo `csv` para leer datos, `archivo.agregar` para escribir un registro y
`textos.de_entero` / `de_flotante` para meter números dentro de un mensaje.

**Inteligencia artificial.** Redes de una y dos capas ocultas, de 1 a 64 entradas y 2 a 16 clases,
entrenamiento por lotes con momento, y guardar y cargar las tres redes. `evaluar_profunda` para medir con
datos que no se vieron al entrenar.

**Lenguaje.** `sino si` encadenado, `para (i, x en coleccion)` con el índice, y prestar el resultado de una
llamada o una variable de tipo propio.

**Repositorio.** Portada con distintivos y captura, guías por audiencia verificadas, paquete de Linux
publicado por la integración continua en cada versión, y una herramienta para bajar la wiki de DeepWiki en
HTML y PDF. Los diez ejemplos del repositorio se compilan en cada `ctest`.

**115 casos de conformidad · 18 suites · Windows y Linux en verde.**

### Sprint 127 — `Vector` de estructuras (2026-11-15)

Ya se pueden guardar estructuras propias en un vector, con su `agregar`, `obtener`, `modificar`, `largo`,
`vaciar` y `soltar`, y recorrerlas con `para (i, x en v)`:

```nooki
variable puntos = Vector<punto>.nuevo()
puntos.agregar(punto { x: 1, y: 2 })
variable p = puntos.obtener(0)
imprimir(p.x)
```

El vector **se queda con el valor**, como `Vector<texto>`: no se copia nada por detrás ni hay que liberar
nada a mano. Por eso esta primera versión admite estructuras **sin campos con dueño** (`texto`, `Vector`,
`Mapa` o `Caja`): esas necesitarían copia y liberación profundas, y el compilador lo dice con claridad en
vez de generar algo que se rompa:

```
error[NOK6003]: el backend todavía no sabe traducir un Vector de la estructura «ficha»,
que tiene campos con dueño
```

De paso hubo que abrir los préstamos: `&punto` sobre una variable de tipo propio era «solo se puede
prestar una Caja». Ahora se presta una estructura, una colección o un texto, y se sigue rechazando
prestar algo que ya es un préstamo (sería un puntero a puntero).

**Y ASan encontró un fallo de verdad.** Al pasar la cadena de saneamiento, el verificador daba
`stack-use-after-return`: el ámbito guardaba el nombre de la estructura como **vista** a una cadena local
que ya había muerto, y `tipo_miembro` leía de ahí. Ahora el ámbito es dueño del nombre. Sin ASan, eso
habría sido un fallo silencioso en un programa con estructuras.

Caso nuevo `0085-vector-estructura`. **115 casos.**

## [1.0.1] — 2026-11-15

### Sprint 126 — Un programa que lo junta todo (2026-11-15)

`ejemplos/10-clasificador`: escribe un CSV con 225 puntos de tres clases, lo lee, entrena una red de dos
capas ocultas y **dibuja la frontera que ha aprendido**. Si hay entorno gráfico la pinta en una ventana de
480×480; si no, en caracteres. Acierta **225 de 225**.

Es el primer programa que usa las tres piezas a la vez —datos, modelo y ventana—, y con él la suite
aprende a comprobar algo que faltaba: **todos los ejemplos de `ejemplos/` se compilan** en cada `ctest`
(10 programas), además de los del manual y la referencia.

Probando la captura de la ventana salió un detalle: `PrintWindow` devuelve estados intermedios del
dibujado, así que no publiqué la imagen. El ejemplo deja constancia de su salida por consola, que sí está
verificada.

## [1.0.1] — 2026-11-15

### Sprint 125 — `Mapa<texto, texto>` (2026-11-15)

El mapa solo llevaba números, así que una agenda, un diccionario o una tabla de configuración había que
inventarla con dos vectores en paralelo. Ahora el valor puede ser **texto**:

```nooki
variable agenda = Mapa<texto, texto>.nuevo()
agenda.insertar(&ana, telefono)
imprimir(agenda.obtener(&ana))
```

El mapa guarda **copias** de la clave y del valor, y libera el valor viejo al actualizar o al quitar — que
es donde este tipo de estructura suele romperse. `obtener` devuelve una **vista**: el texto es del mapa,
no del llamante. Comprobado con ASan: sin fugas ni dobles liberaciones.

Caso nuevo `0084-mapa-texto`. **114 casos.**

## [1.0.1] — 2026-11-15

### Sprint 124 — Portada, paquete de Linux y dos huecos del lenguaje (2026-11-15)

**El LEEME es ahora la portada.** Distintivos de integración continua, versión, licencia y número de
casos; un programa de veinte líneas que entrena una red neuronal y la usa; la captura de la aplicación de
escritorio; y el manual y la referencia al principio de la documentación.

**Paquete de Linux en cada versión.** Un flujo nuevo (`.github/workflows/publicar.yml`) que, al empujar
una etiqueta, compila en Linux, empaqueta y adjunta el archivo a la versión — comprobando antes que el
binario del paquete responde y ejecuta un ejemplo. El empaquetador de Linux incluye ya los mismos
documentos que el de Windows, con las guías y sus PDF.

**Dos huecos del lenguaje, cerrados:**

- **`para (i, x en coleccion)`**: la posición además del elemento, sin llevar un contador a mano.
- **`archivo.agregar(ruta, texto)`**: añadir al final sin borrar lo que hubiera, que es lo que hace falta
  para un registro.

Casos nuevos `0082-para-indice` y `0083-archivo-agregar`. **113 casos.**

## [1.0.1] — 2026-11-15

Primera versión posterior a la 1.0: cierra la API de inteligencia artificial, añade `Vector<flotante>` y
`Mapa<texto, flotante>`, entrenamiento por lotes con momento, lectura de CSV, guardar y cargar las tres
redes, formato de números dentro de un texto y `sino si`. Incluye las guías por audiencia y su
verificación automática.

### Sprint 123 — El mapa de flotantes y prestar una llamada (2026-11-15)

**`Mapa<texto, flotante>`.** Solo había mapas de enteros, así que una tabla de notas o de precios había
que escalarla a mano. Ahora el valor puede ser `entero` o `flotante`, con el mismo juego de operaciones
(`insertar`, `obtener`, `contiene`, `quitar`, `largo`, `claves`) y sin truncar: `notas.obtener(&ana)`
devuelve `9,75`, no `9`.

**Prestar el resultado de una llamada.** `&campos.obtener(1)` era `NOK4008` («solo se puede prestar una
Caja»), así que había que guardar el campo en una variable antes de pasarlo. Ahora vale cualquier
expresión: el compilador le da un sitio temporal donde vivir. Ese rodeo me lo encontré dos veces
escribiendo el ejemplo de CSV, y era la comodidad que más estorbaba.

Casos nuevos `0080-mapa-flotante` y `0081-prestamo-llamada`. **111 casos.**

### Sprint 122 — Cerrar la API de IA y dos comodidades del lenguaje (2026-11-15)

### Sprint 122 — Cerrar la API de IA y dos comodidades del lenguaje (2026-11-15)

**Guardar la red profunda.** `guardar_profunda` / `cargar_profunda`, con cabecera propia (`NOOKIPRF`):
pesos de las dos capas, sesgos, muestras y etiquetas. Cargar el archivo de otra red devuelve `0`. Las
tres redes tienen ya su archivo y no se confunden entre ellas (`NOOKIRED`, `NOOKICLS`, `NOOKIPRF`).

**Medir con datos que no vio.** `evaluar_profunda(red, &p, cual)` dice si acierta una muestra concreta,
que es lo que hace falta para separar entrenamiento de prueba. En el ejemplo: entrena con 63 muestras y
acierta **18 de 18** en las 18 que no usó.

**`sino si`.** Encadenar condiciones ya no obliga a anidar bloques; es azúcar sobre el `sino`.

Casos nuevos `0077-red-profunda-guardar`, `0078-sino-si` y `0079-particion`. **109 casos.**

Queda pendiente de la lista: `Mapa<texto, flotante>` (sólo hay mapas de enteros) y poder prestar el
resultado de una llamada (`&lista.obtener(0)`), que hoy obliga a guardarlo antes en una variable.

### Sprint 121 — Publicado, y entrenar por lotes (2026-11-14)

### Sprint 121 — Publicado, y entrenar por lotes (2026-11-14)

**El proyecto está en GitHub**: <https://github.com/Master2020-A/nooki>, público, 130 commits. La primera
ejecución de la CI dio la razón a publicar: **Windows MSVC y Linux GCC pasan**, y a la vez salieron dos
cosas que sólo se ven fuera de esta máquina — el trabajo de formato se caía porque `.clang-format` tenía
un bloque `Language: YAML` que la versión del ejecutor no entiende, y el trabajo de macOS quedó cancelado
antes de configurar.

**Entrenar por lotes, con momento.** `redes.entrenar_profunda_lote(red, vueltas, tasa, lote, momento)`
acumula los gradientes de `lote` muestras y los aplica de una vez, conservando parte de la dirección
anterior. Con eso:

| Problema | Muestra a muestra | Por lotes + momento |
|---|---|---|
| Paridad de 4 bits | **8 de 16** (clase mayoritaria) con toda configuración probada | **16 de 16** |

**Y los números se pueden meter en un texto.** `"" + 42` era un error de tipo, así que no había forma de
componer un mensaje con un número dentro. Ahora `textos.de_entero(42)` da `"42"` y
`textos.de_flotante(2.5)` da `"2,5"`, con el mismo formato que `imprimir`: coma decimal, sin ceros de más.
Están escritos **en Nooki**, no en C, para que funcionen en cualquier plataforma.

Casos nuevos `0075-lotes-momento` y `0076-formato-numeros`. **106 casos.** La CI, además, enseñó que
`macos-14` no está disponible para esta cuenta: el trabajo se cancela antes de arrancar, así que pasa a
`macos-13` y como opcional, para que no tumbe la ejecución.

### Sprint 120 — Leer CSV y red de dos capas (2026-11-14)

### Sprint 120 — Leer CSV y red de dos capas (2026-11-14)

**Leer datos de verdad.** Faltaba lo más básico para usar el lenguaje con datos: convertir texto en
números y leer un archivo separado por comas.

- **`texto.a_flotante()`**: convierte un número escrito, con punto **o coma** decimal; si no hay dígitos,
  `0`. Con eso `"7,5".a_flotante()` es `7,5` y ya se puede sumar.
- **Módulo `csv`**, escrito en Nooki: `filas`, `campos`, `campos_con`, `columnas`, `a_numero`, `a_entero`.
  `campos_con` existe por un motivo que salió al probarlo: con comas como separador **y** comas
  decimales, las columnas se rompen; con `;` caben las dos cosas.

Y un arreglo que salió de ahí: pasar un parámetro `&texto` a un método de texto (`dividir(separador)`)
generaba C inválido; ahora se desreferencia solo.

**Red de dos capas ocultas.** `nueva_profunda`, `ejemplo_profunda`, `entrenar_profunda`,
`predecir_profunda`, `confianza_profunda`, `aciertos_profunda`, `liberar_profunda`. Va en su propia
estructura para no tocar la de una capa, y aprende las tres bandas diagonales al completo (**81 de 81**).

Lo que **no** consigue, y queda dicho: la **paridad de 4 bits** se queda en 8 de 16 —la clase
mayoritaria— con cualquier configuración probada. El descenso de gradiente plano no basta para eso.

**Y un fallo de fondo: los escapes no se aplicaban.** `"a\nb"` imprimía `a\nb` —el lexer validaba la
secuencia y el generador la volvía a escapar, así que el carácter nunca llegaba—. Ahora `\n`, `\r`, `\t`,
`\\`, `\"` y `\'` se aplican de verdad, y `\0` se rechaza (`NOK1009`) porque un texto con un cero dentro
se cortaría al usarlo como cadena. Salió al probar `csv`: el programa escribía archivos con la barra y la
`n` en vez de un salto.

Casos nuevos `0072-csv`, `0073-red-profunda` y `0074-escapes`. **104 casos.**

### Sprint 119 — Las guías, comprobadas y dentro del paquete (2026-11-14)

### Sprint 119 — Las guías, comprobadas y dentro del paquete (2026-11-14)

Las cuatro guías por audiencia (usuario, administrador, desarrollador y referencia del lenguaje) entran
en el repositorio, en el paquete y en el LEEME, con sus PDF.

**Y se comprueban.** `herramientas/verificar-guia.ps1` extrae los bloques `nooki` de una guía y prueba
cada uno de tres formas —tal cual, metido en un `principal` y con un `principal` añadido—; solo canta
cuando **ninguna** funciona. Resultado:

| Guía | Bloques | Fallos reales |
|---|---|---|
| Usuario | 59 | 0 |
| Desarrollador | — | 0 |
| Administrador | — | 0 |
| Referencia del lenguaje | — | 0 (las plantillas de sintaxis no son código) |

Los únicos bloques que no compilan son fragmentos que usan variables del texto que los rodea, ejemplos
de dos archivos, plantillas de gramática y bloques que enseñan un error a propósito (marcados con
`// error[NOK...]`); el script los distingue.

### Sprint 118 — `Vector<flotante>` y redes de hasta 64 entradas (2026-11-14)

### Sprint 118 — `Vector<flotante>` y redes de hasta 64 entradas (2026-11-14)

**El lenguaje no tenía vectores de decimales.** Solo `Vector<entero>` y `Vector<texto>`, y sin eso no se
pueden pasar datos reales a una red. Ahora `Vector<flotante>` funciona entero: crear, `agregar`,
`obtener`, `modificar`, `largo`, `vaciar`, recorrerlo con `para` y devolverlo de una función. Su elemento
es un `flotante` también para el verificador, así que `suma = suma + x` dentro de un `para` compila.

**Y con eso la red pasa de 4 entradas a 64:**

```
redes.nueva_clases(entradas, ocultas, clases)   // entradas: 1 a 64
redes.ejemplo_datos(red, &v, cual)              // v mide `entradas`
redes.predecir_datos(red, &v)
redes.probabilidad_datos(red, &v, cual)
```

El runtime va en un archivo aparte y no ve el tipo del lenguaje, así que declara una **vista** con los
mismos tres campos (`datos`, `largo`, `capacidad`) y comprueba la longitud antes de leer nada: un vector
con el tamaño equivocado se rechaza con `false` en vez de leer fuera de rango.

Comprobado con una red de **8 entradas y 2 clases: 40 de 40**. Casos nuevos `0070-vector-flotante` y
`0071-red-entradas`. **101 casos.**

De paso se arregló el `para` sobre vectores, que declaraba la variable del bucle como `long long` y
**truncaba los decimales** sin avisar, y las funciones propias que devuelven una colección ahora se
pueden prestar con `&` (antes daba `NOK4008`).

### Sprint 117 — Guardar también la red de clases (2026-11-14)

### Sprint 117 — Guardar también la red de clases (2026-11-14)

> **Incidente, y cómo se cerró.** La herramienta de troceado de literales
> (`herramientas/partir-literales.ps1`) **borró código del fuente**: al reescribir `runtime_ia.cpp` se
> perdió el bloque que acababa de añadir, y con él la corrección del `exp`. Se detectó porque el binario
> de MSVC no encontraba `nooki_ia_clases_guardar` al enlazar, y se comprobó buscando la cadena
> `NOOKICLS` dentro del ejecutable. Se restauró el fuente desde el repositorio, se reinsertó el bloque y
> la herramienta ahora **compara el contenido antes y después** y se niega a escribir si el troceado
> cambiaría una sola línea.

```
redes.guardar_clase(red, &ruta) -> booleano
redes.cargar_clase(&ruta) -> entero
```

Formato propio con cabecera **`NOOKICLS`**, para que no se confundan: cargar el archivo de la red
binaria en la de clases (o al revés) devuelve `0`, igual que un archivo truncado o con una etiqueta
fuera de rango. Se guardan los pesos, los sesgos, las **muestras y sus etiquetas**, así que al cargar se
puede seguir entrenando. Una red de 3 clases con 81 muestras ocupa 3,5 KB.

El ejemplo `09-clases` guarda y recarga, y comprueba que la copia acierta lo mismo. Caso nuevo
`0069-clases-guardar`. **99 casos.**

### Sprint 116 — Clasificar en varias clases (2026-11-14)

### Sprint 115 — Guardar y cargar la red (2026-11-14)

Entrenar cuesta tiempo y al cerrar el programa se perdía todo. Ahora:

```
redes.guardar(red, &ruta) -> booleano
redes.cargar(&ruta) -> entero      // la red, o 0 si el archivo no sirve
```

- **Formato binario** con cabecera `NOOKIRED`, una versión, la forma de la red, los pesos, los sesgos
  y **las muestras**. Un XOR entrenado ocupa 320 bytes.
- Se guardan **también los ejemplos**, así que al cargar se puede **seguir entrenando** donde se dejó.
- Cargar un archivo que no existe, está truncado o no es de Nooki devuelve `0`, sin romper nada.
- Ejemplo `08-red` guarda y recarga la red y comprueba que predice lo mismo; caso `0067-red-guardar`
  para la suite (incluido el rechazo de un archivo ajeno). **97 casos.**

### Sprint 114 — Una red que aprende de verdad (2026-11-14)

### Sprint 114 — Una red que aprende de verdad (2026-11-14)

**El entrenamiento no terminaba.** Escribiendo el ejemplo del círculo, `redes.entrenar(red, 3000, 0.5)`
con 81 ejemplos no acababa en más de diez minutos. La causa: **cada nodo del grafo de autograd se
apilaba en una lista global y no se liberaba nunca**, así que cada pasada creaba decenas de nodos y
`auto_atras` los recorría **todos** para poner los gradientes a cero. Memoria y tiempo crecían de forma
cuadrática: 243 000 pasadas = millones de nodos.

Ahora el entrenamiento marca el principio del grafo antes de cada pasada y lo libera al terminarla, sin
tocar los parámetros. **De más de 10 minutos a 5,7 segundos** el mismo entrenamiento.

Y con eso la red aprende: `ejemplos/08-red` clasifica puntos dentro de un círculo —una frontera que
**ninguna recta separa**, así que hace falta la capa oculta— con error **0,000022** y **81 de 81**
aciertos en 14 segundos. El programa dibuja la frontera que ha aprendido en ASCII.

**Dos fallos más, de paso:**

1. **`booleano == booleano` no compilaba** (`NOK4001`): comparar dos condiciones era imposible.
   Ahora `==` y `!=` funcionan entre booleanos, y el orden (`<`, `>`) sigue siendo un error, que es lo
   correcto.
2. **`nooki.toml` no estaba documentado.** Está en [`docs/PROYECTO.md`](docs/PROYECTO.md): qué es,
   cuándo hace falta, el formato exacto que acepta el parser, cómo resuelve los módulos y lo que
   todavía no hace (versiones, descarga, dependencias transitivas).

Caso nuevo `0066-red-neuronal` (XOR en 2 s, para la suite). **96 casos.**

### Sprint 113 — `construir` entiende las carpetas (2026-11-14)

### Sprint 113 — `construir` entiende las carpetas (2026-11-14)

Estar en `ejemplos/` y escribir `nooki construir` daba «aquí no hay un proyecto», que es verdad y no
ayuda: esa carpeta contiene **siete** programas, uno por subcarpeta. Ahora:

- Si la carpeta tiene subcarpetas con `principal.oki`, el error **las lista** y dice el comando exacto:

```
error[NOK0001]: aquí no hay un proyecto de Nooki
   = ayuda: esta carpeta contiene 7 programas, cada uno en su carpeta:
            01-hola/principal.oki
            …
   = ayuda: «nooki construir 01-hola» construye ese;
            «nooki ejecutar 01-hola/principal.oki» lo ejecuta
```

- **`nooki construir [carpeta]`** acepta una carpeta: construye ese proyecto y deja el ejecutable
  dentro. Ya no hay que cambiar de directorio.

Y de paso: el archivo `ejemplos/hola.oki` que se había quedado con la salida del programa pegada dentro
(`retornar 0empieza`) se ha reparado.

### Sprint 112 — Instalar de verdad (2026-11-14)

`nooki construir` no funcionaba porque **`nooki` no estaba en el `PATH`**: el binario vivía en
`build\windows-mingw\compilador\`, y la documentación daba por hecho que estaba instalado sin decir
cómo. Dos arreglos:

1. **`herramientas\instalar.ps1`**: copia el ejecutable a `%LOCALAPPDATA%\Nooki\bin`, añade esa carpeta
   al `PATH` del usuario (sin administrador, respetando el tipo del valor de registro) y comprueba que
   responde. `-Compilar` compila antes; `-Desinstalar` lo quita y deja el `PATH` **exactamente** como
   estaba (comprobado). El paquete de distribución ahora lo incluye y `INSTALAR.txt` lo usa.
2. **El mensaje de `construir` fuera de un proyecto**: decía «no se encontró el punto de entrada» y
   sugería crear `principal.oki`, sin mencionar `nooki nuevo`. Ahora dice «aquí no hay un proyecto de
   Nooki», distingue si falta el manifiesto, y propone las dos salidas: `nooki nuevo demo` o
   `nooki compilar archivo.oki`.

De paso: el paquete **no incluía `MANUAL.md` ni `REFERENCIA.md`**. Ahora sí (12 documentos).

### Sprint 111 — Colores variados y referencia del lenguaje (2026-11-14)

### Sprint 111 — Colores variados y referencia del lenguaje (2026-11-14)

**Los colores.** `funcion` y `variable` salían del mismo token, así que el editor los pintaba igual.
Ahora cada construcción tiene su categoría y su color: la palabra y el nombre de una función, la de una
variable, los tipos declarados, los tipos primitivos, los genéricos, las llamadas, los métodos, los
campos, las etiquetas de un literal, los literales, los modificadores, los módulos y las referencias
(`&`, `&mutable`, `?`). El plugin pasó de 15 a **26 categorías**, y cada una se puede ajustar en
*Settings → Editor → Color Scheme → Nooki*, con una vista previa que las muestra todas.

**La referencia.** `REFERENCIA.md`: 12 secciones con el léxico, los tipos y sus rangos, la tabla de
precedencia **real** del parser, las sentencias, las declaraciones, los miembros de `texto`/`Vector`/
`Mapa`/`Caja`, la biblioteca (152 firmas), la memoria, los diagnósticos y las herramientas. Las tablas
de palabras reservadas, diagnósticos y biblioteca las **genera** `herramientas/generar-referencia.py`
desde el compilador, así que no pueden desincronizarse.

**Seis fallos que destapó la referencia**, todos arreglados:

1. Los operadores de bits y desplazamientos estaban en el parser y en las asignaciones compuestas, pero
   **no en las expresiones**: `a & b` daba `NOK6003`.
2. `Vector.modificar` y `Vector.vaciar` **no existían** en el runtime: el generador emitía llamadas a
   funciones inexistentes y el C no compilaba. (El manual ya los documentaba: mentía.)
3. `Caja.sacar` y `Caja.poner` tampoco existían, y `imprimir(caja.sacar())` no sabía el tipo.
4. `texto.sin_ultimo()` y `texto.agregar_codigo()` no estaban en la lista de métodos que devuelven
   texto, así que `imprimir(t.sin_ultimo())` generaba C inválido.
5. Los métodos de un mapa o de una caja no tenían tipo, así que no se podían imprimir ni asignar.
6. **Un tipo inventado se aceptaba en silencio**: `variable n: natural = 1` compilaba como `entero`.
   Ahora da `NOK3003`, que es lo que el lenguaje promete: lo que no se puede garantizar se rechaza.

Y dos casos nuevos: `0064-operadores-bits` y `0065-miembros`. **95 casos.**

### Sprint 110 — Plugin de verdad para RustRover (2026-11-14)

### Sprint 110 — Plugin de verdad para RustRover (2026-11-14)

El bundle TextMate no servía en RustRover: **no trae el soporte de TextMate**. Lo comprobé mirando dentro
de un RustRover 2026.1 instalado —`plugins/textmate` no existe—, así que la página *Settings → Editor →
TextMate Bundles* no aparece y no hay nada que instalar desde la tienda.

Así que hay un **plugin de verdad**: `editores/jetbrains/Nooki-1.0.0.jar` (16 KB), con lexer propio,
colores, `Ctrl+/` para comentar y emparejado de llaves.

**Verificado, no supuesto:**

- `.probar-carga.ps1` arranca un RustRover con configuración temporal (sin tocar la del usuario), lee su
  registro y lo cierra. El registro dice `Loaded custom plugins: Nooki (1.0.0)`.
- El lexer se prueba fuera del IDE con `DepurarLexer`: un programa de muestra da **59 tokens y cero
  desconocidos**, con comentarios, textos de tres comillas, `0x1F`, `&&`, `->` y las tildes.
- Se compila **sin Gradle ni descargar el SDK**, contra las bibliotecas del IDE instalado
  (`.\plugin\compilar.ps1`), y el script elige el RustRover y el JDK más nuevos que encuentre.

Un fallo de paso: el `.gitignore` tenía `` `n `` literales en vez de saltos de línea desde hacía tiempo,
así que las reglas `panico/` y `rutas/` no funcionaban. Corregido.

### Sprint 109 — Resaltado para JetBrains (2026-11-14)

Nooki se colorea en **RustRover, IntelliJ, CLion y el resto de IDEs de JetBrains**, con un bundle
TextMate que funciona en todos ellos sin compilar nada:

```
editores/jetbrains/
├── Nooki.tmbundle/          la carpeta que se le pasa al IDE
│   ├── info.plist
│   └── Syntaxes/Nooki.tmLanguage
├── Nooki.tmbundle.zip       lo mismo, para moverlo
└── LEEME.md                 instalación en cuatro pasos
```

- **Una sola definición, tres editores**: `herramientas/generar-gramaticas.py` escribe el JSON de VS
  Code y los dos `.plist` de JetBrains desde la misma tabla de palabras y patrones, así que no pueden
  resaltar cosas distintas.
- **Verificada, no solo generada**: `herramientas/verificar-gramatica.py` comprueba que las **26
  expresiones regulares compilan** y que doce construcciones reales caen en su regla. Se ejecuta sola
  al generar; una expresión mal escrita fallaría en silencio dentro del editor.
- **Las palabras clave son las del lenguaje de verdad**: se quitaron `sinosi`, `intentar`, `atrapar`,
  `asincrono`, `mixin`… que venían del plan original y no existen, y entraron `inseguro`, `asm`,
  `&mutable` y `"""…"""`. El resaltado de Vim tenía el mismo desfase y también se corrigió.
- El `LEEME` de JetBrains explica además cómo conectar **`nooki lsp`** con LSP4IJ para tener errores
  subrayados, autocompletado e ir a la definición.

### Sprint 108 — Manual del lenguaje (2026-11-14)

El manual completo de Nooki: 25 capítulos, de «hola, mundo» a los rasgos, la memoria y la IA. Está en
[MANUAL.md](MANUAL.md).

**Y es verificable**: sus **65 programas** son bloques `nooki` que la suite extrae y compila en cada
`ctest`. Si el lenguaje cambia y un ejemplo se rompe, falla `manual`. La documentación no puede mentir.

Escribirlo destapó **cuatro fallos del lenguaje**, todos arreglados:

1. **`x como flotante` no tenía tipo** en el verificador: cualquier uso de una conversión daba un error
   de tipo falso.
2. **La variable de un `para` sobre `Vector<texto>` se trataba como entero**, así que `para (nombre en
   nombres)` no dejaba usar `nombre` como texto.
3. **`coincide` no contaba la forma `enumeracion.variante`** al comprobar que cubre todos los casos: un
   emparejamiento exhaustivo se rechazaba con `NOK4006`.
4. **Una variante sin datos dentro de una enumeración con datos** generaba C inválido al usarla como
   valor (`forma.sin_forma`). Ahora se construye la estructura con su etiqueta.

Y uno de sintaxis: **`hereda` e `implementa` solo valían en un orden**; ahora valen los dos.

### Sprint 107 — Tres fallos de memoria que destapó el manual (2026-11-14)

Empezando a escribir el manual compilé un programa de muestra con un `Mapa`. **Corrompía el montículo**
y no imprimía nada. Tirando de ese hilo salieron tres fallos de propiedad distintos:

1. **`Mapa.insertar` se quedaba con la clave sin copiarla** y, si la clave ya existía, **liberaba la del
   llamante**. Doble liberación segura. Ahora el mapa guarda su propia copia y nunca libera la ajena.
2. **Asignar un texto nuevo no liberaba el viejo**: `resultado = resultado + " "` filtrada cada vuelta.
   Ahora la asignación pasa por `nooki_texto_asignar`, que libera el búfer anterior — evaluando el valor
   nuevo antes de liberar, que es el orden que importa.
3. **`variable x = *prestamo` aliaseaba el búfer** en lugar de copiarlo, así que los dos lo liberaban.
   Ahora se copia.

Con los tres, los casos de texto y de mapa pasan con ASan **sin una sola marca de corrupción**.

- **Caso `0063-mapa`**: insertar, actualizar la misma clave, buscar, quitar y comprobar que las claves
  siguen siendo del programa. **93 casos.**
- **Limitación anotada**: quedan fugas de unos pocos bytes por **temporales de expresión** —el resultado
  de `textos.unir(...)` pasado directo a `imprimir` no se libera—. No corrompe nada y se cierra el
  programa con memoria del sistema. Está en `DECISIONES.md` (A-18).

### Sprint 106 — Lo que la CI habría encontrado (2026-11-14)

Antes de publicar compilé con **el preset exacto que usa la CI** (`windows-msvc`, Visual Studio 17
2022). Falló, y el fallo era real:

```
error C2026: cadena demasiado grande; caracteres finales truncados
```

**MSVC limita cada literal de cadena a 16 KB.** Los runtimes de interfaz y de IA viven en literales de
30 y 45 KB, así que el compilador **no compilaba con MSVC 2022**, solo con MSVC 2026 (que subió el
límite) y con GCC/Clang. Alguien con Visual Studio 2022 habría encontrado el código sin compilar.

- Los dos literales se parten en trozos de ~7 KB que C++ vuelve a unir al traducir: el contenido es
  idéntico, el límite por token ya no se toca.
- Verificado con los dos MSVC (17 y 18), MinGW, Linux y ASan+UBSan. **17 suites en las cinco cadenas.**

**Las notas de la versión, reescritas.** [NOTAS_DE_VERSION.md](NOTAS_DE_VERSION.md) decía «74 programas,
9 suites, 5 módulos» y listaba la interfaz gráfica y la IA como *pendientes* — cuando ya estaban
hechas. Ahora describe lo que la 1.0 es de verdad: 92 casos, 17 suites, 16 módulos, 13 comandos, y una
tabla honesta de lo que no hay (macOS sin ventanas, PNG, diseñador visual).

Todos los números de ese documento se comprobaron contra el repositorio uno por uno.

### Sprint 105 — Publicación: CI, macOS y el formateador arreglado (2026-11-13)

Un sprint de cierre que encontró dos fallos reales.

- **`verificar` no cargaba los módulos**: daba un falso error («no se ha declarado `ventana`») en
  cualquier programa que usara la biblioteca. Ahora usa el conductor, que carga los módulos igual que
  al compilar, y para antes de generar C. Lo destapó probar el **paquete de distribución**, no la suite.
- **El formateador rompía los textos multilínea**: añadía sangría a las líneas de dentro de un
  `"""..."""`, y esa sangría es contenido del texto. Lo encontró el caso de conformidad `0049-archivo`,
  que lee un archivo con dos líneas. Ahora, dentro de un texto multilínea no se toca nada, y hay una
  prueba que lo cubre.
- **La CI gana la cadena de saneamiento** (ASan + UBSan con avisos como errores) y se le quitan los
  comentarios de sprints viejos. Se comprobaron los presets que referencia: los cuatro existen.
- **macOS compila**: no hay backend de ventanas todavía, así que la capa de UI tiene una rama para
  `__APPLE__` con funciones que devuelven 0 —igual que en una máquina sin pantalla—. Así un programa con
  ventana compila y se prueba en macOS, y la CI no se cae por algo que aún no existe. La rama se
  comprobó con `gcc -fsyntax-only -D__APPLE__`.
- **El código Nooki del repositorio pasa su propio formateador**: ejemplos y los 92 casos de
  conformidad, 107 archivos, formateados y verificados con `--comprobar`.

### Sprint 104 — `nooki documentar` (2026-11-13)

```bash
$ nooki documentar .              # escribe API.md
documentado: API.md (1 archivos)
```

```markdown
## Módulo `geometria.oki`

### Tipos

- **`punto`** (estructura)
  - `x`: entero
  - `y`: entero

### Funciones

- `sumar(a: entero, b: entero) -> entero`
```

- Recorre los `.oki` del directorio, analiza cada uno y escribe **solo lo `publico`**: tipos con sus
  campos, funciones con su firma y variantes de enumeración.
- Un archivo con errores **no se documenta** en lugar de documentarse a medias.
- **`--salida`** escribe por la salida estándar en vez de crear `API.md`.
- Sale con 65 si ningún archivo expone nada.
- Con esto **los 13 comandos de la tabla están implementados**: se retira la prueba de «comando
  todavía no disponible», que ya no tiene caso que probar.

### Sprint 103 — Ir a la definición de variables (2026-11-13)

El servidor ya resuelve también **variables, constantes y parámetros**.

- Con el cursor sobre `total`, salta a la línea donde se declaró.
- Entre varias declaraciones con el mismo nombre —posibles en bloques distintos— se queda con la
  **más cercana por encima del cursor**, que es la que está en juego.
- Cubre `variable`, `constante`, la variable de un `para` y los parámetros de función.
- Caso en la prueba del LSP: la definición de `total` cae en su línea de declaración. **17 suites.**

### Sprint 102 — Formato desde el editor (2026-11-13)

El formateador deja de ser solo un comando y pasa a ser un módulo reutilizable
(`nooki::formato::formatear`), que usan **los dos**: la CLI y el servidor de lenguaje.

- **`textDocument/formatting`**: el editor pide el formato y el servidor devuelve una edición que
  reemplaza el documento; si ya está formateado, devuelve una lista vacía.
- `nooki formatear` usa el mismo código, así que no hay dos formateadores que se puedan
  desincronizar: es la misma función.
- La prueba del LSP comprueba que la edición deja la sangría de cuatro espacios. **17 suites.**

### Sprint 101 — `nooki verificar` (2026-11-13)

```bash
$ nooki verificar ejemplos/01-hola/principal.oki
correcto: ejemplos/01-hola/principal.oki

$ nooki verificar roto.oki; echo $?
error[NOK3001]: no se ha declarado «y»
65
```

- Pasa el archivo por **léxico, parser y verificación**, sin generar C ni llamar al compilador.
- Sale con **0** si está bien, **65** si hay errores y **64** si falta el archivo.
- Es lo que necesita la integración continua para comprobar sin ensuciar el directorio con binarios.
- La prueba de la CLI lo cubre: un archivo correcto, uno con error y la llamada sin argumentos.
  **36 comprobaciones.**

Como `formatear`, el comando estaba en la tabla desde el sprint 59 sin implementarse. Ahora el único
comando pendiente es `documentar`.

### Sprint 100 — Formato: `nooki formatear` (2026-11-13)

```bash
$ nooki formatear ejemplos/06-tareas/principal.oki
formateado: ejemplos/06-tareas/principal.oki

$ nooki formatear principal.oki --comprobar   # 65 si no está formateado
```

- **Sangría canónica** de cuatro espacios por nivel, calculada con las llaves.
- **No toca el contenido**: ni comentarios, ni textos, ni el orden de nada. Solo los espacios del
  principio y del final de cada línea.
- **Las llaves dentro de comentarios y de textos no cuentan**: `// { falsa }` y `"llave { dentro"` se
  ignoran al medir la profundidad. Es el detalle que rompe la mitad de los formateadores sencillos.
- **`--comprobar`** no escribe nada y sale con 65 si algún archivo cambiaría: sirve para la integración
  continua.
- **Idempotente**: formatear lo formateado no cambia nada, y hay una prueba que lo comprueba.
- La prueba de la CLI incluye un archivo desordenado, comprueba la sangría resultante y que la segunda
  pasada no cambia nada. **31 comprobaciones.**

El comando estaba en la tabla desde el sprint 56 y nunca se había implementado: aparecía como
«disponible» y daba un error interno. Ahora está.

### Sprint 99 — Ir a la definición (2026-11-13)

`nooki lsp` completa la tríada del editor: diagnósticos, autocompletado y **navegación**.

- **`textDocument/definition`**: con el cursor sobre un nombre, el editor salta a su declaración —
  funciones, tipos y enumeraciones del documento.
- Se resuelve con el AST ya analizado: no hay índice aparte que se pueda desincronizar.
- La prueba del LSP pide la definición de una llamada y comprueba que cae en la línea de la
  declaración. **17 suites.**

Pendiente del servidor: formato automático y resaltado semántico.

### Sprint 98 — Autocompletado en el editor (2026-11-13)

`nooki lsp` ya no solo subraya errores: **también completa**.

- **`textDocument/completion`**: ofrece las palabras del lenguaje, los tipos, las enumeraciones y las
  funciones declaradas en el documento, **filtradas por lo que se está escribiendo**.
- El servidor guarda el texto de cada documento abierto, así que puede analizarlo al completar.
- Sin prefijo se ofrecen palabras y funciones; al escribir dos letras, la lista se reduce a lo que
  empieza así.
- La prueba del LSP comprueba que aparecen `funcion` (palabra) y `suma` (la función declarada en el
  documento). **17 suites.**

**Lo que sigue faltando**: ir a la definición, formato automático y resaltado semántico. Están en
`DECISIONES.md` como pendientes, no como olvidos.

### Sprint 97 — Cobertura de diagnósticos y código de salida (2026-11-13)

Tres cosas pequeñas que cierran huecos reales.

- **El compilador sale con el código documentado**: un fallo de compilación devolvía `1`; ahora devuelve
  **65** (`DATOS_INCORRECTOS`), que es lo que dice `nooki ayuda`. Quien automatiza puede distinguir un
  uso incorrecto (64) de una compilación fallida (65).
- **`NOK6003` y `NOK3008` pasan a estar probados**: un literal de carácter y un módulo que no existe se
  comprueban en la suite del compilador, que es la que ejercita el backend y la carga de módulos (el
  arnés de conformidad solo llega al front end).
- **Se retira `NOK8001`** («la variable nunca se modifica»). Estaba en el catálogo pero **nunca se
  emitía**: era código muerto y documentación que prometía algo inexistente. Intenté implementarlo y
  daba **31 avisos falsos** en un ejemplo de 200 líneas. Un aviso que grita en falso es peor que no
  tenerlo, así que fuera. Quedan **46 códigos**, 40 con prueba.

**Lo que no se puede probar sin trampas**: `NOK6001` (no hay compilador de C) y `NOK6002` (el
compilador de C falla) exigen una máquina sin `cc` o un `cc` roto, y el compilador valida el candidato
antes de usarlo. Quedan documentados como revisados a mano.

### Sprint 96 — Campo de texto: escribir en la interfaz (2026-11-13)

La ausencia más grande de la UI, cerrada.

```aura
usar controles

variable nombre = nuevo campo { x: 20, y: 20, ancho: 280, alto: 40,
                                contenido: "" + "", fondo: 0x313244, color: 0xFFFFFF,
                                marcador: "Escribe tu nombre" + "" }

// En el bucle de eventos:
si (ventana.es_letra(cual_evento)) {
    nombre.escribir(ventana.evento_letra(v))
}
si (ventana.es_tecla(cual_evento) && ventana.evento_tecla(v) == 8) {
    nombre.borrar()
}
```

- **Evento nuevo: `es_letra`** (clase 6). En Windows llega de `WM_CHAR`; en X11, de `XLookupString`,
  que ya traduce teclas y acentos.
- **`campo`** hereda de `control`: `contenido`, `escribir(codigo)`, `borrar()`, `limpiar_campo()` y
  `dibujar` con marcador cuando está vacío.
- **UTF-8 de verdad**: `texto.agregar_codigo(codigo)` codifica el punto de código y
  `texto.sin_ultimo()` quita **un carácter completo**, no un byte. Escribir `ñ` y borrarla deja el
  texto intacto, que es justo lo que rompe la mitad de los editores.
- Caso `0062-campo-texto`. **92 casos.**

Con esto la interfaz alcanza lo que se pedía al principio: controles, texto y teclado, como en
cualquier biblioteca de escritorio.

### Sprint 95 — A-17 resuelto: la propiedad al salir de una función (2026-11-13)

La fuga de A-17 no era de `pln.distancia`: era **del generador, en toda función que devolvía un valor
teniendo algo con dueño**.

```aura
funcion calcula() -> entero {
    variable v: Vector<entero> = Vector<entero>.nuevo()
    v.agregar(1)
    retornar v.largo()      // v no se liberaba nunca
}
```

**Dos fallos encadenados:**

1. **`retornar v.largo()` conservaba `v`** como si se devolviera, porque el nombre aparece como objeto
   de la llamada. Ahora solo se conserva lo que **viaja dentro** del valor devuelto: los argumentos de
   una llamada, sí; el objeto de un método, no.
2. **Al liberar antes de devolver, se liberaba demasiado pronto**: `soltar(&v); return v.largo();` lee
   memoria liberada. Ahora el valor se evalúa en un temporal, se libera lo que toca y se devuelve el
   temporal.

Resultado: **AddressSanitizer ya no encuentra ninguna fuga** en el caso que la destapó, y `archivo.leer`
—que devuelve un texto dentro de un `Bien`— sigue conservándolo como debe.

Las cuatro cadenas en verde.

### Sprint 94 — Redes: la IA queda cerrada (2026-11-13)

Una red de una capa oculta que **aprende XOR**, el problema que no se puede resolver sin neuronas
ocultas.

```aura
usar redes

variable red = redes.nueva(2, 4)                 // 2 entradas, 4 ocultas
redes.ejemplo(red, 0.0, 1.0, 0.0, 0.0, 1.0)      // 0 xor 1 = 1
redes.entrenar(red, 3000, 1.0)                   // 3000 vueltas, tasa 1.0
imprimir(redes.predecir(red, 1.0, 0.0, 0.0, 0.0))  // 0,98
```

Resultado real tras entrenar: error de **0,261 a 0,0003** y las cuatro combinaciones acertadas
(0,007 · 0,985 · 0,985 · 0,023).

- **`nueva(entradas, ocultas)`**, `ejemplo`, `entrenar`, `predecir`, `error` y `liberar`.
- Activación **sigmoide** y **descenso de gradiente** por ejemplo, con pesos inicializados de forma
  pseudoaleatoria **determinista**: la red parte siempre igual, así que se puede probar.
- Los pesos son **nodos de autograd**: entrenar es construir la expresión de cada ejemplo, llamar a
  `atras` y mover los pesos. No hay un segundo motor de derivadas.
- Caso `0061-redes`, que comprueba la **clasificación** y no los decimales, para que valga en cualquier
  plataforma. **91 casos.**

**Con esto termina la fase de IA**: tensores, estadística, flotantes, PLN, tablas, autograd y redes.

### Sprint 93 — Autograd (2026-11-13)

Derivadas automáticas: el lenguaje ya puede calcular gradientes.

```aura
usar autograd

variable x = autograd.dato(2.0)
variable y = autograd.suma(autograd.producto(x, x), autograd.dato(3.0))   // x² + 3
autograd.atras(y)
imprimir(autograd.gradiente(x))    // 4  →  2x
```

- **Operaciones**: `suma`, `resta`, `producto`, `division`, `relu`, `sigmoides`.
- **`atras(y)`** recorre el grafo en orden topológico (DFS post-orden) y reparte los gradientes.
- Verificado con derivadas conocidas: `d(x²+3)/dx = 4` en x=2; `d(3x)/dx = 3`; `relu(-1)` da 0;
  `sigmoides(0)` da valor `0,5` y gradiente `0,25`.
- Caso `0060-autograd`. **90 casos.**

**Dos cosas que hubo que arreglar:**

1. **La FFI no mapeaba `flotante`** en las firmas externas: declaraba `long long` y devolvía basura
   (el valor impreso era la dirección del nodo). Ahora `flotante` es `double` también ahí.
2. **El primer diseño usaba «áreas» para agrupar nodos** y no funcionaba: cada `dato` creaba la suya, así
   que `atras` limpiaba solo una parte del grafo y los gradientes se acumulaban entre cálculos
   (`3x` daba 7 en vez de 3). Ahora **todos los nodos viven en una lista global**: limpiar y liberar son
   recorridos de esa lista, sin importar cómo se construyó el grafo.

**Nota**: `liberar` suelta **todos** los nodos vivos, no solo los del grafo indicado. Está documentado en
el módulo; es el precio de no tener recolector de basura.

### Sprint 92 — Tablas: el DataFrame (2026-11-13)

```aura
usar tabla

variable t = tabla.crear(4)
tabla.columna_texto(t, &"producto")
tabla.columna_entero(t, &"ventas")
tabla.poner_texto(t, 0, 0, &"pan")
tabla.poner_entero(t, 0, 1, 10)

imprimir(tabla.suma(t, 1))        // 100
imprimir(tabla.media(t, 1))       // 25000 → 25
variable madrid = tabla.filtrar(t, 0, &"pan")
variable ordenada = tabla.ordenar(t, 1)
variable resumen = tabla.contar_por(t, 0)
```

- **Columnas con nombre**, numéricas o de texto, con `crear`, `columna_entero`, `columna_texto`,
  `poner_*`, `valor_*`, `filas`, `columnas` y `nombre_columna`.
- **Resúmenes**: `suma`, `media` (por mil), `minimo`, `maximo`.
- **`filtrar`** por valor de texto, **`ordenar`** por columna numérica y **`contar_por`**, que devuelve
  una tabla con «valor» y «cuenta».
- Caso `0059-tabla`: cuatro filas, filtro por ciudad, orden y agrupación. **89 casos.**

**Un rasgo nuevo del lenguaje**: **las funciones `externo` ya pueden devolver `texto`**. El generador
copia la cadena de C a un texto de Nooki, porque el búfer puede ser de la biblioteca y no nuestro. Era
la pieza que faltaba para que un módulo envuelva datos que vienen de C.

**Y la interfaz en Linux funciona de verdad**: WSLg da un servidor X real, así que la cadena de Linux
ahora abre ventanas y lee píxeles. Eso destapó tres cosas:

1. **`XGetImage` abortaba el programa** cuando la ventana no estaba visible (`BadMatch`). Ahora hay un
   manejador de errores de X y reintentos con `XSync`.
2. **El primer `Expose` borraba lo pintado**: la prueba tiene que procesar los sucesos y volver a
   dibujar antes de leer.
3. **La prueba de ventana ya verifica píxeles en las dos plataformas**, no solo en Windows.

### Sprint 91 — PLN: lenguaje natural (2026-11-13)

```aura
usar pln

imprimir(pln.normalizar(&frase))        // minúsculas y un solo espacio
variable palabras = pln.tokenizar(&frase)
variable cuentas = pln.contar(&palabras)
imprimir(pln.distancia(&"casa", &"cama"))   // 1
imprimir(pln.jaccard(&f1, &f2))             // por mil
```

- **`normalizar`**, **`tokenizar`**, **`contar`** (mapa de frecuencias), **`unicas`**, **`distintas`**,
  **`cuantas`**, **`contiene_texto`**.
- **`distancia`**: distancia de edición entre dos textos.
- **`jaccard`**: parecido por mil, intersección sobre unión de palabras distintas.
- Caso `0058-pln`. **88 casos.**
- Todo en Nooki, sin una línea de C.

**Cuatro fallos del lenguaje que aparecieron al escribirlo**, todos arreglados:

1. **`(*vector).largo()` no se traducía**: el generador no sabía sacar el tipo apuntado ni pasar el
   puntero al runtime. Ahora un desreferenciado vale para cualquier préstamo, no solo para `texto`.
2. **`&x` no tenía tipo puntero**, así que las comprobaciones de «este argumento es un préstamo» no
   acertaban. Ahora `&x` tiene el tipo de `x` más `*`.
3. **Los argumentos prestados a métodos de texto** llegaban como puntero donde el runtime espera el
   valor; ahora se desreferencian.
4. **Asignar un `Vector` era un fallo de memoria**: el generador copiaba la estructura y dejaba que la
   limpieza del bloque liberara el búfer que el destino acababa de recibir. En Windows «funcionaba»; en
   Linux daba resultados falsos. Lo encontró **AddressSanitizer**: *heap-use-after-free*. Ahora la
   asignación **libera el destino, mueve y vacía el origen**, como manda la propiedad.

**Incidencia A-17**: al reasignar una colección dentro de un bucle queda sin liberar el último búfer
(64 bytes por llamada en `distancia`, detectado por LeakSanitizer). No corrompe nada y no afecta a los
resultados, pero está anotada con su reproducción.

### Sprint 90 — Flotantes de verdad (2026-11-13)

El lenguaje gana **coma flotante**. Era lo que faltaba para que el autograd y las redes puedan dar
pasos pequeños: con enteros por mil, `0.001` es `1` y `0.0001` es `0`.

```aura
variable x = 3.5
variable y = x * 2.0        // 7
imprimir(x / 2.0)           // 1,75
imprimir(x - 10.0)          // -6,5
imprimir(3.1416 * 2.0 * 2.0)  // 12,5664

variable n = 7 como flotante
variable m = 3.9 como entero   // 3
```

- **Tipo `flotante`** en el verificador y `double` en el C generado.
- **Literales, variables, parámetros, retornos**, suma, resta, multiplicación, división, comparación
  y negación.
- **`como`**: conversión explícita entre `entero` y `flotante`, en las dos direcciones. **No hay
  conversión implícita**: mezclar `entero` y `flotante` sigue siendo un error, como en el resto del
  lenguaje.
- **Se imprime con coma decimal** y sin ceros de más: `3,5`, `1,75`, `7`, `-0,5`.
- Caso `0057-flotante`. **87 casos.**

**Y un arreglo en la prueba de ventana**: era inestable (1 de cada 10 ejecuciones fallaba por una
carrera al abrir y cerrar la ventana demasiado rápido). Ahora espera 50 ms y atiende 10 fotogramas en
lugar de 3, y se ha ejecutado cinco veces seguidas sin fallar.

**Lo que todavía no hay**: `flotante` en vectores, mapas y campos de estructura; y funciones
matemáticas (`raíz`, `seno`, `potencia`) para flotantes. Llegarán cuando se usen.

### Sprint 89 — Estadística y activaciones (2026-11-13)

Lo que convierte una matriz en algo útil para datos.

```aura
imprimir(tensor.media(a))        // 2500  → 2.5
imprimir(tensor.varianza(a))     // 1250  → 1.25
imprimir(tensor.desviacion(a))   // 1118  → 1.118

variable n = tensor.normalizar(a)   // valores entre 0 y 1000
variable r = tensor.relu(a)         // max(0, x)
variable s = tensor.sigmoides(a)    // 731 para la entrada 1
variable p = tensor.softmax(x)      // 90, 244, 666 — suman 1000
```

**Sin coma flotante en el lenguaje**, todo lo que tiene decimales se da **por mil**: `2500` es `2.5`.
Es exacto, se compara con enteros y se documenta en el módulo. Cuando el generador soporte `flotante`
de verdad, la escala desaparecerá sin cambiar los programas.

- **Estadística**: `minimo`, `maximo`, `media`, `varianza`, `desviacion`, `normalizar`.
- **Activaciones**: `relu`, `sigmoides` y `softmax`. El softmax resta el máximo antes de exponenciar
  —para que no se dispare— y **reparte los puntos del redondeo para que siempre sume 1000**.
- Caso `0056-estadistica` con valores calculables a mano: `[1,2,3,4]` da media 2500, varianza 1250 y
  desviación 1118; `sigmoides(1) = 731`, `sigmoides(4) = 982`; `softmax([1,2,3]) = 90, 244, 666`.

**Un fallo de escala**: la desviación calculaba la raíz de la varianza *ya escalada*, así que daba 35
en vez de 1118. Lo encontró la comprobación a mano del caso.

### Sprint 88 — Tensores: empieza la fase de IA (2026-11-13)

El primer pilar de la IA: **matrices con forma**.

```aura
usar tensor

variable a = tensor.lleno(2, 3, 1)
tensor.poner(a, 0, 0, 1)
imprimir(tensor.leer(a, 1, 2))

variable t = tensor.transpuesta(a)
variable p = tensor.producto(a, t)      // (2x3) por (3x2) = (2x2)
imprimir(tensor.determinante(d))
```

- **`ceros`, `lleno`, `identidad`**, y `filas`, `columnas`, `leer`, `poner`, `liberar`.
- **`suma`, `resta`, `escalar`, `producto`, `transpuesta`**, `suma_total` e `iguales`.
- **`determinante`** por eliminación de Gauss con pivoteo parcial.
- **Todas las operaciones comprueban las formas**: si no encajan devuelven 0 en vez de leer fuera de
  sitio. Un tensor de tamaño absurdo tampoco se crea.
- Enteros de 64 bits: es aritmética exacta, sin sorpresas de coma flotante. Cuando haga falta
  precisión real, se añadirá un tipo de coma flotante.
- Caso `0055-tensor`: forma, traspuesta, producto, determinante, identidad e igualdad. **85 casos.**

**El arreglo de raíz**: el runtime de tensores se compila aparte y le faltaban las cabeceras — el mismo
error que en la capa de ventanas y en la de imágenes, tres veces. Ahora el conductor **antepone
`<stdint.h>`, `<stdlib.h>`, `<stdio.h>`, `<string.h>` y `<stddef.h>` a todo archivo de runtime**, así
que la clase de fallo desaparece en lugar de repetirse.

### Sprint 87 — Cierre de la interfaz: una aplicación de verdad (2026-11-13)

El último sprint de UI no añade funciones: demuestra que lo construido sirve.

**`ejemplos/06-tareas`**: una lista de tareas con cinco tareas, casillas, barra de progreso, foco que
se recorre con **Tab**, marcar con **Intro** y salir con **Esc**. Usa `ventana`, `diseno`, `controles`,
`accesibilidad` y el ratón. ~200 líneas de Nooki.

**La captura es real**: `herramientas/capturar.ps1` abre el programa, espera su ventana, la fotografía
con `PrintWindow` y lo cierra. La imagen está en `docs/captura-tareas.png` y en el LEEME de ejemplos.

**Y al hacerla apareció un fallo de verdad**: la ventana no se repintaba. El programa dibujaba una vez
y, si algo tapaba la ventana o el sistema pedía `WM_PAINT`, el área quedaba en blanco. La captura salió
vacía la primera vez, que es exactamente lo que le pasaría a un usuario al mover la ventana.

- **Nuevo**: `ventana.repintar(v)` devuelve verdadero cuando hay que volver a dibujar. En Win32 lo marca
  `WM_PAINT` (validando la región con `BeginPaint`/`EndPaint`); en X11, `Expose`.
- La aplicación de ejemplo redibuja cuando hace falta y solo entonces.

**`docs/INTERFAZ.md`**: la guía de interfaz con los cinco módulos, el esqueleto, **las cinco trampas**
(repintar, `presentar` inerte en Windows, `abrir` puede devolver 0, `libx11-dev`/`xvfb-run` y las
palabras reservadas que chocan con nombres de controles) y una tabla honesta de lo que falta: entrada
de texto, PNG, recolocación al redimensionar, varias ventanas, antialiasing y macOS.

**`GUIA.md`** gana el capítulo 12, «Una ventana», con un programa completo y ejecutable.

### Sprint 86 — Imágenes (2026-11-13)

El séptimo pilar de la UI: **cargar y dibujar imágenes**.

```aura
usar imagenes

variable icono = imagenes.cargar(&ruta)
si (imagenes.cargada(icono)) {
    imprimir(imagenes.ancho(icono))
    imprimir(imagenes.color_en(icono, 0, 0))
    imagenes.dibujar(pantalla, icono, 10, 10)
    imagenes.soltar(icono)
}
```

- **BMP de 24 o 32 bits sin comprimir**, leído a mano: cabecera, filas de abajo arriba, relleno de
  fila y píxeles en `0xRRGGBB`. Si el archivo no es BMP, devuelve 0 en lugar de fallar.
- **`color_en(x, y)`** permite comprobar la imagen sin dibujarla: la prueba carga un BMP 4×3 generado al
  vuelo y verifica los cuatro colores exactos (rojo, verde, azul, blanco), las dimensiones y que fuera
  de rango devuelve -1.
- **Y que un archivo que no es BMP se rechaza**, no se acepta a medias.
- **Suite `imagenes`**: 17 suites.

**La decisión del formato**: PNG habría costado un sprint entero solo para el decodificador (inflate y
filtros) o una dependencia externa. BMP cabe en un sprint, no añade nada y deja la puerta abierta a PNG
detrás de la misma API.

**Un fallo de plataforma más**: el cargador usaba `intptr_t` sin incluir `<stdint.h>`, y las funciones
quedaron `static`, invisibles desde el programa generado. Las dos cosas las señaló Linux.

### Sprint 85 — Accesibilidad (2026-11-13)

El sexto pilar de la UI: que la interfaz se pueda leer y recorrer con el teclado.

```aura
usar accesibilidad

variable color_texto = accesibilidad.texto_sobre(color_boton)   // negro o blanco
si (accesibilidad.legible(texto, fondo)) { ... }

variable estado_foco = nuevo foco { indice: 0, total: 3 }
estado_foco.siguiente()      // Tab
estado_foco.anterior()       // Mayús+Tab
```

- **`luminancia(color)`**: 0 negro, 255 blanco, con la fórmula YIQ (enteros, sin coma flotante).
- **`legible(texto, fondo)`**: diferencia de luminancia ≥ 128. **No es el contraste WCAG exacto** —eso
  exige corrección gamma con potencias— pero es una heurística honesta que se calcula con enteros y
  acierta en los casos normales. Está dicho en el módulo y en la especificación.
- **`texto_sobre(fondo)`**: devuelve negro o blanco, el que más contraste dé. Es lo que usa el ejemplo
  para el texto de los botones.
- **`foco`**: índice que envuelve entre `total` controles, con `siguiente`, `anterior`, `es_actual` y
  `primera`. Con `total = 0` no rompe: se queda en 0.
- Caso `0054-accesibilidad`, y el ejemplo `05-ventana` dibuja un **contorno de foco** alrededor del
  control activo.

Todo el módulo es computación pura, así que se prueba como el layout: sin ventana.

### Sprint 84 — A-16 resuelto: la cola de eventos no se inicializaba (2026-11-12)

La violación de acceso que aparecía al lanzar un programa con ventana desde `cmd /c` tenía una causa
simple y vergonzosa: **la cola de eventos nunca se inicializaba al crear la ventana**. `cuenta` quedaba
con lo que hubiera en el montículo, así que el primer `evento()` leía fuera del arreglo.

- Por qué "a veces funcionaba": con memoria fresca del sistema, `cuenta` salía a cero y todo iba bien.
  Bajo `cmd /c`, con el montículo ya usado, salía basura.
- **Cómo se encontró**: compilando el programa con **AddressSanitizer** y ejecutándolo en Linux bajo
  **Xvfb** (pantalla virtual). La traza señaló la línea exacta: `estado->cuenta` con `estado` apuntando
  a memoria no válida.
- **Arreglo**: inicializar `cabeza`, `cuenta` y `ultimo` al crear la ventana, en las dos plataformas.
- **`prueba_ventana` vuelve a ejecutar el programa** y comprueba lo que imprime: que se abre, que el
  rectángulo rojo y el fondo blanco llegan a los píxeles, que la cola está vacía sin entrada y que se
  cierra. A-16 **cerrado**.

**Lección**: el fallo no dependía del lanzador, sino de si el montículo devolvía memoria a cero. Un
síntoma que "desaparece" al cambiar cómo se ejecuta el programa casi siempre es memoria sin
inicializar.

### Sprint 83 — Eventos (2026-11-12)

El quinto pilar de la UI: **el ratón y el teclado llegan al programa**.

```aura
mientras (ventana.viva(v)) {
    variable e = ventana.evento(v)
    mientras (e != 0) {
        si (ventana.es_pulsacion(e)) {
            si (boton.pulsado(ventana.evento_x(v), ventana.evento_y(v))) { ... }
        }
        e = ventana.evento(v)
    }
    ventana.pausa(16)
}
```

- **Cola de 64 eventos** en la capa C: el `WindowProc` de Win32 y el bucle de X11 encolan movimiento,
  pulsación, liberación, tecla y cierre. Nada se pierde entre fotogramas.
- **API en Nooki**: `evento`, `evento_x`, `evento_y`, `evento_tecla`, y predicados legibles
  (`es_pulsacion`, `es_tecla`, …) en lugar de números sueltos.
- Los controles del sprint anterior ya se pueden usar de verdad: el ejemplo de la ventana responde al
  ratón.

**Un fallo real, encontrado y no resuelto del todo (A-16)**: `presentar` llamaba a `GdiFlush`, que
invalidaba el contexto de dibujo y hacía terminar el programa con violación de acceso. Ahora no hace
nada en Windows (en GDI el dibujo llega a la ventana al hacerlo) y sigue haciendo `XFlush` en X11.

**Y una incidencia abierta**: un programa con ventana lanzado desde `cmd /c` termina con
`0xC0000005`; lanzado desde una consola normal funciona. No he conseguido aislar la causa. La prueba
`prueba_ventana` comprueba lo que sí es determinista —que un programa con ventana compila y enlaza— y
la incidencia queda anotada con su reproducción exacta en lugar de escondida.

**Fallo de plataforma**: en la rama de X11 la función de encolar eventos se declaraba antes del tipo de
la ventana. Solo lo ve Linux al compilar un programa con ventana.

### Sprint 82 — Controles (2026-11-12)

El cuarto pilar de la UI: **widgets**. Con ventana, dibujo y layout, ya se pueden construir controles.

```aura
usar controles

variable b = nuevo boton { x: 10, y: 10, ancho: 120, alto: 40,
                           contenido: "Aceptar", fondo: 0x89B4FA, color: 0x11111B }
b.dibujar(v)
si (b.pulsado(15, 20)) { ... }
```

- **`control`** como base: posición y tamaño, y `contiene(px, py)` con intervalo **semiabierto**
  (`x ≤ px < x+ancho`), que es lo que hace que dos controles pegados no se solapen.
- **`etiqueta`**, **`boton`** y **`barra`** (progreso) heredan de `control` y sobrescriben `dibujar`.
- **La geometría se prueba sin ventana**: el caso `0053-controles` comprueba pulsaciones dentro, fuera
  y en los dos bordes. **83 casos.**

**Y tres cosas del lenguaje que hubo que arreglar para que esto funcionara:**

1. **`&objeto.campo` no estaba permitido** (`NOK4008`): prestar un campo es un préstamo de un sitio
   válido. Ahora se permite y el dibujo de texto puede prestar `este.contenido` sin copiarlo.
2. **Llamar a un método heredado desde `este`** generaba una llamada a la clase derivada, que no lo
   tiene. Ahora `este` se trata como lo que es —una referencia— y la llamada va por la tabla virtual.
3. **Pero solo en clases**: una estructura con métodos no tiene tabla virtual, y el cambio anterior la
   rompía. La comprobación de «esto tiene tabla» pasó a exigir que el tipo sea una clase.

**Y una dependencia nueva en Linux**: compilar un programa con ventana necesita `libx11-dev`, así que
la CI lo instala.

### Sprint 81 — Layout (2026-11-12)

El tercer pilar de la UI: **colocar las cosas**. Y, a diferencia de los dos anteriores, esto es
computación pura en Nooki: no toca C ni necesita ventana para probarse.

```aura
usar diseno

variable c = nuevo columna { x: 0, y: 0, ancho: 480, alto: 320, usado: 0, separacion: 8 }
variable y1 = c.hueco(40)   // 0
variable y2 = c.hueco(40)   // 48
si (c.cabe(40)) { ... }
```

- **`columna` y `fila`** como clases: reparten hijos en un eje, con separación, y llevan la cuenta de lo
  usado.
- **`hueco(medida)`** devuelve el desplazamiento del siguiente hijo y avanza el cursor; **`cabe`** dice
  si queda sitio; **`reiniciar`** vuelve a empezar.
- **`centrar(disponible, medida)`** y **`repartir(disponible, partes)`** para lo de todos los días.
- Caso `0052-diseno`: doce comprobaciones de reparto, centrado y desbordamiento.
- **Ejemplo `05-ventana`**: una ventana real con título, subtítulo, tres botones y una barra de
  progreso, todo colocado con el módulo de diseño y dibujado con el de dibujo. Cierra sola.

Es el primer sprint de UI que se prueba sin ventana: el layout es aritmética, y por eso entra en
conformidad como cualquier otra función.

### Sprint 80 — Dibujo (2026-11-12)

El segundo pilar de la UI: **pintar dentro de la ventana**, sin añadir una dependencia.

```aura
ventana.limpiar(v, 0xFFFFFF)
ventana.rectangulo(v, 10, 10, 100, 50, 0xFF0000)
ventana.linea(v, 0, 200, 319, 200, 0x0000FF)
ventana.escribir_texto(v, 20, 100, &saludo, 0x000000)
ventana.presentar(v)
```

- **`limpiar`, `rectangulo`, `linea`, `escribir_texto` y `presentar`**, con colores `0xRRGGBB`.
- **Implementado con lo que ya hay**: GDI en Windows (`FillRect`, `CreatePen`, `TextOut`) y X11 en
  Linux (`XFillRectangle`, `XDrawLine`, `XDrawString`). Cero dependencias nuevas.
- **`color_en(manejador, x, y)` lee un píxel** — es lo que hace que el dibujo se pueda probar de verdad:
  `prueba_ventana` pinta un rectángulo rojo sobre fondo blanco y **comprueba los dos colores**.
- `prueba_ventana` reporta ahora: «se abre, dibuja, atiende 20 fotogramas y se cierra» (escritorio) o
  «sin entorno gráfico, la capa responde con 0» (contenedor). Verde en las tres cadenas.

**La decisión de Skia**: el plan pedía Skia. Se ha empezado con GDI y X11 porque Skia son cientos de
miles de líneas que romperían la regla de cero dependencias y multiplicarían el trabajo de
distribución. La API está pensada para que Skia pueda entrar detrás sin cambiar los programas.

**Fallos encontrados**: un `#include <stdlib.h>` que faltaba en la rama de X11 y dos funciones de color
duplicadas al insertar el dibujo en las dos plataformas. Los detectó la cadena de Linux, que hasta
ahora solo compilaba el compilador y no un programa con ventana.

### Sprint 79 — Ventanas (2026-11-12)

El primer pilar de la UI: **una ventana de verdad desde Nooki**, con su bucle de eventos.

```aura
usar ventana

variable v = ventana.abrir(&titulo, 480, 320)
mientras (ventana.viva(v) && fotogramas < 30) {
    si (ventana.siguiente_evento(v) == 1) {
        romper
    }
    ventana.pausa(16)
}
ventana.cerrar(v)
```

- **Capa C por plataforma**: Win32 en Windows y X11 en Linux/Unix, dentro del mismo archivo de
  runtime y separadas por `#ifdef`. macOS llegará con su backend.
- **Solo se compila si se usa**: el conductor detecta el módulo `ventana` y añade el archivo C y las
  bibliotecas del sistema (`user32`+`gdi32` o `X11`) a la orden del compilador de C. Un programa sin
  interfaz no paga nada.
- **Sin entorno gráfico no falla**: `abrir` devuelve 0 y el programa decide. Así una prueba se ejecuta
  igual en un servidor que en un escritorio.
- **`prueba_ventana`**: compila, ejecuta y acepta las dos salidas —«se abre y se cierra» en un
  escritorio, «sin entorno gráfico» en un contenedor—. **16 suites.**

**Dos fallos que aparecieron al montarlo:**

1. **Colisión de nombres**: la función `viva` del módulo se compilaba como `nooki_ventana_viva`, el
   mismo nombre que el extern que envolvía, así que **se llamaba a sí misma** en recursión infinita y
   consumía la CPU sin parar. La capa C pasa a llamarse `nooki_ui_ventana_*`.
2. **Un archivo llamado como el módulo** (`ventana.oki` con `usar ventana`) se rechazaba como
   auto-importación. Ahora ese candidato se ignora y la búsqueda continúa hasta el módulo de la
   biblioteca.

### Sprint 78 — Estabilización (2026-11-12)

Un sprint que no añade lenguaje y cierra lo abierto.

- **La API de la biblioteca queda congelada por una prueba**: el caso `0051-api-biblioteca` llama a
  **todas** las funciones públicas de los seis módulos. Si alguien cambia una firma o borra una función,
  ese caso deja de compilar. Es la forma barata de que la API no se rompa por descuido: el compilador lo
  dice en cada `ctest`.
- **Distribución**: `herramientas/empaquetar.ps1` y `empaquetar.sh` producen
  `dist/nooki-1.0.0-<plataforma>.zip` o `.tar.gz` con el binario, diez documentos, los ejemplos, los
  editores, la especificación y un `INSTALAR.txt` con los pasos. Verificado: el binario empaquetado
  compila y ejecuta un ejemplo.
- **Caso `0019-asm-fuera-de-inseguro`**: el último código nuevo sin prueba, cerrado. **81 casos.**
- **Auditoría de diagnóstico**: 36 de 47 códigos tienen caso. Los 11 restantes son rutas de integración
  —CLI, carga de módulos, símbolo privado, fallos del compilador de C y avisos— y quedan en A-12 con su
  motivo. No hay código muerto.

### Sprint 77 — Biblioteca: archivos y fechas (2026-11-12)

Un lenguaje sin archivos no sirve para programas de verdad. Ahora los tiene, con la API de errores del
lenguaje:

```aura
usar archivo

coincide (archivo.leer(&ruta)) {
    caso Bien(contenido) => imprimir(contenido)
    caso Mal(motivo) => imprimir(motivo)
}
```

- **`archivo`**: `leer`, `escribir` y `existe`, con un enumerado `lectura` que lleva `Bien(contenido)` o
  `Mal(motivo)`; el `?` del lenguaje funciona sobre él.
- **Tres funciones integradas** en el runtime: `leer_archivo`, `escribir_archivo` y `existe_archivo`.
  Leen el archivo entero y son las que un módulo de Nooki envuelve — el lenguaje no gana magia, gana
  una API.
- **`fechas` con aritmética**: `a_dias`, `a_anio`, `a_mes`, `a_dia`, `sumar_dias` y `dia_de_la_semana`.
  Una fecha es un entero: días desde el 1 de enero de 1970. Verificado con valores conocidos
  (`2000-01-01` = 10957, y el 12 de noviembre de 2026 cae en jueves).
- Casos `0049-archivo` y `0050-fechas`. **79 casos de conformidad.**

**Y dos fallos que aparecieron al escribir los casos:**

1. **Los textos multilínea estaban rotos desde el principio**: `"""..."""` conservaba dos comillas a cada
   lado, porque el analizador quitaba solo una. Ahora se quitan las tres.
2. **El impresor del AST no conocía `inseguro` ni `asm!`** (del sprint anterior): `nooki ast` no las
   mostraba y la compilación con avisos como errores fallaba. Lo detectó la cadena de saneamiento.

### Sprint 76 — Servidor de lenguaje (2026-11-12)

`nooki lsp` habla el **Language Server Protocol** por la entrada y salida estándar: cualquier editor con
cliente LSP puede colorear errores mientras se escribe, sin escribir una extensión para Nooki.

```json
{"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{...}}}
```
```json
{"jsonrpc":"2.0","method":"textDocument/publishDiagnostics","params":{"diagnostics":[
  {"range":{"start":{"line":2,"character":4},"end":{"line":2,"character":12}},
   "severity":1,"code":"NOK2002","source":"nooki",
   "message":"se esperaba una expresión y se encontró «retornar»"}]}}
```

- **Sincronización completa** de documentos (`didOpen`, `didChange`): cada cambio reanaliza el archivo.
- **Diagnósticos con rango exacto**: línea y columna del span, gravedad, código `NOKnnnn` y el mensaje en
  español con sus notas y ayudas.
- **Sin dependencias**: un lector y escritor de JSON de 300 líneas y el bucle del protocolo, en C++.
  Nada de bibliotecas externas.
- **`prueba_lsp`**: capacidades, un documento con error (comprueba código, severidad y línea) y un
  documento correcto sin diagnósticos. **15 suites.**

**Configuración en el editor**: cualquier cliente LSP sirve. En Neovim, con `nvim-lspconfig`, basta
registrar el comando `nooki lsp` para los archivos `.oki`. La extensión de VS Code sigue coloreando la
sintaxis; para diagnósticos necesita un cliente LSP (otro sprint).

**Lo que no incluye**: autocompletado, ir a la definición, formato y resaltado semántico. El protocolo
está montado para añadirlos sin tocar lo demás.

### Sprint 75 — Manifiesto y dependencias (2026-11-12)

Un proyecto deja de ser una carpeta suelta: declara qué es y de qué depende.

```toml
# agenda/nooki.toml
[proyecto]
nombre = "agenda"
version = "0.1.0"

[dependencias]
saludos = { ruta = "../saludos" }
```

```aura
usar saludos     // se resuelve en la dependencia, no solo en «biblioteca/»
```

- **`nooki.toml`**: nombre, versión y dependencias por ruta. Se admite un subconjunto de TOML —
  secciones, `clave = "valor"` y `clave = { ruta = "..." }` — y los comentarios con `#`.
- **`nooki construir` lee el manifiesto** y añade, a la búsqueda de módulos, la raíz y la `biblioteca/`
  de cada dependencia.
- **`nooki nuevo` escribe el manifiesto**, así que todo proyecto nuevo ya lo tiene.
- **`prueba_manifiesto`**: lectura, dependencias, rutas de búsqueda y escritura. **14 suites.**
- **Verificado de punta a punta**: dos proyectos (`agenda` y `saludos`), `nooki construir` desde
  `agenda`, binario que imprime «Hola, Nooki» usando el módulo del otro proyecto.

**Lo que no incluye**, y es deliberado: no hay versiones semánticas resueltas, ni descarga desde Git, ni
registro, ni bloqueo de dependencias. Eso es un gestor de paquetes, y necesita usuarios antes que
diseño. El manifiesto deja la puerta abierta: hoy `ruta`, mañana `version` y `git`.

### Sprint 74 — `inseguro` y `asm!` (2026-11-12)

El último pilar del lenguaje: **control de hardware**. Y la válvula de escape que el plan pedía, con
una regla que la hace explícita y localizable.

```aura
inseguro {
    variable envuelto = grande + 1   // se envuelve, no se detiene
    asm!("nop")
}
```

- **`inseguro { ... }`**: dentro, la aritmética se emite tal cual, sin las comprobaciones de ADR-03.
  Es lo que hace falta para escribir controladores, donde el desbordamiento es parte del hardware y no
  un error.
- **`asm!("...")`**: ensamblador en línea. **Solo dentro de `inseguro`** (`NOK3012`), para que cada uso
  se pueda encontrar con una búsqueda.
- **La propiedad no se desactiva**: dentro de `inseguro` se siguen liberando los valores y se siguen
  moviendo al pasar; lo único que se apaga es la comprobación aritmética. Un bloque no puede hacer que
  el compilador libere dos veces.
- Caso `0048-inseguro`, verde en MinGW, MSVC y Linux.

**Límite del `asm!`**: necesita un compilador de C que lo admita (GCC, Clang, MinGW). Si el backend es
`cl.exe`, el C generado avisa con un `#error` claro en lugar de fallar de forma incomprensible.

Con esto, **los cinco pilares del plan original tocan código**: sintaxis fácil, tipos, memoria segura,
OOP completa y control de hardware.

### Sprint 73 — FFI: llamar a C (2026-11-12)

El plan ponía el FFI **antes** de la biblioteca estándar, «porque sin libc no hay archivos, red ni
tiempo». Ha llegado con tres años de retraso, y es el desbloqueo que faltaba.

```aura
externo funcion puts(cadena: &texto) -> booleano
externo funcion time(zona: &entero) -> entero

funcion principal() -> entero {
    puts("hola desde C")
    variable ahora = time(nulo)
    si (ahora > 1000000000) {
        imprimir("el reloj responde")
    }
    retornar 0
}
```

- **`externo funcion`** declara una función de C: nombre, parámetros y tipo de retorno; sin cuerpo.
- **Tipos traducidos**: `entero` ↔ `long long`, `booleano` ↔ `int`, `caracter` ↔ `char`, `vacio` ↔
  `void`, `&texto` ↔ `const char*`, y `&escalar` ↔ puntero al escalar.
- **Los textos viajan como puntero a sus bytes**: el runtime ya los guarda terminados en cero, así que
  no hay copia ni asignación extra.
- **`nulo`** ya se traduce: es el `NULL` de C, y sirve para los parámetros que lo admiten.
- Caso de conformidad `0047-ffi`, verificado en MinGW, MSVC y Linux.

**Límites conocidos, escritos**: no se declaran estructuras, ni funciones que devuelvan `texto`, ni se
elige biblioteca (todo se enlaza contra la libc que el compilador de C use). Y si la firma declarada
no coincide con la de C, el compilador de C avisa del conflicto: `strlen` devuelve `size_t`, así que
declararlo `entero` no compila — hay que declararlo como `natural64`.

### Sprint 72 — Desbordamiento comprobado y rutas con tildes (2026-11-11)

**ADR-03 se cumple por fin.** Era una decisión cerrada y el código la contradecía: `a + b` se emitía
como un `+` de C y desbordaba en silencio. Ahora la aritmética entera pasa por funciones comprobadas:

```aura
variable grande = 9223372036854775807
variable otro = grande + 1
```
```
$ ./programa
error en ejecucion: desbordamiento al sumar
$ echo $?
70
```

- **Comprobadas**: suma, resta, multiplicación, división, módulo y negación. Un desbordamiento, una
  división por cero o negar el mínimo detienen el programa con código **70** y un mensaje en la salida
  de error. Nunca se continúa con un valor equivocado.
- **Sin coste apreciable**: con `-O2` cada comprobación es un salto predicho.
- **`prueba_panico`**: seis casos que compilan, ejecutan y verifican el código y el mensaje.

**Y el fuzzer del sprint anterior destapó un fallo de fondo: los nombres con tildes.** En un lenguaje
en español eso no es un caso raro.

- `nooki ejecutar archivo ñ.oki` **caía el compilador**: Windows entrega los argumentos en la página de
  códigos local y la conversión a ruta lanzaba `Illegal byte sequence`. Ahora la línea de comandos se
  lee en UTF-16 y se convierte a UTF-8 (`CommandLineToArgvW`).
- Todas las conversiones de ruta a texto pasan por UTF-8 (`ruta_a_utf8`), no por `.string()`.
- El archivo `.c` intermedio usa un nombre ASCII, y si el destino tiene tildes se enlaza en una ruta
  ASCII y el binario se mueve después: el enlazador de MinGW no abre rutas con acentos.
- Las órdenes con bytes no ASCII se entregan al intérprete como UTF-16 (`_wsystem`).
- **`prueba_rutas`**: compila y ejecuta `carpeta con tildes á/programa ñ.oki`.

**Y un tercer fallo, silencioso desde el principio**: en Linux y macOS `ejecutar_comando` devolvía el
estado de espera de `system()` en lugar del código de salida, así que un programa que salía con 70 se
leía como 17920. Corregido con `WEXITSTATUS`.

- **13 suites** en `ctest`, verdes en MinGW, MSVC, GCC y ASan+UBSan.

### Sprint 71 — Fuzzing y benchmarks (2026-11-11)

Dos guardarraíles del plan que nunca se habían cumplido: el fuzzing continuo (regla 8) y los
benchmarks (regla 9).

**El fuzzer encontró un fallo real en la primera sesión larga.** Un escape inválido al final del
archivo (`"abc\`) hacía caer el compilador con `std::out_of_range`: `avanzar(2)` dejaba la posición un
byte más allá del final y el `substr` de `crear` lanzaba. Corregido acotando la posición —ahora un
valor movido o un escape al final dan un diagnóstico, no una caída— y con un caso de regresión
(`invalido/0018-escape-al-final`).

- **`nooki_fuzz`**: 600 entradas en `ctest` (2 000 por defecto, semilla elegible). Mezcla mutaciones del
  corpus de conformidad, piezas del lenguaje y bytes aleatorios, y pasa cada entrada por léxico, parser,
  verificación y generación. Guarda la entrada que provoque una excepción.
- **Resultado**: **100 000 entradas con cinco semillas** sin caídas, en Windows y en Linux.
- **`nooki_rendimiento`**: línea base de las métricas del plan. Lexer **25,5 MB/s** (Windows) y **33,3
  MB/s** (Linux); compilación completa **0,0029 s por 1 000 líneas**, unas **setecientas veces** por
  debajo del objetivo de 2 s del plan. Bajo sanitizadores mide sin exigir umbrales.
- **11 suites** en `ctest` (eran 9), en las cinco cadenas.

### Sprint 70 — Verificación fuera de Windows (2026-11-11)

**El compilador compila y pasa las nueve suites en Linux, con tres cadenas distintas.** Era la regla 1
del plan («verde en los tres sistemas») y nunca se había comprobado.

| Cadena | Compilación | Suites |
|---|---|---|
| GCC 13.3 · Ubuntu 24.04 | ✅ sin avisos | **9/9** |
| Clang 18.1 · Ubuntu 24.04 | ✅ sin avisos | **9/9** |
| GCC 13.3 + **ASan + UBSan**, avisos como errores | ✅ | **9/9** sin errores de memoria ni comportamiento indefinido |

Los ejemplos también se ejecutan en Linux (`nooki ejecutar ejemplos/01-hola/principal.oki`).

- **Dos avisos eliminados** que solo veía Clang: un campo `fuentes_` sin usar en el lexer y una función
  `es_clase_c` huérfana en el generador.
- **Faltaba el preset de compilación `saneamiento`**: estaba documentado y solo existía el de
  configuración, así que `cmake --build --preset saneamiento` fallaba al instante. Añadido.
- **macOS sigue sin verificar** (no hay hardware): se anota como A-15. La cadena de Clang en Linux es
  la evidencia más cercana disponible.

### Sprint 69 — Ejemplos y resaltado de sintaxis (2026-11-11)

- **`ejemplos/`**: cuatro programas ejecutables (hola, notas con módulo y errores, inventario con
  herencia, saludos con rasgos), cada uno con su salida real en `ejemplos/LEEME.md`.
- **Resaltado de sintaxis**: extensión de **VS Code** (gramática TextMate + configuración de
  lenguaje), sintaxis de **Vim/Neovim**, e instrucciones para Sublime Text, Zed, TextMate y
  Notepad++. Todo en `editores/`.
- **Fallo corregido**: `nooki ejecutar` con rutas que usan `/` fallaba en Windows, porque el comando se
  entregaba a la consola tal cual y esta no entiende `/` como separador. Ahora las rutas se normalizan
  al separador nativo antes de invocar al sistema.

### Sprint 68 — Preparar la publicación (2026-11-11)

- **Licencias completas**: `LICENCIA-MIT.txt` y `LICENCIA-APACHE.txt` con los textos íntegros, y
  `LICENCIA.md` que enlaza ambos. Cierra A-06.
- **`CONTRIBUIR.md` reescrito**: reflejaba el estado del sprint 3 («casi nada implementado»). Ahora
  explica las reglas reales: C++20 sin dependencias, los tres niveles de prueba, los tres requisitos
  de un diagnóstico nuevo y el commit firmado.
- **`PUBLICAR.md`**: los pasos exactos para subir el repositorio, ejecutar la CI por primera vez y
  qué hacer con lo que falle.
- **CI revisada**: presets y rutas del binario comprobados contra `CMakePresets.json` y el árbol de
  `build/`.

## [1.0.0] — 2026-11-11

**Nooki 1.0: todo lo que la especificación describe está implementado y probado, sin defectos
conocidos.** Las notas completas de la versión están en [NOTAS_DE_VERSION.md](NOTAS_DE_VERSION.md).

### Sprint 67 — Cierre de la 1.0 (2026-11-11)

- **Notas de versión** con lo que incluye, lo que no, para quién es y cómo empezar.
- **Inventario de pendientes** en `DECISIONES.md`, cada uno con su tamaño.
- Versión 1.0.0 en el compilador, la especificación y la documentación.

### Sprint 66 — A-14: mover un campo (2026-11-11)

- **Mover un campo a una colección lo deja vacío, no roto**: al vaciar un `texto` no se ponía su
  longitud a cero, así que quedaba un texto con puntero nulo y longitud vieja — cualquier lectura
  posterior era corrupción. Ahora el valor movido es un texto vacío válido.
- **Un método heredado cuenta como miembro**: `h.f()` donde `f` está en la clase base se rechazaba con
  «no tiene ningún miembro». Se busca en la cadena de bases, en el acceso a campos y en la llamada.
- **Una variable construida recuerda su tipo concreto**: `variable p = nuevo persona {...}` no guardaba
  `persona`, así que `p.nombre + "fin"` no compilaba.
- **`imprimir` acepta booleanos** (1 o 0).
- Caso `0046-campo-a-coleccion`.

### Sprint 65 — Todas las vías del movimiento (2026-11-11)

- **La comprobación de préstamo vive en un solo sitio** (`comprobar_prestamo`), al que llaman todas
  las vías que mueven un valor: reasignación, declaración y argumento por valor. Antes solo la
  reasignación miraba la tabla de préstamos.
- **Reasignar ya no marca el valor como movido**: era un error del arreglo anterior y rompía
  programas válidos que reasignan un `texto`.
- Caso `0017-mover-por-declaracion`.
- **Dos límites encontrados y anotados**: prestar un parámetro que es una clase se rechaza con
  `NOK4008` (solo se prestan variables locales), y mover un *campo* a una colección
  (`vector.agregar(p.nombre)`) no se reconoce como movimiento (A-14).

## [0.9.0] — 2026-11-11

**La biblioteca estándar tiene cinco módulos, todos escritos en Nooki**, y la documentación ya no
puede desincronizarse sin que un `ctest` lo diga.

### Sprint 64 — Cierre de la v0.9 (2026-11-11)

- **Prueba de coherencia documental** (`prueba_documentos`): comprueba que cada sprint terminado del
  plan tiene entrada en el historial y en el README, y que la versión coincide en CMake, historial,
  LEEME y especificación. Está en `ctest` como el resto.
- **Al estrenarla encontró dos huecos**: los sprints 52 y 53 no estaban en el historial. Reparados, y
  también las entradas 0.7.0 y 0.8.0 que faltaban.
- **La guía gana un paso 12** sobre la biblioteca estándar, con su salida verificada.
- v0.9.0.

### Sprint 63 — La biblioteca crece (2026-11-11)

- **`listas`**: `suma`, `mayor`, `menor`, `promedio` y `contiene` sobre `&Vector<entero>`, escritos en
  Nooki.
- **`fechas`**: `bisiesto` (con la regla de los siglos: 2100 no, 2024 sí), `dias_del_mes` y
  `dias_del_anio`.
- Ambos viajan **dentro del compilador**: se usan con `usar` sin copiar archivos.
- Caso de conformidad `0045-biblioteca-listas`.

### Sprint 62 — A-13 no era un fallo (2026-11-11)

- **La biblioteca embebida funciona**: `usar textos` sin archivo resuelve el módulo que viaja dentro
  del compilador.
- **A-13 era un archivo viejo**: `build\textos.oki`, un programa de pruebas olvidado, tapaba al módulo
  embebido. *Diagnóstico anterior equivocado, corregido aquí.*
- **Un módulo no puede definir `principal`**: antes un módulo con punto de entrada producía dos `main`.

### Sprint 61 — Biblioteca en Nooki y tipos de colección (2026-11-10)

- **Módulo `textos`** escrito en Nooki: `unir`, `repetir` y `contar`.
- **El elemento de un `Vector<texto>` es un texto**: se tipaba como entero y la concatenación no
  compilaba.
- **`*prestamo` de texto**: desreferenciar un `&texto` devuelve un texto.
- **Un archivo ya no puede importarse a sí mismo**: `usar principal` dentro de `principal.oki` cargaba
  el propio archivo como módulo y generaba dos `main`.

## [0.8.0] — 2026-11-10

La propiedad queda cerrada por los dos extremos: el valor no se puede mover mientras alguien lo presta
(`NOK5007`), y no se puede devolver un préstamo a algo que muere (`NOK5006`).

### Sprint 60 — Cierre de la v0.8 (2026-11-10)

- **Auditoría de diagnóstico**: los **46 códigos** están definidos, todos se emiten y todos tienen
  mensaje en español e inglés. **36 tienen un caso que los provoca**.
- **Tres códigos ganaron caso**: `NOK4011`, `NOK4012` y `NOK4010`.
- **Los 10 restantes, identificados** como deuda de pruebas (A-12): rutas de integración (CLI, carga de
  módulos, símbolo privado, backend y avisos).

### Sprint 59 — La duración del préstamo (2026-11-10)

- **Reasignar o mover el dueño mientras otro lo presta es error** (`NOK5007`).
- **Prestar una clase presta el objeto**: `&persona` ya no genera un puntero doble, y pasar `&p` a un
  parámetro `&Clase` compila.

### Sprint 58 — Que las pruebas prueben lo que dicen (2026-11-10)

- **Los casos `invalido/` pasan por todas las fases**: léxico, parser y verificación.
- **Cuatro casos inválidos semánticos nuevos**: `NOK5006`, `NOK5001`, `NOK5004` y `NOK3011`.

### Sprint 57 — El préstamo que escapa (2026-11-10)

- **Devolver un préstamo a una variable local es error** (`NOK5006`).
- **Prestar un parámetro sí se puede devolver**; los argumentos de una llamada no escapan.

## [0.7.0] — 2026-11-10

**El sistema de tipos está completo.** Con rasgos y `&Rasgo`, el lenguaje tiene las dos formas de
polimorfismo: por jerarquía y por contrato.

### Sprint 56 — Cierre de la v0.7 (2026-11-10)

- **ADR-026** documenta los rasgos: contrato verificado, tabla por (tipo, rasgo), adaptadores y el
  coste de `&Rasgo`.
- **La guía gana un paso 11** sobre rasgos, con su salida verificada.

### Sprint 55 — `&Rasgo`: despacho por rasgo (2026-11-10)

- **`&Rasgo` como tipo**: una referencia que lleva el objeto y su tabla de métodos.
- **Tabla por (tipo, rasgo)** con un adaptador por método y despacho dinámico entre tipos **no
  emparentados**.

### Sprint 53 — La guía en español (2026-11-10)

- **`GUIA.md`**: recorrido de cero a un programa con módulos en once pasos, cada uno **ejecutado** para
  copiar su salida real.
- **`LEEME.md`**: qué es Nooki, qué pilares tiene y cuáles no, cómo compilarlo y cómo se prueba.

### Sprint 52 — Las herramientas del proyecto (2026-11-10)

- **`nooki nuevo`**, **`nooki probar`** y **`nooki limpiar`**: crear un proyecto que compila, ejecutar
  pruebas contra su `.esperado` y borrar artefactos.
- **La búsqueda de módulos parte de rutas absolutas**, así que `usar` funciona desde subcarpetas.

### Sprint 54 — Rasgos (2026-11-10)

- **`rasgo Nombre { funcion f(...) -> tipo }`** declara un contrato; `clase X implementa A, B` se
  compromete a cumplirlo.
- El compilador exige que **estén todos los métodos** (`NOK3011`) y que la **firma coincida**
  (`NOK4012`).

## [0.6.0] — 2026-11-10

**Sin defectos conocidos.** Por primera vez la lista de «cosas que están mal y se saben» está vacía:
los huecos de propiedad que quedaban abiertos se cerraron, y el único caso que el generador no puede
resolver —una llamada que mueve un valor con dueño dentro de otra expresión— es ahora un error de
compilación, no una fuga silenciosa.

### Sprint 51 — Cerrar A-11 y publicar la v0.6 (2026-11-10)

- **Llamada anidada que movería un valor con dueño**: `f(g(a))` donde `g` mueve `a` ya no compila. El
  generador no tiene dónde emitir el vaciado del origen dentro de una expresión, así que lo rechaza
  con un mensaje que explica la alternativa.
- **Caso `0020-propiedad-parametros` migrado** a la forma que sí funciona: mover en una declaración.
- Versión 0.6.0 y especificación actualizada.

### Sprint 50 — Cerrar los huecos de propiedad (2026-11-10)

- **Copiar un compuesto con campos con dueño es error** (`NOK5004`): compartiría las cadenas y se
  liberarían dos veces. El mensaje sugiere prestarlo o construir otro.
- **`romper` y `continuar` liberan lo que abandonan**: antes se saltaban la liberación de las
  variables con dueño declaradas dentro del bucle.
- **`para` y `mientras` registran su profundidad de ámbito**, de modo que la liberación al saltar
  sabe hasta dónde deshacer.
- **Hueco restante declarado**: una llamada que mueve un valor con dueño dentro de otra expresión
  (`f(g(a))`) sigue sin moverlo. Es una fuga, no corrupción, y está anotada.
- Programas de conformidad `0040-propiedad-saltos`, `0039-compuestos-dueno` y `0038-propiedad-retorno`.

### Sprint 49 — Los compuestos liberan sus campos (2026-11-10)

- **Destructor por instanciación**: un compuesto genérico cuyos campos tienen dueño genera su
  `soltar` y se libera al salir del ámbito. La fuga que quedaba abierta en A-10 se cierra.
- **Solo se libera el compuesto fresco**: el construido en el sitio o devuelto por una llamada. Uno
  copiado desde otra variable comparte los campos y no se libera, para no liberar dos veces.
- **Los campos también se mueven**: `libro.insertar(q.primero, ...)` vacía `q.primero` después de
  insertarlo, de modo que el mapa y el compuesto no comparten la misma cadena.
- Caso de conformidad `0039-compuestos-dueno`, que construye compuestos en un bucle e inserta sus
  campos en un mapa.

### Sprint 48 — Cerrar A-10: el retorno transfiere lo que contiene (2026-11-10)

- **Devolver un compuesto transfiere sus variables**: `retornar par<texto> { primero: nombre }` ya no
  libera `nombre`; la propiedad pasa al llamante.
- **Construir una estructura mueve lo que se le pasa**: `variable p = par<texto> { primero: nombre }`
  anula `nombre`, igual que una asignación. Se cierra el caso indirecto (construir y devolver la
  variable), que seguía dejando punteros colgando.
- **Caso de conformidad `0038-propiedad-retorno`** con las dos formas: la directa y la que construye
  en una variable antes de devolver.
- **Riesgo residual documentado** (A-10, reducido): un compuesto con campos con dueño no los libera al
  salir del ámbito, así que esas cadenas se pierden. Es una fuga, no corrupción.

## [0.5.0] — 2026-11-10

**El lenguaje del plan está implementado.** Con genéricos de tipos, proyectos y biblioteca escrita en
Nooki, todo lo que la especificación describe —salvo tiempos de vida y rasgos— funciona y está
probado: 59 programas que se compilan y ejecutan en cada `ctest`, en GCC y MSVC.

### Sprint 47 — Cierre de la v0.5 (2026-11-10)

- **Auditoría de diagnóstico**: los 41 códigos `NOK` están definidos y todos se emiten en algún
  camino del compilador; ninguno quedó huérfano.
- **Programa de cierre `0037-notas`**: módulos, genéricos (`par<texto>`), errores con variantes,
  `coincide` con ligaduras, mapa, préstamos y entrada estándar, en un solo programa de dos archivos.
- **Hallazgo documentado**: devolver una construcción que embebe variables locales con dueño las
  libera al retornar y deja punteros colgando. Se documenta como limitación conocida y se marca como
  el próximo trabajo de corrección; el ejemplo lo evita construyendo directamente en el `retornar`.
- **v0.5.0**.

### Sprint 46 — Genéricos de tipos (2026-11-10)

- **Tipos con parámetros**: `estructura par<T> { publico primero: T }`.
- **Instanciación explícita**: `par<entero> { primero: 1, segundo: 2 }` y `par<texto> { ... }`.
  Cada combinación genera su propia estructura en C (`nooki_par_entero`, `nooki_par_texto`),
  siguiendo el mismo criterio que las funciones genéricas.
- **Descubrimiento por recorrido**: el generador registra las instanciaciones que aparecen en
  declaraciones, parámetros, campos y construcciones antes de emitir nada.
- Programa de conformidad `0036-genericos-tipo`, compilado y ejecutado.

### Sprint 45 — Paquetes: proyectos (2026-11-10)

- **Búsqueda de módulos**: junto al archivo que importa, en su carpeta `biblioteca/` y subiendo por
  los directorios padre. Un proyecto deja de necesitar rutas ni copias de sus módulos.
- **`nooki construir`**: compila el proyecto del directorio actual, tomando `principal.oki` o
  `fuente/principal.oki` como punto de entrada.
- **La biblioteca integrada sigue disponible**: los módulos del compilador (`resultado`,
  `matematicas`) se usan sin archivo, y un proyecto puede sobrescribirlos con los suyos.
- Programa de conformidad `0035-proyecto`, con un módulo en `biblioteca/`.

### Sprint 44 — La biblioteca crece (2026-11-10)

- **`texto.dividir(separador)`**: parte un texto y devuelve un `Vector<texto>` cuyos elementos son
  copias con dueño. Con esto un programa analiza una línea con varios campos.
- **`texto.mayusculas()`, `minusculas()` y `recortar()`**: copias transformadas; `recortar` quita
  espacios y saltos de los extremos.
- **Módulo `matematicas` escrito en Nooki**: `maximo`, `minimo`, `absoluto` y `cuadrado`.
- Programa de conformidad `0034-biblioteca`, que analiza un CSV y usa la biblioteca matemática.

### Sprint 43 — La biblioteca estándar se escribe en Nooki (2026-11-10)

- **Módulo `resultado` escrito en Nooki**: `usar resultado` trae la enumeración con `Ok(valor)` y
  `Error(mensaje)`. El módulo viaja dentro del compilador, sin archivos ni rutas de búsqueda.
- **`?` es estructural**: funciona con cualquier enumeración que tenga forma de resultado (dos
  variantes y la primera con un campo), no con un tipo concreto del compilador.
- **El compilador adelgaza**: se retira `nooki_resultado` del runtime, los `Ok`/`Error` integrados y
  el caso especial de la verificación.
- **Migrado `0031-resultados`** a la biblioteca: el mismo programa, ahora sin magia en el compilador.
- **ADR-025** documenta la decisión y sus costes.

### Sprint 42 — Variantes con datos (2026-11-10)

- **Variantes con campos**: `enumeracion respuesta { Bien(valor: entero) Mal(mensaje: texto) }`.
- **Representación**: unión etiquetada en C cuando alguna variante lleva datos; entero con constantes
  cuando no, así que nada anterior cambia.
- **Construcción por variante**: `Bien(10)` y `Mal("mensaje")`.
- **Patrones que ligan**: `caso Bien(v)` declara `v` con el tipo del campo dentro de la rama.
- **Cobertura completa**: si están todas las variantes, no hace falta `_`.
- **Liberación**: las variantes con texto sueltan su campo al salir del ámbito.
- **ADR-024** documenta la decisión y sus costes.
- Programa de conformidad `0033-variantes`, compilado y ejecutado con las dos ramas.

### Sprint 41 — Operaciones de texto (2026-11-10)

- **`buscar(otro)`**: posición de la primera aparición, o `-1` si no está.
- **`subtexto(desde, cuantos)`**: copia con dueño, recortada al final del texto si se pide de más.
- **`contiene(otro)`** y **`empieza_con(prefijo)`**: comprobaciones sin extraer copia.
- **`caracter(indice)`**: byte en esa posición, o `-1` fuera de rango.
- **`a_entero()`**: interpreta los dígitos iniciales con signo; 0 si no hay.
- Con esto un programa puede **trocear y analizar** lo que lee de la entrada.
- Programa de conformidad `0032-texto-operaciones`, compilado y ejecutado.

### Sprint 40 — Errores como valores (2026-11-10)

- **`Resultado`**: tipo del lenguaje que representa un entero o un error de texto. `Ok(valor)` y
  `Error(mensaje)` lo construyen; se libera con su texto.
- **Operador `?`**: propaga el error sin comprobarlo a mano. Si el resultado es un error, la función
  devuelve ese mismo error.
- **Verificación de contexto**: `?` fuera de una función que devuelve `Resultado` es `NOK4011`.
- **`es_error()`, `valor()` y `mensaje()`** para tratar el resultado de forma explícita.
- **ADR-023** documenta la decisión, el límite (un solo tipo de resultado hasta los genéricos de
  tipos) y el coste (`?` solo en declaraciones).
- Programa de conformidad `0031-resultados`, compilado y ejecutado con las dos ramas.

## [0.4.0] — 2026-11-10

**La orientación a objetos está completa.** Clases por referencia, herencia simple, despacho por
tabla virtual, `super` y variables de tipo base con polimorfismo. Es el último pilar del lenguaje
tal como se concibió en los planes originales.

### Sprint 39 — Cierre de la v0.4 (2026-11-10)

- **Verificación de firma al sobrescribir**: una derivada que redefine un método virtual con otros
  parámetros o retorno recibe `NOK4010` en Nooki, en lugar de fallar en el C generado.
- **Ejemplo polimórfico entre módulos** (`0030-formas`): un módulo exporta la jerarquía y fábricas,
  y el programa principal guarda derivadas en variables de tipo base y las describe.
- **Especificación v0.4** y versión 0.4.0.

## [0.3.0] — 2026-11-10

**Biblioteca básica completa.** El lenguaje ya tiene estructuras con métodos, enumeraciones y
`coincide`, propiedad determinista, textos, colecciones (`Vector`, `Mapa`), genéricos y módulos con
visibilidad. Un programa puede repartirse en varios archivos.

### Sprint 38 — `super` y conversión a la base (2026-11-10)

- **`super.metodo(...)`**: una clase derivada invoca la implementación de su base sin despacho.
  `super.describe() + " cuadrada"` compone el comportamiento heredado con el propio.
- **Conversión a la base**: `variable f: figura = nuevo cuadrado { ... }` guarda un puntero derivado
  en una variable de tipo base, con conversión explícita en el C generado (la base está al
  principio de la estructura).
- **Polimorfismo real a través de la base**: `f.area()` despacha a la implementación de la clase
  concreta; `f.describe()` usa la heredada si la derivada no la redefine.
- **Un tipo de clase declarado siempre es una referencia**: el `*` lo pone el compilador.
- Programa de conformidad `0029-super`, compilado y ejecutado.

### Sprint 37 — Herencia y polimorfismo (2026-11-10)

- **`clase derivada hereda base`**: herencia simple con campos heredados primero y el puntero a la
  tabla virtual al principio, de modo que un puntero a derivada sirve como puntero a base.
- **Despacho por tabla virtual**: cada clase instala su tabla en `nuevo`, y una llamada a un método
  virtual va por la tabla. Dos derivadas de la misma base responden distinto al mismo mensaje.
- **Reutilización de implementaciones**: si una derivada no redefine un método, su tabla apunta a la
  de la base.
- **ADR-022** documenta la representación, el criterio de virtualidad y los límites (`super` y
  conversión a la base quedan para el sprint siguiente).
- Programa de conformidad `0028-herencia`, compilado y ejecutado.

### Sprint 36 — `clase` por referencia (2026-11-10)

- **`nuevo Tipo { ... }`**: una clase se reserva en el montículo y la variable es su dueña.
- **Misma regla de propiedad que el resto**: mover al asignar (`NOK5001`), prestar con `&` sin
  mover, y liberación determinista al cerrar el ámbito.
- **`estructura` sigue por valor**; ahora la diferencia es real y no una promesa.
- **ADR-021** reemplaza a ADR-015 y documenta la decisión, sus costes y lo que desbloquea.
- Programa de conformidad `0027-clases-referencia`, compilado y ejecutado.
- Corregido el paso de receptores prestados en colecciones tras generalizar los punteros.

### Sprint 35 — Espacios de nombres para tipos (2026-11-10)

- **`modulo.tipo`** en declaraciones, parámetros y campos: una estructura exportada por un módulo se
  usa desde fuera con su nombre cualificado (`figuras.punto`).
- **Nombres de C con prefijo**: `nooki_figuras_punto`, de modo que dos módulos pueden declarar
  `punto` sin chocar.
- **Visibilidad en tipos y enumeraciones**: `publico estructura`, `publico clase` y
  `publico enumeracion` en la cabecera de un archivo.
- **Estructuras devueltas por funciones**: una variable inicializada con una llamada cuyo tipo de
  retorno es una estructura recuerda ese tipo, así que sus métodos y campos se resuelven.
- **Comparación de tipos por definición**: `punto` y `figuras.punto` son la misma estructura aunque
  se escriban distinto.
- Programa de conformidad `0026-tipos-modulo`, con dos archivos, compilado y ejecutado.

### Sprint 34 — Programa interactivo y `claves()` (2026-11-10)

- **`mapa.claves() -> Vector<texto>`**: recorre las claves de un mapa. Las claves devueltas son
  vistas (el mapa sigue siendo el dueño), así que el vector se libera sin tocar los textos.
- **Ejemplo interactivo `0025-gastos`**: un módulo con un mapa, lectura de pares (concepto, importe)
  hasta el final de la entrada, recorrido de claves y total. Compila y ejecuta en cada `ctest`.
- **Corregido un error de orden en el C generado**: las liberaciones de un ámbito se emitían
  *después* de la llave de cierre de la función, es decir, fuera de la función. Solo se veía cuando
  una función declaraba una colección y no retornaba explícitamente.
- **Corregido el prefijo de las llamadas internas de un módulo**: `total(libro)` dentro del módulo
  `gastos` se emitía como `nooki_total` en lugar de `nooki_gastos_total`.

### Sprint 33 — Entrada estándar (2026-11-10)

- **`leer_linea() -> texto`**: lee una línea de la entrada estándar sin el salto y devuelve un texto
  con dueño. Al final de la entrada devuelve un texto vacío.
- **`leer_entero() -> entero`**: lee una línea y la interpreta como entero con signo; si no hay
  dígitos, devuelve 0.
- Ambas son integradas del lenguaje (no funciones de módulo) y respetan la propiedad: el texto que
  devuelven se libera al cerrar el ámbito.
- Un programa ya puede conversar: pide datos, los procesa y responde.
- Programa de conformidad `0024-entrada`, que compila y ejecuta en cada `ctest` con la entrada
  cerrada.

### Sprint 32 — `Mapa`: rehash y eliminación (2026-11-10)

- **`quitar(clave)`**: elimina la entrada, libera su clave y devuelve 1 si existía, 0 si no.
- **Rehash automático**: cuando el mapa pasa de tres cuartos de ocupación, duplica las cubetas y
  recoloca las entradas. `obtener` vuelve a ser O(1) medio con muchas claves.
- El caso de conformidad `0021-mapas` prueba inserción, reemplazo, consulta, eliminación y ausencia.

### Sprint 31 — Consolidación (2026-11-10)

- **Especificación v0.3** con las secciones 18.8 a 18.12: colecciones, mapas, módulos, propiedad en
  llamadas y los códigos nuevos. Ya no hay distancia entre lo que el compilador sabe y lo que está
  escrito.
- **Ejemplo real multifichero** (`0023-agenda`): un módulo con un mapa, préstamos mutables y textos,
  compilado y ejecutado en cada `ctest`.
- **Devuelto no es liberado**: devolver un valor con dueño de una función mueve la propiedad al
  llamante; antes se liberaba al salir del ámbito y quedaba colgando.
- **Un literal puede llegar a un parámetro prestado**: se le da dirección con un literal compuesto
  de C, en lugar de obligar a crear una variable.
- **Una variable local puede tapar el nombre de un módulo**, y se resuelve como método, no como
  módulo.
- v0.3.0.

### Sprint 30 — Módulos e importaciones (2026-11-10)

- **`usar modulo`** carga `modulo.oki` desde el directorio del archivo que lo importa, de forma
  recursiva. Los ciclos se deduplican en lugar de repetirse.
- **`modulo.simbolo`** para llamar a funciones de otro módulo; en C se traducen a
  `nooki_<modulo>_<funcion>`.
- **Visibilidad real**: solo `publico` es accesible desde fuera; usar un símbolo privado es
  `NOK3009`, y un módulo que no existe es `NOK3008` con la ruta buscada.
- Los módulos pueden importar a su vez otros módulos, y el orden no importa.
- Programa de conformidad `0022-modulos` con dos archivos, compilado y ejecutado.

### Sprint 29 — `Mapa<texto, entero>` (2026-11-10)

- **Tabla hash con encadenamiento**: `Mapa<texto, entero>` con `insertar`, `obtener`, `contiene` y
  `largo`.
- **Hash FNV-1a** sobre los bytes de la clave; inserción al principio de la cubeta, O(1) medio.
- **El mapa es dueño de las claves**: `insertar` transfiere el texto y anula el origen; al
  reemplazar una clave existente se libera la nueva y se conserva la que ya estaba.
- **Liberación determinista**: al cerrar el ámbito se sueltan todas las entradas, sus claves y el
  arreglo de cubetas.
- Programa de conformidad `0021-mapas`, compilado y ejecutado.

### Sprint 28 — Una sola regla de propiedad (2026-11-10)

- **Pasar un valor con dueño por valor lo mueve**: `consumir(a)` deja `a` vacío y usarlo después
  es `NOK5001`, con la sugerencia de prestarlo.
- **Prestar no mueve**: `prestado(&a)` deja `a` intacto.
- La regla vive **una sola vez** en el verificador y otra en el generador, mirando el tipo del
  parámetro; desaparece la lógica por tipo de dato que provocó el fallo de S27.
- **`&texto`** funciona como préstamo, también en `.largo()`.
- **ADR-020** documenta la decisión, el coste (la variable se invalida al pasarla) y el límite
  conocido (una llamada anidada todavía no mueve).
- Programa de conformidad `0020-propiedad-parametros`, compilado y ejecutado.

### Sprint 27 — `Vector<texto>` y dueño de los elementos (2026-11-10)

- **`Vector<texto>`**: colección de textos donde **el vector es el dueño** de cada elemento.
- **`agregar` transfiere la propiedad**: usar la variable de origen después es `NOK5001`, igual que
  con las cajas y los textos sueltos.
- **`obtener` devuelve una vista** (`propio = 0`): leer el elemento no lo libera ni lo duplica.
- `soltar` libera primero cada elemento y después el arreglo.
- Programa de conformidad `0019-vectores-texto`, compilado y ejecutado.
- **Corregido un fallo de corrupción de memoria**: la primera versión copiaba el texto al agregarlo
  y ambos lados lo liberaban al salir del ámbito. Lo detectó el propio programa con una excepción
  de montículo corrupto, no una prueba unitaria.

### Sprint 26 — Iteración sobre colecciones (2026-11-10)

- **`para (n en numeros)`**: recorre una colección por índice con una variable interna oculta; el
  cuerpo solo ve el elemento.
- **`Vector<T>` como parámetro prestado**: `&Vector<entero>` se traduce a puntero y el préstamo
  evita duplicar el dueño del arreglo.
- La verificación acepta `Vector<...>` como tipo válido y permite prestar variables que lo sean.
- Programa de conformidad `0018-iteracion`, compilado y ejecutado.

### Sprint 25 — `Vector<T>` (2026-11-10)

- **`Vector<entero>`**: arreglo dinámico en el montículo con `agregar`, `obtener`, `largo` y
  liberación determinista al cerrar el ámbito.
- **Crece multiplicando la capacidad** por dos, con `realloc`; `obtener` comprueba el índice y
  devuelve 0 fuera de rango en lugar de leer memoria ajena.
- **Misma propiedad que las cajas y los textos**: asignar mueve, y el liberador se elige por tipo.
- El parser admite tipos aplicados seguidos de `.` (`Vector<entero>.nuevo()`), no solo de `(`.
- Programa de conformidad `0017-vectores`, compilado y ejecutado.

### Sprint 24 — Genéricos por instanciación (2026-11-10)

- **Funciones genéricas**: `funcion identidad<T>(valor: T) -> T`, con instanciación explícita
  `identidad<entero>(42)` e `identidad<texto>("Nooki")`.
- **Una función de C por combinación de tipos**, con nombre mangleado (`nooki_identidad_texto`) y
  emitida una sola vez. Sin despacho en ejecución.
- **Descubrimiento diferido**: los cuerpos se generan primero en un búfer; las instanciaciones se
  emiten después, hasta que no aparezcan nuevas.
- **ADR-019** documenta el mecanismo, sus límites (sin inferencia) y el coste (una instancia por
  combinación).
- Programa de conformidad `0016-genericos`, compilado y ejecutado.

### Sprint 23 — `texto` de verdad (2026-11-10)
- **Tipo `texto`** con longitud: `datos`, `largo` y marca `propio` (ADR-018). Los literales no se
  liberan; las concatenaciones sí.
- **Concatenación** con `+` y **comparación por contenido** con `==`, `!=`, `<`, `>`, `<=`, `>=`.
- **`.largo()`** en bytes, en tiempo constante.
- **Propiedad**: asignar un texto lo mueve y usar el origen después es `NOK5001`. Liberación
  determinista al cerrar ámbito y antes de cada `retornar`.
- **Mezclar texto y entero** en una operación sigue siendo error (`NOK4001`).
- `imprimir` acepta `texto` en cualquier posición: literal, variable, parámetro o resultado de una
  función.
- Programa de conformidad `0015-textos`, compilado y ejecutado.

### Sprint 22 — Préstamos mutables y exclusividad (2026-11-09)

**Primera versión con memoria determinista.** El compilador ya escribe programas con
estructuras, objetos con métodos, enumeraciones, emparejamiento de patrones, cajas con dueño
único y préstamos que no mueven. Compila a binario nativo en Windows, Linux y macOS con
GCC/Clang/MSVC como cadenas de compilación.

- **`&mutable caja`** y parámetros `&mutable Caja<entero>`: escribir a través del préstamo sin
  mover la caja.
- **Escritura a través de un préstamo inmutable** → `NOK4009`, con la alternativa en el mensaje.
- **Exclusividad comprobada**: en una misma llamada no puede haber `&mutable x` junto a otro
  préstamo de `x` (`NOK5003`). Es la regla de préstamos de Rust verificada donde hoy puede
  violarse.
- **Versión 0.2.0** y **especificación v0.2** con la sección 18, que documenta todo lo añadido
  desde el Sprint 17 y la tabla completa de códigos de diagnóstico.
- Ejemplo real de conformidad (`0014-amortizacion`): un préstamo con estructura, método, caja y
  préstamo mutable, compilado y ejecutado.

### Sprint 21 — Préstamos inmutables (2026-11-09)

- **`&caja`**: préstamo inmutable que **no mueve**. La variable prestada sigue siendo válida y se
  libera donde siempre.
- **Tipo `&Caja<entero>`** en parámetros, traducido a `nooki_caja*` en C. El parser de tipos admite
  ahora `&T`, `&mutable T`, `*T` y genéricos `Caja<T>`.
- **Regla**: un préstamo vive durante la expresión que lo crea. Guardarlo en una variable es error
  `NOK5002`, con la explicación y la alternativa.
- **Prestar algo que no es una caja** → `NOK4008`.
- **ADR-017** documenta la regla, el límite (no se pueden devolver préstamos) y que `&mutable` se
  acepta pero aún no se distingue.
- Programa de conformidad `0012-prestamos`, compilado y ejecutado.

### Sprint 20 — `Caja<T>`: propiedad única y movimiento (2026-11-09)
- **`Caja<T>`**: valor en el montículo con **un único dueño**. `Caja.nuevo(41)` asigna; `*c` lee y
  escribe; al salir del ámbito el dueño libera.
- **Movimiento**: `variable d = c` transfiere la propiedad y anula el origen. Usar `c` después es
  **error de compilación** (`NOK5001`), con span y explicación.
- **Liberación determinista**: el generador registra las cajas de cada ámbito y emite
  `nooki_caja_soltar` al cerrarlo y antes de cada `retornar`. `soltar` es nulo-seguro, así que un
  movimiento no produce doble liberación.
- **`*` sobre algo que no es una caja** → `NOK4007`.
- **ADR-016** documenta la decisión, los límites (solo `Caja<entero>` hasta los genéricos) y el
  riesgo conocido con `romper`/`continuar`.
- **Corregido un fallo grave del parser**: un operador al principio de una línea nueva continuaba la
  expresión anterior. `variable x = 1` seguido de `*p = 2` se interpretaba como `1 * p = 2`, es
  decir, dos sentencias se fusionaban en silencio.
- Programa de conformidad `0011-cajas`, compilado y ejecutado.

### Sprint 19 — Enumeraciones y `coincide` (2026-11-09)

- **Enumeraciones**: `enumeracion color { rojo, verde, azul }`, con variantes accesibles como
  `color.rojo`.
- **Emparejamiento de patrones**: `coincide (valor) { caso 1 => ... caso color.rojo => ... caso _ => ... }`
  con literales, variantes y comodín, en una sola línea o con bloque.
- **Generación**: enumeraciones como constantes enteras y `coincide` como cadena `if/else` sobre una
  temporal, para no evaluar la expresión más de una vez.
- **Verificación**: variante inexistente (`NOK3007`) y `coincide` sin comodín (`NOK4006`).
- El lexer acepta `_` como comodín sin romper los identificadores que empiezan por guion bajo.
- Programa de conformidad `0010-patrones`, compilado y ejecutado.
- **Decisión registrada**: la semántica de referencia para `clase` (y con ella la herencia) se
  pospone a la fase de memoria. Un conteo de referencias a medias produce dobles liberaciones, que
  son corrupción de memoria; hacerlo bien exige liberar en todas las salidas de ámbito e incrementar
  en cada copia. Se hará una sola vez, junto al ownership.

### Sprint 18 — Métodos y `este` (2026-11-09)

- **Métodos dentro de tipos**: `publico funcion suma(otro: punto) -> entero { ... }`, con parámetros
  y retorno como cualquier función.
- **`este`** para acceder a los campos propios: `este.x`, `este.x = ...`.
- **Llamadas a métodos**: `p.suma(q)` se traduce a `nooki_punto_suma(&p, q)`, el mismo modelo que
  necesitará la herencia con tablas virtuales.
- **Verificación de métodos**: miembro inexistente (`NOK3006`), aridad (`NOK4003`) y tipo de
  argumento (`NOK4002`), incluidos los parámetros cuyo tipo es otra estructura.
- Programa de conformidad `0009-metodos`, compilado y ejecutado.
- El parser acepta `este` como inicio de sentencia (`este.x = ...`).

### Sprint 17 — Estructuras y clases con campos (2026-11-09)

- **Declaraciones de tipo**: `estructura punto { publico x: entero }` y `clase persona { ... }`,
  con visibilidad por campo (`publico`, `privado`, …).
- **Construcción con campos nombrados**: `punto { x: 1, y: 2 }`, traducida a literal compuesto de C.
- **Acceso y asignación de campos**: `p.x`, `p.x = 5`, incluido dentro de expresiones.
- **Verificación de la construcción**: tipo inexistente (`NOK3003`), campo inexistente (`NOK3004`),
  campo que falta (`NOK3005`) y tipo del valor incorrecto (`NOK4001`).
- **ADR-015**: en la v0.1, `clase` y `estructura` comparten representación por valor porque la
  semántica de referencia necesita el modelo de memoria. ADR-001 sigue vigente como objetivo.
- Programa de conformidad `0007-estructuras`, compilado y ejecutado.
- 4 casos nuevos de sentencias y 6 de verificación.

### Sprint 16 — Verificación de tipos (2026-11-09)

- **Verificador semántico** con ámbitos: nombres declarados, aridad de llamadas, tipos de
  argumentos, tipos de operadores, condiciones y retornos.
- **Cinco diagnósticos nuevos**, todos en español y con span:
  `NOK4001` tipo incompatible · `NOK4002` argumento incompatible · `NOK4003` aridad incorrecta ·
  `NOK4004` retorno incompatible · `NOK4005` condición no booleana.
- **`si`, `mientras` y `hacer` exigen condición booleana**: `si (1)` ya no compila. Es la defensa
  contra `si (x = 1)`, el error clásico de C.
- **Un solo `Tipo` para todo el compilador** (`semantico/tipos.hpp`): el generador deja de tener su
  propia noción de tipo y usa la misma.
- 22 casos de verificación (10 válidos, 12 inválidos) que comparan el código exacto esperado.
- El hueco que motivó el sprint queda cerrado: `factorial("texto")` ya no llega al compilador de C.

### Sprint 15 — Funciones con parámetros y primera verificación semántica (2026-11-09)

- **Funciones con parámetros**: `funcion factorial(n: entero) -> entero`, con cualquier número de
  parámetros y tipos (`entero`, `texto`, `booleano`).
- **Llamadas con argumentos** traducidas a C, con declaraciones anticipadas: la recursión funciona
  (el factorial recursivo se compila y ejecuta).
- **Inferencia del tipo de una llamada** a partir del tipo de retorno declarado, para que
  `imprimir(factorial(i))` elija la función de impresión correcta.
- **Primera verificación semántica**: `NOK3002` cuando se llama a una función que no existe, con
  span y sugerencia. Antes, ese error aparecía como un mensaje del compilador de C en inglés.
- Dos programas nuevos de conformidad, compilados y ejecutados: factorial recursivo y saludo con
  parámetro de texto.
- 37 comprobaciones de sentencias (4 nuevas sobre parámetros).

### Sprint 14 — El backend C compila variables y control de flujo (2026-11-09)

- **El generador traduce ahora el lenguaje que el parser ya entendía**: variables y constantes con
  tipo explícito o inferido, `si`/`sino`, `mientras`, `para` sobre rangos con `paso`, `hacer`,
  `bucle`, `romper`, `continuar`, bloques y asignaciones.
- **Aritmética y comparaciones** en C, y `**` mediante la función `nooki_potencia` del preludio.
- **`imprimir` distingue enteros de textos**: `nooki_imprimir_entero` y `nooki_imprimir_texto`.
- **Ámbito de tipos mínimo** (pila de mapas) para inferir el tipo de C de cada variable; no es el
  sistema de tipos del lenguaje, que llega en S20.
- **Declaraciones anticipadas** de funciones: se puede llamar a una función definida después.
- Tres programas nuevos de conformidad, **compilados y ejecutados**: FizzBuzz (1..15), Fibonacci
  (10 términos) y tabla de multiplicar con bucles anidados.
- El backend sigue negándose con `NOK6003` ante lo que no sabe traducir (flotantes, `coincide`,
  índice, conversiones) en lugar de generar código incorrecto.

### Sprint 13 — Sentencias y control de flujo (2026-11-09)

- **Sentencias completas**: `variable`/`constante` con tipo opcional, `si`/`sinosi`/`sino`,
  `mientras`, `para` (con `paso`), `hacer`, `bucle`, `romper`, `continuar`, `retornar`, bloques y
  expresiones sueltas.
- **`sinosi` encadenado** sin límite: se representa como un `si` anidado en la rama `sino`.
- **Nueva línea significativa**: `retornar` sin valor se distingue de `retornar x` por el salto de
  línea previo, no por mirar el token siguiente.
- **Impresor de sentencias y programas**: `(para i en (.. 0 10) paso 2 (si ...))`.
- **Comando `nooki ast`**: muestra el árbol canónico de un archivo `.oki`.
- **33 comprobaciones**: 27 programas válidos y 6 inválidos.
- `coincide` produce `NOK2004` con la explicación de que llega en el Sprint 17, en lugar de un error
  de sintaxis críptico.

### Sprint 12 — Parser de expresiones con precedencia (2026-11-09)

- **Parser Pratt** con la tabla de precedencia completa de la especificación §7.1: 15 niveles, con
  asociatividad izquierda, derecha y no asociativa donde corresponde.
- **AST de expresiones** propio: literales, identificador, unaria, binaria, llamada, miembro,
  miembro opcional, índice, propagación, conversión y asignación.
- **Impresor canónico** (`(+ 1 (* 2 3))`) que fija la precedencia por escrito.
- **197 comprobaciones** de precedencia y asociatividad, más 7 expresiones que deben fallar.
- Los rangos encadenados (`a..b..c`) son error `NOK2005`, no asociación implícita.
- El backend avisa con `NOK6003` en lugar de generar C incorrecto cuando el árbol usa algo que aún
  no sabe traducir.
- Dos decisiones de la tabla documentadas y registradas: los operadores de bits se unen más fuerte
  que las comparaciones (como Python, no como C), y `-2 ** 2` = `(-2) ** 2` (asunto A-08 a revisar).

### Sprint 6 — Rebanada vertical: "Hola, Nooki" (2026-11-09)

- **El primer programa Nooki compila y se ejecuta**: `.oki` → parser → C → binario nativo PE/ELF/Mach-O.
- **Backend C** (ADR-014): se genera C y se invoca el compilador del sistema (`cc`, `gcc`, `clang`,
  o `NOOKI_CC`). Decisión motivada porque no hay LLVM en la máquina y porque MSVC y MinGW no pueden
  enlazar las librerías de LLVM entre sí.
- **`nooki compilar`** (con `-o`, `--emitir-c`, `--conservar`) y **`nooki ejecutar`**.
- Parser descendente recursivo mínimo: funciones, bloques, llamadas, literales y `retornar`.
- Generador de C: runtime `imprimir`, traducción de tipos, nombres y separadores numéricos.
- Caso de conformidad `0000-hola-mundo` que **compila y compara la salida real** del binario.
- 13 comprobaciones del compilador; la suite completa queda en 5 pruebas.
- Corrección de dos fallos de plataforma: `cmd` recortaba las comillas iniciales de los comandos, y
  la redirección de Windows convertía `\n` en `\r\n` y hacía fallar toda comparación.

### Sprint 5 — Arnés de conformidad y analizador léxico (2026-11-09)

- **Arnés de conformidad** (`nooki_conformidad`): recorre `pruebas/conformidad/`, ejecuta cada caso
  y compara los códigos obtenidos con los esperados; informa del rendimiento del lexer.
- **22 casos de conformidad**: 12 válidos (léxico completo) y 10 inválidos con su código esperado.
- **Analizador léxico** de la v0.1: 84 palabras reservadas con búsqueda binaria verificada en
  compilación (`static_assert`), identificadores con tildes y eñe, enteros en cuatro bases,
  flotantes, textos con escapes y multilínea, caracteres y tiempos de vida, comentarios anidados y
  de documentación.
- Diagnóstico `NOK1001` con sugerencia cuando una palabra clave se escribe con tilde.
- 33 comprobaciones unitarias del lexer.
- Enlazado **totalmente estático** en MinGW: elimina la dependencia de `libwinpthread-1.dll`, que
  provocaba un fallo de arranque cuando el sistema tenía otra versión en el `PATH`.
- Línea base de rendimiento: **26,7 MB/s · 5,6 M tokens/s** (corpus de 128 KB).
- Re-corte del plan: el lexer pasa de S06 a S05 (el arnés necesitaba un consumidor real); S06 queda
  como parser mínimo + LLVM + enlace.

### Sprint 4 — Sistema de diagnósticos (2026-11-09)

- `nooki/utf8`: decodificador propio que detecta secuencias inválidas sin abortar.
- `GestorFuentes`: registro de archivos y conversión de desplazamientos en bytes a línea y columna
  contadas en puntos de código (ADR-008).
- Códigos `NOKnnnn` con el esquema por fases de la especificación §11.1.
- Catálogo bilingüe español/inglés: los mensajes son código + parámetros, no texto ya montado.
- Render en texto con canaleta, caretos, etiquetas, notas y ayudas; color ANSI opcional con
  detección de terminal y respeto a `NO_COLOR`.
- Render JSON para herramientas y el futuro LSP.
- 5 casos dorados de diagnóstico y 1 dorado JSON, más 11 comprobaciones de catálogo, color y
  ubicaciones.
- Corrección de C4996 en MSVC mediante `_dupenv_s` en lugar de silenciar avisos globalmente.

### Sprint 3 — Andamiaje del repositorio (2026-11-09)

- CMake 3.25+ con presets para MSVC 2022/2026, MinGW, GCC y Clang.
- Objetivo `nooki_nucleo` (biblioteca) + ejecutable `nooki`.
- Cabecera `nooki/version.hpp` generada desde CMake: única fuente de verdad de la versión.
- CLI con `version`, `ayuda`, códigos de salida documentados y sugerencia ante errores de tecleo.
- Pruebas de la CLI con `ctest` (9 casos, sin dependencias externas).
- Integración continua en Windows, Linux y macOS; trabajos informativos de formato y análisis.
- Enlace estático de la biblioteca estándar en MinGW: el binario es autocontenido.
- Licencia dual MIT/Apache-2.0, guía de contribución y plantilla de RFC.

### Sprint 2 — Especificación (2026-11-09)

- `spec/gramatica.ebnf`: gramática normativa completa (125 reglas).
- `spec/especificacion-v0.1.md`: léxico, tipos, OOP, memoria, errores, FFI y conformidad.
- 63 palabras clave + 21 nombres de tipo, sin tildes; identificadores normalizados a NFC.

### Sprint 1 — Decisiones (2026-11-09)

- 13 ADR aprobadas en `docs/adr/`.
- Renombrado del proyecto a **Nooki**, extensión `.oki`.
- Plan recalculado para un solo desarrollador; UI e IA pasan a post-1.0.
