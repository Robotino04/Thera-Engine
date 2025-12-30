#pragma once

#include <concepts>

namespace Thera::Utils {


template <std::unsigned_integral T>
constexpr T binaryOneAt(int i) {
    return T(1) << i;
}

template <std::unsigned_integral T>
constexpr T setBit(T value, int bit, bool bitValue) {
    return (value & ~binaryOneAt<T>(bit)) // clear the bit to be changed
         | ((T(bitValue)) << bit);
}
template <std::unsigned_integral T>
constexpr T getBit(T value, int bit) {
    return (value >> bit) & T(1);
}

template <std::unsigned_integral T>
constexpr T binaryOnes(int numOnes) {
    T result = T(0);
    for (int i = 0; i < numOnes; i++) {
        result <<= 1;
        result |= T(1);
    }
    return result;
}

template <std::unsigned_integral T>
constexpr T reverseBits(T n) {
    short bits = sizeof(n) * 8;
    T mask = ~T(0); // all ones

    while (bits >>= 1) {
        mask ^= mask << (bits); // will convert mask to 0b00000000000000001111111111111111;
        n = (n & ~mask) >> bits | (n & mask) << bits; // divide and conquer
    }

    return n;
}

}
