# Especificación del lenguaje NOOKI v1.0

| Campo | Valor |
|---|---|
| **Estado** | Borrador normativo — v0.1 ampliada en la v0.7 (§18) |
| **Fecha** | 2026-11-09 |
| **Extensión de archivo** | `.oki` |
| **Gramática** | [`gramatica.ebnf`](gramatica.ebnf) (normativa) |
| **Decisiones** | [`DECISIONES.md`](../DECISIONES.md) · [`docs/adr/`](../docs/adr/) |
| **Alcance** | Define el lenguaje que debe implementar un compilador que se declare "Nooki 0.1" |

> **Cómo leer este documento.** Las secciones marcadas **NORMATIVO** son obligatorias para cualquier implementación. Las marcadas **INFORMATIVO** explican el porqué y pueden omitirse sin afectar la conformidad.

---

## 1. Alcance y no-objetivos

**NORMATIVO.** La v0.1 define: léxico, sintaxis, sistema de tipos, semántica de propiedad y préstamos, OOP, modelo de errores, módulos, atributos y FFI con C.

**No forman parte de la v0.1:** `async`/`await` (reservadas), macros procedurales, clases de C++ en FFI, herencia múltiple, `no_std` completo (llega en v0.5), GPU y UI (son paquetes).

---

## 2. Modelo léxico

### 2.1 Codificación — NORMATIVO

- El fuente es **UTF-8**. Un BOM al inicio se ignora.
- Saltos de línea aceptados: `\n`, `\r\n`, `\r`. El compilador los normaliza a `\n`.
- Líneas y columnas se cuentan en **puntos de código**, no en bytes.

### 2.2 Identificadores — NORMATIVO

```
identificador = letra , { letra | digito | "_" }
letra         = a..z | A..Z | á é í ó ú ü ñ | Á É Í Ó Ú Ü Ñ
```

- Se **normalizan a NFC** antes de compararse (ADR-008).
- Distinguen mayúsculas: `valor` ≠ `Valor`.
- No pueden comenzar con dígito ni `_` solo.
- `_` aislado es el patrón comodín, no un identificador.

### 2.3 Palabras reservadas — NORMATIVO

**Tipos (21):** `entero` `entero8` `entero16` `entero32` `entero64` `entero128` `natural` `natural8` `natural16` `natural32` `natural64` `natural128` `flotante` `flotante32` `booleano` `caracter` `texto` `vacio` `byte` `tamano` `tamano_signado`

**Literales (3):** `verdadero` `falso` `nulo`

**Visibilidad y modificadores (6):** `publico` `privado` `protegido` `interna` `estatico` `abstracta`

**Declaraciones (14):** `variable` `constante` `funcion` `clase` `estructura` `rasgo` `interfaz` `enumeracion` `tipo` `modulo` `importar` `exportar` `implementar` `externo`

**OOP (11):** `hereda` `implementa` `nuevo` `este` `Este` `super` `constructor` `destructor` `propiedad` `obtener` `establecer`

**Control de flujo (16):** `si` `sinosi` `sino` `mientras` `para` `en` `paso` `hacer` `bucle` `romper` `continuar` `retornar` `coincide` `caso` `como` `donde`

**Memoria y enlace (3):** `inseguro` `mutable` `asm`

**Errores (4):** `intentar` `atrapar` `finalmente` `lanzar`

**Reservadas para el futuro (6):** `asincrono` `esperar` `macro` `union` `mixin` `es`

> **Total: 63 palabras clave + 21 tipos.** Ninguna lleva tilde. `función`, `tamaño` o `sí` producen error con sugerencia de la forma correcta (ADR-008).

### 2.4 Literales — NORMATIVO

| Tipo | Forma | Notas |
|---|---|---|
| Entero | `42`, `1_000_000`, `0xFF`, `0o755`, `0b1010` | Sin sufijos de tipo; el literal es polimórfico |
| Flotante | `3.14`, `1e-9`, `2.5e10` | Siempre `flotante64` si no hay contexto |
| Texto | `"hola"`, `"año"`, `"línea\n"` | UTF-8; tipo `&texto` |
| Texto multilínea | `"""..."""` | Conserva saltos de línea |
| Carácter | `'a'`, `'ñ'`, `'\n'`, `'\u{1F600}'` | Un escalar Unicode |
| Booleano | `verdadero`, `falso` | |
| Nulo | `nulo` | Solo válido como variante de `opcional<T>` |

**Interpolación de texto — NORMATIVO.** Dentro de un literal de texto, `{expresión}` se sustituye por su representación textual. `{{` y `}}` producen llaves literales.

```nooki
variable nombre = "Ana"
imprimir("Hola, {nombre}. Tienes {2 + 2} mensajes.")
```

**Desambiguación `caracter` / *lifetime*** (ADR-013): tras `'`, si el siguiente carácter es `\` o el escalar va seguido de `'`, es un carácter; si es un identificador, es un *lifetime*.

**Desambiguación de enteros:** si empieza por `0x`, `0o` o `0b`, se interpreta en esa base aunque el primer dígito sea `0`.

### 2.5 Terminación de sentencias — NORMATIVO

El **salto de línea termina la sentencia**. Además se acepta `;` como separador explícito (el formateador los elimina).

El salto de línea **no** la termina cuando:

1. la línea termina en un operador binario (`+ - * / && || = => ..`), en `,` o en `->`;
2. hay un delimitador `(` o `[` sin cerrar;
3. la línea termina dentro de un comentario de bloque abierto.

Dentro de `(` y `[` los saltos de línea se ignoran. Dentro de `{` de un bloque, separan sentencias. En literales de estructura, `coincide` y atributos, el separador es la coma.

### 2.6 Comentarios — NORMATIVO

```nooki
// línea
/* bloque, anidable /* anidado */ */
/// documentación del elemento siguiente
//! documentación del módulo
```

---

## 3. Estructura del programa y módulos — NORMATIVO

- Un archivo `.oki` es un módulo. Su ruta define su nombre: `red/tcp.oki` → `red.tcp`.
- La declaración `modulo` es **opcional** y sirve para fijar el nombre lógicamente cuando la ruta no lo refleja.
- Todo programa necesita **un** módulo con una función `principal`.
- Importación con rutas separadas por punto; llaves para varios símbolos:

```nooki
importar sistema.texto
importar matematicas.{seno, coseno, PI}
importar red.tcp como transporte
exportar funcion calcular() -> entero { retornar 42 }
```

- La visibilidad por defecto es `privado`. `interna` = visible en el módulo y sus hijos. `publico` = visible desde fuera.
- Un `importar` que no se usa es **advertencia**, no error.

---

## 4. Sistema de tipos — NORMATIVO

### 4.1 Primitivos

| Tipo | Ancho | Rango / valor |
|---|---|---|
| `entero` | 64 bits | −2⁶³ … 2⁶³−1 |
| `entero8/16/32/64/128` | 8…128 | con signo, complemento a dos |
| `natural` | 64 bits | 0 … 2⁶⁴−1 |
| `natural8/16/32/64/128` | 8…128 | sin signo |
| `byte` | 8 | alias de `natural8` |
| `flotante` | 64 | IEEE-754 binario64 |
| `flotante32` | 32 | IEEE-754 binario32 |
| `booleano` | 1 byte lógico | `verdadero` / `falso` |
| `caracter` | 32 | escalar Unicode válido (excluye subrogados) |
| `texto` | — | secuencia UTF-8 con propiedad, en montículo |
| `vacio` | 0 | ausencia de valor |
| `tamano` | puntero | entero sin signo del tamaño de un puntero |
| `tamano_signado` | puntero | entero con signo del tamaño de un puntero |

`texto` y `vector<T>` viven en el montículo. Una **referencia a texto** es `&texto`; el literal `"hola"` es un `&texto` estático.

### 4.2 Tipos compuestos

| Forma | Ejemplo |
|---|---|
| Tupla | `(entero, texto, booleano)` |
| Arreglo de tamaño fijo | `arreglo<entero, 10>` |
| Vector dinámico | `vector<texto>` |
| Mapa | `mapa<texto, entero>` |
| Conjunto | `conjunto<texto>` |
| Opcional | `opcional<entero>` o `entero?` |
| Resultado | `resultado<texto, error_io>` |
| Referencia | `&texto`, `&mutable texto` |
| Puntero crudo | `*texto`, `*mutable texto` (solo en `inseguro`) |
| Función | `funcion(entero, entero) -> entero` |
| Cierre | `funcion(entero) -> entero` (mismo tipo) |

### 4.3 Opcional y Resultado

```nooki
variable a: opcional<entero> = algo(5)
variable b: entero? = nulo
variable c: resultado<texto, error_io> = ok("datos")
variable d = error(error_io.crear("no existe"))
```

