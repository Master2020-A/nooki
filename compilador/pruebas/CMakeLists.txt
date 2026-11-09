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
