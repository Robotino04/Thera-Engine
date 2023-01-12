#pragma once

#include "ChessBot/Utils/Math.hpp"

#include <stdint.h>

namespace ChessBot::Utils{


/**
 * @brief Get the x coordinate of a 8x8 square index.
 * 
 * @param square 
 * @return constexpr int8_t 
 */
constexpr int8_t getXCoord(int8_t square){
    return square % 8;
}

/**
 * @brief Get the y coordinate of a 8x8 square index.
 * 
 * @param square 
 * @return constexpr int8_t 
 */
constexpr int8_t getYCoord(int8_t square){
    return square / 8;
}

/**
 * @brief Convert x, y to an array index.
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @param width the grid width
 * @return int8_t index
 */
constexpr int8_t coordToIndex(int8_t x, int8_t y, int8_t width=8){
	return x + y * width;
}

/**
 * @brief Return whether a square is on a given row.
 * 
 * Rows are counted from the bottom (white side) up starting at 0.
 * 
 * @param square the square to test
 * @param row the row
 * @return is the square on the given row
 */
constexpr bool isOnRow(int8_t square, int8_t row){
    return isInRange(square, coordToIndex(0, row), coordToIndex(7, row));
}

}