- `opcional<T>` tiene las variantes `algo(T)` y `nulo`.
- `resultado<T, E>` tiene las variantes `ok(T)` y `error(E)`.
- El operador `?` **solo** se aplica a `resultado`.
- `nulo` no es un tipo: es una variante y un patrón.

### 4.4 Genéricos — NORMATIVO

```nooki
funcion maximo<T: comparable>(a: T, b: T) -> T
clase Caja<T> { privado valor: T }
funcion procesar<T, U>(x: T) -> U donde T: serializable, U: deserializable
```

- Los argumentos de tipo se **infieren siempre** en expresiones (sin *turbofish*, ADR-013).
- La monomorfización es la estrategia de la v0.1: cada instanciación genera código.

### 4.5 Inferencia y conversiones

- Se infiere el tipo de variables locales, retornos y cierres.
- `expr como tipo` es una conversión **explícita y truncante** (puede perder información; el compilador lo advierte si es evidente).
- `convertir<entero>(x)` devuelve `resultado<entero, error_conversion>` y es la forma segura.
- **No hay conversiones implícitas** entre tipos numéricos, ni de `texto` a número. Nunca.

### 4.6 Restricción de la v0.1

No se permite **doble referencia** (`&&T`, `&mutable &mutable T`): el lexer produce `&&` como operador lógico. Se resuelve con un alias de tipo.

---

## 5. Declaraciones — NORMATIVO

```nooki
variable contador = 0
variable edades: vector<entero> = []
constante MAXIMO: entero = 1000
variable (x, y) = obtener_coordenadas()      // desestructuración

tipo identificador = natural64
tipo par<T> = (T, T)
```

- `variable` es mutable; `constante` es inmutable y se verifica en compilación.
- Sin tipo explícito se infiere; si no se puede, es error.
- Las constantes de módulo deben ser **evaluables en compilación**.

---

## 6. Sentencias — NORMATIVO

```nooki
si (edad >= 18) { imprimir("mayor") }
sinosi (edad >= 13) { imprimir("adolescente") }
sino { imprimir("menor") }

mientras (activo) { actualizar() }

para (i en 0..10) { imprimir(i) }            // 0..9
para (i en 0..=10) { imprimir(i) }           // 0..10
para (i en 0..100 paso 5) { imprimir(i) }
para (elemento en coleccion) { imprimir(elemento) }

hacer { intentar_una_vez() } mientras (quedan)

bucle { si (listo) { romper } }

coincide (valor) {
    caso 1 => imprimir("uno")
    caso 2 | 3 => imprimir("dos o tres")
    caso n si n > 10 => imprimir("grande: {n}")
    caso _ => imprimir("otro")
}
```

- `si` es también **expresión**, siempre con bloques: el valor de la rama es el valor de su bloque.
  `variable estado = si (activo) { "activo" } sino { "inactivo" }`
- **La condición de `si`, `mientras` y `hacer` debe ser de tipo `booleano`.** Un entero no vale como condición: `si (1)` es error `NOK4005`. Es la diferencia entre un lenguaje en el que `si (x = 1)` compila y uno en el que no.
- `romper` y `continuar` afectan al bucle más interno.
- Un bloque `{ ... }` como última expresión de una función produce su valor de retorno.

---

## 7. Expresiones — NORMATIVO

### 7.1 Precedencia (de mayor a menor)

| Nivel | Operadores | Asociatividad |
|---|---|---|
| 0 | `()` `.` `?.` `[]` `?` (postfijos) | izquierda |
| 1 | `!` `-` `+` `*` `&` `&mutable` `nuevo` `\|…\|` (prefijos) | derecha |
| 1.5 | `como` | izquierda |
| 2 | `**` | **derecha** |
| 3 | `*` `/` `%` | izquierda |
| 4 | `+` `-` | izquierda |
| 5 | `..` `..=` | no asociativo |
| 6 | `<<` `>>` | izquierda |
| 7 | `&` (bit a bit) | izquierda |
| 8 | `^` | izquierda |
| 9 | `\|` (bit a bit) | izquierda |
| 10 | `==` `!=` `<` `>` `<=` `>=` | no asociativo |
| 11 | `&&` | izquierda, cortocircuito |
| 12 | `\|\|` | izquierda, cortocircuito |
| 13 | `??` | derecha |
| 14 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | derecha |

> `&&` y `||` tienen **cortocircuito obligatorio**: el operando derecho solo se evalúa si hace falta (ADR-013). `??` evalúa el derecho solo si el izquierdo es `nulo` o `error`.

**Dos decisiones de esta tabla que conviene tener presentes:**

1. **Los operadores de bits se unen más fuerte que las comparaciones.** `a & b == c` significa `(a & b) == c`, al contrario que en C (donde significa `a & (b == c)`). Es la misma elección que hace Python: el orden de C es una fuente clásica de errores y casi nadie escribe `a & (b == c)` queriendo.
2. **Los prefijos se unen más fuerte que `**`.** `-2 ** 2` significa `(-2) ** 2`, no `-(2 ** 2)` como en Python o Ruby. Está registrado como asunto **A-08** para revisar antes de la v1.0: es la clase de detalle que conviene decidir con usuarios reales delante.
3. **Los rangos no son asociativos:** `a..b..c` es un **error** (NOK2005), no una cadena implícita.

### 7.2 Operadores especiales

| Operador | Semántica |
|---|---|
| `?` (postfijo) | Propaga el error de un `resultado` (ADR-004) |
| `?.` | Si el izquierdo es `nulo`, el resultado es `nulo`; si no, accede al miembro |
| `??` | Valor por defecto si el izquierdo es `nulo` |
| `como` | Conversión explícita truncante |

> `es` está **reservada** para la comprobación de tipo en tiempo de ejecución, que no forma parte de la v0.1: el despacho se hace con `coincide` sobre variantes y con `@virtual`.

> **v0.1:** las comparaciones (`==`, `!=`, `<`, `>`, `<=`, `>=`) solo se admiten entre valores de tipo `entero`. Comparar textos exige el tipo `texto` completo (longitud y contenido), que llega con la librería estándar.

### 7.3 Cierres

```nooki
variable doble = |x| x * 2
variable sumar = |a: entero, b: entero| -> entero { a + b }
```

Un cierre captura por referencia cuando es posible y por movimiento cuando el valor se usa después de la vida del cierre. `|` y `||` se distinguen del operador lógico por la posición (prefijo vs infijo).

### 7.4 Construcción de valores

```nooki
variable p = punto { x: 1.0, y: 2.0 }        // estructura por valor
variable c = nuevo circulo(5.0, "rojo")      // clase por referencia
variable f = figura.circulo { radio: 5.0 }   // variante de enumeración
variable q = punto { x: 3.0, ..p }           // actualización funcional
```

**Restricción:** un literal de estructura o variante no puede aparecer directamente en la condición de `si`, `mientras` o `para` sin paréntesis (la `{` se confundiría con el bloque).

---

## 8. Patrones — NORMATIVO

Admitidos: `_`, literales, `nulo`, enlaces, tuplas, `ruta { campo }`, `ruta(variante)`, alternancia `|`, guardas `si`, y referencias `&patrón`.

`coincide` debe ser **exhaustivo**: el compilador exige cubrir todos los casos o un `_`. Un `coincide` sin `_` que no sea exhaustivo es **error de compilación**, no advertencia.

---

## 9. Programación orientada a objetos — NORMATIVO

### 9.1 Clases

```nooki
clase animal {
    protegido nombre: texto
    privado edad: entero

    constructor(nombre: texto, edad: entero) {
        este.nombre = nombre
        este.edad = edad
    }

    constructor.desde_texto(linea: texto) { /* constructor alternativo */ }

    @virtual
    publico funcion hablar() -> texto { retornar "..." }

    publico estatico funcion crear_anonimo() -> animal { retornar nuevo animal("anónimo", 0) }

    destructor() { imprimir("adiós {este.nombre}") }
}
```

### 9.2 Herencia y virtualidad (ADR-009)

```nooki
clase perro hereda animal implementa comparable<perro> {
    privado raza: texto

    constructor(nombre: texto, edad: entero, raza: texto) {
        super(nombre, edad)
        este.raza = raza
    }

    @sobrescribir
    publico funcion hablar() -> texto { retornar "¡Guau!" }
}
```

- Un método solo se sobrescribe si el padre lo declara `@virtual`.
- `@sobrescribir` es **obligatorio**; si falta o sobra, es error.
- `@final` impide sobrescribir; `@sellada` impide heredar.
- Una clase con métodos `abstracta` debe declararse `abstracta` y no se puede instanciar.
- Solo hay **herencia simple**.

### 9.3 Propiedades

