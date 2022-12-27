#include "ChessBot/MoveGenerator.hpp"
#include "ChessBot/Board.hpp"
#include "ChessBot/Utils.hpp"

namespace ChessBot{

std::vector<Move> MoveGenerator::generateAllMoves(Board& board){
    generatedMoves.clear();

    generateAllSlidingMoves(board);
    generateAllKingKnightMoves(board);

    return generatedMoves;
}

std::vector<Move> MoveGenerator::generateMoves(Board& board, int8_t square){
    if (board.at(square).getColor() != board.getColorToMove()) return {};
    generatedMoves.clear();

    generateSlidingMoves(board, square);
    generateKingKnightMoves(board, square);

    return generatedMoves;
}

void MoveGenerator::generateSlidingMoves(Board& board, int8_t square){
    if (board.at(square).getColor() != board.getColorToMove()) return;

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

void MoveGenerator::generateKingKnightMoves(Board& board, int8_t square){
    if (board.at(square).getColor() != board.getColorToMove()) return;

    int8_t startDirectionIdx = 0;
    int8_t endDirectionIdx = MoveGenerator::kingKnightOffsets.size();
    if (board.at(square).getType() == PieceType::King) endDirectionIdx = 8;
    else if (board.at(square).getType() == PieceType::Knight) startDirectionIdx = 8;
    else return;
    
    for (int directionIdx = startDirectionIdx; directionIdx < endDirectionIdx; directionIdx++){
        int8_t pos = square;
        int8_t direction = MoveGenerator::kingKnightOffsets.at(directionIdx);

        if (Board::isOnBoard(pos, direction)) {
            pos = Board::applyOffset(pos, direction);

            if (board.at(pos).getType() == PieceType::None)
                // normal move
                generatedMoves.emplace_back(square, pos);
            else if (board.at(pos).getColor() == Utils::oppositeColor(board.getColorToMove())){
                // capture
                generatedMoves.emplace_back(square, pos);
            }
        }
    }
    if (board.at(square).getType() == PieceType::King){
        // add castling moves
        if (board.getCastleRight(board.at(square).getColor())){
            if (board.at(Board::applyOffset(square, 1)).getType() == PieceType::None &&
                board.at(Board::applyOffset(square, 2)).getType() == PieceType::None){
                    // king movement
                    Move& move = generatedMoves.emplace_back();
                    move.startIndex = square;
                    move.endIndex = Board::applyOffset(square, 2);
                    // rook movement
                    move.auxiliaryMove = new Move();
                    move.auxiliaryMove->startIndex = Board::applyOffset(square, 3);
                    move.auxiliaryMove->endIndex = Board::applyOffset(square, 1);
                }
        }
        if (board.getCastleLeft(board.at(square).getColor())){
            if (board.at(Board::applyOffset(square, -1)).getType() == PieceType::None &&
                board.at(Board::applyOffset(square, -2)).getType() == PieceType::None &&
                board.at(Board::applyOffset(square, -3)).getType() == PieceType::None){
                    // king movement
                    Move& move = generatedMoves.emplace_back();
                    move.startIndex = square;
                    move.endIndex = Board::applyOffset(square, -2);
                    // rook movement
                    move.auxiliaryMove = new Move();
                    move.auxiliaryMove->startIndex = Board::applyOffset(square, -4);
                    move.auxiliaryMove->endIndex = Board::applyOffset(square, -1);
                }
        }
    }
}

void MoveGenerator::generateAllKingKnightMoves(Board& board){
    for (int8_t i = 0; i<64; i++)
        generateKingKnightMoves(board, i);
}

}