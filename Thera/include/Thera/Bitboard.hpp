#pragma once

#include "Thera/Move.hpp"

#include "Thera/Utils/BuildType.hpp"
#include "Thera/Utils/Math.hpp"
#include "Thera/Utils/Coordinates.hpp"

#include <array>
#include <stdint.h>
#include <stdexcept>
#include <bit>

namespace Thera{

class Board;

/**
 * @brief A bitboard able to lookup in both ways.
 * 
 * Using any bitwise math operators (&, |, ^, etc.) will result in
 * a "broken" bitboard. Only further math operators and isOccupied
 * will work correctly. The use of a "broken" bitboard is safe but
 * functions like applyMove won't have the desired effect. "Broken"
 * bitboards also can't be checked for desyncs at runtime. 
 * 
 * @tparam N max number of pieces in this bitboard.
 */
template<int N>
class Bitboard{
    static_assert(Utils::isInRange(N, 1, 64), "Bitboards can only store up to 64 pieces.");
    friend class Board;
    
    public:
        struct Reference{
            friend Bitboard;
            public:
                constexpr bool operator = (bool value){
                    parent.bits = Utils::setBit<decltype(parent.bits)>(parent.bits, bitIndex, value);
                    return value;
                }

                constexpr operator bool () const{
                    return Utils::getBit(parent.bits, bitIndex);
                }

            private:
                constexpr Reference(Bitboard<N>& parent, size_t bitIndex): parent(parent), bitIndex(bitIndex){}

                Bitboard<N>& parent;
                const size_t bitIndex = 0;
        };
        friend Reference;


        constexpr void clear(){
            bits = __uint128_t(0);
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
        constexpr bool isOccupied(int8_t square) const{
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug)
                if (!Utils::isInRange<int8_t>(square, 0, 127))
                    throw std::out_of_range("Square index is outside the board");
            return Utils::getBit(bits, square);
        }

        /**
         * @brief Blindly place the piece on the board without any sort of test.
         * 
         * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
         * 
         * @param square
         */
        constexpr void placePiece(int8_t square){
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
         * @brief Blindly remove the piece from the board without any sort of test.
         * 
         * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
         * 
         * @param square 
         */
        void removePiece(int8_t square){
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (!isOccupied(square))
                    throw std::invalid_argument("Tried to remove piece from empty square.");
                if (!Utils::isOnBoard10x12(square))
                    throw std::invalid_argument("Tried to remove piece from outside the board.");
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
         * @brief Blindly apply the base move without any sort of test.
         * 
         * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
         * 
         * @param move 
         */
        constexpr void applyMove(Move const& move){
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (!isOccupied(move.startIndex)){
                    throw std::runtime_error("Tried to make move starting on an empty square.");
                }
                if (!Utils::isInRange<int8_t>(move.startIndex, 0, reverseOccupiedSquares.size()))
                    throw std::out_of_range("Move start index is outside the board");
                if (!Utils::isInRange<int8_t>(move.endIndex, 0, reverseOccupiedSquares.size()))
                    throw std::out_of_range("Move end index is outside the board");
            }

            clearBit(move.startIndex); // will remove the piece
            setBit(move.endIndex); // will place the piece

            int pieceIndex = reverseOccupiedSquares.at(move.startIndex);
            reverseOccupiedSquares.at(move.startIndex) = -1;
            reverseOccupiedSquares.at(move.endIndex) = pieceIndex;
            occupiedSquares.at(pieceIndex) = move.endIndex;
        }

        /**
         * @brief Get a list of pieces. Only does sanity checks in debug builds.
         * 
         * @return constexpr std::array<int8_t, N> list of 8x8 square indices with first numPieces pieces valid
         */
        std::array<int8_t, N> getPieces8x8() const{
            uint64_t x = 0;
            for (int8_t i=0; i<64; i++){
                x = (x << 1) | isOccupied(Utils::to10x12Coords(i));
            }
            x = Utils::reverseBits(x);

            //TODO: change to all 8x8 coords

            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (std::popcount(x) != numPieces)
                    throw std::runtime_error("Desync between bitboard and numPieces detected.");
            }

            std::array<int8_t, N> result;
            auto list = result.begin();



            if (x) do {
                int8_t idx = bitScanForward(x); // square index from 0..63
                *list++ = idx;
            } while (x &= x-1); // reset LS1B

            return result;
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

        constexpr int getNumPieces() const{
            return numPieces;
        }

        uint64_t getBoard8x8() const{
            uint64_t board = 0;
            for (int i=0; i<64; i++){
                board = Utils::setBit<uint64_t>(board, i, (*this)[Utils::to10x12Coords(i)]);
            }
            return board;
        }

        constexpr bool operator[] (int8_t bitIdx) const{
            return Utils::getBit(bits, bitIdx);
        }
        constexpr Reference operator[] (int8_t bitIdx){
            return Reference(*this, bitIdx);
        }
        constexpr void flipBit(int8_t bitIndex){
            bits ^= __uint128_t(1) << bitIndex;
        }

        constexpr void setBit(int8_t bitIndex){
            bits |= __uint128_t(1) << bitIndex;
        }
        constexpr void clearBit(int8_t bitIndex){
            bits &= ~(__uint128_t(1) << bitIndex);
        }

    private:



        /**
         * bitScanForward
         * @author Matt Taylor (2003)
         * @param bb bitboard to scan
         * @precondition bb != 0
         * @return index (0..63) of least significant one bit
         */
        static constexpr int8_t bitScanForward(uint64_t bb) {
            constexpr std::array<int8_t, 64> lsb_64_table ={
                63, 30,  3, 32, 59, 14, 11, 33,
                60, 24, 50,  9, 55, 19, 21, 34,
                61, 29,  2, 53, 51, 23, 41, 18,
                56, 28,  1, 43, 46, 27,  0, 35,
                62, 31, 58,  4,  5, 49, 54,  6,
                15, 52, 12, 40,  7, 42, 45, 16,
                25, 57, 48, 13, 10, 39,  8, 44,
                20, 47, 38, 22, 17, 37, 36, 26
            };

            uint32_t folded = 0;
            bb ^= bb - 1;
            folded = (uint32_t) bb ^ (bb >> 32);
            return lsb_64_table[folded * 0x78291ACF >> 26];
        }
    
    private:
        static_assert(requires {typename __uint128_t;});
        __uint128_t bits;

        std::array<int8_t, N> occupiedSquares;
        std::array<int, 10*12> reverseOccupiedSquares;
        int numPieces = 0;
};

}