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

enum class PieceColor : uint8_t{
	White = 0,
	Black = 1,
};

class Piece{
	public:
		constexpr Piece(PieceType type, PieceColor color): type(type), color(color){}
		constexpr Piece(): type(PieceType::None), color(PieceColor::White){}

		constexpr int getRaw() const{
			return static_cast<int>(color) + (static_cast<int>(type) << 1);
		}

		constexpr bool operator ==(Piece const& other) const{
			return this->getRaw() == other.getRaw();
		}


		constexpr bool operator < (Piece other) const{
			return getRaw() < other.getRaw();
		}
	
		PieceType type;
		PieceColor color;
};

}