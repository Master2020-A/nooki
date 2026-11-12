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

# Coherencia de la documentación: sprints en el historial y versiones iguales.
add_executable(prueba_documentos prueba_documentos.cpp)
nooki_aplicar_opciones(prueba_documentos)
target_compile_definitions(prueba_documentos PRIVATE _CRT_SECURE_NO_WARNINGS)
add_test(NAME documentos COMMAND prueba_documentos)
set_tests_properties(documentos PROPERTIES
    ENVIRONMENT "NOOKI_DIR_RAIZ=${CMAKE_SOURCE_DIR}")

# Fuzzing: entradas aleatorias y mutadas contra todas las fases.
add_executable(nooki_fuzz fuzz.cpp)
target_link_libraries(nooki_fuzz PRIVATE nooki_nucleo)
nooki_aplicar_opciones(nooki_fuzz)
target_compile_definitions(nooki_fuzz
    PRIVATE NOOKI_DIR_CONFORMIDAD="${CMAKE_SOURCE_DIR}/pruebas/conformidad")
add_test(NAME fuzz COMMAND nooki_fuzz 600)
set_tests_properties(fuzz PROPERTIES TIMEOUT 120)

# Aritmética comprobada: desbordamiento y división por cero terminan con 70.
add_executable(prueba_panico panico.cpp)
target_link_libraries(prueba_panico PRIVATE nooki_nucleo)
nooki_aplicar_opciones(prueba_panico)
add_test(NAME panico COMMAND prueba_panico)
set_tests_properties(panico PROPERTIES TIMEOUT 120)

# Rutas con tildes: compilar y ejecutar un archivo «programa ñ.oki».
add_executable(prueba_rutas rutas.cpp)
target_link_libraries(prueba_rutas PRIVATE nooki_nucleo)
nooki_aplicar_opciones(prueba_rutas)
add_test(NAME rutas COMMAND prueba_rutas)
set_tests_properties(rutas PROPERTIES TIMEOUT 120)

# Rendimiento: línea base de las métricas del plan.
add_executable(nooki_rendimiento rendimiento.cpp)
target_link_libraries(nooki_rendimiento PRIVATE nooki_nucleo)
nooki_aplicar_opciones(nooki_rendimiento)
add_test(NAME rendimiento COMMAND nooki_rendimiento)
set_tests_properties(rendimiento PROPERTIES TIMEOUT 300)

# Arnés de conformidad: ejecuta los programas .oki de pruebas/conformidad.
add_executable(nooki_conformidad arnes_conformidad.cpp)
target_link_libraries(nooki_conformidad PRIVATE nooki_nucleo)
nooki_aplicar_opciones(nooki_conformidad)
target_compile_definitions(nooki_conformidad
    PRIVATE NOOKI_DIR_CONFORMIDAD="${CMAKE_SOURCE_DIR}/pruebas/conformidad")
add_test(NAME conformidad COMMAND nooki_conformidad)
