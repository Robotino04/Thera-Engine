#pragma once

#include "Thera/Piece.hpp"
#include "Thera/Bitboard.hpp"
#include "Thera/Utils/Coordinates.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <stack>
#include <optional>

namespace Thera{
struct Move;

/**
 * @brief A chess board representation.
 * 
 */
class Board{
	public:
		struct BoardState;

		/**
		 * @brief Return the piece at (index).
		 * 
		 * @param index index
		 * @return Piece& piece
		 */
		Piece& at(Coordinate8x8 index);

		/**
		 * @brief Return the piece at (index).
		 * 
		 * @param index index
		 * @return Piece const& piece
		 */
		Piece const& at(Coordinate8x8 index) const;
		
		/**
		 * @brief Return if a square is occupied
		 * 
		 * @param square
		 * @return bool
		 */
		bool isOccupied(Coordinate8x8 square) const;

		/**
		 * @brief Return if a square is the color to move. WON'T TEST IF SQUARE IS FILLED!
		 * 
		 * @param square 
		 * @return true 
		 * @return false 
		 */
		bool isFriendly(Coordinate8x8 square) const;

		/**
		 * @brief Load a board position from a FEN string.
		 * 
		 * @see	https://de.wikipedia.org/wiki/Forsyth-Edwards-Notation
		 * 
		 * @param fen fen string
		 */
		void loadFromFEN(std::string fen);

		/**
		 * @brief Store the board position to a FEN string.
		 * 
		 * @see	https://de.wikipedia.org/wiki/Forsyth-Edwards-Notation
		 * 
		 * @return std::string the fen string
		 */
		std::string storeToFEN() const;

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
		 * @brief Rewind the last applied move.
		 * 
		 */
		void rewindMove();

		/**
		 * @brief Get the color that has to make a move.
		 * 
		 * @return PieceColor 
		 */
		PieceColor getColorToMove() const;

		/**
		 * @brief Get the color that has just made a move.
		 * 
		 * @return PieceColor 
		 */
		PieceColor getColorToNotMove() const;

		/**
		 * @brief Get the current state.
		 * 
		 * @return BoardState the current board state
		 */
		BoardState const& getCurrentState() const;

		/**
		 * @brief Get the en passant square.
		 * 
		 * @return std::optional<Coordinate8x8> 
		 */
		std::optional<Coordinate8x8> getEnPassantSquare() const;

		/**
		 * @brief Get the en passant square to capture.
		 * 
		 * @return std::optional<Coordinate8x8> 
		 */
		std::optional<Coordinate8x8> getEnPassantSquareToCapture() const;

		/**
		 * @brief Get the bitboard containing a particular piece. 
		 * 
		 * @param piece the piece
		 * @return Bitboard<12>& the bitboard containing these pieces
		 */
		Bitboard<12>& getBitboard(Piece piece);
		/**
		 * @brief Get the bitboard to place a particular piece. 
		 * 
		 * @param piece the piece
		 * @return Bitboard<12> const& the bitboard containing these pieces
		 */
		Bitboard<12> const& getBitboard(Piece piece) const;

		/**
		 * @brief Get the bitboard containing all pieces 
		 * 
		 * @return Bitboard<32>& the bitboard containing all pieces
		 */
		Bitboard<32>& getAllPieceBitboard();

		/**
		 * @brief Get the bitboard containing all pieces 
		 * 
		 * @return Bitboard<32> const& the bitboard containing all pieces
		 */
		Bitboard<32> const& getAllPieceBitboard() const;

		/**
		 * @brief Place a piece onto the board.
		 * 
		 * @param square the square 
		 * @param piece the piece to place
		 */
		void placePiece(Coordinate8x8 square, Piece piece);

		/**
		 * @brief Remove a piece from the board.
		 * 
		 * @param square the square
		 */
		void removePiece(Coordinate8x8 square);

		/**
		 * @brief Remove castlings.
		 * 
		 * @param movedSquare the square whose piece got moved
		 */
		void removeCastlings(Coordinate8x8 movedSquare);

		/**
		 * @brief Change the color to move.
		 * 
		 * Essentially views the board from the opposite side.
		 * 
		 */
		void switchPerspective();

	public:
		struct BoardState{
			std::array<Piece, 10*12> squares;
			// size 16 to only use one index instead of two (for color and type)
			std::array<Bitboard<12>, 16> pieceBitboards;
			Bitboard<32> allPieceBitboard;

			std::optional<Coordinate8x8> enPassantSquare;
			std::optional<Coordinate8x8> enPassantSquareToCapture;

			bool isWhiteToMove: 1;
			bool canWhiteCastleLeft: 1;
			bool canBlackCastleLeft: 1;
			bool canWhiteCastleRight: 1;
			bool canBlackCastleRight: 1;
		};

	private:
		BoardState currentState;
		std::stack<BoardState> rewindStack;
};
}
