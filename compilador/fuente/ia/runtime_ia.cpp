#include "nooki/ia/runtime_ia.hpp"

namespace nooki::ia {

std::string_view fuente_runtime_ia() {
    return R"nooki(


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

/* Multiplica todos los elementos por un nÃƒÂºmero. */
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

/* Determinante por eliminaciÃƒÂ³n de Gauss con doble precisiÃƒÂ³n. */
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
)nooki" R"nooki(

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

/* --- EstadÃƒÂ­stica. Los resultados con decimales se dan por mil. --- */

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

/* DesviaciÃƒÂ³n tÃƒÂ­pica, por mil. */
long long nooki_ia_tensor_desviacion(long long manejador) {
    const long long varianza = nooki_ia_tensor_varianza(manejador);
    if (varianza <= 0) {
        return 0;
    }
    /* La varianza viene por mil, asÃƒÂ­ que hay que multiplicarla otra vez por mil
     * para que la raÃƒÂ­z quede tambiÃƒÂ©n por mil. */
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

/* e^x sin libm, con reducciÃƒÂ³n de rango.
 *
 * La serie de Taylor directa se rompe con |x| grande: en x = -20 los tÃƒÂ©rminos
 * llegan a 4Ã‚Â·10^7 y se cancelan entre sÃƒÂ­, asÃƒÂ­ que el resultado era basura. Eso
 * estropeaba el softmax, donde los logits pueden separarse mucho.
 *
 * Se escribe x = kÃ‚Â·ln2 + r con r en [-ln2/2, ln2/2]; la serie de e^r converge en
 * pocos tÃƒÂ©rminos y el 2^k solo escala. */
static double nooki_tensor_exp(double x) {
    const double LN2 = 0.69314718055994530942;
    if (x > 700.0) {
        return 1.0e308;
    }
    if (x < -745.0) {
        return 0.0;
    }
    const double k = x / LN2;
    double escala = (k >= 0.0) ? (double)(long long)(k + 0.5) : (double)(long long)(k - 0.5);
    const double r = x - escala * LN2;

    double termino = 1.0;
    double suma = 1.0;
    for (int i = 1; i < 18; ++i) {
        termino *= r / (double)i;
        suma += termino;
    }
    while (escala > 0.0) {
        suma *= 2.0;
        escala -= 1.0;
    }
    while (escala < 0.0) {
        suma *= 0.5;
        escala += 1.0;
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
)nooki" R"nooki(
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
    /* Se resta el mÃƒÂ¡ximo para que la exponencial no se dispare. */
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

/* --- Autograd: derivadas automÃƒÂ¡ticas sobre un grafo de operaciones ---
 * Cada nodo guarda su valor, su gradiente y de dÃƒÂ³nde viene. Todos los nodos
 * viven en una lista global: asÃƒÂ­ se limpian los gradientes y se libera todo de
 * una vez, sin recorrer el grafo dos veces ni compartir ÃƒÂ¡reas. */

typedef struct nooki_auto_nodo {
    double valor;
    double gradiente;
    int operacion; /* 0 dato, 1 suma, 2 resta, 3 producto, 4 divisiÃƒÂ³n, 5 relu, 6 sigmoide */
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

/* Marca el principio del grafo de una pasada: lo creado despuÃƒÂ©s se puede
 * liberar al terminar, dejando vivos los parÃƒÂ¡metros. */
long long nooki_ia_auto_marca(void) {
    return (long long)(intptr_t)nooki_auto_todos;
}

void nooki_ia_auto_soltar_hasta(long long marca) {
    nooki_auto_nodo* limite = (nooki_auto_nodo*)(intptr_t)marca;
    while (nooki_auto_todos != NULL && nooki_auto_todos != limite) {
        nooki_auto_nodo* siguiente = nooki_auto_todos->siguiente_global;
        free(nooki_auto_todos);
        nooki_auto_todos = siguiente;
    }
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
)nooki" R"nooki(
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
    /* La pila va de hojas a raÃƒÂ­z: se recorre al revÃƒÂ©s, de la raÃƒÂ­z a las hojas. */
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

/* Libera todos los nodos vivos. Los manejadores dejan de ser vÃƒÂ¡lidos. */
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
/* --- Redes: una capa oculta, activaciÃƒÂ³n sigmoide, descenso de gradiente ---
 * Los pesos son nodos del grafo de autograd, asÃƒÂ­ que el entrenamiento es
 * construir la expresiÃƒÂ³n de cada ejemplo, llamar a `atras` y mover los pesos. */

typedef struct {
    int entradas;
    int ocultas;
    double* muestras; /* entradas (4) + esperado, por ejemplo */
    int cuantas;
    int capacidad;
    unsigned int semilla;
    nooki_auto_nodo** pesos1;
    nooki_auto_nodo** sesgos1;
    nooki_auto_nodo** pesos2;
    nooki_auto_nodo* sesgo2;
    int tipo; /* 1: binaria (sigmoide), 2: varias clases (softmax) */
} nooki_red;

/* NÃƒÂºmero pseudoaleatorio pequeÃƒÂ±o y determinista: la red parte siempre igual. */
static double nooki_red_aleatorio(nooki_red* red, double amplitud) {
    red->semilla = red->semilla * 1103515245u + 12345u;
    const double unidad = (double)((red->semilla >> 16) & 0x7FFF) / 32767.0;
    return (unidad - 0.5) * 2.0 * amplitud;
}

long long nooki_ia_red_nueva(long long entradas, long long ocultas) {
    if (entradas < 1 || entradas > 4 || ocultas < 1 || ocultas > 64) {
        return 0;
    }
    nooki_red* red = (nooki_red*)malloc(sizeof(nooki_red));
    if (red == NULL) {
        return 0;
    }
    red->entradas = (int)entradas;
    red->ocultas = (int)ocultas;
    red->tipo = 1;
    red->cuantas = 0;
    red->capacidad = 16;
    red->muestras = (double*)malloc((size_t)red->capacidad * 5 * sizeof(double));
    red->semilla = 20261113u;
    red->pesos1 = (nooki_auto_nodo**)malloc((size_t)entradas * (size_t)ocultas *
                                            sizeof(nooki_auto_nodo*));
    red->sesgos1 = (nooki_auto_nodo**)malloc((size_t)ocultas * sizeof(nooki_auto_nodo*));
    red->pesos2 = (nooki_auto_nodo**)malloc((size_t)ocultas * sizeof(nooki_auto_nodo*));
    if (red->muestras == NULL || red->pesos1 == NULL || red->sesgos1 == NULL ||
        red->pesos2 == NULL) {
        free(red->muestras);
        free(red->pesos1);
        free(red->sesgos1);
        free(red->pesos2);
        free(red);
        return 0;
    }
    for (int i = 0; i < entradas * ocultas; ++i) {
        red->pesos1[i] = nooki_auto_nodo_de(nooki_ia_auto_dato(nooki_red_aleatorio(red, 0.9)));
    }
    for (int h = 0; h < ocultas; ++h) {
        red->sesgos1[h] = nooki_auto_nodo_de(nooki_ia_auto_dato(nooki_red_aleatorio(red, 0.9)));
        red->pesos2[h] = nooki_auto_nodo_de(nooki_ia_auto_dato(nooki_red_aleatorio(red, 0.9)));
    }
    red->sesgo2 = nooki_auto_nodo_de(nooki_ia_auto_dato(nooki_red_aleatorio(red, 0.9)));
    return (long long)(intptr_t)red;
}

static nooki_red* nooki_red_de(long long manejador) {
    return (nooki_red*)(intptr_t)manejador;
}

void nooki_ia_red_ejemplo(long long manejador, double x1, double x2, double x3, double x4,
                          double esperado) {
    nooki_red* red = nooki_red_de(manejador);
    if (red == NULL) {
        return;
    }
    if (red->cuantas >= red->capacidad) {
        const int nueva = red->capacidad * 2;
        double* muestras = (double*)realloc(red->muestras, (size_t)nueva * 5 * sizeof(double));
        if (muestras == NULL) {
            return;
        }
        red->muestras = muestras;
        red->capacidad = nueva;
    }
    double* fila = red->muestras + (size_t)red->cuantas * 5;
    fila[0] = x1;
    fila[1] = x2;
    fila[2] = x3;
    fila[3] = x4;
    fila[4] = esperado;
    ++red->cuantas;
}

long long nooki_ia_red_ejemplos(long long manejador) {
    nooki_red* red = nooki_red_de(manejador);
    return red == NULL ? 0 : red->cuantas;
}

/* Salida de la red para una entrada, sin construir grafo. */
double nooki_ia_red_predecir(long long manejador, double x1, double x2, double x3, double x4) {
    nooki_red* red = nooki_red_de(manejador);
    if (red == NULL) {
        return 0.0;
    }
    const double entradas[4] = {x1, x2, x3, x4};
    double ocultas[64];
    for (int h = 0; h < red->ocultas; ++h) {
        double z = red->sesgos1[h]->valor;
        for (int i = 0; i < red->entradas; ++i) {
            z += entradas[i] * red->pesos1[i * red->ocultas + h]->valor;
        }
        ocultas[h] = 1.0 / (1.0 + nooki_auto_exponencial(-z));
)nooki" R"nooki(
    }
    double salida = red->sesgo2->valor;
    for (int h = 0; h < red->ocultas; ++h) {
        salida += ocultas[h] * red->pesos2[h]->valor;
    }
    return 1.0 / (1.0 + nooki_auto_exponencial(-salida));
}

/* Error cuadrÃƒÂ¡tico medio de los ejemplos guardados. */
double nooki_ia_red_error(long long manejador) {
    nooki_red* red = nooki_red_de(manejador);
    if (red == NULL || red->cuantas == 0) {
        return 0.0;
    }
    double total = 0.0;
    for (int m = 0; m < red->cuantas; ++m) {
        const double* fila = red->muestras + (size_t)m * 5;
        const double salida = nooki_ia_red_predecir(manejador, fila[0], fila[1], fila[2], fila[3]);
        const double diferencia = salida - fila[4];
        total += diferencia * diferencia;
    }
    return total / (double)red->cuantas;
}

/* Construye la expresiÃƒÂ³n de un ejemplo y devuelve la pÃƒÂ©rdida. */
static nooki_auto_nodo* nooki_red_adelante(nooki_red* red, const double* fila) {
    nooki_auto_nodo* ocultas[64];
    for (int h = 0; h < red->ocultas; ++h) {
        nooki_auto_nodo* z = red->sesgos1[h];
        for (int i = 0; i < red->entradas; ++i) {
            nooki_auto_nodo* entrada = nooki_auto_nodo_de(
                nooki_ia_auto_dato(fila[i]));
            nooki_auto_nodo* producto = nooki_auto_nodo_de(
                (long long)(intptr_t)nooki_auto_crear(
                    entrada->valor * red->pesos1[i * red->ocultas + h]->valor, 3,

                    red->pesos1[i * red->ocultas + h], entrada));
            z = nooki_auto_nodo_de((long long)(intptr_t)nooki_auto_crear(
                z->valor + producto->valor, 1, z, producto));
        }
        ocultas[h] = nooki_auto_nodo_de((long long)(intptr_t)nooki_ia_auto_sigmoides(
            (long long)(intptr_t)z));
    }
    nooki_auto_nodo* salida = red->sesgo2;
    for (int h = 0; h < red->ocultas; ++h) {
        nooki_auto_nodo* producto = nooki_auto_nodo_de((long long)(intptr_t)nooki_auto_crear(
            ocultas[h]->valor * red->pesos2[h]->valor, 3, red->pesos2[h], ocultas[h]));
        salida = nooki_auto_nodo_de((long long)(intptr_t)nooki_auto_crear(
            salida->valor + producto->valor, 1, salida, producto));
    }
    nooki_auto_nodo* prediccion = nooki_auto_nodo_de(
        (long long)(intptr_t)nooki_ia_auto_sigmoides((long long)(intptr_t)salida));
    nooki_auto_nodo* esperado =
        nooki_auto_nodo_de(nooki_ia_auto_dato(fila[4]));
    nooki_auto_nodo* diferencia = nooki_auto_nodo_de((long long)(intptr_t)nooki_auto_crear(
        prediccion->valor - esperado->valor, 2, prediccion, esperado));
    return nooki_auto_nodo_de((long long)(intptr_t)nooki_auto_crear(
        diferencia->valor * diferencia->valor, 3, diferencia, diferencia));
}

/* Mueve los pesos con el gradiente: descenso de gradiente por ejemplo. */
static void nooki_red_actualizar(nooki_red* red, double tasa) {
    for (int i = 0; i < red->entradas * red->ocultas; ++i) {
        red->pesos1[i]->valor -= tasa * red->pesos1[i]->gradiente;
    }
    for (int h = 0; h < red->ocultas; ++h) {
        red->sesgos1[h]->valor -= tasa * red->sesgos1[h]->gradiente;
        red->pesos2[h]->valor -= tasa * red->pesos2[h]->gradiente;
    }
    red->sesgo2->valor -= tasa * red->sesgo2->gradiente;
}

void nooki_ia_red_entrenar(long long manejador, long long vueltas, double tasa) {
    nooki_red* red = nooki_red_de(manejador);
    if (red == NULL || red->cuantas == 0) {
        return;
    }
    for (long long vuelta = 0; vuelta < vueltas; ++vuelta) {
        for (int m = 0; m < red->cuantas; ++m) {
            const double* fila = red->muestras + (size_t)m * 5;
            // Los nodos de esta pasada se liberan al terminarla: si no, el grafo
            // crecerÃƒÂ­a sin lÃƒÂ­mite y el entrenamiento se volverÃƒÂ­a cuadrÃƒÂ¡tico.
            const long long marca = nooki_ia_auto_marca();
            nooki_auto_nodo* perdida = nooki_red_adelante(red, fila);
            nooki_ia_auto_atras((long long)(intptr_t)perdida);
            nooki_red_actualizar(red, tasa);
            nooki_ia_auto_soltar_hasta(marca);
        }
    }
}

/* --- Guardar y cargar una red ---
 *
 * Formato binario, en el orden en que se escribe y se lee:
 *   "NOOKIRED" (8 bytes) Ã‚Â· version, entradas, ocultas, cuantas (4 int de 32 bits)
 *   pesos1[entradas*ocultas] Ã‚Â· sesgos1[ocultas] Ã‚Â· pesos2[ocultas] Ã‚Â· sesgo2
 *   muestras[cuantas*5]
 * Todo en double de 64 bits. Guarda tambiÃƒÂ©n los ejemplos, para poder seguir
 * entrenando maÃƒÂ±ana donde se dejÃƒÂ³. */

/* Se declara antes porque `cargar` la usa y estÃƒÂ¡ definida mÃƒÂ¡s abajo. */
void nooki_ia_red_liberar(long long manejador);

static int nooki_red_escribir_dobles(FILE* flujo, const double* valores, int cuantos) {
    return fwrite(valores, sizeof(double), (size_t)cuantos, flujo) == (size_t)cuantos;
}

static int nooki_red_leer_dobles(FILE* flujo, double* valores, int cuantos) {
    return fread(valores, sizeof(double), (size_t)cuantos, flujo) == (size_t)cuantos;
}

int nooki_ia_red_guardar(long long manejador, const char* ruta) {
    nooki_red* red = nooki_red_de(manejador);
    if (red == NULL || ruta == NULL) {
        return 0;
    }
    FILE* flujo = fopen(ruta, "wb");
    if (flujo == NULL) {
        return 0;
    }

    int bien = fwrite("NOOKIRED", 1, 8, flujo) == 8;
    const int32_t cabecera[4] = {1, red->entradas, red->ocultas, red->cuantas};
    bien = bien && fwrite(cabecera, sizeof(int32_t), 4, flujo) == 4;

    double* valores = (double*)malloc((size_t)(red->entradas * red->ocultas) * sizeof(double));
    if (valores != NULL) {
        for (int i = 0; i < red->entradas * red->ocultas; ++i) {
            valores[i] = red->pesos1[i]->valor;
        }
        bien = bien && nooki_red_escribir_dobles(flujo, valores, red->entradas * red->ocultas);
        free(valores);
    } else {
        bien = 0;
    }

    double* sesgos = (double*)malloc((size_t)red->ocultas * sizeof(double));
    if (sesgos != NULL) {
        for (int h = 0; h < red->ocultas; ++h) {
            sesgos[h] = red->sesgos1[h]->valor;
        }
        bien = bien && nooki_red_escribir_dobles(flujo, sesgos, red->ocultas);
        for (int h = 0; h < red->ocultas; ++h) {
            sesgos[h] = red->pesos2[h]->valor;
        }
        bien = bien && nooki_red_escribir_dobles(flujo, sesgos, red->ocultas);
        free(sesgos);
    } else {
        bien = 0;
    }

    const double salida = red->sesgo2->valor;
    bien = bien && nooki_red_escribir_dobles(flujo, &salida, 1);
    bien = bien && nooki_red_escribir_dobles(flujo, red->muestras, red->cuantas * 5);

    if (fclose(flujo) != 0) {
        bien = 0;
    }
    return bien;
}

long long nooki_ia_red_cargar(const char* ruta) {
    if (ruta == NULL) {
        return 0;
    }
    FILE* flujo = fopen(ruta, "rb");
    if (flujo == NULL) {
        return 0;
    }

    char magia[8];
    int32_t cabecera[4] = {0, 0, 0, 0};
    long long resultado = 0;
    if (fread(magia, 1, 8, flujo) == 8 && memcmp(magia, "NOOKIRED", 8) == 0 &&
)nooki" R"nooki(        fread(cabecera, sizeof(int32_t), 4, flujo) == 4 && cabecera[0] == 1 &&

        cabecera[1] >= 1 && cabecera[1] <= 64 && cabecera[2] >= 1 && cabecera[2] <= 64 &&
        cabecera[3] >= 0) {
        const int entradas = cabecera[1];
        const int ocultas = cabecera[2];
        const int cuantas = cabecera[3];

        const long long manejador = nooki_ia_red_nueva(entradas, ocultas);
        nooki_red* red = nooki_red_de(manejador);
        if (red != NULL) {
            double* valores = (double*)malloc((size_t)(entradas * ocultas) * sizeof(double));
            double* sesgos = (double*)malloc((size_t)ocultas * sizeof(double));
            double salida = 0.0;

            int bien = valores != NULL && sesgos != NULL;
            if (bien) {
                bien = nooki_red_leer_dobles(flujo, valores, entradas * ocultas);
                for (int i = 0; bien && i < entradas * ocultas; ++i) {
                    red->pesos1[i]->valor = valores[i];
                }
                bien = bien && nooki_red_leer_dobles(flujo, sesgos, ocultas);
                for (int h = 0; bien && h < ocultas; ++h) {
                    red->sesgos1[h]->valor = sesgos[h];
                }
                bien = bien && nooki_red_leer_dobles(flujo, sesgos, ocultas);
                for (int h = 0; bien && h < ocultas; ++h) {
                    red->pesos2[h]->valor = sesgos[h];
                }
                bien = bien && nooki_red_leer_dobles(flujo, &salida, 1);
                if (bien) {
                    red->sesgo2->valor = salida;
                }
            }
            free(valores);
            free(sesgos);

            // Los ejemplos: si no caben, se pide memoria a medida.
            if (bien && cuantas > 0) {
                if (cuantas > red->capacidad) {
                    double* muestras =
                        (double*)realloc(red->muestras, (size_t)cuantas * 5 * sizeof(double));
                    if (muestras == NULL) {
                        bien = 0;
                    } else {
                        red->muestras = muestras;
                        red->capacidad = cuantas;
                    }
                }
                bien = bien && nooki_red_leer_dobles(flujo, red->muestras, cuantas * 5);
                if (bien) {
                    red->cuantas = cuantas;
                }
            }

            if (bien) {
                resultado = manejador;
            } else {
                nooki_ia_red_liberar(manejador);
            }
        }
    }

    fclose(flujo);
    return resultado;
}

void nooki_ia_red_liberar(long long manejador) {
    nooki_red* red = nooki_red_de(manejador);
    if (red == NULL) {
        return;
    }
    free(red->muestras);
    free(red->pesos1);
    free(red->sesgos1);
    free(red->pesos2);
    free(red);
}

/* --- Red de varias clases ---
 *
 * Una capa oculta con sigmoide y una salida softmax, una neurona por clase.
 * Se entrena con gradientes escritos a mano (el grafo de autograd es para la
 * red binaria): asÃƒÂ­ el caso de varias clases no toca el que ya funciona.
 *
 *   h[k]  = sigmoide(b1[k] + ÃŽÂ£_i w1[i][k]Ã‚Â·x[i])
 *   z[c]  = b2[c] + ÃŽÂ£_k w2[k][c]Ã‚Â·h[k]
 *   p[c]  = exp(z[c] Ã¢Ë†â€™ max) / ÃŽÂ£ exp(z Ã¢Ë†â€™ max)          (softmax estable)
 *   pÃƒÂ©rdida = Ã¢Ë†â€™log p[etiqueta]                        (entropÃƒÂ­a cruzada)
 *   Ã¢Ë†â€špÃƒÂ©rdida/Ã¢Ë†â€šz[c] = p[c] Ã¢Ë†â€™ [c == etiqueta]
 *
 * Con esa derivada, el resto son reglas de la cadena normales. */

typedef struct {
    int tipo; /* 2 */
    int entradas;
    int ocultas;
    int clases;
    int cuantas;
    int capacidad;
    unsigned int semilla;
    double* w1;    /* entradas Ãƒâ€” ocultas */
    double* b1;    /* ocultas */
    double* w2;    /* ocultas Ãƒâ€” clases */
    double* b2;    /* clases */
    double* xs;    /* cuantas Ãƒâ€” 4 entradas */
    int* etiquetas;
} nooki_red_clases;

static nooki_red_clases* nooki_clases_de(long long manejador) {
    nooki_red_clases* red = (nooki_red_clases*)(intptr_t)manejador;
    if (red == NULL || red->tipo != 2) {
        return NULL;
    }
    return red;
}

static double nooki_clases_aleatorio(nooki_red_clases* red) {
    red->semilla = red->semilla * 1103515245u + 12345u;
    const double unidad = (double)((red->semilla >> 16) & 0x7FFF) / 32767.0;
    return (unidad - 0.5) * 1.8;
}

long long nooki_ia_clases_nueva(long long entradas, long long ocultas, long long clases) {
    if (entradas < 1 || entradas > 64 || ocultas < 1 || ocultas > 64 || clases < 2 ||
        clases > 16) {
        return 0;
    }
    nooki_red_clases* red = (nooki_red_clases*)malloc(sizeof(nooki_red_clases));
    if (red == NULL) {
        return 0;
    }
    red->tipo = 2;
    red->entradas = (int)entradas;
    red->ocultas = (int)ocultas;
    red->clases = (int)clases;
    red->cuantas = 0;
    red->capacidad = 16;
    red->semilla = 20261114u;
    red->w1 = (double*)malloc((size_t)(entradas * ocultas) * sizeof(double));
    red->b1 = (double*)malloc((size_t)ocultas * sizeof(double));
    red->w2 = (double*)malloc((size_t)(ocultas * clases) * sizeof(double));
    red->b2 = (double*)malloc((size_t)clases * sizeof(double));
    red->xs = (double*)malloc((size_t)red->capacidad * (size_t)entradas * sizeof(double));
    red->etiquetas = (int*)malloc((size_t)red->capacidad * sizeof(int));
    if (red->w1 == NULL || red->b1 == NULL || red->w2 == NULL || red->b2 == NULL ||
        red->xs == NULL || red->etiquetas == NULL) {
        free(red->w1);
        free(red->b1);
        free(red->w2);
        free(red->b2);
        free(red->xs);
        free(red->etiquetas);
        free(red);
        return 0;
    }
    for (int i = 0; i < entradas * ocultas; ++i) {
        red->w1[i] = nooki_clases_aleatorio(red);
    }
    for (int h = 0; h < ocultas; ++h) {
        red->b1[h] = 0.0;
    }
    for (int i = 0; i < ocultas * clases; ++i) {
        red->w2[i] = nooki_clases_aleatorio(red);
    }
    for (int c = 0; c < clases; ++c) {
        red->b2[c] = 0.0;
    }
    return (long long)(intptr_t)red;
}

void nooki_ia_clases_ejemplo(long long manejador, double x1, double x2, double x3, double x4,
                             long long clase) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    if (red == NULL || clase < 0 || clase >= red->clases) {
        return;
    }
    if (red->cuantas >= red->capacidad) {
        const int nueva = red->capacidad * 2;
        double* xs = (double*)realloc(red->xs, (size_t)nueva * (size_t)red->entradas * sizeof(double));
        int* etiquetas = (int*)realloc(red->etiquetas, (size_t)nueva * sizeof(int));
        if (xs == NULL || etiquetas == NULL) {
            return;
        }
        red->xs = xs;
        red->etiquetas = etiquetas;
        red->capacidad = nueva;
    }
    double* fila = red->xs + (size_t)red->cuantas * (size_t)red->entradas;
    for (int i = 0; i < red->entradas; ++i) {
        fila[i] = 0.0;
    }
    fila[0] = x1;
    if (red->entradas > 1) {
)nooki" R"nooki(        fila[1] = x2;
    }
    if (red->entradas > 2) {
        fila[2] = x3;
    }
    if (red->entradas > 3) {
        fila[3] = x4;
    }
    red->etiquetas[red->cuantas] = (int)clase;
    red->cuantas += 1;
}

long long nooki_ia_clases_total(long long manejador) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    return red == NULL ? 0 : red->cuantas;

}

/* Calcula las probabilidades de una muestra. `h` y `p` los pone quien llama. */
static void nooki_clases_adelante(const nooki_red_clases* red, const double* x, double* h,
                                  double* p) {
    for (int k = 0; k < red->ocultas; ++k) {
        double z = red->b1[k];
        for (int i = 0; i < red->entradas; ++i) {
            z += red->w1[i * red->ocultas + k] * x[i];
        }
        h[k] = 1.0 / (1.0 + nooki_tensor_exp(-z));
    }
    double maximo = -1e300;
    for (int c = 0; c < red->clases; ++c) {
        double z = red->b2[c];
        for (int k = 0; k < red->ocultas; ++k) {
            z += red->w2[k * red->clases + c] * h[k];
        }
        p[c] = z;
        if (z > maximo) {
            maximo = z;
        }
    }
    double suma = 0.0;
    for (int c = 0; c < red->clases; ++c) {
        p[c] = nooki_tensor_exp(p[c] - maximo);
        suma += p[c];
    }
    if (suma <= 0.0) {
        suma = 1.0;
    }
    for (int c = 0; c < red->clases; ++c) {
        p[c] /= suma;
    }
}

long long nooki_ia_clases_predecir(long long manejador, double x1, double x2, double x3,
                                   double x4) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    if (red == NULL) {
        return -1;
    }
    double x[4] = {x1, x2, x3, x4};
    double h[64];
    double p[16];
    nooki_clases_adelante(red, x, h, p);

    int mejor = 0;
    for (int c = 1; c < red->clases; ++c) {
        if (p[c] > p[mejor]) {
            mejor = c;
        }
    }
    return mejor;
}

double nooki_ia_clases_probabilidad(long long manejador, double x1, double x2, double x3,
                                    double x4, long long clase) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    if (red == NULL || clase < 0 || clase >= red->clases) {
        return 0.0;
    }
    double x[4] = {x1, x2, x3, x4};
    double h[64];
    double p[16];
    nooki_clases_adelante(red, x, h, p);
    return p[clase];
}

void nooki_ia_clases_entrenar(long long manejador, long long vueltas, double tasa) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    if (red == NULL || red->cuantas == 0) {
        return;
    }
    const int entradas = red->entradas;
    const int ocultas = red->ocultas;
    const int clases = red->clases;

    double* h = (double*)malloc((size_t)ocultas * sizeof(double));
    double* p = (double*)malloc((size_t)clases * sizeof(double));
    double* dz = (double*)malloc((size_t)clases * sizeof(double));
    double* dh = (double*)malloc((size_t)ocultas * sizeof(double));
    if (h == NULL || p == NULL || dz == NULL || dh == NULL) {
        free(h);
        free(p);
        free(dz);
        free(dh);
        return;
    }

    for (long long vuelta = 0; vuelta < vueltas; ++vuelta) {
        for (int m = 0; m < red->cuantas; ++m) {
            const double* x = red->xs + (size_t)m * (size_t)red->entradas;
            const int etiqueta = red->etiquetas[m];
            nooki_clases_adelante(red, x, h, p);

            /* Ã¢Ë†â€špÃƒÂ©rdida/Ã¢Ë†â€šz = p Ã¢Ë†â€™ y */
            for (int c = 0; c < clases; ++c) {
                dz[c] = p[c] - (c == etiqueta ? 1.0 : 0.0);
            }
            for (int c = 0; c < clases; ++c) {
                red->b2[c] -= tasa * dz[c];
                for (int k = 0; k < ocultas; ++k) {
                    red->w2[k * clases + c] -= tasa * dz[c] * h[k];
                }
            }
            /* Ã¢Ë†â€špÃƒÂ©rdida/Ã¢Ë†â€šh[k], y a travÃƒÂ©s de la sigmoide */
            for (int k = 0; k < ocultas; ++k) {
                double suma = 0.0;
                for (int c = 0; c < clases; ++c) {
                    suma += red->w2[k * clases + c] * dz[c];
                }
                dh[k] = suma * h[k] * (1.0 - h[k]);
            }
            for (int k = 0; k < ocultas; ++k) {
                red->b1[k] -= tasa * dh[k];
                for (int i = 0; i < entradas; ++i) {
                    red->w1[i * ocultas + k] -= tasa * dh[k] * x[i];
                }
            }
        }
    }

    free(h);
    free(p);
    free(dz);
    free(dh);
}

long long nooki_ia_clases_aciertos(long long manejador) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    if (red == NULL) {
        return 0;
    }
    double h[64];
    double p[16];
    long long bien = 0;
    for (int m = 0; m < red->cuantas; ++m) {
        const double* x = red->xs + (size_t)m * (size_t)red->entradas;
        nooki_clases_adelante(red, x, h, p);
        int mejor = 0;
        for (int c = 1; c < red->clases; ++c) {
            if (p[c] > p[mejor]) {
                mejor = c;
            }
        }
        if (mejor == red->etiquetas[m]) {
            bien += 1;
        }
    }
    return bien;
}

void nooki_ia_clases_liberar(long long manejador) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    if (red == NULL) {
        return;
    }
    free(red->w1);
    free(red->b1);
    free(red->w2);
    free(red->b2);
    free(red->xs);
    free(red->etiquetas);
    free(red);
}

