#include "nooki/ia/runtime_ia.hpp"

namespace nooki::ia {

std::string_view fuente_runtime_ia() {
    return R"(
/* --- Tensores de Nooki (generado por el compilador) ---
 * Matrices de enteros de 64 bits con forma (filas x columnas). Todas las
 * operaciones comprueban las dimensiones: si no encajan devuelven 0 en lugar de
 * leer fuera de sitio. Un manejador es un puntero a la estructura. */

typedef struct {
    int filas;
    int columnas;
    long long* datos;
} nooki_tensor;

static nooki_tensor* nooki_tensor_de(long long manejador) {
    return (nooki_tensor*)(intptr_t)manejador;
}

static long long nooki_tensor_nuevo(int filas, int columnas, long long valor) {
    if (filas <= 0 || columnas <= 0 || filas > 100000 || columnas > 100000) {
        return 0;
    }
    nooki_tensor* tensor = (nooki_tensor*)malloc(sizeof(nooki_tensor));
    if (tensor == NULL) {
        return 0;
    }
    tensor->filas = filas;
    tensor->columnas = columnas;
    const size_t total = (size_t)filas * (size_t)columnas;
    tensor->datos = (long long*)malloc(total * sizeof(long long));
    if (tensor->datos == NULL) {
        free(tensor);
        return 0;
    }
    for (size_t i = 0; i < total; ++i) {
        tensor->datos[i] = valor;
    }
    return (long long)(intptr_t)tensor;
}

long long nooki_ia_tensor_ceros(long long filas, long long columnas) {
    return nooki_tensor_nuevo((int)filas, (int)columnas, 0);
}

long long nooki_ia_tensor_lleno(long long filas, long long columnas, long long valor) {
    return nooki_tensor_nuevo((int)filas, (int)columnas, valor);
}

long long nooki_ia_tensor_identidad(long long lado) {
    if (lado <= 0) {
        return 0;
    }
    const long long tensor = nooki_tensor_nuevo((int)lado, (int)lado, 0);
    nooki_tensor* t = nooki_tensor_de(tensor);
    if (t == NULL) {
        return 0;
    }
    for (int i = 0; i < t->filas; ++i) {
        t->datos[(size_t)i * t->columnas + i] = 1;
    }
    return tensor;
}

long long nooki_ia_tensor_filas(long long manejador) {
    nooki_tensor* t = nooki_tensor_de(manejador);
    return t == NULL ? 0 : t->filas;
}

long long nooki_ia_tensor_columnas(long long manejador) {
    nooki_tensor* t = nooki_tensor_de(manejador);
    return t == NULL ? 0 : t->columnas;
}

long long nooki_ia_tensor_leer(long long manejador, long long fila, long long columna) {
    nooki_tensor* t = nooki_tensor_de(manejador);
    if (t == NULL || fila < 0 || columna < 0 || fila >= t->filas || columna >= t->columnas) {
        return 0;
    }
    return t->datos[(size_t)fila * t->columnas + columna];
}

long long nooki_ia_tensor_poner(long long manejador, long long fila, long long columna,
                                long long valor) {
    nooki_tensor* t = nooki_tensor_de(manejador);
    if (t == NULL || fila < 0 || columna < 0 || fila >= t->filas || columna >= t->columnas) {
        return 0;
    }
    t->datos[(size_t)fila * t->columnas + columna] = valor;
    return 1;
}

void nooki_ia_tensor_liberar(long long manejador) {
    nooki_tensor* t = nooki_tensor_de(manejador);
    if (t == NULL) {
        return;
    }
    free(t->datos);
    free(t);
}

/* Suma o resta elemento a elemento; 0 si las formas no coinciden. */
static long long nooki_tensor_binaria(long long a_manejador, long long b_manejador, int signo) {
    nooki_tensor* a = nooki_tensor_de(a_manejador);
    nooki_tensor* b = nooki_tensor_de(b_manejador);
    if (a == NULL || b == NULL || a->filas != b->filas || a->columnas != b->columnas) {
        return 0;
    }
    const long long salida = nooki_tensor_nuevo(a->filas, a->columnas, 0);
    nooki_tensor* s = nooki_tensor_de(salida);
    if (s == NULL) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    for (size_t i = 0; i < total; ++i) {
        s->datos[i] = signo > 0 ? a->datos[i] + b->datos[i] : a->datos[i] - b->datos[i];
    }
    return salida;
}

long long nooki_ia_tensor_suma(long long a, long long b) {
    return nooki_tensor_binaria(a, b, 1);
}

long long nooki_ia_tensor_resta(long long a, long long b) {
    return nooki_tensor_binaria(a, b, -1);
}

/* Multiplica todos los elementos por un número. */
long long nooki_ia_tensor_escalar(long long manejador, long long factor) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL) {
        return 0;
    }
    const long long salida = nooki_tensor_nuevo(a->filas, a->columnas, 0);
    nooki_tensor* s = nooki_tensor_de(salida);
    if (s == NULL) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    for (size_t i = 0; i < total; ++i) {
        s->datos[i] = a->datos[i] * factor;
    }
    return salida;
}

