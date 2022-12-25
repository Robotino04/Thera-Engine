#include "ChessBot/Utils.hpp"

#include <utility>
#include <stdexcept>

namespace ChessBot::Utils{

int8_t squareFromString(std::string const& str){
    if (str.size() != 2) throw std::invalid_argument("\"" + str + "\" isn't a valid square");
    if (!Utils::isInRange(str[0], 'a', 'h')) throw std::invalid_argument("\"" + str + "\" isn't a valid square");
    if (!Utils::isInRange(str[1], '1', '8')) throw std::invalid_argument("\"" + str + "\" isn't a valid square");

    // The string should be valid now
    int8_t x, y;
    x = str[0] - 'a';
    y = '8' - str[1];
    return Utils::coordToIndex(x, y);
}

}