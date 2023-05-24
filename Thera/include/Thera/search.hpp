#pragma once

#include "Thera/Move.hpp"
#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"

#include <tuple>
#include <limits>
#include <stdexcept>
#include <optional>
#include <chrono>


namespace Thera{

static constexpr float evalInfinity = std::numeric_limits<float>::max();

struct SearchStopException : public std::exception{};

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
struct SearchResult{
    std::vector<EvaluatedMove> moves;
    int depthReached;
};

float evaluate(Board& board, MoveGenerator& generator);

SearchResult search(Board& board, MoveGenerator& generator, int depth, std::optional<std::chrono::milliseconds> maxSearchTime);

EvaluatedMove getRandomBestMove(SearchResult const& moves);

}