/* --- Tablas: columnas con nombre, como un DataFrame ---
 * Cada columna es numÃƒÂ©rica o de texto. Las de texto guardan copias propias. */

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

/* AÃƒÂ±ade una columna y devuelve su ÃƒÂ­ndice, o -1 si no se puede. */
static long long nooki_tabla_anadir(long long manejador, const char* nombre, int es_texto) {
    nooki_tabla* tabla = nooki_tabla_de(manejador);
    if (tabla == NULL) {
        return -1;
    }
    const int cuantas = tabla->columnas + 1;
    nooki_columna* columnas =
        (nooki_columna*)realloc(tabla->columnas_datos, (size_t)cuantas * sizeof(nooki_columna));
)nooki" R"nooki(    if (columnas == NULL) {
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

/* Nombre de la columna; cadena vacÃƒÂ­a si no existe. */
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
)nooki" R"nooki(    int destino = 0;
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

/* Filas ordenadas de menor a mayor por una columna numÃƒÂ©rica. */
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
    /* Orden por inserciÃƒÂ³n: simple y suficiente para tablas de trabajo. */
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

/* CuÃƒÂ¡ntas filas de cada valor de una columna de texto. La tabla resultante
 * tiene dos columnas: Ã‚Â«valorÃ‚Â» y Ã‚Â«cuentaÃ‚Â». */
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


/* --- Guardar y cargar la red de clases ---
 *
 * Formato propio, con su cabecera, para no confundirla con la binaria:
 *   "NOOKICLS" (8 bytes) Â· versiÃ³n, entradas, ocultas, clases y cuÃ¡ntas (5 int de 32)
 *   w1[entradas*ocultas] Â· b1[ocultas] Â· w2[ocultas*clases] Â· b2[clases]
 *   xs[cuantas*4] Â· etiquetas[cuantas] (int de 32 bits) */

int nooki_ia_clases_guardar(long long manejador, const char* ruta) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    if (red == NULL || ruta == NULL) {
        return 0;
    }
    FILE* flujo = fopen(ruta, "wb");
    if (flujo == NULL) {
        return 0;
    }

    int bien = fwrite("NOOKICLS", 1, 8, flujo) == 8;
    const int32_t cabecera[5] = {1, red->entradas, red->ocultas, red->clases, red->cuantas};
    bien = bien && fwrite(cabecera, sizeof(int32_t), 5, flujo) == 5;
    bien = bien && nooki_red_escribir_dobles(flujo, red->w1, red->entradas * red->ocultas);
    bien = bien && nooki_red_escribir_dobles(flujo, red->b1, red->ocultas);
    bien = bien && nooki_red_escribir_dobles(flujo, red->w2, red->ocultas * red->clases);
    bien = bien && nooki_red_escribir_dobles(flujo, red->b2, red->clases);
    bien = bien && nooki_red_escribir_dobles(flujo, red->xs, red->cuantas * red->entradas);
    if (red->cuantas > 0) {
        bien = bien && fwrite(red->etiquetas, sizeof(int32_t), (size_t)red->cuantas, flujo) ==
)nooki" R"nooki(                           (size_t)red->cuantas;
    }

    if (fclose(flujo) != 0) {
        bien = 0;
    }
    return bien;
}

long long nooki_ia_clases_cargar(const char* ruta) {
    if (ruta == NULL) {
        return 0;
    }
    FILE* flujo = fopen(ruta, "rb");
    if (flujo == NULL) {
        return 0;
    }

    char magia[8];
    int32_t cabecera[5] = {0, 0, 0, 0, 0};
    long long resultado = 0;
    if (fread(magia, 1, 8, flujo) == 8 && memcmp(magia, "NOOKICLS", 8) == 0 &&
        fread(cabecera, sizeof(int32_t), 5, flujo) == 5 && cabecera[0] == 1 &&
        cabecera[1] >= 1 && cabecera[1] <= 64 && cabecera[2] >= 1 && cabecera[2] <= 64 &&
        cabecera[3] >= 2 && cabecera[3] <= 16 && cabecera[4] >= 0) {
        const int entradas = cabecera[1];
        const int ocultas = cabecera[2];
        const int clases = cabecera[3];
        const int cuantas = cabecera[4];

        const long long manejador = nooki_ia_clases_nueva(entradas, ocultas, clases);
        nooki_red_clases* red = nooki_clases_de(manejador);
        if (red != NULL) {
            int bien = nooki_red_leer_dobles(flujo, red->w1, entradas * ocultas) &&
                       nooki_red_leer_dobles(flujo, red->b1, ocultas) &&
                       nooki_red_leer_dobles(flujo, red->w2, ocultas * clases) &&
                       nooki_red_leer_dobles(flujo, red->b2, clases);

            if (bien && cuantas > 0) {
                if (cuantas > red->capacidad) {
                    double* xs = (double*)realloc(red->xs, (size_t)cuantas * (size_t)entradas * sizeof(double));
                    int* etiquetas = (int*)realloc(red->etiquetas, (size_t)cuantas * sizeof(int));
                    if (xs == NULL || etiquetas == NULL) {
                        free(xs);
                        free(etiquetas);
                        bien = 0;
                    } else {
                        red->xs = xs;
                        red->etiquetas = etiquetas;
                        red->capacidad = cuantas;
                    }
                }
                bien = bien && nooki_red_leer_dobles(flujo, red->xs, cuantas * entradas);
                int32_t* leidas = (int32_t*)malloc((size_t)cuantas * sizeof(int32_t));
                if (leidas == NULL) {
                    bien = 0;
                } else {
                    bien = bien && fread(leidas, sizeof(int32_t), (size_t)cuantas, flujo) ==
                                       (size_t)cuantas;
                    for (int m = 0; bien && m < cuantas; ++m) {
                        if (leidas[m] < 0 || leidas[m] >= clases) {
                            bien = 0;
                            break;
                        }
                        red->etiquetas[m] = (int)leidas[m];
                    }
                    free(leidas);
                }
                if (bien) {
                    red->cuantas = cuantas;
                }
            }

            if (bien) {
                resultado = manejador;
            } else {
                nooki_ia_clases_liberar(manejador);
            }
        }
    }

    fclose(flujo);
    return resultado;
}


/* --- Entradas de cualquier tamaÃ±o ---
 *
 * El runtime no puede ver el tipo `Vector<flotante>` del lenguaje (vive en el
 * preludio, en otro archivo), asÃ­ que se declara aquÃ­ una vista con los mismos
 * tres campos y en el mismo orden. Es un contrato entre los dos archivos y estÃ¡
 * comprobado por la suite. */

typedef struct {
    double* datos;
    long long largo;
    long long capacidad;
} nooki_ia_vector_flotante;

static const double* nooki_ia_clases_fila(const nooki_ia_vector_flotante* x, const nooki_red_clases* red) {
    if (x == NULL || x->datos == NULL || x->largo != (long long)red->entradas) {
        return NULL;
    }
    return x->datos;
}

int nooki_ia_clases_ejemplo_datos(long long manejador, const nooki_ia_vector_flotante* x,
                                 long long cual) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    if (red == NULL || cual < 0 || cual >= red->clases) {
        return 0;
    }
    const double* datos = nooki_ia_clases_fila(x, red);
    if (datos == NULL) {
        return 0;
    }
    if (red->cuantas >= red->capacidad) {
        const int nueva = red->capacidad * 2;
        double* xs = (double*)realloc(red->xs,
                                      (size_t)nueva * (size_t)red->entradas * sizeof(double));
        int* etiquetas = (int*)realloc(red->etiquetas, (size_t)nueva * sizeof(int));
        if (xs == NULL || etiquetas == NULL) {
            return 0;
        }
        red->xs = xs;
        red->etiquetas = etiquetas;
        red->capacidad = nueva;
    }
    double* fila = red->xs + (size_t)red->cuantas * (size_t)red->entradas;
    for (int i = 0; i < red->entradas; ++i) {
        fila[i] = datos[i];
    }
    red->etiquetas[red->cuantas] = (int)cual;
    red->cuantas += 1;
    return 1;
}

