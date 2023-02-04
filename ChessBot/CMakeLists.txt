cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE CHESSBOT_SRC "*.cpp" "*.hpp" "*.tpp")

add_library(ChessBot STATIC ${CHESSBOT_SRC})

target_include_directories(ChessBot PUBLIC "include/")
target_compile_features(ChessBot PUBLIC cxx_std_20)