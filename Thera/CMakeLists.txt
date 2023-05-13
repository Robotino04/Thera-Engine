cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE THERA_SRC "*.cpp" "*.hpp" "*.tpp")

add_library(Thera STATIC ${THERA_SRC})

target_include_directories(Thera PUBLIC "include/")
target_compile_features(Thera PUBLIC cxx_std_20)

target_link_libraries(Thera PUBLIC ANSI)