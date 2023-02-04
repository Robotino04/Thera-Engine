#pragma once

#include <functional>

#include "Thera/Board.hpp"
#include "Thera/Move.hpp"
#include "Thera/MoveGenerator.hpp"

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
int perft(int depth, bool bulkCounting, std::function<void(Thera::Move const&, int)> printFn, Thera::Board& board, Thera::MoveGenerator& generator, bool isInitialCall=true);