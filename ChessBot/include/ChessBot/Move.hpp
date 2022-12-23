#pragma once

#include <stdint.h>
#include <string>

namespace ChessBot{

struct Move{
    uint8_t startIndex, endIndex;

    /**
     * @brief Parse a string as a move.
     * 
     * A valid string consists of two sets of squares.
     * Examples:
     *  "e1e4"
     *  "h7d4"
     * 
     * @param str the string to parse from
     * @return true the string was correctly parsed
     * @return false the string was invalid
     */
    bool fromString(std::string const& str);
};

}