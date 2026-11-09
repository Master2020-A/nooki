# Pruebas de conformidad

A partir del **Sprint 5**, esta carpeta contiene los programas `.oki` que definen qué significa
"implementar Nooki". Un compilador que no pase el 100 % de estos casos no puede llamarse
«Nooki 0.1» (ver [especificación §15](../spec/especificacion-v0.1.md)).

## Estructura prevista

```
pruebas/conformidad/
├── valido/                    Programas que deben compilar y producir la salida esperada
│   └── 0001-hola-mundo/
│       ├── principal.oki
│       └── esperado.txt       Salida estándar esperada, línea por línea
│
└── invalido/                  Programas que deben ser rechazados
    └── 0001-falta-punto-y-coma/
        ├── principal.oki
        └── esperado.txt       Códigos de error esperados (uno por línea, p. ej. NOK2001)
```

## Reglas

1. **Un caso = una cosa.** Cada carpeta prueba una sola construcción del lenguaje.
2. **El nombre describe el comportamiento**, no el número: `0007-prestamo-mutable-exclusivo`.
3. **Los casos inválidos declaran el código de error**, no el texto del mensaje: así se pueden
   mejorar los mensajes sin romper la suite.
4. **Nada se borra.** Un caso que deja de fallar se convierte en caso válido; una prueba nunca
   desaparece (regresión).
5. **Cada caso lleva su comentario de cabecera** explicando qué se prueba y por qué.

## Estado actual

**22 casos** (12 válidos, 10 inválidos), todos sobre el análisis léxico. Se ejecutan con la prueba
`conformidad` de `ctest`, o directamente:

```
build/<preset>/compilador/pruebas/nooki_conformidad.exe --detalle
```

El arnés comprueba además el rendimiento del lexer sobre un corpus de 128 KB. Cuando exista el
compilador (S06 en adelante), los casos válidos también se compilarán y se comparará su salida
estándar con `esperado.txt`; los inválidos seguirán comparando códigos.
