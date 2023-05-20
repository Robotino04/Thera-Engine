#pragma once

#include <cstdint>
#include <string>

#include "Thera/Piece.hpp"
#include "Thera/Coordinate.hpp"

#include "Thera/Utils/BuildType.hpp"

namespace Thera{

struct Move{
    constexpr Move(Coordinate start, Coordinate end): startIndex(start), endIndex(end){
    }
    constexpr Move(Coordinate start, Coordinate end, Piece piece): startIndex(start), endIndex(end), piece(piece){
    }
    constexpr Move(){}
    Coordinate startIndex, endIndex;
    Piece piece;

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
     * @return Move the parsed move
     */
    static Move fromString(std::string const& str);

    /**
     * @brief Convert the move to long algebraic notation
     * 
     * @return std::string the move in long algebraic notation
     */
    std::string toString() const;

    constexpr bool operator ==(Move const& other) const{
        bool eq = Move::isSameBaseMove(*this, other);
        if (this->isCastling && other.isCastling)
            eq &= this->castlingStart == other.castlingStart && this->castlingEnd == other.castlingEnd; 
        return eq;
    }

    /**
     * @brief Compare moves for less than or equal. Only used for sorting.
     * 
     * @param other the other move to compare to 
     * @return 
     */
    constexpr bool operator < (auto const& other) const{
		if (this->startIndex != other.startIndex)
			return this->startIndex < other.startIndex;
		if (this->endIndex != other.endIndex)
			return this->endIndex < other.endIndex;
		if (this->promotionType != other.promotionType)
			return static_cast<int>(this->promotionType) < static_cast<int>(other.promotionType);
		
		return false;
	}

    constexpr static bool isSameBaseMove(Move a, Move b){
        return  a.startIndex == b.startIndex &&
                a.endIndex == b.endIndex &&
                a.promotionType == b.promotionType;
    }
};

}