#include "Thera/search.hpp"
#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Utils/ScopeGuard.hpp"

#include <numeric>
#include <cstdlib>
#include <algorithm>
#include <unordered_map>

namespace Thera{

namespace EvaluationValues{
static const std::unordered_map<PieceType, float> pieceValues = {
    {PieceType::Pawn, 100},
    {PieceType::Bishop, 300},
    {PieceType::Knight, 300},
    {PieceType::Rook, 500},
    {PieceType::Queen, 900},
    {PieceType::King, 0},
};

static const float checkBonus = 50;
}

struct TranspositionTableEntry{
    enum class Flag{
        Exact,
        LowerBound,
        UpperBound,
    } flag;
    float eval;
    int depth;
};

float getMaterial(PieceColor color, Board const& board){
    float score = 0;
    for (auto type : Utils::allPieceTypes){
        score += board.getBitboard({type, color}).getNumPieces() * EvaluationValues::pieceValues.at(type);
    }
    return score;
}

float evaluate(Board& board, MoveGenerator& generator){
    PieceColor color = board.getColorToMove();
    PieceColor otherColor = board.getColorToNotMove();

    float eval = 0;
    eval += getMaterial(color, board);
    eval -= getMaterial(otherColor, board);

    // check bonus
    // are we giving check
    board.switchPerspective();
    generator.generateAttackData(board);
    if (generator.isInCheck(board))
        eval += EvaluationValues::checkBonus;

    // does our opponent give check
    board.switchPerspective();
    generator.generateAttackData(board);
    if (generator.isInCheck(board))
        eval -= EvaluationValues::checkBonus;

    return eval;
}

float quiesceNegamax(Board& board, MoveGenerator& generator, float alpha, float beta, std::optional<std::chrono::steady_clock::time_point> searchStop, SearchResult& searchResult){
    if (searchStop.has_value() && std::chrono::steady_clock::now() >= searchStop.value()) throw SearchStopException();

    float eval = evaluate(board, generator);
    searchResult.nodesSearched++;
    if (eval >= beta){
        return beta;
    }
    alpha = std::max(alpha, eval);

    generator.capturesOnly = true;
    auto moves = generator.generateAllMoves(board);
    generator.capturesOnly = false;
    for (auto move : moves){
        board.applyMove(move);
        Utils::ScopeGuard moveRewind_guard([&](){board.rewindMove();});
        eval = -quiesceNegamax(board, generator, -beta, -alpha, searchStop, searchResult);
        if (eval >= beta){
            return beta;
        }
        alpha = std::max(alpha, eval);
    }

    return alpha;
}

float negamax(Board& board, MoveGenerator& generator, int depth, float alpha, float beta, std::optional<std::chrono::steady_clock::time_point> searchStop, std::unordered_map<uint64_t, TranspositionTableEntry>& transpositionTable, SearchResult& searchResult){
    if (searchStop.has_value() && std::chrono::steady_clock::now() >= searchStop.value()) throw SearchStopException();

    if (transpositionTable.contains(board.getCurrentHash())){
        auto entry = transpositionTable.at(board.getCurrentHash());
        if (entry.depth >= depth){
            if (entry.flag == TranspositionTableEntry::Flag::Exact)
                return entry.eval;
            else if (entry.flag == TranspositionTableEntry::Flag::LowerBound)
                alpha = std::max(alpha, entry.eval);
            else if (entry.flag == TranspositionTableEntry::Flag::UpperBound)
                beta = std::min(beta, entry.eval);
            if (alpha >= beta)
                return entry.eval;
        }
    }

    if (depth == 0){
        float eval = quiesceNegamax(board, generator, alpha, beta, searchStop, searchResult);
        return eval;
    }

    auto moves = generator.generateAllMoves(board);

    if (moves.size() == 0){
        if (generator.isInCheck(board)){
            alpha = -evalInfinity;
        }
        else{
            alpha = 0.0f;
        }
    }
    else{
        for (auto move : moves){
            board.applyMove(move);
            Utils::ScopeGuard moveRewind_guard([&](){board.rewindMove();});
            float eval = -negamax(board, generator, depth-1, -beta, -alpha, searchStop, transpositionTable, searchResult);
            if (eval >= beta){
                return beta;
            }
            alpha = std::max(alpha, eval);
        }
    }

    TranspositionTableEntry entry;
    entry.eval = alpha;
    entry.depth = depth;

    if (entry.eval <= alpha)
        entry.flag = TranspositionTableEntry::Flag::UpperBound;
    else if (entry.eval >= beta)
        entry.flag = TranspositionTableEntry::Flag::LowerBound;
    else
        entry.flag = TranspositionTableEntry::Flag::Exact;

    if (transpositionTable.contains(board.getCurrentHash()))
        transpositionTable.at(board.getCurrentHash()) = entry;
    else
        transpositionTable.insert({board.getCurrentHash(), entry});

    return entry.eval;
}

SearchResult search(Board& board, MoveGenerator& generator, int depth, std::optional<std::chrono::milliseconds> maxSearchTime, std::function<void(SearchResult const&)> iterationEndCallback){
    if (depth == 0) throw std::invalid_argument("Depth may not be 0");

    auto moves = generator.generateAllMoves(board);
    // move preordering
    SearchResult result;
    result.depthReached = 0;
    for (auto move : moves){
        result.moves.emplace_back(move);
    }
    SearchResult resultTmp = result;

    std::chrono::steady_clock::time_point searchStopTP = std::chrono::steady_clock::now() + maxSearchTime.value_or(std::chrono::milliseconds(0));


    // iterative deepening
    for (int currentDepth=1; currentDepth <= depth; currentDepth++){
        float alpha = -evalInfinity;
        float beta = evalInfinity;
        // sort in reverse to first search the best moves
        std::sort(resultTmp.moves.rbegin(), resultTmp.moves.rend());
        try{
            for (auto& move : resultTmp.moves){
                std::unordered_map<uint64_t, TranspositionTableEntry> transpositionTable;
                board.applyMove(move.move);
                Utils::ScopeGuard moveRewind_guard([&](){board.rewindMove();});
                move.eval = -negamax(board, generator, currentDepth-1, -beta, -alpha, searchStopTP, transpositionTable, resultTmp);

                alpha = std::max(alpha, move.eval);
                if (alpha > beta)
                    break;
            }
        }
        catch(SearchStopException){
            return result;
        }
        resultTmp.depthReached = currentDepth;
        result = resultTmp;
        resultTmp.nodesSearched = 0;
        // exit early if a checkmate is found
        result.maxEval = -evalInfinity;
        for (auto move : result.moves){
            result.maxEval = std::max(result.maxEval, move.eval);
        }
        result.isMate = std::abs(result.maxEval) == evalInfinity;

        iterationEndCallback(result);

        if (result.isMate){
            return result;
        }
    }

    return result;
}

EvaluatedMove getRandomBestMove(SearchResult const& moves){
    float bestEval = moves.moves.front().eval;
    std::vector<EvaluatedMove> bestMoves;
    for (auto move : moves.moves){
        if (move.eval > bestEval){
            bestMoves.clear();
            bestEval = move.eval;
        }
        if (move.eval == bestEval){
            bestMoves.push_back(move);
        }
    }

    const int selectedMoveIndex = rand() % bestMoves.size();
    return bestMoves.at(selectedMoveIndex);
}

}