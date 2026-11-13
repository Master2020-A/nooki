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
    HDC lienzo;
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
    estado->lienzo = GetDC(estado->ventana);
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

/* --- Dibujo (GDI) --- */
static COLORREF nooki_color_win(long long color) {
    return RGB((BYTE)((color >> 16) & 0xFF), (BYTE)((color >> 8) & 0xFF),
               (BYTE)(color & 0xFF));
}

void nooki_ui_dibujo_limpiar(long long manejador, long long color) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->lienzo == NULL) {
        return;
    }
    RECT area;
    GetClientRect(estado->ventana, &area);
    HBRUSH pincel = CreateSolidBrush(nooki_color_win(color));
    FillRect(estado->lienzo, &area, pincel);
    DeleteObject(pincel);
}

void nooki_ui_dibujo_rectangulo(long long manejador, long long x, long long y,
                                long long ancho, long long alto, long long color) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->lienzo == NULL) {
        return;
    }
    RECT area;
    area.left = (LONG)x;
    area.top = (LONG)y;
    area.right = (LONG)(x + ancho);
    area.bottom = (LONG)(y + alto);
    HBRUSH pincel = CreateSolidBrush(nooki_color_win(color));
    FillRect(estado->lienzo, &area, pincel);
    DeleteObject(pincel);
}

void nooki_ui_dibujo_linea(long long manejador, long long x1, long long y1, long long x2,
                           long long y2, long long color) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->lienzo == NULL) {
        return;
    }
    HPEN lapiz = CreatePen(PS_SOLID, 1, nooki_color_win(color));
    HGDIOBJ anterior = SelectObject(estado->lienzo, lapiz);
    MoveToEx(estado->lienzo, (int)x1, (int)y1, NULL);
    LineTo(estado->lienzo, (int)x2, (int)y2);
    SelectObject(estado->lienzo, anterior);
    DeleteObject(lapiz);
}

void nooki_ui_dibujo_texto(long long manejador, long long x, long long y, const char* contenido,
                           long long color) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->lienzo == NULL) {
        return;
    }
    SetTextColor(estado->lienzo, nooki_color_win(color));
    SetBkMode(estado->lienzo, TRANSPARENT);
    TextOutA(estado->lienzo, (int)x, (int)y, contenido, (int)strlen(contenido));
}

void nooki_ui_dibujo_presentar(long long manejador) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->lienzo == NULL) {
        return;
    }
    GdiFlush();
}

/* Color del píxel como 0xRRGGBB; -1 si no se puede leer. */
long long nooki_ui_dibujo_color_en(long long manejador, long long x, long long y) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->lienzo == NULL) {
        return -1;
    }
    COLORREF color = GetPixel(estado->lienzo, (int)x, (int)y);
    if (color == CLR_INVALID) {
        return -1;
    }
    return (long long)((GetRValue(color) << 16) | (GetGValue(color) << 8) | GetBValue(color));
}

#else

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    Display* pantalla;
    Window ventana;
    GC lienzo;
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
    estado->lienzo = XCreateGC(pantalla, estado->ventana, 0, NULL);
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
        if (estado->lienzo != NULL) {
            XFreeGC(estado->pantalla, estado->lienzo);
        }
        XDestroyWindow(estado->pantalla, estado->ventana);
        XCloseDisplay(estado->pantalla);
    }
    free(estado);
}

void nooki_ui_ventana_esperar(long long milisegundos) {
    usleep((useconds_t)(milisegundos * 1000));
}

/* --- Dibujo (X11) --- */
static unsigned long nooki_color_x11(long long color) {
    return (unsigned long)(color & 0xFFFFFF);
}

void nooki_ui_dibujo_limpiar(long long manejador, long long color) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->pantalla == NULL) {
        return;
    }
    XSetForeground(estado->pantalla, estado->lienzo, nooki_color_x11(color));
    XFillRectangle(estado->pantalla, estado->ventana, estado->lienzo, 0, 0, 4096, 4096);
}

void nooki_ui_dibujo_rectangulo(long long manejador, long long x, long long y,
                                long long ancho, long long alto, long long color) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->pantalla == NULL) {
        return;
    }
    XSetForeground(estado->pantalla, estado->lienzo, nooki_color_x11(color));
    XFillRectangle(estado->pantalla, estado->ventana, estado->lienzo, (int)x, (int)y,
                   (unsigned)ancho, (unsigned)alto);
}

void nooki_ui_dibujo_linea(long long manejador, long long x1, long long y1, long long x2,
                           long long y2, long long color) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->pantalla == NULL) {
        return;
    }
    XSetForeground(estado->pantalla, estado->lienzo, nooki_color_x11(color));
    XDrawLine(estado->pantalla, estado->ventana, estado->lienzo, (int)x1, (int)y1, (int)x2,
              (int)y2);
}

void nooki_ui_dibujo_texto(long long manejador, long long x, long long y, const char* contenido,
                           long long color) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->pantalla == NULL) {
        return;
    }
    XSetForeground(estado->pantalla, estado->lienzo, nooki_color_x11(color));
    XDrawString(estado->pantalla, estado->ventana, estado->lienzo, (int)x, (int)y, contenido,
                (int)strlen(contenido));
}

void nooki_ui_dibujo_presentar(long long manejador) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->pantalla == NULL) {
        return;
    }
    XFlush(estado->pantalla);
}

long long nooki_ui_dibujo_color_en(long long manejador, long long x, long long y) {
    nooki_ventana_estado* estado = (nooki_ventana_estado*)(intptr_t)manejador;
    if (estado == NULL || estado->pantalla == NULL) {
        return -1;
    }
    XImage* imagen =
        XGetImage(estado->pantalla, estado->ventana, (int)x, (int)y, 1, 1, AllPlanes, ZPixmap);
    if (imagen == NULL) {
        return -1;
    }
    unsigned long pixel = XGetPixel(imagen, 0, 0);
    XDestroyImage(imagen);
    return (long long)(pixel & 0xFFFFFF);
}

#endif
)";
}

}  // namespace nooki::ui
