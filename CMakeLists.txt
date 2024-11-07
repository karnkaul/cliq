cmake_minimum_required(VERSION 3.23)

project(cliqr VERSION "0.4.3")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

option(CLIQR_BUILD_EXAMPLE "Build cliqr example" ${CMAKE_PROJECT_IS_TOP_LEVEL})
option(CLIQR_BUILD_TEST "Build cliqr tests" ${CMAKE_PROJECT_IS_TOP_LEVEL})
option(CLIQER_INSTALL "Setup CMake install for ${PROJECT_NAME}" ${CMAKE_PROJECT_IS_TOP_LEVEL})

add_library(${PROJECT_NAME}-compile-options INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME}-compile-options ALIAS ${PROJECT_NAME}-compile-options)

if(CMAKE_CXX_COMPILER_ID STREQUAL Clang OR CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  target_compile_options(${PROJECT_NAME}-compile-options INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Werror=return-type
  )
endif()

add_subdirectory(lib)

# TEMP
add_subdirectory(lab)

if(CLIQER_BUILD_EXAMPLE)
  # add_subdirectory(example)
endif()

if(CLIQER_BUILD_TEST)
  enable_testing()

  # add_subdirectory(test)
endif()
