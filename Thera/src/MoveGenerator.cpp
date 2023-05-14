#include "Thera/MoveGenerator.hpp"
#include "Thera/Board.hpp"
#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Utils/BuildType.hpp"

#include <tuple>

namespace Thera{

std::vector<Move> MoveGenerator::generateAllMoves(Board const& board){
    generatedMoves.clear();

    generateAttackData(board);

    generateAllKingMoves(board, Utils::binaryOnes<uint64_t>(64));
    generateAllSlidingMoves(board, possibleTargets);
    generateAllKnightMoves(board, possibleTargets);
    generateAllPawnMoves(board, possibleTargets);

    return generatedMoves;
}

// Kogge-Stone Algorithm
Bitboard occludedFill (Bitboard gen, Bitboard pro, int dir8){
   int r = MoveGenerator::slidingPieceShiftAmounts[dir8];
   pro &= MoveGenerator::slidingPieceAvoidWrapping[dir8];
   gen |= pro & gen.rotateLeft(r);
   pro &=       pro.rotateLeft(r);
   gen |= pro & gen.rotateLeft(2*r);
   pro &=       pro.rotateLeft(2*r);
   gen |= pro & gen.rotateLeft(4*r);
   return gen;
}

Bitboard shiftOne (Bitboard b, int dir8){
   int r = MoveGenerator::slidingPieceShiftAmounts[dir8];
   return b.rotateLeft(r) & MoveGenerator::slidingPieceAvoidWrapping[dir8];
}

Bitboard slidingAttacks (Bitboard sliders, Bitboard empty, int dir8){
   Bitboard fill = occludedFill(sliders, empty, dir8);
   return shiftOne(fill, dir8);
}

// Dumb7fill
// Bitboard slidingAttacks (Bitboard sliders, Bitboard empty, int dir8) {
//    Bitboard flood = sliders;
//    int r = MoveGenerator::slidingPieceShiftAmounts[dir8]; // {+-1,7,8,9}
//    empty &= MoveGenerator::slidingPieceAvoidWrapping[dir8];
//    flood |= sliders = (sliders << r) & empty;
//    flood |= sliders = (sliders << r) & empty;
//    flood |= sliders = (sliders << r) & empty;
//    flood |= sliders = (sliders << r) & empty;
//    flood |= sliders = (sliders << r) & empty;
//    flood |=           (sliders << r) & empty;
//    return             (flood << r)   & MoveGenerator::slidingPieceAvoidWrapping[dir8];
// }

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

bool MoveGenerator::isCheck(Board const& board) const{
    return (attackedSquares & board.getBitboard({PieceType::King, board.getColorToMove()})).hasPieces();
}

void MoveGenerator::generatePins(Board const& board) {
    const auto squareOfKing = board.getBitboard({PieceType::King, board.getColorToMove()});
    const uint8_t squareOfKingIndex = squareOfKing.getLS1B();

    const auto oppositeQueens = board.getBitboard({PieceType::Queen, board.getColorToNotMove()});
    const auto oppositeRooks = board.getBitboard({PieceType::Rook, board.getColorToNotMove()}) | oppositeQueens;
    const auto oppositeBishops = board.getBitboard({PieceType::Bishop, board.getColorToNotMove()}) | oppositeQueens;
    const auto occupiedSquares = board.getAllPieceBitboard();
    const auto ownPieces = board.getAllPieceBitboard();

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
    possibleTargets = 0;
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
    squaresAttackedBySquare.fill(Bitboard(0));
    squaresAttackingSquare.fill(Bitboard(0));
    generatePins(board);

    // sliding pieces
    // rook and queen
    Bitboard oppositeSlidingPiecesAndPawns;
    const Bitboard slidingBlockers = board.getAllPieceBitboard() ^ board.getBitboard({PieceType::King, board.getColorToMove()});

    Bitboard bitboard = board.getBitboard({PieceType::Rook, board.getColorToNotMove()}) | board.getBitboard({PieceType::Queen, board.getColorToNotMove()});
    oppositeSlidingPiecesAndPawns |= bitboard;

    while (bitboard.hasPieces()){
        auto const origin_square = bitboard.getLS1B();
        Bitboard targetSquares = allDirectionSlidingAttacks<0, 4>(slidingBlockers, Utils::binaryOneAt<uint64_t>(origin_square));

        attackedSquares |= targetSquares;
        squaresAttackedBySquare[origin_square] |= targetSquares;
        while (targetSquares.hasPieces()){
            squaresAttackingSquare.at(targetSquares.getLS1B()).setBit(origin_square);

            targetSquares.clearLS1B();
        }

        bitboard.clearLS1B();
    }

    // bishop and queen
    bitboard = board.getBitboard({PieceType::Bishop, board.getColorToNotMove()}) | board.getBitboard({PieceType::Queen, board.getColorToNotMove()});
    oppositeSlidingPiecesAndPawns |= bitboard;

    while (bitboard.hasPieces()){
        auto const origin_square = bitboard.getLS1B();
        Bitboard targetSquares = allDirectionSlidingAttacks<4, 8>(slidingBlockers, Utils::binaryOneAt<uint64_t>(origin_square));

        attackedSquares |= targetSquares;
        squaresAttackedBySquare[origin_square] |= targetSquares;
        while (targetSquares.hasPieces()){
            squaresAttackingSquare.at(targetSquares.getLS1B()).setBit(origin_square);

            targetSquares.clearLS1B();
        }

        bitboard.clearLS1B();
    }


    // knight moves
    bitboard = board.getBitboard({PieceType::Knight, board.getColorToNotMove()});

    while (bitboard.hasPieces()){
        auto const knightSquare = bitboard.getLS1B();
        Bitboard targetSquares = knightSquaresValid.at(knightSquare);
        attackedSquares |= targetSquares;
        squaresAttackedBySquare[knightSquare] |= targetSquares;
        while (targetSquares.hasPieces()){
            squaresAttackingSquare.at(targetSquares.getLS1B()).setBit(knightSquare);

            targetSquares.clearLS1B();
        }
        bitboard.clearLS1B();
    }

    // king moves
    bitboard = board.getBitboard({PieceType::King, board.getColorToNotMove()});
    if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
        if (!bitboard.hasPieces()) throw std::runtime_error("No king for the opposite color found.");
    }

