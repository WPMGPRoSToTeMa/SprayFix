cmake_minimum_required(VERSION 3.9)

project(SprayFix)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_SIZEOF_VOID_P 4)

set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS FALSE)
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB32_PATHS TRUE)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
endif()

add_library(${PROJECT_NAME} SHARED SprayFix.cxx)

target_link_libraries(${PROJECT_NAME} dl)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
