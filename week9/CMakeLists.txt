set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/week9)
set(CMAKE_EXE_LINKER_FLAGS "-pthread -Wall -O3")
add_executable(node_unified node.c)