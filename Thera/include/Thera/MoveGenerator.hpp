#pragma once

#include "Thera/Move.hpp"
#include "Thera/Utils/Coordinates.hpp"
#include "Thera/TemporyryCoordinateTypes.hpp"

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
         * May generate incorrect results for empty squares or the color to not move. 
         * 
         * @param board the position to operate on
         * @param square the square to generate moves for
         */
        void generateSlidingMoves(Board& board, Coordinate8x8 square, uint_fast8_t startDirectionIdx, uint_fast8_t endDirectionIdx);

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
        void generateKnightMoves(Board& board, Coordinate8x8 square);

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
        void generateKingMoves(Board& board, Coordinate8x8 square);

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
        static constexpr std::array<int8_t, 8> knightOffsets = {-21, -19, -8, 12, 21, 19, 8, -12};
        static constexpr std::array<int8_t, 8> kingOffsets = {1, -1, 10, -10, 9, -9, 11, -11};

        /*
        contents: 
        - squares:
          - directions:
            - number of squares in this direction
            - list of squares in this direction

        */
        static constexpr std::array<std::array<std::pair<int, std::array<Coordinate8x8, 7>>, slidingPieceOffsets.size()>, 8*8> squaresInDirection = [](){
            std::array<std::array<std::pair<int, std::array<Coordinate8x8, 7>>, slidingPieceOffsets.size()>, 8*8> result = {};
            for (int x=0; x<8; x++){
                for (int y=0; y<8; y++){
                    for (int dirIdx = 0; dirIdx < slidingPieceOffsets.size(); dirIdx++){
                        int& numSquares = result.at(Utils::coordToIndex(x, y).pos).at(dirIdx).first;
                        auto& squares = result.at(Utils::coordToIndex(x, y).pos).at(dirIdx).second;

                        Coordinate8x8 square = Utils::coordToIndex(x, y);
                        while (Utils::isOnBoard(Utils::applyOffset(square, slidingPieceOffsets.at(dirIdx)*(numSquares+1)))){
                            squares.at(numSquares) = Utils::applyOffset(square, slidingPieceOffsets.at(dirIdx)*(numSquares+1));
                            numSquares++;
                        }
                    }
                }
            }
            return result;
        }();

        /*
        returns: 
        - squares:
          - directions:
            - is target valid
            - target square

        */
        static constexpr auto isKingKnightMoveValidGenerator = [](std::array<int8_t, 8> offsets) constexpr{
            std::array<std::array<std::pair<bool, Coordinate8x8>, offsets.size()>, 8*8> result = {};
            for (int x=0; x<8; x++){
                for (int y=0; y<8; y++){
                    for (int dirIdx = 0; dirIdx < offsets.size(); dirIdx++){
                        bool& isPossible = result.at(Utils::coordToIndex(x, y).pos).at(dirIdx).first;
                        
                        Coordinate8x8 square = Utils::coordToIndex(x, y);
                        const PossiblyOffTheBoardCoordinate targetSquare = Utils::applyOffset(square, offsets.at(dirIdx));

                        isPossible = Utils::isOnBoard(targetSquare);
                        if (isPossible)
                            result.at(Utils::coordToIndex(x, y).pos).at(dirIdx).second = targetSquare;
                    }
                }
            }
            return result;
        }; 

        static constexpr auto knightSquaresValid = isKingKnightMoveValidGenerator(knightOffsets);
        static constexpr auto kingSquaresValid = isKingKnightMoveValidGenerator(kingOffsets);

    private:
        std::vector<Move> generatedMoves;
        
};
}