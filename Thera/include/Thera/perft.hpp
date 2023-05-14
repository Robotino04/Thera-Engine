#pragma once

#include "Thera/Move.hpp"
#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"

#include <functional>


namespace Thera{

struct PerftResult{
    int numNodesSearched = 0;

    struct SingleMove{
        Move move;
        int numNodesSearched = 0;

        /**
         * @brief Compare two moves
         * 
         * Only used for sorting
         * 
         * @param other the single move to compare to
         * @return bool 
         */
        bool operator < (SingleMove other) const{
            if (move != other.move) return move < other.move;
            else return numNodesSearched < other.numNodesSearched;
        }
        /**
         * @brief Test if two moves are equal
         * 
         * @param other the single move to compare to
         * @return bool are they equal
         */
        bool operator == (SingleMove other) const{ return move == other.move && numNodesSearched == other.numNodesSearched; }
    };
    std::vector<SingleMove> moves;
    int numNodesFiltered = 0;
};

/**
 * @brief Run the perft algorithm on a given board.
 * 
 * @param board the board to run perft for
 * @param generator the move generator
 * @param depth the maximum search depth
 * @param bulkCounting is bulk counting allowed
 * @param printFn called for every move with the number of submoves
 * @param isInitialCall is this the first call, so not yet recursive
 * @return PerftResult the result
 */
PerftResult perft_instrumented(Board& board, MoveGenerator& generator, int depth, bool bulkCounting, bool isInitialCall=true);

/**
 * @brief Run the perft algorithm on a given board.
 * 
 * @param board the board to run perft for
 * @param generator the move generator
 * @param depth the maximum search depth
 * @param bulkCounting is bulk counting allowed
 * @return PerftResult the result
 */
PerftResult perft(Board& board, MoveGenerator& generator, int depth, bool bulkCounting);
}
