#pragma once

#include <stdint.h>
#include <string>

namespace ChessBot{

struct Move{
    Move(int8_t start, int8_t end): startIndex(start), endIndex(end), auxiliaryMove(nullptr){}
    Move(): startIndex(0), endIndex(0), auxiliaryMove(nullptr){}
    Move(Move const& move): startIndex(move.startIndex), endIndex(move.endIndex){
        if (move.auxiliaryMove)
            auxiliaryMove = new Move(*move.auxiliaryMove);
    }
    ~Move(){
        if (auxiliaryMove) delete auxiliaryMove;
    }
    int8_t startIndex, endIndex;
    Move* auxiliaryMove = nullptr;

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

    bool operator ==(Move const& other) const;

    static bool isSameBaseMove(Move const& a, Move const& b);
};

}