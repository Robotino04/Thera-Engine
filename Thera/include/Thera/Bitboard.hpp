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

        constexpr Bitboard(): bits(0){}

        constexpr Bitboard(uint64_t raw): bits(raw){}

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
                if (!square.isOnBoard())
                    throw std::invalid_argument("Tried to remove piece from outside the board.");
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
            if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
                if (!isOccupied(move.startIndex)){
                    throw std::runtime_error("Tried to make move starting on an empty square.");
                }
                if (!move.startIndex.isOnBoard())
                    throw std::out_of_range("Move start index is outside the board");
                if (!move.endIndex.isOnBoard())
                    throw std::out_of_range("Move end index is outside the board");
            }

            clearBit(move.startIndex.getIndex64()); // will remove the piece
            setBit(move.endIndex.getIndex64()); // will place the piece
        }

        constexpr int getNumPieces() const{
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

        constexpr Bitboard operator | (Bitboard const& other) const{
            return {this->bits | other.bits};
        }
        constexpr Bitboard operator & (Bitboard const& other) const{
            return {this->bits & other.bits};
        }
        constexpr Bitboard operator ^ (Bitboard const& other) const{
            return {this->bits ^ other.bits};
        }
        constexpr Bitboard operator << (int n) const{
            return {this->bits << n};
        }
        constexpr Bitboard operator >> (int n) const{
            return {this->bits >> n};
        }
        constexpr Bitboard operator ~ () const{
            return {~this->bits};
        }

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

        static constexpr uint8_t getLS1B(uint64_t bb) {
            return std::countr_zero(bb);
        }
        constexpr uint8_t getLS1B() const {
            return getLS1B(bits);
        }
        static constexpr uint64_t clearLS1B(uint64_t x){
            return x & (x-1);
        }

        constexpr void clearLS1B(){
            bits = clearLS1B(bits);
        }
    
    private:
        uint64_t bits;
};

}