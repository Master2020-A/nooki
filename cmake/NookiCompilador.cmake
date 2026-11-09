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

        # Windows + MinGW: enlazar libstdc++ y libgcc de forma estática.
        # Así el binario no depende de las DLL del compilador (que además pueden
        # entrar en conflicto con otras versiones instaladas en el sistema).
        if(WIN32)
            target_link_options(${objetivo} PRIVATE -static-libgcc -static-libstdc++)
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
