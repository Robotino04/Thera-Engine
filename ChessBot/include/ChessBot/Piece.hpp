#pragma once

#include <stdint.h>

namespace ChessBot{

enum class PieceType : uint8_t{
	None = 0,
	Pawn = 1,
	Knight = 2,
	Bishop = 3,
	Rook = 4,
	Queen = 5,
	King = 6,
};

enum class PieceColor: uint8_t{
	White = 0,
	Black = 1,
};

class Piece{
	public:
		bool operator ==(Piece const& other) const{
			return this->raw == other.raw;
		}

		void setColor(PieceColor color){
			setBit(colorBit, static_cast<uint8_t>(color));
		}
		void setType(PieceType type){
			raw = (raw & (0xFF ^ typeBitMask)) | static_cast<uint8_t>(type);
		}

		PieceColor getColor() const{
			return static_cast<PieceColor>(getBit(colorBit));
		}
		PieceType getType() const{
			return static_cast<PieceType>(raw & typeBitMask);
		}
	
	private:

		inline void setBit(int bit, int8_t value){
			raw = (raw & ~(1 << bit)) | ((value & 1) << bit);
		}
		inline uint8_t getBit(int bit) const{
			return (raw >> bit) & 1;
		}
		

		uint8_t raw;

		static constexpr uint8_t colorBit = 7;
		static constexpr uint8_t castleBit = 4;
		static constexpr uint8_t hasMovedBit = 3;

		static constexpr uint8_t colorBitMask = 1 << colorBit;
		static constexpr uint8_t castleBitMask = 1 << castleBit;
		static constexpr uint8_t hasMovedBitMask = 1 << hasMovedBit;
		static constexpr uint8_t typeBitMask = 0b111;
};
static_assert(sizeof(Piece) == 1);

}