/* Producto matricial: (n x m) por (m x p) da (n x p). */
long long nooki_ia_tensor_producto(long long a_manejador, long long b_manejador) {
    nooki_tensor* a = nooki_tensor_de(a_manejador);
    nooki_tensor* b = nooki_tensor_de(b_manejador);
    if (a == NULL || b == NULL || a->columnas != b->filas) {
        return 0;
    }
    const long long salida = nooki_tensor_nuevo(a->filas, b->columnas, 0);
    nooki_tensor* s = nooki_tensor_de(salida);
    if (s == NULL) {
        return 0;
    }
    for (int i = 0; i < a->filas; ++i) {
        for (int j = 0; j < b->columnas; ++j) {
            long long suma = 0;
            for (int k = 0; k < a->columnas; ++k) {
                suma += a->datos[(size_t)i * a->columnas + k] *
                        b->datos[(size_t)k * b->columnas + j];
            }
            s->datos[(size_t)i * s->columnas + j] = suma;
        }
    }
    return salida;
}

long long nooki_ia_tensor_transpuesta(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL) {
        return 0;
    }
    const long long salida = nooki_tensor_nuevo(a->columnas, a->filas, 0);
    nooki_tensor* s = nooki_tensor_de(salida);
    if (s == NULL) {
        return 0;
    }
    for (int i = 0; i < a->filas; ++i) {
        for (int j = 0; j < a->columnas; ++j) {
            s->datos[(size_t)j * s->columnas + i] = a->datos[(size_t)i * a->columnas + j];
        }
    }
    return salida;
}

/* Determinante por eliminación de Gauss con doble precisión. */
long long nooki_ia_tensor_determinante(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL || a->filas != a->columnas) {
        return 0;
    }
    const int n = a->filas;
    double* copia = (double*)malloc((size_t)n * (size_t)n * sizeof(double));
    if (copia == NULL) {
        return 0;
    }
    for (size_t i = 0; i < (size_t)n * (size_t)n; ++i) {
        copia[i] = (double)a->datos[i];
    }

    double determinante = 1.0;
    for (int columna = 0; columna < n; ++columna) {
        int pivote = columna;
        for (int fila = columna + 1; fila < n; ++fila) {
            const double actual = copia[(size_t)fila * n + columna];
            const double mejor = copia[(size_t)pivote * n + columna];
            if ((actual < 0 ? -actual : actual) > (mejor < 0 ? -mejor : mejor)) {
                pivote = fila;
            }
        }
        const double valor = copia[(size_t)pivote * n + columna];
        if (valor == 0.0) {
            free(copia);
            return 0;
        }
        if (pivote != columna) {
            for (int j = 0; j < n; ++j) {
                const double temporal = copia[(size_t)columna * n + j];
                copia[(size_t)columna * n + j] = copia[(size_t)pivote * n + j];
                copia[(size_t)pivote * n + j] = temporal;
            }
            determinante = -determinante;
        }
        determinante *= copia[(size_t)columna * n + columna];
        for (int fila = columna + 1; fila < n; ++fila) {
            const double factor = copia[(size_t)fila * n + columna] / copia[(size_t)columna * n + columna];
            for (int j = columna; j < n; ++j) {
                copia[(size_t)fila * n + j] -= factor * copia[(size_t)columna * n + j];
            }
        }
    }
    free(copia);
    return (long long)(determinante < 0 ? determinante - 0.5 : determinante + 0.5);
}

/* Suma de todos los elementos. */
long long nooki_ia_tensor_suma_total(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    long long suma = 0;
    for (size_t i = 0; i < total; ++i) {
        suma += a->datos[i];
    }
    return suma;
}

/* --- Estadística. Los resultados con decimales se dan por mil. --- */

long long nooki_ia_tensor_minimo(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL || a->filas == 0 || a->columnas == 0) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    long long minimo = a->datos[0];
    for (size_t i = 1; i < total; ++i) {
        if (a->datos[i] < minimo) {
            minimo = a->datos[i];
        }
    }
    return minimo;
}

long long nooki_ia_tensor_maximo(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL || a->filas == 0 || a->columnas == 0) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    long long maximo = a->datos[0];
    for (size_t i = 1; i < total; ++i) {
        if (a->datos[i] > maximo) {
            maximo = a->datos[i];
        }
    }
    return maximo;
}

