cmake_minimum_required(VERSION 3.10)

project(SH_img_packer)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})


add_executable(${PROJECT_NAME}
src/main.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    includes
)