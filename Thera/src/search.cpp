#include "Thera/search.hpp"
#include "Thera/Utils/ChessTerms.hpp"

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
    if (generator.isCheck(board)) eval += EvaluationValues::checkBonus;

    // does our opponent give check
    board.switchPerspective();
    generator.generateAttackData(board);
    if (generator.isCheck(board)) eval -= EvaluationValues::checkBonus;


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


float negamax(Board& board, MoveGenerator& generator, int depth, float alpha, float beta){
    if (depth == 0) return evaluate(board, generator);

    auto moves = generator.generateAllMoves(board);
    if (moves.size() == 0){
        generator.generateAttackData(board);
        if ((generator.getAttackedSquares() & board.getBitboard({PieceType::King, board.getColorToMove()})).hasPieces()){
            return -std::numeric_limits<float>::infinity();
        }
        else{
            return 0;
        }
    }

    float bestEvaluation = std::numeric_limits<float>::lowest();
    for (auto move : moves){
        board.applyMove(move);
        float eval = -negamax(board, generator, depth-1, -beta, -alpha);
        board.rewindMove();
        alpha = std::max(alpha, eval);
        bestEvaluation = std::max(bestEvaluation, eval);
        if (alpha > beta)
            break;
    }

    return bestEvaluation;
}

std::vector<EvaluatedMove> search(Board& board, MoveGenerator& generator, int depth){
    if (depth == 0) throw std::invalid_argument("Depth may not be 0");

    auto moves = generator.generateAllMoves(board);
    // move preordering
    std::vector<EvaluatedMove> evaluatedMoves;
    for (auto move : moves){
        evaluatedMoves.emplace_back(move);
    }


    // iterative deepening
    for (int currentDepth=1; currentDepth < depth; currentDepth++){
        float alpha = -std::numeric_limits<float>::infinity();
        float beta = std::numeric_limits<float>::infinity();
        std::sort(evaluatedMoves.begin(), evaluatedMoves.end());

        for (auto& move : evaluatedMoves){
            board.applyMove(move.move);
            move.eval = -negamax(board, generator, currentDepth-1, -beta, -alpha);
            board.rewindMove();

            alpha = std::max(alpha, move.eval);
            if (alpha > beta)
                break;

        }
    }

    return evaluatedMoves;
}

EvaluatedMove getRandomBestMove(std::vector<EvaluatedMove> const& moves){
    float bestEval = moves.front().eval;
    std::vector<EvaluatedMove> bestMoves;
    for (auto move : moves){
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