#pragma once

#include "Thera/Move.hpp"
#include "Thera/Coordinate.hpp"
#include "Thera/Bitboard.hpp"

#include <vector>
#include <array>


namespace Thera{
struct Board;

class MoveGenerator{
    public:
        /**
         * @brief Generate all legal moves in the given position.
         * 
         * @param board the position to operate on
         * @return std::vector<Move> the generated moves
         */
        std::vector<Move> generateAllMoves(Board const& board);

        /**
         * @brief Generate all attacked squares.
         * 
         * @param board the position to operate on
         */
        void generateAttackData(Board const& board);


        constexpr Bitboard getAttackedSquares() const { return attackedSquares; }
        constexpr Bitboard getPinnedPieces() const{ return pinnedPieces; }
        constexpr Bitboard getSquaresAttackedBy(Coordinate square) const{
            return squaresAttackedBySquare.at(square.getIndex64());
        }
        constexpr Bitboard getSquaresAttacking(Coordinate square) const{
            return squaresAttackingSquare.at(square.getIndex64());
        }
        constexpr Bitboard getPossibleMoveTargets() const{ return possibleTargets; }

    private:
        
        bool isCheck(Board const& board) const;

        /**
         * @brief Generate all pin data.
         * 
         * @param board the position to operate on
         */
        void generatePins(Board const& board);
        

        /**
         * @brief Generate all bishop, rook and queen moves.
         * 
         * May generate incorrect results for empty squares or the color to not move.
         * 
         * @param board the position to operate on
         * @param targetMask a mask to restrict move targets
         */
        void generateAllSlidingMoves(Board const& board, Bitboard targetMask);

        /**
         * @brief Generate knight moves.
         * 
         * May generate incorrect results for empty squares or the color to not move.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         * @param targetMask a mask to restrict move targets
         */
        void generateKnightMoves(Board const& board, Coordinate square, Bitboard targetMask);

        /**
         * @brief Generate all knight moves.
         * 
         * @param board the position to operate on
         * @param targetMask a mask to restrict move targets
         */
        void generateAllKnightMoves(Board const& board, Bitboard targetMask);

        /**
         * @brief Generate king moves.
         * 
         * May generate incorrect results for empty squares or the color to not move.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         * @param targetMask a mask to restrict move targets
         */
        void generateKingMoves(Board const& board, Coordinate square, Bitboard targetMask);

        /**
         * @brief Generate all king moves.
         * 
         * @param board the position to operate on
         * @param targetMask a mask to restrict move targets
         */
        void generateAllKingMoves(Board const& board, Bitboard targetMask);

        /**
         * @brief Generate all pawn moves.
         * 
         * @param board the position to operate on
         * @param targetMask a mask to restrict move targets
         */
        void generateAllPawnMoves(Board const& board, Bitboard targetMask);

        /**
         * @brief Add a pawn move and apply promotions if needed.
         * 
         * @param startIndex the starting square
         * @param endIndex the ending square
         */
        void addPawnMovePossiblyPromotion(Move move, Board const& board);

    public:

        /**
         * @brief A bitboard only used for debugging.
         * 
         * It is most likely used to display a arbitrary bitboard in a UI.
         * 
         */
        static inline Bitboard debugBitboard = 0;

        static constexpr int maxMovesPerPosition = 218;

        static constexpr std::array<Coordinate, 8> slidingPieceOffsets{
            // rook
            Direction::N, Direction::W, Direction::E, Direction::S,

            // bishop
            Direction::NW, Direction::NE, Direction::SW, Direction::SE,
        };
        static constexpr std::array<int, 8> slidingPieceShiftAmounts = {
            // rook
            DirectionIndex64::N, DirectionIndex64::W, DirectionIndex64::E, DirectionIndex64::S,

            // bishop
            DirectionIndex64::NW, DirectionIndex64::NE, DirectionIndex64::SW, DirectionIndex64::SE,
        };
        static constexpr std::array<Bitboard, 8> slidingPieceAvoidWrapping = {
            // rook
                                0xFFFFFFFFFFFFFF00,

            0x7F7F7F7F7F7F7F7F,                     0xFEFEFEFEFEFEFEFE,

                                0x00FFFFFFFFFFFFFF,

            // bishop
            0x7F7F7F7F7F7F7F00,                     0xFEFEFEFEFEFEFE00,



            0x007F7F7F7F7F7F7F,                     0x00FEFEFEFEFEFEFE,
        };