/* Media de todos los elementos, por mil. */
long long nooki_ia_tensor_media(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    if (total == 0) {
        return 0;
    }
    double suma = 0.0;
    for (size_t i = 0; i < total; ++i) {
        suma += (double)a->datos[i];
    }
    const double media = suma / (double)total;
    return (long long)(media < 0 ? media * 1000.0 - 0.5 : media * 1000.0 + 0.5);
}

/* Varianza poblacional, por mil al cuadrado (escala de la media). */
long long nooki_ia_tensor_varianza(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    if (total == 0) {
        return 0;
    }
    double suma = 0.0;
    for (size_t i = 0; i < total; ++i) {
        suma += (double)a->datos[i];
    }
    const double media = suma / (double)total;
    double acumulado = 0.0;
    for (size_t i = 0; i < total; ++i) {
        const double diferencia = (double)a->datos[i] - media;
        acumulado += diferencia * diferencia;
    }
    const double varianza = acumulado / (double)total;
    return (long long)(varianza * 1000.0 + 0.5);
}

/* Desviación típica, por mil. */
long long nooki_ia_tensor_desviacion(long long manejador) {
    const long long varianza = nooki_ia_tensor_varianza(manejador);
    if (varianza <= 0) {
        return 0;
    }
    /* La varianza viene por mil, así que hay que multiplicarla otra vez por mil
     * para que la raíz quede también por mil. */
    double raiz = (double)varianza * 1000.0;
    double estimacion = raiz > 1.0 ? raiz / 2.0 : 1.0;
    for (int i = 0; i < 40; ++i) {
        estimacion = 0.5 * (estimacion + raiz / estimacion);
    }
    return (long long)(estimacion + 0.5);
}

/* Lleva los valores a 0..1000 conservando las proporciones. */
long long nooki_ia_tensor_normalizar(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    if (total == 0) {
        return 0;
    }
    const long long minimo = nooki_ia_tensor_minimo(manejador);
    const long long maximo = nooki_ia_tensor_maximo(manejador);
    const long long rango = maximo - minimo;
    const long long salida = nooki_tensor_nuevo(a->filas, a->columnas, 0);
    nooki_tensor* s = nooki_tensor_de(salida);
    if (s == NULL) {
        return 0;
    }
    for (size_t i = 0; i < total; ++i) {
        s->datos[i] = rango == 0 ? 0 : (a->datos[i] - minimo) * 1000 / rango;
    }
    return salida;
}

/* --- Activaciones --- */

long long nooki_ia_tensor_relu(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL) {
        return 0;
    }
    const long long salida = nooki_tensor_nuevo(a->filas, a->columnas, 0);
    nooki_tensor* s = nooki_tensor_de(salida);
    if (s == NULL) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    for (size_t i = 0; i < total; ++i) {
        s->datos[i] = a->datos[i] > 0 ? a->datos[i] : 0;
    }
    return salida;
}

static double nooki_tensor_exp(double x) {
    /* Serie de Taylor para x negativos moderados, que es el uso de la sigmoide. */
    if (x > 40.0) {
        return 1e17;
    }
    if (x < -40.0) {
        return 0.0;
    }
    double termino = 1.0;
    double suma = 1.0;
    for (int i = 1; i < 60; ++i) {
        termino *= x / (double)i;
        suma += termino;
    }
    return suma;
}

/* Sigmoide por mil: 0..1000. */
long long nooki_ia_tensor_sigmoides(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL) {
        return 0;
    }
    const long long salida = nooki_tensor_nuevo(a->filas, a->columnas, 0);
    nooki_tensor* s = nooki_tensor_de(salida);
    if (s == NULL) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    for (size_t i = 0; i < total; ++i) {
        const double valor = 1000.0 / (1.0 + nooki_tensor_exp(-(double)a->datos[i]));
        s->datos[i] = (long long)(valor + 0.5);
    }
    return salida;
}

/* Softmax por mil: cada elemento entre 0 y 1000, y el total suma 1000. */
long long nooki_ia_tensor_softmax(long long manejador) {
    nooki_tensor* a = nooki_tensor_de(manejador);
    if (a == NULL) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    if (total == 0) {
        return 0;
    }
    const long long salida = nooki_tensor_nuevo(a->filas, a->columnas, 0);
    nooki_tensor* s = nooki_tensor_de(salida);
    if (s == NULL) {
        return 0;
    }
    /* Se resta el máximo para que la exponencial no se dispare. */
    const long long maximo = nooki_ia_tensor_maximo(manejador);
    double suma = 0.0;
    double* temporal = (double*)malloc(total * sizeof(double));
    if (temporal == NULL) {
        nooki_ia_tensor_liberar(salida);
        return 0;
    }
    for (size_t i = 0; i < total; ++i) {
        temporal[i] = nooki_tensor_exp((double)(a->datos[i] - maximo));
        suma += temporal[i];
    }
    long long repartido = 0;
    for (size_t i = 0; i < total; ++i) {
        s->datos[i] = (long long)(temporal[i] / suma * 1000.0);
        repartido += s->datos[i];
    }
    /* El redondeo deja algunos puntos sueltos: se le dan al mayor. */
    if (repartido != 1000) {
        int mayor = 0;
        for (size_t i = 1; i < total; ++i) {
            if (s->datos[i] > s->datos[mayor]) {
                mayor = (int)i;
            }
        }
        s->datos[mayor] += 1000 - repartido;
    }
    free(temporal);
    return salida;
}

