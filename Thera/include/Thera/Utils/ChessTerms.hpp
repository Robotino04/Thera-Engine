#pragma once

#include "Thera/Piece.hpp"
#include "Thera/Utils/Math.hpp"
#include "Thera/Utils/Coordinates.hpp"

#include <string>
#include <array>
#include <stdexcept>
#include <string>

namespace Thera::Utils{

inline const std::string startingFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
inline constexpr std::array<PieceType, 4> promotionPieces = {PieceType::Bishop, PieceType::Knight, PieceType::Queen, PieceType::Rook};
inline constexpr std::array<PieceType, 6> allPieceTypes = {PieceType::Pawn, PieceType::Bishop, PieceType::Knight, PieceType::Rook, PieceType::Queen, PieceType::King};
inline constexpr std::array<PieceColor, 2> allPieceColors = {PieceColor::White, PieceColor::Black};

namespace Detail{
    constexpr auto generateAllPieces(){
        std::array<Piece, allPieceTypes.size() * allPieceColors.size()> result;
        int i=0;
        for (auto type : allPieceTypes){
            for (auto color : allPieceColors){
                result.at(i) = {type, color};
            }
        }
        return result;
    }
}

inline constexpr auto allPieces = Detail::generateAllPieces();


/**
 * @brief Parse a square index (8x8) from a string containing algebraic notation.
 * 
 * Examples:
 *  "e4"
 *  "h7"
 *  "f2"
 * 
 * @param str the string to parse from
 * @return Coordinate8x8
 */
Coordinate8x8 squareFromAlgebraicNotation(std::string const& str);

/**
 * @brief Generate a string containing algebraic notation for a square (8x8).
 * 
 * @param square 
 * @return std::string 
 */
std::string squareToAlgebraicNotation(Coordinate8x8 square);

/**
 * @brief Convert a piece color to a human readable string. ("white", "black")
 * 
 * @param color the color to convert
 * @return std::string 
 */
std::string pieceColorToString(PieceColor color);
/**
 * @brief Convert a piece type to a human readable string. ("pawn", "rook", etc.)
 * 
 * @param type the type to convert
 * @param isPlural should the type be in english plural
 * @return std::string 
 */
std::string pieceTypeToString(PieceType type, bool isPlural=false);

/**
 * @brief Convert a piece to a human readable string. ("white pawn", "black rook", etc.)
 * 
 * @param piece the piece to convert
 * @param isPlural should the piece be in english plural
 * @return std::string 
 */
std::string pieceToString(Piece piece, bool isPlural=false);

}