```nooki
publico propiedad nombre_completo: texto {
    obtener { retornar "{este.nombre} {este.apellido}" }
}
publico propiedad activo: booleano {
    obtener { retornar este.edad > 0 }
    establecer(valor) { este.edad = si (valor) 1 sino 0 }
}
```

### 9.4 Rasgos e interfaces (ADR-013: sinónimos)

```nooki
rasgo serializable {
    funcion a_json() -> texto
    funcion desde_json(entrada: texto) -> resultado<Este, error_parseo>
    funcion imprimir() { imprimir(este.a_json()) }     // método por defecto
}

interfaz comparable<T> {
    funcion comparar_con(otro: &T) -> entero
    funcion es_mayor_que(otro: &T) -> booleano { retornar este.comparar_con(otro) > 0 }
}

implementar serializable para perro {
    funcion a_json() -> texto { retornar "{\"raza\": \"{este.raza}\"}" }
    funcion desde_json(entrada: texto) -> resultado<Este, error_parseo> { /* ... */ }
}
```

- La implementación debe cubrir **todos** los métodos sin cuerpo del rasgo.
- Regla de coherencia: solo se puede implementar un rasgo para un tipo si el rasgo **o** el tipo son del módulo actual (regla de huérfanos).

### 9.5 Sobrecarga de operadores

Se hace implementando rasgos de `core.operadores`:

| Rasgo | Operador | Método |
|---|---|---|
| `suma` | `+` | `sumar(otro: &Este) -> Este` |
| `resta` | `-` | `restar(otro: &Este) -> Este` |
| `multiplicacion` | `*` | `multiplicar(otro: &Este) -> Este` |
| `division` | `/` | `dividir(otro: &Este) -> Este` |
| `potencia` | `**` | `potenciar(otro: &Este) -> Este` |
| `negacion` | `-` (unario) | `negar() -> Este` |
| `igualdad` | `==`, `!=` | `es_igual(otro: &Este) -> booleano` |
| `orden` | `<` `>` `<=` `>=` | `comparar_con(otro: &Este) -> entero` |
| `indice<K>` | `[i]` | `indice(i: K) -> &Este` |
| `indice_mutable<K>` | `[i] = v` | `indice_mutable(i: K) -> &mutable Este` |

---

## 10. Memoria — NORMATIVO

### 10.1 Propiedad

1. Todo valor tiene **un único propietario**.
2. Asignar una `estructura` copiable **copia**; asignar una `clase` o un tipo no copiable **mueve** y deja el origen inutilizable.
3. Al salir del ámbito, el propietario libera el valor (`destructor` si existe).
4. `.clonar()` produce una copia profunda explícita.

### 10.2 Préstamos

1. Se permite **cualquier número** de `&T` simultáneos.
2. Se permite **un solo** `&mutable T` a la vez, y **ningún** `&T` concurrente con él.
3. Una referencia nunca sobrevive a su propietario.

```nooki
variable saludo = "hola"
variable r1: &texto = &saludo
variable r2: &texto = &saludo        // correcto
variable r3: &mutable texto = &mutable saludo   // correcto solo si r1 y r2 ya no se usan
```

### 10.3 Tiempos de vida

Se infieren. Reglas de elisión:

1. Cada parámetro de referencia sin anotar recibe un *lifetime* propio.
2. Si hay **un** *lifetime* de entrada, la salida usa ese.
3. Si el método tiene receptor `este`, la salida usa el *lifetime* de `este`.

Anotación explícita cuando sea necesario:

```nooki
funcion el_mas_largo<'v>(a: &'v texto, b: &'v texto) -> &'v texto
```

### 10.4 Memoria insegura

`inseguro { }` habilita punteros crudos, `asm!`, FFI y manipulación directa. No se hereda a bloques anidados de forma implícita.

```nooki
inseguro {
    variable registro: *mutable natural32 = 0xFF00_0000 como *mutable natural32
    *registro = 1
}
```

---

## 11. Errores y pánico — NORMATIVO

Ver ADR-004.

```nooki
funcion leer_config(ruta: texto) -> resultado<config, error_io> {
    variable archivo = abrir(ruta)?
    variable datos = archivo.leer_todo()?
    retornar ok(analizar(datos))
}

intentar {
    procesar_peticion()
} atrapar (e) {
    registrar("fallo interno: {e}")
} finalmente {
    limpiar()
}
```

- `?` exige que el error del llamador sea el mismo tipo o implemente `desde_error<F>`.
- `lanzar "mensaje"` provoca un **pánico**; se usa para estados irrecuperables.
- Un pánico **no** cruza una frontera `externo "C"`: el proceso aborta con mensaje.

### 11.1 Códigos de diagnóstico — NORMATIVO

Todo diagnóstico del compilador lleva un código estable con el formato `NOKnnnn`, que **no cambia
nunca** entre versiones (los mensajes sí pueden mejorar). El rango indica la fase que lo emite:

| Rango | Fase | Ejemplo |
|---|---|---|
| `NOK0xxx` | Uso del compilador, opciones, archivos | `NOK0001`: no se encuentra el archivo |
| `NOK1xxx` | Léxico | `NOK1001`: palabra reservada escrita con tilde |
| `NOK2xxx` | Sintáctico | `NOK2001`: falta cerrar un paréntesis |
| `NOK3xxx` | Semántico (nombres, módulos, visibilidad) | `NOK3001`: identificador no declarado |
| `NOK4xxx` | Tipos y genéricos | `NOK4001`: tipos incompatibles |
| `NOK5xxx` | Propiedad, préstamos y tiempos de vida | `NOK5001`: uso después de mover |
| `NOK6xxx` | Generación de código y enlazado | `NOK6001`: símbolo no encontrado |
| `NOK7xxx` | FFI y ABI | `NOK7001`: tipo sin `@repr(C)` en la frontera |
| `NOK8xxx` | Advertencias (no impiden compilar) | sin códigos por ahora |
| `NOK9xxx` | Error interno del compilador | `NOK9001`: invariante roto |

Formato de salida en texto:

```
error[NOK5001]: no se puede usar «datos» después de moverla
  --> principal.oki:12:9
   |
12 |     imprimir(datos)
   |               ^^^^^ el valor se movió en la línea 10
   |
   = ayuda: usa «datos.clonar()» o pasa una referencia «&datos»
```

El compilador debe ofrecer además `--diagnosticos=json` con los mismos campos (código, gravedad,
archivo, línea, columna, longitud, mensaje, notas y sugerencias) para las herramientas.

---

## 12. Atributos — NORMATIVO

| Atributo | Aplica a | Efecto |
|---|---|---|
| `@virtual` | método | Permite sobrescritura |
| `@sobrescribir` | método | Obligatorio al sobrescribir |
| `@final` | método | Prohíbe sobrescritura |
| `@sellada` | clase | Prohíbe herencia |
| `@estatico` | función/método | Alternativa a la palabra `estatico` |
| `@repr(C)` | estructura/clase | Layout compatible con C |
| `@derivar(...)` | estructura/clase/enumeración | Genera `igualdad`, `orden`, `clonar`, `depurar`, `serializar` |
| `@prueba` | función | Marca una prueba |
| `@rendimiento` | función | Marca un benchmark |
| `@si(condición)` | cualquier elemento | Compilación condicional (`plataforma = "windows"`, `arquitectura = "arm64"`) |
| `@exportar("nombre")` | función | Exporta con nombre C |
| `@envolver` | función/bloque | Desactiva comprobación de desbordamiento |
| `@rapido` | función | Relaja IEEE-754 (rompe reproducibilidad) |
| `@deprecado("mensaje")` | cualquier elemento | Aviso al usarse |
| `@permitir(...)` / `@avisar(...)` | módulo/función | Control de diagnósticos |
| `@inmutable_sugerido` | variable | Fuerza el aviso de "podría ser constante" |
| `@desnudo` | función | Sin prólogo ni epílogo (manejadores de interrupción, arranque) |
| `@insegura` | función | Toda la función es un bloque `inseguro` (auditada por el linter) |

Un atributo desconocido es **error**, con sugerencia del más parecido.

---

## 13. FFI con C — NORMATIVO

```nooki
externo "C" {
    funcion printf(formato: *caracter, ...) -> entero
    funcion malloc(tamano: tamano) -> *mutable vacio
    funcion free(ptr: *mutable vacio)
}

@repr(C)
estructura punto_c { x: flotante64, y: flotante64 }

@exportar("nooki_sumar")
funcion sumar_c(a: entero32, b: entero32) -> entero32 { retornar a + b }
```

- Sin `@repr(C)`, el *layout* es opaco y el tipo no puede cruzar la frontera (error de compilación).
- Una función `externo "C"` **no toma propiedad** de sus argumentos.
- Solo se admiten variádicas en declaraciones externas.