        static constexpr std::array<Coordinate, 8> knightOffsets = {
            Direction::N + Direction::NE,
            Direction::N + Direction::NW,

            Direction::W + Direction::NW,
            Direction::W + Direction::SW,

            Direction::S + Direction::SE,
            Direction::S + Direction::SW,

            Direction::E + Direction::NE,
            Direction::E + Direction::SE,
        };

        /*
        contents: 
        - squares:
          - directions:
            - number of squares in this direction
            - list of squares in this direction

        */
        static constexpr auto isKingKnightMoveValidGenerator = [](auto const& offsets, int maxDelta) constexpr {
            std::array<Bitboard, 8*8> result = {};
            for (int x=0; x<8; x++){
                for (int y=0; y<8; y++){
                    const Coordinate square = Coordinate(x, y);
                    result.at(square.getIndex64()) = 0;
                    for (int dirIdx = 0; dirIdx < offsets.size(); dirIdx++){
                        const Coordinate targetSquare = Coordinate(square) + offsets.at(dirIdx);
                        const int deltaX = int(targetSquare.x) - int(square.x);
                        const int deltaY = int(targetSquare.y) - int(square.y);
                        if (std::abs(deltaX) > maxDelta) continue;
                        if (std::abs(deltaY) > maxDelta) continue;

                        result.at(square.getIndex64()).setBit(targetSquare.getIndex64());
                    }
                }
            }
            return result;
        };
        
        static constexpr auto knightSquaresValid = isKingKnightMoveValidGenerator(MoveGenerator::knightOffsets, 2);
        static constexpr auto kingSquaresValid = isKingKnightMoveValidGenerator(MoveGenerator::slidingPieceOffsets, 1);

        static constexpr auto obstructedLUT = []() constexpr {
            // https://www.chessprogramming.org/Square_Attacked_By#Pure_Calculation
            const auto singleEvaluation = [](uint8_t sq1, uint8_t sq2) -> Bitboard{
                const Bitboard m1(0xFFFFFFFFFFFFFFFF);
                const Bitboard a2a7(0x0001010101010100);
                const Bitboard b2g7(0x0040201008040200);
                const Bitboard h1b7(0x0002040810204080); /* Thanks Dustin, g2b7 did not work for c1-a3 */
                Bitboard btwn, line, rank, file;

                btwn  = (m1 << sq1) ^ (m1 << sq2);
                file  =   (sq2 & 7) - (sq1   & 7);
                rank  =  ((sq2 | 7) -  sq1) >> 3 ;
                line  =      (   (file  &  7) - 1) & a2a7; /* a2a7 if same file */
                line += Bitboard(2) * ((   (rank  &  7) - 1) >> 58); /* b1g1 if same rank */
                line += (((rank - file) & 15) - 1) & b2g7; /* b2g7 if same diagonal */
                line += (((rank + file) & 15) - 1) & h1b7; /* h1b7 if same antidiag */
                line *= btwn & -btwn; /* mul acts like shift by smaller square */
                return line & btwn;   /* return the bits on that line in-between */
            };

            std::array<std::array<Bitboard, 64>, 64> result;

            for (int a=0; a<64; a++){
                for (int b=0; b<64; b++){
                    result.at(a).at(b) = singleEvaluation(a, b);
                }
            }
            return result;
        }();

        struct DirectionPair{
            uint8_t dir1: 3;
            uint8_t dir2: 3;
        };

    private:
        std::vector<Move> generatedMoves;
        std::array<Bitboard, 64> squaresAttackingSquare;
        std::array<Bitboard, 64> squaresAttackedBySquare;
        Bitboard attackedSquares;
        Bitboard pinnedPieces;
        Bitboard possibleTargets;
        std::array<DirectionPair, 64> pinDirection;
};
}