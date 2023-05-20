#include "Thera/Move.hpp"
#include "Thera/Utils/ChessTerms.hpp"

#include <stdexcept>

namespace Thera{

Move Move::fromString(std::string const& str){
    Move move;
    if (!Utils::isInRange<int>(str.size(), 4, 5)) throw std::invalid_argument("\"" + str + "\" isn't a valid move");
    move.startIndex = Utils::squareFromAlgebraicNotation(str.substr(0, 2));
    move.endIndex = Utils::squareFromAlgebraicNotation(str.substr(2, 2));
    move.promotionType = PieceType::None;

    if (str.size() == 5){
        switch(std::tolower(str.at(4))){
            case 'n': move.promotionType = PieceType::Knight; break;
            case 'b': move.promotionType = PieceType::Bishop; break;
            case 'r': move.promotionType = PieceType::Rook; break;
            case 'q': move.promotionType = PieceType::Queen; break;
            default:
                throw std::invalid_argument("'" + str.substr(4,1) + "' isn't a valid promotion type!");
        }
    }
    return move;
}

std::string Move::toString() const{
    std::string result;
    result += Utils::squareToAlgebraicNotation(startIndex);
    result += Utils::squareToAlgebraicNotation(endIndex);
    switch (promotionType){
        case PieceType::Bishop: result +=  "b"; break;
        case PieceType::Knight: result +=  "n"; break;
        case PieceType::Rook:   result +=  "r"; break;
        case PieceType::Queen:  result +=  "q"; break;
        case PieceType::None:   break;
        default:                result +=  "[invalid promotion type]"; break;
    }
    return result;
}

}