---

## 14. Ejecución — NORMATIVO

- Punto de entrada: `funcion principal() -> vacio` **o** `funcion principal(argumentos: vector<texto>) -> entero`.
- En el segundo caso, el valor devuelto es el código de salida del proceso.
- `imprimir`, `leer_linea` y `pánico` están en el preludio (no requieren `importar`).

---

## 15. Conformidad — NORMATIVO

Un compilador puede declararse **"Nooki 0.1"** solo si:

- [ ] Acepta la gramática completa de `gramatica.ebnf` sin extensiones.
- [ ] Rechaza con error todo programa que esta especificación prohíbe.
- [ ] Pasa el 100 % de la suite de conformidad (`pruebas/conformidad/`).
- [ ] Emite diagnósticos con código, ubicación y sugerencia, en español.
- [ ] Compila a código nativo para al menos x86_64 (Windows, Linux, macOS).
- [ ] Implementa las comprobaciones de desbordamiento (ADR-003).
- [ ] Implementa el verificador de préstamos (ADR-001, §10).
- [ ] No permite cruzar un pánico por la frontera FFI.

---

## 16. Glosario español → inglés (para implementadores)

| Nooki | Inglés | Nooki | Inglés |
|---|---|---|---|
| `clase` | class | `rasgo` / `interfaz` | trait |
| `estructura` | struct | `implementar` | impl |
| `enumeracion` | enum | `variable` / `constante` | let / const |
| `prestamo` | borrow | `&mutable` | `&mut` |
| `tiempo de vida` | lifetime | `inseguro` | unsafe |
| `coincide` | match | `caso` | arm |
| `desbordamiento` | overflow | `envolver` | wrapping |
| `pánico` | panic | `resultado` / `opcional` | Result / Option |

---

## 17. Restricciones conocidas de la v0.1 (declaradas, no olvidadas)
1. Sin `async`/`await` (ADR-005).
2. Sin doble referencia (`&&T`).
3. Sin etiquetas de bucle (`romper` solo al bucle más interno).
4. Sin argumentos de tipo explícitos en llamadas (ADR-013).
5. Sin literales de estructura en condiciones sin paréntesis.
6. Sin herencia múltiple ni *mixins*.
7. Sin clases de C++ en FFI (solo `extern "C"`).
8. Sin macros de usuario (llegan en v0.7).
9. Sin `no_std` (llega en v0.5).


---

## 18. Ampliación de la v0.2 — NORMATIVO

Todo lo anterior sigue vigente. Esta sección añade lo que el compilador implementa desde el Sprint 17.

### 18.1 Estructuras, clases y métodos

```aura
estructura punto {
    publico x: entero
    publico y: entero

    publico funcion suma(otro: punto) -> entero {
        retornar este.x + este.y + otro.x + otro.y
    }
}
```

- `estructura` y `clase` comparten representación **por valor** en la v0.1/v0.2 (ADR-015). La semántica de referencia de `clase` llega con el modelo de memoria.
- Los campos se declaran `nombre: tipo`; la visibilidad por defecto es `privado`.
- Dentro de un método, `este` designa el objeto actual. Los métodos se traducen a funciones con el objeto como primer parámetro.
- Construcción con campos nombrados: `punto { x: 1, y: 2 }`. Todos los campos son obligatorios.

### 18.2 Enumeraciones y `coincide`

```aura
enumeracion color { rojo, verde, azul }

coincide (c) {
    caso color.rojo => imprimir("rojo")
    caso 1           => imprimir("uno")
    caso _           => imprimir("otro")
}
```

- Los patrones admiten literales enteros y booleanos, variantes de enumeración y el comodín `_`.
- **El comodín es obligatorio**; sin él, el `coincide` es error `NOK4006`.
- El valor examinado se evalúa **una sola vez**.

### 18.3 `Caja<T>`: propiedad y movimiento

```aura
variable c = Caja.nuevo(41)
*c = *c + 1
variable d = c      // movimiento: «c» deja de ser válida
imprimir(*d)        // 42
```

- `Caja<T>` es un valor en el montículo con **un único dueño** (ADR-016).
- Asignar una caja **transfiere** la propiedad. Usar el origen después de moverlo es error `NOK5001`.
- El dueño **libera** al salir del ámbito y antes de cada `retornar`.
- En la v0.2 solo existe `Caja<entero>`; los genéricos llegan con el sistema de tipos completo.

### 18.4 Préstamos

```aura
funcion sumar_a(p: &mutable Caja<entero>, cuanto: entero) -> vacio {
    *p = *p + cuanto
}

funcion leer(p: &Caja<entero>) -> entero {
    retornar *p
}
```

- `&caja` presta **sin mover**: la variable sigue siendo válida y se libera donde siempre.
- **El préstamo vive durante la expresión que lo crea.** Guardarlo en una variable es error `NOK5002` (ADR-017).
- `&mutable` permite escribir a través del préstamo; a través de un préstamo `&` es error `NOK4009`.
- **Exclusividad**: en una misma llamada no se puede pasar `&mutable` y otro préstamo de la misma variable (`NOK5003`).
- Solo se presta una variable que sea una `Caja` (`NOK4008`).

### 18.5 Códigos nuevos

| Código | Significado |
|---|---|
| `NOK3003` | Tipo inexistente |
| `NOK3004` | Campo inexistente en una construcción |
| `NOK3005` | Campo que falta en una construcción |
| `NOK3006` | Miembro inexistente |
| `NOK3007` | Variante inexistente |
| `NOK4002` | Argumento con tipo incorrecto |
| `NOK4003` | Aridad incorrecta |
| `NOK4004` | Retorno incompatible |
| `NOK4005` | Condición no booleana |
| `NOK4006` | `coincide` sin comodín |
| `NOK4007` | Desreferencia inválida |
| `NOK4008` | Préstamo de algo que no es una `Caja` |
| `NOK4009` | Escritura a través de un préstamo inmutable |
| `NOK5001` | Uso después de mover |
| `NOK5002` | Préstamo guardado en una variable |
| `NOK5003` | Préstamo mutable y compartido en la misma llamada |


### 18.6 Texto

```aura
variable saludo = "Hola, " + nombre + "!"
imprimir(saludo)
imprimir(saludo.largo())

si (saludo == "Hola, Nooki!") {
    imprimir("igual")
}
```

- `texto` lleva su longitud; `largo()` es constante.
- `+` concatena dos textos; comparar con `==`, `!=`, `<`, `>`, `<=`, `>=` es **por contenido**, no por dirección.
- Los literales no se liberan; las concatenaciones sí. La marca `propio` del runtime (ADR-018) distingue ambos casos.
- Asignar un texto lo **mueve**; usar el origen después es `NOK5001`.
- Mezclar `texto` con `entero` en una operación es `NOK4001`.
- `largo()` cuenta **bytes**, no caracteres.


### 18.7 Genéricos

```aura
funcion identidad<T>(valor: T) -> T {
    retornar valor
}

imprimir(identidad<entero>(42))        // 42
imprimir(identidad<texto>("Nooki"))    // Nooki
```

- Una función puede declarar parámetros de tipo: `funcion nombre<T, U>(...)`.
- **Los argumentos de tipo son explícitos** en cada llamada: `f<entero>(x)`. No hay inferencia
  todavía.
- Cada combinación de tipos produce una función distinta en el binario (instanciación), sin
  despacho en ejecución.
- `T` se puede usar en parámetros, en el tipo de retorno y en declaraciones dentro del cuerpo.


### 18.8 Colecciones

```aura
variable numeros = Vector<entero>.nuevo()
numeros.agregar(3)
numeros.agregar(7)
imprimir(numeros.largo())        // 2
imprimir(numeros.obtener(0))     // 3

para (n en numeros) {
    imprimir(n)
}
```

- `Vector<T>` es una colección dinámica cuyo **dueño es la variable**.
- `agregar`, `obtener` y `largo`. `obtener` comprueba el índice y devuelve el valor cero fuera de rango.
- **`agregar` transfiere la propiedad del elemento**: usar el origen después es `NOK5001`.
- `obtener` devuelve una **vista**: no libera ni duplica el elemento.
- `para (n en coleccion)` recorre por índice con una variable interna que no se expone.
- Pasar una colección a una función se hace con `&Vector<T>` (préstamo); pasarla por valor la mueve.

### 18.9 `Mapa<texto, entero>`

```aura
variable edades = Mapa<texto, entero>.nuevo()
edades.insertar("Ana", 30)
imprimir(edades.obtener("Ana"))    // 30
imprimir(edades.contiene("Eva"))   // 0 (falso)
imprimir(edades.largo())           // 1
```

