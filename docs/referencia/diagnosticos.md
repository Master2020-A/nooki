<!-- GENERADO por herramientas/generar-referencia.py: no editar a mano -->

Hay **47 códigos**. El mensaje es el que imprime el compilador, con sus
huecos entre llaves.

| Código | Constante | Mensaje |
|---|---|---|
| `NOK0001` | `ARCHIVO_NO_ENCONTRADO` | no se pudo leer el archivo «{archivo}» |
| `NOK1001` | `PALABRA_RESERVADA_CON_TILDE` | «{palabra}» no es una palabra reservada: las palabras clave se escriben sin tilde |
| `NOK1003` | `TEXTO_SIN_CERRAR` | texto sin cerrar |
| `NOK1004` | `CARACTER_INVALIDO` | el carácter «{caracter}» no se usa en Nooki |
| `NOK1005` | `NUMERO_MAL_FORMADO` | número mal formado |
| `NOK1006` | `UTF8_INVALIDO` | secuencia UTF-8 no válida |
| `NOK1007` | `CARACTER_LITERAL_SIN_CERRAR` | falta cerrar el carácter literal |
| `NOK1008` | `COMENTARIO_SIN_CERRAR` | comentario sin cerrar |
| `NOK1009` | `ESCAPE_DESCONOCIDO` | la secuencia de escape «{escape}» no existe |
| `NOK2001` | `PARENTESIS_SIN_CERRAR` | falta cerrar «{apertura}» |
| `NOK2002` | `TOKEN_INESPERADO` | se esperaba {esperado} y se encontró «{encontrado}» |
| `NOK2003` | `SIN_PRINCIPAL` | el programa no declara una función «principal» |
| `NOK2004` | `NO_SOPORTADO` | «{construccion}» todavía no está soportado |
| `NOK2005` | `RANGO_ENCADENADO` | los rangos no se pueden encadenar |
| `NOK3001` | `NOMBRE_NO_DECLARADO` | no se ha declarado «{nombre}» |
| `NOK3002` | `FUNCION_NO_DECLARADA` | no existe ninguna función «{nombre}» |
| `NOK3003` | `TIPO_DESCONOCIDO` | no existe ningún tipo «{nombre}» |
| `NOK3004` | `CAMPO_DESCONOCIDO` | «{tipo}» no tiene ningún campo «{campo}» |
| `NOK3005` | `CAMPO_FALTANTE` | a «{tipo}» le falta el campo «{campo}» |
| `NOK3006` | `MIEMBRO_DESCONOCIDO` | «{tipo}» no tiene ningún miembro «{nombre}» |
| `NOK3007` | `VARIANTE_DESCONOCIDA` | «{enumeracion}» no tiene ninguna variante «{variante}» |
| `NOK3008` | `MODULO_NO_ENCONTRADO` | no se encontró el módulo «{modulo}» |
| `NOK3009` | `SIMBOLO_PRIVADO` | «{simbolo}» es privado del módulo «{modulo}» |
| `NOK3011` | `METODO_DE_RASGO_FALTANTE` | a «{tipo}» le falta «{metodo}», que exige el rasgo «{rasgo}» |
| `NOK3012` | `ASM_FUERA_DE_INSEGURO` | «asm!» solo se permite dentro de un bloque «inseguro { }» |
| `NOK3013` | `PUNTERO_FUERA_DE_INSEGURO` | los punteros solo se usan dentro de un bloque «inseguro { }» |
| `NOK4001` | `TIPO_INCOMPATIBLE` | se esperaba un valor de tipo «{esperado}» pero se encontró «{encontrado}» |
| `NOK4002` | `ARGUMENTO_INCOMPATIBLE` | el argumento «{parametro}» de «{funcion}» espera «{esperado}» y se pasa «{encontrado}» |
| `NOK4003` | `ARIDAD_INCORRECTA` | «{funcion}» espera {esperados} argumento(s) y se han pasado {dados} |
| `NOK4004` | `RETORNO_INCOMPATIBLE` | «{funcion}» debe devolver «{esperado}» y se devuelve «{encontrado}» |
| `NOK4005` | `CONDICION_NO_BOOLEANA` | la condición debe ser de tipo «booleano» y es «{tipo}» |
| `NOK4006` | `COINCIDE_NO_EXHAUSTIVO` | el «coincide» no cubre todos los casos: añada un caso «_» |
| `NOK4007` | `DEREFERENCIA_INVALIDA` | «*» solo se puede aplicar a una «Caja» |
| `NOK4008` | `PRESTAMO_INVALIDO` | solo se puede prestar una variable que sea una «Caja» |
| `NOK4009` | `ESCRITURA_EN_PRESTAMO_INMUTABLE` | no se puede escribir a través de un préstamo inmutable: use «&mutable» |
| `NOK4010` | `FIRMA_INCOMPATIBLE` | «{metodo}» no coincide con la firma de «{base}.{metodo}» |
| `NOK4011` | `PROPAGACION_SIN_RESULTADO` | «?» solo se usa en una función que devuelve «Resultado» |
| `NOK4012` | `FIRMA_DE_RASGO` | «{metodo}» no coincide con la firma que exige el rasgo «{rasgo}» |
| `NOK5001` | `USO_TRAS_MOVER` | no se puede usar «{nombre}»: el valor se movió antes |
| `NOK5002` | `PRESTAMO_GUARDADO` | un préstamo no se puede guardar en una variable: solo vive durante la llamada |
| `NOK5004` | `COPIA_COMPUESTO` | «{nombre}» tiene campos con dueño y no se puede copiar: páselo con «&» o construya otro |
| `NOK5006` | `PRESTAMO_QUE_ESCAPA` | no se puede devolver un préstamo a «{nombre}»: es local y muere al salir de la función |
| `NOK5007` | `MOVER_CON_PRESTAMO` | no se puede mover ni reasignar «{nombre}»: «{prestamo}» lo está prestando |
| `NOK5003` | `PRESTAMO_EXCLUSIVO` | «{nombre}» no puede prestarse mutable y compartido a la vez en la misma llamada |
| `NOK6001` | `SIN_COMPILADOR_C` | no se encontró un compilador de C; instale gcc, clang o MSVC, o indique uno con  |
| `NOK6002` | `FALLO_COMPILADOR_C` | el compilador de C «{compilador}» terminó con el código {codigo} |
| `NOK6003` | `BACKEND_NO_SOPORTA` | el backend todavía no sabe traducir {construccion} |
