#include "Thera/MoveGenerator.hpp"
#include "Thera/Board.hpp"
#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Utils/BuildType.hpp"

#include <tuple>

namespace Thera{

std::vector<Move> MoveGenerator::generateAllMoves(Board const& board){
    generatedMoves.clear();

    generateAttackData(board);

    generateAllKingMoves(board);
    generateAllSlidingMoves(board);
    generateAllKnightMoves(board);
    generateAllPawnMoves(board);

    return generatedMoves;
}

Bitboard slidingAttacks (Bitboard sliders, Bitboard empty, int dir8) {
   Bitboard flood = sliders;
   int r = MoveGenerator::slidingPieceShiftAmounts[dir8]; // {+-1,7,8,9}
   empty &= MoveGenerator::slidingPieceAvoidWrapping[dir8];
   flood |= sliders = (sliders << r) & empty;
   flood |= sliders = (sliders << r) & empty;
   flood |= sliders = (sliders << r) & empty;
   flood |= sliders = (sliders << r) & empty;
   flood |= sliders = (sliders << r) & empty;
   flood |=           (sliders << r) & empty;
   return             (flood << r)   & MoveGenerator::slidingPieceAvoidWrapping[dir8];
}

template <int startDirectionIndex, int endDirectionIndex>
Bitboard allDirectionSlidingAttacks(Bitboard occupied, Bitboard square){
    static_assert(startDirectionIndex < endDirectionIndex, "Infinite loop prevented");
    Bitboard targetSquares;

    for (int directionIdx = startDirectionIndex; directionIdx < endDirectionIndex; directionIdx++){
        targetSquares |= slidingAttacks(square, ~occupied, directionIdx);
    }
    targetSquares &= ~square;

    return targetSquares;
}

template <int startDirectionIndex, int endDirectionIndex>
Bitboard xRayAttacks(Bitboard occupiedSquares, Bitboard blockers, Bitboard square) {
   Bitboard attacks = allDirectionSlidingAttacks<startDirectionIndex, endDirectionIndex>(occupiedSquares, square);
   blockers &= attacks;
   return attacks ^ allDirectionSlidingAttacks<startDirectionIndex, endDirectionIndex>(occupiedSquares ^ blockers, square);
}

Bitboard generatePins(Board const& board) {
    const auto squareOfKing = board.getBitboard({PieceType::King, board.getColorToMove()});
    const uint8_t squareOfKingIndex = squareOfKing.getLS1B();

    const auto oppositeQueens = board.getBitboard({PieceType::Queen, board.getColorToNotMove()});
    const auto oppositeRooks = board.getBitboard({PieceType::Rook, board.getColorToNotMove()}) | oppositeQueens;
    const auto oppositeBishops = board.getBitboard({PieceType::Bishop, board.getColorToNotMove()}) | oppositeQueens;
    const auto occupiedSquares = board.getAllPieceBitboard();
    const auto ownPieces = board.getPieceBitboardForOneColor(board.getColorToMove());


    Bitboard pinned = 0;
    Bitboard pinner = xRayAttacks<0, 4>(occupiedSquares, ownPieces, squareOfKing) & oppositeRooks;
    while (pinner.hasPieces()) {
        uint8_t sq = pinner.getLS1B();
        pinned |= MoveGenerator::obstructedLUT.at(sq).at(squareOfKingIndex) & ownPieces;
        pinner.clearLS1B();
    }
    pinner = xRayAttacks<4, 8>(occupiedSquares, ownPieces, squareOfKing) & oppositeBishops;
    while (pinner.hasPieces()) {
        uint8_t sq = pinner.getLS1B();
        pinned |= MoveGenerator::obstructedLUT.at(sq).at(squareOfKingIndex) & ownPieces;
        pinner.clearLS1B();
    }
    return pinned;
}

void MoveGenerator::generateAttackData(Board const& board){
    attackedSquares = 0;
    pinnedPieces = generatePins(board);

    // sliding pieces
    // rook and queen

    Bitboard bitboard = board.getBitboard({PieceType::Rook, board.getColorToNotMove()}) | board.getBitboard({PieceType::Queen, board.getColorToNotMove()});

    while (bitboard.hasPieces()){
        Bitboard targetSquares = allDirectionSlidingAttacks<0, 4>(board.getAllPieceBitboard(), uint64_t(1) << bitboard.getLS1B());

        attackedSquares |= targetSquares;
        attackedSquaresRook |= targetSquares;

        bitboard.clearLS1B();
    }

    // bishop and queen
    bitboard = board.getBitboard({PieceType::Bishop, board.getColorToNotMove()}) | board.getBitboard({PieceType::Queen, board.getColorToNotMove()});

    while (bitboard.hasPieces()){
        Bitboard targetSquares = allDirectionSlidingAttacks<4, 8>(board.getAllPieceBitboard(), uint64_t(1) << bitboard.getLS1B());

        attackedSquares |= targetSquares;
        attackedSquaresRook |= targetSquares;

        bitboard.clearLS1B();
    }


    // knight moves
    bitboard = board.getBitboard({PieceType::Knight, board.getColorToNotMove()});

    while (bitboard.hasPieces()){
        attackedSquares |= knightSquaresValid.at(bitboard.getLS1B()) & ~board.getPieceBitboardForOneColor(board.getColorToNotMove());
        bitboard.clearLS1B();
    }

    // king moves
    bitboard = board.getBitboard({PieceType::King, board.getColorToNotMove()});
    if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
        if (!bitboard.hasPieces()) throw std::runtime_error("No king for the opposite color found.");
    }