- `insertar`, `obtener`, `contiene` y `largo`.
- **`insertar` transfiere la propiedad de la clave**; reemplazar un valor existente conserva la clave guardada y libera la nueva.
- `obtener` devuelve `0` si la clave no está; use `contiene` para distinguir el cero de la ausencia.

### 18.10 Módulos

```aura
// matematicas.oki
publico funcion cuadrado(n: entero) -> entero {
    retornar n * n
}

// principal.oki
usar matematicas

imprimir(matematicas.cuadrado(7))
```

- `usar modulo` carga `modulo.oki` desde el **directorio del archivo que lo importa**.
- Los módulos pueden importar otros módulos; el orden no importa y los ciclos se cargan una sola vez.
- **Solo lo marcado `publico` es accesible** desde otro módulo (`NOK3009` si no). Lo que no lleva
  visibilidad es privado.
- Un módulo que no existe es `NOK3008`, con la ruta buscada.
- Las funciones de un módulo viven en su espacio de nombres. **Los tipos y enumeraciones todavía no**:
  comparten un espacio global.

### 18.11 Propiedad en las llamadas

```aura
funcion consumir(t: texto) -> entero {
    retornar t.largo()
}

variable a = "uno" + "!"
variable n = consumir(a)   // 'a' se mueve
imprimir(a)                // NOK5001
```

- Pasar un valor con dueño (`Caja`, `texto`, `Vector`, `Mapa`) a un parámetro que **no es préstamo**
  lo mueve.
- Prestar con `&` o `&mutable` **no** mueve: la variable sigue siendo válida.
- Esta regla es única para todos los tipos con dueño (ADR-020).

### 18.12 Códigos nuevos de la v0.3

| Código | Significado |
|---|---|
| `NOK3008` | Módulo no encontrado |
| `NOK3009` | Símbolo privado de un módulo |


### 18.13 Entrada estándar

```aura
imprimir("¿Cómo te llamas?")
variable nombre = leer_linea()
imprimir("Hola, " + nombre)

variable n = leer_entero()
imprimir(n * 2)
```

- `leer_linea() -> texto` lee una línea **sin el salto** y devuelve un texto con dueño.
- `leer_entero() -> entero` lee una línea y la interpreta como entero con signo; si no contiene
  dígitos, devuelve `0`.
- Al final de la entrada, `leer_linea` devuelve un texto vacío y `leer_entero` devuelve `0`: un
  programa nunca se queda bloqueado ni recibe valores indefinidos.
- Ambas son **integrales del lenguaje**: no se declaran ni se importan.


### 18.14 Tipos de un módulo

```aura
// figuras.oki
publico estructura punto {
    publico x: entero
    publico y: entero
}

publico funcion crear(x: entero, y: entero) -> punto {
    variable p = punto { x: x, y: y }
    retornar p
}
```

```aura
usar figuras

variable a: figuras.punto = figuras.crear(1, 2)
```

- Un tipo exportado se nombra desde fuera como `modulo.tipo`.
- Dentro de su módulo se sigue usando el nombre corto.
- Dos módulos pueden declarar un tipo con el mismo nombre: no chocan.
- `publico` en `estructura`, `clase` o `enumeracion` los hace visibles; sin ella son privados.
- `punto` y `figuras.punto` designan la misma estructura: el compilador compara por definición.


### 18.15 Clases por referencia

```aura
clase contador {
    publico valor: entero

    publico funcion incrementar() -> vacio {
        este.valor = este.valor + 1
    }
}

variable c = nuevo contador { valor: 10 }
c.incrementar()
imprimir(c.valor)     // 11
```

- Una `clase` se crea con `nuevo Tipo { ... }` y vive en el montículo.
- La variable es **dueña** de la referencia: se libera al salir del ámbito.
- Asignarla **mueve**; usar el origen después es `NOK5001`. Prestar con `&` no mueve.
- `estructura` sigue siendo **por valor**: se copia al asignar y no tiene identidad.
- La diferencia es observable: dos variables de estructura con los mismos campos son iguales; dos
  referencias a la misma clase son el mismo objeto.


### 18.16 Herencia y polimorfismo

```aura
clase figura {
    publico nombre: texto
    publico funcion area() -> entero { retornar 0 }
}

clase cuadrado hereda figura {
    publico lado: entero
    publico funcion area() -> entero { retornar este.lado * este.lado }
}

variable c = nuevo cuadrado { nombre: "c", lado: 4 }
imprimir(c.area())    // 16
```

- `clase derivada hereda base` establece una jerarquía simple (una sola base).
- La derivada **hereda los campos y los métodos** de la base; los campos heredados se nombran igual
  en la construcción.
- **Un método es virtual si lo declara la raíz de la jerarquía.** Se despacha por tabla, así que una
  derivada que lo redefine responde con su versión.
- Si la derivada no lo redefine, hereda la implementación de la base.
- Los métodos propios de una derivada que no existen en la base no son virtuales.


### 18.17 `super` y variables de tipo base

```aura
clase cuadrado hereda figura {
    publico funcion describe() -> texto {
        retornar super.describe() + " cuadrada"
    }
}

variable f: figura = nuevo cuadrado { nombre: "c", lado: 4 }
imprimir(f.area())     // despacha a la implementación de cuadrado
```

- `super.metodo(args)` llama a la implementación de la clase base **sin despacho**.
- Un puntero a una clase derivada se puede guardar en una variable de tipo base; la conversión es
  automática y segura porque la base ocupa el principio de la estructura.
- Las llamadas a métodos virtuales a través de la variable base **despachan** a la clase concreta.


### 18.18 Errores como valores

```aura
usar resultado

funcion dividir(a: entero, b: entero) -> resultado {
    si (b == 0) {
        retornar Error("no se puede dividir por cero")
    }
    retornar Ok(a / b)
}

funcion repartir(a: entero, divisor: entero) -> resultado {
    variable cociente = dividir(a, divisor)?
    retornar Ok(cociente + 1)
}
```

- **No hay excepciones.** Un fallo es un valor de una enumeración.
- `resultado` es un **módulo de la biblioteca** escrito en Nooki: `Ok(valor: entero)` y
  `Error(mensaje: texto)`.
- **`?` propaga**: si la variante no es la primera, la función devuelve ese mismo valor; si lo es, la
  expresión vale el contenido de su campo.
- `?` exige que la función devuelva una enumeración **con forma de resultado**: al menos dos
  variantes y la primera con un único campo (`NOK4011`).
- El tratamiento explícito es con `coincide`, ligando el contenido en cada rama.


### 18.19 Operaciones de texto

```aura
variable linea = "nombre:Ana"
variable corte = linea.buscar(":")          // 6
variable clave = linea.subtexto(0, corte)   // "nombre"
variable valor = linea.subtexto(corte + 1, 100)  // "Ana"
```

| Llamada | Devuelve |
|---|---|
| `t.largo()` | bytes del texto |
| `t.buscar(otro)` | posición de la primera aparición, o `-1` |
| `t.subtexto(desde, cuantos)` | texto nuevo, recortado al final si se pide de más |
| `t.contiene(otro)` | booleano |
| `t.empieza_con(prefijo)` | booleano |
| `t.caracter(indice)` | byte en esa posición, o `-1` |
| `t.a_entero()` | dígitos iniciales con signo; `0` si no hay |

- `subtexto` devuelve un texto **con dueño**: se libera al cerrar el ámbito.
- `buscar`, `caracter` y `a_entero` comprueban sus límites y nunca leen fuera del texto.


### 18.20 Variantes con datos

```aura
enumeracion respuesta {
    Bien(valor: entero)
    Mal(mensaje: texto)
}

coincide (comprobar(5)) {
    caso Bien(v) => imprimir(v)
    caso Mal(m)  => imprimir(m)
}
```

- Una variante puede declarar campos: `Bien(valor: entero)`.
- Se construye con su nombre: `Bien(10)`, `Mal("mensaje")`.
- El patrón `caso Bien(v)` **liga** `v` con el valor del campo, y su tipo es el del campo.
- Si están todas las variantes, el `coincide` es exhaustivo sin `caso _`.
- Las enumeraciones **sin datos** siguen siendo enteros con constantes: nada anterior cambia.
- Los campos de texto de una variante se liberan al salir del ámbito.


### 18.21 Más biblioteca

| Llamada | Devuelve |
|---|---|
| `t.dividir(separador)` | `Vector<texto>` con copias de los trozos |
| `t.mayusculas()` | copia en mayúsculas (ASCII) |
| `t.minusculas()` | copia en minúsculas (ASCII) |
| `t.recortar()` | copia sin espacios ni saltos en los extremos |

```aura
usar matematicas

matematicas.maximo(3, 9)      // 9
matematicas.minimo(3, 9)      // 3
matematicas.absoluto(0 - 7)   // 7
matematicas.cuadrado(6)       // 36
```

