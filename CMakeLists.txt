cmake_minimum_required(VERSION 3.3)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CMakeDependentOption)
include(HunterGate)

HunterGate(
    URL "https://github.com/cpp-pm/hunter/archive/v0.23.249.tar.gz"
    SHA1 "d45d77d8bba9da13e9290a180e0477e90accd89b"
    LOCAL
)

project(cjaeger VERSION 0.5)

hunter_add_package(thrift)
find_package(thrift CONFIG REQUIRED)

hunter_add_package(opentracing-cpp)
find_package(OpenTracing CONFIG REQUIRED)

add_library(cjaeger SHARED src/cjaeger.cpp)
set_target_properties(cjaeger PROPERTIES POSITION_INDEPENDET_CODE ON)
set_target_properties(cjaeger PROPERTIES PUBLIC_HEADER src/cjaeger.h)
target_link_libraries(cjaeger PUBLIC thrift::thrift_static jaegertracing)

include(GNUInstallDirs)

install(
    TARGETS cjaeger 
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
