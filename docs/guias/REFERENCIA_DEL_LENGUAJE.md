# Referencia del lenguaje Nooki

**Consulta normativa de la implementación 1.0.0: qué existe, cómo se escribe, qué hace y qué no.**

| | |
|---|---|
| Lenguaje | Nooki |
| Versión del compilador | **1.0.0** |
| Edición | **2026** |
| Extensión | `.oki` |
| Implementación | C++20 + backend C (ADR-014) |
| Documento normativo de diseño | [`spec/especificacion-v0.1.md`](../../spec/especificacion-v0.1.md) |

> **Cómo leer esta referencia.** Describe lo que **el compilador 1.0.0 acepta y traduce**,
> comprobado contra el código y con programas ejecutados. La especificación de `spec/`
> contiene además características planificadas que todavía no existen; cuando discrepan,
> esta referencia indica la diferencia. Si algo de aquí no compila, es un fallo del
> compilador o una limitación documentada, no una promesa.

## Índice

1. [Léxico](#1-léxico)
2. [Tipos](#2-tipos)
3. [Expresiones y operadores](#3-expresiones-y-operadores)
4. [Sentencias](#4-sentencias)
5. [Declaraciones](#5-declaraciones)
6. [Propiedad y préstamos](#6-propiedad-y-préstamos)
7. [Módulos, proyectos y visibilidad](#7-módulos-proyectos-y-visibilidad)
8. [Biblioteca estándar](#8-biblioteca-estándar)
9. [Diagnósticos](#9-diagnósticos)
10. [Herramientas](#10-herramientas)
11. [Gramática resumida](#11-gramática-resumida)
12. [Apéndices](#12-apéndices)

---

## 1. Léxico

### 1.1 Archivos

| Aspecto | Regla |
|---|---|
| Extensión | `.oki` |
| Codificación | UTF-8 |
| BOM | **no** se admite; produce `NOK1004` |
| Saltos de línea | `\n`, `\r\n`, `\r`; se normalizan a `\n` |
| Terminación de sentencia | salto de línea; no se usa `;` |
| Bloques | `{ }` |
| Comentarios | `//` de línea y `/* ... */` de bloque, anidables |

### 1.2 Identificadores

- Empiezan por una letra ASCII, una letra acentuada del español (`á é í ó ú ü ñ` y sus
  mayúsculas) o `_` seguido de algo.
- Continúan con letras, dígitos o `_`.
- Distinguen mayúsculas: `valor` y `Valor` son distintos.
- No pueden ser una palabra reservada.
- `_` aislado es el comodín de `coincide`, no un identificador.

```text
válidos      nombre_pila   año   número   _interno   árbol2
no válidos   2nombre       texto (reservada)   clase (reservada)
```

Las **palabras clave se escriben sin tilde**. `función` no es una palabra clave; el lexer
emite `NOK1001` y sugiere `funcion`.

### 1.3 Palabras reservadas (85)

| Grupo | Palabras |
|---|---|
| Declaraciones (15) | `clase`, `constante`, `enumeracion`, `estructura`, `exportar`, `externo`, `funcion`, `implementar`, `importar`, `interfaz`, `modulo`, `rasgo`, `tipo`, `usar`, `variable` |
| Control de flujo (11) | `caso`, `coincide`, `continuar`, `en`, `hacer`, `mientras`, `para`, `retornar`, `romper`, `si`, `sino` |
| Objetos (11) | `Este`, `constructor`, `destructor`, `establecer`, `este`, `hereda`, `implementa`, `nuevo`, `obtener`, `propiedad`, `super` |
| Acceso (7) | `abstracta`, `estatico`, `interna`, `mutable`, `privado`, `protegido`, `publico` |
| Tipos (21) | `booleano`, `byte`, `caracter`, `entero`, `entero128`, `entero16`, `entero32`, `entero64`, `entero8`, `flotante`, `flotante32`, `natural`, `natural128`, `natural16`, `natural32`, `natural64`, `natural8`, `tamano`, `tamano_signado`, `texto`, `vacio` |
| Literales (3) | `falso`, `nulo`, `verdadero` |
| Conversión (1) | `como` |
| Bajo nivel (2) | `asm`, `inseguro` |
| Reservadas sin uso (14) | `asincrono`, `atrapar`, `bucle`, `donde`, `es`, `esperar`, `finalmente`, `intentar`, `lanzar`, `macro`, `mixin`, `paso`, `sinosi`, `union` |

Aunque `bucle`, `paso` y `sinosi` figuran en el último grupo de la tabla generada, el parser
**sí** los usa. Las palabras realmente sin implementación en la 1.0.0 incluyen `asincrono`,
`atrapar`, `donde`, `es`, `esperar`, `finalmente`, `intentar`, `lanzar`, `macro`, `mixin` y
`union`.

### 1.4 Comentarios

```nooki
// hasta el final de la línea

/* bloque
   /* anidado */
   sigue fuera del anidado */

funcion principal() -> entero {
    imprimir("// esto es texto, no comentario")
    retornar 0
}
```

Los comentarios de bloque **se anidan** en la implementación. Un bloque sin cerrar produce
`NOK1008`.

### 1.5 Literales

| Tipo | Formas | Notas |
|---|---|---|
| `entero` | `0`, `42`, `-7`, `1_000`, `0x1F`, `0b1010`, `0o17` | 64 bits con signo; `_` es separador |
| `flotante` | `3.14`, `-0.5`, `2.0`, `1e-9`, `2.5e10` | doble precisión IEEE 754; el punto o el exponente son obligatorios |
| `texto` | `"hola"`, `"a\nb"`, `"""..."""` | UTF-8; `"""` es multilínea |
| `booleano` | `verdadero`, `falso` | |
| `nulo` | `nulo` | literal; no hay tipo opcional utilizable |
| carácter | `'a'` | el lexer lo reconoce, el backend no lo traduce (`NOK6003`) |

Escapes reconocidos por el lexer dentro de `"..."`: `\n`, `\t`, `\\`, `\"`. Otro escape
produce `NOK1009`.

> **Estado real de los escapes (1.0.0).** El generador de C conserva la barra invertida, así
> que `imprimir("a\nb")` muestra `a\nb` literalmente, no un salto. Para saltos reales, usa
> un texto multilínea `"""..."""`. Los textos multilínea no interpretan escapes.

No hay:

- interpolación `{expresión}` (las llaves se imprimen tal cual);
- literales de carácter utilizables;
- cadenas sin procesar;
- números complejos;
- literales octales con prefijo `0` (solo `0o`);
- literales binarios con `0b` (sí existen) ni separadores distintos de `_`.

### 1.6 Operadores y signos

```text
+  -  *  /  %  **  como
==  !=  <  >  <=  >=
&&  ||  !
&  |  ^  <<  >>
=  +=  -=  *=  /=  %=  &=  |=  ^=  <<=  >>=
..  ..=
?  ??  ?.
.  ,  :  (  )  {  }  [  ]
&  *  (unarios)
```

`?.` y `??` se lexan y parsean, pero el backend actual no los traduce (`NOK6003`).

---

## 2. Tipos

### 2.1 Tipos implementados

| Tipo | Representación en C | Tamaño / forma | Literales |
|---|---|---|---|
| `entero` | `long long` | 64 bits con signo | `42`, `0x1F`, `0b1010`, `0o17` |
| `flotante` | `double` | 64 bits IEEE 754 | `3.14`, `1e-9` |
| `booleano` | `int` (0 o 1) | — | `verdadero`, `falso` |
| `texto` | `struct { char* datos; long long largo; int propio; }` | UTF-8 con dueño | `"..."`, `"""..."""` |
| `vacio` | `void` | ausencia de valor | solo tipo de retorno |

`texto` cuenta **bytes**, no caracteres. `largo()`, `subtexto`, `buscar` y `caracter` usan
posiciones de byte; `sin_ultimo` y `agregar_codigo` trabajan con puntos de código UTF-8.

### 2.2 Tipos compuestos y especiales

| Forma | Estado | Notas |
|---|---|---|
| `Vector<entero>` | implementado | vector dinámico de enteros |
| `Vector<texto>` | implementado | vector dueño de textos |
| `Mapa<texto, entero>` | implementado | tabla hash; claves de texto y valores enteros |
| `Caja<entero>` | implementado | caja de un entero con dueño |
| `Caja<texto>` | **no** | genera C inválido (`NOK6002`) |
| `Vector<flotante>` | **no** | el generador lo trata como vector de enteros |
| `resultado` | implementado | enumeración `Ok(entero)` / `Error(texto)` |
| `&T` | implementado | préstamo inmutable |
| `&mutable T` | implementado | préstamo mutable |
| `&Rasgo` | implementado | referencia a una clase que cumple un rasgo |
| estructuras, clases, enumeraciones | implementado | definidas por el usuario; las estructuras con métodos/herencia tienen soporte parcial |
| tipos genéricos `T<...>` | implementado | instanciación explícita |
| `nulo` | literal | sin tipo opcional |
| `ninguno`, tuplas, arreglos fijos, conjuntos, opcionales | **no** | — |

### 2.3 Tipos reservados sin implementar

Estas palabras son reservadas, pero usarlas en una anotación produce `NOK3003`:

```text
entero8  entero16  entero32  entero64  entero128
natural  natural8  natural16  natural32  natural64  natural128
flotante32  caracter  byte  tamano  tamano_signado  lista  esperar
```

`entero` ya es de 64 bits; `entero64` no aporta nada. `lista`, `caracter`, `byte` y `esperar`
vienen del plan original y no tienen implementación.

### 2.4 Conversiones

Solo existen las conversiones explícitas con `como`:

```nooki
expresion como entero      // trunca hacia cero
expresion como flotante
```

No hay conversión implícita entre `entero` y `flotante` (`NOK4001`). Tampoco hay conversión
de `texto` a número con `como`; para eso está `texto.a_entero()`.

```nooki
funcion principal() -> entero {
    variable a = 7 como flotante      // 7,0
    variable b = 3.9 como entero      // 3
    imprimir(a)
    imprimir(b)
    retornar 0
}
```

### 2.5 Igualdad y orden por tipo

| Operación | `entero` | `flotante` | `texto` | `booleano` |
|---|---|---|---|---|
| `==` `!=` | sí | sí | por contenido | sí |
| `<` `>` `<=` `>=` | sí | sí | orden lexicográfico por bytes | no |
| `+` | suma comprobada | suma | concatenación | no |
| `-` `*` `/` `%` | comprobadas | IEEE 754 | no | no |
| `**` | potencia entera | `pow` | no | no |

---

## 3. Expresiones y operadores

### 3.1 Precedencia y asociatividad

De menor a mayor potencia de enlace:

| Potencia | Operadores | Asociatividad | Traduce |
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

Los paréntesis agrupan. `-2 ** 2` es un asunto abierto (A-08): usa `(-2) ** 2` o
`-(2 ** 2)` según lo que quieras.

### 3.2 Aritmética entera comprobada

`entero` usa 64 bits con signo. Sumar, restar, multiplicar, dividir o calcular el resto
comprueba el desbordamiento y la división por cero. Si falla:

```text
error en ejecucion: desbordamiento al sumar
```

y el proceso termina con **70**. La división entera trunca hacia cero; el resto sigue la
semántica de C (signo del dividendo).

Dentro de `inseguro { }` se usan los operadores de C sin comprobar. Esto **solo** afecta a
la aritmética; la propiedad, los tipos y los préstamos se siguen verificando.

### 3.3 `flotante`

Sigue IEEE 754. No hay comprobación de desbordamiento. `imprimir` usa hasta seis decimales,
quita ceros sobrantes y usa coma decimal:

```nooki
imprimir(3.5)          // 3,5
imprimir(2.0)          // 2
imprimir(1.0 / 3.0)    // 0,333333
```

### 3.4 Operadores lógicos

`&&` y `||` cortocircuitan. `!` niega un booleano. Las condiciones de `si`, `mientras`,
`hacer` y `para` deben ser booleanas; un entero no vale (`NOK4005`).

```nooki
si (n != 0) { imprimir("distinto de cero") }
```

### 3.5 Operadores de bits y desplazamientos

`&`, `|`, `^`, `<<`, `>>` operan sobre `entero`. `<<` y `>>` se comportan como en C:
desplazar 64 bits o más es comportamiento indefinido; no hay comprobación. No los uses con
cantidades no verificadas.

### 3.6 Asignación

```nooki
variable a = 1
a = 2
a += 3
a -= 1
a *= 2
a /= 2
a %= 2
a &= 1
a |= 2
a ^= 3
a <<= 1
a >>= 1
```

Asignar a una `constante` produce `NOK2001`.

### 3.7 Rangos

`inicio..fin` excluye el final; `inicio..=fin` lo incluye. Solo se traducen dentro de un
`para`:

```nooki
para (i en 0..3) { imprimir(i) }    // 0 1 2
para (i en 1..=3) { imprimir(i) }   // 1 2 3
para (i en 0..10 paso 3) { imprimir(i) }  // 0 3 6 9
```

Un rango suelto en otra expresión produce `NOK6003`. Los rangos no se pueden encadenar
(`NOK2005`).

### 3.8 `?` (propagación de resultado)

`?` solo se aplica a un valor de tipo `resultado`, dentro de una función que devuelve
`resultado`. Si el valor es `Error`, la función devuelve ese error; si es `Ok`, la expresión
vale el entero interior.

```nooki
usar resultado

funcion mitad(a: entero) -> resultado {
    si (a % 2 == 1) { retornar Error("impar") }
    retornar Ok(a / 2)
}

funcion doble(a: entero) -> resultado {
    variable m = mitad(a)?
    retornar Ok(m * 2)
}
```

Fuera de una función `resultado`: `NOK4011`.

### 3.9 Acceso a miembros y llamadas

```nooki
objeto.campo
objeto.metodo(argumentos)
modulo.funcion(argumentos)
Vector<entero>.nuevo()
Caja<entero>.nuevo(1)
identidad<entero>(5)
```

`?.` (acceso opcional) se parsea pero no se traduce. `??` (coalescencia) tampoco.

### 3.10 Construcción de valores

```nooki
variable p = punto { x: 1, y: 2 }              // estructura
variable q = par<entero> { primero: 1, segundo: 2 }
variable c = nuevo contador { valor: 10 }      // clase
```

Los nombres de campo se escriben con `:` y se separan con comas. El orden no importa, pero
deben estar todos los campos del tipo (los heredados incluidos) o se emite `NOK3005`.

### 3.11 `este` y `super`

- `este` es el objeto actual dentro de un método de clase.
- `super.metodo(...)` llama a la implementación de la clase base.

```nooki
clase perro hereda animal {
    publico funcion presentarse() -> texto {
        retornar super.presentarse() + " y ladro"
    }
}
```

`este` no existe en funciones libres ni en estructuras.

### 3.12 `inseguro` y `asm!`

`inseguro { ... }` es una expresión de bloque que desactiva las comprobaciones aritméticas.
`asm!("texto")` inserta ensamblador en línea y **solo** se permite dentro de `inseguro`:

```nooki
inseguro {
    asm!("nop")
}
```

Fuera de `inseguro`: `NOK3012`. El texto se copia tal cual a `__asm__(...)`; funciona con
GCC/Clang y no con MSVC.

---

## 4. Sentencias

Cada sentencia ocupa su línea. No hay `;` obligatorio. Los bloques son `{ }`.

### 4.1 Declaración de variable o constante

```nooki
variable nombre = expresion
variable nombre: tipo = expresion
constante NOMBRE = expresion
constante NOMBRE: tipo = expresion
```

El tipo y el valor se pueden omitir, pero no ambos. `constante` no se puede reasignar
(`NOK2001`). El tipo se deduce del valor si no se escribe.

### 4.2 Asignación

```nooki
destino = expresion
destino += expresion
destino -= expresion
destino *= expresion
destino /= expresion
destino %= expresion
destino &= expresion
destino |= expresion
destino ^= expresion
destino <<= expresion
destino >>= expresion
```

### 4.3 Condicional

```nooki
si (condicion) {
    ...
} sinosi (otra_condicion) {
    ...
} sino {
    ...
}
```

- La condición debe ser `booleano`.
- `sinosi` es la forma de encadenar; **no** existe `sino si`.
- `sino` es opcional.

### 4.4 Bucles

```nooki
mientras (condicion) { ... }

hacer {
    ...
} mientras (condicion)

para (i en 0..10) { ... }
para (i en 0..=10) { ... }
para (i en 0..10 paso 2) { ... }
para (elemento en coleccion) { ... }

bucle {
    ...
    si (listo) { romper }
}
```

- `mientras` comprueba antes de cada vuelta.
- `hacer` ejecuta al menos una vez.
- `para` con rango usa `..`/`..=`; con `paso` avanza de `paso` en `paso`. Si el inicio es
  mayor o igual que el final, no hay vueltas.
- `para` con colección recorre un `Vector` (no un `Mapa`).
- `bucle` es un bucle infinito explícito.
- `romper` sale del bucle; `continuar` salta a la siguiente vuelta.

### 4.5 Retorno

```nooki
retornar expresion
retornar
```

El tipo debe coincidir con el declarado (`NOK4004`). En `principal`, el entero es el código
de salida. Si no hay `retornar`, `principal` sale con 0.

### 4.6 `imprimir`

```nooki
imprimir(expresion)
```

Acepta `entero`, `flotante`, `booleano` y `texto`. Añade un salto de línea. No acepta
estructuras, clases, enumeraciones ni `nulo` (`NOK6003`).

### 4.7 `coincide`

```nooki
coincide (expresion) {
    caso patron => sentencia
    caso patron => {
        ...
    }
}
```

Patrones:

| Patrón | Ejemplo | Liga |
|---|---|---|
| Variante con datos | `caso circulo(r) =>` | `r` |
| Variante sin datos | `caso forma.sin_forma =>` | — |
| Literal entero | `caso 1 =>` | — |
| Literal texto | `caso "ana" =>` | — |
| Literal booleano | `caso verdadero =>` | — |
| Comodín | `caso _ =>` | — |

Reglas:

- Sobre una enumeración debe cubrir todas las variantes o incluir `caso _` (`NOK4006`).
- Sobre enteros o textos, incluye `caso _` para el resto.
- No hay guardas (`caso n si ...`).
- Una rama que devuelve un valor necesita un bloque `{ }`.

### 4.8 Bloques y ámbitos

```nooki
{
    variable temporal = 1
}
// «temporal» ya no existe
```

Cada bloque crea un ámbito. Los valores con dueño se liberan al salir, incluso al `romper`
o `continuar`.

---

## 5. Declaraciones

Solo puede haber declaraciones en el nivel superior: funciones, estructuras, clases, rasgos,
enumeraciones y `usar`. **No hay variables globales.**

### 5.1 Funciones

```text
funcion nombre(parametro: tipo, otro: &tipo, mutable: &mutable tipo) -> tipo_retorno {
    ...
}
```

- Sin `->` o con `-> vacio`, no devuelve valor.
- Los parámetros con `&` prestan; con `&mutable`, prestan para escribir.
- Un `texto`, `Vector`, `Mapa`, `Caja` o clase por valor **se mueve**.
- `funcion principal() -> entero` es la entrada; su retorno es el código de salida.
- No hay sobrecarga, parámetros por defecto, cierres ni funciones de primera clase.

### 5.2 Funciones externas

```nooki
externo funcion strlen(cadena: &texto) -> entero
```

Declara una función de C. El enlazador la resuelve contra libc o las bibliotecas que añada
el wrapper de `NOOKI_CC`. `texto` se pasa como `const char*`; una función que devuelve
`texto` se copia al llegar. No hay `externo` con cuerpo.

### 5.3 Estructuras

```nooki
estructura punto {
    publico x: entero
    publico y: entero
    privado id: entero
}
```

- Se pasan **por valor**; al asignarlas, se copian.
- El modelo previsto es datos sin identidad; **no** hay despacho dinámico ni `nuevo`.
- En la implementación 1.0.0 el parser acepta métodos, `este`, `super` y `hereda` en una
  estructura, pero el soporte es **parcial**: una llamada a un método heredado directamente
  sobre la estructura hija puede fallar en el backend (`NOK6002`). El uso recomendado es
  datos planos; para comportamiento usa una clase.
- Los campos pueden separarse por saltos de línea o comas.
- `publico`/`privado` controlan la lectura desde fuera del módulo.
- Una estructura con campos con dueño no se puede copiar (`NOK5004`); pásala con `&` o
  construye otra. Una estructura tampoco se puede prestar como `&Rasgo` en la 1.0.0
  (`NOK4008`).

### 5.4 Clases

```nooki
clase nombre hereda base implementa Rasgo, Otro {
    publico campo: tipo

    publico funcion metodo() -> tipo {
        ...
    }
}
```

- Se crean con `nuevo nombre { campo: valor }`.
- Se pasan **por referencia**; la variable es dueña del objeto.
- `este` es el objeto; `super` es la base.
- Se hereda de **una** clase y se pueden implementar **varios** rasgos.
- `hereda` e `implementa` pueden ir en cualquier orden, pero `hereda` es único.
- El despacho de métodos es dinámico por tabla de clase.
- No hay constructores/destructores explícitos, `static`, propiedades ni herencia múltiple
  en la 1.0.0.

### 5.5 Rasgos

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
```

- Un rasgo es un contrato de métodos con firma exacta.
- La clase que lo declara debe implementarlos todos (`NOK3011`) con la firma correcta
  (`NOK4012`).
- `&Rasgo` acepta cualquier clase que lo implemente; el despacho es dinámico.
- Los rasgos se declaran sin `publico`; se importan con el módulo.

### 5.6 Enumeraciones

```nooki
enumeracion forma {
    circulo(radio: entero)
    cuadrado(lado: entero)
    sin_forma
}
```

- Variantes **con datos**: se usan sin cualificar (`circulo(2)`, `caso circulo(r)`).
- Variantes **sin datos**: se cualifican (`forma.sin_forma`, `caso forma.sin_forma`).
- Las variantes pueden separarse por saltos de línea o comas.
- `coincide` debe ser exhaustivo.

### 5.7 Tipos genéricos

```nooki
estructura par<T> {
    publico primero: T
    publico segundo: T
}

funcion identidad<T>(x: T) -> T {
    retornar x
}
```

Se instancian explícitamente:

```nooki
variable p = par<entero> { primero: 1, segundo: 2 }
variable t = par<texto> { primero: "a", segundo: "b" }
imprimir(identidad<entero>(7))
```

Cada combinación genera su propia versión en C. No hay restricciones `<T: Rasgo>` ni
inferencia de argumentos de tipo sin escribirlos.

### 5.8 Visibilidad

- `publico` expone funciones, estructuras, clases y enumeraciones fuera del módulo.
- Sin `publico`, son privadas del módulo.
- `privado` se usa en campos.
- `protegido` e `interna` se aceptan en campos, pero en la práctica no aportan una
  semántica distinta de privado en la 1.0.0.
- La visibilidad de métodos de clase no se aplica de forma estricta; escribe `publico` por
  claridad y no diseñes una API basada en métodos privados.

### 5.9 `usar`

```nooki
usar geometria
usar resultado
usar ventana
```

- Solo un identificador por `usar`; no hay rutas con puntos, alias ni importación selectiva.
- Las funciones se llaman con prefijo (`geometria.area`).
- Los tipos y variantes de enumeración se importan sin prefijo.
- Un módulo es un archivo `.oki` sin `principal`.

---

## 6. Propiedad y préstamos

### 6.1 Tipos con dueño

Tienen dueño: `texto`, `Vector`, `Mapa`, `Caja` y las clases. Las estructuras y los tipos
básicos se copian por valor; una estructura con campos con dueño no se puede copiar
(`NOK5004`).

### 6.2 Reglas

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
| Solo se desreferencia una `Caja` con `*` | `NOK4007` |

### 6.3 Mover

```nooki
variable original = "hola"
variable copia = original
imprimir(copia)          // bien
// imprimir(original)    // NOK5001: se movió
```

También se mueve al pasar un valor por valor, al retornarlo y al insertarlo en una colección
que se queda con él.

### 6.4 Prestar

```nooki
funcion longitud(t: &texto) -> entero {
    retornar t.largo()
}

variable saludo = "hola"
imprimir(longitud(&saludo))
imprimir(saludo)          // sigue siendo mío
```

Dentro de la función, `t` es un puntero; para copiar el valor se desreferencia con `*`.

### 6.5 Prestar para escribir

```nooki
funcion sumar(c: &mutable Caja<entero>, n: entero) -> vacio {
    c.poner(c.sacar() + n)
}

variable caja = Caja<entero>.nuevo(0)
sumar(&mutable caja, 5)
```

### 6.6 Qué libera el compilador

- Locales con dueño al salir del bloque/función.
- El búfer viejo de un `texto` al reasignarlo.
- El contenido viejo de una colección al reasignarla.
- Claves y entradas de un mapa al liberarlo.
- Locales al salir de un bucle con `romper`/`continuar`.
- Campos con dueño de un compuesto cuando se libera el compuesto.

Los **temporales** de una expresión no se liberan (fuga menor A-18). No corrompe memoria.

### 6.7 `inseguro`

`inseguro { }` desactiva solo la aritmética comprobada. No es un modo «sin reglas»: la
propiedad, los tipos y los préstamos se siguen verificando. `asm!` solo vive dentro.

---

## 7. Módulos, proyectos y visibilidad

### 7.1 Módulos

- Un módulo es un archivo `.oki` sin `principal`.
- `usar nombre` lo importa.
- El nombre del módulo es el nombre del archivo sin `.oki`.
- Solo lo `publico` es visible desde fuera.
- Las funciones se llaman `modulo.funcion`; los tipos y variantes, sin prefijo.
- La carga es recursiva y evita duplicados; un ciclo no duplica el módulo.

### 7.2 Búsqueda de módulos

Para `usar nombre`, en orden:

1. `nombre.oki` en la carpeta del archivo que hace el `usar`.
2. La raíz y la `biblioteca/` de cada dependencia del `nooki.toml`.
3. `biblioteca/nombre.oki` en la carpeta del archivo y en las carpetas padre, subiendo.
4. La biblioteca estándar embebida.

Consecuencias:

- Una dependencia puede tapar un módulo local con el mismo nombre; evita colisiones.
- Un `textos.oki` en una carpeta padre puede tapar el módulo estándar `textos`.
- El archivo de entrada no puede importarse a sí mismo; se ignora ese candidato y se sigue
  buscando.

### 7.3 Proyecto

```text
proyecto/
├── nooki.toml
├── principal.oki          o fuente/principal.oki
├── biblioteca/
├── pruebas/
└── API.md                 (generado)
```

`nooki construir` busca `principal.oki` y, si no está, `fuente/principal.oki`. Sin entrada
no hay proyecto (`NOK0001`). Si la entrada está en `fuente/`, el binario por defecto se escribe
en `fuente/` (salvo que se use `-o`).

### 7.4 Manifiesto `nooki.toml`

```toml
[proyecto]
nombre = "demo"
version = "0.1.0"

[dependencias]
utilidades = { ruta = "../utilidades" }
```

Reglas:

- Secciones entre corchetes.
- `clave = valor`; `#` inicia comentario.
- `[proyecto]`: `nombre` y `version`; las comillas son opcionales.
- `[dependencias]`: solo `nombre = { ruta = "..." }`; la ruta es obligatoria, entre
  comillas y relativa al manifiesto.
- No hay listas, tablas anidadas, versiones, descarga, bloqueo ni dependencias transitivas.

### 7.5 Pruebas del proyecto

```text
pruebas/
├── saludo.oki
└── saludo.oki.esperado
```

`nooki probar` compila cada `.oki`, lo ejecuta y compara su salida con
`<archivo>.oki.esperado`. La comparación ignora `\r`, pero no espacios ni líneas de más. Si
no existe el `.esperado`, la prueba se considera correcta y se informa «sin .esperado».

### 7.6 Limitaciones de herramientas

- `nooki verificar` y `nooki probar` **no** cargan las dependencias del `nooki.toml`; solo
  `nooki construir` aplica esas rutas.
- `nooki documentar` no incluye métodos de clase.
- `nooki limpiar` borra todos los `.exe`, `.bin` y `.salida` del árbol, además de `build/`
  y `artefactos/`.

---

## 8. Biblioteca estándar

16 módulos, escritos en Nooki (con runtime C solo para UI/IA). Se importan con `usar`:

```nooki
usar resultado
usar matematicas
usar textos
usar listas
usar fechas
usar archivo
usar pln
usar ventana
usar diseno
usar controles
usar accesibilidad
usar imagenes
usar tensor
usar tabla
usar autograd
usar redes
```

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

### 8.1 `resultado`

```text
publico enumeracion resultado {
    Ok(valor: entero)
    Error(mensaje: texto)
}
```

Se usa con `coincide` y `?`. `?` solo en funciones que devuelven `resultado`.

### 8.2 Tipos integrados

#### `texto`

| Miembro | Firma | Qué hace |
|---|---|---|
| `largo` | `() -> entero` | número de bytes |
| `subtexto` | `(desde: entero, cuantos: entero) -> texto` | trozo; si `desde` no es válido, vacío |
| `buscar` | `(&aguja: texto) -> entero` | posición en bytes, o `-1`; aguja vacía da `-1` |
| `contiene` | `(&aguja: texto) -> booleano` | |
| `empieza_con` | `(&prefijo: texto) -> booleano` | |
| `mayusculas` | `() -> texto` | solo ASCII |
| `minusculas` | `() -> texto` | solo ASCII |
| `recortar` | `() -> texto` | quita espacios ASCII de los extremos |
| `dividir` | `(&separador: texto) -> Vector<texto>` | separador vacío devuelve el texto entero |
| `caracter` | `(i: entero) -> entero` | byte en esa posición, o `-1` |
| `a_entero` | `() -> entero` | lee un número al principio; para en el primer carácter no dígito |
| `agregar_codigo` | `(codigo: entero) -> texto` | añade un punto de código UTF-8 |
| `sin_ultimo` | `() -> texto` | quita el último punto de código |

#### `Vector<T>`

| Miembro | Firma | Qué hace |
|---|---|---|
| `nuevo` | `Vector<T>.nuevo() -> Vector<T>` | vector vacío |
| `agregar` | `(valor: T) -> vacio` | añade al final (mueve si T tiene dueño) |
| `obtener` | `(i: entero) -> T` | lee; fuera de rango, valor cero o texto vacío |
| `modificar` | `(i: entero, valor: T) -> vacio` | escribe; fuera de rango no hace nada |
| `largo` | `() -> entero` | cuántos hay |
| `vaciar` | `() -> vacio` | los quita todos y libera los textos |

Solo se usan `Vector<entero>` y `Vector<texto>`. `Vector<flotante>` no está soportado de
verdad: el generador lo trata como vector de enteros.

#### `Mapa<texto, entero>`

| Miembro | Firma | Qué hace |
|---|---|---|
| `nuevo` | `Mapa<texto, entero>.nuevo() -> Mapa` | mapa vacío |
| `insertar` | `(clave: texto, valor: entero) -> vacio` | inserta o actualiza; la clave se copia |
| `obtener` | `(clave: texto) -> entero` | valor, o `0` si no está |
| `contiene` | `(clave: texto) -> booleano` | |
| `quitar` | `(clave: texto) -> entero` | `1` si estaba, `0` si no |
| `largo` | `() -> entero` | cuántas claves |
| `claves` | `() -> Vector<texto>` | las claves, para recorrerlas |

No se recorre un `Mapa` con `para`; usa `claves()`.

#### `Caja<entero>`

| Miembro | Firma | Qué hace |
|---|---|---|
| `nuevo` | `Caja<entero>.nuevo(valor) -> Caja<entero>` | caja con un valor |
| `sacar` | `() -> entero` | lee el valor |
| `poner` | `(valor: entero) -> vacio` | lo cambia |

`Caja<texto>` no genera C válido en la 1.0.0.

### 8.3 `matematicas`

```text
publico funcion maximo(a: entero, b: entero) -> entero
publico funcion minimo(a: entero, b: entero) -> entero
publico funcion absoluto(n: entero) -> entero
publico funcion cuadrado(n: entero) -> entero
```

### 8.4 `textos`

```text
publico funcion unir(partes: &Vector<texto>, separador: texto) -> texto
publico funcion repetir(pieza: &texto, veces: entero) -> texto
publico funcion contar(pieza: &texto, aguja: &texto) -> entero
```

`unir` concatena con el separador (que se mueve). `repetir` repite el texto. `contar` cuenta
apariciones.

### 8.5 `listas`

```text
publico funcion suma(numeros: &Vector<entero>) -> entero
publico funcion mayor(numeros: &Vector<entero>) -> entero
publico funcion menor(numeros: &Vector<entero>) -> entero
publico funcion contiene(numeros: &Vector<entero>, buscado: entero) -> booleano
publico funcion promedio(numeros: &Vector<entero>) -> entero
```

`mayor`/`menor` sobre un vector vacío devuelven 0; `promedio` devuelve 0 si está vacío.

### 8.6 `fechas`

```text
publico funcion bisiesto(anio: entero) -> booleano
publico funcion dias_del_mes(mes: entero, anio: entero) -> entero
publico funcion dias_del_anio(anio: entero) -> entero
publico funcion a_dias(anio: entero, mes: entero, dia: entero) -> entero
publico funcion a_anio(dias: entero) -> entero
publico funcion a_mes(dias: entero) -> entero
publico funcion a_dia(dias: entero) -> entero
publico funcion sumar_dias(dias: entero, cantidad: entero) -> entero
publico funcion dia_de_la_semana(dias: entero) -> entero
```

Una fecha se representa como días desde el 1 de enero de 1970. `dia_de_la_semana` devuelve
0 = lunes … 6 = domingo.

### 8.7 `archivo`

```text
publico enumeracion lectura {
    Bien(contenido: texto)
    Mal(motivo: texto)
}

publico funcion existe(ruta: &texto) -> booleano
publico funcion leer(ruta: &texto) -> lectura
publico funcion escribir(ruta: &texto, contenido: &texto) -> lectura
```

`escribir` sobrescribe y devuelve `Bien("")` si fue bien. No hay modo de añadir ni de listar
directorios.

### 8.8 `pln`

```text
publico funcion normalizar(frase: &texto) -> texto
publico funcion tokenizar(frase: &texto) -> Vector<texto>
publico funcion contar(palabras: &Vector<texto>) -> Mapa<texto, entero>
publico funcion unicas(palabras: &Vector<texto>) -> Vector<texto>
publico funcion distancia(a: &texto, b: &texto) -> entero
publico funcion contiene_texto(palabras: &Vector<texto>, buscada: &texto) -> booleano
publico funcion distintas(palabras: &Vector<texto>) -> entero
publico funcion jaccard(a: &texto, b: &texto) -> entero
publico funcion cuantas(palabras: &Vector<texto>) -> entero
```

`normalizar` pasa a minúsculas y deja un espacio entre palabras. `tokenizar` separa por
espacios. `distancia` es la distancia de edición (Levenshtein) sobre bytes. `jaccard`
devuelve el parecido **por mil** (0..1000).

### 8.9 `ventana`

```text
publico funcion abrir(titulo: &texto, ancho: entero, alto: entero) -> entero
publico funcion viva(manejador: entero) -> booleano
publico funcion siguiente_evento(manejador: entero) -> entero
publico funcion evento(manejador: entero) -> entero
publico funcion evento_x(manejador: entero) -> entero
publico funcion evento_y(manejador: entero) -> entero
publico funcion evento_tecla(manejador: entero) -> entero
publico funcion evento_letra(manejador: entero) -> entero
publico funcion es_cerrar(cual: entero) -> booleano
publico funcion es_movimiento(cual: entero) -> booleano
publico funcion es_pulsacion(cual: entero) -> booleano
publico funcion es_liberacion(cual: entero) -> booleano
publico funcion es_tecla(cual: entero) -> booleano
publico funcion es_letra(cual: entero) -> booleano
publico funcion cerrar(manejador: entero) -> vacio
publico funcion pausa(milisegundos: entero) -> vacio
publico funcion repintar(manejador: entero) -> booleano
publico funcion limpiar(manejador: entero, color: entero) -> vacio
publico funcion rectangulo(manejador: entero, x: entero, y: entero, ancho: entero,
                           alto: entero, color: entero) -> vacio
publico funcion linea(manejador: entero, x1: entero, y1: entero, x2: entero, y2: entero,
                      color: entero) -> vacio
publico funcion escribir_texto(manejador: entero, x: entero, y: entero,
                               contenido: &texto, color: entero) -> vacio
publico funcion presentar(manejador: entero) -> vacio
publico funcion color_en(manejador: entero, x: entero, y: entero) -> entero
```

Códigos de evento: 0 nada, 1 cerrar, 2 movimiento, 3 pulsación, 4 liberación, 5 tecla,
6 letra. Los colores son `0xRRGGBB`. En Windows usa GDI; en Linux X11; en macOS `abrir`
devuelve 0.

### 8.10 `diseno`

```text
publico funcion centrar(disponible: entero, medida: entero) -> entero
publico funcion repartir(disponible: entero, partes: entero) -> entero

publico clase columna {
    publico x: entero
    publico y: entero
    publico ancho: entero
    publico alto: entero
    publico usado: entero
    publico separacion: entero
    publico funcion hueco(alto_hijo: entero) -> entero
    publico funcion cabe(alto_hijo: entero) -> booleano
    publico funcion reiniciar() -> vacio
}

publico clase fila {
    publico x: entero
    publico y: entero
    publico ancho: entero
    publico alto: entero
    publico usado: entero
    publico separacion: entero
    publico funcion hueco(ancho_hijo: entero) -> entero
    publico funcion cabe(ancho_hijo: entero) -> booleano
    publico funcion reiniciar() -> vacio
}
```

`hueco` devuelve la posición del siguiente hijo y avanza el cursor. `cabe` dice si queda
sitio. `reiniciar` vuelve a 0.

### 8.11 `controles`

```text
publico clase control {
    publico x: entero
    publico y: entero
    publico ancho: entero
    publico alto: entero
    publico funcion contiene(px: entero, py: entero) -> booleano
    publico funcion dibujar(pantalla: entero) -> vacio
}

publico clase etiqueta hereda control {
    publico contenido: texto
    publico color: entero
    publico funcion dibujar(pantalla: entero) -> vacio
}

publico clase boton hereda control {
    publico contenido: texto
    publico fondo: entero
    publico color: entero
    publico funcion dibujar(pantalla: entero) -> vacio
    publico funcion pulsado(px: entero, py: entero) -> booleano
}

publico clase campo hereda control {
    publico contenido: texto
    publico fondo: entero
    publico color: entero
    publico marcador: texto
    publico funcion dibujar(pantalla: entero) -> vacio
    publico funcion escribir(codigo: entero) -> vacio
    publico funcion borrar() -> vacio
    publico funcion limpiar_campo() -> vacio
}

publico clase barra hereda control {
    publico fondo: entero
    publico relleno: entero
    publico tanto: entero
    publico funcion dibujar(pantalla: entero) -> vacio
}
```

`campo.escribir` añade un punto de código UTF-8; `borrar` quita el último carácter;
`limpiar_campo` lo deja vacío. `barra.tanto` va de 0 a 100.

### 8.12 `accesibilidad`

```text
publico funcion luminancia(color: entero) -> entero
publico funcion diferencia(a: entero, b: entero) -> entero
publico funcion legible(texto_color: entero, fondo: entero) -> booleano
publico funcion texto_sobre(fondo: entero) -> entero

publico clase foco {
    publico indice: entero
    publico total: entero
    publico funcion siguiente() -> vacio
    publico funcion anterior() -> vacio
    publico funcion es_actual(cual: entero) -> booleano
    publico funcion primera() -> vacio
}
```

`luminancia` usa YIQ (0 negro, 255 blanco). `legible` considera suficiente una diferencia
de 128; no es el contraste WCAG exacto. `texto_sobre` devuelve negro o blanco.

### 8.13 `imagenes`

```text
publico funcion cargar(ruta: &texto) -> entero
publico funcion cargada(imagen: entero) -> booleano
publico funcion ancho(imagen: entero) -> entero
publico funcion alto(imagen: entero) -> entero
publico funcion color_en(imagen: entero, x: entero, y: entero) -> entero
publico funcion soltar(imagen: entero) -> vacio
publico funcion dibujar(pantalla: entero, imagen: entero, x: entero, y: entero) -> vacio
```

Solo BMP de 24 o 32 bits sin comprimir. `cargar` devuelve 0 si no es válido.

### 8.14 `tensor`

Matrices de enteros de 64 bits. Los valores con decimales se representan **por mil** (2500 =
2,5).

```text
publico funcion ceros(filas: entero, columnas: entero) -> entero
publico funcion lleno(filas: entero, columnas: entero, valor: entero) -> entero
publico funcion identidad(lado: entero) -> entero
publico funcion filas(t: entero) -> entero
publico funcion columnas(t: entero) -> entero
publico funcion leer(t: entero, fila: entero, columna: entero) -> entero
publico funcion poner(t: entero, fila: entero, columna: entero, valor: entero) -> vacio
publico funcion suma(a: entero, b: entero) -> entero
publico funcion resta(a: entero, b: entero) -> entero
publico funcion escalar(a: entero, factor: entero) -> entero
publico funcion producto(a: entero, b: entero) -> entero
publico funcion transpuesta(a: entero) -> entero
publico funcion determinante(a: entero) -> entero
publico funcion suma_total(a: entero) -> entero
publico funcion iguales(a: entero, b: entero) -> booleano
publico funcion liberar(a: entero) -> vacio
publico funcion minimo(t: entero) -> entero
publico funcion maximo(t: entero) -> entero
publico funcion media(t: entero) -> entero
publico funcion varianza(t: entero) -> entero
publico funcion desviacion(t: entero) -> entero
publico funcion normalizar(t: entero) -> entero
publico funcion relu(t: entero) -> entero
publico funcion sigmoides(t: entero) -> entero
publico funcion softmax(t: entero) -> entero
```

### 8.15 `tabla`

Tablas con columnas con nombre, tipo DataFrame. Las columnas son de enteros o de texto; los
decimales van por mil.

```text
publico funcion crear(filas: entero) -> entero
publico funcion columna_entero(t: entero, nombre: &texto) -> entero
publico funcion columna_texto(t: entero, nombre: &texto) -> entero
publico funcion filas(t: entero) -> entero
publico funcion columnas(t: entero) -> entero
publico funcion nombre_columna(t: entero, columna: entero) -> texto
publico funcion poner_entero(t: entero, fila: entero, columna: entero, valor: entero) -> vacio
publico funcion poner_texto(t: entero, fila: entero, columna: entero, valor: &texto) -> vacio
publico funcion valor_entero(t: entero, fila: entero, columna: entero) -> entero
publico funcion valor_texto(t: entero, fila: entero, columna: entero) -> texto
publico funcion suma(t: entero, columna: entero) -> entero
publico funcion media(t: entero, columna: entero) -> entero
publico funcion minimo(t: entero, columna: entero) -> entero
publico funcion maximo(t: entero, columna: entero) -> entero
publico funcion filtrar(t: entero, columna: entero, valor: &texto) -> entero
publico funcion ordenar(t: entero, columna: entero) -> entero
publico funcion contar_por(t: entero, columna: entero) -> entero
publico funcion liberar(t: entero) -> vacio
```

`columna_entero`/`columna_texto` añaden una columna y devuelven su índice. `filtrar`
devuelve una tabla nueva con las filas cuyo valor de texto coincide. `ordenar` devuelve una
tabla nueva ordenada por una columna numérica. `contar_por` devuelve una tabla con las
columnas `valor` y `cuenta`.

### 8.16 `autograd`

Grafo de operaciones con derivadas automáticas. Los nodos son enteros; los valores,
flotantes.

```text
publico funcion dato(valor: flotante) -> entero
publico funcion valor(nodo: entero) -> flotante
publico funcion gradiente(nodo: entero) -> flotante
publico funcion suma(a: entero, b: entero) -> entero
publico funcion resta(a: entero, b: entero) -> entero
publico funcion producto(a: entero, b: entero) -> entero
publico funcion division(a: entero, b: entero) -> entero
publico funcion relu(a: entero) -> entero
publico funcion sigmoides(a: entero) -> entero
publico funcion atras(nodo: entero) -> vacio
publico funcion liberar(nodo: entero) -> vacio
```

Se construye la expresión, se llama `atras(nodo)` y cada nodo tiene su gradiente.

### 8.17 `redes`

Red de una capa oculta que aprende con descenso de gradiente.

```text
publico funcion nueva(entradas: entero, ocultas: entero) -> entero
publico funcion ejemplo(red: entero, x1: flotante, x2: flotante, x3: flotante,
                        x4: flotante, esperado: flotante) -> vacio
publico funcion ejemplos(red: entero) -> entero
publico funcion entrenar(red: entero, vueltas: entero, tasa: flotante) -> vacio
publico funcion predecir(red: entero, x1: flotante, x2: flotante, x3: flotante,
                         x4: flotante) -> flotante
publico funcion error(red: entero) -> flotante
publico funcion guardar(red: entero, ruta: &texto) -> booleano
publico funcion cargar(ruta: &texto) -> entero
publico funcion liberar(red: entero) -> vacio

// Clasificación en varias clases
publico funcion nueva_clases(entradas: entero, ocultas: entero, clases: entero) -> entero
publico funcion ejemplo_clase(red: entero, x1: flotante, x2: flotante, x3: flotante,
                              x4: flotante, cual: entero) -> vacio
publico funcion entrenar_clases(red: entero, vueltas: entero, tasa: flotante) -> vacio
publico funcion predecir_clase(red: entero, x1: flotante, x2: flotante, x3: flotante,
                               x4: flotante) -> entero
publico funcion probabilidad(red: entero, x1: flotante, x2: flotante, x3: flotante,
                             x4: flotante, cual: entero) -> flotante
publico funcion aciertos_clase(red: entero) -> entero
publico funcion total_clase(red: entero) -> entero
publico funcion liberar_clase(red: entero) -> vacio
publico funcion guardar_clase(red: entero, ruta: &texto) -> booleano
publico funcion cargar_clase(ruta: &texto) -> entero
```

- `nueva` admite 1..4 entradas y 1..64 ocultas; devuelve 0 si los tamaños no valen.
- Las entradas no usadas se pasan como `0.0`.
- `predecir` devuelve un valor entre 0 y 1.
- `error` es el error cuadrático medio.
- `guardar` escribe un archivo binario con cabecera `NOOKIRED`, pesos, sesgos y ejemplos.
- `cargar` devuelve la red, o 0 si el archivo no sirve.
- `nueva_clases` admite de 2 a 16 clases.

---

## 9. Diagnósticos

### 9.1 Formato

```text
error[NOK5001]: no se puede usar «letras»: el valor se movió antes
  --> ejemplo.oki:3:15
  |
3 |     imprimir(letras)
  |               ^^^^^^
  |
   = ayuda: se movió a otro sitio: use «&» para prestarlo sin transferirlo
```

- El código es estable y no se renumera.
- `error`, `advertencia`, `nota` y `ayuda` se traducen; el catálogo tiene español e inglés.
- `nooki` imprime texto; la biblioteca tiene un render JSON, pero el CLI no expone `--json`.

### 9.2 Rangos

| Rango | Etapa |
|---|---|
| `NOK0xxx` | el propio compilador / archivos |
| `NOK1xxx` | léxico |
| `NOK2xxx` | sintaxis |
| `NOK3xxx` | nombres, tipos y módulos |
| `NOK4xxx` | tipos y comprobaciones semánticas |
| `NOK5xxx` | propiedad y préstamos |
| `NOK6xxx` | backend de C y ejecución |

### 9.3 Tabla completa (46 códigos)

| Código | Mensaje | Causa típica / arreglo |
|---|---|---|
| `NOK0001` | no se pudo leer el archivo «{archivo}» | ruta incorrecta; revisa el archivo |
| `NOK1001` | «{palabra}» no es una palabra reservada: las palabras clave se escriben sin tilde | quita la tilde: `funcion`, no `función` |
| `NOK1003` | texto sin cerrar | falta la comilla final |
| `NOK1004` | el carácter «{caracter}» no se usa en Nooki | BOM o carácter inválido; guarda UTF-8 sin BOM |
| `NOK1005` | número mal formado | dígitos, base o exponente incorrectos |
| `NOK1006` | secuencia UTF-8 no válida | vuelve a guardar el archivo en UTF-8 |
| `NOK1007` | falta cerrar el carácter literal | cierra `'...'`; los caracteres no están implementados |
| `NOK1008` | comentario sin cerrar | falta `*/` |
| `NOK1009` | la secuencia de escape «{escape}» no existe | usa `\n`, `\t`, `\\` o `\"` |
| `NOK2001` | falta cerrar «{apertura}» | cierra el paréntesis, corchete o llave |
| `NOK2002` | se esperaba {esperado} y se encontró «{encontrado}» | revisa la sintaxis o una palabra reservada |
| `NOK2003` | el programa no declara una función «principal» | añade `funcion principal() -> entero` |
| `NOK2004` | «{construccion}» todavía no está soportado | evita esa construcción |
| `NOK2005` | los rangos no se pueden encadenar | usa un solo `..`/`..=` |
| `NOK3001` | no se ha declarado «{nombre}» | declara la variable o revisa la tilde |
| `NOK3002` | no existe ninguna función «{nombre}» | define la función o importa el módulo |
| `NOK3003` | no existe ningún tipo «{nombre}» | revisa el tipo; muchos reservados no existen |
| `NOK3004` | «{tipo}» no tiene ningún campo «{campo}» | revisa el nombre del campo |
| `NOK3005` | a «{tipo}» le falta el campo «{campo}» | añade todos los campos al construir |
| `NOK3006` | «{tipo}» no tiene ningún miembro «{nombre}» | revisa el método/campo |
| `NOK3007` | «{enumeracion}» no tiene ninguna variante «{variante}» | revisa la variante |
| `NOK3008` | no se encontró el módulo «{modulo}» | revisa `biblioteca/`, `nooki.toml` o la carpeta |
| `NOK3009` | «{simbolo}» es privado del módulo «{modulo}» | marca `publico` en el símbolo |
| `NOK3011` | a «{tipo}» le falta «{metodo}», que exige el rasgo «{rasgo}» | implementa el método |
| `NOK3012` | «asm!» solo se permite dentro de un bloque «inseguro { }» | envuelve el `asm!` en `inseguro` |
| `NOK4001` | se esperaba un valor de tipo «{esperado}» pero se encontró «{encontrado}» | convierte con `como` o cambia el valor |
| `NOK4002` | el argumento «{parametro}» de «{funcion}» espera «{esperado}» y se pasa «{encontrado}» | ajusta el argumento |
| `NOK4003` | «{funcion}» espera {esperados} argumento(s) y se han pasado {dados} | ajusta la aridad |
| `NOK4004` | «{funcion}» debe devolver «{esperado}» y se devuelve «{encontrado}» | ajusta el retorno |
| `NOK4005` | la condición debe ser de tipo «booleano» y es «{tipo}» | compara: `si (n != 0)` |
| `NOK4006` | el «coincide» no cubre todos los casos: añada un caso «_» | añade la variante que falta o `caso _` |
| `NOK4007` | «*» solo se puede aplicar a una «Caja» | usa `*` solo con `Caja<entero>` |
| `NOK4008` | solo se puede prestar una variable que sea una «Caja» | presta una variable prestable; no prestes una expresión |
| `NOK4009` | no se puede escribir a través de un préstamo inmutable: use «&mutable» | cambia el parámetro a `&mutable` |
| `NOK4010` | «{metodo}» no coincide con la firma de «{base}.{metodo}» | copia la firma de la base |
| `NOK4011` | «?» solo se usa en una función que devuelve «Resultado» | devuelve `resultado` o trata el error |
| `NOK4012` | «{metodo}» no coincide con la firma que exige el rasgo «{rasgo}» | copia la firma del rasgo |
| `NOK5001` | no se puede usar «{nombre}»: el valor se movió antes | presta con `&` o reestructura |
| `NOK5002` | un préstamo no se puede guardar en una variable: solo vive durante la llamada | pásalo directo como argumento |
| `NOK5004` | «{nombre}» tiene campos con dueño y no se puede copiar: páselo con «&» o construya otro | presta o construye otro |
| `NOK5006` | no se puede devolver un préstamo a «{nombre}»: es local y muere al salir de la función | devuelve por valor |
| `NOK5007` | no se puede mover ni reasignar «{nombre}»: «{prestamo}» lo está prestando | termina el préstamo antes |
| `NOK5003` | «{nombre}» no puede prestarse mutable y compartido a la vez en la misma llamada | separa las llamadas |
| `NOK6001` | no se encontró un compilador de C; instale gcc, clang o MSVC, o indique uno con la variable NOOKI_CC | instala `cc`/`gcc`/`clang` o define `NOOKI_CC` |
| `NOK6002` | el compilador de C «{compilador}» terminó con el código {codigo} | revisa el C con `--conservar`/`--emitir-c` |
| `NOK6003` | el backend todavía no sabe traducir {construccion} | usa una forma soportada |

> `NOK6001` menciona MSVC en el texto histórico, pero el sondeo real no incluye `cl.exe`.
> En Windows, usa MinGW o Clang.

---

## 10. Herramientas

### 10.1 Comandos

| Comando | Opciones | Qué hace |
|---|---|---|
| `nooki nuevo <nombre>` | — | crea proyecto |
| `nooki construir [carpeta]` | `-o <salida>` | compila el proyecto |
| `nooki compilar <archivo>` | `-o <salida>` / `--salida`, `--emitir-c`, `--conservar` | compila un archivo |
| `nooki ejecutar <archivo>` | las de `compilar` | compila y ejecuta |
| `nooki probar` | — | pruebas del proyecto |
| `nooki verificar <archivo>` | — | comprueba sin generar C |
| `nooki formatear <archivo>...` | `--comprobar` | normaliza sangría |
| `nooki documentar [carpeta\|archivo]` | `--salida` | genera `API.md` |
| `nooki ast <archivo>` | — | árbol de sintaxis |
| `nooki lsp` | — | servidor de lenguaje |
| `nooki limpiar` | — | borra artefactos |
| `nooki ayuda` | — | ayuda |
| `nooki version` | — | versión |

### 10.2 Códigos de salida

| Código | Significado |
|---|---|
| `0` | éxito |
| `64` | uso incorrecto |
| `65` | errores en los datos de entrada |
| `69` | funcionalidad no disponible (reservado) |
| `70` | error interno o pánico en ejecución |

Matices: `nuevo` devuelve `2`/`1`; `probar` devuelve `2`/`1`; `ejecutar` devuelve el código
del programa. Ver la [guía del administrador](GUIA_DEL_ADMINISTRADOR.md#52-códigos-de-salida).

### 10.3 Servidor de lenguaje

| Método | Qué da |
|---|---|
| `initialize` | capacidades: sincronización completa, completion, definition, formatting |
| `textDocument/didOpen`, `didChange` | sincronización de texto completo |
| `textDocument/publishDiagnostics` | diagnósticos con código y rango |
| `textDocument/completion` | palabras, tipos y funciones del documento |
| `textDocument/definition` | funciones, tipos, enumeraciones, variables y parámetros |
| `textDocument/formatting` | documento entero |

No hay módulos, hover, rename, referencias ni workspace symbols.

### 10.4 Resaltado

`editores/` contiene gramáticas para VS Code, Vim/Neovim, JetBrains, Sublime/TextMate/Zed.
El `scopeName` es `source.nooki` y la extensión `.oki`.

---

## 11. Gramática resumida

> Resumen no normativo de las formas que acepta el parser de la 1.0.0. La gramática
> normativa de diseño está en [`spec/gramatica.ebnf`](../../spec/gramatica.ebnf), que incluye
> construcciones todavía no implementadas.

```ebnf
programa        = { elemento } ;
elemento        = usar | declaracion ;
usar            = "usar" identificador ;

declaracion     = [ "publico" | "privado" ]
                  ( funcion | estructura | clase | enumeracion )
                | rasgo
                | externo ;

funcion         = "funcion" identificador [ "<" identificador { "," identificador } ">" ]
                  "(" [ parametros ] ")" [ "->" tipo ] bloque ;
externo         = "externo" funcion_sin_cuerpo ;
parametros      = parametro { "," parametro } ;
parametro       = identificador ":" tipo ;

estructura      = "estructura" identificador [ "<" identificador { "," identificador } ">" ]
                  [ "hereda" tipo ] { "implementa" tipo { "," tipo } }
                  "{" { campo | metodo } "}" ;  (* métodos/herencia: soporte parcial *)
clase           = "clase" identificador [ "<" ... ">" ]
                  [ "hereda" tipo ] { "implementa" tipo { "," tipo } }
                  "{" { campo | metodo } "}" ;
campo           = [ "publico" | "privado" | "protegido" | "interna" ]
                  identificador ":" tipo ;
metodo          = [ "publico" | "privado" ] "funcion" identificador
                  "(" [ parametros ] ")" [ "->" tipo ] bloque ;

rasgo           = "rasgo" identificador "{" { funcion_sin_cuerpo } "}" ;
enumeracion     = "enumeracion" identificador "{" { variante } "}" ;
variante        = identificador [ "(" [ campo_variante { "," campo_variante } ] ")" ] ;
campo_variante  = identificador ":" tipo ;

tipo            = [ "&" [ "mutable" ] ] identificador
                  [ "<" tipo { "," tipo } ">" ] ;

sentencia       = declaracion_variable | asignacion | si | mientras | hacer | para
                | bucle | romper | continuar | retornar | coincide
                | inseguro | asm | bloque | expresion ;

declaracion_variable = ( "variable" | "constante" ) identificador
                       [ ":" tipo ] "=" expresion ;
asignacion      = expresion ( "=" | "+=" | "-=" | "*=" | "/=" | "%="
                | "&=" | "|=" | "^=" | "<<=" | ">>=" ) expresion ;
si              = "si" "(" expresion ")" bloque
                  { "sinosi" "(" expresion ")" bloque }
                  [ "sino" bloque ] ;
mientras        = "mientras" "(" expresion ")" bloque ;
hacer           = "hacer" bloque "mientras" "(" expresion ")" ;
para            = "para" "(" identificador "en" expresion
                  [ "paso" expresion ] ")" bloque ;
bucle           = "bucle" bloque ;
retornar        = "retornar" [ expresion ] ;
coincide        = "coincide" "(" expresion ")" "{" { caso } "}" ;
caso            = "caso" patron "=>" ( bloque | sentencia ) ;
patron          = "_" | literal | identificador [ "(" identificador ")" ]
                | identificador "." identificador ;
inseguro        = "inseguro" bloque ;
asm             = "asm!" "(" texto ")" ;
bloque          = "{" { sentencia } "}" ;
expresion       = ... ;  (* precedencia de la tabla §3.1 *)
```

---

## 12. Apéndices

### 12.1 Lo que no existe (o no está terminado)

| No existe / no funciona | Alternativa |
|---|---|
| Excepciones | `resultado`, `?`, `coincide` |
| Conversión implícita | `como` |
| `sino si` | `sinosi` |
| Interpolación `{}` | concatenar con `+` |
| Escapes interpretados en la salida | texto multilínea `"""` |
| Literales de carácter `'a'` | no soportados por el backend |
| `"" + 42` | no hay número→texto |
| `??`, `?.` | backend no los traduce |
| `Caja<texto>` | clase o `Vector<texto>` |
| Métodos/herencia en estructuras | soporte parcial; usa clases |
| `Vector<flotante>` | `Vector<entero>` o clase |
| Tuplas, arreglos, conjuntos, opcionales | — |
| Sobrecarga, parámetros por defecto | nombres distintos |
| Cierres, funciones de primera clase | — |
| `interfaz`, `tipo`, `modulo`, `importar`, `exportar`, `implementar` | `usar` y rasgos |
| `constructor`, `destructor`, `propiedad`, `static` | construcción con `nuevo` |
| Macros, `async`, `await` | reservadas |
| Herencia múltiple de clases | varios rasgos |
| Recolector de basura | propiedad estática |
| `inseguro` como sandbox | solo desactiva aritmética |
| UI macOS, PNG, recolocación, cursor | ver `docs/INTERFAZ.md` |
| LSP con módulos, hover, rename | limitación del servidor |
| `nooki documentar` con métodos | solo funciones/tipos/enums |
| `nooki probar`/`verificar` con dependencias | usa `construir` |

### 12.2 Tabla rápida de sintaxis

```nooki
// Programa mínimo
funcion principal() -> entero {
    imprimir("Hola")
    retornar 0
}

// Variables y tipos
variable n: entero = 42
constante PI: flotante = 3.1416
variable texto = "hola"

// Control
si (n > 0) { imprimir("positivo") } sino { imprimir("no positivo") }
mientras (n > 0) { n = n - 1 }
para (i en 0..10 paso 2) { imprimir(i) }
para (x en vector) { imprimir(x) }
coincide (n) {
    caso 0 => imprimir("cero")
    caso _ => imprimir("otro")
}

// Funciones
funcion sumar(a: entero, b: entero) -> entero {
    retornar a + b
}

// Estructura
estructura punto { publico x: entero  publico y: entero }
variable p = punto { x: 1, y: 2 }

// Clase y herencia
clase base { publico funcion f() -> texto { retornar "base" } }
clase hija hereda base {
    publico funcion f() -> texto { retornar super.f() + " hija" }
}
variable h = nuevo hija { }

// Rasgo
rasgo Saluda { funcion saludar() -> texto }
clase persona implementa Saluda {
    publico funcion saludar() -> texto { retornar "hola" }
}

// Enumeración con datos
enumeracion opcion { Si(valor: entero)  No }
variable o = Si(1)

// Resultado
usar resultado
funcion f() -> resultado { retornar Ok(1) }
```

### 12.3 Glosario

| Término | Significado |
|---|---|
| dueño | variable responsable de liberar un valor |
| préstamo | referencia temporal `&`/`&mutable` |
| mover | transferir la propiedad; el origen queda inválido |
| monomorfización | una versión de código por instanciación genérica |
| pánico | parada en ejecución con código 70 |
| `NOK` | prefijo de los códigos de diagnóstico |
| `.oki` | extensión de fuente |
| `nooki.toml` | manifiesto del proyecto |
| `biblioteca/` | módulos propios del proyecto |
| `.oki.esperado` | salida esperada de una prueba |