long long nooki_ia_clases_predecir_datos(long long manejador, const nooki_ia_vector_flotante* x) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    if (red == NULL) {
        return -1;
    }
    const double* datos = nooki_ia_clases_fila(x, red);
    if (datos == NULL) {
        return -1;
    }
    double h[64];
    double p[16];
    nooki_clases_adelante(red, datos, h, p);

    int mejor = 0;
    for (int c = 1; c < red->clases; ++c) {
        if (p[c] > p[mejor]) {
            mejor = c;
        }
    }
    return mejor;
}

double nooki_ia_clases_probabilidad_datos(long long manejador,
                                          const nooki_ia_vector_flotante* x, long long cual) {
    nooki_red_clases* red = nooki_clases_de(manejador);
    if (red == NULL || cual < 0 || cual >= red->clases) {
        return 0.0;
    }
    const double* datos = nooki_ia_clases_fila(x, red);
    if (datos == NULL) {
        return 0.0;
    }
    double h[64];
    double p[16];
    nooki_clases_adelante(red, datos, h, p);
    return p[cual];
}


/* --- Red de dos capas ocultas ---
 *
 * La de una capa oculta no puede aprender problemas que necesitan combinar
 * rasgos en dos niveles. Esta tiene dos, con softmax a la salida:
 *
 *   h1 = sigmoide(W1Â·x + b1)
 *   h2 = sigmoide(W2Â·h1 + b2)
 *   z  = W3Â·h2 + b3
 *   p  = softmax(z)
 *
 * Se entrena con gradientes escritos a mano, igual que la de una capa. Va en su
 * propia estructura para no tocar la que ya funciona y estÃ¡ probada. */

