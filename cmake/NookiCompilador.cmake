# NookiCompilador.cmake
#
# Reglas comunes de compilación para todos los objetivos del proyecto.
# Se aplican con la función `nooki_aplicar_opciones(<objetivo>)`.

function(nooki_aplicar_opciones objetivo)
    target_compile_features(${objetivo} PUBLIC cxx_std_20)
    set_target_properties(${objetivo} PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON)

    if(MSVC)
        # /utf-8 es imprescindible: el código fuente está en español.
        target_compile_options(${objetivo} PRIVATE
            /W4
            /permissive-
            /utf-8
            /Zc:__cplusplus
            /EHsc)
        if(NOOKI_AVISOS_COMO_ERRORES)
            target_compile_options(${objetivo} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${objetivo} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wold-style-cast
            -Wnon-virtual-dtor
            -Wcast-qual
            -Wunused
            -Woverloaded-virtual)
        if(NOOKI_AVISOS_COMO_ERRORES)
            target_compile_options(${objetivo} PRIVATE -Werror)
        endif()

        # Windows + MinGW: enlazado totalmente estático.
        # El binario no debe depender de las DLL del compilador ni de la versión de
        # libwinpthread que haya en el PATH del usuario (una versión antigua provoca
        # "no se encuentra el punto de entrada" al arrancar).
        if(WIN32)
            target_link_options(${objetivo} PRIVATE -static)
        endif()
    endif()

    if(NOOKI_SANEAMIENTO)
        if(MSVC)
            message(WARNING "El saneamiento (ASan/UBSan) no se aplica con MSVC en este proyecto")
        else()
            target_compile_options(${objetivo} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer)
            target_link_options(${objetivo} PRIVATE
                -fsanitize=address,undefined)
        endif()
    endif()
endfunction()
