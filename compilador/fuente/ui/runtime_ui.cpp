#include "nooki/ui/runtime_ui.hpp"

namespace nooki::ui {

std::string_view fuente_runtime_ui() {
    return R"(
/* --- Ventanas de Nooki (generado por el compilador) ---
 * Un manejador es un entero: el puntero a la estructura de la ventana. Las
 * funciones devuelven 0 cuando no hay pantalla disponible, así que un programa
 * puede compilarse y probarse sin entorno gráfico. */
#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef struct {
    HWND ventana;
    int viva;
    int cerrada;
} nooki_ventana_estado;

static LRESULT CALLBACK nooki_ventana_proc(HWND ventana, UINT mensaje, WPARAM wparam,
                                           LPARAM lparam) {
    nooki_ventana_estado* estado =
        (nooki_ventana_estado*)GetWindowLongPtrA(ventana, GWLP_USERDATA);
    if (mensaje == WM_CLOSE) {
        if (estado != NULL) {
            estado->cerrada = 1;
        }
        DestroyWindow(ventana);
        return 0;
    }
    if (mensaje == WM_DESTROY) {
        if (estado != NULL) {
            estado->viva = 0;
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(ventana, mensaje, wparam, lparam);
}

long long nooki_ui_ventana_crear(const char* titulo, long long ancho, long long alto) {
    static int registrada = 0;
    HINSTANCE instancia = GetModuleHandleA(NULL);
    if (!registrada) {
        WNDCLASSA clase;
        memset(&clase, 0, sizeof(clase));
        clase.lpfnWndProc = nooki_ventana_proc;
        clase.hInstance = instancia;
        clase.hCursor = LoadCursor(NULL, IDC_ARROW);
        clase.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        clase.lpszClassName = "NookiVentana";
        if (RegisterClassA(&clase) == 0) {
            return 0;
        }
        registrada = 1;
    }

    nooki_ventana_estado* estado = (nooki_ventana_estado*)malloc(sizeof(nooki_ventana_estado));
    if (estado == NULL) {
        return 0;
    }
    estado->viva = 1;
    estado->cerrada = 0;
    estado->ventana = CreateWindowExA(0, "NookiVentana", titulo, WS_OVERLAPPEDWINDOW,
                                      CW_USEDEFAULT, CW_USEDEFAULT, (int)ancho, (int)alto,
                                      NULL, NULL, instancia, NULL);
    if (estado->ventana == NULL) {
        free(estado);
        return 0;
    }
    SetWindowLongPtrA(estado->ventana, GWLP_USERDATA, (LONG_PTR)estado);
    ShowWindow(estado->ventana, SW_SHOW);
    UpdateWindow(estado->ventana);
    return (long long)(intptr_t)estado;
}

int nooki_ui_ventana_viva(long long manejador) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || !estado->viva) {
        return 0;
    }
    MSG mensaje;
    while (PeekMessageA(&mensaje, NULL, 0, 0, PM_REMOVE)) {
        if (mensaje.message == WM_QUIT) {
            estado->viva = 0;
            return 0;
        }
        TranslateMessage(&mensaje);
        DispatchMessageA(&mensaje);
    }
    return estado->viva;
}

/* 0 = nada, 1 = cerrar, 2 = tecla. */
int nooki_ui_ventana_evento(long long manejador) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL) {
        return 0;
    }
    if (estado->cerrada) {
        return 1;
    }
    return 0;
}

void nooki_ui_ventana_cerrar(long long manejador) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL) {
        return;
    }
    if (estado->ventana != NULL) {
        DestroyWindow(estado->ventana);
    }
    free(estado);
}

void nooki_ui_ventana_esperar(long long milisegundos) {
    Sleep((DWORD)milisegundos);
}

#else

#include <X11/Xlib.h>
#include <unistd.h>

typedef struct {
    Display* pantalla;
    Window ventana;
    int viva;
    int cerrada;
} nooki_ventana_estado;

long long nooki_ui_ventana_crear(const char* titulo, long long ancho, long long alto) {
    Display* pantalla = XOpenDisplay(NULL);
    if (pantalla == NULL) {
        return 0;  /* sin entorno gráfico */
    }
    nooki_ventana_estado* estado = (nooki_ventana_estado*)malloc(sizeof(nooki_ventana_estado));
    if (estado == NULL) {
        XCloseDisplay(pantalla);
        return 0;
    }
    estado->pantalla = pantalla;
    estado->viva = 1;
    estado->cerrada = 0;
    estado->ventana = XCreateSimpleWindow(pantalla, RootWindow(pantalla, 0), 0, 0,
                                          (unsigned)ancho, (unsigned)alto, 0,
                                          BlackPixel(pantalla, 0), WhitePixel(pantalla, 0));
    XStoreName(pantalla, estado->ventana, titulo);
    XSelectInput(pantalla, estado->ventana, ExposureMask | KeyPressMask | StructureNotifyMask);
    XMapWindow(pantalla, estado->ventana);
    XFlush(pantalla);
    return (long long)(intptr_t)estado;
}

int nooki_ui_ventana_viva(long long manejador) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || !estado->viva) {
        return 0;
    }
    while (XPending(estado->pantalla) > 0) {
        XEvent evento;
        XNextEvent(estado->pantalla, &evento);
        if (evento.type == DestroyNotify) {
            estado->viva = 0;
        }
        if (evento.type == ClientMessage) {
            estado->cerrada = 1;
        }
    }
    return estado->viva;
}

int nooki_ui_ventana_evento(long long manejador) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL) {
        return 0;
    }
    return estado->cerrada ? 1 : 0;
}

void nooki_ui_ventana_cerrar(long long manejador) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL) {
        return;
    }
    if (estado->pantalla != NULL) {
        XDestroyWindow(estado->pantalla, estado->ventana);
        XCloseDisplay(estado->pantalla);
    }
    free(estado);
}

void nooki_ui_ventana_esperar(long long milisegundos) {
    usleep((useconds_t)(milisegundos * 1000));
}

#endif
)";
}

}  // namespace nooki::ui
