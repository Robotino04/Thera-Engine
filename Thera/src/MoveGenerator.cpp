#include "Thera/MoveGenerator.hpp"
#include "Thera/Board.hpp"
#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Utils/Coordinates.hpp"

namespace Thera{

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

        while (Utils::isOnBoard8x8(pos, direction)) {
            pos = Utils::applyOffset(pos, direction);

            if (board.at(pos).getType() == PieceType::None)
                // normal move
                generatedMoves.emplace_back(Utils::to10x12Coords(square), Utils::to10x12Coords(pos));
            else if (board.at(pos).getColor() == board.getColorToMove().opposite()){
                // capture
                generatedMoves.emplace_back(Utils::to10x12Coords(square), Utils::to10x12Coords(pos));
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

        if (Utils::isOnBoard8x8(pos, direction)) {
            pos = Utils::applyOffset(pos, direction);

            if (board.at(pos).getType() == PieceType::None)
                // normal move
                generatedMoves.emplace_back(Utils::to10x12Coords(square), Utils::to10x12Coords(pos));
            else if (board.at(pos).getColor() == board.getColorToMove().opposite()){
                // capture
                generatedMoves.emplace_back(Utils::to10x12Coords(square), Utils::to10x12Coords(pos));
            }
        }
    }
    if (board.at(square).getType() == PieceType::King){
        // add castling moves
        if (board.getCastleRight(board.at(square).getColor())){
            if (board.at(Utils::applyOffset(square, 1)).getType() == PieceType::None &&
                board.at(Utils::applyOffset(square, 2)).getType() == PieceType::None){
                    // king movement
                    Move& move = generatedMoves.emplace_back(Utils::to10x12Coords(square), Utils::to10x12Coords(Utils::applyOffset(square, 2)));
                    move.isCastling = true;
                    // rook movement
                    move.auxiliaryMove = new Move();
                    move.auxiliaryMove->startIndex = Utils::to10x12Coords(Utils::applyOffset(square, 3));
                    move.auxiliaryMove->endIndex = Utils::to10x12Coords(Utils::applyOffset(square, 1));
                }
        }
        if (board.getCastleLeft(board.at(square).getColor())){
            if (board.at(Utils::applyOffset(square, -1)).getType() == PieceType::None &&
                board.at(Utils::applyOffset(square, -2)).getType() == PieceType::None &&
                board.at(Utils::applyOffset(square, -3)).getType() == PieceType::None){
                    // king movement
                    Move& move = generatedMoves.emplace_back(Utils::to10x12Coords(square), Utils::to10x12Coords(Utils::applyOffset(square, -2)));
                    move.isCastling = true;
                    // rook movement
                    move.auxiliaryMove = new Move();
                    move.auxiliaryMove->startIndex = Utils::to10x12Coords(Utils::applyOffset(square, -4));
                    move.auxiliaryMove->endIndex = Utils::to10x12Coords(Utils::applyOffset(square, -1));
                }
        }
    }
}

void MoveGenerator::generateAllKingKnightMoves(Board& board){
    auto const bitboard = board.getBitboard({PieceType::King, board.getColorToMove()}) | board.getBitboard({PieceType::Knight, board.getColorToMove()});
    const auto squares = bitboard.getPieces8x8();
    const auto numPieces = bitboard.getNumPieces();

    for (int i=0; i<numPieces; i++){
        generateKingKnightMoves(board, squares.at(i));
    }
}

void MoveGenerator::generatePawnMoves(Board& board, int8_t square){
    if (board.at(square).getColor() != board.getColorToMove()) return;
    if (board.at(square).getType() != PieceType::Pawn) return;

    const int8_t baseLine = board.at(square).getColor() == PieceColor::White ? 6 : 1;
    const int8_t targetLine = board.at(square).getColor() == PieceColor::White ? 0 : 7;
    const int8_t direction = board.at(square).getColor() == PieceColor::White ? -1 : 1;
    const int8_t direction10x12 = board.at(square).getColor() == PieceColor::White ? -10 : 10;

    if (!board.isOccupied(Utils::to10x12Coords(Utils::applyOffset(square, direction10x12)))){
        // normal move
        Move move;
        move.startIndex = Utils::to10x12Coords(square);
        move.endIndex = Utils::to10x12Coords(Utils::applyOffset(square, direction10x12));
        addPawnMove(move, board);

        // double move
        if (Utils::isOnRow(square, baseLine) && !board.isOccupied(Utils::to10x12Coords(Utils::applyOffset(square, direction10x12*2)))){
            Move move;
            move.startIndex = Utils::to10x12Coords(square);
            move.endIndex = Utils::to10x12Coords(Utils::applyOffset(square, direction10x12*2));
            move.enPassantFile = Utils::getXCoord(Utils::to8x8Coords(move.endIndex));
            move.isDoublePawnMove = true;
            addPawnMove(move, board);
        }
    }

    // captures
    if (Utils::isOnBoard8x8(square, direction10x12-1) && board.isOccupied(Utils::to10x12Coords(Utils::applyOffset(square, direction10x12-1))) && !board.isFriendly(Utils::to10x12Coords(Utils::applyOffset(square, direction10x12-1)))){
        // left
        Move move;
        move.startIndex = Utils::to10x12Coords(square);
        move.endIndex = Utils::to10x12Coords(Utils::applyOffset(square, direction10x12-1));
        addPawnMove(move, board);
    }
    if (Utils::isOnBoard8x8(square, direction10x12+1) && board.isOccupied(Utils::to10x12Coords(Utils::applyOffset(square, direction10x12+1))) && !board.isFriendly(Utils::to10x12Coords(Utils::applyOffset(square, direction10x12+1)))){
        // right
        Move move;
        move.startIndex = Utils::to10x12Coords(square);
        move.endIndex = Utils::to10x12Coords(Utils::applyOffset(square, direction10x12+1));
        addPawnMove(move, board);
    }

    // en passant
    if (Utils::isOnBoard8x8(square, -1) && board.getEnPassantSquare() == Utils::to10x12Coords(Utils::applyOffset(square, direction10x12-1))){
        // left
        Move move;
        move.startIndex = Utils::to10x12Coords(square);
        move.endIndex = Utils::to10x12Coords(Utils::applyOffset(square, direction10x12-1));
        move.isEnPassant = true;
        addPawnMove(move, board);
    }
    else if (Utils::isOnBoard8x8(square, 1) && board.getEnPassantSquare() == Utils::to10x12Coords(Utils::applyOffset(square, direction10x12+1))){
        // right
        Move move;
        move.startIndex = Utils::to10x12Coords(square);
        move.endIndex = Utils::to10x12Coords(Utils::applyOffset(square, direction10x12+1));
        move.isEnPassant = true;
        addPawnMove(move, board);
    }
    

}

void MoveGenerator::generateAllPawnMoves(Board& board){
    auto const& bitboard = board.getBitboard({PieceType::Pawn, board.getColorToMove()});
    const auto squares = bitboard.getPieces8x8();

    for (auto squareIt = squares.begin(); squareIt != std::next(squares.begin(), bitboard.getNumPieces()); squareIt++)
        generatePawnMoves(board, *squareIt);
}

void MoveGenerator::addPawnMove(Move const& move, Board const& board){
    const int8_t targetLine = board.at10x12(move.startIndex).getColor() == PieceColor::White ? 0 : 7;
    

    if (Utils::isOnRow(Utils::to8x8Coords(move.endIndex), targetLine)){
        // promotion
        for (PieceType promotionType : Utils::promotionPieces){
            Move& newMove = generatedMoves.emplace_back(move);
            newMove.promotionType = promotionType;
        }
    }
    else{
        generatedMoves.push_back(move);
    }
}
}