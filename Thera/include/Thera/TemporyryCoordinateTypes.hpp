#pragma once
#warning "This file is temporary and shouldn't be needed in the end"

#include <stdexcept>
#include <stdint.h>
#include <array>

namespace Thera{

namespace _________Detail{
	constexpr std::array<int, 10*12> generateMailboxBigToSmall(){
		std::array<int, 10*12> result = {};
		for (auto& x : result) x = -1;

		for (int x=0; x<8; x++){
			for (int y=0; y<8; y++){
				result[(x+1) + (y+2) * 10] = x + y * 8;
			}
		}
		return result;
	}
	constexpr std::array<int, 8*8> generateMailboxSmallToBig(){
		std::array<int, 8*8> result = {};
		for (auto& x : result) x = -1;

		for (int x=0; x<8; x++){
			for (int y=0; y<8; y++){
				result[x + y * 8] = (x+1) + (y+2) * 10;
			}
		}
		return result;
	}

    // converts from 10x12 to 8x8
    static constexpr std::array<int, 10*12> mailboxBigToSmall = generateMailboxBigToSmall();
    // converts from 8x8 to 10x12
    static constexpr std::array<int, 8*8> mailboxSmallToBig = generateMailboxSmallToBig();
}


struct Coordinate8x8{
    int8_t pos = 0;

    constexpr explicit Coordinate8x8(int8_t x){
        try{
            _________Detail::mailboxSmallToBig.at(x);
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

struct Coordinate10x12{
    int8_t pos = 0;

    constexpr explicit Coordinate10x12(int8_t x){
        try{
            _________Detail::mailboxBigToSmall.at(x);
        }
        catch (std::out_of_range const&){
            throw std::invalid_argument(std::to_string(x) + " isn't a valid 10x12 square");
        }
        pos = x;
    }
    constexpr Coordinate10x12(){
        pos = 0;
    }

    constexpr operator Coordinate8x8() const{
        return Coordinate8x8(_________Detail::mailboxBigToSmall.at(pos));
    }

    constexpr bool operator == (Coordinate10x12 const& other) const{
        return this->pos == other.pos;
    }
    constexpr bool operator == (Coordinate8x8 const& other) const;
};


constexpr Coordinate8x8::operator auto() const{
    return Coordinate10x12(_________Detail::mailboxSmallToBig.at(pos));
}
constexpr bool Coordinate8x8::operator == (auto const& other) const{
    static_assert(std::is_same_v<decltype(other), Coordinate10x12>, "Can only compare to itself and Coordinate10x12");
    return Coordinate10x12(*this) == other;
}


constexpr bool Coordinate10x12::operator == (Coordinate8x8 const& other) const{
    return *this == Coordinate10x12(other);
}

}