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

std::vector<Move> MoveGenerator::generateMoves(Board& board, Coordinate8x8 square){
    if (board.at(square).getColor() != board.getColorToMove()) return {};
    generatedMoves.clear();

    generateSlidingMoves(board, square);
    generateKingKnightMoves(board, square);
    generatePawnMoves(board, square);

    return generatedMoves;
}

void MoveGenerator::generateSlidingMoves(Board& board, Coordinate8x8 square){
    if (board.at(square).getColor() != board.getColorToMove()) return;

    int8_t startDirectionIdx = 0;
    int8_t endDirectionIdx = MoveGenerator::slidingPieceOffsets.size();
    if (board.at(square).getType() == PieceType::Rook) endDirectionIdx = 4;
    else if (board.at(square).getType() == PieceType::Bishop) startDirectionIdx = 4;
    else if (board.at(square).getType() == PieceType::Queen);
    else return;
    
    for (int directionIdx = startDirectionIdx; directionIdx < endDirectionIdx; directionIdx++){
        Coordinate10x12 pos = square;
        int8_t direction10x12 = MoveGenerator::slidingPieceOffsets.at(directionIdx);

        while (Utils::isOnBoard(pos, direction10x12)) {
            pos = Utils::applyOffset(pos, direction10x12);

            if (board.at(pos).getType() == PieceType::None)
                // normal move
                generatedMoves.emplace_back(square, pos);
            else if (board.at(pos).getColor() == board.getColorToMove().opposite()){
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
        generateSlidingMoves(board, Coordinate8x8(i));
}

void MoveGenerator::generateKingKnightMoves(Board& board, Coordinate8x8 square){
    if (board.at(square).getColor() != board.getColorToMove()) return;

    int8_t startDirectionIdx = 0;
    int8_t endDirectionIdx = MoveGenerator::kingKnightOffsets.size();
    if (board.at(square).getType() == PieceType::King) endDirectionIdx = 8;
    else if (board.at(square).getType() == PieceType::Knight) startDirectionIdx = 8;
    else return;
    
    for (int directionIdx = startDirectionIdx; directionIdx < endDirectionIdx; directionIdx++){
        Coordinate8x8 pos = square;
        int8_t direction10x12 = MoveGenerator::kingKnightOffsets.at(directionIdx);

        if (Utils::isOnBoard(pos, direction10x12)) {
            pos = Utils::applyOffset(pos, direction10x12);

            if (board.at(pos).getType() == PieceType::None)
                // normal move
                generatedMoves.emplace_back(square, pos);
            else if (board.at(pos).getColor() == board.getColorToMove().opposite()){
                // capture
                generatedMoves.emplace_back(square, pos);
            }
        }
    }
    if (board.at(square).getType() == PieceType::King){
        // add castling moves
        if (board.getCastleRight(board.at(square).getColor())){
            if (board.at(Utils::applyOffset(square, 1)).getType() == PieceType::None &&
                board.at(Utils::applyOffset(square, 2)).getType() == PieceType::None){
                    // king movement
                    Move& move = generatedMoves.emplace_back(square, Utils::applyOffset(square, 2));
                    move.isCastling = true;
                    // rook movement
                    move.auxiliaryMove = new Move();
                    move.auxiliaryMove->startIndex = Utils::applyOffset(square, 3);
                    move.auxiliaryMove->endIndex = Utils::applyOffset(square, 1);
                }
        }
        if (board.getCastleLeft(board.at(square).getColor())){
            if (board.at(Utils::applyOffset(square, -1)).getType() == PieceType::None &&
                board.at(Utils::applyOffset(square, -2)).getType() == PieceType::None &&
                board.at(Utils::applyOffset(square, -3)).getType() == PieceType::None){
                    // king movement
                    Move& move = generatedMoves.emplace_back(square, Utils::applyOffset(square, -2));
                    move.isCastling = true;
                    // rook movement
                    move.auxiliaryMove = new Move();
                    move.auxiliaryMove->startIndex = Utils::applyOffset(square, -4);
                    move.auxiliaryMove->endIndex = Utils::applyOffset(square, -1);
                }
        }
    }
}

void MoveGenerator::generateAllKingKnightMoves(Board& board){
    auto const bitboard = board.getBitboard({PieceType::King, board.getColorToMove()}) | board.getBitboard({PieceType::Knight, board.getColorToMove()});
    const auto squares = bitboard.getPieces8x8();
    const auto numPieces = bitboard.getNumPieces();

    for (int i=0; i<numPieces; i++){
        generateKingKnightMoves(board, Coordinate8x8(squares.at(i)));
    }
}

void MoveGenerator::generatePawnMoves(Board& board, Coordinate8x8 square){
    if (board.at(square).getColor() != board.getColorToMove()) return;
    if (board.at(square).getType() != PieceType::Pawn) return;

    const int8_t baseLine = board.at(square).getColor() == PieceColor::White ? 6 : 1;
    const int8_t targetLine = board.at(square).getColor() == PieceColor::White ? 0 : 7;
    const int8_t direction = board.at(square).getColor() == PieceColor::White ? -1 : 1;
    const int8_t direction10x12(board.at(square).getColor() == PieceColor::White ? -10 : 10);
    const int8_t direction10x12_times_two(direction10x12*2);

    if (!board.isOccupied(Utils::applyOffset(square, direction10x12))){
        // normal move
        Move move;
        move.startIndex = square;
        move.endIndex = Utils::applyOffset(square, direction10x12);
        addPawnMove(move, board);

        // double move
        if (Utils::isOnRow(square, baseLine) && !board.isOccupied(Utils::applyOffset(square, direction10x12_times_two))){
            Move move;
            move.startIndex = square;
            move.endIndex = Utils::applyOffset(square, direction10x12_times_two);
            move.enPassantFile = Utils::getXCoord(Coordinate10x12(move.endIndex));
            move.isDoublePawnMove = true;
            addPawnMove(move, board);
        }
    }

    const int8_t offsetLeft = direction10x12-1;
    const Coordinate10x12 squareLeft = Utils::applyOffset(square, offsetLeft);

    // captures
    if (Utils::isOnBoard(squareLeft) && board.isOccupied(squareLeft) && !board.isFriendly(squareLeft)){
        // left
        Move move;
        move.startIndex = square;
        move.endIndex = squareLeft;
        addPawnMove(move, board);
    }
    
    const int8_t offsetRight = direction10x12+1;
    const Coordinate10x12 squareRight = Utils::applyOffset(square, offsetRight);

    if (Utils::isOnBoard(squareRight) && board.isOccupied(squareRight) && !board.isFriendly(squareRight)){
        // right
        Move move;
        move.startIndex = square;
        move.endIndex = squareRight;
        addPawnMove(move, board);
    }

    // en passant
    if (Utils::isOnBoard(squareLeft) && board.getEnPassantSquare() == squareLeft){
        // left
        Move move;
        move.startIndex = square;
        move.endIndex = squareLeft;
        move.isEnPassant = true;
        addPawnMove(move, board);
    }
    else if (Utils::isOnBoard(squareRight) && board.getEnPassantSquare() == squareRight){
        // right
        Move move;
        move.startIndex = square;
        move.endIndex = squareRight;
        move.isEnPassant = true;
        addPawnMove(move, board);
    }
    

}

void MoveGenerator::generateAllPawnMoves(Board& board){
    auto const& bitboard = board.getBitboard({PieceType::Pawn, board.getColorToMove()});
    const auto squares = bitboard.getPieces8x8();

    for (int i=0; i < bitboard.getNumPieces(); i++)
        generatePawnMoves(board, Coordinate8x8(squares.at(i)));
}

void MoveGenerator::addPawnMove(Move const& move, Board const& board){
    const int8_t targetLine = board.at(move.startIndex).getColor() == PieceColor::White ? 0 : 7;
    

    if (Utils::isOnRow(move.endIndex, targetLine)){
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