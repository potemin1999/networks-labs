set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/week10)
set(CMAKE_EXE_LINKER_FLAGS "-pthread -Wall -Wextra -O3")
add_executable(node_unified_secured node.c)