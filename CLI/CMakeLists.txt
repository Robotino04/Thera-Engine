cmake_minimum_required(VERSION 3.0)

add_executable(CLI "src/main.cpp")

target_link_libraries(CLI PUBLIC ChessBot ANSI)
target_include_directories(CLI PUBLIC "include/")