- El módulo `matematicas` está **escrito en Nooki** y viaja con el compilador, igual que `resultado`.
- `dividir` reparte también el último trozo cuando el separador no aparece al final.


### 18.22 Proyectos y búsqueda de módulos

```bash
$ tree
principal.oki
biblioteca/
    utilidades.oki

$ nooki construir
```

- `usar modulo` busca, en este orden: junto al archivo que importa, en su carpeta `biblioteca/`, y
  subiendo por los directorios padre hasta encontrarlo.
- Los módulos del compilador (`resultado`, `matematicas`) están disponibles sin archivo; un proyecto
  puede sustituirlos con su propia versión en `biblioteca/`.
- `nooki construir` compila el proyecto del directorio actual usando `principal.oki` o
  `fuente/principal.oki` como punto de entrada.


### 18.23 Genéricos de tipos

```aura
estructura par<T> {
    publico primero: T
    publico segundo: T
}

variable numeros = par<entero> { primero: 1, segundo: 2 }
variable textos = par<texto> { primero: "hola", segundo: " mundo" }
```

- Una `estructura` o `clase` puede declarar parámetros de tipo: `<T, U>`.
- Se instancia **explícitamente** al construirla: `par<entero> { ... }`.
- Cada combinación de tipos produce una estructura propia en el binario, sin despacho en
  ejecución, igual que en las funciones genéricas.
- Los campos cuyo tipo es un parámetro se sustituyen por el argumento correspondiente.
- Los métodos de un tipo genérico todavía no se instancian.


### 18.24 Reglas de propiedad en compuestos y saltos

- **Un compuesto con campos con dueño no se puede copiar** (`NOK5004`). Se construye otro o se pasa
  con `&`. Copiarlo compartiría las cadenas y se liberarían dos veces.
- **`romper` y `continuar` liberan** los valores con dueño declarados en el ámbito que abandonan.
- Los compuestos construidos en el sitio o devueltos por una llamada **liberan sus campos** al salir
  del ámbito.


### 18.25 Una llamada que mueve no puede ir anidada

```aura
funcion consumir(t: texto) -> entero { ... }

variable c = "adios"
variable n = consumir(c)     // correcto: c se mueve aquí
imprimir(consumir(c))        // error: no hay dónde vaciar c
```

- Una llamada que **mueve** un valor con dueño (parámetro por valor) solo puede aparecer como
  **sentencia** o como **inicializador de una declaración**.
- Dentro de otra expresión (`f(g(a))`, `imprimir(f(a))`), el compilador no tiene dónde emitir el
  vaciado del origen, así que lo rechaza en lugar de dejar una fuga silenciosa.
- La alternativa es siempre la misma: guardar el resultado en una variable, o prestar el valor con
  `&` si la función lo acepta.


### 18.26 Herramientas del proyecto

```bash
$ nooki nuevo demo          # crea demo/principal.oki y demo/biblioteca/herramientas.oki
$ cd demo
$ nooki construir           # compila el proyecto a un binario
$ nooki probar              # ejecuta pruebas/*.oki y compara con su .esperado
$ nooki limpiar             # borra binarios y artefactos
```

- `nooki probar` considera cada `pruebas/<nombre>.oki` una prueba: lo compila, lo ejecuta y compara
  su salida con `pruebas/<nombre>.oki.esperado`. Sin archivo esperado, basta con que termine bien.
- La comparación ignora los saltos de línea de Windows.
- `nooki limpiar` no toca el código fuente: solo `build/`, `artefactos/` y binarios sueltos.


### 18.27 Rasgos

```aura
rasgo Describible {
    funcion describir() -> texto
}

clase persona implementa Describible {
    publico nombre: texto

    publico funcion describir() -> texto {
        retornar "persona " + este.nombre
    }
}
```

- Un `rasgo` declara **firmas** de métodos, sin cuerpo.
- Un tipo declara `implementa A, B` y se compromete a ofrecer esos métodos.
- El compilador exige que **estén todos** (`NOK3011`) y que la **firma coincida** (`NOK4012`).
- Los rasgos no generan código: son una comprobación de tiempo de compilación.
- Todavía no hay despacho por rasgo (`&Rasgo`): los métodos se llaman sobre el tipo concreto.


### 18.28 `&Rasgo`: despacho por rasgo

```aura
rasgo Saluda {
    funcion saludar() -> texto
}

funcion presentar(quien: &Saluda) -> vacio {
    imprimir(quien.saludar())
}

variable p = nuevo persona { nombre: "Ana" }
presentar(&p)          // despacha a persona.saludar
```

- `&Rasgo` es un tipo: una referencia que lleva el **objeto** y su **tabla de métodos**.
- Se crea con `&objeto` cuando el tipo declara `implementa` ese rasgo.
- La llamada despacha a la implementación del tipo concreto, aunque el tipo no herede de nadie.
- Cada par (tipo, rasgo) genera su tabla y un adaptador por método.
- Sirve para tratar igual a tipos **no emparentados**, que es lo que la herencia no puede dar.


### 18.29 El préstamo que escapa

```aura
funcion malo() -> &texto {
    variable local = "temporal" + ""
    retornar &local          // error: «local» muere al salir
}
```

- **No se puede devolver un préstamo a una variable local** (`NOK5006`).
- **Sí se puede devolver un préstamo recibido como parámetro**: el valor vive en quien llamó.
- Los préstamos pasados como **argumento** no escapan: solo duran la llamada.
- Esto cubre el caso en que una referencia quedaría colgando al volver. La duración de un préstamo
  entre variables del mismo ámbito todavía no se verifica.


### 18.30 Duración del préstamo

```aura
variable p = nuevo persona { nombre: "Ana" }
variable d: &Saluda = &p
p = nuevo persona { nombre: "Eva" }   // error NOK5007
```

- **Mientras un valor está prestado, su dueño no se puede mover ni reasignar** (`NOK5007`).
- **Prestar una clase es prestar el objeto**: `&persona` conserva una sola referencia; no hay puntero
  doble.
- La duración de un préstamo temporal (tomado y usado entre sentencias del mismo ámbito) todavía no
  se verifica.


### 18.31 Biblioteca escrita en Nooki

```aura
// biblioteca/textos.oki
publico funcion unir(partes: &Vector<texto>, separador: texto) -> texto { ... }

// principal.oki
usar textos
imprimir(textos.unir(&partes, ", "))
```

- Un módulo de biblioteca es **código Nooki normal**: `biblioteca/<nombre>.oki` con funciones
  `publico`.
- `nooki` lo busca junto al archivo, en `biblioteca/` y subiendo por los directorios padre.
- Un archivo **no puede importarse a sí mismo**: se informa como módulo no encontrado.
- El elemento de un `Vector<texto>` es un `texto`; `*prestamo` de un `&texto` es un `texto`.


### 18.32 Módulos y biblioteca embebida

- `usar m` busca **primero un archivo**: junto al que importa, en `biblioteca/` y subiendo por los
  directorios padre. Si no lo encuentra, usa el módulo de la biblioteca que viaja en el compilador.
- **Un archivo con el nombre de un módulo lo sustituye.** Es intencionado: permite que un proyecto
  reemplace una pieza de la biblioteca, pero conviene saberlo al depurar.
- **Un módulo no puede definir `principal`**: un módulo es una biblioteca y se informa en el módulo.
- Un archivo **no puede importarse a sí mismo**.


### 18.33 La biblioteca estándar

Los módulos viajan dentro del compilador: se usan con «usar» sin copiar ningún archivo.

| Módulo | Contenido |
|---|---|
| `resultado` | `Ok(valor)`, `Error(mensaje)` |
| `matematicas` | `maximo`, `minimo`, `absoluto`, `cuadrado` |
| `textos` | `unir(&Vector<texto>, separador)`, `repetir(&texto, veces)`, `contar(&texto, aguja)` |
| `listas` | `suma`, `mayor`, `menor`, `promedio`, `contiene` sobre `&Vector<entero>` |
| `fechas` | `bisiesto(anio)`, `dias_del_mes(mes, anio)`, `dias_del_anio(anio)` |

Todos están escritos en Nooki y se compilan con el mismo compilador: no hay runtime en C detrás de ellos.


### 18.34 Las vías del movimiento

Un dueño no puede moverse ni reasignarse mientras alguien lo presta. Se comprueba en todas las
vías: la reasignación (`p = ...`), la declaración que mueve (`variable q = p`) y el argumento por
valor (`f(p)`).

- **Reasignar no marca el valor como movido**: sustituye el valor, y la variable sigue siendo válida.
- Prestar un parámetro que es una clase se rechaza: solo se prestan variables locales.
- Mover un campo a una colección todavía no se reconoce como movimiento (A-14).


