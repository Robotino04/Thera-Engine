#include "Thera/search.hpp"

#include <numeric>

namespace Thera{

float evaluate(Board const& board){
    PieceColor color = board.getColorToMove();
    PieceColor otherColor = board.getColorToNotMove();
    return board.getBitboard({PieceType::Pawn, color}).getNumPieces()
        +  board.getBitboard({PieceType::Bishop, color}).getNumPieces() * 3
        +  board.getBitboard({PieceType::Knight, color}).getNumPieces() * 3
        +  board.getBitboard({PieceType::Rook, color}).getNumPieces() * 5
        +  board.getBitboard({PieceType::Queen, color}).getNumPieces() * 9
        -  board.getBitboard({PieceType::Pawn, otherColor}).getNumPieces()
        -  board.getBitboard({PieceType::Bishop, otherColor}).getNumPieces() * 3
        -  board.getBitboard({PieceType::Knight, otherColor}).getNumPieces() * 3
        -  board.getBitboard({PieceType::Rook, otherColor}).getNumPieces() * 5
        -  board.getBitboard({PieceType::Queen, otherColor}).getNumPieces() * 9;
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
    if (depth == 0) return evaluate(board);

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

std::pair<Move, float> search(Board& board, MoveGenerator& generator, int depth){
    if (depth == 0) throw std::invalid_argument("Depth may not be 0");

    auto moves = generator.generateAllMoves(board);

    float bestEvaluation = -std::numeric_limits<float>::infinity();
    Move bestMove;
    float alpha = -std::numeric_limits<float>::infinity();
    float beta = std::numeric_limits<float>::infinity();
    for (auto move : moves){
        board.applyMove(move);
        float eval = -negamax(board, generator, depth-1, -beta, -alpha);
        board.rewindMove();

        alpha = std::max(alpha, eval);
        if (alpha > beta)
            break;

        // has to be >= to make sura a move is selected even if the position is unwinnable
        if (eval >= bestEvaluation){
            bestEvaluation = eval;
            bestMove = move;
        }

    }

    return {bestMove, bestEvaluation};
}

}