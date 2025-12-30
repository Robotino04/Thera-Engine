cmake_minimum_required(VERSION 4.0)

file(GLOB_RECURSE UCI_SRC "*.cpp" "*.hpp" "*.tpp")

add_executable(TheraUCI ${UCI_SRC})

target_link_libraries(TheraUCI PUBLIC Thera)
target_include_directories(TheraUCI PUBLIC "include/")