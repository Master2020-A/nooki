# Manual del lenguaje Nooki

Un recorrido completo, de lo más simple a lo más avanzado. Cada capítulo trae programas completos que
se pueden copiar, compilar y ejecutar tal cual.

> **Los ejemplos están verificados.** Todos los bloques marcados como `nooki` son programas completos
> que la suite de pruebas compila en cada `ctest`: si uno deja de ser válido, la documentación falla.

## Índice

1. [Antes de empezar](#1-antes-de-empezar)
2. [La forma de un programa](#2-la-forma-de-un-programa)
3. [Valores y variables](#3-valores-y-variables)
4. [Operadores](#4-operadores)
5. [Textos](#5-textos)
6. [Decisiones](#6-decisiones)
7. [Bucles](#7-bucles)
8. [Funciones](#8-funciones)
9. [Colecciones](#9-colecciones)
10. [Estructuras](#10-estructuras)
11. [Clases y herencia](#11-clases-y-herencia)
12. [Rasgos](#12-rasgos)
13. [Enumeraciones y `coincide`](#13-enumeraciones-y-coincide)
14. [Errores como valores](#14-errores-como-valores)
15. [Genéricos](#15-genéricos)
16. [Módulos y proyectos](#16-módulos-y-proyectos)
17. [Memoria: propiedad y préstamos](#17-memoria-propiedad-y-préstamos)
18. [Llamar a C](#18-llamar-a-c)
19. [`inseguro` y `asm!`](#19-inseguro-y-asm)
20. [La biblioteca estándar](#20-la-biblioteca-estándar)
21. [Interfaz gráfica](#21-interfaz-gráfica)
22. [Inteligencia artificial](#22-inteligencia-artificial)
23. [Las herramientas](#23-las-herramientas)
24. [Errores frecuentes](#24-errores-frecuentes)
25. [Apéndice: palabras reservadas](#25-apéndice-palabras-reservadas)

---

## 1. Antes de empezar

### Qué es Nooki

Un lenguaje **compilado**: el código se traduce a C y ese C se entrega a un compilador del sistema. El
resultado es un ejecutable nativo, sin máquina virtual ni intérprete.

Tres cosas lo distinguen:

- **Se escribe en español**: `funcion`, `variable`, `mientras`, `retornar`. Sin tildes en las palabras
  del lenguaje (los identificadores sí pueden llevarlas).
- **La memoria es determinista**: sin recolector de basura y sin excepciones. El compilador sabe en
  todo momento quién es el dueño de cada valor, y cuando no puede garantizarlo, **lo rechaza**.
- **Trae interfaz gráfica e IA en la biblioteca**: no hay que buscar dependencias.

### Instalar

Necesitas un compilador de C en el `PATH` (`cc`, `gcc`, `clang` o MSVC): es lo único que Nooki no
incluye, porque usa el que ya tengas.

```bash
# comprobar que está
cc --version
```

Y el compilador. Si tienes el paquete de la distribución, un comando lo deja en el `PATH`:

```powershell
.\herramientas\instalar.ps1
```

```bash
# comprobar que Nooki está (después de cerrar y abrir la terminal)
nooki version
```

En Linux, para programas con ventana, hace falta además `libx11-dev`.

### El primer programa

Crea un archivo `hola.oki`:

```nooki
funcion principal() -> entero {
    imprimir("¡Hola, Nooki!")
    retornar 0
}
```

Y ejecútalo:

```bash
$ nooki ejecutar hola.oki
¡Hola, Nooki!
```

Eso es todo: no hay proyecto, ni configuración, ni archivo de dependencias.

### Compilar a un ejecutable

```bash
$ nooki compilar hola.oki
$ ./hola
¡Hola, Nooki!
```

El ejecutable tiene el mismo nombre que el fuente, junto a él. Con `-o` se elige otro nombre.

---

## 2. La forma de un programa

### El archivo

Los archivos de Nooki terminan en **`.oki`** y se escriben en UTF-8.

### `funcion principal`

Todo programa empieza a ejecutarse en `principal`. Devuelve un `entero`, que es el código de salida del
proceso: `0` significa que fue bien.

```nooki
funcion principal() -> entero {
    imprimir("empieza")
    retornar 0
}
```

Si el programa termina sin `retornar`, sale con `0`.

### Declaraciones de primer nivel

Fuera de las funciones solo puede haber **declaraciones**: funciones, estructuras, clases, rasgos y
enumeraciones. No hay variables globales.

```nooki
// Tipos
estructura punto {
    publico x: entero
    publico y: entero
}

// Funciones
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
// Un comentario de línea

/* Y un comentario
   de varias líneas */

funcion principal() -> entero {
    imprimir("texto con // dentro de comillas") // sí, esto es un comentario
    retornar 0
}
```

### La sentencia termina al final de la línea

Nooki **no usa punto y coma**. Cada sentencia ocupa su línea, y los bloques se delimitan con llaves.

```nooki
funcion principal() -> entero {
    variable a = 1
    variable b = 2
    imprimir(a + b)
    retornar 0
}
```

### Salida por pantalla

`imprimir` acepta un `entero`, un `flotante`, un `booleano` o un `texto`, y añade un salto de línea:

```nooki
funcion principal() -> entero {
    imprimir(42)
    imprimir(3.5)
    imprimir(verdadero)
    imprimir("una frase")
    retornar 0
}
```

---

## 3. Valores y variables

### `variable` y `constante`

`variable` declara un valor que puede cambiar. `constante`, uno que no:

```nooki
funcion principal() -> entero {
    variable contador = 0
    contador = contador + 1        // se puede

    constante limite = 10          // no se puede reasignar
    imprimir(contador + limite)
    retornar 0
}
```

Asignar a una `constante` es un error de compilación (`NOK2001`), no un aviso.

### Los cuatro tipos básicos

```nooki
funcion principal() -> entero {
    variable entero_pequeno = 42
    variable grande = 9_000_000_000        // los guiones bajos son para leerlo mejor
    variable decimal = 3.1416
    variable cierto = verdadero
    variable frase = "hola"

    imprimir(entero_pequeno)
    imprimir(grande)
    imprimir(decimal)
    imprimir(cierto)
    imprimir(frase)
    retornar 0
}
```

| Tipo | Qué guarda | Ejemplos |
|---|---|---|
| `entero` | números enteros de 64 bits | `0`, `42`, `-7`, `9_000_000_000` |
| `flotante` | números con decimales (doble precisión) | `3.14`, `-0.5`, `2.0` |
| `booleano` | `verdadero` o `falso` | `verdadero` |
| `texto` | cadenas de caracteres | `"hola"`, `"línea\nnueva"` |

### Anotar el tipo

Normalmente el tipo se deduce del valor. Se puede escribir cuando aclara algo:

```nooki
funcion principal() -> entero {
    variable precio: flotante = 19.99
    variable cantidad: entero = 3
    variable total: flotante = precio * (cantidad como flotante)
    imprimir(total)
    retornar 0
}
```

### Convertir entre tipos: `como`

**No hay conversión automática** entre `entero` y `flotante`: se pide con `como`.

```nooki
funcion principal() -> entero {
    variable a = 7 como flotante      // 7.0
    variable b = 3.9 como entero      // 3: trunca
    imprimir(a)
    imprimir(b)
    retornar 0
}
```

Mezclar `entero` y `flotante` sin convertir es un error de tipo (`NOK4001`). Es a propósito: un lenguaje
que convierte solo donde le conviene deja de avisar donde importa.

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

---

## 4. Operadores

### Aritméticos

| Operador | Qué hace | Ejemplo |
|---|---|---|
| `+` | suma (y concatena textos) | `2 + 3` → `5` |
| `-` | resta | `5 - 2` → `3` |
| `*` | multiplicación | `4 * 3` → `12` |
| `/` | división entera | `7 / 2` → `3` |
| `%` | resto | `7 % 2` → `1` |
| `**` | potencia | `2 ** 10` → `1024` |

Con `flotante`, `/` da el resultado exacto:

```nooki
funcion principal() -> entero {
    imprimir(7 / 2)          // 3   (división entera)
    imprimir(7.0 / 2.0)      // 3,5
    imprimir(2 ** 10)        // 1024
    imprimir(7 % 2)          // 1
    retornar 0
}
```

**La aritmética entera está comprobada**: sumar hasta desbordar un `entero` no da la vuelta en
silencio, detiene el programa con un mensaje (`NOK5xxx` no: es un pánico en ejecución, código de salida
70). Dividir entre cero, también.

### Comparación

```nooki
funcion principal() -> entero {
    variable a = 5
    variable b = 3
    imprimir(a == b)     // falso
    imprimir(a != b)     // verdadero
    imprimir(a > b)      // verdadero
    imprimir(a >= b)     // verdadero
    imprimir(a < b)      // falso
    imprimir(a <= b)     // falso
    retornar 0
}
```

### Lógicos

```nooki
funcion principal() -> entero {
    variable hay_sol = verdadero
    variable hace_frio = falso
    imprimir(hay_sol && !hace_frio)    // verdadero: las dos se cumplen
    imprimir(hay_sol || hace_frio)     // verdadero: al menos una
    imprimir(!hay_sol)                 // falso
    retornar 0
}
```

`&&` y `||` **cortocircuitan**: si el primer lado decide, el segundo no se evalúa.

### Rangos

`..` excluye el final y `..=` lo incluye. Se usan en los bucles:

```nooki
funcion principal() -> entero {
    para (i en 0..3) {
        imprimir(i)      // 0, 1, 2
    }
    para (j en 1..=3) {
        imprimir(j)      // 1, 2, 3
    }
    retornar 0
}
```

### Precedencia

De mayor a menor: unarios (`!`, `-`), `**`, `* / %`, `+ -`, comparaciones, `&&`, `||`. Los paréntesis
mandan.

```nooki
funcion principal() -> entero {
    imprimir(2 + 3 * 4)        // 14, no 20
    imprimir((2 + 3) * 4)      // 20
    retornar 0
}
```

---

## 5. Textos

### Literales

Se escriben entre comillas dobles. Las comillas simples son para caracteres y **no están soportadas**
todavía.

```nooki
funcion principal() -> entero {
    variable simple = "hola"
    variable con_escapado = "una línea\ny otra"       // \n es un salto de línea
    variable con_comillas = "dijo \"hola\""
    variable con_tab = "a\tb"
    imprimir(simple)
    imprimir(con_escapado)
    imprimir(con_comillas)
    imprimir(con_tab)
    retornar 0
}
```

Escapes disponibles: `\n`, `\t`, `\\`, `\"`.

### Textos de varias líneas

Con tres comillas se escribe texto tal cual, **sin escapar nada**:

```nooki
funcion principal() -> entero {
    variable poema = """primera línea
segunda línea
    con sangría"""
    imprimir(poema)
    retornar 0
}
```

Todo lo que hay entre `"""` y `"""` es contenido, incluida la sangría. El formateador no lo toca.

### Concatenar y comparar

`+` une textos. `==`, `!=`, `<`, `>` comparan **por contenido**:

```nooki
funcion principal() -> entero {
    variable nombre = "Ana"
    variable saludo = "Hola, " + nombre
    imprimir(saludo)

    si (nombre == "Ana") {
        imprimir("es Ana")
    }
    si ("abeja" < "barco") {
        imprimir("va antes por orden alfabético")
    }
    retornar 0
}
```

Concatenar un `entero` no está permitido: `"" + 42` es un error de tipo. Para pasar números a texto hay
que esperar a que exista la conversión (está en la lista de pendientes).

### Métodos de texto

| Método | Devuelve | Qué hace |
|---|---|---|
| `largo()` | `entero` | número de bytes |
| `subtexto(desde, cuantos)` | `texto` | un trozo |
| `buscar(&texto)` | `entero` | posición, o `-1` si no está |
| `contiene(&texto)` | `booleano` | si aparece |
| `empieza_con(&texto)` | `booleano` | si empieza así |
| `mayusculas()` | `texto` | en mayúsculas |
| `minusculas()` | `texto` | en minúsculas |
| `recortar()` | `texto` | sin espacios en los extremos |
| `dividir(&separador)` | `Vector<texto>` | trozos |
| `caracter(i)` | `entero` | el byte en esa posición, o `-1` |
| `a_entero()` | `entero` | convierte un número escrito |
| `agregar_codigo(codigo)` | `texto` | añade un carácter (UTF-8) |
| `sin_ultimo()` | `texto` | quita el último carácter |

```nooki
funcion principal() -> entero {
    variable frase = "  Hola, Mundo  "
    imprimir(frase.largo())
    imprimir(frase.recortar())
    imprimir(frase.recortar().mayusculas())
    imprimir(frase.recortar().minusculas())

    variable limpia = frase.recortar()
    variable coma = ","
    imprimir(limpia.buscar(&coma))
    si (limpia.contiene(&coma)) {
        imprimir("tiene coma")
    }
    imprimir(limpia.subtexto(0, 4))

    variable numero = "42"
    imprimir(numero.a_entero() + 1)
    retornar 0
}
```

### Dividir en palabras

`dividir` devuelve un `Vector<texto>`:

```nooki
funcion principal() -> entero {
    variable frase = "uno dos tres"
    variable espacio = " "
    variable partes = frase.dividir(&espacio)

    variable i = 0
    mientras (i < partes.largo()) {
        imprimir(partes.obtener(i))
        i = i + 1
    }
    retornar 0
}
```

---

## 6. Decisiones

### `si` / `sino`

La condición tiene que ser un `booleano`. Un número no vale como condición:

```nooki
funcion principal() -> entero {
    variable edad = 18

    si (edad >= 18) {
        imprimir("mayor de edad")
    } sino {
        imprimir("menor")
    }

    // Encadenar es tan simple como anidar
    si (edad < 13) {
        imprimir("niño")
    } sino si (edad < 18) {
        imprimir("adolescente")
    } sino {
        imprimir("adulto")
    }
    retornar 0
}
```

### Decidir sobre varios valores: `coincide`

Se explica en el capítulo 13, con las enumeraciones, que es donde más se usa. Con números y textos
también funciona:

```nooki
funcion principal() -> entero {
    variable dia = 3

    coincide (dia) {
        caso 1 => imprimir("lunes")
        caso 2 => imprimir("martes")
        caso 3 => imprimir("miércoles")
        caso _ => imprimir("otro día")
    }
    retornar 0
}
```

`_` es el comodín: cubre lo que no hayan cubierto los demás casos. **Sin él, el `coincide` tiene que ser
exhaustivo** o no compila.

---

## 7. Bucles

### `mientras`

Repite mientras la condición sea verdadera:

```nooki
funcion principal() -> entero {
    variable n = 0
    mientras (n < 5) {
        imprimir(n)
        n = n + 1
    }
    retornar 0
}
```

### `para` con un rango

Recorre números. `..` excluye el final, `..=` lo incluye:

```nooki
funcion principal() -> entero {
    para (i en 0..3) {
        imprimir(i)          // 0, 1, 2
    }
    para (i en 1..=3) {
        imprimir(i)          // 1, 2, 3
    }
    retornar 0
}
```

### `para` con una colección

Recorre un `Vector`, elemento a elemento:

```nooki
funcion principal() -> entero {
    variable nombres = Vector<texto>.nuevo()
    nombres.agregar("Ana")
    nombres.agregar("Luis")

    para (nombre en nombres) {
        imprimir("Hola, " + nombre)
    }
    retornar 0
}
```

### Vectores de estructuras

Un vector puede guardar tus propias estructuras:

```nooki
estructura punto {
    publico x: entero
    publico y: entero
}

funcion principal() -> entero {
    variable puntos = Vector<punto>.nuevo()
    puntos.agregar(punto { x: 1, y: 2 })
    puntos.agregar(punto { x: 10, y: 20 })

    variable p = puntos.obtener(1)
    imprimir(p.x)

    variable suma = 0
    para (i, q en puntos) {
        suma = suma + q.x
    }
    imprimir(suma)
    retornar 0
}
```

El vector se queda con el valor que le das (como `Vector<texto>`), así que no lo uses después de
meterlo. La estructura puede tener campos de `texto`: el vector guarda su propia copia y la libera, así que
la variable de la que salió sigue siendo tuya:

```nooki
estructura ficha {
    publico nombre: texto
    publico edad: entero
}

funcion principal() -> entero {
    variable lista = Vector<ficha>.nuevo()
    variable ana = "Ana" + ""
    lista.agregar(ficha { nombre: ana, edad: 30 })

    variable primera = lista.obtener(0)
    imprimir(primera.nombre)   // Ana
    imprimir(ana)              // Ana, la original sigue viva
    lista.soltar()
    retornar 0
}
```

`obtener` devuelve una **vista**: el texto es del vector mientras no lo modifiques ni lo vacíes.

**Un aviso sobre el CSV**: los flotantes se escriben con coma (`3,5`), que es la convención del lenguaje
también al imprimir. En un CSV la coma separa campos, así que una columna con decimales escrita con
`textos.de_flotante` y guardada con `datos.guardar` **rompe la tabla**. Mientras eso no esté resuelto, los
números que vayan a un CSV se dejan salir de DuckDB (que usa punto), o se convierten en la consulta con
`CAST(x AS VARCHAR)`. Y eso tiene
una consecuencia que se paga cara: **una función que devuelve `v.obtener(i)` y cuya `v` es local devuelve
una vista a memoria que ya no existe**. Si el llamante la usa, lee basura.

```nooki
// Mal: el vector es local y muere al salir; la vista apunta a memoria liberada.
funcion nombre_malo(n: entero) -> texto {
    variable d = Vector<texto>.nuevo()
    d.agregar("Ana")
    retornar d.obtener(n)
}

// Bien: se copia el texto antes de devolverlo.
funcion nombre_bueno(n: entero) -> texto {
    variable d = Vector<texto>.nuevo()
    d.agregar("Ana")
    variable copia = d.obtener(n) + ""
    retornar copia
}
```

La regla, mientras no haya análisis de escape: **si un texto sale de una función y venía de un vector
local, se copia con `+ ""`**. Lo que
todavía no se admite es una estructura con una **colección o una caja** dentro (`Vector`, `Mapa`,
`Caja`): copiarlas en profundidad es otro trabajo, y el compilador lo dice con `NOK6003`.

### Con el índice

Si además del elemento hace falta la posición, se piden las dos:

```nooki
funcion principal() -> entero {
    variable nombres = Vector<texto>.nuevo()
    nombres.agregar("Ana")
    nombres.agregar("Luis")

    para (i, nombre en nombres) {
        imprimir(i)
        imprimir(nombre)
    }
    retornar 0
}
```

El índice empieza en 0 y es un `entero`.

### `hacer` ... `mientras`

Ejecuta el cuerpo al menos una vez:

```nooki
funcion principal() -> entero {
    variable n = 10
    hacer {
        imprimir(n)
        n = n + 1
    } mientras (n < 3)
    retornar 0
}
```

### `romper` y `continuar`

`romper` sale del bucle; `continuar` salta a la siguiente vuelta:

```nooki
funcion principal() -> entero {
    variable i = 0
    mientras (verdadero) {
        i = i + 1
        si (i == 2) {
            continuar          // salta esta vuelta
        }
        si (i > 4) {
            romper             // sale del bucle
        }
        imprimir(i)            // 1, 3, 4
    }
    retornar 0
}
```

---

## 8. Funciones

### Declarar y llamar

```nooki
funcion sumar(a: entero, b: entero) -> entero {
    retornar a + b
}

funcion saludar(nombre: texto) -> vacio {
    imprimir("Hola, " + nombre)
}

funcion principal() -> entero {
    imprimir(sumar(2, 3))
    saludar("Ana")
    retornar 0
}
```

`-> vacio` (o nada) significa que no devuelve valor.

### Parámetros prestados

Un parámetro con `&` **presta** el valor en lugar de moverlo: la función lo lee sin quedarse con él.
Es obligatorio para tipos que tienen dueño (`texto`, `Vector`, `Mapa`, clases).

```nooki
funcion longitud(frase: &texto) -> entero {
    retornar frase.largo()
}

funcion principal() -> entero {
    variable saludo = "hola mundo"
    imprimir(longitud(&saludo))
    imprimir(saludo)        // sigue siendo mío: solo se prestó
    retornar 0
}
```

Se llama con `&` delante del nombre. Esa es la diferencia con pasar por valor, que **mueve**.

### Parámetros mutables

`&mutable` presta para **leer y escribir**:

```nooki
funcion sumar_a(contador: &mutable Caja<entero>, cuanto: entero) -> vacio {
    contador.poner(contador.sacar() + cuanto)
}

funcion principal() -> entero {
    variable caja = Caja<entero>.nuevo(0)
    sumar_a(&mutable caja, 5)
    sumar_a(&mutable caja, 3)
    imprimir(caja.sacar())
    retornar 0
}
```

### Recursión

```nooki
funcion factorial(n: entero) -> entero {
    si (n <= 1) {
        retornar 1
    }
    retornar n * factorial(n - 1)
}

funcion principal() -> entero {
    imprimir(factorial(5))
    retornar 0
}
```

### Devolver varios valores

Una función devuelve **un** valor, pero ese valor puede ser una estructura o una variante:

```nooki
estructura division {
    publico cociente: entero
    publico resto: entero
}

funcion dividir(a: entero, b: entero) -> division {
    retornar division { cociente: a / b, resto: a % b }
}

funcion principal() -> entero {
    variable d = dividir(17, 5)
    imprimir(d.cociente)
    imprimir(d.resto)
    retornar 0
}
```

---

## 9. Colecciones

### `Vector`

Una lista que crece. `Vector<entero>` o `Vector<texto>`:

```nooki
funcion principal() -> entero {
    variable numeros = Vector<entero>.nuevo()
    numeros.agregar(10)
    numeros.agregar(20)
    numeros.agregar(30)

    imprimir(numeros.largo())
    imprimir(numeros.obtener(1))

    numeros.modificar(0, 99)
    imprimir(numeros.obtener(0))

    variable suma = 0
    para (n en numeros) {
        suma = suma + n
    }
    imprimir(suma)
    retornar 0
}
```

| Método | Qué hace |
|---|---|
| `agregar(valor)` | añade al final |
| `obtener(i)` | lee la posición |
| `modificar(i, valor)` | cambia una posición |
| `largo()` | cuántos hay |
| `vaciar()` | los quita todos |

### `Mapa`

Un diccionario de `texto` a `entero`:

```nooki
funcion principal() -> entero {
    variable edades = Mapa<texto, entero>.nuevo()

    variable ana = "Ana"
    variable luis = "Luis"
    edades.insertar(&ana, 30)
    edades.insertar(&luis, 25)

    imprimir(edades.largo())
    imprimir(edades.obtener(&ana))

    si (edades.contiene(&ana)) {
        imprimir("Ana está")
    }

    edades.quitar(&luis)
    imprimir(edades.largo())
    retornar 0
}
```

El mapa **copia la clave**: la variable que usaste sigue siendo tuya. El valor puede ser `entero`,
`flotante` o `texto`:

```nooki
funcion principal() -> entero {
    variable agenda = Mapa<texto, texto>.nuevo()
    variable ana = "Ana" + ""
    variable telefono = "555-1234" + ""
    agenda.insertar(&ana, telefono)
    imprimir(agenda.obtener(&ana))
    retornar 0
}
```

Con valores de texto, `obtener` devuelve una **vista**: el texto pertenece al mapa, así que no hay que
liberarlo ni se puede modificar por ahí.

### `Caja`

Un valor con dueño dentro de una variable prestable:

```nooki
funcion leer(caja: &Caja<entero>) -> entero {
    retornar caja.sacar()
}

funcion principal() -> entero {
    variable caja = Caja<entero>.nuevo(7)
    imprimir(leer(&caja))
    retornar 0
}
```

---

## 10. Estructuras

Agrupan datos y se pasan **por valor**: al asignarlas, se copian.

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

    imprimir(c.x)
    imprimir(c.y)

    // Cambiar la copia no toca el original
    c.x = 99
    imprimir(a.x)      // 1
    imprimir(c.x)      // 99
    retornar 0
}
```

Los campos se declaran con `publico` o `privado`:

```nooki
estructura cuenta {
    publico titular: texto
    privado saldo: entero
}

funcion principal() -> entero {
    variable c = cuenta { titular: "Ana", saldo: 100 }
    imprimir(c.titular)
    // imprimir(c.saldo)   // error: «saldo» es privado
    retornar 0
}
```

Una estructura **no puede heredar** ni tener métodos virtuales: para eso están las clases.

---

## 11. Clases y herencia

Una **clase** es como una estructura, pero se pasa **por referencia** (el valor vive en el montículo y
la variable es un puntero) y admite herencia y polimorfismo.

### Declarar y usar

```nooki
clase persona {
    publico nombre: texto
    publico edad: entero

    publico funcion saludar() -> texto {
        retornar "Hola, soy " + este.nombre
    }
}

funcion principal() -> entero {
    variable p = nuevo persona { nombre: "Ana", edad: 30 }
    imprimir(p.saludar())
    imprimir(p.edad)
    retornar 0
}
```

Dos diferencias con una `estructura`:

- Se crea con **`nuevo`**.
- Dentro de un método, **`este`** se refiere al objeto.

### Herencia

`clase hija hereda base`. La hija hereda campos y métodos, y puede **redefinir** los métodos:

```nooki
clase animal {
    publico nombre: texto

    publico funcion hablar() -> texto {
        retornar "..."
    }
}

clase perro hereda animal {
    publico funcion hablar() -> texto {
        retornar "Guau"
    }
}

clase gato hereda animal {
    publico funcion hablar() -> texto {
        retornar "Miau"
    }
}

funcion principal() -> entero {
    variable a = nuevo animal { nombre: "Bicho" }
    variable p = nuevo perro { nombre: "Toby" }
    variable g = nuevo gato { nombre: "Michi" }

    imprimir(a.hablar())
    imprimir(p.hablar())
    imprimir(g.hablar())
    retornar 0
}
```

### Polimorfismo

Una variable declarada con el tipo **base** puede apuntar a un objeto de la clase hija, y la llamada va
a la implementación de la hija:

```nooki
clase figura {
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

clase rectangulo hereda figura {
    publico ancho: entero
    publico alto: entero

    publico funcion area() -> entero {
        retornar este.ancho * este.alto
    }
}

funcion principal() -> entero {
    variable c = nuevo cuadrado { lado: 3 }
    variable r = nuevo rectangulo { ancho: 2, alto: 5 }

    variable f: figura = c
    imprimir(f.area())      // 9: la de cuadrado

    f = r
    imprimir(f.area())      // 10: la de rectangulo
    retornar 0
}
```

La llamada se resuelve en ejecución por la tabla de la clase: es **despacho dinámico** de verdad, no una
decisión del compilador.

### `super`

Para llamar a la implementación de la base:

```nooki
clase animal {
    publico nombre: texto

    publico funcion presentarse() -> texto {
        retornar "Soy un animal"
    }
}

clase perro hereda animal {
    publico funcion presentarse() -> texto {
        retornar super.presentarse() + " y ladro"
    }
}

funcion principal() -> entero {
    variable p = nuevo perro { nombre: "Toby" }
    imprimir(p.presentarse())
    retornar 0
}
```

### Clases y estructuras: cuándo usar cada una

| | `estructura` | `clase` |
|---|---|---|
| Paso de valores | por valor (copia) | por referencia |
| Creación | `punto { x: 1 }` | `nuevo punto { x: 1 }` |
| Herencia | no | sí |
| Métodos virtuales | no | sí |
| Cuándo usarla | datos pequeños, sin identidad | objetos con identidad y jerarquía |

---

## 12. Rasgos

Un **rasgo** es un contrato: una lista de métodos que un tipo se compromete a tener. Sirve para tratar
igual a tipos que **no comparten padre**.

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

- El compilador **verifica** que la clase tiene todos los métodos del rasgo, con la firma correcta. Si
  falta uno, no compila.
- Un parámetro `&Rasgo` acepta cualquier tipo que implemente ese rasgo.
- Una clase puede implementar **varios** rasgos: `clase cuadrado implementa Describible, Medible`.

### Herencia y rasgos a la vez

Una clase puede heredar y cumplir rasgos, en cualquier orden:

```nooki
rasgo Describible {
    funcion describir() -> texto
}

clase animal {
    publico nombre: texto
}

clase gato hereda animal implementa Describible {
    publico funcion describir() -> texto {
        retornar "gato " + este.nombre
    }
}

funcion presentar(d: &Describible) -> vacio {
    imprimir(d.describir())
}

funcion principal() -> entero {
    variable g = nuevo gato { nombre: "Michi" }
    presentar(&g)
    retornar 0
}
```

---

## 13. Enumeraciones y `coincide`

Una **enumeración** es un tipo cuyos valores son un conjunto cerrado de variantes, y cada variante puede
llevar datos.

```nooki
enumeracion forma {
    circulo(radio: entero)
    cuadrado(lado: entero)
    sin_forma
}

funcion area(f: forma) -> entero {
    coincide (f) {
        // Para devolver desde una rama hace falta un bloque
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

    imprimir(area(c))
    imprimir(area(q))
    imprimir(area(n))
    retornar 0
}
```

### `coincide` es exhaustivo

Si falta una variante, **no compila**. Es lo que evita el clásico «olvidé un caso».

```nooki
enumeracion color {
    rojo,
    verde,
}

funcion nombre(c: color) -> texto {
    coincide (c) {
        caso color.rojo => {
            retornar "rojo"
        }
        caso color.verde => {
            retornar "verde"
        }
    }
}

funcion principal() -> entero {
    variable c = color.rojo
    imprimir(nombre(c))
    retornar 0
}
```

El comodín `_` cubre el resto cuando no quieres enumerar todas.

### Con textos y números

```nooki
funcion principal() -> entero {
    variable dia = 2

    coincide (dia) {
        caso 1 => imprimir("lunes")
        caso 2 => imprimir("martes")
        caso _ => imprimir("otro")
    }

    variable nombre = "Ana"
    coincide (nombre) {
        caso "Ana" => imprimir("hola Ana")
        caso _ => imprimir("hola desconocido")
    }
    retornar 0
}
```

---

## 14. Errores como valores

Nooki **no tiene excepciones**. Una operación que puede fallar devuelve un `resultado`: o el valor, o el
motivo del fallo.

### `Ok` y `Error`

```nooki
usar resultado

funcion dividir(a: entero, b: entero) -> resultado {
    si (b == 0) {
        retornar Error("no se puede dividir entre cero")
    }
    retornar Ok(a / b)
}

funcion principal() -> entero {
    coincide (dividir(10, 2)) {
        caso Ok(v) => imprimir(v)
        caso Error(motivo) => imprimir(motivo)
    }

    coincide (dividir(10, 0)) {
        caso Ok(v) => imprimir(v)
        caso Error(motivo) => imprimir(motivo)
    }
    retornar 0
}
```

### El operador `?`

Propaga el error sin escribir el `coincide`: si el resultado es `Mal`, la función devuelve ese error tal
cual; si es `Bien`, sigue con el valor.

```nooki
usar resultado

funcion dividir(a: entero, b: entero) -> resultado {
    si (b == 0) {
        retornar Error("división entre cero")
    }
    retornar Ok(a / b)
}

funcion mitad(a: entero) -> resultado {
    variable resultado = dividir(a, 2)?
    retornar Ok(resultado)
}

funcion principal() -> entero {
    coincide (mitad(10)) {
        caso Ok(v) => imprimir(v)
        caso Error(motivo) => imprimir(motivo)
    }
    coincide (mitad(3)) {
        caso Ok(v) => imprimir(v)
        caso Error(motivo) => imprimir(motivo)
    }
    retornar 0
}
```

`?` es la forma corta de: «si esto falló, devuelve el fallo; si no, dame el valor».

### Por qué así

Sin excepciones, el compilador sabe exactamente dónde puede fallar algo mirando la firma. Y no hay
`try`/`catch` que se pueda olvidar.

---

## 15. Genéricos

Un tipo puede llevar un parámetro de tipo. Se instancia **explícitamente**, con el tipo entre ángulos:

```nooki
estructura par<T> {
    publico primero: T
    publico segundo: T
}

funcion principal() -> entero {
    variable p = par<entero> { primero: 1, segundo: 2 }
    imprimir(p.primero + p.segundo)

    variable q = par<texto> { primero: "a", segundo: "b" }
    imprimir(q.primero + q.segundo)
    retornar 0
}
```

Cada combinación de tipos genera su propia versión del tipo en C. No hay borrado de tipos ni coste en
ejecución.

Los genéricos de la biblioteca son `Vector<T>`, `Mapa<texto, entero>` y `Caja<T>`.

---

## 16. Módulos y proyectos

### Un archivo puede usar otro

`usar` importa un módulo. Un módulo es un archivo `.oki` que **no tiene `principal`**:

```text
geometria.oki
    publico funcion area_cuadrado(lado: entero) -> entero {
        retornar lado * lado
    }

principal.oki
    usar geometria

    funcion principal() -> entero {
        imprimir(geometria.area_cuadrado(4))
        retornar 0
    }
```

El primer archivo no tiene `principal`: es un módulo. El segundo lo importa y llama a su función
pública.

Un programa completo que usa la biblioteca estándar:

```nooki
usar matematicas

funcion principal() -> entero {
    imprimir(matematicas.maximo(3, 9))
    imprimir(matematicas.cuadrado(5))
    retornar 0
}
```

Para que la función se pueda usar desde fuera tiene que ser **`publico`**. Lo que no lleva `publico` es
privado del módulo.

### Dónde busca el compilador

1. Junto al archivo que hace el `usar`.
2. En una carpeta `biblioteca/` del proyecto.
3. Subiendo por las carpetas padre.
4. En las dependencias del `nooki.toml`.
5. En la biblioteca estándar, que va dentro del compilador.

### Un proyecto

```bash
$ nooki nuevo demo
$ cd demo
$ nooki construir
```

`nooki nuevo` crea:

```
demo/
├── nooki.toml          el manifiesto
├── principal.oki       el programa
├── biblioteca/         aquí van los módulos del proyecto
└── pruebas/            aquí van las pruebas
```

El manifiesto (explicado a fondo en [`docs/PROYECTO.md`](docs/PROYECTO.md)):

```text
[proyecto]
nombre = "demo"
version = "0.1.0"

[dependencias]
utilidades = { ruta = "../utilidades" }
```

### Pruebas del proyecto

`nooki probar` ejecuta los archivos de `pruebas/` y compara su salida con el `.esperado` de al lado:

```
pruebas/
├── suma.oki
└── suma.esperado
```

---

## 17. Memoria: propiedad y préstamos

Esta es la parte que hace a Nooki distinto. Se lee una vez y se entiende para siempre.

### La regla: cada valor tiene un dueño

Un valor con dueño —`texto`, `Vector`, `Mapa`, `Caja` y las clases— pertenece **a una sola variable** en
cada momento. Cuando lo pasas a otro sitio, **se mueve**: el original deja de valer.

```nooki
funcion principal() -> entero {
    variable original = "hola"
    variable copia = original          // se MUEVE: «original» queda vacío
    imprimir(copia)
    retornar 0
}
```

Usar `original` después de moverlo es un error de compilación (`NOK5001`):

```text
funcion principal() -> entero {
    variable original = "hola"
    variable copia = original
    imprimir(original)      // error[NOK5001]: el valor se movió antes
    retornar 0
}
```

Es el mismo mecanismo que un `std::move` de C++, pero sin escribirlo y comprobado por el compilador.

### Prestar en lugar de mover: `&`

Si solo quieres **leer**, presta:

```nooki
funcion longitud(frase: &texto) -> entero {
    retornar frase.largo()
}

funcion principal() -> entero {
    variable saludo = "hola"
    imprimir(longitud(&saludo))
    imprimir(saludo)        // sigue siendo mío
    retornar 0
}
```

### Prestar para escribir: `&mutable`

```nooki
funcion sumar_a(contador: &mutable Caja<entero>, cuanto: entero) -> vacio {
    contador.poner(contador.sacar() + cuanto)
}

funcion principal() -> entero {
    variable caja = Caja<entero>.nuevo(0)
    sumar_a(&mutable caja, 1)
    imprimir(caja.sacar())
    retornar 0
}
```

Dentro de la función, el parámetro prestado es un **puntero**: para usar el valor como tal se
desreferencia con `*`:

```nooki
funcion longitud(frase: &texto) -> entero {
    variable copia = *frase      // se copia el valor prestado
    retornar copia.largo()
}

funcion principal() -> entero {
    variable saludo = "hola"
    imprimir(longitud(&saludo))
    retornar 0
}
```

Un préstamo normal no deja modificar:

```text
funcion cambiar(frase: &texto) -> vacio {
    frase = "otra"      // error: no se puede escribir en un préstamo inmutable (NOK4009)
}
```

### Las reglas, en una tabla

| Regla | Qué evita | Código si se rompe |
|---|---|---|
| Un solo dueño | dos liberaciones del mismo valor | `NOK5004` |
| Pasar por valor mueve | copias caras invisibles | — |
| No se usa lo movido | leer memoria liberada | `NOK5001` |
| No se mueve mientras está prestado | referencias colgando | `NOK5007` |
| No se guarda un préstamo | préstamos que sobreviven al dueño | `NOK5002` |
| No escapa un préstamo local | devolver algo que muere | `NOK5006` |
| Un préstamo mutable, solo | escrituras simultáneas | `NOK5003` |

**Cuando el compilador no puede garantizar algo, lo rechaza.** No hay casos «ya veremos en ejecución».

### Qué hace el compilador por ti

- Libera los textos y colecciones al final del bloque, al salir de un bucle con `romper` y al volver de
  una función.
- Al asignar una colección, libera la que hubiera en el destino y mueve la nueva.
- Al asignar un texto, libera el búfer viejo.

No tienes que escribir `free` ni `liberar`. Y si te olvidas de algo, no compila.

---

## 18. Llamar a C

`externo funcion` declara una función de C. Nooki genera la llamada y deja que el enlazador la resuelva
contra la biblioteca del sistema.

```nooki
// La libc está enlazada siempre: strlen cuenta los bytes de una cadena.
externo funcion strlen(cadena: &texto) -> entero

funcion principal() -> entero {
    variable saludo = "hola"
    imprimir(strlen(&saludo))
    retornar 0
}
```

### Cómo se traducen los tipos

| Nooki | C |
|---|---|
| `entero` | `long long` |
| `flotante` | `double` |
| `booleano` | `int` |
| `texto` (ida) | `const char*` |
| `texto` (vuelta) | `const char*` — Nooki copia la cadena |
| `&texto` | `const char*` |
| `&entero` | `long long*` |
| `nulo` | `NULL` |

```nooki
// Una función de C que devuelve un texto: Nooki se queda con una copia.
externo funcion getenv(nombre: &texto) -> texto

funcion principal() -> entero {
    variable cual = "PATH"
    variable ruta = getenv(&cual)
    si (ruta.largo() > 0) {
        imprimir("hay PATH")
    }
    retornar 0
}
```

### Para la biblioteca estándar

Los módulos de `archivo` usan esta vía para leer y escribir archivos: `externo` a `fopen`/`fread`, y una
envoltura en Nooki que devuelve `Bien` o `Mal`.

---

## 19. `inseguro` y `asm!`

### `inseguro`

Dentro de un bloque `inseguro { }` se desactivan **las comprobaciones de la aritmética entera**. Nada
más: la propiedad y los tipos siguen comprobándose igual.

```nooki
funcion principal() -> entero {
    variable a = 1

    inseguro {
        // Aquí un desbordamiento no detiene el programa: da la vuelta.
        a = a + 1
    }

    imprimir(a)
    retornar 0
}
```

Sirve para código que sabe lo que hace con los números —un hash, un decodificador— y no quiere pagar la
comprobación en cada suma.

### `asm!`

### Una tabla en memoria

`datos.tabla(sql)` devuelve una **`Tabla`**: el resultado ya cargado, con el que se trabaja fila a fila en
lugar de leer texto.

```nooki
usar datos
usar textos

funcion principal() -> entero {
    variable consulta = "SELECT ciudad, SUM(unidades) AS u FROM read_csv('ventas.csv') GROUP BY ciudad" + ""
    variable t = datos.tabla(&consulta)

    si (!datos.hay(t)) {
        imprimir(datos.error())
        retornar 1
    }

    variable total = 0
    variable f = 0
    mientras (f < datos.filas(t)) {
        variable ciudad = datos.valor(t, f, 0)
        variable unidades = datos.entero_de(t, f, 1)
        imprimir(ciudad + ": " + textos.de_entero(unidades))
        total = total + unidades
        f = f + 1
    }
    imprimir(textos.de_entero(total))

    datos.liberar(t)
    retornar 0
}
```

Y se le pueden **añadir columnas calculadas**: se construye un `Vector<texto>` con un bucle —lo que en pandas
sería un `apply`— y se añade a la tabla.

```nooki
usar datos
usar textos

funcion principal() -> entero {
    variable consulta = "SELECT * FROM (VALUES ('Madrid', 10, 5), ('Lima', 7, 4)) AS v(ciudad, unidades, precio)" + ""
    variable t = datos.tabla(&consulta)

    variable importes = Vector<texto>.nuevo()
    variable f = 0
    mientras (f < datos.filas(t)) {
        variable unidades = datos.entero_de(t, f, 1)
        variable precio = datos.numero(t, f, 2)
        importes.agregar(textos.de_flotante((unidades como flotante) * precio) + "")
        f = f + 1
    }

    variable nombre = "importe" + ""
    si (datos.agregar_columna(t, &nombre, importes)) {
        imprimir(datos.a_texto(t))
    }
    datos.liberar(t)
    retornar 0
}
```

Y se puede pedir una columna **por su nombre**, que es lo más cómodo: `indice_columna(t, "unidades")` da su
posición (o `-1`), `hay_columna(t, "unidades")` dice si está, y `columna_por_nombre`, `valor_por_nombre`,
`numero_por_nombre` y `entero_por_nombre` leen directamente sin llevar el índice a mano. La tabla se guarda
con `guardar(t, &ruta)`.

Los accesos: `filas(t)`, `columnas(t)`, `nombre_columna(t, c)`, `valor(t, f, c)` (texto),
`numero(t, f, c)` y `entero_de(t, f, c)` (ya convertidos), `columna(t, c)` (un `Vector<texto>` con la
columna entera) y `a_texto(t)` (la tabla en CSV). Hay que **liberarla** con `datos.liberar(t)`: la tabla
vive en memoria del programa, no en la base de datos.

### Funciones como argumento

Una función se puede pasar a otra, siempre que su tipo encaje. Se escribe con `funcion(...) -> tipo`:

```nooki
usar textos

funcion es_par(n: entero) -> booleano {
    retornar n % 2 == 0
}

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

funcion principal() -> entero {
    variable numeros = Vector<entero>.nuevo()
    numeros.agregar(1)
    numeros.agregar(2)
    numeros.agregar(3)
    numeros.agregar(4)

    variable pares = filtrar(&numeros, es_par)
    variable k = 0
    mientras (k < pares.largo()) {
        imprimir(textos.de_entero(pares.obtener(k)))
        k = k + 1
    }
    retornar 0
}
```

Dentro de la función que la recibe, el parámetro se llama como cualquier función.

Y una función se puede **guardar en una variable**, escribiendo su tipo:

```nooki
funcion doblar(n: entero) -> entero {
    retornar n * 2
}

funcion sumar_uno(n: entero) -> entero {
    retornar n + 1
}

funcion principal() -> entero {
    variable f: funcion(entero) -> entero = doblar
    imprimir(f(21))

    f = sumar_uno
    imprimir(f(21))
    retornar 0
}
```

Y se puede escribir **ahí mismo**, sin declararla aparte, con una lambda:

```nooki
funcion aplicar(valor: entero, cambio: funcion(entero) -> entero) -> entero {
    retornar cambio(valor)
}

funcion principal() -> entero {
    imprimir(aplicar(21, funcion(n: entero) -> entero => n * 2))

    variable triple = funcion(n: entero) -> entero => n * 3
    imprimir(triple(5))
    retornar 0
}
```

La forma es `funcion(parámetros) -> tipo => expresión`, y el cuerpo es **una sola expresión**.

Si la lambda usa una variable de fuera, se la **lleva copiada**: eso es un **cierre**, y su tipo es
`cierre(A) -> R`.

```nooki
usar textos

funcion principal() -> entero {
    variable base = 10
    variable saludo = "hola " + ""

    variable suma = funcion(n: entero) -> entero => n + base
    variable saluda = funcion(nombre: texto) -> texto => saludo + nombre

    imprimir(textos.de_entero(suma(5)))   // 15
    imprimir(saluda("Ana"))               // hola Ana
    retornar 0
}
```

El cierre **es dueño de lo que capturó** y lo libera al salir del ámbito, igual que un `Vector`. Se copia
el valor en el momento de crear la lambda: si después cambias `base`, el cierre sigue con el valor viejo, y
dentro de un bucle cada vuelta captura lo suyo.

Un cierre se puede **devolver** desde una función —lo que da fábricas de callbacks— escribiendo su tipo:

```nooki
usar textos

funcion sumador(base: entero) -> cierre(entero) -> entero {
    retornar funcion(n: entero) -> entero => n + base
}

funcion principal() -> entero {
    variable mas_cinco: cierre(entero) -> entero = sumador(5)
    imprimir(textos.de_entero(mas_cinco(2)))   // 7
    retornar 0
}
```

Cada llamada a la fábrica da un entorno nuevo, así que dos sumadores no se pisan.

**Lo que todavía no se puede** es copiar un cierre o pasarlo a otra función: el entorno tiene un solo dueño, y
moverlo en esos dos caminos es el siguiente escalón. El compilador lo dice con `NOK6003` en vez de generar
código raro. Tampoco se pueden capturar `Vector`, `Mapa` ni `Caja`: se mueven, no se copian.

### Recibir argumentos

Un programa puede leer con qué argumentos se lanzó:

```nooki
usar sistema

funcion principal() -> entero {
    variable lista = sistema.argumentos()
    variable i = 0
    mientras (i < lista.largo()) {
        imprimir(lista.obtener(i))
        i = i + 1
    }
    si (sistema.cuantos_argumentos() > 0) {
        imprimir("el primero es " + sistema.argumento(0))
    }
    retornar 0
}
```

Con `nooki ejecutar programa.oki uno dos`, `argumentos()` devuelve los dos y el nombre del programa no
aparece. Lo que va detrás del fuente son argumentos del programa; las opciones del compilador empiezan por
guion y van antes.

### Análisis de datos con SQL

Para lo que en pandas sería un `groupby`, un `pivot_table`, un `merge` o un `rolling`, hay un módulo `datos`
con SQL y un motor de verdad detrás:

```nooki
usar datos

funcion principal() -> entero {
    variable consulta = "SELECT ciudad, SUM(unidades) AS total FROM read_csv('ventas.csv', delim=';') GROUP BY ciudad" + ""
    imprimir(datos.consultar(&consulta))
    retornar 0
}
```

`consultar` devuelve el resultado en CSV; `a_csv` y `a_parquet` lo guardan en un archivo.

La biblioteca de **DuckDB** viaja con la distribución (`duckdb.dll` junto al ejecutable, o
`libduckdb.so`), y el programa la carga la primera vez que la usa: si no analizas datos, no se abre. Si
prefieres la tuya, apunta `NOOKI_DUCKDB` a su ruta. Y si lo que necesitas es Python con pandas —un
`apply` con una función tuya, una gráfica—, usa `sistema` con el guion de Python: las dos cosas conviven.

### Hablar con otro programa

A veces la herramienta existe ya y no tiene sentido reescribirla: un guion de Python con pandas, un
programa del sistema, un conversor. El módulo `sistema` lanza la orden y recoge lo que responda:

```nooki
usar sistema

funcion principal() -> entero {
    variable listado = sistema.capturar("echo hola")
    imprimir(listado)

    variable codigo = sistema.ejecutar("python analisis.py datos.csv salida.csv")
    si (codigo != 0) {
        imprimir("el guion falló")
    }
    retornar 0
}
```

La orden es una **línea de comandos**: se ejecuta con el intérprete del sistema (`cmd` en Windows, `sh` en
Linux), y las comillas de las rutas con espacios son cosa de quien la escribe. `capturar` devuelve la salida
estándar; `ejecutar`, el código de salida.

### Punteros y memoria manual

Dentro de `inseguro` también se puede pedir memoria y tocarla. Es lo que hace falta para hablar con el
hardware o con una biblioteca de C:

```nooki
funcion principal() -> entero {
    variable total = 0
    inseguro {
        variable p = reservar(16)
        escribir_en(p, 0, 42)
        escribir_en(p, 8, 100)
        total = leer_en(p, 0) + leer_en(p, 8)
        liberar(p)
    }
    imprimir(total)
    retornar 0
}
```

`desde_direccion(n)` construye un puntero a una dirección concreta, que es como se leen y escriben los
registros de un dispositivo; `direccion_de(p)` la devuelve. **No hay comprobación de límites ni libertad
automática**: si te sales de lo reservado o liberas dos veces, el programa se rompe, y eso es lo que
significa `inseguro`.

Permite insertar ensamblador en línea. **Solo dentro de `inseguro`**; fuera es un error (`NOK3012`).

```nooki
funcion principal() -> entero {
    variable resultado = 0

    inseguro {
        asm!("nop")
    }

    imprimir(resultado)
    retornar 0
}
```

El texto se copia tal cual al C generado como `__asm__(...)`, así que funciona con GCC, Clang y MinGW, y
**no** con MSVC (que rechaza compilarlo).

---

## 20. La biblioteca estándar

Dieciséis módulos, todos escritos en Nooki. Se usan con `usar` y el nombre.

### Datos y textos

| Módulo | Contenido |
|---|---|
| `resultado` | `Ok(valor)`, `Error(mensaje)` |
| `matematicas` | `maximo(a, b)`, `minimo(a, b)`, `absoluto(n)`, `cuadrado(n)` |
| `textos` | `unir(&partes, separador)`, `repetir(&pieza, veces)`, `contar(&frase, &aguja)` |
| `listas` | `suma(&v)`, `mayor(&v)`, `menor(&v)`, `promedio(&v)`, `contiene(&v, valor)` |
| `fechas` | `bisiesto(a)`, `dias_del_mes(m, a)`, `dias_del_anio(a)`, `a_dias(a, m, d)`, `a_anio(dias)`, `a_mes(dias)`, `a_dia(dias)`, `sumar_dias(dias, n)`, `dia_de_la_semana(dias)` |
| `archivo` | `existe(&ruta)`, `leer(&ruta)`, `escribir(&ruta, &contenido)` |
| `pln` | `normalizar(&frase)`, `tokenizar(&frase)`, `contar(&palabras)`, `unicas(&palabras)`, `distintas(&palabras)`, `distancia(&a, &b)`, `jaccard(&a, &b)`, `contiene_texto(&palabras, &buscada)`, `cuantas(&palabras)` |

```nooki
usar matematicas
usar textos

funcion principal() -> entero {
    imprimir(matematicas.maximo(3, 9))
    imprimir(matematicas.absoluto(0 - 7))

    variable trozo = "ab"
    imprimir(textos.repetir(&trozo, 3))
    retornar 0
}
```

### Leer datos de un archivo CSV

```nooki
usar csv

funcion principal() -> entero {
    variable ruta = "notas.csv" + ""

    variable filas = csv.filas(&ruta)
    variable punto_coma = ";" + ""

    variable i = 1
    mientras (i < filas.largo()) {
        variable fila = filas.obtener(i)
        variable campos = csv.campos_con(&fila, &punto_coma)
        variable nombre = campos.obtener(0)
        variable nota = campos.obtener(2)
        imprimir(nombre)
        imprimir(csv.a_numero(&nota))
        i = i + 1
    }
    retornar 0
}
```

`csv.campos` separa por comas; `csv.campos_con` admite otro separador, que es lo que hace falta cuando los
números llevan coma decimal y el archivo usa `;`. La primera fila suele ser la cabecera: empieza en 1.

### Interfaz gráfica

| Módulo | Contenido |
|---|---|
| `ventana` | `abrir`, `viva`, `evento`, `evento_x/y/tecla/letra`, `es_*`, `limpiar`, `rectangulo`, `linea`, `escribir_texto`, `presentar`, `color_en`, `cerrar`, `pausa`, `repintar` |
| `diseno` | `columna`, `fila`, `centrar`, `repartir` |
| `controles` | `etiqueta`, `boton`, `barra`, `campo` |
| `accesibilidad` | `luminancia`, `legible`, `texto_sobre`, `foco` |
| `imagenes` | `cargar`, `cargada`, `ancho`, `alto`, `color_en`, `dibujar`, `soltar` |

### Inteligencia artificial

| Módulo | Contenido |
|---|---|
| `tensor` | `ceros`, `lleno`, `identidad`, `leer`, `poner`, `suma`, `resta`, `escalar`, `producto`, `transpuesta`, `determinante`, `media`, `varianza`, `normalizar`, `relu`, `sigmoides`, `softmax` |
| `tabla` | `crear`, `columna_entero`, `columna_texto`, `poner_*`, `valor_*`, `filtrar`, `ordenar`, `contar_por`, `suma`, `media`, `minimo`, `maximo` |
| `autograd` | `dato`, `valor`, `gradiente`, `suma`, `resta`, `producto`, `division`, `relu`, `sigmoides`, `atras`, `liberar` |
| `redes` | `nueva`, `ejemplo`, `ejemplos`, `entrenar`, `predecir`, `error`, `liberar` |

---

## 21. Interfaz gráfica

Una ventana con un botón y un campo de texto, que responde al ratón y al teclado:

```nooki
usar ventana
usar controles

funcion principal() -> entero {
    variable titulo = "Nooki"
    variable v = ventana.abrir(&titulo, 400, 300)

    si (v == 0) {
        imprimir("sin entorno gráfico")
        retornar 0
    }

    variable nombre = nuevo campo { x: 20, y: 20, ancho: 240, alto: 40,
                                    contenido: "", fondo: 0x313244, color: 0xF5E0DC,
                                    marcador: "Escribe tu nombre" }

    variable saludar = nuevo boton { x: 20, y: 80, ancho: 120, alto: 40,
                                     contenido: "Saludar", fondo: 0x89B4FA, color: 0x11111B }

    variable dibujar = verdadero
    mientras (ventana.viva(v)) {
        si (dibujar || ventana.repintar(v)) {
            ventana.limpiar(v, 0x1E1E2E)
            nombre.dibujar(v)
            saludar.dibujar(v)
            ventana.presentar(v)
            dibujar = falso
        }

        variable e = ventana.evento(v)
        mientras (e != 0) {
            si (ventana.es_letra(e)) {
                nombre.escribir(ventana.evento_letra(v))
                dibujar = verdadero
            }
            si (ventana.es_pulsacion(e)) {
                si (saludar.pulsado(ventana.evento_x(v), ventana.evento_y(v))) {
                    imprimir("Hola, " + nombre.contenido)
                }
                dibujar = verdadero
            }
            si (ventana.es_cerrar(e)) {
                romper
            }
            e = ventana.evento(v)
        }
        ventana.pausa(16)
    }

    ventana.cerrar(v)
    retornar 0
}
```

Las cinco trampas —repintar, `presentar`, `abrir` devuelve 0, `libx11-dev` y las palabras reservadas—
están explicadas en [`docs/INTERFAZ.md`](docs/INTERFAZ.md).

---

## 22. Inteligencia artificial

### Tensores

```nooki
usar tensor

funcion principal() -> entero {
    variable a = tensor.lleno(2, 2, 1)
    tensor.poner(a, 0, 0, 2)

    imprimir(tensor.filas(a))
    imprimir(tensor.leer(a, 0, 0))
    imprimir(tensor.suma_total(a))
    imprimir(tensor.determinante(a))
    tensor.liberar(a)
    retornar 0
}
```

### Una red que aprende

```nooki
usar redes

funcion principal() -> entero {
    variable red = redes.nueva(2, 4)

    redes.ejemplo(red, 0.0, 0.0, 0.0, 0.0, 0.0)
    redes.ejemplo(red, 0.0, 1.0, 0.0, 0.0, 1.0)
    redes.ejemplo(red, 1.0, 0.0, 0.0, 0.0, 1.0)
    redes.ejemplo(red, 1.0, 1.0, 0.0, 0.0, 0.0)

    redes.entrenar(red, 3000, 1.0)

    imprimir(redes.predecir(red, 1.0, 0.0, 0.0, 0.0))
    redes.liberar(red)
    retornar 0
}
```

Eso aprende XOR. Los detalles de cada módulo están en la referencia.

### Dos capas ocultas

Cuando una capa no basta —problemas que necesitan combinar rasgos en dos niveles— hay otra red, con dos:

```nooki
usar redes

funcion principal() -> entero {
    variable red = redes.nueva_profunda(2, 12, 3)

    variable datos = Vector<flotante>.nuevo()
    datos.agregar(0.5)
    datos.agregar(0.5)
    redes.ejemplo_profunda(red, &datos, 2)

    redes.entrenar_profunda(red, 10000, 0.3)
    imprimir(redes.predecir_profunda(red, &datos))
    redes.liberar_profunda(red)
    retornar 0
}
```

La tasa de aprendizaje importa más que en la de una capa: con `1.0` se queda atascada en la clase
mayoritaria y con `0.3` aprende.

### Guardar lo aprendido

Entrenar cuesta tiempo; el resultado se puede guardar y recuperar:

```nooki
usar redes

funcion principal() -> entero {
    variable archivo = "modelo.red" + ""

    variable red = redes.cargar(&archivo)
    si (red == 0) {
        // La primera vez: se crea y se entrena.
        red = redes.nueva(2, 4)
        redes.ejemplo(red, 0.0, 0.0, 0.0, 0.0, 0.0)
        redes.ejemplo(red, 0.0, 1.0, 0.0, 0.0, 1.0)
        redes.ejemplo(red, 1.0, 0.0, 0.0, 0.0, 1.0)
        redes.ejemplo(red, 1.0, 1.0, 0.0, 0.0, 0.0)
        redes.entrenar(red, 1500, 1.0)
        si (!redes.guardar(red, &archivo)) {
            imprimir("no se pudo guardar")
        }
    }

    imprimir(redes.predecir(red, 1.0, 0.0, 0.0, 0.0))
    redes.liberar(red)
    retornar 0
}
```

La segunda vez que se ejecuta, la red ya viene entrenada del archivo. Con los pesos viajan también los
ejemplos, así que se puede **seguir entrenando** donde se dejó.

---

## 23. Las herramientas

| Comando | Qué hace |
|---|---|
| `nooki nuevo <nombre>` | crea un proyecto |
| `nooki construir` | compila el proyecto actual |
| `nooki compilar <archivo>` | compila un archivo (`-o` para el nombre) |
| `nooki ejecutar <archivo>` | compila y ejecuta |
| `nooki probar` | ejecuta las pruebas del proyecto |
| `nooki verificar <archivo>` | comprueba sin generar C |
| `nooki formatear <archivo>` | sangría canónica (`--comprobar` para la CI) |
| `nooki documentar` | genera `API.md` con lo público |
| `nooki lsp` | servidor de lenguaje para el editor |
| `nooki ast <archivo>` | muestra el árbol de sintaxis |
| `nooki limpiar` | borra artefactos |
| `nooki ayuda` | lista los comandos |
| `nooki version` | la versión |

Códigos de salida: **0** bien, **64** uso incorrecto, **65** error en el código, **69** no disponible, **70**
pánico en ejecución.

---

## 24. Errores frecuentes

| Código | Qué significa | Cómo se arregla |
|---|---|---|
| `NOK2002` | el analizador esperaba otra cosa | mirar el símbolo marcado |
| `NOK2004` | construcción no soportada todavía | evitar esa forma |
| `NOK3001` | nombre no declarado | declararlo antes, o revisar la tilde |
| `NOK3003` | tipo desconocido | revisar el nombre del tipo |
| `NOK3008` | módulo no encontrado | revisar la ruta o el `nooki.toml` |
| `NOK3011` | falta un método del rasgo | implementarlo con la firma exacta |
| `NOK4001` | tipo incompatible | convertir con `como` o cambiar el valor |
| `NOK4005` | la condición de `si` no es booleana | comparar: `si (n != 0)` |
| `NOK4006` | `coincide` sin cubrir todos los casos | añadir la variante que falta o `caso _` |
| `NOK4008` | se intenta prestar algo que no se puede | prestar una variable, no una expresión |
| `NOK4012` | firma distinta a la del rasgo | copiar la firma del rasgo |
| `NOK5001` | usar un valor movido | prestar con `&` en vez de mover |
| `NOK5002` | guardar un préstamo en una variable | pasarlo directo como argumento |
| `NOK5004` | copiar algo con dueño | prestar, o mover |
| `NOK5006` | devolver un préstamo local | devolver por valor |
| `NOK5007` | mover mientras está prestado | terminar el préstamo antes |
| `NOK6003` | el backend no traduce algo | evitar esa construcción |
| `NOK8001` | *(retirado)* | — |

### Las tres causas más comunes

**1. Las palabras reservadas.** `texto`, `clase`, `esperar`, `vacio`, `interna`, `lista`, `tamano`… no
pueden ser nombres de variable. El error es `NOK2002`.

```nooki
funcion principal() -> entero {
    // variable clase = 1     // error: «clase» es palabra reservada
    variable categoria = 1
    imprimir(categoria)
    retornar 0
}
```

**2. Olvidar prestar.** Pasar un `texto` mueve; si lo usas después, `NOK5001`.

```nooki
funcion saludar(nombre: &texto) -> vacio {
    // Para usar el texto prestado como valor se desreferencia con «*»
    imprimir("Hola, " + *nombre)
}

funcion principal() -> entero {
    variable quien = "Ana"
    saludar(&quien)          // prestado: sigue siendo mío
    imprimir(quien)
    retornar 0
}
```

**3. Comparar con `=` en vez de `==`.** `=` asigna; la condición se comprueba con `==`.

---

## 25. Apéndice: palabras reservadas

Estas palabras **no** pueden usarse como nombres:

```text
variable  constante  funcion  clase  estructura  rasgo  interfaz
enumeracion  tipo  modulo  importar  exportar  implementar  externo  usar

si  sino  mientras  para  en  hacer  romper  continuar  retornar
coincide  caso

nuevo  este  Este  super  hereda  implementa
constructor  destructor  propiedad  obtener  establecer

publico  privado  protegido  interna  mutable  estatico  abstracta

inseguro  asm

como

verdadero  falso  nulo

entero  entero8  entero16  entero32  entero64  entero128
natural  natural8  natural16  natural32  natural64  natural128
flotante  flotante32  booleano  texto  caracter  vacio  byte
tamano  tamano_signado

# Reservadas pero sin uso todavía: asincrono atrapar bucle donde es esperar
# finalmente intentar lanzar macro mixin paso union sinosi
```

Son **85**. La lista exacta, agrupada y generada desde el compilador, está en
[`docs/referencia/palabras.md`](docs/referencia/palabras.md); y la tabla completa de los 46
diagnósticos, en [`docs/referencia/diagnosticos.md`](docs/referencia/diagnosticos.md).

Los **identificadores sí pueden llevar tildes**: `año`, `número`, `más`. Lo que no puede es tener una
palabra reservada con tilde donde el lenguaje espera una sin ella —`variable` sin tilde es la palabra;
`variáble` no existe—.

---

## Y ahora qué

- La **[guía](GUIA.md)** enseña lo mismo con más calma, capítulo a capítulo.
- La **[especificación](spec/especificacion-v0.1.md)** es el documento normativo, si necesitas el detalle
  exacto de algo.
- Los **[ejemplos](ejemplos/)** son programas completos, del «hola mundo» a una lista de tareas.
- Si algo no funciona como dice este manual, es un fallo: los ejemplos de aquí se compilan en cada
  `ctest`.
