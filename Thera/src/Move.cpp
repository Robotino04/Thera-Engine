#include "Thera/Move.hpp"
#include "Thera/Utils/ChessTerms.hpp"

#include <stdexcept>

namespace Thera{

void Move::fromString(std::string const& str){
    if (str.size() != 4) throw std::invalid_argument("\"" + str + "\" isn't a valid square");
    startIndex = Utils::squareFromAlgebraicNotation(str.substr(0, 2));
    endIndex = Utils::squareFromAlgebraicNotation(str.substr(2, 2));
}

}