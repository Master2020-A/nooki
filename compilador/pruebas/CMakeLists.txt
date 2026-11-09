# compilador/pruebas/CMakeLists.txt

add_executable(prueba_cli prueba_cli.cpp)
target_link_libraries(prueba_cli PRIVATE nooki_nucleo)
nooki_aplicar_opciones(prueba_cli)

add_test(NAME cli COMMAND prueba_cli)
