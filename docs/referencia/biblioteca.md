<!-- GENERADO por herramientas/generar-referencia.py: no editar a mano -->

**19 módulos**, todos escritos en Nooki. Estas son sus declaraciones públicas,
copiadas de la fuente que va dentro del compilador.

### `accesibilidad`

```text
publico funcion luminancia(color: entero) -> entero {
```
```text
publico funcion diferencia(a: entero, b: entero) -> entero {
```
```text
publico funcion legible(texto_color: entero, fondo: entero) -> booleano {
```
```text
publico funcion texto_sobre(fondo: entero) -> entero {
```
```text
publico funcion siguiente() -> vacio {
```
```text
publico funcion anterior() -> vacio {
```
```text
publico funcion es_actual(cual: entero) -> booleano {
```
```text
publico funcion primera() -> vacio {
```

### `archivo`

```text
publico enumeracion lectura {
```
```text
publico funcion existe(ruta: &texto) -> booleano {
```
```text
publico funcion leer(ruta: &texto) -> lectura {
```
```text
publico funcion escribir(ruta: &texto, contenido: &texto) -> lectura {
```
```text
publico funcion agregar(ruta: &texto, contenido: &texto) -> lectura {
```

### `autograd`

```text
publico funcion dato(valor: flotante) -> entero {
```
```text
publico funcion valor(nodo: entero) -> flotante {
```
```text
publico funcion gradiente(nodo: entero) -> flotante {
```
```text
publico funcion suma(a: entero, b: entero) -> entero {
```
```text
publico funcion resta(a: entero, b: entero) -> entero {
```
```text
publico funcion producto(a: entero, b: entero) -> entero {
```
```text
publico funcion division(a: entero, b: entero) -> entero {
```
```text
publico funcion relu(a: entero) -> entero {
```
```text
publico funcion sigmoides(a: entero) -> entero {
```
```text
publico funcion atras(nodo: entero) -> vacio {
```
```text
publico funcion liberar(nodo: entero) -> vacio {
```

### `controles`

```text
publico funcion contiene(px: entero, py: entero) -> booleano {
```
```text
publico funcion dibujar(pantalla: entero) -> vacio {
```
```text
publico funcion dibujar(pantalla: entero) -> vacio {
```
```text
publico funcion dibujar(pantalla: entero) -> vacio {
```
```text
publico funcion pulsado(px: entero, py: entero) -> booleano {
```
```text
publico funcion dibujar(pantalla: entero) -> vacio {
```
```text
publico funcion escribir(codigo: entero) -> vacio {
```
```text
publico funcion borrar() -> vacio {
```
```text
publico funcion limpiar_campo() -> vacio {
```
```text
publico funcion dibujar(pantalla: entero) -> vacio {
```

### `csv`

```text
publico funcion filas(ruta: &texto) -> Vector<texto> {
```
```text
publico funcion campos(linea: &texto) -> Vector<texto> {
```
```text
publico funcion campos_con(linea: &texto, separador: &texto) -> Vector<texto> {
```
```text
publico funcion a_numero(campo: &texto) -> flotante {
```
```text
publico funcion a_entero(campo: &texto) -> entero {
```
```text
publico funcion columnas(ruta: &texto) -> entero {
```

### `datos`

```text
publico funcion disponible() -> booleano {
```
```text
publico funcion error() -> texto {
```
```text
publico funcion version() -> texto {
```
```text
publico funcion consultar(consulta: &texto) -> texto {
```
```text
publico funcion ejecutar(consulta: &texto) -> booleano {
```
```text
publico funcion a_csv(consulta: &texto, destino: &texto) -> booleano {
```
```text
publico funcion a_parquet(consulta: &texto, destino: &texto) -> booleano {
```

### `diseno`

```text
publico funcion centrar(disponible: entero, medida: entero) -> entero {
```
```text
publico funcion repartir(disponible: entero, partes: entero) -> entero {
```
```text
publico funcion hueco(alto_hijo: entero) -> entero {
```
```text
publico funcion cabe(alto_hijo: entero) -> booleano {
```
```text
publico funcion reiniciar() -> vacio {
```
```text
publico funcion hueco(ancho_hijo: entero) -> entero {
```
```text
publico funcion cabe(ancho_hijo: entero) -> booleano {
```
```text
publico funcion reiniciar() -> vacio {
```