    attackedSquares |= kingSquaresValid.at(bitboard.getLS1B());
    
    // pawn moves
    const Bitboard pawns = board.getBitboard({PieceType::Pawn, board.getColorToNotMove()});
    const int dir = board.getCurrentState().isWhiteToMove ? DirectionIndex64::S : DirectionIndex64::N;

    attackedSquares |= (pawns & 0xfefefefefefefefe) << (dir + DirectionIndex64::W);
    attackedSquares |= (pawns & 0x7f7f7f7f7f7f7f7f) << (dir + DirectionIndex64::E);
}

void MoveGenerator::generateAllSlidingMoves(Board const& board){
    {
        Bitboard bitboard = board.getBitboard({PieceType::Bishop, board.getColorToMove()}) | board.getBitboard({PieceType::Queen, board.getColorToMove()});

        while (bitboard.hasPieces()){
            Bitboard targetSquares = allDirectionSlidingAttacks<4, 8>(board.getAllPieceBitboard(), uint64_t(1) << bitboard.getLS1B());
            targetSquares &= ~board.getPieceBitboardForOneColor(board.getColorToMove());

            const Coordinate origin = Coordinate::fromIndex64(bitboard.getLS1B());
            while (targetSquares.hasPieces()){
                generatedMoves.emplace_back(origin, Coordinate::fromIndex64(targetSquares.getLS1B()));
                targetSquares.clearLS1B();
            }
            
            bitboard.clearLS1B();
        }
    }

    {
        Bitboard bitboard = board.getBitboard({PieceType::Rook, board.getColorToMove()}) | board.getBitboard({PieceType::Queen, board.getColorToMove()});

        while (bitboard.hasPieces()){
            Bitboard targetSquares = allDirectionSlidingAttacks<0, 4>(board.getAllPieceBitboard(), uint64_t(1) << bitboard.getLS1B());
            targetSquares &= ~board.getPieceBitboardForOneColor(board.getColorToMove());

            const Coordinate origin = Coordinate::fromIndex64(bitboard.getLS1B());
            while (targetSquares.hasPieces()){
                generatedMoves.emplace_back(origin, Coordinate::fromIndex64(targetSquares.getLS1B()));
                targetSquares.clearLS1B();
            }
            
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

void MoveGenerator::generateAllKingMoves(Board const& board){
    Bitboard bitboard = board.getBitboard({PieceType::King, board.getColorToMove()});

    // TODO: maybe remove this since every position should have a king. Only there for debugging
    if (!bitboard.hasPieces()) return;
    Coordinate square = Coordinate::fromIndex64(bitboard.getLS1B());

    Bitboard targets = kingSquaresValid.at(bitboard.getLS1B()) & ~(board.getPieceBitboardForOneColor(board.getColorToMove())  | attackedSquares);

    while (targets.hasPieces()){
        auto const target = targets.getLS1B();
        generatedMoves.emplace_back(square, Coordinate::fromIndex64(target));
        targets.clearLS1B();
    }

    const int shiftAmount = (board.getCurrentState().isWhiteToMove ? 0 : DirectionIndex64::N*7);
    const Bitboard leftCastlingMap = Bitboard(0x00000000000000000e) << shiftAmount;
    const Bitboard rightCastlingMap = Bitboard(0x000000000000000060) << shiftAmount;
    const Bitboard leftCastlingMapKing = Bitboard(0x00000000000000001c) << shiftAmount;
    const Bitboard rightCastlingMapKing = Bitboard(0x000000000000000070) << shiftAmount;

    // add castling moves
    if (board.getCurrentState().isWhiteToMove ? board.getCurrentState().canWhiteCastleRight : board.getCurrentState().canBlackCastleRight){
        if (!uint64_t(rightCastlingMap & board.getAllPieceBitboard()) && !uint64_t(rightCastlingMapKing & attackedSquares)){
                // king movement
                Move& move = generatedMoves.emplace_back(square, square + Direction::E*2);
                move.isCastling = true;
                // rook movement
                move.castlingStart = square + Direction::E*3;
                move.castlingEnd = square + Direction::E;
            }
    }
    if (board.getCurrentState().isWhiteToMove ? board.getCurrentState().canWhiteCastleLeft : board.getCurrentState().canBlackCastleLeft){
        if (!uint64_t(leftCastlingMap & board.getAllPieceBitboard()) && !uint64_t(leftCastlingMapKing & attackedSquares)){
                // king movement
                Move& move = generatedMoves.emplace_back(square, square + Direction::W*2);
                move.isCastling = true;
                // rook movement
                move.castlingStart = square + Direction::W*4;
                move.castlingEnd = square + Direction::W;
            }
    }
}

void MoveGenerator::generateAllPawnMoves(Board const& board){
    const Bitboard pawns = board.getBitboard({PieceType::Pawn, board.getColorToMove()});
    const Bitboard occupied = board.getAllPieceBitboard();
    const Bitboard occupied_other_color = board.getPieceBitboardForOneColor(board.getColorToNotMove());

    Bitboard single_pushes, double_pushes, captures_left, captures_right;
    // directions to get from target square to origin square
    uint8_t reverseDirection, reverseDirectionLeft, reverseDirectionRight;
    if (board.getCurrentState().isWhiteToMove) {
        single_pushes = (pawns << DirectionIndex64::N) & ~occupied;
        double_pushes = ((single_pushes & 0x0000000000ff0000) << DirectionIndex64::N) & ~occupied;
        captures_left = ((pawns &  0xfefefefefefefefe) << DirectionIndex64::NW) & occupied_other_color;
        captures_right = ((pawns & 0x7f7f7f7f7f7f7f7f) << DirectionIndex64::NE) & occupied_other_color;
        reverseDirection = DirectionIndex64::S;
        reverseDirectionLeft = DirectionIndex64::SE;
        reverseDirectionRight = DirectionIndex64::SW;
    } else {
        single_pushes = (pawns << -8) & ~occupied;
        double_pushes = ((single_pushes & 0x0000ff0000000000) << DirectionIndex64::S) & ~occupied;
        captures_left = ((pawns & 0xfefefefefefefefe) << DirectionIndex64::SW) & occupied_other_color;
        captures_right = ((pawns & 0x7f7f7f7f7f7f7f7f) << DirectionIndex64::SE) & occupied_other_color;
        reverseDirection = DirectionIndex64::N;
        reverseDirectionLeft = DirectionIndex64::NE;
        reverseDirectionRight = DirectionIndex64::NW;
    }

    while (single_pushes.hasPieces()) {
        const int target_square = single_pushes.getLS1B();
        const int origin_square = target_square + reverseDirection;
        addPawnMovePossiblyPromotion({Coordinate::fromIndex64(origin_square), Coordinate::fromIndex64(target_square)}, board);
        single_pushes.clearLS1B();
    }
    while (double_pushes.hasPieces()) {
        const int target_square = double_pushes.getLS1B();
        const int origin_square = target_square + reverseDirection*2;
        Move& move = generatedMoves.emplace_back(Coordinate::fromIndex64(origin_square), Coordinate::fromIndex64(target_square));
        move.isDoublePawnMove = true;
        double_pushes.clearLS1B();
    }
    while (captures_left.hasPieces()) {
        const int target_square = captures_left.getLS1B();
        const int origin_square = target_square + reverseDirectionLeft;
        addPawnMovePossiblyPromotion({Coordinate::fromIndex64(origin_square), Coordinate::fromIndex64(target_square)}, board);
        captures_left.clearLS1B();
    }
    while (captures_right.hasPieces()) {
        const int target_square = captures_right.getLS1B();
        const int origin_square = target_square + reverseDirectionRight;
        addPawnMovePossiblyPromotion({Coordinate::fromIndex64(origin_square), Coordinate::fromIndex64(target_square)}, board);
        captures_right.clearLS1B();
    }

    // en passant
    if (board.hasEnPassant()){
        auto const epCaptureSquare = board.getEnPassantSquareToCapture();
        if (epCaptureSquare.x != 0 && pawns.isOccupied(epCaptureSquare + Direction::W)){
            // to the left
            Move& move = generatedMoves.emplace_back(epCaptureSquare + Direction::W, board.getEnPassantSquareForFEN());
            move.isEnPassant = true;
        }
        if (epCaptureSquare.x != 7 && pawns.isOccupied(epCaptureSquare + Direction::E)){
            // to the right
            Move& move = generatedMoves.emplace_back(epCaptureSquare + Direction::E, board.getEnPassantSquareForFEN());
            move.isEnPassant = true;
        }
    }
}

void MoveGenerator::addPawnMovePossiblyPromotion(Move const& move, Board const& board){
    const uint8_t targetLine = board.getCurrentState().isWhiteToMove ? 7 : 0;

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