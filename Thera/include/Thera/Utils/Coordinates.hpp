#pragma once

#include "Thera/Utils/Math.hpp"

#include <stdint.h>
#include <utility>
#include <array>

namespace Thera::Utils{


/**
 * @brief Get the x coordinate of an 8x8 square index.
 * 
 * @param square 
 * @return constexpr int8_t 
 */
constexpr int8_t getXCoord(int8_t square){
    return square % 8;
}

/**
 * @brief Get the y coordinate of an 8x8 square index.
 * 
 * @param square 
 * @return constexpr int8_t 
 */
constexpr int8_t getYCoord(int8_t square){
    return square / 8;
}

/**
 * @brief Get the x, y coordinates of an 8x8 square index.
 * 
 * @param square 
 * @return constexpr std::pair<int8_t, int8_t> 
 */
constexpr std::pair<int8_t, int8_t> getCoords(int8_t square){
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
 * @param width the grid width
 * @return int8_t index
 */
constexpr int8_t coordToIndex(int8_t x, int8_t y, int8_t width=8){
	return x + y * width;
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
constexpr bool isOnRow(int8_t square, int8_t row){
    return isInRange(square, coordToIndex(0, row), coordToIndex(7, row));
}

namespace Detail{
	constexpr std::array<int, 10*12> generateMailboxBigToSmall(){
		std::array<int, 10*12> result = {};
		for (auto& x : result) x = -1;

		for (int x=0; x<8; x++){
			for (int y=0; y<8; y++){
				result[Utils::coordToIndex(x+1, y+2, 10)] = Utils::coordToIndex(x, y);
			}
		}
		return result;
	}
	constexpr std::array<int, 8*8> generateMailboxSmallToBig(){
		std::array<int, 8*8> result = {};
		for (auto& x : result) x = -1;

		for (int x=0; x<8; x++){
			for (int y=0; y<8; y++){
				result[Utils::coordToIndex(x, y)] = Utils::coordToIndex(x+1, y+2, 10);
			}
		}
		return result;
	}

    // converts from 10x12 to 8x8
    static constexpr std::array<int, 10*12> mailboxBigToSmall = Detail::generateMailboxBigToSmall();
    // converts from 8x8 to 10x12
    static constexpr std::array<int, 8*8> mailboxSmallToBig = Detail::generateMailboxSmallToBig();
}
		
/**
 * @brief Return if given index(10x12) and offset(10x12) are still on the board.
 * 
 * @param index the base index
 * @param offset the offset
 * @return bool
 */
static constexpr bool isOnBoard10x12(int8_t index, int8_t offset=0) {
    return Detail::mailboxBigToSmall.at(index + offset) != -1;
}

/**
 * @brief Apply a 10x12 offset to 8x8 coordinates.
 * 
 * @param index the base index
 * @param offset the 10x12 offset
 * @return constexpr int8_t the new 8x8 coordinates
 */
static constexpr int8_t applyOffset(int8_t index, int8_t offset){
    return Detail::mailboxBigToSmall.at(Detail::mailboxSmallToBig.at(index) + offset);
}

static constexpr int8_t to8x8Coords(int8_t index){
    return Detail::mailboxBigToSmall.at(index);
}
static constexpr int8_t to10x12Coords(int8_t index){
    return Detail::mailboxSmallToBig.at(index);
}

/**
 * @brief Return if given index(8x8) and offset(10x12) are still on the board.
 * 
 * @param index the base index
 * @param offset the offset
 * @return bool
 */
static constexpr bool isOnBoard8x8(int8_t index, int8_t offset=0) {
    return applyOffset(index, offset) != -1;
}


}