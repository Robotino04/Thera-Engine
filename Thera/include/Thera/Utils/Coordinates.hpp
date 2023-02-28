#pragma once

#include "Thera/Utils/Math.hpp"

#include "Thera/TemporyryCoordinateTypes.hpp"

#include <stdint.h>
#include <utility>
#include <array>

namespace Thera::Utils{

/**
 * @brief Get the x coordinate of an 8x8 square index.
 * 
 * @param square 
 * @return constexpr uint8_t 
 */
constexpr uint8_t getXCoord(Coordinate8x8 square){
    return square.pos % 8;
}

/**
 * @brief Get the y coordinate of an 8x8 square index.
 * 
 * @param square 
 * @return constexpr uint8_t 
 */
constexpr uint8_t getYCoord(Coordinate8x8 square){
    return square.pos / 8;
}

/**
 * @brief Get the x, y coordinates of an 8x8 square index.
 * 
 * @param square 
 * @return constexpr std::pair<uint8_t, uint8_t> 
 */
constexpr std::pair<uint8_t, uint8_t> getCoords(Coordinate8x8 square){
    return {
        getXCoord(square),
        getYCoord(square)
    };
}

/**
 * @brief Convert x, y to an array index.
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @return Coordinate8x8
 */
constexpr Coordinate8x8 coordToIndex(uint8_t x, uint8_t y){
	return Coordinate8x8(x + y * 8);
}

/**
 * @brief Return whether a square(8x8) is on a given row.
 * 
 * Rows are counted from the bottom (white side) up starting at 0.
 * 
 * @param square the square to test
 * @param row the row
 * @return is the square on the given row
 */
constexpr bool isOnRow(Coordinate8x8 square, uint8_t row){
    return isInRange<uint8_t>(square.pos, coordToIndex(0, row).pos, coordToIndex(7, row).pos);
}

/**
 * @brief Apply a 10x12 offset to 10x12 coordinates.
 * 
 * @param index the base index
 * @param offset the 10x12 offset
 * @return constexpr PossiblyOffTheBoardCoordinate the new 10x12 coordinates
 */
constexpr PossiblyOffTheBoardCoordinate applyOffset(PossiblyOffTheBoardCoordinate index, int8_t offset){
    return PossiblyOffTheBoardCoordinate(index.pos + offset);
}

/**
 * @brief Return if given index(10x12) and offset(10x12) are still on the board.
 * 
 * @param index the base index
 * @param offset the offset
 * @return bool
 */
constexpr bool isOnBoard(PossiblyOffTheBoardCoordinate index, int8_t offset=0) {
    return Detail::mailboxBigToSmall.at(index.pos + offset) != -1;
}

}