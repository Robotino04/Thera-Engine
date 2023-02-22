#pragma once

#include <stdint.h>
#include <string>

#include "Thera/Piece.hpp"
#include "Thera/TemporyryCoordinateTypes.hpp"

namespace Thera{

struct Move{
    Move(Coordinate8x8 start, Coordinate8x8 end): startIndex(start), endIndex(end), auxiliaryMove(nullptr){}
    Move(): startIndex(), endIndex(), auxiliaryMove(nullptr){}
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
    Coordinate8x8 startIndex, endIndex;
    Move* auxiliaryMove = nullptr;
    PieceType promotionType = PieceType::None;
    int8_t enPassantFile;
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