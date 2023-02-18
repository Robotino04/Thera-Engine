#pragma once

#include "Thera/Move.hpp"

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
        std::vector<Move> generateAllMoves(Board& board);

        /**
         * @brief Generate legal from the given square.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         * @return std::vector<Move> the generated moves
         */
        std::vector<Move> generateMoves(Board& board, Coordinate8x8 square);

    private:
        /**
         * @brief Generate bishop, rook and queen moves.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generateSlidingMoves(Board& board, Coordinate8x8 square);

        /**
         * @brief Generate all bishop, rook and queen moves.
         * 
         * @param board the position to operate on
         */
        void generateAllSlidingMoves(Board& board);

        /**
         * @brief Generate king and knight moves.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generateKingKnightMoves(Board& board, Coordinate8x8 square);

        /**
         * @brief Generate all king and knight moves.
         * 
         * @param board the position to operate on
         */
        void generateAllKingKnightMoves(Board& board);

        /**
         * @brief Generate pawn moves.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generatePawnMoves(Board& board, Coordinate8x8 square);

        /**
         * @brief Generate all pawn moves.
         * 
         * @param board the position to operate on
         */
        void generateAllPawnMoves(Board& board);

        /**
         * @brief Add a pawn move and apply promotions if needed.
         * 
         * @param startIndex the starting square
         * @param endIndex the ending square
         */
        void addPawnMove(Move const& move, Board const& board);

    public:
        static const int maxMovesPerPosition = 218;

        static constexpr std::array<int8_t, 8> slidingPieceOffsets = {
            1, -1, 10, -10, // Rook
            9, -9, 11, -11, // Bishop
        };
        static constexpr std::array<int8_t, 16> kingKnightOffsets = {
            1, -1, 10, -10, 9, -9, 11, -11,   // King
            -21, -19, -8, 12, 21, 19, 8, -12, // Knight
        };

    private:
        std::vector<Move> generatedMoves;
        
};

}