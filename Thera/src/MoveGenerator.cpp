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


void MoveGenerator::generatePins(Board const& board) {
    const auto squareOfKing = board.getBitboard({PieceType::King, board.getColorToMove()});
    const uint8_t squareOfKingIndex = squareOfKing.getLS1B();

    const auto oppositeQueens = board.getBitboard({PieceType::Queen, board.getColorToNotMove()});
    const auto oppositeRooks = board.getBitboard({PieceType::Rook, board.getColorToNotMove()}) | oppositeQueens;
    const auto oppositeBishops = board.getBitboard({PieceType::Bishop, board.getColorToNotMove()}) | oppositeQueens;
    const auto occupiedSquares = board.getAllPieceBitboard();
    const auto ownPieces = board.getPieceBitboardForOneColor(board.getColorToMove());

    const auto helper = [&]<int idx>(Bitboard oppositePieces){
        static_assert(idx >= 0 && idx < 8, "Index has to be between 0 and 7");
        Bitboard pinner = xRayAttacks<idx, idx+1>(occupiedSquares, ownPieces, squareOfKing) & oppositePieces;
        if (pinner.hasPieces()) {
            Bitboard result = MoveGenerator::obstructedLUT.at(pinner.getLS1B()).at(squareOfKingIndex) & ownPieces;
            pinnedPieces |= result;
            switch(idx){
                case 0: pinDirection.at(result.getLS1B()) = {0, 3}; break;
                case 1: pinDirection.at(result.getLS1B()) = {1, 2}; break;
                case 2: pinDirection.at(result.getLS1B()) = {2, 1}; break;
                case 3: pinDirection.at(result.getLS1B()) = {3, 0}; break;
                case 4: pinDirection.at(result.getLS1B()) = {4, 7}; break;
                case 5: pinDirection.at(result.getLS1B()) = {5, 6}; break;
                case 6: pinDirection.at(result.getLS1B()) = {6, 5}; break;
                case 7: pinDirection.at(result.getLS1B()) = {7, 4}; break;
    }
    }
    };

    pinDirection.fill({0,0});
    pinnedPieces = 0;
    // stupid syntax but helper<0>(oppositeRooks) could mean that "helper" is templated
    helper.operator()<0>(oppositeRooks);
    helper.operator()<1>(oppositeRooks);
    helper.operator()<2>(oppositeRooks);
    helper.operator()<3>(oppositeRooks);

    helper.operator()<4>(oppositeBishops);
    helper.operator()<5>(oppositeBishops);
    helper.operator()<6>(oppositeBishops);
    helper.operator()<7>(oppositeBishops);
}

