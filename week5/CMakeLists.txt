set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/week5)
SET(CMAKE_EXE_LINKER_FLAGS "-pthread")
add_executable(server_udp server.c)
add_executable(client_udp client.c)