typedef struct {
    int tipo; /* 3 */
    int entradas;
    int ocultas;
    int clases;
    int cuantas;
    int capacidad;
    unsigned int semilla;
    double* w1; /* entradas Ã— ocultas */
    double* b1; /* ocultas */
    double* w2; /* ocultas Ã— ocultas */
    double* b2; /* ocultas */
    double* w3; /* ocultas Ã— clases */
    double* b3; /* clases */
    double* xs; /* cuantas Ã— entradas */
    int* etiquetas;
} nooki_red_profunda;

static nooki_red_profunda* nooki_profunda_de(long long manejador) {
    nooki_red_profunda* red = (nooki_red_profunda*)(intptr_t)manejador;
)nooki" R"nooki(    if (red == NULL || red->tipo != 3) {
        return NULL;
    }
    return red;
}

static double nooki_profunda_aleatorio(nooki_red_profunda* red, double amplitud) {
    red->semilla = red->semilla * 1103515245u + 12345u;
    const double unidad = (double)((red->semilla >> 16) & 0x7FFF) / 32767.0;
    return (unidad - 0.5) * amplitud;
}

long long nooki_ia_profunda_nueva(long long entradas, long long ocultas, long long clases) {
    if (entradas < 1 || entradas > 64 || ocultas < 1 || ocultas > 64 || clases < 2 ||
        clases > 16) {
        return 0;
    }
    nooki_red_profunda* red = (nooki_red_profunda*)malloc(sizeof(nooki_red_profunda));
    if (red == NULL) {
        return 0;
    }
    const size_t e = (size_t)entradas;
    const size_t h = (size_t)ocultas;
    const size_t c = (size_t)clases;

    red->tipo = 3;
    red->entradas = (int)entradas;
    red->ocultas = (int)ocultas;
    red->clases = (int)clases;
    red->cuantas = 0;
    red->capacidad = 16;
    red->semilla = 20261115u;
    red->w1 = (double*)malloc(e * h * sizeof(double));
    red->b1 = (double*)malloc(h * sizeof(double));
    red->w2 = (double*)malloc(h * h * sizeof(double));
    red->b2 = (double*)malloc(h * sizeof(double));
    red->w3 = (double*)malloc(h * c * sizeof(double));
    red->b3 = (double*)malloc(c * sizeof(double));
    red->xs = (double*)malloc((size_t)red->capacidad * e * sizeof(double));
    red->etiquetas = (int*)malloc((size_t)red->capacidad * sizeof(int));
    if (red->w1 == NULL || red->b1 == NULL || red->w2 == NULL || red->b2 == NULL ||
        red->w3 == NULL || red->b3 == NULL || red->xs == NULL || red->etiquetas == NULL) {
        free(red->w1);
        free(red->b1);
        free(red->w2);
        free(red->b2);
        free(red->w3);
        free(red->b3);
        free(red->xs);
        free(red->etiquetas);
        free(red);
        return 0;
    }

    /* Xavier: la escala depende del nÃºmero de conexiones de cada capa. */
    const double escala1 = 1.0 / (double)(entradas > 0 ? entradas : 1);
    const double escala2 = 1.0 / (double)(ocultas > 0 ? ocultas : 1);
    for (size_t i = 0; i < e * h; ++i) {
        red->w1[i] = nooki_profunda_aleatorio(red, 2.0 * escala1 + 0.5);
    }
    for (size_t i = 0; i < h; ++i) {
        red->b1[i] = 0.0;
        red->b2[i] = 0.0;
    }
    for (size_t i = 0; i < h * h; ++i) {
        red->w2[i] = nooki_profunda_aleatorio(red, 2.0 * escala2 + 0.5);
    }
    for (size_t i = 0; i < h * c; ++i) {
        red->w3[i] = nooki_profunda_aleatorio(red, 2.0 * escala2 + 0.5);
    }
    for (size_t i = 0; i < c; ++i) {
        red->b3[i] = 0.0;
    }
    return (long long)(intptr_t)red;
}