/* --- Autograd: derivadas automáticas sobre un grafo de operaciones ---
 * Cada nodo guarda su valor, su gradiente y de dónde viene. Todos los nodos
 * viven en una lista global: así se limpian los gradientes y se libera todo de
 * una vez, sin recorrer el grafo dos veces ni compartir áreas. */

typedef struct nooki_auto_nodo {
    double valor;
    double gradiente;
    int operacion; /* 0 dato, 1 suma, 2 resta, 3 producto, 4 división, 5 relu, 6 sigmoide */
    struct nooki_auto_nodo* izquierda;
    struct nooki_auto_nodo* derecha;
    struct nooki_auto_nodo* siguiente_global;
    int visitado;
} nooki_auto_nodo;

static nooki_auto_nodo* nooki_auto_todos = NULL;

static nooki_auto_nodo* nooki_auto_nodo_de(long long manejador) {
    return (nooki_auto_nodo*)(intptr_t)manejador;
}

static nooki_auto_nodo* nooki_auto_crear(double valor, int operacion, nooki_auto_nodo* izquierda,
                                         nooki_auto_nodo* derecha) {
    nooki_auto_nodo* nodo = (nooki_auto_nodo*)malloc(sizeof(nooki_auto_nodo));
    if (nodo == NULL) {
        return NULL;
    }
    nodo->valor = valor;
    nodo->gradiente = 0.0;
    nodo->operacion = operacion;
    nodo->izquierda = izquierda;
    nodo->derecha = derecha;
    nodo->visitado = 0;
    nodo->siguiente_global = nooki_auto_todos;
    nooki_auto_todos = nodo;
    return nodo;
}

long long nooki_ia_auto_dato(double valor) {
    return (long long)(intptr_t)nooki_auto_crear(valor, 0, NULL, NULL);
}

double nooki_ia_auto_valor(long long manejador) {
    nooki_auto_nodo* nodo = nooki_auto_nodo_de(manejador);
    return nodo == NULL ? 0.0 : nodo->valor;
}

double nooki_ia_auto_gradiente(long long manejador) {
    nooki_auto_nodo* nodo = nooki_auto_nodo_de(manejador);
    return nodo == NULL ? 0.0 : nodo->gradiente;
}

static long long nooki_auto_binaria(long long a_manejador, long long b_manejador, int operacion,
                                    double valor) {
    nooki_auto_nodo* a = nooki_auto_nodo_de(a_manejador);
    nooki_auto_nodo* b = nooki_auto_nodo_de(b_manejador);
    if (a == NULL || b == NULL) {
        return 0;
    }
    return (long long)(intptr_t)nooki_auto_crear(valor, operacion, a, b);
}

long long nooki_ia_auto_suma(long long a, long long b) {
    nooki_auto_nodo* na = nooki_auto_nodo_de(a);
    nooki_auto_nodo* nb = nooki_auto_nodo_de(b);
    if (na == NULL || nb == NULL) {
        return 0;
    }
    return nooki_auto_binaria(a, b, 1, na->valor + nb->valor);
}

long long nooki_ia_auto_resta(long long a, long long b) {
    nooki_auto_nodo* na = nooki_auto_nodo_de(a);
    nooki_auto_nodo* nb = nooki_auto_nodo_de(b);
    if (na == NULL || nb == NULL) {
        return 0;
    }
    return nooki_auto_binaria(a, b, 2, na->valor - nb->valor);
}

long long nooki_ia_auto_producto(long long a, long long b) {
    nooki_auto_nodo* na = nooki_auto_nodo_de(a);
    nooki_auto_nodo* nb = nooki_auto_nodo_de(b);
    if (na == NULL || nb == NULL) {
        return 0;
    }
    return nooki_auto_binaria(a, b, 3, na->valor * nb->valor);
}

long long nooki_ia_auto_division(long long a, long long b) {
    nooki_auto_nodo* na = nooki_auto_nodo_de(a);
    nooki_auto_nodo* nb = nooki_auto_nodo_de(b);
    if (na == NULL || nb == NULL || nb->valor == 0.0) {
        return 0;
    }
    return nooki_auto_binaria(a, b, 4, na->valor / nb->valor);
}

