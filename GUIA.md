# Guía de Nooki

Un recorrido de cero a un programa con módulos, en doce pasos. **Todos los ejemplos de esta guía se
compilan y se ejecutan**: la salida que ves es la que da el compilador, copiada de una ejecución real.

---

## Antes de empezar

Nooki es un lenguaje compilado, con sintaxis en español, tipado comprobado antes de ejecutar y
memoria determinista: no hay recolector de basura y no hay excepciones.

Necesitas el compilador `nooki` en el `PATH` y un compilador de C (`cc`, `gcc`, `clang` o MSVC), que
Nooki usa como backend. Para compilar Nooki desde el código fuente, mira [LEEME](LEEME.md).

Todo programa Nooki vive en un archivo `.oki` y necesita una función `principal`.

---

## 1. Hola, mundo

```aura
funcion principal() -> entero {
    imprimir("¡Hola, Nooki!")
    retornar 0
}
```

```bash
$ nooki ejecutar hola.oki
¡Hola, Nooki!
```

`principal` devuelve un entero, que es el código de salida del programa.

---

## 2. Variables, constantes y cuentas

```aura
funcion principal() -> entero {
    variable nombre = "Nooki"
    constante anio = 2026
    imprimir("Hola, " + nombre)
    imprimir(anio)
    imprimir(7 * 6)
    retornar 0
}
```

```
Hola, Nooki
2026
42
```

- `variable` se puede cambiar; `constante` no.
- El tipo se deduce del valor.
- `+` concatena textos o suma enteros, según lo que haya a los lados.

---

## 3. Decisiones y bucles

```aura
funcion principal() -> entero {
    variable total = 0
    para (i en 1..=5) {
        si (i % 2 == 0) {
            total = total + i
        }
    }
    imprimir(total)

    variable intentos = 0
    mientras (intentos < 3) {
        intentos = intentos + 1
    }
    imprimir(intentos)
    retornar 0
}
```

```
6
3
```

- `1..=5` incluye el 5; `1..5` no.
- La condición de un `si` **tiene que ser booleana**: `si (1)` es error de compilación.

---

## 4. Funciones

```aura
funcion doblar(n: entero) -> entero {
    retornar n * 2
}

funcion saludar(nombre: texto) -> texto {
    retornar "Hola, " + nombre
}

funcion principal() -> entero {
    imprimir(doblar(21))
    imprimir(saludar("Nooki"))
    retornar 0
}
```

```
42
Hola, Nooki
```

Los parámetros llevan su tipo. El compilador comprueba que las llamadas encajan antes de generar
código.

---

## 5. Texto que se analiza

```aura
funcion principal() -> entero {
    variable linea = "nombre:Ana"
    variable corte = linea.buscar(":")
    imprimir(linea.subtexto(0, corte))
    imprimir(linea.subtexto(corte + 1, 100))
    imprimir(linea.empieza_con("nombre"))
    retornar 0
}
```

```
nombre
Ana
1
```

- `buscar` devuelve la posición o `-1`.
- `subtexto` devuelve una copia **con dueño**: se libera sola al salir del ámbito.
- Un `booleano` impreso se ve como `1` o `0`.

---

## 6. Colecciones

```aura
funcion principal() -> entero {
    variable numeros = Vector<entero>.nuevo()
    numeros.agregar(3)
    numeros.agregar(7)

    variable suma = 0
    para (n en numeros) {
        suma = suma + n
    }
    imprimir(suma)

    variable edades = Mapa<texto, entero>.nuevo()
    edades.insertar("Ana", 30)
    imprimir(edades.obtener("Ana"))
    imprimir(edades.largo())
    retornar 0
}
```

```
10
30
1
```

`para (n en coleccion)` recorre sin exponer el índice. `insertar` **mueve** la clave: el mapa pasa a
ser su dueño.

---

## 7. Objetos

```aura
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
    imprimir(c.valor)
    retornar 0
}
```

```
12
```

- `nuevo` crea el objeto en el montículo; la variable es su **dueña** y lo libera al salir.
- `este` es el objeto dentro de sus métodos.
- Una `estructura` es un dato por valor; una `clase` es un objeto con identidad.

---

## 8. Herencia y polimorfismo

```aura
clase figura {
    publico nombre: texto
    publico funcion area() -> entero { retornar 0 }
}

clase cuadrado hereda figura {
    publico lado: entero
    publico funcion area() -> entero { retornar este.lado * este.lado }
}

funcion principal() -> entero {
    variable f: figura = nuevo cuadrado { nombre: "c", lado: 4 }
    imprimir(f.area())
    imprimir(f.nombre)
    retornar 0
}
```

```
16
c
```

`f` es de tipo `figura` pero apunta a un `cuadrado`: `f.area()` despacha a la implementación del
cuadrado. Los campos heredados se nombran igual al construir.

---

## 9. Errores como valores

