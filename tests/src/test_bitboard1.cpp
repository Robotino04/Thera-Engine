#include "Thera/Bitboard.hpp"
#include "ANSI/ANSI.hpp"

#include <iostream>
#include <optional>

template<int SIZE, int LIMIT>
std::optional<int> recursiveTestBitboardOfSizeWithValue(const __uint128_t value, const int expected){
    if constexpr(SIZE == LIMIT-1){
        return std::optional<int>();
    }
    else{
        const Thera::Bitboard<SIZE> board(value);

        if (board.hasPieces()){
            std::optional<int> opt;
            opt = SIZE;
            return opt;
        }
        else{
            return recursiveTestBitboardOfSizeWithValue<SIZE-1, LIMIT>(value, expected);
        }
    }
};

int main(){
    const auto errorSize = recursiveTestBitboardOfSizeWithValue<64, 0>(0, 0);
    
    if (errorSize.has_value()){
        std::cout << ANSI::set4BitColor(ANSI::Red) << "Bitboard of size " << errorSize.value() << " wasn't empty. ✗";
        return 1;
    }
    else{
        std::cout << ANSI::set4BitColor(ANSI::Green) << "Every bitboard was empty. ✓" << ANSI::reset() << "\n";
        return 0;
    }
}