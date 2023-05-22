#pragma once

#include "Thera/Move.hpp"
#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"

#include <tuple>
#include <limits>


namespace Thera{

struct EvaluatedMove{
    Move move;
    float eval = -std::numeric_limits<float>::infinity();

    bool operator < (EvaluatedMove other) const{
        if (eval != other.eval){
            return eval < other.eval;
        }
        return move < other.move;
    }
};

float evaluate(Board const& board);

std::vector<EvaluatedMove> search(Board& board, MoveGenerator& generator, int depth);

EvaluatedMove getRandomBestMove(std::vector<EvaluatedMove> const& moves);

}