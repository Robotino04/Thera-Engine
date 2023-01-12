#pragma once

namespace ChessBot::Utils{

template<typename T>
[[nodiscard]] constexpr T setBit(T value, int bit, T bitValue){
   return (value & ~(T(1) << bit)) | ((bitValue & T(1)) << bit);
}
template<typename T>
[[nodiscard]] constexpr T getBit(T value, int bit){
    return (value >> bit) & T(1);
}

}