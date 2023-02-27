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

    switch (board.at(square).getType()){
        case PieceType::Bishop:
        case PieceType::Rook:
        case PieceType::Queen:
            generateSlidingMoves(board, square);
            break;
        
        case PieceType::King:
        case PieceType::Knight:
            generateKingKnightMoves(board, square);
            break;
        
        case PieceType::Pawn:
            generatePawnMoves(board, square);
            break;
    }

    return generatedMoves;
}

void MoveGenerator::generateSlidingMoves(Board& board, Coordinate8x8 square){
    int8_t startDirectionIdx = 0;
    int8_t endDirectionIdx = MoveGenerator::slidingPieceOffsets.size();
    if (board.at(square).getType() == PieceType::Rook) endDirectionIdx = 4;
    else if (board.at(square).getType() == PieceType::Bishop) startDirectionIdx = 4;
    else if (board.at(square).getType() == PieceType::Queen);
    else return;

    for (int directionIdx = startDirectionIdx; directionIdx < endDirectionIdx; directionIdx++){

        for (int i=0; i<squaresInDirection[square.pos][directionIdx].first; i++) {
            Coordinate8x8 pos = squaresInDirection[square.pos][directionIdx].second[i];

            if (board.at(pos).getType() == PieceType::None)
                // normal move
                generatedMoves.emplace_back(square, pos);
            else if (board.at(pos).getColor() == board.getColorToNotMove()){
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
    auto const bitboard = board.getBitboard({PieceType::Bishop, board.getColorToMove()})
                        | board.getBitboard({PieceType::Rook, board.getColorToMove()})
                        | board.getBitboard({PieceType::Queen, board.getColorToMove()});
    const auto squares = bitboard.getPieces();
    const auto numPieces = bitboard.getNumPieces();

    for (int i=0; i<numPieces; i++){
        generateSlidingMoves(board, Coordinate8x8(squares.at(i)));
    }
}

void MoveGenerator::generateKingKnightMoves(Board& board, Coordinate8x8 square){
    int8_t startDirectionIdx = 0;
    int8_t endDirectionIdx = MoveGenerator::kingKnightOffsets.size();
    if (board.at(square).getType() == PieceType::King) endDirectionIdx = 8;
    else if (board.at(square).getType() == PieceType::Knight) startDirectionIdx = 8;
    else return;
    
    for (int directionIdx = startDirectionIdx; directionIdx < endDirectionIdx; directionIdx++){
        auto const& target = kingKnightSquaresValid[square.pos][directionIdx];

        if (target.first) {
            if (board.at(target.second).getType() == PieceType::None)
                // normal move
                generatedMoves.emplace_back(square, target.second);
            else if (board.at(target.second).getColor() == board.getColorToNotMove()){
                // capture
                generatedMoves.emplace_back(square, target.second);
            }
        }
    }
    if (board.at(square).getType() == PieceType::King){
        // add castling moves
        if (board.at(square).getColor() == PieceColor::White ? board.getCurrentState().canWhiteCastleRight : board.getCurrentState().canBlackCastleRight){
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
        if (board.at(square).getColor() == PieceColor::White ? board.getCurrentState().canWhiteCastleLeft : board.getCurrentState().canBlackCastleLeft){
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
    const auto squares = bitboard.getPieces();
    const auto numPieces = bitboard.getNumPieces();

    for (int i=0; i<numPieces; i++){
        generateKingKnightMoves(board, Coordinate8x8(squares.at(i)));
    }
}

void MoveGenerator::generatePawnMoves(Board& board, Coordinate8x8 square){
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
            move.enPassantFile = Utils::getXCoord(move.endIndex);
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
    const auto squares = bitboard.getPieces();

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