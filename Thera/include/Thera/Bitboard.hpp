#pragma once

#include "Thera/Move.hpp"

#include "Thera/Utils/BuildType.hpp"
#include "Thera/Utils/Math.hpp"
#include "Thera/Coordinate.hpp"

#include <cstdint>
#include <stdexcept>
#include <bit>

namespace Thera {

class Board;

/**
 * @brief A bitboard stored in an uint64_t.
 */
class Bitboard {
    friend class Board;

public:
    constexpr Bitboard(): bits(0) {}

    constexpr Bitboard(uint64_t raw): bits(raw) {}


    static constexpr Bitboard fromIndex64(int idx) {
        return Utils::binaryOneAt<uint64_t>(idx);
    }

    /**
     * @brief Test if square is occupied. Only does bounds checking in debug builds.
     *
     * @param square
     * @return true
     * @return false
     */
    constexpr bool isOccupied(Coordinate square) const {
        if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug)
            if (!Utils::isInRange<uint8_t>(square.getIndex64(), 0, 63))
                throw std::out_of_range("Square index is outside the board");
        return Utils::getBit(bits, square.getIndex64());
    }

    /**
     * @brief Test if square is occupied. Only does bounds checking in debug builds.
     *
     * @param square
     * @return true
     * @return false
     */
    constexpr bool isOccupied(int square) const {
        if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug)
            if (!Utils::isInRange(square, 0, 63))
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
    constexpr void placePiece(Coordinate square) {
        if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug) {
            if (isOccupied(square))
                throw std::invalid_argument("Tried to place piece on already occupied square.");
        }

        setBit(square.getIndex64());
    }

    /**
     * @brief Blindly remove the piece from the board without any sort of test.
     *
     * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
     *
     * @param square
     */
    constexpr void removePiece(Coordinate square) {
        clearBit(square.getIndex64());
    }

    /**
     * @brief Blindly apply the base move without any sort of test.
     *
     * In debug builds there are some tests to ensure that numPiece doesn't get out of sync.
     *
     * @param move
     */
    constexpr void applyMove(Move move) {
        clearBit(move.startIndex.getIndex64()); // will remove the piece
        setBit(move.endIndex.getIndex64());     // will place the piece
    }

    constexpr int getNumPieces() const {
        return std::popcount(bits);
    }

    constexpr bool operator[](Coordinate bitIdx) const {
        return Utils::getBit(bits, bitIdx.getIndex64());
    }
    constexpr bool operator[](uint8_t bitIdx) const {
        return Utils::getBit(bits, bitIdx);
    }
    constexpr bool operator[](Coordinate bitIdx) {
        return Utils::getBit(bits, bitIdx.getIndex64());
    }
    constexpr bool operator[](uint8_t bitIdx) {
        return Utils::getBit(bits, bitIdx);
    }

    constexpr bool hasPieces() const {
        return bits;
    }

    // clang-format off
    constexpr Bitboard operator|(Bitboard other) const { return this->bits | other.bits; }
    constexpr Bitboard operator&(Bitboard other) const { return this->bits & other.bits; }
    constexpr Bitboard operator^(Bitboard other) const { return this->bits ^ other.bits; }
    constexpr Bitboard operator+(Bitboard other) const { return this->bits + other.bits; }
    constexpr Bitboard operator-(Bitboard other) const { return this->bits - other.bits; }
    constexpr Bitboard operator*(Bitboard other) const { return this->bits * other.bits; }
    constexpr Bitboard operator/(Bitboard other) const { return this->bits / other.bits; }
    // clang-format on

    constexpr Bitboard operator-() const {
        return -bits;
    }
    /**
     * @brief Shift to the left. Allows for negative shift amounts.
     *
     * @param n number of bits to shift by
     * @return Bitboard the shifted Bitboard
     */
    constexpr Bitboard operator<<(int n) const {
        return (n > 0) ? (this->bits << n) : (this->bits >> -n);
    }
    /**
     * @brief Shift to the right. Allows for negative shift amounts.
     *
     * @param n number of bits to shift by
     * @return Bitboard the shifted Bitboard
     */
    constexpr Bitboard operator>>(int n) const {
        return *this << -n;
    }

    constexpr Bitboard rotateLeft(int n) const {
        return std::rotl(bits, n);
    }
    constexpr Bitboard rotateRight(int n) const {
        return std::rotr(bits, n);
    }

    constexpr Bitboard operator~() const {
        return ~this->bits;
    }

    // clang-format off
    constexpr Bitboard operator|=(Bitboard other) { return this->bits |= other.bits; }
    constexpr Bitboard operator&=(Bitboard other) { return this->bits &= other.bits; }
    constexpr Bitboard operator^=(Bitboard other) { return this->bits ^= other.bits; }
    constexpr Bitboard operator+=(Bitboard other) { return this->bits += other.bits; }
    constexpr Bitboard operator-=(Bitboard other) { return this->bits -= other.bits; }
    constexpr Bitboard operator*=(Bitboard other) { return this->bits *= other.bits; }
    constexpr Bitboard operator/=(Bitboard other) { return this->bits /= other.bits; }
    constexpr Bitboard operator<<=(int n) { return this->bits = this->bits << n; }
    constexpr Bitboard operator>>=(int n) { return this->bits = this->bits >> n; }
    // clang-format on

    explicit constexpr operator uint64_t() const {
        return bits;
    }

    constexpr void flipBit(uint8_t bitIndex) {
        bits ^= Utils::binaryOneAt<uint64_t>(bitIndex);
    }

    constexpr void setBit(uint8_t bitIndex) {
        bits |= Utils::binaryOneAt<uint64_t>(bitIndex);
    }
    constexpr void clearBit(uint8_t bitIndex) {
        bits &= ~Utils::binaryOneAt<uint64_t>(bitIndex);
    }
    constexpr Bitboard flipped() const {
        return Bitboard(
            ((bits >> 8 * 0) & 0b11111111) << 8 * 7 | ((bits >> 8 * 1) & 0b11111111) << 8 * 6
            | ((bits >> 8 * 2) & 0b11111111) << 8 * 5 | ((bits >> 8 * 3) & 0b11111111) << 8 * 4
            | ((bits >> 8 * 4) & 0b11111111) << 8 * 3 | ((bits >> 8 * 5) & 0b11111111) << 8 * 2
            | ((bits >> 8 * 6) & 0b11111111) << 8 * 1 | ((bits >> 8 * 7) & 0b11111111) << 8 * 0
        );
    }

    constexpr uint8_t getLS1B() const {
        return std::countr_zero(bits);
    }

    constexpr void clearLS1B() {
        bits &= (bits - 1);
    }

private:
    uint64_t bits;
};

namespace SquareBitboard {
#define defSq(NAME) constexpr Bitboard NAME = Bitboard::fromIndex64(SquareIndex64::NAME);
#define defRow(NAME)                                                                                         \
    defSq(NAME##1) defSq(NAME##2) defSq(NAME##3) defSq(NAME##4) defSq(NAME##5) defSq(NAME##6) defSq(NAME##7) \
        defSq(NAME##8)

    defRow(a);
    defRow(b);
    defRow(c);
    defRow(d);
    defRow(e);
    defRow(f);
    defRow(g);
    defRow(h);

#undef defSq
#undef defRow
}

}
