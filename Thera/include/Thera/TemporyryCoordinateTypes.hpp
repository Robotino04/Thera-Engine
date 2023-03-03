#pragma once
// #warning "This file is temporary and shouldn't be needed in the end"

#include "Thera/Utils/Math.hpp"
#include "Thera/Utils/Bits.hpp"

#include <stdexcept>
#include <cstdint>
#include <array>
#include <type_traits>

namespace Thera{

struct Coordinate8x8{
    uint8_t pos = 0;

    constexpr explicit Coordinate8x8(uint8_t x){
        if (!Utils::isInRange<uint8_t>(x, 0, 63))
            throw std::out_of_range(std::to_string(x) + " isn't a valid 8x8 square");

        pos = x;
    }
    constexpr explicit Coordinate8x8(uint8_t x, uint8_t y){
        pos = x + y * 8;
        if (!Utils::isInRange<uint8_t>(pos, 0, 63))
            throw std::out_of_range(std::to_string(x) + "x" + std::to_string(y) + " isn't a valid 8x8 square");
    }
    constexpr Coordinate8x8(){
        pos = 0;
    }

    constexpr bool operator == (Coordinate8x8 const& other) const{
        return this->pos == other.pos;
    }
    constexpr bool operator == (auto const& other) const;


    constexpr operator auto() const;
};

// implements 0x88
struct PossiblyOffTheBoardCoordinate{
    uint8_t x: 4;
    uint8_t y: 4;

    constexpr explicit PossiblyOffTheBoardCoordinate(uint8_t pos){
        x = pos & Utils::binaryOnes<uint8_t>(4);
        y = (pos >> 4) & Utils::binaryOnes<uint8_t>(4);
    }
    constexpr explicit PossiblyOffTheBoardCoordinate(uint8_t x, uint8_t y){
        this->x = x;
        this->y = y;
    }
    constexpr PossiblyOffTheBoardCoordinate(){
        x = 0;
        y = 0;
    }
    
    constexpr uint8_t getRaw() const{
        return x | y << 4;
    }

    constexpr operator Coordinate8x8() const{
        return Coordinate8x8(x & 0b111, y & 0b111);
    }

    constexpr bool operator == (PossiblyOffTheBoardCoordinate const& other) const{
        return this->getRaw() == other.getRaw();
    }
    constexpr bool operator == (Coordinate8x8 const& other) const;
};


constexpr Coordinate8x8::operator auto() const{
    return PossiblyOffTheBoardCoordinate(pos + (pos & ~7));
}
constexpr bool Coordinate8x8::operator == (auto const& other) const{
    static_assert(std::is_same_v<decltype(other), PossiblyOffTheBoardCoordinate>, "Can only compare to itself and PossiblyOffTheBoardCoordinate");
    return PossiblyOffTheBoardCoordinate(*this) == other;
}


constexpr bool PossiblyOffTheBoardCoordinate::operator == (Coordinate8x8 const& other) const{
    return *this == PossiblyOffTheBoardCoordinate(other);
}

}