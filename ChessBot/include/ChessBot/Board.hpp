#pragma once

#include "ChessBot/Piece.hpp"
#include "ChessBot/Move.hpp"

#include <array>
#include <stdint.h>
#include <string>

namespace ChessBot{

/**
 * @brief A chess board representation.
 * 
 */
class Board{
	public:
		/**
		 * @brief Return the piece at (x, y).
		 * 
		 * @param x X coordinate
		 * @param y Y coordinate
		 * @return Piece& piece
		 */
		Piece& at(uint8_t x, uint8_t y);

		/**
		 * @brief Return the piece at (index).
		 * 
		 * @param index index
		 * @return Piece& piece
		 */
		Piece& at(uint8_t index);

		/**
		 * @brief Return the piece at (x, y).
		 * 
		 * @param x X coordinate
		 * @param y Y coordinate
		 * @return Piece const& piece
		 */
		
		Piece const& at(uint8_t x, uint8_t y) const;

		/**
		 * @brief Return the piece at (index).
		 * 
		 * @param index index
		 * @return Piece const& piece
		 */
		Piece const& at(uint8_t index) const;

		/**
		 * @brief Load a board position from a FEN string.
		 * 
		 * @see	https://de.wikipedia.org/wiki/Forsyth-Edwards-Notation
		 * 
		 * @param fen fen string
		 */
		void loadFromFEN(std::string fen);

		/**
		 * @brief Make a mve on the board and update the state.
		 * 
		 * @param move
		 */
		void applyMove(Move const& move);

	private:
		std::array<Piece, 64> squares;

		PieceColor colorToMove = PieceColor::White;
};
}
