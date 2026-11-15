# Guía del usuario de Nooki

**Aprender a programar en Nooki desde cero, con ejemplos que se compilan y se ejecutan.**

| | |
|---|---|
| Versión documentada | **Nooki 1.0.0** (edición 2026) |
| Extensión de fuentes | `.oki` |
| Comando | `nooki` |
| Plataformas | Windows, Linux y macOS |
| Requisito externo | un compilador de C (`cc`, `gcc` o `clang`) |

> Esta guía está escrita contra la implementación real del compilador, no contra el plan
> original. Todos los programas de ejemplo se han ejecutado con el binario de la versión
> 1.0.0. Cuando una función existe pero tiene un límite, se dice expresamente.

Si solo quieres el recorrido mínimo, empieza por el capítulo 1 y sigue en orden. Si ya
programas en otro lenguaje, los capítulos 3 a 9 son la parte que más te interesa.

---

## Índice

1. [Qué es Nooki y qué necesitas](#1-qué-es-nooki-y-qué-necesitas)
2. [Tu primer programa](#2-tu-primer-programa)
3. [La forma de un programa](#3-la-forma-de-un-programa)
4. [Variables, constantes y tipos](#4-variables-constantes-y-tipos)
5. [Operadores y expresiones](#5-operadores-y-expresiones)
6. [Decisiones y bucles](#6-decisiones-y-bucles)
7. [Funciones](#7-funciones)
8. [Textos](#8-textos)
9. [Colecciones](#9-colecciones)
10. [Estructuras y clases](#10-estructuras-y-clases)
11. [Rasgos](#11-rasgos)
12. [Enumeraciones y `coincide`](#12-enumeraciones-y-coincide)
13. [Errores como valores](#13-errores-como-valores)
14. [Genéricos](#14-genéricos)
15. [Módulos y proyectos](#15-módulos-y-proyectos)
16. [Memoria: propiedad y préstamos](#16-memoria-propiedad-y-préstamos)
17. [Entrada, salida y archivos](#17-entrada-salida-y-archivos)
18. [Una ventana](#18-una-ventana)
19. [Inteligencia artificial](#19-inteligencia-artificial)
20. [Las herramientas del día a día](#20-las-herramientas-del-día-a-día)
21. [Errores frecuentes](#21-errores-frecuentes)
22. [Lo que todavía no existe](#22-lo-que-todavía-no-existe)
23. [Siguientes pasos](#23-siguientes-pasos)

---

## 1. Qué es Nooki y qué necesitas

Nooki es un lenguaje **compilado**. El compilador traduce tu archivo `.oki` a C y entrega ese
C a un compilador del sistema; el resultado es un ejecutable nativo. No hay máquina virtual
ni intérprete.

Tres ideas lo distinguen:

- **Se escribe en español.** Las palabras del lenguaje son `funcion`, `variable`,
  `mientras`, `retornar`, `coincide`… Los identificadores pueden llevar tildes (`año`,
  `número`), pero las palabras clave se escriben sin tilde.
- **La memoria es determinista.** No hay recolector de basura. Cada valor con dueño
  (`texto`, `Vector`, `Mapa`, una clase) se libera al salir del ámbito, y el compilador
  rechaza los usos peligrosos antes de generar código.
- **Todo está comprobado antes de ejecutar.** Los tipos, la aridad de las llamadas y los
  préstamos se verifican en compilación.

### Lo que necesitas

1. El compilador **`nooki`** en el `PATH`.
2. Un compilador de C en el `PATH`: `cc`, `gcc` o `clang`.

```bash
nooki version
cc --version
```

En Windows, el backend que funciona es MinGW (`gcc`/`cc`) o Clang; el compilador `cl.exe`
de MSVC no se detecta automáticamente. En Linux, para programas con ventana, hacen falta
las cabeceras de X11:

```bash
sudo apt install libx11-dev
```

### Instalar el compilador

Si tienes la distribución, descomprímela y añade su carpeta al `PATH`. En Windows, desde la
carpeta de la distribución:

```powershell
.\herramientas\instalar.ps1
```

En Linux o macOS:

```bash
echo 'export PATH="'"$PWD"'":$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Si compilas desde el repositorio, mira la [guía del desarrollador](GUIA_DEL_DESARROLLADOR.md)
y [LEEME](../../LEEME.md). El binario queda en `build/<preset>/compilador/` (en MSVC, dentro
de `Release/`).

---

## 2. Tu primer programa

Crea un archivo `hola.oki`:

```nooki
funcion principal() -> entero {
    imprimir("¡Hola, Nooki!")
    retornar 0
}
```

Ejecútalo:

```bash
nooki ejecutar hola.oki
```

```text
¡Hola, Nooki!
```

`principal` es el punto de entrada. Devuelve un `entero`, que es el código de salida del
programa: `0` significa que todo fue bien. Si el programa termina sin `retornar`, sale con
`0`.

### Compilar a un ejecutable

```bash
nooki compilar hola.oki
```

Genera `hola.exe` en Windows o `hola` en Linux/macOS, junto al fuente. Con `-o` eliges otro
nombre:

```bash
nooki compilar hola.oki -o saludo
```

### Un proyecto completo

```bash
nooki nuevo demo
cd demo
nooki construir
./principal
```

`nooki nuevo` crea `principal.oki`, `nooki.toml` y `biblioteca/herramientas.oki`. La carpeta
`pruebas/` no se crea automáticamente: créala cuando vayas a escribir pruebas (capítulo 15).

---

## 3. La forma de un programa

### Archivos y codificación

- Extensión **`.oki`**.
- Codificación **UTF-8 sin BOM**. Un BOM al principio produce `NOK1004`.
- Los saltos de línea pueden ser `\n`, `\r\n` o `\r`.

### Declaraciones de primer nivel

Fuera de las funciones solo puede haber **declaraciones**: funciones, estructuras, clases,
rasgos y enumeraciones. **No hay variables globales.**

```nooki
// Una estructura
estructura punto {
    publico x: entero
    publico y: entero
}

// Una función
funcion distancia(p: punto) -> entero {
    retornar p.x + p.y
}

funcion principal() -> entero {
    variable p = punto { x: 3, y: 4 }
    imprimir(distancia(p))
    retornar 0
}
```

### Comentarios

```nooki
// Comentario de una línea

/* Comentario de varias líneas.
   Los comentarios de bloque se pueden anidar:
   /* esto sigue dentro del comentario */ */

funcion principal() -> entero {
    imprimir("texto con // dentro de comillas") // esto sí es un comentario
    retornar 0
}
```

### Las sentencias terminan en el salto de línea

Nooki **no usa punto y coma**. Cada sentencia ocupa su línea y los bloques se delimitan con
llaves `{ }`. La sangría es una convención, no cambia el significado; el formateador la
normaliza a cuatro espacios.

```nooki
funcion principal() -> entero {
    variable a = 1
    variable b = 2
    imprimir(a + b)
    retornar 0
}
```

### `imprimir`

`imprimir` escribe un valor y añade un salto de línea. Acepta `entero`, `flotante`,
`booleano` y `texto`:

```nooki
funcion principal() -> entero {
    imprimir(42)
    imprimir(3.5)
    imprimir(verdadero)
    imprimir("una frase")
    retornar 0
}
```

Salida real (en la configuración regional en español):

```text
42
3,5
1
una frase
```

- Un `booleano` se imprime como `1` (verdadero) o `0` (falso).
- Un `flotante` se imprime con coma decimal, hasta seis decimales, sin ceros sobrantes.
- `imprimir` no acepta estructuras, clases, enumeraciones ni `nulo`.

---

## 4. Variables, constantes y tipos

### Declarar

```nooki
variable contador = 0       // se puede reasignar
constante limite = 10       // no se puede reasignar

contador = contador + 1
```

Asignar a una `constante` es un error de compilación (`NOK2001`). El tipo se deduce del
valor; también puedes escribirlo:

```nooki
variable precio: flotante = 19.99
variable cantidad: entero = 3
variable total: flotante = precio * (cantidad como flotante)
```

### Los cuatro tipos básicos

| Tipo | Qué guarda | Ejemplos |
|---|---|---|
| `entero` | entero de 64 bits con signo | `0`, `42`, `-7`, `9_000_000_000`, `0x1F`, `0b1010`, `0o17` |
| `flotante` | número de doble precisión IEEE 754 | `3.14`, `-0.5`, `2.0`, `1e-9` |
| `booleano` | `verdadero` o `falso` | `verdadero` |
| `texto` | cadena UTF-8 con dueño | `"hola"`, `"""varias líneas"""` |

En los enteros, `_` es un separador visual: `1_000_000` es un millón. Se aceptan las bases
hexadecimal (`0x`), binaria (`0b`) y octal (`0o`).

### Alcance

Una variable vive desde donde se declara hasta el final del bloque `{ }` que la contiene:

```nooki
funcion principal() -> entero {
    si (verdadero) {
        variable dentro = 1
        imprimir(dentro)
    }
    // aquí «dentro» ya no existe
    retornar 0
}
```

### Conversiones: `como`

No hay conversión automática entre `entero` y `flotante`. Se pide explícitamente:

```nooki
funcion principal() -> entero {
    variable a = 7 como flotante       // 7,0
    variable b = 3.9 como entero       // 3: trunca hacia cero
    imprimir(a)
    imprimir(b)
    retornar 0
}
```

Mezclar `entero` y `flotante` sin `como` es un error de tipo (`NOK4001`). Tampoco se puede
concatenar un número a un texto: `"" + 42` es error.

---

## 5. Operadores y expresiones

### Aritméticos

| Operador | Qué hace | Ejemplo |
|---|---|---|
| `+` | suma, y concatenación de textos | `2 + 3` → `5`; `"a" + "b"` → `"ab"` |
| `-` | resta | `5 - 2` → `3` |
| `*` | multiplicación | `4 * 3` → `12` |
| `/` | división entera | `7 / 2` → `3` |
| `%` | resto | `7 % 2` → `1` |
| `**` | potencia | `2 ** 10` → `1024` |

Con `flotante`, `/` da el resultado exacto:

```nooki
funcion principal() -> entero {
    imprimir(7 / 2)        // 3
    imprimir(7.0 / 2.0)    // 3,5
    imprimir(2 ** 10)      // 1024
    imprimir(7 % 2)        // 1
    retornar 0
}
```

La aritmética entera está **comprobada**. Un desbordamiento o una división por cero detiene
el programa con un mensaje y código de salida **70**:

```text
error en ejecucion: desbordamiento al sumar
```

Dentro de un bloque `inseguro { }` se desactivan solo esas comprobaciones aritméticas; la
propiedad y los tipos se siguen comprobando.

### Comparación y lógica

```nooki
funcion principal() -> entero {
    variable a = 5
    variable b = 3
    imprimir(a == b)       // 0
    imprimir(a != b)       // 1
    imprimir(a > b)        // 1
    imprimir(a >= b)       // 1
    imprimir(a < b)        // 0
    imprimir(a <= b)       // 0

    variable hay_sol = verdadero
    variable hace_frio = falso
    imprimir(hay_sol && !hace_frio)   // 1
    imprimir(hay_sol || hace_frio)    // 1
    imprimir(!hay_sol)                // 0
    retornar 0
}
```

`&&` y `||` cortocircuitan: si el primer lado decide, el segundo no se evalúa. La condición
de un `si` o un `mientras` **tiene que ser `booleano`**; `si (1)` es `NOK4005`.

Los textos se comparan por contenido:

```nooki
si (nombre == "Ana") { imprimir("es Ana") }
si ("abeja" < "barco") { imprimir("va antes") }
```

### Operadores de bits y asignación compuesta

| Operadores | Uso |
|---|---|
| `&` `\|` `^` `<<` `>>` | operaciones de bits sobre `entero` |
| `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | asignación |

`<<` y `>>` se comportan como en C: desplazar 64 bits o más es comportamiento indefinido, no
un error controlado. No los uses con cantidades no verificadas.

### Rangos

`..` excluye el final y `..=` lo incluye. Se usan en los bucles `para`:

```nooki
para (i en 0..3) { imprimir(i) }    // 0, 1, 2
para (j en 1..=3) { imprimir(j) }   // 1, 2, 3
```

Un rango suelto fuera de un `para` no lo traduce el backend (`NOK6003`).

### Precedencia (de menor a mayor)

| Potencia | Operadores | Asociatividad |
|---|---|---|
| 10 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | derecha |
| 30 | `??` (reservado; el backend no lo traduce) | derecha |
| 40 | `\|\|` | izquierda |
| 50 | `&&` | izquierda |
| 60 | `==` `!=` `<` `>` `<=` `>=` | izquierda |
| 70 | `\|` | izquierda |
| 75 | `^` | izquierda |
| 80 | `&` | izquierda |
| 90 | `<<` `>>` | izquierda |
| 100 | `..` `..=` | izquierda |
| 110 | `+` `-` | izquierda |
| 120 | `*` `/` `%` | izquierda |
| 130 | `**` | derecha |
| 135 | `como` | izquierda |
| 140 | `-` `!` `&` `*` (unarios) | — |

Los paréntesis mandan:

```nooki
imprimir(2 + 3 * 4)      // 14
imprimir((2 + 3) * 4)    // 20
```

---

## 6. Decisiones y bucles

### `si`, `sinosi`, `sino`

```nooki
funcion principal() -> entero {
    variable edad = 18

    si (edad < 13) {
        imprimir("niño")
    } sinosi (edad < 18) {
        imprimir("adolescente")
    } sino {
        imprimir("adulto")
    }
    retornar 0
}
```

`sinosi` es una palabra del lenguaje; **no existe `sino si`**. Si necesitas más ramas,
encadena `sinosi` o anida un `si` dentro de un `sino`.

### `mientras`

```nooki
variable n = 0
mientras (n < 5) {
    imprimir(n)
    n = n + 1
}
```

### `hacer` … `mientras`

Ejecuta el cuerpo al menos una vez:

```nooki
variable n = 10
hacer {
    imprimir(n)
    n = n + 1
} mientras (n < 3)
```

### `para` con un rango

```nooki
para (i en 0..3) {
    imprimir(i)          // 0, 1, 2
}
para (i en 1..=3) {
    imprimir(i)          // 1, 2, 3
}
```

Con `paso` se avanza de varios en varios. El paso debe ser positivo para que el bucle
avance; si el inicio es mayor o igual que el final, no se ejecuta ninguna vuelta.

```nooki
para (i en 0..10 paso 3) {
    imprimir(i)          // 0, 3, 6, 9
}
```

### `para` con una colección

```nooki
variable nombres = Vector<texto>.nuevo()
nombres.agregar("Ana" + "")
nombres.agregar("Luis" + "")

para (nombre en nombres) {
    imprimir("Hola, " + nombre)
}
```

### `bucle`

Un bucle infinito explícito; se sale con `romper`:

```nooki
variable i = 0
bucle {
    i = i + 1
    si (i >= 3) { romper }
}
imprimir(i)   // 3
```

### `romper` y `continuar`

`romper` sale del bucle; `continuar` salta a la siguiente vuelta:

```nooki
variable i = 0
mientras (verdadero) {
    i = i + 1
    si (i == 2) { continuar }
    si (i > 4) { romper }
    imprimir(i)          // 1, 3, 4
}
```

---

## 7. Funciones

### Declarar y llamar

```nooki
funcion sumar(a: entero, b: entero) -> entero {
    retornar a + b
}

funcion saludar(nombre: &texto) -> vacio {
    imprimir("Hola, " + *nombre)
}

funcion principal() -> entero {
    imprimir(sumar(2, 3))
    variable quien = "Ana"
    saludar(&quien)
    retornar 0
}
```

`-> vacio` (o no poner `->`) significa que no devuelve valor. Una función devuelve **un**
valor; si necesitas varios, devuelve una estructura o una variante.

### Parámetros prestados

Un parámetro con `&` **presta** el valor: la función lo lee sin quedarse con él. Es la forma
correcta de pasar `texto`, `Vector`, `Mapa`, `Caja` o una clase sin moverla.

```nooki
funcion longitud(frase: &texto) -> entero {
    retornar frase.largo()
}

funcion principal() -> entero {
    variable saludo = "hola mundo"
    imprimir(longitud(&saludo))
    imprimir(saludo)        // sigue siendo mío
    retornar 0
}
```

Dentro de la función, el parámetro prestado es un puntero: para usar el valor como tal se
desreferencia con `*`.

```nooki
funcion longitud(frase: &texto) -> entero {
    variable copia = *frase
    retornar copia.largo()
}
```

### Parámetros mutables

`&mutable` presta para leer y escribir:

```nooki
funcion sumar_a(contador: &mutable Caja<entero>, cuanto: entero) -> vacio {
    contador.poner(contador.sacar() + cuanto)
}

funcion principal() -> entero {
    variable caja = Caja<entero>.nuevo(0)
    sumar_a(&mutable caja, 5)
    sumar_a(&mutable caja, 3)
    imprimir(caja.sacar())   // 8
    retornar 0
}
```

Un préstamo normal no deja escribir: `frase = "otra"` dentro de una función con `&texto` es
`NOK4009`; usa `&mutable`.

### Recursión

```nooki
funcion factorial(n: entero) -> entero {
    si (n <= 1) { retornar 1 }
    retornar n * factorial(n - 1)
}

funcion principal() -> entero {
    imprimir(factorial(5))   // 120
    retornar 0
}
```

### Funciones genéricas

```nooki
funcion identidad<T>(x: T) -> T {
    retornar x
}

funcion principal() -> entero {
    imprimir(identidad<entero>(9))
    variable t = "hola" + ""
    imprimir(identidad<texto>(t))
    retornar 0
}
```

El argumento de tipo se escribe explícitamente entre ángulos. Cada combinación genera su
propia versión en C.

---

## 8. Textos

### Literales

```nooki
variable simple = "hola"
variable con_comillas = "dijo \"hola\""
variable multilinea = """primera línea
segunda línea
    con sangría"""
```

Los textos multilínea conservan todo lo que hay entre `"""` y `"""`, incluidos los saltos y
la sangría.

> **Nota de la versión 1.0.0.** El lexer reconoce los escapes `\n`, `\t`, `\\` y `\"`, pero
> el backend actual los conserva como texto literal al imprimir. Para escribir un salto de
> línea real, usa un texto multilínea; para una comilla, ten en cuenta que `\"` también se
> muestra con la barra. Esta es una limitación conocida, no una característica del lenguaje.

No hay interpolación: `"Hola, {nombre}"` imprime las llaves tal cual.

### Concatenar y comparar

`+` une textos; `==`, `!=`, `<`, `>` comparan por contenido:

```nooki
variable nombre = "Ana"
variable saludo = "Hola, " + nombre
imprimir(saludo)

si (nombre == "Ana") { imprimir("es Ana") }
si ("abeja" < "barco") { imprimir("va antes por orden alfabético") }
```

No se puede concatenar un `entero`: `"" + 42` es error de tipo. No hay conversión de número
a texto en la 1.0.0.

### Métodos de `texto`

| Método | Devuelve | Qué hace |
|---|---|---|
| `largo()` | `entero` | número de **bytes** |
| `subtexto(desde, cuantos)` | `texto` | trozo; si `desde` no es válido, vacío |
| `buscar(&aguja)` | `entero` | posición en bytes, o `-1` |
| `contiene(&aguja)` | `booleano` | si aparece |
| `empieza_con(&prefijo)` | `booleano` | si empieza así |
| `mayusculas()` | `texto` | mayúsculas **solo ASCII** |
| `minusculas()` | `texto` | minúsculas **solo ASCII** |
| `recortar()` | `texto` | quita espacios de los extremos |
| `dividir(&separador)` | `Vector<texto>` | trozos |
| `caracter(i)` | `entero` | byte en esa posición, o `-1` |
| `a_entero()` | `entero` | lee un número al principio del texto |
| `agregar_codigo(codigo)` | `texto` | añade un carácter UTF-8 |
| `sin_ultimo()` | `texto` | quita el último carácter (punto de código) |

```nooki
funcion principal() -> entero {
    variable frase = "  Hola, Mundo  "
    variable limpia = frase.recortar()
    imprimir(limpia.largo())          // 11
    imprimir(limpia.mayusculas())     // HOLA, MUNDO
    imprimir(limpia.buscar(","))      // 4
    imprimir(limpia.subtexto(0, 4))   // Hola

    variable espacio = " " + ""
    variable partes = limpia.dividir(&espacio)
    imprimir(partes.largo())          // 2
    retornar 0
}
```

`largo`, `subtexto`, `buscar` y `caracter` trabajan con **bytes**, no con caracteres
Unicode. Para texto en español normal (UTF-8 de dos bytes para tildes) tenlo en cuenta al
calcular posiciones.

---

## 9. Colecciones

### `Vector<T>`

Un vector es una lista dinámica con dueño. En la práctica se usan `Vector<entero>` y
`Vector<texto>`.

```nooki
funcion principal() -> entero {
    variable numeros = Vector<entero>.nuevo()
    numeros.agregar(10)
    numeros.agregar(20)
    numeros.agregar(30)

    imprimir(numeros.largo())     // 3
    imprimir(numeros.obtener(1))  // 20

    numeros.modificar(0, 99)
    imprimir(numeros.obtener(0))  // 99

    variable suma = 0
    para (n en numeros) {
        suma = suma + n
    }
    imprimir(suma)                // 149
    retornar 0
}
```

| Método | Qué hace |
|---|---|
| `Vector<T>.nuevo()` | crea un vector vacío |
| `agregar(valor)` | añade al final |
| `obtener(i)` | lee; fuera de rango devuelve el valor cero (o texto vacío) |
| `modificar(i, valor)` | cambia una posición; fuera de rango no hace nada |
| `largo()` | cuántos hay |
| `vaciar()` | los quita todos (libera los textos) |

Un `Vector<texto>` es dueño de sus elementos. `obtener(i)` devuelve una vista del elemento:
no la guardes más allá de la vida del vector.

### `Mapa<texto, entero>`

Un diccionario de texto a entero.

```nooki
funcion principal() -> entero {
    variable edades = Mapa<texto, entero>.nuevo()
    variable ana = "Ana" + ""
    variable luis = "Luis" + ""
    edades.insertar(&ana, 30)
    edades.insertar(&luis, 25)

    imprimir(edades.largo())          // 2
    imprimir(edades.obtener("Ana"))   // 30

    si (edades.contiene("Ana")) {
        imprimir("Ana está")
    }

    edades.quitar("Luis")
    imprimir(edades.largo())          // 1

    variable claves = edades.claves()
    imprimir(claves.obtener(0))       // Ana
    retornar 0
}
```

| Método | Qué hace |
|---|---|
| `Mapa<texto, entero>.nuevo()` | crea un mapa vacío |
| `insertar(clave, valor)` | inserta o actualiza; la clave se copia |
| `obtener(clave)` | valor, o `0` si no está |
| `contiene(clave)` | `booleano` |
| `quitar(clave)` | `1` si estaba, `0` si no |
| `largo()` | cuántas claves |
| `claves()` | `Vector<texto>` con las claves |

No se puede recorrer un `Mapa` directamente con `para`; recorre `mapa.claves()`.

### `Caja<entero>`

Una caja es un valor en el montículo con un único dueño. Se usa para prestar un valor
mutable a una función:

```nooki
funcion principal() -> entero {
    variable caja = Caja<entero>.nuevo(7)
    imprimir(caja.sacar())   // 7
    caja.poner(9)
    imprimir(caja.sacar())   // 9
    retornar 0
}
```

| Método | Qué hace |
|---|---|
| `Caja<entero>.nuevo(valor)` | crea la caja |
| `sacar()` | lee el valor |
| `poner(valor)` | lo cambia |

En la implementación actual la caja es de enteros de 64 bits; `Caja<texto>` no genera C
válido. Usa `Vector<texto>` o una clase si necesitas otro tipo.

---

## 10. Estructuras y clases

### Estructuras: datos por valor

Una `estructura` agrupa campos y se pasa **por valor**: al asignarla, se copia.

```nooki
estructura punto {
    publico x: entero
    publico y: entero
}

funcion sumar(a: punto, b: punto) -> punto {
    retornar punto { x: a.x + b.x, y: a.y + b.y }
}

funcion principal() -> entero {
    variable a = punto { x: 1, y: 2 }
    variable b = punto { x: 10, y: 20 }
    variable c = sumar(a, b)

    imprimir(c.x)   // 11
    imprimir(c.y)   // 22

    c.x = 99
    imprimir(a.x)   // 1: la copia no toca el original
    retornar 0
}
```

Los campos se declaran con `publico` o `privado`. Un campo `privado` no se puede leer desde
fuera del módulo.

El uso previsto de una estructura es **datos por valor**, sin identidad ni jerarquía. En la
implementación 1.0.0 el parser también acepta métodos y `hereda` dentro de una estructura,
pero el soporte es **parcial**: una llamada a un método heredado directamente sobre la
estructura hija puede fallar en el backend. Para comportamiento y polimorfismo, usa clases.

### Clases: objetos por referencia

Una `clase` vive en el montículo y la variable es una referencia. Se crea con `nuevo`:

```nooki
clase contador {
    publico valor: entero

    publico funcion subir() -> vacio {
        este.valor = este.valor + 1
    }
}

funcion principal() -> entero {
    variable c = nuevo contador { valor: 10 }
    c.subir()
    c.subir()
    imprimir(c.valor)   // 12
    retornar 0
}
```

- `nuevo` reserva el objeto; la variable es su dueña y lo libera al salir del ámbito.
- `este` es el objeto dentro de sus métodos.
- Los campos heredados se nombran igual al construir.

### Herencia y polimorfismo

```nooki
clase figura {
    publico nombre: texto

    publico funcion area() -> entero {
        retornar 0
    }
}

clase cuadrado hereda figura {
    publico lado: entero

    publico funcion area() -> entero {
        retornar este.lado * este.lado
    }
}

funcion principal() -> entero {
    variable f: figura = nuevo cuadrado { nombre: "c", lado: 4 }
    imprimir(f.area())    // 16: despacho a cuadrado
    imprimir(f.nombre)    // c
    retornar 0
}
```

Una variable declarada con el tipo base puede apuntar a un objeto de una clase hija. La
llamada se resuelve en ejecución por la tabla de la clase: es **despacho dinámico** de
verdad, no una decisión del compilador.

### `super`

Para llamar a la implementación de la base:

```nooki
clase animal {
    publico funcion presentarse() -> texto {
        retornar "Soy un animal"
    }
}

clase perro hereda animal {
    publico funcion presentarse() -> texto {
        retornar super.presentarse() + " y ladro"
    }
}
```

### Cuándo usar cada una

| | `estructura` | `clase` |
|---|---|---|
| Paso de valores | por valor (copia) | por referencia |
| Creación | `punto { x: 1 }` | `nuevo punto { x: 1 }` |
| Herencia | no (el parser acepta `hereda`, con soporte parcial) | sí |
| Métodos | sí, pero sin despacho dinámico ni uso recomendado | sí, con despacho dinámico |
| Uso típico | datos pequeños sin identidad | objetos con identidad y jerarquía |

---

## 11. Rasgos

Un **rasgo** es un contrato: una lista de métodos que un tipo se compromete a tener. Sirve
para tratar igual a tipos que **no comparten padre**.

```nooki
rasgo Saluda {
    funcion saludar() -> texto
}

clase persona implementa Saluda {
    publico nombre: texto

    publico funcion saludar() -> texto {
        retornar "Hola, soy " + este.nombre
    }
}

clase robot implementa Saluda {
    publico modelo: texto

    publico funcion saludar() -> texto {
        retornar "Bip. Unidad " + este.modelo
    }
}

funcion presentar(quien: &Saluda) -> vacio {
    imprimir(quien.saludar())
}

funcion principal() -> entero {
    variable p = nuevo persona { nombre: "Ana" }
    variable r = nuevo robot { modelo: "R2" }
    presentar(&p)
    presentar(&r)
    retornar 0
}
```

Salida:

```text
Hola, soy Ana
Bip. Unidad R2
```

El compilador verifica que la clase tiene todos los métodos del rasgo con la firma exacta.
Si falta uno, no compila (`NOK3011` o `NOK4012`). Una clase puede implementar varios rasgos:

```nooki
clase cuadrado implementa Describible, Medible {
    // ...
}
```

Y puede heredar y cumplir rasgos a la vez:

```nooki
clase gato hereda animal implementa Describible {
    // ...
}
```

---

## 12. Enumeraciones y `coincide`

Una **enumeración** es un tipo cuyos valores son un conjunto cerrado de variantes. Cada
variante puede llevar datos.

```nooki
enumeracion forma {
    circulo(radio: entero)
    cuadrado(lado: entero)
    sin_forma
}

funcion area(f: forma) -> entero {
    coincide (f) {
        caso circulo(r) => {
            retornar 3 * r * r
        }
        caso cuadrado(l) => {
            retornar l * l
        }
        caso forma.sin_forma => {
            retornar 0
        }
    }
}

funcion principal() -> entero {
    variable c = circulo(2)
    variable q = cuadrado(4)
    variable n = forma.sin_forma

    imprimir(area(c))   // 12
    imprimir(area(q))   // 16
    imprimir(area(n))   // 0
    retornar 0
}
```

Reglas de escritura:

- Las variantes **con datos** se usan sin cualificar: `circulo(2)`, `caso circulo(r)`.
- Las variantes **sin datos** se cualifican: `forma.sin_forma`, `caso forma.sin_forma`.

### `coincide` es exhaustivo

Si falta una variante, **no compila** (`NOK4006`). Es lo que evita el clásico «olvidé un
caso». El comodín `_` cubre el resto:

```nooki
coincide (dia) {
    caso 1 => imprimir("lunes")
    caso 2 => imprimir("martes")
    caso _ => imprimir("otro")
}
```

También funciona con textos y booleanos:

```nooki
coincide (nombre) {
    caso "Ana" => imprimir("hola Ana")
    caso _ => imprimir("hola desconocido")
}
```

Una rama que devuelve un valor necesita un bloque `{ }`; una rama de una sola sentencia puede
escribirse directamente después de `=>`.

---

## 13. Errores como valores

Nooki **no tiene excepciones**. Una operación que puede fallar devuelve un `resultado`: o el
valor, o el motivo del fallo.

```nooki
usar resultado

funcion dividir(a: entero, b: entero) -> resultado {
    si (b == 0) {
        retornar Error("no se puede dividir por cero")
    }
    retornar Ok(a / b)
}

funcion principal() -> entero {
    coincide (dividir(10, 2)) {
        caso Ok(v) => imprimir(v)
        caso Error(m) => imprimir(m)
    }

    coincide (dividir(10, 0)) {
        caso Ok(v) => imprimir(v)
        caso Error(m) => imprimir(m)
    }
    retornar 0
}
```

Salida:

```text
5
no se puede dividir por cero
```

`resultado` es una enumeración de la biblioteca estándar:

```nooki
enumeracion resultado {
    Ok(valor: entero)
    Error(mensaje: texto)
}
```

### El operador `?`

Dentro de una función que devuelve `resultado`, `?` propaga el error sin escribir el
`coincide`:

```nooki
usar resultado

funcion mitad(a: entero) -> resultado {
    si (a % 2 == 1) {
        retornar Error("es impar")
    }
    retornar Ok(a / 2)
}

funcion doble_mitad(a: entero) -> resultado {
    variable m = mitad(a)?
    retornar Ok(m * 2)
}

funcion principal() -> entero {
    coincide (doble_mitad(8)) {
        caso Ok(v) => imprimir(v)      // 8
        caso Error(m) => imprimir(m)
    }
    retornar 0
}
```

`?` solo se puede usar en una función que devuelve `resultado` (`NOK4011`). No se aplica a
otros tipos ni a `principal`, que devuelve `entero`.

### El módulo `archivo` tiene su propio resultado

`archivo.leer` y `archivo.escribir` devuelven una enumeración `lectura` con `Bien` y `Mal`,
no el `resultado` de `Ok`/`Error`. Se trata igual, con `coincide`:

```nooki
usar archivo

funcion principal() -> entero {
    variable ruta = "datos.txt" + ""
    variable escrito = archivo.escribir(&ruta, "hola")
    coincide (escrito) {
        caso Bien(v) => imprimir("escrito")
        caso Mal(m) => imprimir(m)
    }
    retornar 0
}
```

---

## 14. Genéricos

Un tipo o una función puede llevar un parámetro de tipo. Se instancia explícitamente:

```nooki
estructura par<T> {
    publico primero: T
    publico segundo: T
}

funcion principal() -> entero {
    variable p = par<entero> { primero: 3, segundo: 4 }
    imprimir(p.primero + p.segundo)   // 7

    variable t = par<texto> { primero: "a", segundo: "b" }
    imprimir(t.primero + t.segundo)   // ab
    retornar 0
}
```

Cada combinación de tipos genera su propia versión en C: no hay borrado de tipos ni coste en
ejecución.

Los genéricos integrados son:

- `Vector<entero>` y `Vector<texto>`
- `Mapa<texto, entero>`
- `Caja<entero>` (la caja es de enteros en la implementación actual)
- `resultado` (en la práctica, con `Ok(entero)` y `Error(texto)`)

No hay restricciones de tipo (`<T: Rasgo>`) ni genéricos con varios parámetros en la
biblioteca más allá de `Mapa<texto, entero>`.

---

## 15. Módulos y proyectos

### Un archivo puede usar otro

Un módulo es un archivo `.oki` **sin `principal`**. Se importa con `usar`:

```nooki
// geometria.oki
publico funcion area_cuadrado(lado: entero) -> entero {
    retornar lado * lado
}
```

```nooki
// principal.oki
usar geometria

funcion principal() -> entero {
    imprimir(geometria.area_cuadrado(4))   // 16
    retornar 0
}
```

Las funciones se llaman con el prefijo del módulo (`geometria.area_cuadrado`). Los **tipos**
y las **variantes de enumeración** se importan sin prefijo: después de `usar controles`
puedes escribir `nuevo boton { ... }`.

Solo lo marcado `publico` es visible desde fuera; lo demás es privado del módulo.

### Dónde busca el compilador

Cuando un archivo hace `usar nombre`, el compilador busca, en este orden:

1. `nombre.oki` junto al archivo que hace el `usar`.
2. Las carpetas de las dependencias del `nooki.toml` (su raíz y su `biblioteca/`).
3. `biblioteca/nombre.oki` en la carpeta del archivo y en sus carpetas padre, subiendo.
4. La biblioteca estándar, que va dentro del compilador.

Si hay un módulo local y otro en una dependencia con el mismo nombre, la dependencia se mira
antes que la `biblioteca/` local. Evita nombres duplicados entre proyectos.

### Un proyecto

```bash
nooki nuevo agenda
cd agenda
nooki construir
./principal
```

Estructura creada:

```text
agenda/
├── nooki.toml            el manifiesto
├── principal.oki         el programa
└── biblioteca/
    └── herramientas.oki  un módulo del proyecto
```

`nooki construir` busca la entrada en `principal.oki`; si no está, en
`fuente/principal.oki`. Sin `principal.oki` no hay proyecto que construir. Si la entrada está
en `fuente/`, el binario por defecto queda en `fuente/principal` (o `fuente/principal.exe`),
no en la raíz; usa `-o` si prefieres otra ubicación.

### El manifiesto `nooki.toml`

```toml
[proyecto]
nombre = "agenda"
version = "0.1.0"

[dependencias]
utilidades = { ruta = "../utilidades" }
```

- `[proyecto]`: `nombre` y `version`. Si faltan, se usan valores por defecto.
- `[dependencias]`: solo se admite `nombre = { ruta = "..." }`; la ruta va entre comillas y
  es relativa al manifiesto.
- No hay versiones, descarga, registro de paquetes ni dependencias transitivas: una
  dependencia es una carpeta que ya está en el disco.

### Pruebas del proyecto

Crea `pruebas/` y un archivo por prueba. La salida se compara con el archivo
`<nombre>.oki.esperado`:

```text
agenda/
└── pruebas/
    ├── saludo.oki
    └── saludo.oki.esperado
```

```bash
nooki probar
```

```text
ok    pruebas\saludo.oki

todas las pruebas pasaron
```

El archivo `.esperado` debe contener exactamente la salida, incluido el salto de línea final.
La comparación ignora los `\r` de Windows, pero no los espacios ni las líneas de más.

---

## 16. Memoria: propiedad y préstamos

Esta es la parte que hace a Nooki distinto. La regla es una:

> **Cada valor con dueño tiene un solo dueño a la vez. Pasarlo por valor lo mueve.**

Los tipos con dueño son `texto`, `Vector`, `Mapa`, `Caja` y las clases.

### Mover

```nooki
funcion principal() -> entero {
    variable original = "hola"
    variable copia = original       // se MUEVE: «original» queda vacío
    imprimir(copia)
    retornar 0
}
```

Usar `original` después de moverlo es error de compilación:

```nooki
variable original = "hola"
variable copia = original
imprimir(original)                  // error[NOK5001]: el valor se movió antes
```

### Prestar

Si solo quieres leer, presta con `&`:

```nooki
funcion longitud(frase: &texto) -> entero {
    retornar frase.largo()
}

funcion principal() -> entero {
    variable saludo = "hola"
    imprimir(longitud(&saludo))
    imprimir(saludo)                // sigue siendo mío
    retornar 0
}
```

### Prestar para escribir

```nooki
funcion sumar_a(contador: &mutable Caja<entero>, cuanto: entero) -> vacio {
    contador.poner(contador.sacar() + cuanto)
}

funcion principal() -> entero {
    variable caja = Caja<entero>.nuevo(0)
    sumar_a(&mutable caja, 1)
    imprimir(caja.sacar())          // 1
    retornar 0
}
```

### Las reglas, en una tabla

| Regla | Qué evita | Código si se rompe |
|---|---|---|
| Un solo dueño | dos liberaciones del mismo valor | `NOK5004` |
| Pasar por valor mueve | copias caras invisibles | — |
| No se usa lo movido | leer memoria liberada | `NOK5001` |
| No se mueve mientras está prestado | referencias colgando | `NOK5007` |
| No se guarda un préstamo en una variable | préstamos que sobreviven al dueño | `NOK5002` |
| No escapa un préstamo local | devolver algo que muere | `NOK5006` |
| Un préstamo mutable, solo | escrituras simultáneas | `NOK5003` |
| No se escribe por un préstamo inmutable | cambios inesperados | `NOK4009` |
| No se copia un compuesto con campos con dueño | doble liberación | `NOK5004` |

**Cuando el compilador no puede garantizar algo, lo rechaza.** No hay casos «ya veremos en
ejecución».

### Qué libera el compilador por ti

- Los locales con dueño, al salir del bloque o de la función.
- El búfer viejo de un `texto` al reasignarlo.
- El contenido viejo de una colección al reasignarla.
- Las claves y entradas de un mapa al liberarlo.
- Los locales al salir de un bucle con `romper` o `continuar`.

No tienes que escribir `free` ni `liberar` para los valores normales. Los **temporales** de
una expresión no se liberan (fuga menor conocida, asunto A-18); no corrompe nada.

### `inseguro`

`inseguro { }` desactiva **solo** las comprobaciones de la aritmética entera. La propiedad y
los tipos se siguen comprobando. Sirve para código que sabe lo que hace con los números:

```nooki
variable a = 1
inseguro {
    a = a + 1        // sin comprobación de desbordamiento
}
```

`asm!("...")` solo se permite dentro de `inseguro` y solo funciona con GCC/Clang, no con
MSVC.

---

## 17. Entrada, salida y archivos

### Leer de la entrada estándar

```nooki
funcion principal() -> entero {
    variable linea = leer_linea()
    mientras (linea.largo() > 0) {
        imprimir("leído: " + linea)
        linea = leer_linea()
    }
    retornar 0
}
```

- `leer_linea()` devuelve la línea sin el salto; al final de la entrada devuelve un texto
  vacío.
- `leer_entero()` lee una línea y la convierte a entero.

```bash
printf "Ana\nLuis\n" | ./programa
```

### Archivos

```nooki
usar archivo

funcion principal() -> entero {
    variable ruta = "datos.txt" + ""
    variable escrito = archivo.escribir(&ruta, "hola archivo")
    coincide (escrito) {
        caso Bien(v) => imprimir("escrito")
        caso Mal(m) => imprimir(m)
    }

    variable leido = archivo.leer(&ruta)
    coincide (leido) {
        caso Bien(contenido) => imprimir(contenido)
        caso Mal(motivo) => imprimir(motivo)
    }
    retornar 0
}
```

| Función | Devuelve |
|---|---|
| `archivo.existe(&ruta)` | `booleano` |
| `archivo.leer(&ruta)` | `lectura` con `Bien(contenido)` o `Mal(motivo)` |
| `archivo.escribir(&ruta, &contenido)` | `lectura`; `Bien("")` si fue bien |

`archivo.escribir` sobrescribe el archivo. No hay modo de añadir al final ni de listar
directorios en la biblioteca estándar.

---

## 18. Una ventana

Nooki trae una interfaz gráfica sencilla: ventana, dibujo, colocación, controles,
accesibilidad e imágenes. Todo en la biblioteca estándar.

```nooki
usar ventana
usar controles

funcion principal() -> entero {
    variable titulo = "Hola" + ""
    variable v = ventana.abrir(&titulo, 400, 300)

    si (v == 0) {
        imprimir("sin entorno grafico")
        retornar 0
    }

    variable boton = nuevo boton { x: 20, y: 20, ancho: 160, alto: 44,
                                   contenido: "Pulsame" + "",
                                   fondo: 0x89B4FA, color: 0x11111B }

    variable dibujar = verdadero
    mientras (ventana.viva(v)) {
        si (dibujar || ventana.repintar(v)) {
            ventana.limpiar(v, 0x1E1E2E)
            boton.dibujar(v)
            ventana.presentar(v)
            dibujar = falso
        }

        variable e = ventana.evento(v)
        mientras (e != 0) {
            si (ventana.es_pulsacion(e)) {
                si (boton.pulsado(ventana.evento_x(v), ventana.evento_y(v))) {
                    imprimir("pulsado")
                }
                dibujar = verdadero
            }
            e = ventana.evento(v)
        }
        ventana.pausa(16)
    }

    ventana.cerrar(v)
    retornar 0
}
```

Las cinco trampas de la interfaz —repintar, `presentar`, `abrir` devuelve 0,
`libx11-dev` y las palabras reservadas— están explicadas en la
[guía de interfaz](../INTERFAZ.md). En resumen:

1. Vuelve a dibujar cuando `ventana.repintar(v)` sea verdadero.
2. Llama a `ventana.presentar(v)` después de dibujar.
3. Si `ventana.abrir` devuelve `0`, no hay entorno gráfico: termina bien.
4. En Linux, instala `libx11-dev` para compilar y usa `xvfb-run` si no hay pantalla.
5. No llames `texto`, `clase`, `vacio`, `tamano` o `lista` a tus variables: son palabras
   reservadas.

---

## 19. Inteligencia artificial

La biblioteca incluye tensores, tablas, autograd y redes neuronales. Los números con
decimales se representan a veces como enteros **por mil** (2500 significa 2,5).

### Una red que aprende XOR

```nooki
usar redes

funcion principal() -> entero {
    variable red = redes.nueva(2, 4)

    redes.ejemplo(red, 0.0, 0.0, 0.0, 0.0, 0.0)
    redes.ejemplo(red, 0.0, 1.0, 0.0, 0.0, 1.0)
    redes.ejemplo(red, 1.0, 0.0, 0.0, 0.0, 1.0)
    redes.ejemplo(red, 1.0, 1.0, 0.0, 0.0, 0.0)

    redes.entrenar(red, 1500, 1.0)
    imprimir(redes.predecir(red, 1.0, 0.0, 0.0, 0.0))
    redes.liberar(red)
    retornar 0
}
```

La salida es un valor entre 0 y 1 (por ejemplo `0,975788`). `nueva` admite de 1 a 4
entradas y de 1 a 64 neuronas ocultas. Las entradas que no uses se pasan como `0.0`.

### Guardar lo aprendido

```nooki
usar redes

funcion principal() -> entero {
    variable archivo = "modelo.red" + ""
    variable red = redes.cargar(&archivo)

    si (red == 0) {
        red = redes.nueva(2, 4)
        redes.ejemplo(red, 0.0, 0.0, 0.0, 0.0, 0.0)
        redes.ejemplo(red, 1.0, 1.0, 0.0, 0.0, 1.0)
        redes.entrenar(red, 500, 1.0)
        redes.guardar(red, &archivo)
    }

    imprimir(redes.predecir(red, 1.0, 1.0, 0.0, 0.0))
    redes.liberar(red)
    retornar 0
}
```

El archivo es binario, con cabecera `NOOKIRED`; guarda pesos, sesgos y ejemplos. Cargar un
archivo que no es de Nooki devuelve `0`.

---

## 20. Las herramientas del día a día

| Comando | Qué hace |
|---|---|
| `nooki nuevo <nombre>` | crea un proyecto |
| `nooki construir [carpeta]` | compila el proyecto actual o el de esa carpeta |
| `nooki compilar <archivo>` | compila un `.oki` a un ejecutable |
| `nooki ejecutar <archivo>` | compila y ejecuta |
| `nooki probar` | ejecuta `pruebas/*.oki` y compara con `.esperado` |
| `nooki verificar <archivo>` | comprueba sin generar C |
| `nooki formatear <archivo>...` | normaliza la sangría |
| `nooki documentar` | genera `API.md` con lo público |
| `nooki ast <archivo>` | muestra el árbol de sintaxis |
| `nooki limpiar` | borra artefactos |
| `nooki lsp` | servidor de lenguaje para el editor |
| `nooki ayuda` | lista los comandos |
| `nooki version` | muestra la versión |

### Opciones útiles

```bash
nooki compilar hola.oki -o saludo
nooki compilar hola.oki --emitir-c     # muestra el C generado y no compila
nooki compilar hola.oki --conservar    # conserva el .c intermedio
nooki formatear principal.oki --comprobar
nooki documentar --salida              # escribe el Markdown por pantalla
```

`--comprobar` devuelve código de salida 65 si el archivo no está formateado; es lo que se usa
en integración continua.

### El editor

Los archivos `.oki` se colorean en VS Code, Vim/Neovim, Sublime Text, Zed, JetBrains y
cualquier editor con gramáticas TextMate. Las instrucciones están en
[`editores/LEEME.md`](../../editores/LEEME.md).

Para el servidor de lenguaje, configura tu editor para lanzar `nooki lsp` por entrada/salida
estándar. Da diagnósticos, autocompletado, ir a la definición y formato del documento.

---

## 21. Errores frecuentes

Los diagnósticos tienen la forma:

```text
error[NOK5001]: no se puede usar «letras»: el valor se movió antes
  --> ejemplo.oki:3:15
  |
3 |     imprimir(letras)
  |               ^^^^^^
  |
   = ayuda: se movió a otro sitio: use «&» para prestarlo sin transferirlo
```

| Código | Qué significa | Cómo se arregla |
|---|---|---|
| `NOK1001` | palabra clave escrita con tilde | quita la tilde: `funcion`, no `función` |
| `NOK1004` | carácter no válido o BOM | guarda UTF-8 sin BOM; quita el carácter |
| `NOK2002` | el analizador esperaba otra cosa | revisa la línea y el símbolo marcado |
| `NOK2003` | falta `principal` | añade `funcion principal() -> entero` |
| `NOK3001` | nombre no declarado | decláralo antes o revisa la tilde |
| `NOK3003` | tipo desconocido | revisa el nombre; muchos tipos reservados no existen |
| `NOK3008` | módulo no encontrado | revisa la ruta, `biblioteca/` o `nooki.toml` |
| `NOK3011` | falta un método del rasgo | impleméntalo con la firma exacta |
| `NOK4001` | tipo incompatible | convierte con `como` o cambia el valor |
| `NOK4005` | la condición no es booleana | compara: `si (n != 0)` |
| `NOK4006` | `coincide` sin cubrir todos los casos | añade la variante que falta o `caso _` |
| `NOK4011` | `?` fuera de una función `resultado` | devuelve `resultado` o trata el error |
| `NOK5001` | usar un valor movido | presta con `&` en vez de mover |
| `NOK5002` | guardar un préstamo en una variable | pásalo directo como argumento |
| `NOK5004` | copiar algo con dueño | presta con `&` o construye otro |
| `NOK5006` | devolver un préstamo local | devuelve por valor |
| `NOK5007` | mover mientras está prestado | termina el préstamo antes |
| `NOK6001` | no hay compilador de C | instala `cc`/`gcc`/`clang` o define `NOOKI_CC` |
| `NOK6002` | el compilador de C falló | revisa el C generado con `--conservar` |
| `NOK6003` | el backend no traduce esa construcción | usa una forma soportada |

### Las tres causas más comunes

1. **Palabras reservadas como nombres.** `texto`, `clase`, `esperar`, `vacio`, `interna`,
   `lista`, `tamano` no pueden ser variables. El error es `NOK2002`.
2. **Olvidar prestar.** Pasar un `texto` por valor lo mueve; si lo usas después,
   `NOK5001`. Usa `&` en el parámetro y `&` en la llamada.
3. **Confundir `=` con `==`.** `=` asigna; `==` compara.

---

## 22. Lo que todavía no existe

Dicho aquí para que no lo busques:

| No existe | Alternativa |
|---|---|
| Excepciones | `resultado`, `?` y `coincide` |
| Conversión implícita entre tipos | `como` |
| `sino si` | `sinosi` o anidar |
| Interpolación de textos | concatenar con `+` |
| Literales de carácter `'a'` | no están soportados por el backend |
| `"" + 42` | no hay conversión de número a texto |
| `??` | el parser lo acepta, pero el backend no lo traduce |
| `?.` | el parser lo acepta, pero el backend no lo traduce |
| Sobrecarga de funciones | nombres distintos |
| Parámetros por defecto | funciones con otro nombre |
| Cierres y funciones de primera clase | — |
| Tuplas, arreglos fijos, conjuntos, opcionales | — |
| Herencia múltiple de clases | un rasgo por contrato |
| Macros, `async`, `await` | reservadas, sin uso |
| `Caja<texto>` | usa una clase o `Vector<texto>` |
| Métodos o herencia en estructuras | soporte parcial; usa clases |
| `Vector<flotante>` | usa `Vector<entero>` o una clase |
| PNG | solo BMP en `imagenes` |
| Ventanas en macOS | no hay backend Cocoa; `abrir` devuelve 0 y el enlace con UI no está validado |
| Cursor y selección en el campo de texto | el cursor siempre está al final |
| Recolocación al redimensionar | la ventana se estira, el contenido no se recoloca |

Todo esto está inventariado en [`DECISIONES.md`](../../DECISIONES.md) y en las
[notas de la versión](../../NOTAS_DE_VERSION.md).

---

## 23. Siguientes pasos

Ya tienes lo necesario para escribir programas de consola, datos y escritorio. Para seguir:

1. **La guía paso a paso:** [`GUIA.md`](../../GUIA.md), trece capítulos con salida real.
2. **El manual completo:** [`MANUAL.md`](../../MANUAL.md), con programas verificados.
3. **La referencia del lenguaje:** [`REFERENCIA.md`](../../REFERENCIA.md) y la
   [referencia de esta colección](REFERENCIA_DEL_LENGUAJE.md), para consultar tipos,
   operadores, biblioteca y diagnósticos.
4. **La especificación:** [`spec/especificacion-v0.1.md`](../../spec/especificacion-v0.1.md),
   el documento normativo (incluye cosas que la implementación todavía no tiene).
5. **Los ejemplos:** [`ejemplos/`](../../ejemplos/), ocho programas ejecutables.
6. **La interfaz:** [`docs/INTERFAZ.md`](../INTERFAZ.md).
7. **Los proyectos y dependencias:** [`docs/PROYECTO.md`](../PROYECTO.md).
8. **La guía del administrador:** [instalación, equipos y CI](GUIA_DEL_ADMINISTRADOR.md).
9. **La guía del desarrollador:** [arquitectura y contribución](GUIA_DEL_DESARROLLADOR.md).

### Ejercicios para practicar

1. Imprime los números del 1 al 100, pero sustituye los múltiplos de 3 por `Fizz` y los de 5
   por `Buzz`.
2. Pide una línea y di cuántas palabras tiene (usa `dividir(" ")`).
3. Crea una estructura `rectangulo` y una función que calcule su área.
4. Crea una clase `cuenta` con `saldo` privado y métodos `ingresar` y `retirar` que no
   permitan saldo negativo.
5. Haz una jerarquía `figura` → `circulo` y `cuadrado`, y recórrela con `&figura`.
6. Define una enumeración `estado` con `Pendiente`, `Hecha` y `Cancelada`, y usa `coincide`.
7. Escribe una función que divida y devuelva `resultado`; propaga el error con `?`.
8. Guarda una lista de nombres en un archivo y vuelve a leerla.
9. Haz una ventana con un botón que imprima un saludo.
10. Entrena la red XOR y guarda el modelo; vuelve a cargarlo en otra ejecución.
