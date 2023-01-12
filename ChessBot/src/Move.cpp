#include "ChessBot/Move.hpp"
#include "ChessBot/Utils/ChessTerms.hpp"

#include <stdexcept>

namespace ChessBot{

void Move::fromString(std::string const& str){
    if (str.size() != 4) throw std::invalid_argument("\"" + str + "\" isn't a valid square");
    startIndex = Utils::squareFromAlgebraicNotation(str.substr(0, 2));
    endIndex = Utils::squareFromAlgebraicNotation(str.substr(2, 2));
}

bool Move::operator ==(Move const& other) const{
    bool eq = Move::isSameBaseMove(*this, other);
    if (this->auxiliaryMove && other.auxiliaryMove && *this->auxiliaryMove == *other.auxiliaryMove)
        eq &= true; 
    return eq;
}
bool Move::isSameBaseMove(Move const& a, Move const& b){
    return  a.startIndex == b.startIndex &&
            a.endIndex == b.endIndex;
}

}