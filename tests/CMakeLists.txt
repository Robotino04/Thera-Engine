cmake_minimum_required(VERSION 3.0)

add_executable(test_perft_fen "src/test_perft_fen.cpp")
target_link_libraries(test_perft_fen PUBLIC Thera ANSI)

function(add_fen_test_range NAME FEN RESULTS)
    list(LENGTH RESULTS NUM_RESULTS)
    foreach(DEPTH RANGE 1 ${NUM_RESULTS})
        math(EXPR INDEX ${DEPTH}-1)
        list(GET RESULTS ${INDEX} EXPECTED_RESULT)
        add_test(NAME ${NAME}_${DEPTH} COMMAND test_perft_fen "${DEPTH}" "${FEN}" "1" "${EXPECTED_RESULT}")
    endforeach(DEPTH)
endfunction(add_fen_test_range)

function(add_test_from_source_file NAME)
    add_executable(test_${NAME} "src/test_${NAME}.cpp")
    target_link_libraries(test_${NAME} PUBLIC Thera ANSI)
    add_test(NAME ${NAME} COMMAND test_${NAME})
endfunction(add_test_from_source_file)

# perft results from https://www.chessprogramming.org/Perft_Results
add_fen_test_range(startpos "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" "20;400;8902;197281;4865609")
add_fen_test_range(position2 "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -" "48;2039;97862;4085603")
add_fen_test_range(position3 "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -" "14;191;2812;43238;674624")
add_fen_test_range(position4 "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1" "6;264;9467;422333")
add_fen_test_range(position5 "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8" "44;1486;62379;2103487")
add_fen_test_range(position6 "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 " "46;2079;89890;3894594")
add_fen_test_range(castling_when_in_check "r3k2r/p1pPqpb1/1n3np1/1b2N3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R b KQkq - 0 1" "6")
add_fen_test_range(castling_when_promotion "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q2/PPPBBPpP/1R2K2R w Kqk - 0 1" "44")

add_test_from_source_file(bitboard1)