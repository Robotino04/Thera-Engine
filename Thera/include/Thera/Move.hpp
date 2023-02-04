#pragma once

#include <stdint.h>
#include <string>

#include "Thera/Piece.hpp"

namespace Thera{

struct Move{
    Move(int8_t start, int8_t end): startIndex(start), endIndex(end), auxiliaryMove(nullptr){}
    Move(): startIndex(0), endIndex(0), auxiliaryMove(nullptr){}
    Move(Move const& move){
        startIndex = move.startIndex;
        endIndex = move.endIndex;
        promotionType = move.promotionType;
        enPassantFile = move.enPassantFile;
        isEnPassant = move.isEnPassant;
        isCastling = move.isCastling;
        isDoublePawnMove = move.isDoublePawnMove;

        if (move.auxiliaryMove)
            auxiliaryMove = new Move(*move.auxiliaryMove);
    }
    ~Move(){
        if (auxiliaryMove) delete auxiliaryMove;
    }
    int8_t startIndex, endIndex;
    Move* auxiliaryMove = nullptr;
    PieceType promotionType = PieceType::None;
    int8_t enPassantFile = 0;
    bool isEnPassant = false;
    bool isCastling = false;
    bool isDoublePawnMove = false;

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