static double nooki_auto_exponencial(double x) {
    if (x > 40.0) {
        return 1e17;
    }
    if (x < -40.0) {
        return 0.0;
    }
    double termino = 1.0;
    double suma = 1.0;
    for (int i = 1; i < 60; ++i) {
        termino *= x / (double)i;
        suma += termino;
    }
    return suma;
}

long long nooki_ia_auto_relu(long long manejador) {
    nooki_auto_nodo* a = nooki_auto_nodo_de(manejador);
    if (a == NULL) {
        return 0;
    }
    return (long long)(intptr_t)nooki_auto_crear(a->valor > 0.0 ? a->valor : 0.0, 5, a, NULL);
}

long long nooki_ia_auto_sigmoides(long long manejador) {
    nooki_auto_nodo* a = nooki_auto_nodo_de(manejador);
    if (a == NULL) {
        return 0;
    }
    const double valor = 1.0 / (1.0 + nooki_auto_exponencial(-a->valor));
    return (long long)(intptr_t)nooki_auto_crear(valor, 6, a, NULL);
}

typedef struct {
    nooki_auto_nodo** datos;
    int cuantos;
    int capacidad;
} nooki_auto_pila;

static void nooki_auto_orden(nooki_auto_nodo* nodo, nooki_auto_pila* pila) {
    if (nodo == NULL || nodo->visitado) {
        return;
    }
    nodo->visitado = 1;
    nooki_auto_orden(nodo->izquierda, pila);
    nooki_auto_orden(nodo->derecha, pila);
    if (pila->cuantos >= pila->capacidad) {
        const int nueva_capacidad = pila->capacidad == 0 ? 16 : pila->capacidad * 2;
        nooki_auto_nodo** nueva = (nooki_auto_nodo**)realloc(
            pila->datos, (size_t)nueva_capacidad * sizeof(nooki_auto_nodo*));
        if (nueva == NULL) {
            return;
        }
        pila->datos = nueva;
        pila->capacidad = nueva_capacidad;
    }
    pila->datos[pila->cuantos++] = nodo;
}

void nooki_ia_auto_atras(long long manejador) {
    nooki_auto_nodo* raiz = nooki_auto_nodo_de(manejador);
    if (raiz == NULL) {
        return;
    }
    /* Se limpian gradientes y marcas de todos los nodos vivos. */
    for (nooki_auto_nodo* nodo = nooki_auto_todos; nodo != NULL; nodo = nodo->siguiente_global) {
        nodo->gradiente = 0.0;
        nodo->visitado = 0;
    }

    nooki_auto_pila pila;
    pila.datos = NULL;
    pila.cuantos = 0;
    pila.capacidad = 0;
    nooki_auto_orden(raiz, &pila);
    if (pila.datos == NULL) {
        return;
    }

    raiz->gradiente = 1.0;
    /* La pila va de hojas a raíz: se recorre al revés, de la raíz a las hojas. */
    for (int i = pila.cuantos - 1; i >= 0; --i) {
        nooki_auto_nodo* nodo = pila.datos[i];
        const double g = nodo->gradiente;
        switch (nodo->operacion) {
            case 1:
                if (nodo->izquierda) nodo->izquierda->gradiente += g;
                if (nodo->derecha) nodo->derecha->gradiente += g;
                break;
            case 2:
                if (nodo->izquierda) nodo->izquierda->gradiente += g;
                if (nodo->derecha) nodo->derecha->gradiente -= g;
                break;
            case 3:
                if (nodo->izquierda) nodo->izquierda->gradiente += g * nodo->derecha->valor;
                if (nodo->derecha) nodo->derecha->gradiente += g * nodo->izquierda->valor;
                break;
            case 4:
                if (nodo->izquierda) nodo->izquierda->gradiente += g / nodo->derecha->valor;
                if (nodo->derecha) {
                    nodo->derecha->gradiente -=
                        g * nodo->izquierda->valor /
                        (nodo->derecha->valor * nodo->derecha->valor);
                }
                break;
            case 5:
                if (nodo->izquierda) nodo->izquierda->gradiente += nodo->valor > 0.0 ? g : 0.0;
                break;
            case 6:
                if (nodo->izquierda) {
                    nodo->izquierda->gradiente += g * nodo->valor * (1.0 - nodo->valor);
                }
                break;
            default:
                break;
        }
    }
    free(pila.datos);
}

/* Libera todos los nodos vivos. Los manejadores dejan de ser válidos. */
void nooki_ia_auto_liberar(long long manejador) {
    (void)manejador;
    nooki_auto_nodo* nodo = nooki_auto_todos;
    while (nodo != NULL) {
        nooki_auto_nodo* siguiente = nodo->siguiente_global;
        free(nodo);
        nodo = siguiente;
    }
    nooki_auto_todos = NULL;
}
/* --- Tablas: columnas con nombre, como un DataFrame ---
 * Cada columna es numérica o de texto. Las de texto guardan copias propias. */

