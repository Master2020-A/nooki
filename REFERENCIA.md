# Referencia del lenguaje Nooki

Documento normativo de consulta: qué existe, cómo se escribe y qué no. Para aprender el lenguaje con
ejemplos, el [manual](MANUAL.md). Para el porqué de cada decisión, [DECISIONES.md](DECISIONES.md).

> **Verificada contra el compilador.** Las tablas de palabras reservadas, diagnósticos y biblioteca se
> generan de la fuente (`herramientas/generar-referencia.py`), y las afirmaciones sobre qué funciona se
> comprobaron compilando y ejecutando. Si algo de aquí no compila, es un fallo del compilador, no de la
> documentación.

## Índice

1. [Léxico](#1-léxico)
2. [Tipos](#2-tipos)
3. [Operadores](#3-operadores)
4. [Sentencias](#4-sentencias)
5. [Declaraciones](#5-declaraciones)
6. [Miembros de los tipos de la biblioteca](#6-miembros-de-los-tipos-de-la-biblioteca)
7. [Biblioteca estándar](#7-biblioteca-estándar)
8. [Propiedad y préstamos](#8-propiedad-y-préstamos)
9. [Módulos y visibilidad](#9-módulos-y-visibilidad)
10. [Diagnósticos](#10-diagnósticos)
11. [Herramientas](#11-herramientas)
12. [Lo que no existe](#12-lo-que-no-existe)

---

## 1. Léxico

### Archivos

Extensión **`.oki`**, codificación **UTF-8 sin BOM** (un BOM es `NOK1004`). Las líneas terminan en `\n`,
`\r\n` o `\r`.

### Identificadores

Empiezan por una letra (incluidas `á é í ó ú ü ñ` y sus mayúsculas) o `_`, y continúan con letras,
dígitos o `_`. **No pueden ser una palabra reservada.**

```text
valido      nombre_pila   año   número   _interno   árbol2
no valido   2nombre       texto (reservada)   clase (reservada)
```

Las palabras reservadas se escriben **sin tilde**. Escribir `variáble` no es una palabra clave: es un
identificador.

### Comentarios

| Forma | Alcance |
|---|---|
| `// hasta el final de la línea` | una línea |
| `/* ... */` | varias líneas, sin anidar |

### Literales

| Literal | Ejemplos | Notas |
|---|---|---|
| `entero` | `0` `42` `-7` `1_000` `0x1F` | 64 bits; los `_` son separadores |
| `flotante` | `3.14` `-0.5` `2.0` | doble precisión; el punto es obligatorio |
| `texto` | `"hola"` `"a\nb"` `"""..."""` | ver escapes |
| `booleano` | `verdadero` `falso` | |
| `nulo` | `nulo` | valor ausente |

Escapes admitidos dentro de `"..."`: `\n`, `\t`, `\\`, `\"`. Un `"""` abre un texto multilínea que
termina en el siguiente `"""` y **no interpreta escapes**.

No existen: literales de carácter entre comillas simples, cadenas sin procesar, números complejos,
literales binarios (`0b`) ni octales.

### Palabras reservadas

<!-- GENERADO:palabras -->
Ver [`docs/referencia/palabras.md`](docs/referencia/palabras.md), que el script regenera desde
`lexer.cpp` (85 palabras agrupadas).
<!-- FIN -->

---

## 2. Tipos

### Implementados

| Tipo | En C | Tamaño | Literales |
|---|---|---|---|
| `entero` | `long long` | 64 bits con signo | `42`, `0x1F`, `1_000` |
| `flotante` | `double` | 64 bits IEEE 754 | `3.14` |
| `booleano` | `int` (0 o 1) | — | `verdadero`, `falso` |
| `texto` | estructura `{char* datos; long long largo; int propio;}` | — | `"..."` |
| `vacio` | `void` | — | solo como tipo de retorno |

Las colecciones y los tipos genéricos están en [§6](#6-miembros-de-los-tipos-de-la-biblioteca).

### Aritmética

`entero` usa **aritmética comprobada**: sumar, restar, multiplicar, dividir o sacar el resto comprueba
el desbordamiento y la división por cero, y detiene el programa con un mensaje y **código de salida 70**.
Dentro de `inseguro { }` se usan los operadores de C sin comprobar.

`flotante` sigue IEEE 754: no hay comprobación de desbordamiento.

### Conversiones

Solo existen las explícitas, con `como`:

```text
expresion como entero      // trunca hacia cero
expresion como flotante
```

**No hay conversión implícita** entre `entero` y `flotante` (`NOK4001`). `"" + 42` tampoco: un `texto`
solo se concatena con `texto`.

### Tipos reservados sin implementar

Estas palabras son reservadas, pero **no son tipos válidos**: usarlas en una anotación da `NOK3003`.

```text
entero8  entero16  entero32  entero64  entero128
natural  natural8  natural16  natural32  natural64  natural128
flotante32  caracter  byte  tamano  tamano_signado  lista  esperar
```

`lista`, `esperar`, `caracter` y `byte` vienen del plan original del lenguaje y aún no tienen
implementación. `entero` ya es de 64 bits, así que `entero64` no aporta nada.

---

## 3. Operadores

### Precedencia y asociatividad

De menor a mayor potencia de enlace. La columna «¿traduce?» dice si el backend de C los implementa.

| Potencia | Operadores | Asociatividad | ¿Traduce? |
|---|---|---|---|
| 10 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | derecha | sí |
| 30 | `??` | derecha | **no** (`NOK6003`) |
| 40 | `\|\|` | izquierda | sí |
| 50 | `&&` | izquierda | sí |
| 60 | `==` `!=` `<` `>` `<=` `>=` | izquierda | sí |
| 70 | `\|` | izquierda | sí |
| 75 | `^` | izquierda | sí |
| 80 | `&` | izquierda | sí |
| 90 | `<<` `>>` | izquierda | sí |
| 100 | `..` `..=` | izquierda | solo en `para` |
| 110 | `+` `-` | izquierda | sí |
| 120 | `*` `/` `%` | izquierda | sí |
| 130 | `**` | derecha | sí |
| 135 | `como` | izquierda | sí |
| 140 | `-` `!` `&` `*` (unarios) | — | sí |

### Semántica por tipo

| Operador | `entero` | `flotante` | `texto` | `booleano` |
|---|---|---|---|---|
| `+` | suma comprobada | suma | **concatenación** | — |
| `-` `*` `/` `%` | comprobadas | IEEE 754 | — | — |
| `**` | potencia entera | `pow` | — | — |
| `==` `!=` | igualdad | igualdad | comparación por contenido | igualdad |
| `<` `>` `<=` `>=` | orden | orden | orden lexicográfico por bytes | — |
| `&&` `\|\|` | — | — | — | cortocircuito |
| `!` | — | — | — | negación |
| `&` `\|` `^` `<<` `>>` | bit a bit | — | — | — |
| `&` `*` (unarios) | prestar / desreferenciar | — | prestar / copiar | — |

Notas:

- `<<` y `>>` son los de C: desplazar 64 bits o más es comportamiento indefinido, no un error.
- `??` (coalescencia de nulo) está en el parser pero el backend todavía no lo traduce: `NOK6003`.
- `a &= b` funciona aunque `a & b` en una expresión... también funciona desde la versión actual: los dos
  caminos están implementados.

---

## 4. Sentencias

Cada sentencia ocupa su línea. No hay `;` al final.

| Sentencia | Sintaxis |
|---|---|
| Declaración | `variable nombre: tipo = expresion` · `constante nombre: tipo = expresion` (el tipo y el valor se pueden omitir, no los dos) |
| Asignación | `destino = expresion` · `destino += expresion` (y `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=`) |
| Condicional | `si (condicion) { ... } sino { ... }` — el `sino` es opcional; no hay `sino si` |
| Bucle mientras | `mientras (condicion) { ... }` |
| Bucle hacer | `hacer { ... } mientras (condicion)` |
| Bucle para | `para (nombre en inicio..fin) { ... }` · `para (nombre en inicio..=fin)` · `para (nombre en coleccion)` |
| Salida de bucle | `romper` · `continuar` |
| Retorno | `retornar expresion` · `retornar` |
| Coincidencia | `coincide (expresion) { caso patron => { ... } ... }` |
| Impresión | `imprimir(expresion)` — acepta `entero`, `flotante`, `booleano` y `texto` |
| Bloque sin comprobar | `inseguro { ... }` |
| Ensamblador | `asm!("texto")` — solo dentro de `inseguro` |
| Expresión | `llamada(...)` |

### Condiciones

Tienen que ser `booleano`. Un `entero` no vale como condición (`NOK4005`): se escribe `si (n != 0)`.

### `coincide`

| Patrón | Ejemplo | Cuándo |
|---|---|---|
| Variante con datos | `caso circulo(r) =>` | liga `r` |
| Variante sin datos | `caso forma.sin_forma =>` | nombre cualificado |
| Literal | `caso 1 =>` · `caso "ana" =>` · `caso verdadero =>` | |
| Comodín | `caso _ =>` | cubre el resto |

Un `coincide` sobre una enumeración debe cubrir **todas** las variantes o tener `caso _` (`NOK4006`).

---

## 5. Declaraciones

### Funciones

```text
funcion nombre(parametro: tipo, otro: &tipo, mutable: &mutable tipo) -> tipo_retorno { ... }
```

- Sin `->` o con `-> vacio`, no devuelve valor.
- Los parámetros con `&` **prestan**; con `&mutable`, prestan para escribir.
- Un `texto`, `Vector`, `Mapa`, clase o `Caja` **por valor se mueve**: el llamante deja de poder usarlo.
- `funcion principal() -> entero` es el punto de entrada; su retorno es el código de salida.

### Estructuras

```text
estructura nombre {
    publico campo: tipo
    privado otro: tipo
}
```

Se pasan **por valor**. No heredan ni tienen métodos.

### Clases

```text
clase nombre hereda base implementa Rasgo, Otro {
    publico campo: tipo
    publico funcion metodo() -> tipo { ... }
}
```

- Se crean con `nuevo`: `nuevo nombre { campo: valor }`.
- Se pasan **por referencia**.
- Dentro de un método, `este` es el objeto y `super` la base.
- Se puede heredar de **una** clase y cumplir **varios** rasgos. `hereda` e `implementa` valen en
  cualquier orden.

### Rasgos

```text
rasgo Nombre {
    funcion metodo() -> tipo
}
```

Un contrato: la clase que lo declara debe tener esos métodos con la firma exacta (`NOK3011`,
`NOK4012`). Se usa como tipo de parámetro: `&Nombre`.

### Enumeraciones

```text
enumeracion nombre {
    variante(dato: tipo)
    otra_sin_datos
}
```

- Las variantes **con datos** se usan sin cualificar: `nombre(1)`, `caso nombre(x)`.
- Las variantes **sin datos** se cualifican: `nombre.otra_sin_datos`, `caso nombre.otra_sin_datos`.

### Tipos genéricos

```text
estructura par<T> {
    publico primero: T
}
```

Se instancian explícitamente: `par<entero>`. Cada combinación genera su propio tipo en C.

### Funciones externas

```text
externo funcion strlen(cadena: &texto) -> entero
```

Declara una función de C; el enlazador la resuelve. `texto` se pasa como `const char*` y una función
que devuelve `texto` se copia al llegar.

### Importar módulos

```text
usar nombre_del_modulo
```

Ver [§9](#9-módulos-y-visibilidad).

---

## 6. Miembros de los tipos de la biblioteca

### `texto`

| Miembro | Firma | Qué hace |
|---|---|---|
| `largo` | `() -> entero` | número de bytes |
| `subtexto` | `(desde: entero, cuantos: entero) -> texto` | trozo |
| `buscar` | `(&aguja: texto) -> entero` | posición o `-1` |
| `contiene` | `(&aguja: texto) -> booleano` | |
| `empieza_con` | `(&prefijo: texto) -> booleano` | |
| `mayusculas` / `minusculas` | `() -> texto` | solo ASCII |
| `recortar` | `() -> texto` | quita espacios de los extremos |
| `dividir` | `(&separador: texto) -> Vector<texto>` | |
| `caracter` | `(i: entero) -> entero` | byte en esa posición, o `-1` |
| `a_entero` | `() -> entero` | convierte un número escrito |
| `agregar_codigo` | `(codigo: entero) -> texto` | añade un carácter UTF-8 |
| `sin_ultimo` | `() -> texto` | quita el último carácter |

### `Vector<T>`

| Miembro | Firma | Qué hace |
|---|---|---|
| `nuevo` | `Vector<T>.nuevo() -> Vector<T>` | vector vacío |
| `agregar` | `(valor: T) -> vacio` | añade al final |
| `obtener` | `(i: entero) -> T` | lee; fuera de rango devuelve el valor cero |
| `modificar` | `(i: entero, valor: T) -> vacio` | escribe; fuera de rango no hace nada |
| `largo` | `() -> entero` | cuántos hay |
| `vaciar` | `() -> vacio` | los quita todos (libera los textos) |

### `Mapa<texto, entero>`

| Miembro | Firma | Qué hace |
|---|---|---|
| `nuevo` | `Mapa<texto, entero>.nuevo() -> Mapa` | mapa vacío |
| `insertar` | `(clave: texto, valor: entero) -> vacio` | **copia** la clave; si existía, actualiza |
| `obtener` | `(clave: texto) -> entero` | valor, o `0` si no está |
| `contiene` | `(clave: texto) -> booleano` | |
| `quitar` | `(clave: texto) -> entero` | `1` si estaba, `0` si no |
| `largo` | `() -> entero` | cuántas claves |
| `claves` | `() -> Vector<texto>` | las claves, para recorrerlas |

### `redes`

| Miembro | Firma | Qué hace |
|---|---|---|
| `nueva` | `(entradas: entero, ocultas: entero) -> entero` | red nueva; entradas 1..4, ocultas 1..64 |
| `ejemplo` | `(red, x1, x2, x3, x4, esperado: flotante) -> vacio` | añade una muestra |
| `ejemplos` | `(red: entero) -> entero` | cuántas muestras tiene |
| `entrenar` | `(red, vueltas: entero, tasa: flotante) -> vacio` | descenso de gradiente |
| `predecir` | `(red, x1, x2, x3, x4: flotante) -> flotante` | salida entre 0 y 1 |
| `error` | `(red: entero) -> flotante` | error cuadrático medio |
| `guardar` | `(red: entero, ruta: &texto) -> booleano` | pesos **y ejemplos** a un archivo |
| `cargar` | `(ruta: &texto) -> entero` | devuelve la red, o `0` si el archivo no sirve |
| `liberar` | `(red: entero) -> vacio` | libera la memoria |

El archivo de `guardar` es binario y lleva una cabecera `NOOKIRED`: una versión, la forma de la red,
los pesos, los sesgos y las muestras. Cargar un archivo que no es de Nooki devuelve `0`.

**Varias clases.** Una segunda red, con una salida por clase (softmax) y hasta 16 clases:

| Miembro | Firma | Qué hace |
|---|---|---|
| `nueva_clases` | `(entradas: entero, ocultas: entero, clases: entero) -> entero` | red de 2 a 16 clases |
| `ejemplo_clase` | `(red, x1, x2, x3, x4: flotante, cual: entero) -> vacio` | añade una muestra con su clase (desde 0) |
| `entrenar_clases` | `(red, vueltas: entero, tasa: flotante) -> vacio` | entropía cruzada con softmax |
| `predecir_clase` | `(red, x1..x4: flotante) -> entero` | la clase más probable, o `-1` si la red no vale |
| `probabilidad` | `(red, x1..x4: flotante, cual: entero) -> flotante` | probabilidad de esa clase, entre 0 y 1 |
| `aciertos_clase` | `(red: entero) -> entero` | cuántas muestras acierta |
| `total_clase` | `(red: entero) -> entero` | cuántas muestras tiene |
| `liberar_clase` | `(red: entero) -> vacio` | libera la memoria |

Las dos redes son independientes: `liberar` para la binaria, `liberar_clase` para la de clases, y cada
una rechaza el identificador de la otra. La de clases **no** se guarda todavía en archivo.

### `Caja<T>`

| Miembro | Firma | Qué hace |
|---|---|---|
| `nuevo` | `Caja<entero>.nuevo(valor) -> Caja<entero>` | caja con un valor |
| `sacar` | `() -> T` | lee el valor |
| `poner` | `(valor: T) -> vacio` | lo cambia |

### Ejemplo: los miembros, ejecutados

```nooki
funcion principal() -> entero {
    variable t = "Hola Mundo" + ""
    imprimir(t.largo())
    imprimir(t.mayusculas())
    imprimir(t.caracter(0))
    imprimir(t.caracter(999))

    variable o = "o" + ""
    imprimir(t.buscar(&o))
    variable m = "Mundo" + ""
    si (t.contiene(&m)) {
        imprimir("contiene")
    }
    imprimir(t.sin_ultimo())

    variable n = "42" + ""
    imprimir(n.a_entero())

    variable v = Vector<entero>.nuevo()
    v.agregar(4)
    v.modificar(0, 9)
    imprimir(v.obtener(0))
    imprimir(v.obtener(99))
    v.vaciar()
    imprimir(v.largo())

    variable caja = Caja<entero>.nuevo(1)
    caja.poner(caja.sacar() + 6)
    imprimir(caja.sacar())
    retornar 0
}
```

### `resultado`

```text
enumeracion resultado {
    Ok(valor: entero)
    Error(mensaje: texto)
}
```

Se usa con `?` para propagar el error y con `coincide` para tratarlo.

---

## 7. Biblioteca estándar

<!-- GENERADO:biblioteca -->
**16 módulos** con **152 funciones públicas**, todas escritas en Nooki. La lista completa de firmas está
en [`docs/referencia/biblioteca.md`](docs/referencia/biblioteca.md), generada desde la fuente que va
dentro del compilador.
<!-- FIN -->

| Módulo | Para qué |
|---|---|
| `resultado` | errores como valores |
| `matematicas` | máximo, mínimo, absoluto, cuadrado |
| `textos` | unir, repetir, contar |
| `listas` | suma, mayor, menor, promedio, contiene |
| `fechas` | calendario y aritmética de días |
| `archivo` | leer, escribir, existe |
| `pln` | normalizar, tokenizar, contar, distancias |
| `ventana` | abrir ventanas, eventos, dibujo |
| `diseno` | columnas, filas, centrado, reparto |
| `controles` | etiqueta, botón, barra, campo de texto |
| `accesibilidad` | contraste y foco |
| `imagenes` | BMP |
| `tensor` | matrices y estadística |
| `tabla` | columnas con nombre, filtros, orden |
| `autograd` | gradientes |
| `redes` | red neuronal |

---

## 8. Propiedad y préstamos

### Reglas

| Regla | Si se rompe |
|---|---|
| Un valor con dueño tiene un solo dueño | `NOK5004` al copiarlo |
| Pasar por valor **mueve** | usar el original da `NOK5001` |
| `&` presta para leer y no mueve | — |
| `&mutable` presta para escribir | — |
| No se mueve mientras hay un préstamo vivo | `NOK5007` |
| Un préstamo no se guarda en una variable | `NOK5002` |
| No se devuelve un préstamo a algo local | `NOK5006` |
| No se escribe a través de un préstamo inmutable | `NOK4009` |
| Dos préstamos mutables a la vez, no | `NOK5003` |

### Qué libera el compilador

- Los locales con dueño, al salir del bloque o de la función, y al `romper` o `continuar`.
- El búfer viejo de un `texto` al reasignarlo.
- El contenido viejo de una colección al reasignarla.
- Las claves y entradas de un mapa al liberarlo.

Los **temporales** de una expresión no se liberan: `imprimir(textos.unir(...))` deja unos bytes al
final del proceso (asunto abierto A-18). No corrompe nada.

### `inseguro`

Desactiva **solo** las comprobaciones aritméticas. La propiedad y los tipos se siguen comprobando.

---

## 9. Módulos y visibilidad

- Un módulo es un archivo `.oki` sin `principal`.
- `usar nombre` importa otro archivo.
- Solo lo marcado `publico` es visible desde fuera; el resto es privado del módulo.
- La búsqueda de módulos, en orden: junto al archivo, `biblioteca/`, carpetas padre, dependencias del
  `nooki.toml`, biblioteca estándar.
- El **`nooki.toml`** es el manifiesto del proyecto: nombre, versión y dependencias (rutas a otras
  carpetas). Está explicado en [`docs/PROYECTO.md`](docs/PROYECTO.md).

---

## 10. Diagnósticos

<!-- GENERADO:diagnosticos -->
Hay **46 códigos**. La tabla completa, con el mensaje exacto de cada uno, está en
[`docs/referencia/diagnosticos.md`](docs/referencia/diagnosticos.md), generada desde `codigos.hpp` y
`catalogo.cpp`.
<!-- FIN -->

Rangos:

| Rango | Etapa |
|---|---|
| `NOK1xxx` | léxico (caracteres, textos, números) |
| `NOK2xxx` | sintaxis |
| `NOK3xxx` | nombres, tipos y módulos |
| `NOK4xxx` | tipos y comprobaciones semánticas |
| `NOK5xxx` | propiedad y préstamos |
| `NOK6xxx` | backend de C y ejecución |
| `NOK0xxx` | el propio compilador |

---

## 11. Herramientas

### Comandos

| Comando | Qué hace | Opciones |
|---|---|---|
| `nooki nuevo <nombre>` | crea un proyecto | |
| `nooki construir [carpeta]` | compila el proyecto actual, o el de esa carpeta | `-o <salida>` |
| `nooki compilar <archivo>` | compila | `-o <salida>`, `--emitir-c` |
| `nooki ejecutar <archivo>` | compila y ejecuta | |
| `nooki probar` | pruebas del proyecto | |
| `nooki verificar <archivo>` | comprueba sin generar C | |
| `nooki formatear <archivo>` | sangría canónica | `--comprobar` |
| `nooki documentar` | genera `API.md` | `--salida` |
| `nooki lsp` | servidor de lenguaje | |
| `nooki ast <archivo>` | árbol de sintaxis | |
| `nooki limpiar` | borra artefactos | |
| `nooki ayuda`, `nooki version` | | |

### Códigos de salida

| Código | Significado |
|---|---|
| 0 | todo bien |
| 64 | uso incorrecto (`USO_INCORRECTO`) |
| 65 | el programa tiene errores (`DATOS_INCORRECTOS`) |
| 69 | herramienta del sistema no disponible (`NO_DISPONIBLE`) |
| 70 | pánico en ejecución: desbordamiento, división por cero (`ERROR_INTERNO`) |

### Servidor de lenguaje

| Método | Qué da |
|---|---|
| `initialize` | capacidades |
| `textDocument/didOpen`, `didChange` | sincronización de texto completo |
| `textDocument/publishDiagnostics` | errores con código y span |
| `textDocument/completion` | palabras, tipos y funciones |
| `textDocument/definition` | funciones, tipos, enumeraciones, variables y parámetros |
| `textDocument/formatting` | el documento entero |

---

## 12. Lo que no existe

Dicho aquí para que nadie lo busque:

| No existe | Alternativa |
|---|---|
| Excepciones | `resultado` y `?` |
| Conversión implícita entre tipos | `como` |
| `sino si` | anidar `sino { si ... }` |
| Literales de carácter `'a'` | `caracter` no está implementado |
| `"" + 42` | pendiente (formato de números a texto) |
| `??` | `coincide` sobre `nulo` — el backend no lo traduce |
| Sobrecarga de funciones | nombres distintos |
| Parámetros por defecto | funciones con otro nombre |
| Cierres y funciones de primera clase | — |
| `static` dentro de una clase | — |
| Genéricos con restricciones (`<T: Rasgo>`) | — |
| Herencia múltiple de clases | un rasgo por cada contrato |
| Interfaces separadas de los rasgos | `interfaz` está reservada, sin uso |
| `macro`, `mixin`, `union`, `asincrono` | reservadas, sin uso |
| Recolector de basura | propiedad estática |
