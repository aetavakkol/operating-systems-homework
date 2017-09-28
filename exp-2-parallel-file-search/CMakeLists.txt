cmake_minimum_required(VERSION 3.7)
project(bbm342-exp2)

set(CMAKE_C_STANDARD 99)
set(SOURCE_FILES main.c minion.c)

add_executable(get_sources ${SOURCE_FILES})

add_custom_target(bbm342-exp2 COMMAND make -C ${bbm342-exp2_SOURCE_DIR}
        CLION_EXE_DIR=${PROJECT_BINARY_DIR})
