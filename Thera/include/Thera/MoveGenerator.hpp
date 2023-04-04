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
         * @brief A bitboard only used for debugging.
         * 
         * It is most likely used to display a arbitrary bitboard in a UI.
         * 
         */
        static inline Bitboard debugBitboard = 0;


    private:

        /**
         * @brief Generate all attacked squares.
         * 
         * @param board the position to operate on
         */
        void generateAttackData(Board const& board);

        /**
         * @brief Generate bishop, rook and queen moves.
         * 
         * May generate incorrect results for empty squares or the color to not move. 
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generateSlidingMoves(Board const& board, Bitboard square, uint8_t startDirectionIdx, uint8_t endDirectionIdx);

        /**
         * @brief Generate all bishop, rook and queen moves.
         * 
         * May generate incorrect results for empty squares or the color to not move.
         * 
         * @param board the position to operate on
         */
        void generateAllSlidingMoves(Board const& board);

        /**
         * @brief Generate knight moves.
         * 
         * May generate incorrect results for empty squares or the color to not move.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generateKnightMoves(Board const& board, Coordinate square);

        /**
         * @brief Generate all knight moves.
         * 
         * @param board the position to operate on
         */
        void generateAllKnightMoves(Board const& board);

        /**
         * @brief Generate king moves.
         * 
         * May generate incorrect results for empty squares or the color to not move.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generateKingMoves(Board const& board, Coordinate square);

        /**
         * @brief Generate all king moves.
         * 
         * @param board the position to operate on
         */
        void generateAllKingMoves(Board const& board);

        /**
         * @brief Generate all pawn moves.
         * 
         * @param board the position to operate on
         */
        void generateAllPawnMoves(Board const& board);

        /**
         * @brief Add a pawn move and apply promotions if needed.
         * 
         * @param startIndex the starting square
         * @param endIndex the ending square
         */
        void addPawnMovePossiblyPromotion(Move const& move, Board const& board);

    private:
        using CDNT = Coordinate;

    public:
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
        static constexpr auto isKingKnightMoveValidGenerator = [](auto const& offsets) constexpr {
            std::array<Bitboard, 8*8> result = {};
            for (int x=0; x<8; x++){
                for (int y=0; y<8; y++){
                    const Coordinate square = Coordinate(x, y);
                    result.at(square.getIndex64()) = 0;
                    for (int dirIdx = 0; dirIdx < offsets.size(); dirIdx++){
                        const Coordinate targetSquare = Coordinate(square) + offsets.at(dirIdx);

                        if (targetSquare.isOnBoard())
                            result.at(square.getIndex64())[targetSquare.getIndex64()] = true;
                    }
                }
            }
            return result;
        };
        
        static constexpr auto knightSquaresValid = isKingKnightMoveValidGenerator(MoveGenerator::knightOffsets);
        static constexpr auto kingSquaresValid = isKingKnightMoveValidGenerator(MoveGenerator::slidingPieceOffsets);

        
        
        /*
        returns: 
        - squares:
          - directions:
            - is target valid
            - target square

        */
       static constexpr std::array<std::array<std::pair<int, std::array<Coordinate, 7>>, MoveGenerator::slidingPieceOffsets.size()>, 8*8> squaresInDirection = [](){
            std::array<std::array<std::pair<int, std::array<Coordinate, 7>>, MoveGenerator::slidingPieceOffsets.size()>, 8*8> result = {};
            for (int x=0; x<8; x++){
                for (int y=0; y<8; y++){
                    for (int dirIdx = 0; dirIdx < MoveGenerator::slidingPieceOffsets.size(); dirIdx++){
                        const Coordinate square = Coordinate(x, y);
                        int& numSquares = result.at(square.getIndex64()).at(dirIdx).first;
                        auto& squares = result.at(square.getIndex64()).at(dirIdx).second;

                        auto target = square + MoveGenerator::slidingPieceOffsets.at(dirIdx);
                        numSquares = 0;
                        while (target.isOnBoard()){
                            squares.at(numSquares) = target;
                            numSquares++;

                            target += MoveGenerator::slidingPieceOffsets.at(dirIdx);
                        }
                    }
                }
            }
            return result;
        }();

    private:
        std::vector<Move> generatedMoves;
        Bitboard attackedSquares;
        Bitboard attackedSquaresBishop;
        Bitboard attackedSquaresRook;
};
}