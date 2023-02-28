#pragma once
// #warning "This file is temporary and shouldn't be needed in the end"

#include "Thera/Utils/Math.hpp"

#include <stdexcept>
#include <stdint.h>
#include <array>
#include <type_traits>

namespace Thera{

namespace Detail{
	// converts from 10x12 to 8x8
    constexpr std::array<int, 10*12> mailboxBigToSmall = [](){
		std::array<int, 10*12> result = {};
		for (auto& x : result) x = -1;

		for (int x=0; x<8; x++){
			for (int y=0; y<8; y++){
				result[(x+1) + (y+2) * 10] = x + y * 8;
			}
		}
		return result;
	}();
    
    // converts from 8x8 to 10x12
    constexpr std::array<int, 8*8> mailboxSmallToBig = [](){
		std::array<int, 8*8> result = {};
		for (auto& x : result) x = -1;

		for (int x=0; x<8; x++){
			for (int y=0; y<8; y++){
				result[x + y * 8] = (x+1) + (y+2) * 10;
			}
		}
		return result;
	}();
}


struct Coordinate8x8{
    uint8_t pos = 0;

    constexpr explicit Coordinate8x8(uint8_t x){
        try{
            Detail::mailboxSmallToBig.at(x);
        }
        catch (std::out_of_range const&){
            throw std::invalid_argument(std::to_string(x) + " isn't a valid 8x8 square");
        }
        pos = x;
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
    int8_t pos = 0;

    constexpr explicit PossiblyOffTheBoardCoordinate(int8_t x){
        try{
            Detail::mailboxBigToSmall.at(x);
        }
        catch (std::out_of_range const&){
            throw std::invalid_argument(std::to_string(x) + " isn't a valid 10x12 square");
        }
        pos = x;
    }
    constexpr PossiblyOffTheBoardCoordinate(){
        pos = 0;
    }

    constexpr operator Coordinate8x8() const{
        return Coordinate8x8(Detail::mailboxBigToSmall.at(pos));
    }

    constexpr bool operator == (PossiblyOffTheBoardCoordinate const& other) const{
        return this->pos == other.pos;
    }
    constexpr bool operator == (Coordinate8x8 const& other) const;
};


constexpr Coordinate8x8::operator auto() const{
    return PossiblyOffTheBoardCoordinate(Detail::mailboxSmallToBig.at(pos));
}
constexpr bool Coordinate8x8::operator == (auto const& other) const{
    static_assert(std::is_same_v<decltype(other), PossiblyOffTheBoardCoordinate>, "Can only compare to itself and PossiblyOffTheBoardCoordinate");
    return PossiblyOffTheBoardCoordinate(*this) == other;
}


constexpr bool PossiblyOffTheBoardCoordinate::operator == (Coordinate8x8 const& other) const{
    return *this == PossiblyOffTheBoardCoordinate(other);
}

}