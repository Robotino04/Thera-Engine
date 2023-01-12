#pragma once

#include "ChessBot/Move.hpp"

#include "ChessBot/Utils/BuildType.hpp"
#include "ChessBot/Utils/Math.hpp"

#include <bitset>
#include <array>
#include <stdint.h>
#include <stdexcept>

namespace ChessBot{

class Board;

/**
 * @brief A birboard able to lookup in both ways.
 * 
 * Using any binary math operators (&, |, ^, etc.) will result in
 * a "broken" bitboard. Only further math operators and isOccupied
 * will work correctly. The use of a "broken" bitboard is safe but
 * functions like applyMove won't have the desired effect.
 * 
 * @tparam N max number of pieces in this bitboard.
 */
template<int N>
class Bitboard: private std::bitset<10*12>{
    friend class Board;
    
    public:
        void clear(){
            *this &= 0;
            numPieces = 0;
            occupiedSquares.fill(-1);
        }

        /**
         * @brief Test if square is occupied. Only does bounds checking in debug builds.
         * 
         * @param square 
         * @return true 
         * @return false 
         */
        bool isOccupied(int8_t square) const{
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug)
                return test(square);
            else
                return (*this)[square];
        }

        /**
         * @brief Blindly places the piece on the board without any sort of test.
         * 
         * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
         * 
         * @param square 
         */
        void placePiece(int8_t square){
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (isOccupied(square))
                    throw std::invalid_argument("Tried to place piece on already occupied square.");
            }
            
            occupiedSquares[numPieces] = square;
            reverseOccupiedSquares[square] = numPieces;
            (*this)[square] = true;
            numPieces++;
        }

        /**
         * @brief Blindly removes the piece from the board without any sort of test.
         * 
         * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
         * 
         * @param square 
         */
        void removePiece(int8_t square){
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (!isOccupied(square))
                    throw std::invalid_argument("Tried to remove piece from empty square.");
                // if (!Board::isOnBoard10x12(square))
                //     throw std::invalid_argument("Tried to remove piece from outside the board.");
            }
            
            (*this)[square] = false;

            const int pieceIndex = reverseOccupiedSquares.at(square);

            // move last entry in occupiedSquares to the empty spot
            occupiedSquares.at(pieceIndex) = occupiedSquares.at(numPieces-1);
            reverseOccupiedSquares.at(occupiedSquares.at(pieceIndex)) = pieceIndex;

            reverseOccupiedSquares.at(square) = -1;
            numPieces--;
        }

        /**
         * @brief Blindly applies the base move without any sort of test.
         * 
         * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
         * 
         * @param move 
         */
        void applyMove(Move const& move){
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (isOccupied(move.endIndex)){
                    // capture
                    removePiece(move.endIndex);
                }
                if (!isOccupied(move.startIndex)){
                    throw std::runtime_error("Tried to make move starting on an empty square.");
                }
                if (!Utils::isInRange<int8_t>(move.startIndex, 0, reverseOccupiedSquares.size()))
                    throw std::out_of_range("Move start index is outside the board");
                if (!Utils::isInRange<int8_t>(move.endIndex, 0, reverseOccupiedSquares.size()))
                    throw std::out_of_range("Move end index is outside the board");
            }

            flip(move.startIndex); // will remove the piece
            flip(move.endIndex); // will place the piece

            int pieceIndex = reverseOccupiedSquares.at(move.startIndex);
            reverseOccupiedSquares.at(move.startIndex) = -1;
            reverseOccupiedSquares.at(move.endIndex) = pieceIndex;
            occupiedSquares.at(pieceIndex) = move.endIndex;
        }

        
        constexpr auto begin(){
            return occupiedSquares.begin();
        }
        constexpr auto begin() const{
            return occupiedSquares.begin();
        }
        constexpr auto end(){
            return std::next(occupiedSquares.begin(), numPieces);
        }
        constexpr auto end() const{
            return std::next(occupiedSquares.begin(), numPieces);
        }
    
    private:
        std::array<int8_t, N> occupiedSquares;
        std::array<int, 10*12> reverseOccupiedSquares;
        int numPieces = 0;
};

}