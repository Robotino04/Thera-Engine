#pragma once

#include <concepts>

namespace ChessBot::Utils{

template<typename T>
[[nodiscard]] constexpr T setBit(T value, int bit, T bitValue){
   return (value & ~(T(1) << bit)) | ((bitValue & T(1)) << bit);
}
template<typename T>
[[nodiscard]] constexpr T getBit(T value, int bit){
    return (value >> bit) & T(1);
}

template<typename T>
[[nodiscard]] constexpr T binaryOnes(int numOnes){
    T result = T(0);
    for (int i=0; i<numOnes; i++){
        result = (result | T(1)) << 1;
    }
    return result;
}

template <std::unsigned_integral T>
[[nodiscard]] constexpr T reverseBits(T n) {
    short bits = sizeof(n) * 8; 
    T mask = ~T(0); // all ones
    
    while (bits >>= 1) {
        mask ^= mask << (bits); // will convert mask to 0b00000000000000001111111111111111;
        n = (n & ~mask) >> bits | (n & mask) << bits; // divide and conquer
    }

    return n;
}

}