void MoveGenerator::generateAttackData(Board const& board){
    attackedSquares = 0;
    generatePins(board);

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
    auto const helper = [&]<int startIdx, int endIdx>(PieceType pieceType){
        const Bitboard bitboard = board.getBitboard({pieceType, board.getColorToMove()}) | board.getBitboard({PieceType::Queen, board.getColorToMove()});
        Bitboard unpinnedBitboard = bitboard & ~pinnedPieces;

        while (unpinnedBitboard.hasPieces()){
            Bitboard targetSquares = allDirectionSlidingAttacks<startIdx, endIdx>(board.getAllPieceBitboard(), uint64_t(1) << unpinnedBitboard.getLS1B());
            targetSquares &= ~board.getPieceBitboardForOneColor(board.getColorToMove());

            const Coordinate origin = Coordinate::fromIndex64(unpinnedBitboard.getLS1B());
            while (targetSquares.hasPieces()){
                generatedMoves.emplace_back(origin, Coordinate::fromIndex64(targetSquares.getLS1B()));
                targetSquares.clearLS1B();
            }
            
            unpinnedBitboard.clearLS1B();
        }

        Bitboard pinnedBitboard = bitboard & pinnedPieces;
        while (pinnedBitboard.hasPieces()){
            // avoids duplication for queens
            if (pinDirection.at(pinnedBitboard.getLS1B()).dir1 < startIdx || pinDirection.at(pinnedBitboard.getLS1B()).dir1 >= endIdx){
                pinnedBitboard.clearLS1B();
                continue;
    }

            Bitboard originBB = uint64_t(1) << pinnedBitboard.getLS1B();
            Bitboard targetSquares = slidingAttacks(originBB, ~board.getAllPieceBitboard(), pinDirection.at(pinnedBitboard.getLS1B()).dir1);
            targetSquares |=         slidingAttacks(originBB, ~board.getAllPieceBitboard(), pinDirection.at(pinnedBitboard.getLS1B()).dir2);

            targetSquares &= ~board.getPieceBitboardForOneColor(board.getColorToMove());

            const Coordinate origin = Coordinate::fromIndex64(pinnedBitboard.getLS1B());
            while (targetSquares.hasPieces()){
                generatedMoves.emplace_back(origin, Coordinate::fromIndex64(targetSquares.getLS1B()));
                targetSquares.clearLS1B();
            }
            
            pinnedBitboard.clearLS1B();
        }
    };
    helper.operator()<0, 4>(PieceType::Rook);
    helper.operator()<4, 8>(PieceType::Bishop);
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
    Bitboard bitboard = board.getBitboard({PieceType::Knight, board.getColorToMove()}) & ~pinnedPieces;

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
    const Bitboard unpinnedPawns = board.getBitboard({PieceType::Pawn, board.getColorToMove()}) & ~pinnedPieces;
    const Bitboard occupied = board.getAllPieceBitboard();
    const Bitboard occupied_other_color = board.getPieceBitboardForOneColor(board.getColorToNotMove());

    Bitboard unpinnedPawnsLeft = unpinnedPawns, unpinnedPawnsRight = unpinnedPawns; 

    Bitboard pawns = unpinnedPawns;
    Bitboard pinnedPawns = board.getBitboard({PieceType::Pawn, board.getColorToMove()}) & pinnedPieces;
    while (pinnedPawns.hasPieces()){
        if (pinDirection.at(pinnedPawns.getLS1B()).dir1 == 0 || pinDirection.at(pinnedPawns.getLS1B()).dir2 == 0)
            pawns |= uint64_t(1) << pinnedPawns.getLS1B();
        pinnedPawns.clearLS1B();
    }

    const int8_t mainDirection = board.getCurrentState().isWhiteToMove ? DirectionIndex64::N : DirectionIndex64::S;
    const Bitboard doublePushMask = board.getCurrentState().isWhiteToMove ? 0x0000000000ff0000 : 0x0000ff0000000000;

    const int8_t reverseDirection = -mainDirection;
    const int8_t reverseDirectionLeft = reverseDirection + DirectionIndex64::E;
    const int8_t reverseDirectionRight = reverseDirection + DirectionIndex64::W;

    pinnedPawns = board.getBitboard({PieceType::Pawn, board.getColorToMove()}) & pinnedPieces;
    while (pinnedPawns.hasPieces()){
        if (pinDirection.at(pinnedPawns.getLS1B()).dir1 == 4 || pinDirection.at(pinnedPawns.getLS1B()).dir1 == 6)
            unpinnedPawnsLeft |= uint64_t(1) << pinnedPawns.getLS1B();
        else if (pinDirection.at(pinnedPawns.getLS1B()).dir1 == 5 || pinDirection.at(pinnedPawns.getLS1B()).dir1 == 7)
            unpinnedPawnsRight |= uint64_t(1) << pinnedPawns.getLS1B();
        pinnedPawns.clearLS1B();
    }

    Bitboard single_pushes = (pawns << mainDirection) & ~occupied;
    Bitboard double_pushes = ((single_pushes & doublePushMask) << mainDirection) & ~occupied;
    Bitboard captures_left = ((unpinnedPawnsLeft & 0xfefefefefefefefe) << (mainDirection + DirectionIndex64::W)) & occupied_other_color;
    Bitboard captures_right = ((unpinnedPawnsRight & 0x7f7f7f7f7f7f7f7f) << (mainDirection + DirectionIndex64::E)) & occupied_other_color;

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