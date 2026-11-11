# Nooki 1.0 — notas de la versión

**Un lenguaje compilado, con sintaxis en español, tipos comprobados y memoria determinista.**

---

## Qué es la 1.0

La 1.0 significa una cosa concreta: **todo lo que la especificación describe está implementado y
probado**. No es "el lenguaje completo de la visión original" —esa visión incluye interfaz gráfica,
bibliotecas de IA y control de hardware, que son fases posteriores— sino el lenguaje que hoy se puede
usar para escribir programas de verdad.

- **74 programas de conformidad** que se compilan y ejecutan en cada `ctest`: si un cambio rompe el
  código generado, se ve ahí.
- **9 suites** de pruebas, en GCC y MSVC.
- **Ningún defecto conocido.** Lo que falta está en la lista de pendientes, no escondido.

## Qué incluye

### El lenguaje

Sintaxis en español, tipado estático comprobado antes de generar código, y memoria determinista: sin
recolector de basura y sin excepciones.

```aura
funcion principal() -> entero {
    variable nombres = Vector<texto>.nuevo()
    nombres.agregar("Ana")
    para (nombre en nombres) {
        imprimir("Hola, " + nombre)
    }
    retornar 0
}
```

- **Valores**: enteros, flotantes, booleanos, caracteres y `texto`.
- **Colecciones**: `Vector<entero>`, `Vector<texto>`, `Mapa<texto, entero>`.
- **Funciones** con tipos y genéricos por instanciación explícita.
- **Estructuras** por valor y **clases** por referencia, con herencia, polimorfismo y `super`.
- **Rasgos**: contrato verificado y despacho dinámico con `&Rasgo` entre tipos no emparentados.
- **Enumeraciones con datos** y emparejamiento exhaustivo con `coincide`.
- **Errores como valores**: `resultado` y `?`, sin excepciones.
- **Genéricos de tipos**: `estructura par<T>` con una instancia por combinación.
- **Módulos** con visibilidad de funciones y tipos.

### La memoria

El rasgo que más distingue a Nooki:

| Regla | Efecto |
|---|---|
| Dueño único; pasar por valor mueve | no hay copias invisibles |
| `&` presta sin transferir | se lee sin mover |
| Devolver un compuesto transfiere lo que contiene | no queda memoria muerta detrás |
| Copiar un compuesto con dueño es error | no hay doble liberación |
| No se mueve el dueño mientras está prestado | no hay referencias colgando |
| No se devuelve un préstamo a algo local | no escapa lo que muere |
| `romper` y `continuar` liberan | no hay fugas al salir de un bucle |

**Cuando el compilador no puede garantizarlo, lo rechaza.** Esa es la regla que explica casi todas
las decisiones del lenguaje.

### La biblioteca

Cinco módulos escritos **en Nooki**, que viajan dentro del compilador:

| Módulo | Qué trae |
|---|---|
| `resultado` | `Ok(valor)`, `Error(mensaje)` |
| `matematicas` | `maximo`, `minimo`, `absoluto`, `cuadrado` |
| `textos` | `unir`, `repetir`, `contar` |
| `listas` | `suma`, `mayor`, `menor`, `promedio`, `contiene` |
| `fechas` | `bisiesto`, `dias_del_mes`, `dias_del_anio` |

### Las herramientas

```bash
nooki nuevo demo        # crea un proyecto que compila
nooki construir         # compila el proyecto del directorio actual
nooki probar            # ejecuta pruebas/*.oki y compara con su .esperado
nooki limpiar           # borra artefactos sin tocar el código
nooki compilar, ejecutar, ast, ayuda, version
```

### La documentación

- **[Guía](GUIA.md)**: doce pasos, cada uno con su programa y su salida real.
- **[Especificación](spec/especificacion-v0.1.md)**: 35 secciones, el documento normativo.
- **[Decisiones](DECISIONES.md)**: 26 ADR con el porqué de cada decisión, y los asuntos abiertos.
- **Diagnósticos**: 46 códigos `NOK` con mensaje en español e inglés, y su documentación.

## Qué no incluye

Está escrito, con su tamaño, en [DECISIONES.md](DECISIONES.md). En resumen:

| Pendiente | Tamaño | Qué daría |
|---|---|---|
| Tiempos de vida completos (duración de un préstamo entre sentencias) | medio | cerrar el último rincón teórico de la memoria |
| Paquetes con manifiesto y dependencias | medio | compartir código entre proyectos |
| Biblioteca estándar amplia (`texto` avanzado, `archivo`, `fecha` con aritmética) | medio | programas sin escribir lo básico |
| Interfaz gráfica nativa | grande | aplicaciones de escritorio |
| Bibliotecas de IA (equivalente a pandas y PyTorch) | grande | análisis de datos y modelos |
| `inseguro` y `asm!` para hardware | grande | control de dispositivos y sistemas |

## Para quién es

Para quien quiera **escribir en español** sin renunciar a un lenguaje compilado, tipado y con control
de memoria. Hoy sirve para programas de línea de comandos, procesamiento de texto y datos, y para
aprender cómo se construye un lenguaje: el compilador, sus pruebas y sus decisiones están a la vista.

## Cómo empezar

```bash
$ nooki nuevo demo
$ cd demo
$ nooki construir
$ ./principal
Hola, Nooki
```

Y después, la [guía](GUIA.md).

## Licencia

MIT o Apache-2.0, a elegir. Ver [LICENCIA](LICENCIA.md).