typedef struct {
    char* nombre;
    int es_texto;
    long long* numeros;
    char** textos;
} nooki_columna;

typedef struct {
    int filas;
    int columnas;
    nooki_columna* columnas_datos;
} nooki_tabla;

void nooki_ia_tabla_liberar(long long manejador);

static nooki_tabla* nooki_tabla_de(long long manejador) {
    return (nooki_tabla*)(intptr_t)manejador;
}

static char* nooki_tabla_copiar(const char* texto) {
    if (texto == NULL) {
        texto = "";
    }
    const size_t largo = strlen(texto);
    char* copia = (char*)malloc(largo + 1);
    if (copia == NULL) {
        return NULL;
    }
    memcpy(copia, texto, largo + 1);
    return copia;
}

long long nooki_ia_tabla_crear(long long filas) {
    if (filas < 0 || filas > 10000000) {
        return 0;
    }
    nooki_tabla* tabla = (nooki_tabla*)malloc(sizeof(nooki_tabla));
    if (tabla == NULL) {
        return 0;
    }
    tabla->filas = (int)filas;
    tabla->columnas = 0;
    tabla->columnas_datos = NULL;
    return (long long)(intptr_t)tabla;
}

/* Añade una columna y devuelve su índice, o -1 si no se puede. */
static long long nooki_tabla_anadir(long long manejador, const char* nombre, int es_texto) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL) {
        return -1;
    }
    const int cuantas = tabla->columnas + 1;
    nooki_columna* columnas =
        (nooki_columna*)realloc(tabla->columnas_datos, (size_t)cuantas * sizeof(nooki_columna));
    if (columnas == NULL) {
        return -1;
    }
    tabla->columnas_datos = columnas;
    nooki_columna* columna = &columnas[tabla->columnas];
    columna->nombre = nooki_tabla_copiar(nombre);
    columna->es_texto = es_texto;
    columna->numeros = NULL;
    columna->textos = NULL;

    const size_t filas = (size_t)tabla->filas;
    if (es_texto) {
        columna->textos = (char**)calloc(filas == 0 ? 1 : filas, sizeof(char*));
        if (columna->textos == NULL) {
            free(columna->nombre);
            return -1;
        }
    } else {
        columna->numeros = (long long*)calloc(filas == 0 ? 1 : filas, sizeof(long long));
        if (columna->numeros == NULL) {
            free(columna->nombre);
            return -1;
        }
    }
    ++tabla->columnas;
    return tabla->columnas - 1;
}

long long nooki_ia_tabla_columna_entero(long long manejador, const char* nombre) {
    return nooki_tabla_anadir(manejador, nombre, 0);
}

long long nooki_ia_tabla_columna_texto(long long manejador, const char* nombre) {
    return nooki_tabla_anadir(manejador, nombre, 1);
}

long long nooki_ia_tabla_filas(long long manejador) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    return tabla == NULL ? 0 : tabla->filas;
}

long long nooki_ia_tabla_columnas(long long manejador) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    return tabla == NULL ? 0 : tabla->columnas;
}

/* Nombre de la columna; cadena vacía si no existe. */
const char* nooki_ia_tabla_nombre(long long manejador, long long columna) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL || columna < 0 || columna >= tabla->columnas) {
        return "";
    }
    return tabla->columnas_datos[columna].nombre;
}

static nooki_columna* nooki_tabla_columna(nooki_tabla* tabla, long long fila, long long columna) {
    if (tabla == NULL || columna < 0 || columna >= tabla->columnas || fila < 0 ||
        fila >= tabla->filas) {
        return NULL;
    }
    return &tabla->columnas_datos[columna];
}

void nooki_ia_tabla_poner_entero(long long manejador, long long fila, long long columna,
                                 long long valor) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    nooki_columna* c = nooki_tabla_columna(tabla, fila, columna);
    if (c == NULL || c->es_texto) {
        return;
    }
    c->numeros[fila] = valor;
}

void nooki_ia_tabla_poner_texto(long long manejador, long long fila, long long columna,
                                const char* valor) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    nooki_columna* c = nooki_tabla_columna(tabla, fila, columna);
    if (c == NULL || !c->es_texto) {
        return;
    }
    free(c->textos[fila]);
    c->textos[fila] = nooki_tabla_copiar(valor);
}

long long nooki_ia_tabla_valor_entero(long long manejador, long long fila, long long columna) {
    nooki_columna* c = nooki_tabla_columna(nooki_tabla_de(manejador), fila, columna);
    if (c == NULL || c->es_texto) {
        return 0;
    }
    return c->numeros[fila];
}

const char* nooki_ia_tabla_valor_texto(long long manejador, long long fila, long long columna) {
    nooki_columna* c = nooki_tabla_columna(nooki_tabla_de(manejador), fila, columna);
    if (c == NULL || !c->es_texto || c->textos[fila] == NULL) {
        return "";
    }
    return c->textos[fila];
}