int nooki_ia_profunda_ejemplo(long long manejador, const nooki_ia_vector_flotante* x,
                             long long cual) {
    nooki_red_profunda* red = nooki_profunda_de(manejador);
    if (red == NULL || cual < 0 || cual >= red->clases || x == NULL || x->datos == NULL ||
        x->largo != (long long)red->entradas) {
        return 0;
    }
    if (red->cuantas >= red->capacidad) {
        const int nueva = red->capacidad * 2;
        double* xs = (double*)realloc(red->xs,
                                      (size_t)nueva * (size_t)red->entradas * sizeof(double));
        int* etiquetas = (int*)realloc(red->etiquetas, (size_t)nueva * sizeof(int));
        if (xs == NULL || etiquetas == NULL) {
            return 0;
        }
        red->xs = xs;
        red->etiquetas = etiquetas;
        red->capacidad = nueva;
    }
    double* fila = red->xs + (size_t)red->cuantas * (size_t)red->entradas;
    for (int i = 0; i < red->entradas; ++i) {
        fila[i] = x->datos[i];
    }
    red->etiquetas[red->cuantas] = (int)cual;
    red->cuantas += 1;
    return 1;
}

long long nooki_ia_profunda_total(long long manejador) {
    nooki_red_profunda* red = nooki_profunda_de(manejador);
    return red == NULL ? 0 : red->cuantas;
}

