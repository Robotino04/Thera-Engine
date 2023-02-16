#pragma once

#include <utility>

namespace Thera::Utils{

/**
 * @brief Test if x is in the range [begin, end] 
 *  
 * @tparam T the type of value to test
 * @param x the value to test
 * @param begin begin of range (inclusive)
 * @param end end of range (inclusive)
 * @return bool is x in the range [begin, end]
 */
template<typename T>
constexpr bool isInRange(T x, T begin, T end){
    if (begin > end) std::swap(begin, end);

    return begin <= x  && x <= end;
}


/**
 * @brief Return the sign (1, 0, -1).
 * 
 * @tparam T the type of value to operate on
 * @param x the value
 * @return T the sign of x
 */
template<typename T>
constexpr T sign(T x){
    if (x > T(0)) return T(1);
    if (x < T(0)) return T(-1);
    return T(0);
}

}