#pragma once

#include "Thera/Move.hpp"
#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"

#include <tuple>


namespace Thera{
float evaluate(Board const& board);

std::pair<Move, float> search(Board& board, MoveGenerator& generator, int depth);

}