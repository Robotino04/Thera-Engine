#include "Thera/MoveGenerator.hpp"
#include "Thera/Board.hpp"
#include "Thera/Utils/ChessTerms.hpp"

namespace Thera{

std::vector<Move> MoveGenerator::generateAllMoves(Board const& board){
    generatedMoves.clear();

    generateAllSlidingMoves(board);
    generateAllKingMoves(board);
    generateAllKnightMoves(board);
    generateAllPawnMoves(board);

    return generatedMoves;
}

void MoveGenerator::generateSlidingMoves(Board const& board, Coordinate square, uint_fast8_t startDirectionIdx, uint_fast8_t endDirectionIdx){
    for (int directionIdx = startDirectionIdx; directionIdx < endDirectionIdx; directionIdx++){
        for (int i=0; i<squaresInDirection[square.getIndex64()][directionIdx].first; i++) {
            Coordinate pos = squaresInDirection[square.getIndex64()][directionIdx].second[i];

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

void MoveGenerator::generateAllSlidingMoves(Board const& board){
    {
        Bitboard bitboard = board.getBitboard({PieceType::Bishop, board.getColorToMove()});

        while (bitboard.hasPieces()){
            generateSlidingMoves(board, Coordinate::fromIndex64(bitboard.getLS1B()), 4, 8);
            bitboard.clearLS1B();
        }
    }

    {
        Bitboard bitboard = board.getBitboard({PieceType::Rook, board.getColorToMove()});

        while (bitboard.hasPieces()){
            generateSlidingMoves(board, Coordinate::fromIndex64(bitboard.getLS1B()), 0, 4);
            bitboard.clearLS1B();
        }
    }
    
    {
        Bitboard bitboard = board.getBitboard({PieceType::Queen, board.getColorToMove()});

        while (bitboard.hasPieces()){
            generateSlidingMoves(board, Coordinate::fromIndex64(bitboard.getLS1B()), 0, 8);
            bitboard.clearLS1B();
        }
    }
}

void MoveGenerator::generateKnightMoves(Board const& board, Coordinate square){
    Bitboard targets = knightSquaresValid.at(square.getIndex64()) & ~board.getPieceBitboardForOneColor(board.getColorToMove());
    while (targets.hasPieces()){
        auto const target = targets.getLS1B();
        generatedMoves.emplace_back(square, Coordinate::fromIndex64(target));
        targets.clearLS1B();
    }
}

void MoveGenerator::generateAllKnightMoves(Board const& board){
    Bitboard bitboard = board.getBitboard({PieceType::Knight, board.getColorToMove()});

    while (bitboard.hasPieces()){
        generateKnightMoves(board, Coordinate::fromIndex64(bitboard.getLS1B()));
        bitboard.clearLS1B();
    }
}

void MoveGenerator::generateKingMoves(Board const& board, Coordinate square){
    Bitboard targets = kingSquaresValid.at(square.getIndex64()) & ~board.getPieceBitboardForOneColor(board.getColorToMove());
    while (targets.hasPieces()){
        auto const target = targets.getLS1B();
        generatedMoves.emplace_back(square, Coordinate::fromIndex64(target));
        targets.clearLS1B();
    }

    // add castling moves
    if (board.at(square).getColor() == PieceColor::White ? board.getCurrentState().canWhiteCastleRight : board.getCurrentState().canBlackCastleRight){
        if (board.at(square + Coordinate(1, 0)).getType() == PieceType::None &&
            board.at(square + Coordinate(2, 0)).getType() == PieceType::None){
                // king movement
                Move& move = generatedMoves.emplace_back(square, square + Coordinate(2, 0));
                move.isCastling = true;
                // rook movement
                move.castlingStart = square + Coordinate(3, 0);
                move.castlingEnd = square + Coordinate(1, 0);
            }
    }
    if (board.at(square).getColor() == PieceColor::White ? board.getCurrentState().canWhiteCastleLeft : board.getCurrentState().canBlackCastleLeft){
        if (board.at(square + Coordinate(-1, 0)).getType() == PieceType::None &&
            board.at(square + Coordinate(-2, 0)).getType() == PieceType::None &&
            board.at(square + Coordinate(-3, 0)).getType() == PieceType::None){
                // king movement
                Move& move = generatedMoves.emplace_back(square, square + Coordinate(-2, 0));
                move.isCastling = true;
                // rook movement
                move.castlingStart = square + Coordinate(-4, 0);
                move.castlingEnd = square + Coordinate(-1, 0);
            }
    }
}

void MoveGenerator::generateAllKingMoves(Board const& board){
    Bitboard bitboard = board.getBitboard({PieceType::King, board.getColorToMove()});

    generateKingMoves(board, Coordinate::fromIndex64(bitboard.getLS1B()));
}

void MoveGenerator::generateAllPawnMoves(Board const& board){
    const Bitboard pawns = board.getBitboard({PieceType::Pawn, board.getColorToMove()});
    const Bitboard occupied = board.getAllPieceBitboard();
    const Bitboard occupied_other_color = board.getPieceBitboardForOneColor(board.getColorToNotMove());

    Bitboard single_pushes, double_pushes, captures_left, captures_right;
    uint8_t direction, dir_left, dir_right;
    if (board.getColorToMove() == PieceColor::White) {
        single_pushes = (pawns >> 8) & ~occupied;
        double_pushes = ((single_pushes & 0x0000ff0000000000) >> 8) & ~occupied;
        captures_left = ((pawns &  0xfefefefefefefefe) >> 9) & occupied_other_color;
        captures_right = ((pawns & 0x7f7f7f7f7f7f7f7f) >> 7) & occupied_other_color;
        direction = 1;
        dir_left = 9;
        dir_right = 7;
    } else {
        single_pushes = (pawns << 8) & ~occupied;
        double_pushes = ((single_pushes & 0x0000000000ff0000) << 8) & ~occupied;
        captures_left = ((pawns & 0xfefefefefefefefe) << 7) & occupied_other_color;
        captures_right = ((pawns & 0x7f7f7f7f7f7f7f7f) << 9) & occupied_other_color;
        direction = -1;
        dir_left = -7;
        dir_right = -9;
    }

    while (single_pushes.hasPieces()) {
        const int target_square = single_pushes.getLS1B();
        const int origin_square = target_square + 8*direction;
        addPawnMovePossiblyPromotion({Coordinate::fromIndex64(origin_square), Coordinate::fromIndex64(target_square)}, board);
        single_pushes.clearLS1B();
    }
    while (double_pushes.hasPieces()) {
        const int target_square = double_pushes.getLS1B();
        const int origin_square = target_square + 16*direction;
        Move& move = generatedMoves.emplace_back(Coordinate::fromIndex64(origin_square), Coordinate::fromIndex64(target_square));
        move.isDoublePawnMove = true;
        double_pushes.clearLS1B();
    }
    while (captures_left.hasPieces()) {
        const int target_square = captures_left.getLS1B();
        const int origin_square = target_square + dir_left;
        addPawnMovePossiblyPromotion({Coordinate::fromIndex64(origin_square), Coordinate::fromIndex64(target_square)}, board);
        captures_left.clearLS1B();
    }
    while (captures_right.hasPieces()) {
        const int target_square = captures_right.getLS1B();
        const int origin_square = target_square + dir_right;
        addPawnMovePossiblyPromotion({Coordinate::fromIndex64(origin_square), Coordinate::fromIndex64(target_square)}, board);
        captures_right.clearLS1B();
    }

    // en passant
    if (board.hasEnPassant()){
        auto const epCaptureSquare = board.getEnPassantSquareToCapture();
        if (epCaptureSquare.x != 0 && pawns.isOccupied(epCaptureSquare - Coordinate::fromIndex64(1))){
            // to the left
            Move& move = generatedMoves.emplace_back(epCaptureSquare - Coordinate::fromIndex64(1), board.getEnPassantSquareForFEN());
            move.isEnPassant = true;
        }
        if (epCaptureSquare.x != 7 && pawns.isOccupied(epCaptureSquare + Coordinate::fromIndex64(1))){
            // to the right
            Move& move = generatedMoves.emplace_back(epCaptureSquare + Coordinate::fromIndex64(1), board.getEnPassantSquareForFEN());
            move.isEnPassant = true;
        }
    }
}

void MoveGenerator::addPawnMovePossiblyPromotion(Move const& move, Board const& board){
    const uint8_t targetLine = board.at(move.startIndex).getColor() == PieceColor::White ? 0 : 7;

    if (move.endIndex.y == targetLine){
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