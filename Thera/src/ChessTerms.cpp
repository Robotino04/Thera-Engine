#include "Thera/Utils/ChessTerms.hpp"

namespace Thera::Utils{

Coordinate8x8 squareFromAlgebraicNotation(std::string const& str){
    if (str.size() != 2) throw std::invalid_argument("\"" + str + "\" isn't a valid square");
    if (!Utils::isInRange(str[0], 'a', 'h')) throw std::invalid_argument("\"" + str + "\" isn't a valid square");
    if (!Utils::isInRange(str[1], '1', '8')) throw std::invalid_argument("\"" + str + "\" isn't a valid square");

    // The string should be valid now
    int8_t x, y;
    x = str[0] - 'a';
    y = '8' - str[1];
    return Utils::coordToIndex(x, y);
}

std::string squareToAlgebraicNotation(Coordinate8x8 square){
    auto [x, y] = Utils::getCoords(square);
    if (!Utils::isInRange<int8_t>(x, 0, 7) || !Utils::isInRange<int8_t>(y, 0, 7)) throw std::invalid_argument(std::to_string(square.pos) + " isn't a valid square");

    return std::string(1, 'a' + x) + std::string(1, '8' - y);
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
    return pieceColorToString(piece.getColor()) + " " + pieceTypeToString(piece.getType(), isPlural);
}

}