    const auto kingSquare = bitboard.getLS1B();
    bitboard = kingSquaresValid.at(kingSquare);
    attackedSquares |= bitboard;
    squaresAttackedBySquare[kingSquare] |= bitboard;
    while(bitboard.hasPieces()){
        squaresAttackingSquare.at(bitboard.getLS1B()).setBit(kingSquare);
        bitboard.clearLS1B();
    }
    
    // pawn moves
    {
        const Bitboard pawns = board.getBitboard({PieceType::Pawn, board.getColorToNotMove()});
        const int8_t mainDirection = board.getCurrentState().isWhiteToMove ? DirectionIndex64::S : DirectionIndex64::N;

        oppositeSlidingPiecesAndPawns |= pawns;

        const int8_t reverseDirectionLeft = -mainDirection + DirectionIndex64::E;
        const int8_t reverseDirectionRight = -mainDirection + DirectionIndex64::W;
        Bitboard captures_left = ((pawns & 0xfefefefefefefefe) << (mainDirection + DirectionIndex64::W));
        Bitboard captures_right = ((pawns & 0x7f7f7f7f7f7f7f7f) << (mainDirection + DirectionIndex64::E));
        attackedSquares |= captures_left;
        attackedSquares |= captures_right;
        
        while (captures_left.hasPieces()) {
            const int target_square = captures_left.getLS1B();
            const int origin_square = target_square + reverseDirectionLeft;
            squaresAttackedBySquare.at(origin_square).setBit(target_square);
            squaresAttackingSquare.at(target_square).setBit(origin_square);
            captures_left.clearLS1B();
        }
        while (captures_right.hasPieces()) {
            const int target_square = captures_right.getLS1B();
            const int origin_square = target_square + reverseDirectionRight;
            squaresAttackedBySquare.at(origin_square).setBit(target_square);
            squaresAttackingSquare.at(target_square).setBit(origin_square);
            captures_right.clearLS1B();
        }
    }