### `fechas`

```text
publico funcion bisiesto(anio: entero) -> booleano {
```
```text
publico funcion dias_del_mes(mes: entero, anio: entero) -> entero {
```
```text
publico funcion dias_del_anio(anio: entero) -> entero {
```
```text
publico funcion a_dias(anio: entero, mes: entero, dia: entero) -> entero {
```
```text
publico funcion a_anio(dias: entero) -> entero {
```
```text
publico funcion a_mes(dias: entero) -> entero {
```
```text
publico funcion a_dia(dias: entero) -> entero {
```
```text
publico funcion sumar_dias(dias: entero, cantidad: entero) -> entero {
```
```text
publico funcion dia_de_la_semana(dias: entero) -> entero {
```

### `imagenes`

```text
publico funcion cargar(ruta: &texto) -> entero {
```
```text
publico funcion cargada(imagen: entero) -> booleano {
```
```text
publico funcion ancho(imagen: entero) -> entero {
```
```text
publico funcion alto(imagen: entero) -> entero {
```
```text
publico funcion color_en(imagen: entero, x: entero, y: entero) -> entero {
```
```text
publico funcion soltar(imagen: entero) -> vacio {
```
```text
publico funcion dibujar(pantalla: entero, imagen: entero, x: entero, y: entero) -> vacio {
```

### `listas`

```text
publico funcion suma(numeros: &Vector<entero>) -> entero {
```
```text
publico funcion mayor(numeros: &Vector<entero>) -> entero {
```
```text
publico funcion menor(numeros: &Vector<entero>) -> entero {
```
```text
publico funcion contiene(numeros: &Vector<entero>, buscado: entero) -> booleano {
```
```text
publico funcion promedio(numeros: &Vector<entero>) -> entero {
```

### `matematicas`

```text
publico funcion maximo(a: entero, b: entero) -> entero {
```
```text
publico funcion minimo(a: entero, b: entero) -> entero {
```
```text
publico funcion absoluto(n: entero) -> entero {
```
```text
publico funcion cuadrado(n: entero) -> entero {
```

### `pln`

```text
publico funcion normalizar(frase: &texto) -> texto {
```
```text
publico funcion tokenizar(frase: &texto) -> Vector<texto> {
```
```text
publico funcion contar(palabras: &Vector<texto>) -> Mapa<texto, entero> {
```
```text
publico funcion unicas(palabras: &Vector<texto>) -> Vector<texto> {
```
```text
publico funcion distancia(a: &texto, b: &texto) -> entero {
```
```text
publico funcion contiene_texto(palabras: &Vector<texto>, buscada: &texto) -> booleano {
```
```text
publico funcion distintas(palabras: &Vector<texto>) -> entero {
```
```text
publico funcion jaccard(a: &texto, b: &texto) -> entero {
```
```text
publico funcion cuantas(palabras: &Vector<texto>) -> entero {
```

### `redes`

