#pragma once
// #warning "This file is temporary and shouldn't be needed in the end"

#include "Thera/Utils/Math.hpp"
#include "Thera/Utils/Bits.hpp"

#include <stdexcept>
#include <cstdint>
#include <array>
#include <type_traits>

namespace Thera{

/**
 * @brief A position on the board stored using 0x88.
 * 
 * Allows for fast access to x, y and conversion to array indices. 
 * 
 */
struct Coordinate{
    uint8_t x: 4;
    uint8_t y: 4;

    constexpr explicit Coordinate(uint8_t pos){
        x = pos & Utils::binaryOnes<uint8_t>(4);
        y = (pos >> 4) & Utils::binaryOnes<uint8_t>(4);
    }
    constexpr explicit Coordinate(uint8_t x, uint8_t y){
        this->x = x;
        this->y = y;
    }
    constexpr Coordinate(){
        x = 0;
        y = 0;
    }

    /**
     * @brief Create a Coordinate from an index.
     * 
     * @param index index into an array of size 64
     * @return constexpr Coordinate 
     */
    static constexpr Coordinate fromIndex64(uint8_t index){
        return Coordinate(index + (index & ~7));
    }
    
    constexpr uint8_t getRaw() const{
        return x | y << 4;
    }

    static constexpr uint8_t xyToIndex64(uint8_t x, uint8_t y){
        return x + y * 8;
    }


    /**
     * @brief Is the coordinate valid (in the playable area)?
     * 
     * @return bool
     */
    constexpr bool isOnBoard() const{
        return !(getRaw() & 0x88);
    }

    /**
     * @brief Get the 0-63 index.
     * 
     * @return constexpr uint8_t index into array of size 64
     */
    constexpr uint8_t getIndex64() const{
        return (getRaw() + (getRaw() & 7)) >> 1;
    }

    // TODO: replace pass by reference with pass by value

    /**
     * @brief Apply an offset to a coordinate.
     * 
     */
    constexpr Coordinate operator + (Coordinate const& other) const{
        return Coordinate(this->x + other.x, this->y + other.y);
    }

    /**
     * @brief Apply an offset to a coordinate.
     * 
     */
    constexpr Coordinate operator += (Coordinate const& other){
        this->x += other.x;
        this->y += other.y;
        
        return *this;
    };
    /**
     * @brief Apply an offset to a coordinate.
     * 
     */
    constexpr Coordinate operator - (Coordinate const& other) const{
        return Coordinate(this->x - other.x, this->y - other.y);
    }

    /**
     * @brief Apply an offset to a coordinate.
     * 
     */
    constexpr Coordinate operator -= (Coordinate const& other){
        this->x -= other.x;
        this->y -= other.y;
        
        return *this;
    };

    constexpr Coordinate operator * (int factor) const{
        return Coordinate(this->x * factor, this->y * factor);
    }
    
    /**
     * @brief Apply an offset to a coordinate.
     * 
     */
    constexpr Coordinate operator - () const{
        return Coordinate(-x, -y);
    }

    constexpr bool operator == (Coordinate const& other) const{
        return this->getRaw() == other.getRaw();
    }
};


namespace DirectionIndex64 {
    enum : int{
        N = 8,
        E = 1,
        S = -8,
        W = -1,

        NE = N + E,
        NW = N + W,
        SE = S + E,
        SW = S + W,
    };
};

namespace Direction {
    constexpr Coordinate N = Coordinate( 0,  1);
    constexpr Coordinate E = Coordinate( 1,  0);
    constexpr Coordinate S = Coordinate( 0, -1);
    constexpr Coordinate W = Coordinate(-1,  0);


    constexpr Coordinate NE = N + E;
    constexpr Coordinate NW = N + W;
    constexpr Coordinate SE = S + E;
    constexpr Coordinate SW = S + W;
};


namespace SquareIndex64 {
    enum : int{
        a1, b1, c1, d1, e1, f1, g1, h1,
        a2, b2, c2, d2, e2, f2, g2, h2,
        a3, b3, c3, d3, e3, f3, g3, h3,
        a4, b4, c4, d4, e4, f4, g4, h4,
        a5, b5, c5, d5, e5, f5, g5, h5,
        a6, b6, c6, d6, e6, f6, g6, h6,
        a7, b7, c7, d7, e7, f7, g7, h7,
        a8, b8, c8, d8, e8, f8, g8, h8
    };
};

namespace Square{
    #define defSq(NAME) constexpr Coordinate NAME = Coordinate::fromIndex64(static_cast<uint8_t>(SquareIndex64::NAME));
    #define defRow(NAME) defSq(NAME##1) defSq(NAME##2) defSq(NAME##3) defSq(NAME##4) defSq(NAME##5) defSq(NAME##6) defSq(NAME##7) defSq(NAME##8)

    defRow(a);
    defRow(b);
    defRow(c);
    defRow(d);
    defRow(e);
    defRow(f);
    defRow(g);
    defRow(h);

    #undef defSq
    #undef defRow
}

}