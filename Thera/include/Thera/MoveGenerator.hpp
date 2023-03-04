#pragma once

#include "Thera/Move.hpp"
#include "Thera/Coordinate.hpp"

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
        std::vector<Move> generateMoves(Board& board, Coordinate square);

    private:
        /**
         * @brief Generate bishop, rook and queen moves.
         * 
         * May generate incorrect results for empty squares or the color to not move. 
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generateSlidingMoves(Board& board, Coordinate square, uint_fast8_t startDirectionIdx, uint_fast8_t endDirectionIdx);

        /**
         * @brief Generate all bishop, rook and queen moves.
         * 
         * May generate incorrect results for empty squares or the color to not move.
         * 
         * @param board the position to operate on
         */
        void generateAllSlidingMoves(Board& board);

        /**
         * @brief Generate knight moves.
         * 
         * May generate incorrect results for empty squares or the color to not move.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generateKnightMoves(Board& board, Coordinate square);

        /**
         * @brief Generate all knight moves.
         * 
         * @param board the position to operate on
         */
        void generateAllKnightMoves(Board& board);

        /**
         * @brief Generate king moves.
         * 
         * May generate incorrect results for empty squares or the color to not move.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generateKingMoves(Board& board, Coordinate square);

        /**
         * @brief Generate all king moves.
         * 
         * @param board the position to operate on
         */
        void generateAllKingMoves(Board& board);

        /**
         * @brief Generate pawn move.
         * 
         * May generate incorrect results for empty squares or the color to not move.
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generatePawnMoves(Board& board, Coordinate square);

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

    private:
        using POTBC = Coordinate;
    public:
        static const int maxMovesPerPosition = 218;

        static constexpr std::array<POTBC, 8> slidingPieceOffsets = {
            POTBC(1, 0), POTBC(-1, 0), POTBC(0, 1), POTBC(0, -1),  // Rook
            POTBC(1, 1), POTBC(-1, -1), POTBC(1, -1), POTBC(-1, 1), // Bishop
        };
        static constexpr std::array<POTBC, 8> knightOffsets = {
            POTBC(1, 2), POTBC(2, 1), POTBC(2, -1), POTBC(1, -2), POTBC(-1, -2), POTBC(-2, -1), POTBC(-2, 1), POTBC(-1, 2)    
        };
        static constexpr std::array<POTBC, 8> kingOffsets = {
            POTBC(1, 0), POTBC(1, -1), POTBC(0, -1), POTBC(-1, -1), POTBC(-1, 0), POTBC(-1, 1), POTBC(0, 1), POTBC(1, 1)
        };

        /*
        contents: 
        - squares:
          - directions:
            - number of squares in this direction
            - list of squares in this direction

        */
        static constexpr auto isKingKnightMoveValidGenerator = [](std::array<POTBC, 8> offsets) constexpr{
            std::array<std::array<std::pair<bool, Coordinate>, offsets.size()>, 8*8> result = {};
            for (int x=0; x<8; x++){
                for (int y=0; y<8; y++){
                    for (int dirIdx = 0; dirIdx < offsets.size(); dirIdx++){
                        const Coordinate square = Coordinate(x, y);
                        
                        bool& isPossible = result.at(square.getIndex64()).at(dirIdx).first;
                        const POTBC targetSquare = Coordinate(square) + offsets.at(dirIdx);

                        isPossible = targetSquare.isOnBoard();
                        if (isPossible)
                            result.at(square.getIndex64()).at(dirIdx).second = targetSquare;
                    }
                }
            }
            return result;
        };
        
        static constexpr auto knightSquaresValid = isKingKnightMoveValidGenerator(MoveGenerator::knightOffsets);
        static constexpr auto kingSquaresValid = isKingKnightMoveValidGenerator(MoveGenerator::kingOffsets);
        

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
};
}