long long nooki_ia_tabla_suma(long long manejador, long long columna) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL || columna < 0 || columna >= tabla->columnas) {
        return 0;
    }
    nooki_columna* c = &tabla->columnas_datos[columna];
    if (c->es_texto) {
        return 0;
    }
    long long total = 0;
    for (int i = 0; i < tabla->filas; ++i) {
        total += c->numeros[i];
    }
    return total;
}

long long nooki_ia_tabla_minimo(long long manejador, long long columna) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL || columna < 0 || columna >= tabla->columnas || tabla->filas == 0) {
        return 0;
    }
    nooki_columna* c = &tabla->columnas_datos[columna];
    if (c->es_texto) {
        return 0;
    }
    long long minimo = c->numeros[0];
    for (int i = 1; i < tabla->filas; ++i) {
        if (c->numeros[i] < minimo) {
            minimo = c->numeros[i];
        }
    }
    return minimo;
}

long long nooki_ia_tabla_maximo(long long manejador, long long columna) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL || columna < 0 || columna >= tabla->columnas || tabla->filas == 0) {
        return 0;
    }
    nooki_columna* c = &tabla->columnas_datos[columna];
    if (c->es_texto) {
        return 0;
    }
    long long maximo = c->numeros[0];
    for (int i = 1; i < tabla->filas; ++i) {
        if (c->numeros[i] > maximo) {
            maximo = c->numeros[i];
        }
    }
    return maximo;
}

/* Media de la columna, por mil. */
long long nooki_ia_tabla_media(long long manejador, long long columna) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL || columna < 0 || columna >= tabla->columnas || tabla->filas == 0) {
        return 0;
    }
    nooki_columna* c = &tabla->columnas_datos[columna];
    if (c->es_texto) {
        return 0;
    }
    double suma = 0.0;
    for (int i = 0; i < tabla->filas; ++i) {
        suma += (double)c->numeros[i];
    }
    const double media = suma / (double)tabla->filas;
    return (long long)(media < 0 ? media * 1000.0 - 0.5 : media * 1000.0 + 0.5);
}

/* Copia la estructura de la tabla y las columnas elegidas. */
static nooki_tabla* nooki_tabla_nueva_de(const nooki_tabla* origen, int filas) {
    const long long manejador = nooki_ia_tabla_crear(filas);
    nooki_tabla* nueva = nooki_tabla_de(manejador);
    if (nueva == NULL) {
        return NULL;
    }
    for (int c = 0; c < origen->columnas; ++c) {
        if (origen->columnas_datos[c].es_texto) {
            nooki_ia_tabla_columna_texto((long long)(intptr_t)nueva,
                                         origen->columnas_datos[c].nombre);
        } else {
            nooki_ia_tabla_columna_entero((long long)(intptr_t)nueva,
                                          origen->columnas_datos[c].nombre);
        }
    }
    return nueva;
}

/* Filas cuyo valor de texto coincide exactamente. */
long long nooki_ia_tabla_filtrar_texto(long long manejador, long long columna,
                                       const char* valor) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL || columna < 0 || columna >= tabla->columnas) {
        return 0;
    }
    nooki_columna* c = &tabla->columnas_datos[columna];
    if (!c->es_texto) {
        return 0;
    }
    int cuantas = 0;
    for (int i = 0; i < tabla->filas; ++i) {
        if (c->textos[i] != NULL && strcmp(c->textos[i], valor) == 0) {
            ++cuantas;
        }
    }
    nooki_tabla* nueva = nooki_tabla_nueva_de(tabla, cuantas);
    if (nueva == NULL) {
        return 0;
    }
    int destino = 0;
    for (int i = 0; i < tabla->filas; ++i) {
        if (c->textos[i] == NULL || strcmp(c->textos[i], valor) != 0) {
            continue;
        }
        for (int j = 0; j < tabla->columnas; ++j) {
            nooki_columna* origen = &tabla->columnas_datos[j];
            if (origen->es_texto) {
                nooki_ia_tabla_poner_texto((long long)(intptr_t)nueva, destino, j,
                                           origen->textos[i] == NULL ? "" : origen->textos[i]);
            } else {
                nooki_ia_tabla_poner_entero((long long)(intptr_t)nueva, destino, j,
                                            origen->numeros[i]);
            }
        }
        ++destino;
    }
    return (long long)(intptr_t)nueva;
}