    // generate target restriction
    Bitboard kingBB = board.getBitboard({PieceType::King, board.getColorToMove()});
    Bitboard attackers = squaresAttackingSquare.at(kingBB.getLS1B());
    const auto preselection = obstructedLUT.at(kingBB.getLS1B());
    
    if (attackers.getNumPieces() >= 2){
        possibleTargets = 0;
    }
    if ((attackers & board.getBitboard({PieceType::Knight, board.getColorToNotMove()})).hasPieces()){
        possibleTargets = attackers;
    }
    else if (attackers.hasPieces()){
        attackers &= oppositeSlidingPiecesAndPawns;
        possibleTargets |= attackers;
        while (attackers.hasPieces()) {
            possibleTargets |= preselection.at(attackers.getLS1B());
            attackers.clearLS1B();
        }
    }
    else{
        possibleTargets = Utils::binaryOnes<uint64_t>(64);
    }
}

void MoveGenerator::generateAllSlidingMoves(Board const& board, Bitboard targetMask){
    auto const helper = [&]<int startIdx, int endIdx>(PieceType pieceType){
        const Bitboard bitboard = board.getBitboard({pieceType, board.getColorToMove()}) | board.getBitboard({PieceType::Queen, board.getColorToMove()});
        Bitboard unpinnedBitboard = bitboard & ~pinnedPieces;

        while (unpinnedBitboard.hasPieces()){
            Bitboard targetSquares = allDirectionSlidingAttacks<startIdx, endIdx>(board.getAllPieceBitboard(), Utils::binaryOneAt<uint64_t>(unpinnedBitboard.getLS1B()));
            targetSquares &= ~board.getPieceBitboardForOneColor(board.getColorToMove());
            targetSquares &= targetMask;

            const Coordinate origin = Coordinate::fromIndex64(unpinnedBitboard.getLS1B());
            while (targetSquares.hasPieces()){
                generatedMoves.emplace_back(origin, Coordinate::fromIndex64(targetSquares.getLS1B()));
                targetSquares.clearLS1B();
            }
            
            unpinnedBitboard.clearLS1B();
        }

        Bitboard pinnedBitboard = bitboard & pinnedPieces;
        while (pinnedBitboard.hasPieces()){
            // avoids duplication for queensr4rk1/1pp1qBpp/p1np1n2/2b1p1B1/4P1b1/P1NP1N2/1PP1QPPP/R4RK1 b - - 0 1
            if (pinDirection.at(pinnedBitboard.getLS1B()).dir1 < startIdx || pinDirection.at(pinnedBitboard.getLS1B()).dir1 >= endIdx){
                pinnedBitboard.clearLS1B();
                continue;
    }

            Bitboard originBB = Utils::binaryOneAt<uint64_t>(pinnedBitboard.getLS1B());
            Bitboard targetSquares = slidingAttacks(originBB, ~board.getAllPieceBitboard(), pinDirection.at(pinnedBitboard.getLS1B()).dir1);
            targetSquares |=         slidingAttacks(originBB, ~board.getAllPieceBitboard(), pinDirection.at(pinnedBitboard.getLS1B()).dir2);

            targetSquares &= ~board.getPieceBitboardForOneColor(board.getColorToMove());
            targetSquares &= targetMask;

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

void MoveGenerator::generateKnightMoves(Board const& board, Coordinate square, Bitboard targetMask){
    Bitboard targets = knightSquaresValid.at(square.getIndex64()) & ~board.getPieceBitboardForOneColor(board.getColorToMove()) & targetMask;
    
    while (targets.hasPieces()){
        auto const target = targets.getLS1B();
        generatedMoves.emplace_back(square, Coordinate::fromIndex64(target));
        targets.clearLS1B();
    }
}

void MoveGenerator::generateAllKnightMoves(Board const& board, Bitboard targetMask){
    Bitboard bitboard = board.getBitboard({PieceType::Knight, board.getColorToMove()}) & ~pinnedPieces;

    while (bitboard.hasPieces()){
        generateKnightMoves(board, Coordinate::fromIndex64(bitboard.getLS1B()), targetMask);
        bitboard.clearLS1B();
    }
}

void MoveGenerator::generateAllKingMoves(Board const& board, Bitboard targetMask){
    Bitboard bitboard = board.getBitboard({PieceType::King, board.getColorToMove()});

    // TODO: maybe remove this since every position should have a king. Only there for debugging
    if (!bitboard.hasPieces()) return;
    Coordinate square = Coordinate::fromIndex64(bitboard.getLS1B());

    Bitboard targets = kingSquaresValid.at(bitboard.getLS1B()) & ~(board.getPieceBitboardForOneColor(board.getColorToMove())  | attackedSquares);
    targets &= targetMask;

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

void MoveGenerator::generateAllPawnMoves(Board const& board, Bitboard targetMask){
    const auto colorToMove = board.getColorToMove();


    const Bitboard unpinnedPawns = board.getBitboard({PieceType::Pawn, colorToMove}) & ~pinnedPieces;
    const Bitboard occupied = board.getAllPieceBitboard();
    const Bitboard occupied_other_color = board.getPieceBitboardForOneColor(board.getColorToNotMove());

    Bitboard unpinnedPawnsLeft = unpinnedPawns, unpinnedPawnsRight = unpinnedPawns; 

    Bitboard pawns = unpinnedPawns;
    Bitboard pinnedPawns = board.getBitboard({PieceType::Pawn, colorToMove}) & pinnedPieces;
    while (pinnedPawns.hasPieces()){
        if (pinDirection.at(pinnedPawns.getLS1B()).dir1 == 0 || pinDirection.at(pinnedPawns.getLS1B()).dir2 == 0)
            pawns.setBit(pinnedPawns.getLS1B());
        pinnedPawns.clearLS1B();
    }

    const int8_t mainDirection = board.getCurrentState().isWhiteToMove ? DirectionIndex64::N : DirectionIndex64::S;
    const Bitboard doublePushMask = board.getCurrentState().isWhiteToMove ? 0x0000000000ff0000 : 0x0000ff0000000000;

    const int8_t reverseDirection = -mainDirection;
    const int8_t reverseDirectionLeft = reverseDirection + DirectionIndex64::E;
    const int8_t reverseDirectionRight = reverseDirection + DirectionIndex64::W;

    pinnedPawns = board.getBitboard({PieceType::Pawn, colorToMove}) & pinnedPieces;
    while (pinnedPawns.hasPieces()){
        if (pinDirection.at(pinnedPawns.getLS1B()).dir1 == 4 || pinDirection.at(pinnedPawns.getLS1B()).dir1 == 6)
            unpinnedPawnsLeft.setBit(pinnedPawns.getLS1B());
        else if (pinDirection.at(pinnedPawns.getLS1B()).dir1 == 5 || pinDirection.at(pinnedPawns.getLS1B()).dir1 == 7)
            unpinnedPawnsRight.setBit(pinnedPawns.getLS1B());
        pinnedPawns.clearLS1B();
    }

    Bitboard single_pushes = (pawns << mainDirection) & ~occupied;
    Bitboard double_pushes = ((single_pushes & doublePushMask) << mainDirection) & ~occupied & targetMask;
    Bitboard captures_left = ((unpinnedPawnsLeft & 0xfefefefefefefefe) << (mainDirection + DirectionIndex64::W)) & occupied_other_color & targetMask;
    Bitboard captures_right = ((unpinnedPawnsRight & 0x7f7f7f7f7f7f7f7f) << (mainDirection + DirectionIndex64::E)) & occupied_other_color & targetMask;

    single_pushes &= targetMask;

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
        const Coordinate kingSquare = Coordinate::fromIndex64(board.getBitboard({PieceType::King, colorToMove}).getLS1B()); 
        const int correctY = board.getCurrentState().isWhiteToMove ? 4 : 3;
        const Bitboard QandRsOnCorrectY = Bitboard(Utils::binaryOnes<uint64_t>(8) << correctY*8) & (board.getBitboard({PieceType::Rook, board.getColorToNotMove()}) | board.getBitboard({PieceType::Queen, board.getColorToNotMove()}));
        // intentional shadow
        const auto pawns = board.getBitboard({PieceType::Pawn, colorToMove}) | board.getBitboard({PieceType::Pawn, board.getColorToNotMove()});

        auto const oneSideEP = [&](int side, Coordinate direction, int ignoreX){
            auto const originSquare = epCaptureSquare + direction;
            if (epCaptureSquare.x == ignoreX || !board.getBitboard({PieceType::Pawn, colorToMove}).isOccupied(originSquare)) return;

            const Bitboard theTwoPawnsBB = Utils::binaryOneAt<uint64_t>(epCaptureSquare.getIndex64()) | Utils::binaryOneAt<uint64_t>(epCaptureSquare.getIndex64()+side);
            const Bitboard modifiedOccupied = occupied & ~theTwoPawnsBB;
            if (kingSquare.y == correctY){
                // the pawns might be "pinned"
                int dir = kingSquare.getIndex64() < epCaptureSquare.getIndex64() ? 1 : -1;
                int stopSquare = (kingSquare.getIndex64() / 8) * 8 + (dir == 1 ? 8 : -1);
                // loop until queen or rook
                for (int square = kingSquare.getIndex64()+dir; square != stopSquare; square += dir){
                    if (QandRsOnCorrectY.isOccupied(square)) return;
                    if (modifiedOccupied.isOccupied(square)) break;
                }
            }

            // handle individually pinned pawns
            // the moving pawn
            if (pinnedPieces.isOccupied(originSquare)){
                int allowedPinDirection;
                switch(mainDirection - side){
                    case DirectionIndex64::NW: allowedPinDirection = 4; break;
                    case DirectionIndex64::NE: allowedPinDirection = 5; break;
                    case DirectionIndex64::SW: allowedPinDirection = 6; break;
                    case DirectionIndex64::SE: allowedPinDirection = 7; break;
                }
                auto actualDirection = pinDirection.at(originSquare.getIndex64());

                // test if the pawn is pinned
                if (actualDirection.dir1 != allowedPinDirection && actualDirection.dir2 != allowedPinDirection) return;
            }

            // the pawn getting captured
            if (pinnedPieces.isOccupied(epCaptureSquare)){
                const int allowedPinDirection = 0; // vertical pins are fine
                auto actualDirection = pinDirection.at(epCaptureSquare.getIndex64());

                // test if the pawn is pinned
                if (actualDirection.dir1 != allowedPinDirection && actualDirection.dir2 != allowedPinDirection) return;
            }

            Move& move = generatedMoves.emplace_back(originSquare, board.getEnPassantSquareForFEN());
            move.isEnPassant = true;
        };

        bool onlyPossibleMove = targetMask.getNumPieces() == 1 && targetMask.isOccupied(board.getEnPassantSquareForFEN().getIndex64() + reverseDirection);
        if (targetMask.isOccupied(board.getEnPassantSquareForFEN()) || onlyPossibleMove){
            oneSideEP(-1, Direction::W, 0);
            oneSideEP(1, Direction::E, 7);
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