#pragma once

#include "ChessBot/Piece.hpp"

#include <string>
#include <tuple>
#include <stdexcept>

namespace ChessBot::Utils{

inline const std::string startingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

/**
 * @brief Test if x is in the range [begin, end] 
 * 
 * @tparam T the type of value to test
 * @param x the value to test
 * @param begin begin of range (inclusive)
 * @param end end of range (inclusive)
 * @return bool is x in the range [begin, end]
 */
template<typename T>
constexpr bool isInRange(T x, T begin, T end){
    if (begin > end) std::swap(begin, end);

    return begin <= x  && x <= end;
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
 * @brief Return the opposite of the given color.
 * 
 * @param c the color to invert
 * @return PieceColor 
 */
constexpr PieceColor oppositeColor(PieceColor color){
	if(color == PieceColor::White) return PieceColor::Black;
	else if(color == PieceColor::Black) return PieceColor::White;
    else throw std::invalid_argument("Invalid value for PieceColor enum.");
}

/**
 * @brief Parse a square index from a string.
 * 
 * Examples:
 *  "e4"
 *  "h7"
 *  "f2"
 * 
 * @param str the string to parse from
 * @return int8_t the index
 */
int8_t squareFromString(std::string const& str);

}