```text
publico funcion nueva(entradas: entero, ocultas: entero) -> entero {
```
```text
publico funcion ejemplo(red: entero, x1: flotante, x2: flotante, x3: flotante, x4: flotante,
```
```text
publico funcion ejemplos(red: entero) -> entero {
```
```text
publico funcion guardar(red: entero, ruta: &texto) -> booleano {
```
```text
publico funcion cargar(ruta: &texto) -> entero {
```
```text
publico funcion nueva_clases(entradas: entero, ocultas: entero, clases: entero) -> entero {
```
```text
publico funcion ejemplo_clase(red: entero, x1: flotante, x2: flotante, x3: flotante,
```
```text
publico funcion entrenar_clases(red: entero, vueltas: entero, tasa: flotante) -> vacio {
```
```text
publico funcion predecir_clase(red: entero, x1: flotante, x2: flotante, x3: flotante,
```
```text
publico funcion probabilidad(red: entero, x1: flotante, x2: flotante, x3: flotante,
```
```text
publico funcion aciertos_clase(red: entero) -> entero {
```
```text
publico funcion total_clase(red: entero) -> entero {
```
```text
publico funcion liberar_clase(red: entero) -> vacio {
```
```text
publico funcion guardar_clase(red: entero, ruta: &texto) -> booleano {
```
```text
publico funcion cargar_clase(ruta: &texto) -> entero {
```
```text
publico funcion ejemplo_datos(red: entero, x: &Vector<flotante>, cual: entero) -> booleano {
```
```text
publico funcion predecir_datos(red: entero, x: &Vector<flotante>) -> entero {
```
```text
publico funcion probabilidad_datos(red: entero, x: &Vector<flotante>, cual: entero) -> flotante {
```
```text
publico funcion nueva_profunda(entradas: entero, ocultas: entero, clases: entero) -> entero {
```
```text
publico funcion ejemplo_profunda(red: entero, x: &Vector<flotante>, cual: entero) -> booleano {
```
```text
publico funcion entrenar_profunda(red: entero, vueltas: entero, tasa: flotante) -> vacio {
```
```text
publico funcion predecir_profunda(red: entero, x: &Vector<flotante>) -> entero {
```
```text
publico funcion confianza_profunda(red: entero, x: &Vector<flotante>) -> flotante {
```
```text
publico funcion aciertos_profunda(red: entero) -> entero {
```
```text
publico funcion total_profunda(red: entero) -> entero {
```
```text
publico funcion liberar_profunda(red: entero) -> vacio {
```
```text
publico funcion entrenar_profunda_lote(red: entero, vueltas: entero, tasa: flotante,
```
```text
publico funcion evaluar_profunda(red: entero, x: &Vector<flotante>, cual: entero) -> booleano {
```
```text
publico funcion guardar_profunda(red: entero, ruta: &texto) -> booleano {
```
```text
publico funcion cargar_profunda(ruta: &texto) -> entero {
```
```text
publico funcion entrenar(red: entero, vueltas: entero, tasa: flotante) -> vacio {
```
```text
publico funcion predecir(red: entero, x1: flotante, x2: flotante, x3: flotante,
```
```text
publico funcion error(red: entero) -> flotante {
```
```text
publico funcion liberar(red: entero) -> vacio {
```

### `resultado`

```text
publico enumeracion resultado {
```

### `sistema`

```text
publico funcion ejecutar(comando: &texto) -> entero {
```
```text
publico funcion capturar(comando: &texto) -> texto {
```

### `tabla`

```text
publico funcion crear(filas: entero) -> entero {
```
```text
publico funcion columna_entero(t: entero, nombre: &texto) -> entero {
```
```text
publico funcion columna_texto(t: entero, nombre: &texto) -> entero {
```
```text
publico funcion filas(t: entero) -> entero {
```
```text
publico funcion columnas(t: entero) -> entero {
```
```text
publico funcion nombre_columna(t: entero, columna: entero) -> texto {
```
```text
publico funcion poner_entero(t: entero, fila: entero, columna: entero, valor: entero) -> vacio {
```
```text
publico funcion poner_texto(t: entero, fila: entero, columna: entero, valor: &texto) -> vacio {
```
```text
publico funcion valor_entero(t: entero, fila: entero, columna: entero) -> entero {
```
```text
publico funcion valor_texto(t: entero, fila: entero, columna: entero) -> texto {
```
```text
publico funcion suma(t: entero, columna: entero) -> entero {
```
```text
publico funcion media(t: entero, columna: entero) -> entero {
```
```text
publico funcion minimo(t: entero, columna: entero) -> entero {
```
```text
publico funcion maximo(t: entero, columna: entero) -> entero {
```
```text
publico funcion filtrar(t: entero, columna: entero, valor: &texto) -> entero {
```
```text
publico funcion ordenar(t: entero, columna: entero) -> entero {
```
```text
publico funcion contar_por(t: entero, columna: entero) -> entero {
```
```text
publico funcion liberar(t: entero) -> vacio {
```

### `tensor`