/* Filas ordenadas de menor a mayor por una columna numérica. */
long long nooki_ia_tabla_ordenar(long long manejador, long long columna) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL || columna < 0 || columna >= tabla->columnas) {
        return 0;
    }
    nooki_columna* c = &tabla->columnas_datos[columna];
    if (c->es_texto) {
        return 0;
    }
    nooki_tabla* nueva = nooki_tabla_nueva_de(tabla, tabla->filas);
    if (nueva == NULL) {
        return 0;
    }
    int* orden = (int*)malloc((size_t)tabla->filas * sizeof(int));
    if (orden == NULL) {
        nooki_ia_tabla_liberar((long long)(intptr_t)nueva);
        return 0;
    }
    for (int i = 0; i < tabla->filas; ++i) {
        orden[i] = i;
    }
    /* Orden por inserción: simple y suficiente para tablas de trabajo. */
    for (int i = 1; i < tabla->filas; ++i) {
        const int actual = orden[i];
        int j = i - 1;
        while (j >= 0 && c->numeros[orden[j]] > c->numeros[actual]) {
            orden[j + 1] = orden[j];
            --j;
        }
        orden[j + 1] = actual;
    }
    for (int i = 0; i < tabla->filas; ++i) {
        for (int j = 0; j < tabla->columnas; ++j) {
            nooki_columna* origen = &tabla->columnas_datos[j];
            const int fila = orden[i];
            if (origen->es_texto) {
                nooki_ia_tabla_poner_texto((long long)(intptr_t)nueva, i, j,
                                           origen->textos[fila] == NULL ? ""
                                                                        : origen->textos[fila]);
            } else {
                nooki_ia_tabla_poner_entero((long long)(intptr_t)nueva, i, j,
                                            origen->numeros[fila]);
            }
        }
    }
    free(orden);
    return (long long)(intptr_t)nueva;
}

/* Cuántas filas de cada valor de una columna de texto. La tabla resultante
 * tiene dos columnas: «valor» y «cuenta». */
long long nooki_ia_tabla_contar_por(long long manejador, long long columna) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL || columna < 0 || columna >= tabla->columnas) {
        return 0;
    }
    nooki_columna* c = &tabla->columnas_datos[columna];
    if (!c->es_texto) {
        return 0;
    }
    /* Valores distintos. */
    char** valores = (char**)calloc((size_t)tabla->filas, sizeof(char*));
    if (valores == NULL) {
        return 0;
    }
    int cuantos = 0;
    for (int i = 0; i < tabla->filas; ++i) {
        const char* valor = c->textos[i] == NULL ? "" : c->textos[i];
        int encontrado = -1;
        for (int j = 0; j < cuantos; ++j) {
            if (strcmp(valores[j], valor) == 0) {
                encontrado = j;
                break;
            }
        }
        if (encontrado < 0) {
            valores[cuantos] = nooki_tabla_copiar(valor);
            ++cuantos;
        }
    }
    const long long salida = nooki_ia_tabla_crear(cuantos);
    nooki_tabla* nueva = nooki_tabla_de(salida);
    if (nueva == NULL) {
        for (int i = 0; i < cuantos; ++i) {
            free(valores[i]);
        }
        free(valores);
        return 0;
    }
    nooki_ia_tabla_columna_texto(salida, "valor");
    nooki_ia_tabla_columna_entero(salida, "cuenta");
    for (int i = 0; i < cuantos; ++i) {
        int cuenta = 0;
        for (int f = 0; f < tabla->filas; ++f) {
            const char* valor = c->textos[f] == NULL ? "" : c->textos[f];
            if (strcmp(valor, valores[i]) == 0) {
                ++cuenta;
            }
        }
        nooki_ia_tabla_poner_texto(salida, i, 0, valores[i]);
        nooki_ia_tabla_poner_entero(salida, i, 1, cuenta);
        free(valores[i]);
    }
    free(valores);
    return salida;
}

void nooki_ia_tabla_liberar(long long manejador) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL) {
        return;
    }
    for (int c = 0; c < tabla->columnas; ++c) {
        nooki_columna* columna = &tabla->columnas_datos[c];
        free(columna->nombre);
        if (columna->es_texto) {
            for (int i = 0; i < tabla->filas; ++i) {
                free(columna->textos[i]);
            }
            free(columna->textos);
        } else {
            free(columna->numeros);
        }
    }
    free(tabla->columnas_datos);
    free(tabla);
}

/* 1 si las dos formas y todos los elementos coinciden. */
long long nooki_ia_tensor_iguales(long long a_manejador, long long b_manejador) {
    nooki_tensor* a = nooki_tensor_de(a_manejador);
    nooki_tensor* b = nooki_tensor_de(b_manejador);
    if (a == NULL || b == NULL || a->filas != b->filas || a->columnas != b->columnas) {
        return 0;
    }
    const size_t total = (size_t)a->filas * (size_t)a->columnas;
    for (size_t i = 0; i < total; ++i) {
        if (a->datos[i] != b->datos[i]) {
            return 0;
        }
    }
    return 1;
}
)";
}

}  // namespace nooki::ia
