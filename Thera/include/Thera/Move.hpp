#pragma once

#include <cstdint>
#include <string>

#include "Thera/Piece.hpp"
#include "Thera/Coordinate.hpp"

#include "Thera/Utils/BuildType.hpp"

namespace Thera{

struct Move{
    constexpr Move(Coordinate start, Coordinate end): startIndex(start), endIndex(end), auxiliaryMove(nullptr){
        debugValidate();
    }
    constexpr Move(): startIndex(), endIndex(), auxiliaryMove(nullptr){}
    constexpr Move(Move const& move){
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
    constexpr ~Move(){
        if (auxiliaryMove) delete auxiliaryMove;
    }
    Coordinate startIndex, endIndex;
    Move* auxiliaryMove = nullptr;
    PieceType promotionType = PieceType::None;
    uint8_t enPassantFile;
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

    constexpr bool operator ==(Move const& other) const{
        bool eq = Move::isSameBaseMove(*this, other);
        if (this->auxiliaryMove && other.auxiliaryMove && *this->auxiliaryMove == *other.auxiliaryMove)
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
            if (!startIndex.isOnBoard()) throw std::invalid_argument(std::to_string(startIndex.x) + ";" + std::to_string(startIndex.y) + " isn't on the board");
            if (!endIndex.isOnBoard()) throw std::invalid_argument(std::to_string(endIndex.x) + ";" + std::to_string(endIndex.y) + " isn't on the board");
        
            if (auxiliaryMove){
                auxiliaryMove->debugValidate();
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