```aura
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

```
5
no se puede dividir por cero
```

No hay excepciones: un fallo es un valor. El módulo `resultado` viene con el compilador y está escrito
en Nooki. Dentro de una función que devuelve un resultado, `?` propaga el error:

```aura
funcion mitad_y_mas(a: entero, b: entero) -> resultado {
    variable mitad = dividir(a, 2)?
    retornar Ok(mitad + b)
}
```

---

## 10. Genéricos de tipos

```aura
estructura par<T> {
    publico primero: T
    publico segundo: T
}

funcion principal() -> entero {
    variable p = par<entero> { primero: 3, segundo: 4 }
    imprimir(p.primero + p.segundo)

    variable t = par<texto> { primero: "a", segundo: "b" }
    imprimir(t.primero + t.segundo)
    retornar 0
}
```

```
7
ab
```

Un mismo tipo sirve para varios: el compilador genera una estructura por combinación, sin despacho en
ejecución.

---

## 11. Rasgos: polimorfismo sin parentesco

```aura
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

```
Hola, soy Ana
Bip. Unidad R2
```

`persona` y `robot` no comparten padre, pero ambos cumplen `Saluda`, y `presentar` los trata igual:
cada uno responde con su implementación. Si a una clase le falta un método del rasgo, no compila.

Fíjate en la diferencia con la herencia del paso 8: allí había una jerarquía; aquí solo un contrato.

---

## 12. La biblioteca estándar

Nooki trae cinco módulos escritos en Nooki. Se usan con `usar`, sin copiar archivos:

```aura
usar listas
usar fechas
usar textos

funcion principal() -> entero {
    variable notas = Vector<entero>.nuevo()
    notas.agregar(8)
    notas.agregar(6)
    notas.agregar(10)

    imprimir(listas.suma(&notas))
    imprimir(listas.promedio(&notas))
    imprimir(listas.mayor(&notas))

    variable coma = ", " + ""
    variable partes = Vector<texto>.nuevo()
    partes.agregar("uno" + "")
    partes.agregar("dos" + "")
    imprimir(textos.unir(&partes, coma))

    si (fechas.bisiesto(2024)) {
        imprimir("2024 es bisiesto")
    }
    retornar 0
}
```

```
24
8
10
uno, dos
2024 es bisiesto
```

| Módulo | Trae |
|---|---|
| `resultado` | `Ok(valor)` y `Error(mensaje)` |
| `matematicas` | `maximo`, `minimo`, `absoluto`, `cuadrado` |
| `textos` | `unir`, `repetir`, `contar` |
| `listas` | `suma`, `mayor`, `menor`, `promedio`, `contiene` |
| `fechas` | `bisiesto`, `dias_del_mes`, `dias_del_anio` |

Un proyecto puede **sustituir** un módulo con el suyo: un archivo `biblioteca/<nombre>.oki` tiene
prioridad sobre el que viaja en el compilador.

---

## Un proyecto de verdad

```bash
$ nooki nuevo agenda
$ cd agenda
$ nooki construir
$ ./principal
Hola, Nooki
```

`nooki nuevo` crea `principal.oki` y un módulo en `biblioteca/`. Un módulo se importa con `usar` y se
usa con `modulo.simbolo`:

```aura
usar herramientas

funcion principal() -> entero {
    imprimir(herramientas.saludo("Nooki"))
    retornar 0
}
```

Para probar:

```bash
$ nooki probar
ok    pruebas\saludo.oki
todas las pruebas pasaron
```

Cada `pruebas/<nombre>.oki` se compila y se ejecuta, y su salida se compara con
`pruebas/<nombre>.oki.esperado`.

---

## Memoria: lo que hace distinto a Nooki

Nooki no tiene recolector de basura. Cada valor con dueño (`texto`, `Vector`, `Mapa`, una clase) se
libera al salir del ámbito, y el compilador **impide** los usos peligrosos:

```aura
variable letras = "uno" + "!"
variable copia = letras      // mueve: letras deja de ser válida
imprimir(letras)             // error NOK5001
```

Para leer sin mover, se presta:

```aura
funcion largura(t: &texto) -> entero {
    retornar t.largo()
}

variable letras = "uno" + "!"
imprimir(largura(&letras))
imprimir(letras)             // sigue siendo válida
```

---

## Cuando algo falla

Los diagnósticos están en español y dicen dónde, qué y cómo arreglarlo:

```
error[NOK5001]: no se puede usar «letras»: el valor se movió antes
  --> ejemplo.oki:3:15
  |
3 |     imprimir(letras)
  |               ^^^^^^
  |
   = ayuda: se movió a otro sitio: use «&» para prestarlo sin transferirlo
```

Los códigos empiezan por `NOK` y están catalogados en la [especificación](spec/especificacion-v0.1.md#1810-módulos),
sección 18.

---

## Siguientes pasos

- La [especificación](spec/especificacion-v0.1.md) es el documento normativo.
- Las [decisiones de arquitectura](docs/adr/) explican por qué el lenguaje es como es.
- El [plan de sprints](PLAN_IMPLEMENTACION_NOOKI_SPRINTS.md) cuenta lo que falta.
- Los [ejemplos de conformidad](pruebas/conformidad/valido) son programas que se compilan y ejecutan
  en cada compilación del compilador: son la mejor referencia de lo que funciona.
