#pragma once

#include "ChessBot/Move.hpp"

#include <vector>
#include <array>


namespace ChessBot{
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
        std::vector<Move> generateMoves(Board& board, int8_t square);

    private:
        /**
         * @brief Generate bishop, rook and queen moves.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generateSlidingMoves(Board& board, int8_t square);

        /**
         * @brief Generate all bishop, rook and queen moves.
         * 
         * @param board the position to operate on
         */
        void generateAllSlidingMoves(Board& board);


    public:
        static const int maxMovesPerPosition = 218;

        static constexpr std::array<int8_t, 8> slidingPieceOffsets = {
            1, -1, 10, -10, // Rook
            9, -9, 11, -11, // Bishop
        };

    private:
        std::vector<Move> generatedMoves;
        
};

}