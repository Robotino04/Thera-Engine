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

}