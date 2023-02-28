#include "Thera/MoveGenerator.hpp"
#include "Thera/Board.hpp"
#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Utils/Coordinates.hpp"

namespace Thera{

std::vector<Move> MoveGenerator::generateAllMoves(Board& board){
    generatedMoves.clear();

    generateAllSlidingMoves(board);
    generateAllKingMoves(board);
    generateAllKnightMoves(board);
    generateAllPawnMoves(board);

    return generatedMoves;
}

std::vector<Move> MoveGenerator::generateMoves(Board& board, Coordinate8x8 square){
    if (board.at(square).getColor() != board.getColorToMove()) return {};
    generatedMoves.clear();

    switch (board.at(square).getType()){
        case PieceType::Bishop:
            generateSlidingMoves(board, square, 4, 8);
            break;
        
        case PieceType::Rook:
            generateSlidingMoves(board, square, 0, 4);
            break;
        
        case PieceType::Queen:
            generateSlidingMoves(board, square, 0, 8);
            break;
        
        case PieceType::King:
            generateKingMoves(board, square);
            break;

        case PieceType::Knight:
            generateKnightMoves(board, square);
            break;
        
        case PieceType::Pawn:
            generatePawnMoves(board, square);
            break;
    }

    return generatedMoves;
}

void MoveGenerator::generateSlidingMoves(Board& board, Coordinate8x8 square, uint_fast8_t startDirectionIdx, uint_fast8_t endDirectionIdx){
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
    {
        auto const bitboard = board.getBitboard({PieceType::Bishop, board.getColorToMove()});
        const auto squares = bitboard.getPieces();
        const auto numPieces = bitboard.getNumPieces();

        for (int i=0; i<numPieces; i++)
            generateSlidingMoves(board, squares.at(i), 4, 8);
    }

    {
        auto const bitboard = board.getBitboard({PieceType::Rook, board.getColorToMove()});
        const auto squares = bitboard.getPieces();
        const auto numPieces = bitboard.getNumPieces();

        for (int i=0; i<numPieces; i++)
            generateSlidingMoves(board, squares.at(i), 0, 4);
    }
    
    {
        auto const bitboard = board.getBitboard({PieceType::Queen, board.getColorToMove()});
        const auto squares = bitboard.getPieces();
        const auto numPieces = bitboard.getNumPieces();

        for (int i=0; i<numPieces; i++)
            generateSlidingMoves(board, squares.at(i), 0, 8);
    }
}

void MoveGenerator::generateKnightMoves(Board& board, Coordinate8x8 square){
    for (int directionIdx = 0; directionIdx < 8; directionIdx++){
        auto const& target = knightSquaresValid[square.pos][directionIdx];

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
}

void MoveGenerator::generateAllKnightMoves(Board& board){
    auto const bitboard = board.getBitboard({PieceType::Knight, board.getColorToMove()});
    const auto squares = bitboard.getPieces();
    const auto numPieces = bitboard.getNumPieces();

    for (int i=0; i<numPieces; i++){
        generateKnightMoves(board, squares.at(i));
    }
}

void MoveGenerator::generateKingMoves(Board& board, Coordinate8x8 square){
    for (int directionIdx = 0; directionIdx < 8; directionIdx++){
        auto const& target = kingSquaresValid[square.pos][directionIdx];

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

void MoveGenerator::generateAllKingMoves(Board& board){
    auto const bitboard = board.getBitboard({PieceType::King, board.getColorToMove()});
    const auto squares = bitboard.getPieces();
    const auto numPieces = bitboard.getNumPieces();

    for (int i=0; i<numPieces; i++){
        generateKingMoves(board, squares.at(i));
    }
}

void MoveGenerator::generatePawnMoves(Board& board, Coordinate8x8 square){
    const uint8_t baseLine = board.at(square).getColor() == PieceColor::White ? 6 : 1;
    const uint8_t targetLine = board.at(square).getColor() == PieceColor::White ? 0 : 7;
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
    const PossiblyOffTheBoardCoordinate squareLeft = Utils::applyOffset(square, offsetLeft);

    // captures
    if (Utils::isOnBoard(squareLeft) && board.isOccupied(squareLeft) && !board.isFriendly(squareLeft)){
        // left
        Move move;
        move.startIndex = square;
        move.endIndex = squareLeft;
        addPawnMove(move, board);
    }
    
    const int8_t offsetRight = direction10x12+1;
    const PossiblyOffTheBoardCoordinate squareRight = Utils::applyOffset(square, offsetRight);

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
    const uint8_t targetLine = board.at(move.startIndex).getColor() == PieceColor::White ? 0 : 7;
    

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