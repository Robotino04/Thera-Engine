#pragma once

#include <functional>

#include "ChessBot/Board.hpp"
#include "ChessBot/Move.hpp"
#include "ChessBot/MoveGenerator.hpp"

/**
 * @brief Run the perft algorithm on a given board.
 * 
 * @param depth the maximum search depth
 * @param bulkCounting is bulk counting allowed
 * @param printFn called for every move with the number of submoves
 * @param board the board to run perft for
 * @param generator the move generator
 * @param isInitialCall is this the first call, so not yet recursive
 * @return int the number of nodes searched
 */
int perft(int depth, bool bulkCounting, std::function<void(ChessBot::Move const&, int)> printFn, ChessBot::Board& board, ChessBot::MoveGenerator& generator, bool isInitialCall=true);