### 18.35 Mover un campo

- Mover un campo a una colección (`vector.agregar(p.nombre)`) transfiere la cadena y deja el campo
  **vacío y válido**: longitud cero, sin puntero colgando.
- Un método heredado cuenta como miembro del tipo, tanto al leer un campo como al llamarlo.
- `imprimir` acepta enteros, textos y booleanos.


### 18.36 Aritmética comprobada

Sumar, restar, multiplicar, dividir, calcular el módulo o negar un `entero` **comprueba el resultado**
(ADR-03). Si desborda, si se divide por cero o si se niega el mínimo del tipo, el programa se detiene:

```
error en ejecucion: desbordamiento al sumar
```

El código de salida es **70** y el mensaje va a la salida de error. Un desbordamiento nunca se
ignora en silencio ni produce un valor equivocado.

- El coste es un salto predicho por operación: con `-O2` el compilador de C lo resuelve en una
  instrucción y una bifurcación.
- `entero` es de 64 bits con signo.


### 18.37 FFI: llamar a C

```aura
externo funcion puts(cadena: &texto) -> booleano
externo funcion time(zona: &entero) -> entero
```

- `externo funcion` **declara** una función de C: no lleva cuerpo y conserva su nombre de C.
- Tipos: `entero` es `long long`, `booleano` es `int`, `caracter` es `char`, `vacio` es `void`.
- `&texto` viaja como `const char*` apuntando a los bytes del texto, que ya terminan en cero: no hay
  copia.
- `&escalar` viaja como puntero al escalar, y `nulo` es el `NULL` de C.
- La firma declarada debe coincidir con la de C. Si no, el compilador de C informa del conflicto al
  compilar el código generado.
- Todavía no se declaran estructuras ni funciones que devuelvan `texto`, y no se elige biblioteca:
  se enlaza contra la de C del sistema.


### 18.38 `inseguro` y `asm!`

```aura
inseguro {
    variable envuelto = grande + 1   // se envuelve: no se comprueba
    asm!("nop")
}
```

- Dentro de un bloque `inseguro { }` la aritmética **no** se comprueba: se emite el operador de C tal
  cual. Es la vía para el código que habla con el hardware.
- **`asm!("...")`** emite ensamblador en línea y **solo se permite dentro de `inseguro`** (`NOK3012`).
- **La propiedad no se desactiva**: dentro del bloque los valores se siguen moviendo al pasar y se
  siguen liberando al salir. Lo único que se apaga es la comprobación aritmética.
- `asm!` necesita un compilador de C que admita ensamblador en línea (GCC, Clang, MinGW). Con `cl.exe`
  el C generado lo dice con un `#error`.
- Los bloques `inseguro` deben documentar por qué están: son la lista de sitios donde el lenguaje no
  garantiza nada.


### 18.39 Manifiesto y dependencias

```toml
# nooki.toml
[proyecto]
nombre = "agenda"
version = "0.1.0"

[dependencias]
saludos = { ruta = "../saludos" }
```

- `nooki.toml` declara el proyecto y sus dependencias por **ruta**.
- `nooki construir` añade a la búsqueda de módulos la raíz y la `biblioteca/` de cada dependencia,
  así que `usar saludos` funciona sin copiar archivos.
- `nooki nuevo` crea el manifiesto con el nombre del proyecto.
- No hay versiones semánticas, descarga desde Git ni registro: son la fase siguiente, y necesitan
  usuarios antes que diseño.


### 18.40 Servidor de lenguaje

`nooki lsp` atiende el protocolo LSP por la entrada y salida estándar:

- `initialize`: capacidades (sincronización completa de documentos).
- `textDocument/didOpen` y `didChange`: reanaliza el documento.
- `textDocument/publishDiagnostics`: envía los diagnósticos con rango, gravedad, código y mensaje.
- `shutdown` y `exit`.

Los diagnósticos llevan la línea y la columna del span, así que el editor subraya exactamente lo que
está mal. Cualquier cliente LSP sirve; no hay una extensión propia.


### 18.41 Archivos y fechas

```aura
usar archivo

coincide (archivo.leer(&ruta)) {
    caso Bien(contenido) => imprimir(contenido)
    caso Mal(motivo) => imprimir(motivo)
}
```

- **`archivo`**: `existe(ruta)`, `leer(ruta)` y `escribir(ruta, contenido)`. Las dos últimas devuelven
  el enumerado `lectura`, con `Bien(contenido)` o `Mal(motivo)`; `?` funciona sobre él.
- **`fechas`**: una fecha es un entero, días desde el 1 de enero de 1970. `a_dias(anio, mes, dia)`,
  `a_anio`, `a_mes`, `a_dia`, `sumar_dias` y `dia_de_la_semana` (0 = lunes).
- Los textos multilínea se escriben con tres comillas: `"""..."""`.


### 18.42 Estabilidad de la biblioteca

- La API pública de los módulos de la biblioteca **está congelada por una prueba**: el programa de
  conformidad `0051-api-biblioteca` llama a todas las funciones públicas. Cambiar una firma sin querer
  rompe `ctest` en lugar de romper a un usuario.
- Al añadir una función pública, añádela también a ese programa.
- La distribución se genera con `herramientas/empaquetar.ps1` (Windows) o `empaquetar.sh` (Linux y
  macOS): binario, documentación, ejemplos, editores, especificación e `INSTALAR.txt`.


### 18.43 Ventanas

```aura
usar ventana

variable v = ventana.abrir(&titulo, 480, 320)
mientras (ventana.viva(v)) {
    si (ventana.siguiente_evento(v) == 1) { romper }
    ventana.pausa(16)
}
ventana.cerrar(v)
```

- `abrir(titulo, ancho, alto)`: devuelve un **manejador** (un entero). **0 significa que no hay
  entorno gráfico**, y el programa decide qué hacer; no es un error del lenguaje.
- `viva(manejador)`: atiende los mensajes pendientes y dice si la ventana sigue abierta.
- `siguiente_evento(manejador)`: 0 nada, 1 cerrar.
- `cerrar(manejador)` y `pausa(milisegundos)`.
- La capa C se compila **solo si el programa usa el módulo** y se enlaza con las bibliotecas del
  sistema: `user32` y `gdi32` en Windows, `X11` en Linux y Unix. macOS necesita su backend.


### 18.44 Dibujo

```aura
ventana.limpiar(v, 0xFFFFFF)
ventana.rectangulo(v, 10, 10, 100, 50, 0xFF0000)
ventana.linea(v, 0, 200, 319, 200, 0x0000FF)
ventana.escribir_texto(v, 20, 100, &saludo, 0x000000)
ventana.presentar(v)
variable color = ventana.color_en(v, 50, 30)
```

- Los colores son enteros `0xRRGGBB`.
- `color_en(x, y)` devuelve el color del píxel: sirve para probar el dibujo, no solo para pintarlo.
- La implementación es la del sistema: **GDI** en Windows y **X11** en Linux. No hay biblioteca de
  gráficos añadida; si algún día hace falta antialiasing, Skia puede entrar detrás de esta misma API.


### 18.45 Layout

```aura
usar diseno

variable c = nuevo columna { x: 0, y: 0, ancho: 480, alto: 320, usado: 0, separacion: 8 }
variable y = c.hueco(40)      // desplazamiento del siguiente hijo
si (c.cabe(40)) { ... }
```

- `columna` y `fila` reparten hijos en su eje y llevan la cuenta de lo usado.
- `hueco(medida)` devuelve el desplazamiento y avanza el cursor; `cabe(medida)` dice si queda sitio;
  `reiniciar()` vuelve al principio.
- `centrar(disponible, medida)` y `repartir(disponible, partes)`.
- Es computación pura: se prueba sin abrir una ventana.


### 18.46 Controles

```aura
usar controles

variable b = nuevo boton { x: 10, y: 10, ancho: 120, alto: 40,
                           contenido: "Aceptar", fondo: 0x89B4FA, color: 0x11111B }
b.dibujar(v)
si (b.pulsado(15, 20)) { ... }
```

- `control`: base con `x`, `y`, `ancho`, `alto` y `contiene(px, py)` — intervalo semiabierto, de modo
  que dos controles pegados no se solapan.
- `etiqueta`, `boton` y `barra` heredan de `control` y sobrescriben `dibujar(pantalla)`.
- `boton.pulsado(px, py)` es lo mismo que `contiene`: la diferencia es de intención.
- La geometría se prueba sin ventana; solo `dibujar` necesita una.


### 18.47 Eventos

```aura
variable e = ventana.evento(v)
mientras (e != 0) {
    si (ventana.es_pulsacion(e)) {
        variable x = ventana.evento_x(v)
        variable y = ventana.evento_y(v)
    }
    e = ventana.evento(v)
}
```