/* Adelante: deja en `h1`, `h2` y `p` lo necesario para el atrÃ¡s. */
static void nooki_profunda_adelante(const nooki_red_profunda* red, const double* x,
                                    double* h1, double* h2, double* p) {
    for (int a = 0; a < red->ocultas; ++a) {
        double z = red->b1[a];
        for (int i = 0; i < red->entradas; ++i) {
            z += red->w1[i * red->ocultas + a] * x[i];
        }
        h1[a] = 1.0 / (1.0 + nooki_tensor_exp(-z));
    }
    for (int k = 0; k < red->ocultas; ++k) {
        double z = red->b2[k];
        for (int a = 0; a < red->ocultas; ++a) {
            z += red->w2[a * red->ocultas + k] * h1[a];
        }
        h2[k] = 1.0 / (1.0 + nooki_tensor_exp(-z));
    }
    double maximo = -1e300;
    for (int c = 0; c < red->clases; ++c) {
        double z = red->b3[c];
        for (int k = 0; k < red->ocultas; ++k) {
            z += red->w3[k * red->clases + c] * h2[k];
        }
        p[c] = z;
        if (z > maximo) {
            maximo = z;
        }
    }
    double suma = 0.0;
    for (int c = 0; c < red->clases; ++c) {
        p[c] = nooki_tensor_exp(p[c] - maximo);
        suma += p[c];
    }
    if (suma <= 0.0) {
        suma = 1.0;
    }
    for (int c = 0; c < red->clases; ++c) {
        p[c] /= suma;
    }
}

