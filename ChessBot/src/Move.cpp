#include "ChessBot/Move.hpp"

#include "ChessBot/Utils.hpp"

namespace ChessBot{

namespace Detail{
    
}

bool Move::fromString(std::string const& str){
    if (str.size() != 4) return false;
    if (!Utils::isCharInRange(str[0], 'a', 'h')) return false;
    if (!Utils::isCharInRange(str[1], '1', '8')) return false;
    if (!Utils::isCharInRange(str[2], 'a', 'h')) return false;
    if (!Utils::isCharInRange(str[3], '1', '8')) return false;

    // The string should be valid now
    uint8_t x, y;
    x = str[0] - 'a';
    y = '8' - str[1];
    startIndex = Utils::coordToIndex(x, y);

    x = str[2] - 'a';
    y = '8' - str[3];
    endIndex = Utils::coordToIndex(x, y);
    return true;
}

}