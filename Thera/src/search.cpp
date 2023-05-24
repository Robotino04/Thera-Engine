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

float evaluate(Board& board, MoveGenerator& generator){
    PieceColor color = board.getColorToMove();
    PieceColor otherColor = board.getColorToNotMove();

    // checkmates
    board.switchPerspective();
    auto moves = generator.generateAllMoves(board);
    if (moves.size() == 0){
        if (generator.isInCheck(board)){
            return evalInfinity;
        }
    }

    board.switchPerspective();
    moves = generator.generateAllMoves(board);
    if (moves.size() == 0){
        if (generator.isInCheck(board)){
            return -evalInfinity;
        }
    }

    float eval = 0;
    // piece values
    for (auto type : Utils::allPieceTypes){
        eval += board.getBitboard({type, color}).getNumPieces() * EvaluationValues::pieceValues.at(type);
        eval -= board.getBitboard({type, otherColor}).getNumPieces() * EvaluationValues::pieceValues.at(type);
    }
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


// int miniMax(int spieler, int tiefe) {
//     if (tiefe == 0 or keineZuegeMehr(spieler))
//        return bewerten(spieler);
//     int maxWert = -unendlich;
//     generiereMoeglicheZuege(spieler);
//     while (noch Zug da) {
//        fuehreNaechstenZugAus();
//        int wert = -miniMax(-spieler, tiefe-1);
//        macheZugRueckgaengig();
//        if (wert > maxWert) {
//           maxWert = wert;
//           if (tiefe == gewuenschteTiefe)
//              gespeicherterZug = Zug;
//        }
//     }
//     return maxWert;
//  }

float negamax(Board& board, MoveGenerator& generator, int depth, float alpha, float beta, std::optional<std::chrono::steady_clock::time_point> searchStop){
    if (depth == 0) return evaluate(board, generator);
    if (searchStop.has_value() && std::chrono::steady_clock::now() >= searchStop.value()) throw SearchStopException();

    auto moves = generator.generateAllMoves(board);

    float bestEvaluation = std::numeric_limits<float>::lowest();
    for (auto move : moves){
        board.applyMove(move);
        Utils::ScopeGuard moveRewind_guard([&](){board.rewindMove();});
        float eval = -negamax(board, generator, depth-1, -beta, -alpha, searchStop);
        alpha = std::max(alpha, eval);
        bestEvaluation = std::max(bestEvaluation, eval);
        if (alpha > beta)
            break;
    }

    return bestEvaluation;
}

SearchResult search(Board& board, MoveGenerator& generator, int depth, std::optional<std::chrono::milliseconds> maxSearchTime){
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
                board.applyMove(move.move);
                Utils::ScopeGuard moveRewind_guard([&](){board.rewindMove();});
                move.eval = -negamax(board, generator, currentDepth-1, -beta, -alpha, searchStopTP);

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
        // exit early if a checkmate is found
        for (auto move : result.moves){
            if (std::abs(move.eval) == evalInfinity){
                return result;
            }
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