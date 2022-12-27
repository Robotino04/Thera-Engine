#pragma once

#include "ChessBot/Piece.hpp"
#include "ChessBot/Utils.hpp"

#include <array>
#include <stdint.h>
#include <string>

namespace ChessBot{
struct Move;

namespace Detail{
	constexpr std::array<int, 10*12> generateMailboxBigToSmall(){
		std::array<int, 10*12> result = {};
		for (auto& x : result) x = -1;

		for (int x=0; x<8; x++){
			for (int y=0; y<8; y++){
				result[Utils::coordToIndex(x+1, y+2, 10)] = Utils::coordToIndex(x, y);
			}
		}
		return result;
	}
	constexpr std::array<int, 8*8> generateMailboxSmallToBig(){
		std::array<int, 8*8> result = {};
		for (auto& x : result) x = -1;

		for (int x=0; x<8; x++){
			for (int y=0; y<8; y++){
				result[Utils::coordToIndex(x, y)] = Utils::coordToIndex(x+1, y+2, 10);
			}
		}
		return result;
	}
}

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
		Piece& at(int8_t x, int8_t y);
		/**
		 * @brief Return the piece at (x, y) + offset.
		 * 
		 * @param x X coordinate
		 * @param y Y coordinate
		 * @param offset an offset into the 10x12 board
		 * @return Piece& piece
		 */
		Piece& at(int8_t x, int8_t y, int8_t offset);

		/**
		 * @brief Return the piece at (index).
		 * 
		 * @param index index
		 * @return Piece& piece
		 */
		Piece& at(int8_t index);

		/**
		 * @brief Return the piece at (x, y).
		 * 
		 * @param x X coordinate
		 * @param y Y coordinate
		 * @return Piece const& piece
		 */
		
		Piece const& at(int8_t x, int8_t y) const;

		/**
		 * @brief Return the piece at (x, y) + offset.
		 * 
		 * @param x X coordinate
		 * @param y Y coordinate
		 * @param offset an offset into the 10x12 board
		 * @return Piece& piece
		 */
		Piece const& at(int8_t x, int8_t y, int8_t offset) const;

		/**
		 * @brief Return the piece at (index).
		 * 
		 * @param index index
		 * @return Piece const& piece
		 */
		Piece const& at(int8_t index) const;

		/**
		 * @brief Load a board position from a FEN string.
		 * 
		 * @see	https://de.wikipedia.org/wiki/Forsyth-Edwards-Notation
		 * 
		 * @param fen fen string
		 */
		void loadFromFEN(std::string fen);

		/**
		 * @brief Make a move on the board and update the state.
		 * 
		 * @param move
		 */
		void applyMove(Move const& move);

		/**
		 * @brief Make a move on the board and keep the state static. (don't change color to move, etc.)
		 * 
		 * @param move
		 */
		void applyMoveStatic(Move const& move);

		/**
		 * @brief Get the color that has to make a move.
		 * 
		 * @return PieceColor 
		 */
		PieceColor getColorToMove() const;

		/**
		 * @brief Get the castling rights for the left side (queen side).
		 * 
		 * @return std::array<bool, 2> const& array of [can black castle, can white castle]
		 */
		std::array<bool, 2> const& getCastleLeft() const;

		/**
		 * @brief Get the castling rights for the left side (queen side).
		 * 
		 * @return bool can [color] castle left
		 */
		bool getCastleLeft(PieceColor color) const;

		/**
		 * @brief Get the castling rights for the right side (king side).
		 * 
		 * @return std::array<bool, 2> const& array of [can black castle, can white castle]
		 */
		std::array<bool, 2> const& getCastleRight() const;

		/**
		 * @brief Get the castling rights for the right side (king side).
		 * 
		 * @return bool can [color] castle right
		 */
		bool getCastleRight(PieceColor color) const;

		/**
		 * @brief Return if given index and offset are still on the board.
		 * 
		 * @param index the base index
		 * @param offset the offset
		 * @return bool
		 */
		static constexpr bool isOnBoard(int8_t index, int8_t offset) {
			return mailboxBigToSmall.at(mailboxSmallToBig.at(index) + offset) != -1;
		};

		/**
		 * @brief Apply a 10x12 offset to 8x8 coordinates.
		 * 
		 * @param index the base index
		 * @param offset the 10x12 offset
		 * @return constexpr int8_t the new 8x8 coordinates
		 */
		static constexpr int8_t applyOffset(int8_t index, int8_t offset){
			return mailboxBigToSmall.at(mailboxSmallToBig.at(index) + offset);
		}
	private:
		std::array<Piece, 10*12> squares;

		PieceColor colorToMove = PieceColor::White;
		std::array<bool, 2> canCastleLeft = {true, true};
		std::array<bool, 2> canCastleRight = {true, true};
		

		static constexpr std::array<int, 10*12> mailboxBigToSmall = Detail::generateMailboxBigToSmall();
		static constexpr std::array<int, 8*8> mailboxSmallToBig = Detail::generateMailboxSmallToBig();
};
}
