#pragma once

#include <string>

namespace ChessBot::Utils{

inline const std::string startingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

/**
 * @brief Test if a char is in a range of chars.
 * 
 * @param x the char to test
 * @param begin begin of range (inclusive)
 * @param end end of range (inclusive)
 * @return bool is x in the range [begin, end]
 */
bool isCharInRange(char x, char begin, char end);

/**
 * @brief Convert x, y to an array index.
 * 
 * @param x X coordinate
 * @param y Y coordinate
 * @return uint8_t index
 */
uint8_t coordToIndex(uint8_t x, uint8_t y);

}
