#include "ChessBot/Board.hpp"
#include "ChessBot/Move.hpp"

#include "ChessBot/Utils/Coordinates.hpp"
#include "ChessBot/Utils/Math.hpp"
#include "ChessBot/Utils/ChessTerms.hpp"
#include "ChessBot/Utils/BuildType.hpp"

#include <stdexcept>
#include <assert.h>
#include <map>

namespace ChessBot{

Piece& Board::at(int8_t x, int8_t y){
	return at(Utils::coordToIndex(x, y));
}
Piece& Board::at(int8_t x, int8_t y, int8_t offset){
	if constexpr(Utils::BuildType::Current == Utils::BuildType::Debug)
		return currentState.squares.at(to10x12Coords(Utils::coordToIndex(x, y) + offset));
	else
		return currentState.squares[to10x12Coords(Utils::coordToIndex(x, y) + offset)];
}

Piece& Board::at(int8_t index){
	if constexpr(Utils::BuildType::Current == Utils::BuildType::Debug)
		return currentState.squares.at(to10x12Coords(index));
	else
		return currentState.squares[to10x12Coords(index)];
}

Piece& Board::at10x12(int8_t index){
	if constexpr(Utils::BuildType::Current == Utils::BuildType::Debug)
		return currentState.squares.at(index);
	else
		return currentState.squares[index];
}

Piece const& Board::at(int8_t x, int8_t y) const{
	return at(Utils::coordToIndex(x, y));
}
Piece const& Board::at(int8_t x, int8_t y, int8_t offset) const{
	if constexpr(Utils::BuildType::Current == Utils::BuildType::Debug)
		return currentState.squares.at(to10x12Coords(Utils::coordToIndex(x, y) + offset));
	else
		return currentState.squares[to10x12Coords(Utils::coordToIndex(x, y) + offset)];
}

Piece const& Board::at(int8_t index) const{
	if constexpr(Utils::BuildType::Current == Utils::BuildType::Debug)
		return currentState.squares.at(to10x12Coords(index));
	else
		return currentState.squares[to10x12Coords(index)];
}

Piece const& Board::at10x12(int8_t index) const {
	if constexpr(Utils::BuildType::Current == Utils::BuildType::Debug)
		return currentState.squares.at(index);
	else
		return currentState.squares[index];
}

bool Board::isOccupied(int8_t square) const{
	return at10x12(square).getType() != PieceType::None;
}

bool Board::isFriendly(int8_t square) const{
	return at10x12(square).getColor() == currentState.colorToMove;
}

void Board::loadFromFEN(std::string fen){
	static const std::map<char, PieceType> fenCharsToPieceType = {
		{'p', PieceType::Pawn},
		{'b', PieceType::Bishop},
		{'n', PieceType::Knight},
		{'r', PieceType::Rook},
		{'q', PieceType::Queen},
		{'k', PieceType::King},
	};
	for (auto& piece : currentState.squares){
		piece.clear();
	}
	currentState.allPieceBitboard.clear();
	for (auto& bitboard : currentState.pieceBitboards){
		bitboard.clear();
	}

	int8_t x = 0, y = 0;
	int charIndex = -1;
	while(++charIndex < fen.size()){
		char c = fen.at(charIndex);
		switch (tolower(c)){
			case '/':
				y++;
				x = 0;
				break;
			case '0'...'8':
				x += c - '0';
				break;
			case ' ':
				goto parse_turn;
			default:{
				Piece piece;
				try{
					piece = Piece(fenCharsToPieceType.at(tolower(c)), islower(c) ? PieceColor::Black : PieceColor::White);
				}
				catch(std::out_of_range){
					throw std::invalid_argument(std::string("Invalid character '") + c + "' in FEN string!");
				}

				placePiece(Board::to10x12Coords(Utils::coordToIndex(x, y)), piece);
				x++;
				break;
			}
		}
	}
	parse_turn:
	charIndex++; // skip space


	if (fen.at(charIndex) == 'w') currentState.colorToMove = PieceColor::White;
	else if(fen.at(charIndex) == 'b') currentState.colorToMove = PieceColor::Black;
	else
		throw std::invalid_argument(std::string("Invalid character '") + fen.at(charIndex) + "' in FEN string!");
	charIndex ++; // consume side to move

	currentState.canCastleLeft.fill(false);
	currentState.canCastleRight.fill(false);

	while (fen.at(++charIndex) != ' '){
		switch(fen.at(charIndex)){
			case 'k': currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = true; break;
			case 'K': currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = true; break;
			case 'q': currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = true; break;
			case 'Q': currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = true; break;
			case '-': goto parse_en_passant;
			default:
				throw std::invalid_argument(std::string("Invalid character '") + fen.at(charIndex-1) + "' in FEN string!");
				break;
		}
	}
	parse_en_passant:
	charIndex++; // skip space

	if (!(Utils::isInRange(fen.at(charIndex), 'a', 'h') || fen.at(charIndex) == '-')){
		throw std::invalid_argument(std::string("Invalid character '") + fen.at(charIndex) + "' in FEN string!");
	}
	if (fen.at(charIndex) != '-'){
		currentState.enPassantSquare = Utils::squareFromAlgebraicNotation(fen.substr(charIndex, 2));
		charIndex += 1;
	}
	charIndex += 2; // skip sth and space
	

	// TODO: Implement move counters

	while (!rewindStack.empty()) rewindStack.pop();
}

void Board::applyMove(Move const& move){
	// save the current state
	rewindStack.push(currentState);

	applyMoveStatic(move);

	// update State
	currentState.colorToMove = currentState.colorToMove.opposite();
}

void Board::applyMoveStatic(Move const& move){
	assert(move.startIndex != move.endIndex);

	removeCastlings(move.startIndex);
	removeCastlings(move.endIndex);

	// apply the move to the bitboards
	if (currentState.allPieceBitboard.isOccupied(move.endIndex)){
		// capture
		getBitboard(at10x12(move.endIndex)).removePiece(move.endIndex);
	}
	currentState.allPieceBitboard.applyMove(move);
	getBitboard(at10x12(move.startIndex)).applyMove(move);

	// apply the move to the square centric representation
	at10x12(move.endIndex) = at10x12(move.startIndex);
	at10x12(move.startIndex).clear();

	// promotion
	if (move.promotionType != PieceType::None){
		placePiece(move.endIndex, Piece(move.promotionType, currentState.colorToMove));
	}

	// en passant
	if (move.isEnPassant){
		removePiece(currentState.enPassantSquare);
	}
	if (move.isDoublePawnMove){
		currentState.enPassantSquare = move.endIndex;
	}
	else{
		currentState.enPassantSquare = -1;
	}

	// apply auxiliary moves
	if (move.auxiliaryMove)
		applyMoveStatic(*move.auxiliaryMove);
}

void Board::rewindMove(){
	currentState = rewindStack.top();
	rewindStack.pop();
}

PieceColor Board::getColorToMove() const{
	return currentState.colorToMove;
}

Bitboard<12>& Board::getBitboard(Piece piece){
	return currentState.pieceBitboards.at(piece.getRaw());
}
Bitboard<12> const& Board::getBitboard(Piece piece) const {
	return currentState.pieceBitboards.at(piece.getRaw());
}

std::array<bool, 2> const& Board::getCastleLeft() const {
	return currentState.canCastleLeft;
}

bool Board::getCastleLeft(PieceColor color) const{
	return currentState.canCastleLeft.at(static_cast<uint8_t>(color));
}

std::array<bool, 2> const& Board::getCastleRight() const {
	return currentState.canCastleRight;
}

bool Board::getCastleRight(PieceColor color) const{
	return currentState.canCastleRight.at(static_cast<uint8_t>(color));
}

int8_t Board::getEnPassantSquare(){
	return currentState.enPassantSquare;
}

void Board::placePiece(int8_t square, Piece piece){
	at10x12(square) = piece;

	getBitboard(piece).placePiece(square);
	currentState.allPieceBitboard.placePiece(square);
}

void Board::removePiece(int8_t square){
	Piece& piece = at10x12(square);

	currentState.allPieceBitboard.removePiece(square);
	getBitboard(piece).removePiece(square);

	piece.clear();
}

void Board::removeCastlings(int8_t movedSquare){
	switch(movedSquare){
		// rook moves
		case Board::to10x12Coords(Utils::coordToIndex(0,0)): currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Board::to10x12Coords(Utils::coordToIndex(0,7)): currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false; break;
		case Board::to10x12Coords(Utils::coordToIndex(7,0)): currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Board::to10x12Coords(Utils::coordToIndex(7,7)): currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false; break;

		// king moves
		case Board::to10x12Coords(Utils::coordToIndex(4, 0)):
			currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			break;
		case Board::to10x12Coords(Utils::coordToIndex(4, 7)):
			currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false;
			currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false;
			break;
	}
}

}
