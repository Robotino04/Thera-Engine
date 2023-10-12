#pragma once

#include "Thera/Piece.hpp"
#include "Thera/Bitboard.hpp"
#include "Thera/Coordinate.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <stack>
#include <optional>
#include <unordered_map>

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
		 * @return Piece piece
		 */
		Piece at(Coordinate index) const;

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
		constexpr PieceColor getColorToMove() const { return currentState.isWhiteToMove ? PieceColor::White : PieceColor::Black; }

		/**
		 * @brief Get the color that has just made a move.
		 * 
		 * @return PieceColor 
		 */
		constexpr PieceColor getColorToNotMove() const { return currentState.isWhiteToMove ? PieceColor::Black : PieceColor::White; }

		/**
		 * @brief Get the current state.
		 * 
		 * @return BoardState the current board state
		 */
		constexpr BoardState const& getCurrentState() const { return currentState; }

		/**
		 * @brief Get the en passant square.
		 * 
		 * @return Coordinate 
		 */
		constexpr Coordinate getEnPassantSquareForFEN() const { return currentState.enPassantSquareForFEN; }

		/**
		 * @brief Get the en passant square to capture.
		 * 
		 * @return Coordinate 
		 */
		constexpr Coordinate getEnPassantSquareToCapture() const { return currentState.enPassantSquareToCapture; }

		/**
		 * @brief Is en passant possible.
		*/
		constexpr bool hasEnPassant() const{ return currentState.hasEnPassant; }

		/**
		 * @brief Get the bitboard containing a particular piece. 
		 * 
		 * @param piece the piece
		 * @return Bitboard& the bitboard containing these pieces
		 */
		constexpr Bitboard& getBitboard(Piece piece){ return currentState.pieceBitboards.at(piece.getRaw()); }
		
		/**
		 * @brief Get the bitboard to place a particular piece. 
		 * 
		 * @param piece the piece
		 * @return Bitboard the bitboard containing these pieces
		 */
		constexpr Bitboard getBitboard(Piece piece) const{ return currentState.pieceBitboards.at(piece.getRaw()); }

		/**
		 * @brief Get the bitboard containing all pieces 
		 * 
		 * @return Bitboard& the bitboard containing all pieces
		 */
		constexpr Bitboard& getAllPieceBitboard(){ return currentState.allPieceBitboard; }

		/**
		 * @brief Get the bitboard containing all pieces 
		 * 
		 * @return Bitboard the bitboard containing all pieces
		 */
		constexpr Bitboard getAllPieceBitboard() const{ return currentState.allPieceBitboard; }

		/**
		 * @brief Get the bitboard containing all pieces of one color
		 * 
		 * @param color whose pieces to return
		 * @return Bitboard& the bitboard containing all pieces one color
		 */
		constexpr Bitboard& getPieceBitboardForOneColor(PieceColor color){
			return currentState.pieceBitboards.at(Piece(PieceType::None, color).getRaw());
		}

		/**
		 * @brief Get the bitboard containing all pieces of one color
		 * 
		 * @param color whose pieces to return
		 * @return Bitboard the bitboard containing all pieces one color
		 */
		constexpr Bitboard getPieceBitboardForOneColor(PieceColor color) const {
			return currentState.pieceBitboards.at(Piece(PieceType::None, color).getRaw());
		}

		/**
		 * @brief Place a piece onto the board.
		 * 
		 * @param square the square 
		 * @param piece the piece to place
		 */
		void placePiece(Coordinate square, Piece piece);

		/**
		 * @brief Remove a piece from the board.
		 * 
		 * @param square the square
		 */
		void removePiece(Coordinate square);

		/**
		 * @brief Remove castlings.
		 * 
		 * @param movedSquare the square whose piece got moved
		 */
		void removeCastlings(Coordinate movedSquare);

		/**
		 * @brief Change the color to move.
		 * 
		 * Essentially views the board from the opposite side.
		 * 
		 */
		constexpr void switchPerspective(){
			currentState.isWhiteToMove = !currentState.isWhiteToMove;
			currentState.zobristHash ^= zobristBlackToMove;
		}

		/**
		 * @brief Get the Current zorist hash
		 * 
		 * @return constexpr uint64_t 
		 */
		constexpr uint64_t getCurrentHash() const { return currentState.zobristHash; }

		/**
		 * @brief Compares the hashes of the boards. DO NOT USE OUTSIDE OF HASHMAP!
		 * 
		 * This is just for std::unordered_map to work. It completely ignores the actual equalness and thus avoids any collisions.
		 * We can do this, since it is somewhat ok to use the wrong evaluation while searching.
		 * 
		 * @param other the other board
		 * @return bool do they have the same hash
		 */
		bool operator==(Board const& other) const{
			return this->getCurrentHash() == other.getCurrentHash();
		}

		bool is3FoldRepetition() const{
			uint64_t hash = getCurrentHash();
			return numberOfPositionRepetitions.contains(hash) && numberOfPositionRepetitions.at(hash) >= 3;
		}

	public:
		struct BoardState{
			/**
			 * @brief Bitboards for all pieces. None and some color represent all pieces of said color.
			 * 
			 */
			std::array<Bitboard, 16> pieceBitboards;
			Bitboard allPieceBitboard;

			bool hasEnPassant = false;
			Coordinate enPassantSquareForFEN;
			Coordinate enPassantSquareToCapture;

			bool isWhiteToMove: 1;
			bool canWhiteCastleLeft: 1;
			bool canBlackCastleLeft: 1;
			bool canWhiteCastleRight: 1;
			bool canBlackCastleRight: 1;
			uint64_t zobristHash;
		};

	private:
		BoardState currentState;
		std::stack<BoardState> rewindStack;

		std::array<std::array<uint64_t, 16>, 64> zobristTable;
		uint64_t zobristBlackToMove;
		
		std::unordered_map<uint64_t, int> numberOfPositionRepetitions;
};
}