- Clases de evento: 0 nada, 1 cerrar, 2 movimiento, 3 pulsación, 4 liberación, 5 tecla.
- `es_cerrar`, `es_movimiento`, `es_pulsacion`, `es_liberacion` y `es_tecla` evitan usar números
  sueltos en el programa.
- `evento_x`, `evento_y` y `evento_tecla` dan los datos del último evento entregado.
- La cola guarda 64 eventos, así que no se pierde entrada entre fotogramas.


### 18.48 Accesibilidad

```aura
usar accesibilidad

variable color_texto = accesibilidad.texto_sobre(fondo)
si (accesibilidad.legible(texto, fondo)) { ... }

variable estado = nuevo foco { indice: 0, total: 3 }
estado.siguiente()
```

- `luminancia(color)`: 0 (negro) a 255 (blanco), fórmula YIQ con enteros.
- `legible(texto, fondo)`: diferencia de luminancia de al menos 128.
- `texto_sobre(fondo)`: negro o blanco, el que más contraste dé.
- `foco`: índice que envuelve entre controles para navegar con Tab y Mayús+Tab.
- La regla de legibilidad es una **heurística**, no el contraste WCAG exacto: ese exige corrección
  gamma con potencias y aquí no se calcula con coma flotante.


### 18.49 Imágenes

```aura
usar imagenes

variable icono = imagenes.cargar(&ruta)
si (imagenes.cargada(icono)) {
    imagenes.dibujar(pantalla, icono, 10, 10)
    imagenes.soltar(icono)
}
```

- Formato: **BMP de 24 o 32 bits sin comprimir**. Cualquier otro archivo devuelve 0.
- `ancho`, `alto`, `color_en(x, y)` (0xRRGGBB, o -1 fuera de rango) y `soltar`.
- `cargar` devuelve un manejador; hay que llamar a `soltar` cuando ya no se use.
- PNG queda pendiente: exige un decodificador entero (inflate y filtros) o una dependencia externa.


### 18.50 Repintado

```aura
variable pedido = ventana.repintar(v)
si (dibujar || pedido) {
    // volver a dibujar todo
}
```

- `repintar(manejador)` devuelve verdadero cuando el sistema pide volver a dibujar (la ventana quedó
  al descubierto, se redimensionó o se restauró) y limpia la marca.
- Un programa con bucle **debe** redibujar cuando lo pide; si no, el contenido desaparece.
- En Windows lo marca `WM_PAINT` y en X11 `Expose`.


### 18.51 Tensores

```aura
usar tensor

variable a = tensor.lleno(2, 3, 1)
variable p = tensor.producto(a, tensor.transpuesta(a))
imprimir(tensor.leer(p, 0, 0))
```

- Creación: `ceros(filas, columnas)`, `lleno(filas, columnas, valor)`, `identidad(lado)`.
- Forma y acceso: `filas`, `columnas`, `leer(t, i, j)`, `poner(t, i, j, v)`.
- Operaciones: `suma`, `resta`, `escalar`, `producto` (matricial), `transpuesta`, `suma_total`,
  `iguales` y `determinante`.
- Los elementos son enteros de 64 bits.
- Si las formas no encajan, la operación devuelve 0 en lugar de fallar.
- `liberar(t)` cuando ya no se use.


### 18.52 Estadística y activaciones

```aura
imprimir(tensor.media(a))        // por mil: 2500 es 2.5
variable n = tensor.normalizar(a)
variable p = tensor.softmax(x)   // suma 1000
```

- `minimo`, `maximo`, `media`, `varianza`, `desviacion` y `normalizar`.
- `relu`, `sigmoides` y `softmax`.
- **Todo lo que tiene decimales se da por mil**: 2500 significa 2.5, 1118 significa 1.118. Es la
  convención mientras el lenguaje no tenga coma flotante.
- `normalizar` lleva los valores a 0..1000 conservando las proporciones.
- `softmax` devuelve valores que **suman 1000**: los puntos del redondeo se le dan al mayor.


### 18.53 Flotantes

```aura
variable x = 3.5
variable y = x * 2.0           // 7
imprimir(x / 2.0)              // 1,75
variable n = 7 como flotante
variable m = 3.9 como entero   // 3
```

- Tipo `flotante`: literales, variables, parámetros y retornos.
- Operaciones: `+`, `-`, `*`, `/`, comparaciones y negación.
- **No hay conversión implícita**: entre `entero` y `flotante` se convierte con `como`, en las dos
  direcciones. Mezclarlos sin convertir es un error de tipo.
- La división por cero de un flotante da infinito, no un pánico: es aritmética IEEE 754.
- Se imprime con **coma decimal** y sin ceros de más.


### 18.54 PLN

```aura
usar pln

variable palabras = pln.tokenizar(&frase)
variable cuentas = pln.contar(&palabras)
imprimir(pln.distancia(&"casa", &"cama"))   // 1
imprimir(pln.jaccard(&f1, &f2))             // 0..1000
```

- `normalizar`: minúsculas y un solo espacio entre palabras.
- `tokenizar`, `contar` (frecuencias), `unicas`, `distintas`, `cuantas`, `contiene_texto`.
- `distancia`: distancia de edición entre dos textos.
- `jaccard`: parecido por mil, intersección sobre unión de palabras distintas.
- Todo está escrito en Nooki: no hay C detrás.


### 18.55 Tablas (DataFrame)

```aura
usar tabla

variable t = tabla.crear(4)
tabla.columna_texto(t, &"producto")
tabla.columna_entero(t, &"ventas")
tabla.poner_texto(t, 0, 0, &"pan")
tabla.poner_entero(t, 0, 1, 10)
imprimir(tabla.suma(t, 1))
```

- Columnas con nombre, numéricas o de texto.
- `suma`, `media` (por mil), `minimo`, `maximo`.
- `filtrar(t, columna, &valor)` por texto; `ordenar(t, columna)` por número; `contar_por(t, columna)`
  devuelve una tabla con «valor» y «cuenta».
- Las funciones `externo` **pueden devolver `texto`**: el generador copia la cadena de C.


### 18.56 Autograd

```aura
usar autograd

variable x = autograd.dato(2.0)
variable y = autograd.suma(autograd.producto(x, x), autograd.dato(3.0))
autograd.atras(y)
imprimir(autograd.gradiente(x))   // 4
```

- Operaciones: `suma`, `resta`, `producto`, `division`, `relu`, `sigmoides`.
- `atras(y)` calcula los gradientes de todo el grafo con orden topológico.
- `valor(nodo)` y `gradiente(nodo)` para leer el resultado.
- `liberar(nodo)` suelta **todos** los nodos vivos del proceso, no solo los del grafo: no hay
  recolector de basura.


### 18.57 Redes

```aura
usar redes

variable red = redes.nueva(2, 4)
redes.ejemplo(red, 0.0, 1.0, 0.0, 0.0, 1.0)
redes.entrenar(red, 3000, 1.0)
imprimir(redes.predecir(red, 1.0, 0.0, 0.0, 0.0))
```

- Una capa oculta, activación sigmoide y descenso de gradiente por ejemplo.
- `nueva(entradas, ocultas)`, `ejemplo(red, x1..x4, esperado)`, `entrenar(red, vueltas, tasa)`,
  `predecir(red, x1..x4)`, `error(red)` y `liberar(red)`.
- Los pesos iniciales son pseudoaleatorios **deterministas**: el mismo programa da el mismo resultado.
- Los pesos son nodos de `autograd`: no hay un segundo motor de derivadas.


### 18.58 Campo de texto

```aura
usar controles

variable nombre = nuevo campo { x: 20, y: 20, ancho: 280, alto: 40,
                                contenido: "" + "", fondo: 0x313244, color: 0xFFFFFF,
                                marcador: "Escribe" + "" }
nombre.escribir(ventana.evento_letra(v))
nombre.borrar()
```

- `campo` hereda de `control`: `contenido`, `escribir(codigo)`, `borrar()` y `limpiar_campo()`.
- El evento de letra llega con `es_letra(clase)` y `evento_letra(manejador)`. En Windows viene de
  `WM_CHAR` y en X11 de `XLookupString`, que ya traducen teclas y acentos.
- `texto.agregar_codigo(codigo)` codifica UTF-8 y `texto.sin_ultimo()` quita un carácter completo, no
  un byte.


### 18.59 Autocompletado

- `nooki lsp` responde a `textDocument/completion` con las palabras del lenguaje, los tipos, las
  enumeraciones y las funciones del documento.
- La lista se filtra por el prefijo que se está escribiendo.
- El servidor guarda el texto de cada documento abierto para poder analizarlo.
- Pendiente: ir a la definición, formato automático y resaltado semántico.
