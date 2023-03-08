#pragma once

#include <cstdint>
#include <string>

#include "Thera/Piece.hpp"
#include "Thera/Coordinate.hpp"

#include "Thera/Utils/BuildType.hpp"

namespace Thera{

struct Move{
    constexpr Move(Coordinate start, Coordinate end): startIndex(start), endIndex(end){
        debugValidate();
    }
    constexpr Move(){}
    Coordinate startIndex, endIndex;

    PieceType promotionType = PieceType::None;

    bool isEnPassant = false;

    bool isCastling = false;
    Coordinate castlingStart, castlingEnd;
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

    constexpr bool operator ==(Move const& other) const{
        bool eq = Move::isSameBaseMove(*this, other);
        if (this->isCastling && other.isCastling && this->castlingStart == other.castlingStart && this->castlingEnd == other.castlingEnd)
            eq &= true; 
        return eq;
    }
    
    /**
     * @brief Validate that the move only contains squares on the board.
     * 
     * Should reduce to nop in release builds
     */
    constexpr void debugValidate() const{
        if (Utils::BuildType::Current == Utils::BuildType::Debug){
            if (!startIndex.isOnBoard()) throw std::invalid_argument(std::to_string(startIndex.x) + ";" + std::to_string(startIndex.y) + " isn't on the board (start square)");
            if (!endIndex.isOnBoard()) throw std::invalid_argument(std::to_string(endIndex.x) + ";" + std::to_string(endIndex.y) + " isn't on the board (end square)");
        
            if (isCastling){
                Move(castlingStart, castlingEnd).debugValidate();
            }
        }
    }

    constexpr static bool isSameBaseMove(Move const& a, Move const& b){
        a.debugValidate();
        b.debugValidate();

        return  a.startIndex.getRaw() == b.startIndex.getRaw() &&
                a.endIndex.getRaw() == b.endIndex.getRaw() &&
                a.promotionType == b.promotionType;
    }
};

}