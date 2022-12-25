#pragma once

#include <stdint.h>
#include <string>

namespace ChessBot{

struct Move{
    Move(int8_t start, int8_t end): startIndex(start), endIndex(end){}
    Move(): startIndex(0), endIndex(0){}
    int8_t startIndex, endIndex;

    /**
     * @brief Parse a string as a move.
     * 
     * A valid string consists of two sets of squares.
     * Examples:
     *  "e1e4"
     *  "h7d4"
     * 
     * @param str the string to parse from
     */
    void fromString(std::string const& str);
};

}