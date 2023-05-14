#pragma once

#include <cstdint>

#include "Thera/Utils/Bits.hpp"

namespace Thera{

enum class PieceType : uint8_t{
	None = 0,
	Pawn = 1,
	Knight = 2,
	Bishop = 3,
	Rook = 4,
	Queen = 5,
	King = 6,
};

class PieceColor{
	public:
  		enum Value : uint8_t{
			White = 0,
			Black = 1,
		};

	public:
		constexpr PieceColor() = default;
		constexpr PieceColor(Value color) : value(color) {}

		// Allow switch and comparisons.
		constexpr operator Value() const { return value; }

		// Prevent usage: if(pieceColor)
		constexpr explicit operator bool() const = delete;
		constexpr bool operator ==(PieceColor other) const { return value == other.value; }
		constexpr bool operator !=(PieceColor other) const { return value != other.value; }

		constexpr bool operator ==(Value other) const { return value == other; }
		constexpr bool operator !=(Value other) const { return value != other; }
		
		constexpr PieceColor opposite() const { return value == White ? Black : White; }

	private:
		Value value;
};

class Piece{
	public:
		constexpr Piece(): raw(0) {}
		constexpr Piece(PieceType type, PieceColor color){
			setType(type);
			setColor(color);
		}

		constexpr bool operator ==(Piece const& other) const{
			return this->raw == other.raw;
		}

		constexpr void setColor(PieceColor color){
			setBit(colorBit, static_cast<uint8_t>(color));
		}
		constexpr void setType(PieceType type){
			raw = (raw & (~typeBitMask)) | static_cast<uint8_t>(type);
		}
		constexpr void clear(){
			raw = 0;
		}

		constexpr PieceColor getColor() const{
			return static_cast<PieceColor::Value>(getBit(colorBit));
		}
		constexpr PieceType getType() const{
			return static_cast<PieceType>(raw & typeBitMask);
		}

		constexpr uint8_t getRaw() const{
			return raw;
		}

		constexpr bool operator < (Piece other) const{
			return getRaw() < other.getRaw();
		}
	
	private:

		constexpr  void setBit(int bit, uint8_t value){
			raw = Utils::setBit(raw, bit, value);
		}
		constexpr  uint8_t getBit(int bit) const{
			return Utils::getBit(raw, bit);
		}
		

		/*
		Layout:
			76543210
			XXXXCTTT

			C: piece color
			T: piece type
			X: unused
		*/

		uint8_t raw = 0;

		static constexpr uint8_t colorBit = 3;

		static constexpr uint8_t colorBitMask = 1 << colorBit;
		static constexpr uint8_t typeBitMask = 0b111;
};
static_assert(sizeof(Piece) == 1);

}