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
