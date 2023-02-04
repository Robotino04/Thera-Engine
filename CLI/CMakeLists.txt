cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE CLI_SRC "*.cpp" "*.hpp" "*.tpp")

add_executable(CLI ${CLI_SRC})

target_link_libraries(CLI PUBLIC Thera ANSI)
target_include_directories(CLI PUBLIC "include/")