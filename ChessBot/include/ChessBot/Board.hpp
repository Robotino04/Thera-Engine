#pragma once

#include "ChessBot/Piece.hpp"

#include <array>
#include <stdint.h>
#include <string>

namespace ChessBot{

static uint8_t coordToIndex(uint8_t x, uint8_t y);

class Board{
	public:
		Piece& at(uint8_t x, uint8_t y);
		Piece& at(uint8_t index);

		Piece const& at(uint8_t x, uint8_t y) const;
		Piece const& at(uint8_t index) const;

		void loadFromFEN(std::string fen);

	private:
		std::array<Piece, 64> squares;

		PieceColor colorToMove = PieceColor::White;
};
}
