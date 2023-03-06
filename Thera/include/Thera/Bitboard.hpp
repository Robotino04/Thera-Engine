#pragma once

#include "Thera/Move.hpp"

#include "Thera/Utils/BuildType.hpp"
#include "Thera/Utils/Math.hpp"
#include "Thera/Coordinate.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <bit>

namespace Thera{

class Board;

/**
 * @brief A bitboard stored in an uint64_t.
 */
class Bitboard{
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
                constexpr Reference(Bitboard& parent, uint8_t bitIndex): parent(parent), bitIndex(bitIndex){}

                Bitboard& parent;
                const uint8_t bitIndex = 0;
        };
        friend Reference;

        constexpr Bitboard(){
            clear();
        }

        constexpr Bitboard(uint64_t raw){
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){

                numPieces = std::popcount(raw);
                bits = raw;
                const auto pieces = getPieces();

                clear();
                for (int i=0; i<std::popcount(raw); i++){
                    placePiece(pieces.at(i));
                }
                if (std::popcount(bits) != numPieces)
                    throw std::runtime_error("Desync between bitboard and numPieces detected.");
            }

            bits = raw;
        }


        constexpr void clear(){
            bits = uint64_t(0);
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
        constexpr bool isOccupied(Coordinate square) const{
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug)
                if (!Utils::isInRange<uint8_t>(square.getIndex64(), 0, 63))
                    throw std::out_of_range("Square index is outside the board");
            return Utils::getBit(bits, square.getIndex64());
        }

        /**
         * @brief Blindly place the piece on the board without any sort of test.
         * 
         * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
         * 
         * @param square
         */
        constexpr void placePiece(Coordinate square){
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (isOccupied(square))
                    throw std::invalid_argument("Tried to place piece on already occupied square.");

                occupiedSquares.at(numPieces) = square.getIndex64();
                reverseOccupiedSquares.at(square.getIndex64()) = numPieces;
                numPieces++;
            }
            
            (*this)[square] = true;
        }

        /**
         * @brief Blindly remove the piece from the board without any sort of test.
         * 
         * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
         * 
         * @param square 
         */
        constexpr void removePiece(Coordinate square){
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (!isOccupied(square))
                    throw std::invalid_argument("Tried to remove piece from empty square.");
                if (!square.isOnBoard())
                    throw std::invalid_argument("Tried to remove piece from outside the board.");

                const int pieceIndex = reverseOccupiedSquares.at(square.getIndex64());

                // move last entry in occupiedSquares to the empty spot
                occupiedSquares.at(pieceIndex) = occupiedSquares.at(numPieces-1);
                reverseOccupiedSquares.at(occupiedSquares.at(pieceIndex)) = pieceIndex;

                reverseOccupiedSquares.at(square.getIndex64()) = -1;
                numPieces--;
            }
            
            (*this)[square] = false;
        }

        /**
         * @brief Blindly apply the base move without any sort of test.
         * 
         * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
         * 
         * @param move 
         */
        constexpr void applyMove(Move const& move){
            const uint8_t startIndex64 = move.startIndex.getIndex64();
            const uint8_t endIndex64 = move.endIndex.getIndex64();

            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (!isOccupied(move.startIndex)){
                    throw std::runtime_error("Tried to make move starting on an empty square.");
                }
                if (!Utils::isInRange<uint8_t>(startIndex64, 0, reverseOccupiedSquares.size()))
                    throw std::out_of_range("Move start index is outside the board");
                if (!Utils::isInRange<uint8_t>(move.endIndex.getIndex64(), 0, reverseOccupiedSquares.size()))
                    throw std::out_of_range("Move end index is outside the board");
            }

            clearBit(startIndex64); // will remove the piece
            setBit(move.endIndex.getIndex64()); // will place the piece

            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                int pieceIndex = reverseOccupiedSquares.at(startIndex64);
                reverseOccupiedSquares.at(startIndex64) = -1;
                reverseOccupiedSquares.at(move.endIndex.getIndex64()) = pieceIndex;
                occupiedSquares.at(pieceIndex) = move.endIndex.getIndex64();
            }
        }

        /**
         * @brief Get a list of pieces. Only does sanity checks in debug builds.
         * 
         * The number of valid coordinates can be read by getNumPieces().
         * 
         * @return constexpr std::array<Coordinate, 64> list of coordinates
         */
        constexpr std::array<Coordinate, 64> getPieces() const{
            auto x = bits;

            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (std::popcount(x) != numPieces)
                    throw std::runtime_error("Desync between bitboard and numPieces detected.");
            }

            std::array<uint8_t, 64> resultIndices;
            uint8_t* list = resultIndices.data();

            if (x) do {
                *list++ = bitScanForward(x);
            } while (x &= x-1); // reset LS1B

            std::array<Coordinate, 64> result;
            for (int i=0; i<list - resultIndices.data(); i++){
                result[i] = Coordinate::fromIndex64(resultIndices[i]);
            }

            return result;
        }

        constexpr int getNumPieces() const{
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (std::popcount(bits) != numPieces)
                    throw std::runtime_error("Desync between bitboard and numPieces detected.");
            }
            return std::popcount(bits);
        }

        constexpr bool operator[] (Coordinate bitIdx) const{
            return Utils::getBit(bits, bitIdx.getIndex64());
        }
        constexpr bool operator[] (uint8_t bitIdx) const{
            return Utils::getBit(bits, bitIdx);
        }

        constexpr bool hasPieces() const{
            return bits;
        }

        constexpr Bitboard operator | (Bitboard const& other){
            return {this->bits | other.bits};
        }
        constexpr Bitboard operator & (Bitboard const& other){
            return {this->bits & other.bits};
        }
        constexpr Bitboard operator ^ (Bitboard const& other){
            return {this->bits ^ other.bits};
        }

    private:
        constexpr void flipBit(uint8_t bitIndex){
            bits ^= Utils::binardOneAt<uint64_t>(bitIndex);
        }

        constexpr void setBit(uint8_t bitIndex){
            bits |= Utils::binardOneAt<uint64_t>(bitIndex);
        }
        constexpr void clearBit(uint8_t bitIndex){
            bits &= ~ Utils::binardOneAt<uint64_t>(bitIndex);
        }


        constexpr Reference operator[] (Coordinate bitIdx){
            return Reference(*this, bitIdx.getIndex64());
        }
        constexpr Reference operator[] (uint8_t bitIdx){
            return Reference(*this, bitIdx);
        }

        /**
         * bitScanForward
         * @author Matt Taylor (2003)
         * @param bb bitboard to scan
         * @precondition bb != 0
         * @return index (0..63) of least significant one bit
         */
        static constexpr uint8_t bitScanForward(uint64_t bb) {
            constexpr std::array<uint8_t, 64> lsb_64_table ={
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
        uint64_t bits;

        // these are only used in debug builds to aid in debugging
        std::array<uint8_t, 64> occupiedSquares;
        std::array<int, 10*12> reverseOccupiedSquares;
        int numPieces = 0;
};

}