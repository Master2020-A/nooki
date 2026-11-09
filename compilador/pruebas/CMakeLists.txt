# compilador/pruebas/CMakeLists.txt

add_executable(prueba_cli prueba_cli.cpp)
target_link_libraries(prueba_cli PRIVATE nooki_nucleo)
nooki_aplicar_opciones(prueba_cli)
add_test(NAME cli COMMAND prueba_cli)

add_executable(prueba_diagnosticos prueba_diagnosticos.cpp)
target_link_libraries(prueba_diagnosticos PRIVATE nooki_nucleo)
nooki_aplicar_opciones(prueba_diagnosticos)
target_compile_definitions(prueba_diagnosticos
    PRIVATE NOOKI_DIR_CASOS="${CMAKE_CURRENT_SOURCE_DIR}/diagnostico")
add_test(NAME diagnosticos COMMAND prueba_diagnosticos)

add_executable(prueba_lexico prueba_lexico.cpp)
target_link_libraries(prueba_lexico PRIVATE nooki_nucleo)
nooki_aplicar_opciones(prueba_lexico)
add_test(NAME lexico COMMAND prueba_lexico)

add_executable(prueba_compilador prueba_compilador.cpp)
target_link_libraries(prueba_compilador PRIVATE nooki_nucleo)
nooki_aplicar_opciones(prueba_compilador)
add_test(NAME compilador COMMAND prueba_compilador)

add_executable(prueba_expresiones prueba_expresiones.cpp)
target_link_libraries(prueba_expresiones PRIVATE nooki_nucleo)
nooki_aplicar_opciones(prueba_expresiones)
add_test(NAME expresiones COMMAND prueba_expresiones)

add_executable(prueba_sentencias prueba_sentencias.cpp)
target_link_libraries(prueba_sentencias PRIVATE nooki_nucleo)
nooki_aplicar_opciones(prueba_sentencias)
add_test(NAME sentencias COMMAND prueba_sentencias)

add_executable(prueba_semantico prueba_semantico.cpp)
target_link_libraries(prueba_semantico PRIVATE nooki_nucleo)
nooki_aplicar_opciones(prueba_semantico)
add_test(NAME semantico COMMAND prueba_semantico)

# Arnés de conformidad: ejecuta los programas .oki de pruebas/conformidad.
add_executable(nooki_conformidad arnes_conformidad.cpp)
target_link_libraries(nooki_conformidad PRIVATE nooki_nucleo)
nooki_aplicar_opciones(nooki_conformidad)
target_compile_definitions(nooki_conformidad
    PRIVATE NOOKI_DIR_CONFORMIDAD="${CMAKE_SOURCE_DIR}/pruebas/conformidad")
add_test(NAME conformidad COMMAND nooki_conformidad)
