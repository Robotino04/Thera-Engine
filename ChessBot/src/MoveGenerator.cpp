#include "ChessBot/MoveGenerator.hpp"
#include "ChessBot/Board.hpp"
#include "ChessBot/Utils.hpp"

namespace ChessBot{

std::vector<Move> MoveGenerator::generateAllMoves(Board& board){
    generatedMoves.clear();

    generateAllSlidingMoves(board);
    generateAllKingKnightMoves(board);
    generateAllPawnMoves(board);

    return generatedMoves;
}

std::vector<Move> MoveGenerator::generateMoves(Board& board, int8_t square){
    if (board.at(square).getColor() != board.getColorToMove()) return {};
    generatedMoves.clear();

    generateSlidingMoves(board, square);
    generateKingKnightMoves(board, square);
    generatePawnMoves(board, square);

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
                    Move& move = generatedMoves.emplace_back(square, Board::applyOffset(square, 2));
                    move.isCastling = true;
                    // rook movement
                    move.auxiliaryMove = new Move(Board::applyOffset(square, 3), Board::applyOffset(square, 1));
                }
        }
        if (board.getCastleLeft(board.at(square).getColor())){
            if (board.at(Board::applyOffset(square, -1)).getType() == PieceType::None &&
                board.at(Board::applyOffset(square, -2)).getType() == PieceType::None &&
                board.at(Board::applyOffset(square, -3)).getType() == PieceType::None){
                    // king movement
                    Move& move = generatedMoves.emplace_back(square, Board::applyOffset(square, -2));
                    move.isCastling = true;
                    // rook movement
                    move.auxiliaryMove = new Move(Board::applyOffset(square, -4), Board::applyOffset(square, -1));
                }
        }
    }
}

void MoveGenerator::generateAllKingKnightMoves(Board& board){
    for (int8_t i = 0; i<64; i++)
        generateKingKnightMoves(board, i);
}

void MoveGenerator::generatePawnMoves(Board& board, int8_t square){
    if (board.at(square).getColor() != board.getColorToMove()) return;
    if (board.at(square).getType() != PieceType::Pawn) return;

    const int8_t baseLine = board.at(square).getColor() == PieceColor::White ? 6 : 1;
    const int8_t targetLine = board.at(square).getColor() == PieceColor::White ? 0 : 7;
    const int8_t direction = board.at(square).getColor() == PieceColor::White ? -1 : 1;
    const int8_t direction10x12 = board.at(square).getColor() == PieceColor::White ? -10 : 10;

    if (board.isEmpty(Board::applyOffset(square, direction10x12))){
        // normal move
        Move move;
        move.startIndex = square;
        move.endIndex = Board::applyOffset(square, direction10x12);
        addPawnMove(move, board);

        // double move
        if (Utils::isOnLine(square, baseLine) && board.isEmpty(Board::applyOffset(square, direction10x12*2))){
            Move move;
            move.startIndex = square;
            move.endIndex = Board::applyOffset(square, direction10x12*2);
            move.enPassantFile = Utils::getXCoord(move.endIndex);
            move.isDoublePawnMove = true;
            addPawnMove(move, board);
        }
    }

    // captures
    if (Board::isOnBoard(square, direction10x12-1) && !board.isEmpty(Board::applyOffset(square, direction10x12-1)) && !board.isFriendly(Board::applyOffset(square, direction10x12-1))){
        // left
        Move move;
        move.startIndex = square;
        move.endIndex = Board::applyOffset(square, direction10x12-1);
        addPawnMove(move, board);
    }
    if (Board::isOnBoard(square, direction10x12+1) && !board.isEmpty(Board::applyOffset(square, direction10x12+1)) && !board.isFriendly(Board::applyOffset(square, direction10x12+1))){
        // right
        Move move;
        move.startIndex = square;
        move.endIndex = Board::applyOffset(square, direction10x12+1);
        addPawnMove(move, board);
    }

    // en passant
    if (Board::isOnBoard(square, -1) && board.getEnPassantSquare() == Board::applyOffset(square, -1)){
        // left
        Move move;
        move.startIndex = square;
        move.endIndex = Board::applyOffset(square, direction10x12-1);
        move.isEnPassant = true;
        addPawnMove(move, board);
    }
    else if (Board::isOnBoard(square, 1) && board.getEnPassantSquare() == Board::applyOffset(square, 1)){
        // right
        Move move;
        move.startIndex = square;
        move.endIndex = Board::applyOffset(square, direction10x12+1);
        move.isEnPassant = true;
        addPawnMove(move, board);
    }
    

}

void MoveGenerator::generateAllPawnMoves(Board& board){
    for (int8_t i = 0; i<64; i++)
        generatePawnMoves(board, i);
}

void MoveGenerator::addPawnMove(Move const& move, Board const& board){
    const int8_t targetLine = board.at(move.startIndex).getColor() == PieceColor::White ? 0 : 7;

    if (Utils::isOnLine(move.endIndex, targetLine)){
        // promotion
        for (PieceType promoitionType : Utils::promotionPieces){
            Move& newMove = generatedMoves.emplace_back(move);
            newMove.promotionType = promoitionType;
        }
    }
    else{
        generatedMoves.push_back(move);
    }
}
}