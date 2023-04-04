#pragma once

#include "Thera/Move.hpp"
#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"

#include <functional>


namespace Thera{

/**
 * @brief Run the perft algorithm on a given board.
 * 
 * @param board the board to run perft for
 * @param generator the move generator
 * @param depth the maximum search depth
 * @param bulkCounting is bulk counting allowed
 * @param printFn called for every move with the number of submoves
 * @param isInitialCall is this the first call, so not yet recursive
 * @return int the number of nodes searched
 */
int perft(Board& board, MoveGenerator& generator, int depth, bool bulkCounting, std::function<void(Move const&, int)> printFn, int& filteredMoves, bool isInitialCall=true);
}
