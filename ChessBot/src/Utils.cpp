#include "ChessBot/Utils.hpp"

#include <utility>

namespace ChessBot::Utils{

bool isCharInRange(char x, char begin, char end){
    if (begin > end) std::swap(begin, end);

    return begin <= x  && x <= end;
}

uint8_t coordToIndex(uint8_t x, uint8_t y){
	return x + y * 8;
}

}