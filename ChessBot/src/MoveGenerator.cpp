#include "ChessBot/MoveGenerator.hpp"
#include "ChessBot/Board.hpp"
#include "ChessBot/Utils.hpp"

namespace ChessBot{

std::vector<Move> MoveGenerator::generateAllMoves(Board& board){
    generatedMoves.clear();

    generateAllSlidingMoves(board);

    return generatedMoves;
}

std::vector<Move> MoveGenerator::generateMoves(Board& board, int8_t square){
    generatedMoves.clear();

    generateSlidingMoves(board, square);

    return generatedMoves;
}

void MoveGenerator::generateSlidingMoves(Board& board, int8_t square){
    int8_t startDirectionIdx = 0;
    int8_t endDirectionIdx = MoveGenerator::slidingPieceOffsets.size();
    if (board.at(square).getType() == PieceType::Rook) endDirectionIdx = 4;
    else if (board.at(square).getType() == PieceType::Bishop) startDirectionIdx = 4;
    else if (board.at(square).getType() == PieceType::Queen);
    else return;
    
    for (int directionIdx = startDirectionIdx; directionIdx < endDirectionIdx; directionIdx++){
        int8_t pos = square;
        int8_t direction = MoveGenerator::slidingPieceOffsets.at(directionIdx);

        while (Board::isOnBoard(pos, direction)) {
            pos = Board::applyOffset(pos, direction);

            if (board.at(pos).getType() == PieceType::None)
                // normal move
                generatedMoves.emplace_back(square, pos);
            else if (board.at(pos).getColor() == Utils::oppositeColor(board.getColorToMove())){
                // capture
                generatedMoves.emplace_back(square, pos);
                break;
            }
            else
                // we hit our own piece
                break;
        }
    }
}

void MoveGenerator::generateAllSlidingMoves(Board& board){
    for (int8_t i = 0; i<64; i++)
        generateSlidingMoves(board, i);
}

}