```text
publico funcion ceros(filas: entero, columnas: entero) -> entero {
```
```text
publico funcion lleno(filas: entero, columnas: entero, valor: entero) -> entero {
```
```text
publico funcion identidad(lado: entero) -> entero {
```
```text
publico funcion filas(t: entero) -> entero {
```
```text
publico funcion columnas(t: entero) -> entero {
```
```text
publico funcion leer(t: entero, fila: entero, columna: entero) -> entero {
```
```text
publico funcion poner(t: entero, fila: entero, columna: entero, valor: entero) -> vacio {
```
```text
publico funcion suma(a: entero, b: entero) -> entero {
```
```text
publico funcion resta(a: entero, b: entero) -> entero {
```
```text
publico funcion escalar(a: entero, factor: entero) -> entero {
```
```text
publico funcion producto(a: entero, b: entero) -> entero {
```
```text
publico funcion transpuesta(a: entero) -> entero {
```
```text
publico funcion determinante(a: entero) -> entero {
```
```text
publico funcion suma_total(a: entero) -> entero {
```
```text
publico funcion iguales(a: entero, b: entero) -> booleano {
```
```text
publico funcion liberar(a: entero) -> vacio {
```
```text
publico funcion minimo(t: entero) -> entero {
```
```text
publico funcion maximo(t: entero) -> entero {
```
```text
publico funcion media(t: entero) -> entero {
```
```text
publico funcion varianza(t: entero) -> entero {
```
```text
publico funcion desviacion(t: entero) -> entero {
```
```text
publico funcion normalizar(t: entero) -> entero {
```
```text
publico funcion relu(t: entero) -> entero {
```
```text
publico funcion sigmoides(t: entero) -> entero {
```
```text
publico funcion softmax(t: entero) -> entero {
```

### `textos`

```text
publico funcion de_entero(valor: entero) -> texto {
```
```text
publico funcion de_flotante(valor: flotante) -> texto {
```
```text
publico funcion unir(partes: &Vector<texto>, separador: texto) -> texto {
```
```text
publico funcion repetir(pieza: &texto, veces: entero) -> texto {
```
```text
publico funcion contar(pieza: &texto, aguja: &texto) -> entero {
```

### `ventana`

```text
publico funcion abrir(titulo: &texto, ancho: entero, alto: entero) -> entero {
```
```text
publico funcion viva(manejador: entero) -> booleano {
```
```text
publico funcion siguiente_evento(manejador: entero) -> entero {
```
```text
publico funcion cerrar(manejador: entero) -> vacio {
```
```text
publico funcion pausa(milisegundos: entero) -> vacio {
```
```text
publico funcion repintar(manejador: entero) -> booleano {
```
```text
publico funcion evento(manejador: entero) -> entero {
```
```text
publico funcion evento_x(manejador: entero) -> entero {
```
```text
publico funcion evento_y(manejador: entero) -> entero {
```
```text
publico funcion evento_tecla(manejador: entero) -> entero {
```
```text
publico funcion es_cerrar(cual: entero) -> booleano { retornar cual == 1 }
```
```text
publico funcion es_movimiento(cual: entero) -> booleano { retornar cual == 2 }
```
```text
publico funcion es_pulsacion(cual: entero) -> booleano { retornar cual == 3 }
```
```text
publico funcion es_liberacion(cual: entero) -> booleano { retornar cual == 4 }
```
```text
publico funcion es_tecla(cual: entero) -> booleano { retornar cual == 5 }
```
```text
publico funcion es_letra(cual: entero) -> booleano { retornar cual == 6 }
```
```text
publico funcion evento_letra(manejador: entero) -> entero {
```
```text
publico funcion limpiar(manejador: entero, color: entero) -> vacio {
```
```text
publico funcion rectangulo(manejador: entero, x: entero, y: entero, ancho: entero,
```
```text
publico funcion linea(manejador: entero, x1: entero, y1: entero, x2: entero, y2: entero,
```
```text
publico funcion escribir_texto(manejador: entero, x: entero, y: entero, contenido: &texto,
```
```text
publico funcion presentar(manejador: entero) -> vacio {
```
```text
publico funcion color_en(manejador: entero, x: entero, y: entero) -> entero {
```

