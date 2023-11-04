#pragma once

#include "Thera/Move.hpp"
#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"

#include <tuple>
#include <limits>
#include <stdexcept>
#include <optional>
#include <chrono>
#include <functional>
#include <atomic>


namespace Thera{

static constexpr int evalInfinity = std::numeric_limits<int>::max();

struct SearchStopException : public std::exception{};

struct EvaluatedMove{
    Move move;
    int eval = -std::numeric_limits<int>::infinity();
    std::optional<Move> ponderMove;

    bool operator < (EvaluatedMove other) const{
        if (eval != other.eval){
            return eval < other.eval;
        }
        return move < other.move;
    }
};
struct SearchResult{
    std::vector<EvaluatedMove> moves;
    int depthReached=0;
    bool isMate=false;
    int maxEval;
    uint64_t nodesSearched=0;
};

struct NegamaxState{
    int depth;
    int alpha;
    int beta;

    NegamaxState nextDepth(int searchExtensions=0){
        return NegamaxState{
            .depth = depth-1+searchExtensions,
            .alpha = -beta,
            .beta = -alpha,
        };
    }
    
    bool negamaxStep(int newEval, int& bestEval){
        alpha = std::max(alpha, newEval);
        bestEval = std::max(bestEval, newEval);
        return alpha > beta;
    }
};

int evaluate(Board& board, MoveGenerator& generator);

SearchResult search(Board& board, MoveGenerator& generator, int depth, std::optional<std::chrono::milliseconds> maxSearchTime, std::atomic<bool> const& searchWasTerminated, std::function<void(SearchResult const&)> iterationEndCallback);

EvaluatedMove getRandomBestMove(SearchResult const& moves);

}