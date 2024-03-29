#include "Thera/Utils/ChessTerms.hpp"

namespace Thera::Utils{

Coordinate squareFromAlgebraicNotation(std::string const& str){
    if (str.size() != 2) throw std::invalid_argument("\"" + str + "\" isn't a valid square");
    if (!Utils::isInRange(str[0], 'a', 'h')) throw std::invalid_argument("\"" + str + "\" isn't a valid square");
    if (!Utils::isInRange(str[1], '1', '8')) throw std::invalid_argument("\"" + str + "\" isn't a valid square");

    // The string should be valid now
    uint8_t x, y;
    x = str[0] - 'a';
    y = str[1] - '1';
    return Coordinate(x, y);
}

std::string squareToAlgebraicNotation(Coordinate square){
    if (!Utils::isInRange<uint8_t>(square.x, 0, 7) || !Utils::isInRange<uint8_t>(square.y, 0, 7)) throw std::invalid_argument(std::to_string(square.x) + ";" + std::to_string(square.y) + " isn't a valid square");

    return std::string(1, 'a' + square.x) + std::string(1, square.y + '1');
}

std::string pieceColorToString(PieceColor color){
    switch(color){
        case PieceColor::White: return "white";
        case PieceColor::Black: return "black";
        default:                return "invalid color"; // failsafe since Piece does static_cast<enum>(int)
    }
}

std::string pieceTypeToString(PieceType type, bool isPlural){
    const std::string ending = isPlural ? "s" : "";
    switch(type){
        case PieceType::Pawn:   return std::string("pawn")   + ending;
        case PieceType::Bishop: return std::string("bishop") + ending;
        case PieceType::Knight: return std::string("knight") + ending;
        case PieceType::Rook:   return std::string("rook")   + ending;
        case PieceType::Queen:  return std::string("queen")  + ending;
        case PieceType::King:   return std::string("king")   + ending;
        default:                return "invalid piece "; // failsafe since Piece does static_cast<enum>(int)
    }
}

std::string pieceToString(Piece piece, bool isPlural){
    return pieceColorToString(piece.color) + " " + pieceTypeToString(piece.type, isPlural);
}

}