static int nooki_profunda_clase(const nooki_red_profunda* red, const double* x, double* confianza) {
    double h1[64];
    double h2[64];
    double p[16];
    nooki_profunda_adelante(red, x, h1, h2, p);
    int mejor = 0;
    for (int c = 1; c < red->clases; ++c) {
        if (p[c] > p[mejor]) {
            mejor = c;
        }
    }
    if (confianza != NULL) {
        *confianza = p[mejor];
    }
    return mejor;
}

long long nooki_ia_profunda_predecir(long long manejador, const nooki_ia_vector_flotante* x) {
    nooki_red_profunda* red = nooki_profunda_de(manejador);
    if (red == NULL || x == NULL || x->datos == NULL ||
        x->largo != (long long)red->entradas) {
        return -1;
    }
    return nooki_profunda_clase(red, x->datos, NULL);
}

double nooki_ia_profunda_confianza(long long manejador, const nooki_ia_vector_flotante* x) {
    nooki_red_profunda* red = nooki_profunda_de(manejador);
    if (red == NULL || x == NULL || x->datos == NULL ||
        x->largo != (long long)red->entradas) {
        return 0.0;
    }
    double confianza = 0.0;
    nooki_profunda_clase(red, x->datos, &confianza);
    return confianza;
}

void nooki_ia_profunda_entrenar(long long manejador, long long vueltas, double tasa) {
    nooki_red_profunda* red = nooki_profunda_de(manejador);
    if (red == NULL || red->cuantas == 0) {
        return;
    }
    const int e = red->entradas;
    const int h = red->ocultas;
    const int c = red->clases;

    double* h1 = (double*)malloc((size_t)h * sizeof(double));
    double* h2 = (double*)malloc((size_t)h * sizeof(double));
    double* p = (double*)malloc((size_t)c * sizeof(double));
    double* dz = (double*)malloc((size_t)c * sizeof(double));
    double* d2 = (double*)malloc((size_t)h * sizeof(double));
    double* d1 = (double*)malloc((size_t)h * sizeof(double));
)nooki" R"nooki(    if (h1 == NULL || h2 == NULL || p == NULL || dz == NULL || d2 == NULL || d1 == NULL) {
        free(h1);
        free(h2);
        free(p);
        free(dz);
        free(d2);
        free(d1);
        return;
    }

    for (long long vuelta = 0; vuelta < vueltas; ++vuelta) {
        for (int m = 0; m < red->cuantas; ++m) {
            const double* x = red->xs + (size_t)m * (size_t)e;
            const int etiqueta = red->etiquetas[m];
            nooki_profunda_adelante(red, x, h1, h2, p);

            for (int k = 0; k < c; ++k) {
                dz[k] = p[k] - (k == etiqueta ? 1.0 : 0.0);
            }
            for (int k = 0; k < c; ++k) {
                red->b3[k] -= tasa * dz[k];
                for (int b = 0; b < h; ++b) {
                    red->w3[b * c + k] -= tasa * dz[k] * h2[b];
                }
            }
            for (int b = 0; b < h; ++b) {
                double suma = 0.0;
                for (int k = 0; k < c; ++k) {
                    suma += red->w3[b * c + k] * dz[k];
                }
                d2[b] = suma * h2[b] * (1.0 - h2[b]);
            }
            for (int b = 0; b < h; ++b) {
                red->b2[b] -= tasa * d2[b];
                for (int a = 0; a < h; ++a) {
                    red->w2[a * h + b] -= tasa * d2[b] * h1[a];
                }
            }
            for (int a = 0; a < h; ++a) {
                double suma = 0.0;
                for (int b = 0; b < h; ++b) {
                    suma += red->w2[a * h + b] * d2[b];
                }
                d1[a] = suma * h1[a] * (1.0 - h1[a]);
            }
            for (int a = 0; a < h; ++a) {
                red->b1[a] -= tasa * d1[a];
                for (int i = 0; i < e; ++i) {
                    red->w1[i * h + a] -= tasa * d1[a] * x[i];
                }
            }
        }
    }

    free(h1);
    free(h2);
    free(p);
    free(dz);
    free(d2);
    free(d1);
}

long long nooki_ia_profunda_aciertos(long long manejador) {
    nooki_red_profunda* red = nooki_profunda_de(manejador);
    if (red == NULL) {
        return 0;
    }
    long long bien = 0;
    for (int m = 0; m < red->cuantas; ++m) {
        const double* x = red->xs + (size_t)m * (size_t)red->entradas;
        if (nooki_profunda_clase(red, x, NULL) == red->etiquetas[m]) {
            bien += 1;
        }
    }
    return bien;
}

void nooki_ia_profunda_liberar(long long manejador) {
    nooki_red_profunda* red = nooki_profunda_de(manejador);
    if (red == NULL) {
        return;
    }
    free(red->w1);
    free(red->b1);
    free(red->w2);
    free(red->b2);
    free(red->w3);
    free(red->b3);
    free(red->xs);
    free(red->etiquetas);
    free(red);
}



)nooki";
}

}  // namespace nooki::ia
