#include "Thera/Board.hpp"
#include "Thera/Move.hpp"

#include "Thera/Utils/Coordinates.hpp"
#include "Thera/Utils/Math.hpp"
#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Utils/BuildType.hpp"
#include "Thera/Utils/ScopeGuard.hpp"

#include "Thera/TemporyryCoordinateTypes.hpp"

#include <stdexcept>
#include <assert.h>
#include <map>
#include <sstream>

namespace Thera{

Piece& Board::at(Coordinate8x8 index){
	if constexpr(Utils::BuildType::Current == Utils::BuildType::Debug)
		return currentState.squares.at(Coordinate10x12(index).pos);
	else
		return currentState.squares[Coordinate10x12(index).pos];
}

Piece const& Board::at(Coordinate8x8 index) const{
	if constexpr(Utils::BuildType::Current == Utils::BuildType::Debug)
		return currentState.squares.at(Coordinate10x12(index).pos);
	else
		return currentState.squares[Coordinate10x12(index).pos];
}

bool Board::isOccupied(Coordinate10x12 square) const{
	return at(square).getType() != PieceType::None;
}

bool Board::isFriendly(Coordinate10x12 square) const{
	return at(square).getColor() == getColorToMove();
}

static std::string generateFenErrorText(std::string const& fen, int charIndex){
	std::stringstream ss;
	ss << "Invalid character '" << fen.at(charIndex) << "' in FEN string!\n";
	ss << " \"" << fen << "\"\n  ";
	for (int i=0; i<charIndex;i++){
		ss << " ";
	}
	ss << "Ʌ";
	return ss.str();
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
					throw std::invalid_argument(generateFenErrorText(fen, charIndex));
				}

				placePiece(Utils::coordToIndex(x, y), piece);
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
		throw std::invalid_argument(generateFenErrorText(fen, charIndex));
	charIndex += 2; // consume side to move and space

	currentState.canCastleLeft.fill(false);
	currentState.canCastleRight.fill(false);

	while (fen.at(charIndex) != ' '){
		switch(fen.at(charIndex)){
			case 'k': currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = true; break;
			case 'K': currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = true; break;
			case 'q': currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = true; break;
			case 'Q': currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = true; break;
			case '-': charIndex++; goto parse_en_passant; // skip char since loop would normaly do that
			default:
				throw std::invalid_argument(generateFenErrorText(fen, charIndex));
				break;
		}
		charIndex++;
	}
	parse_en_passant:
	charIndex++; // skip space

	if (!(Utils::isInRange(fen.at(charIndex), 'a', 'h') || fen.at(charIndex) == '-')){
		throw std::invalid_argument(generateFenErrorText(fen, charIndex));
	}
	if (fen.at(charIndex) == '-'){
		currentState.enPassantSquare.reset();
	}
	else{
		currentState.enPassantSquare = Utils::squareFromAlgebraicNotation(fen.substr(charIndex, 2));
		charIndex += 1;
	}
	charIndex += 2; // skip sth and space
	

	// TODO: Implement move counters

	while (!rewindStack.empty()) rewindStack.pop();
}
std::string Board::storeToFEN() const{
	static const std::map<PieceType, char> pieceTypeToFenChars = {
		{PieceType::Pawn, 'p'},
		{PieceType::Bishop, 'b'},
		{PieceType::Knight, 'n'},
		{PieceType::Rook, 'r'},
		{PieceType::Queen, 'q'},
		{PieceType::King, 'k'},
	};

	std::string fen = "";

	for (int i=0; i<64; i++){
		try{
			char c = pieceTypeToFenChars.at(at(Coordinate8x8(i)).getType());
			if (at(Coordinate8x8(i)).getColor() == PieceColor::White){
				c = std::toupper(c);
			}

			fen += c;
		}
		catch(std::out_of_range){
			// square is empty
			if (Utils::isInRange(fen.back(), '0', '7')){
				fen.back()++;
			}
			else{
				fen += '1';
			}
		}

		if ((i + 1) % 8 == 0 && i<63){
			fen += '/';
		}
	}

	fen += ' ';
	fen += getColorToMove() == PieceColor::White ? 'w' : 'b';

	fen += ' ';
	if (getCastleLeft(PieceColor::White)) fen += 'Q';
	if (getCastleRight(PieceColor::White)) fen += 'K';
	if (getCastleLeft(PieceColor::Black)) fen += 'q';
	if (getCastleRight(PieceColor::Black)) fen += 'k';

	if (!(getCastleLeft(PieceColor::White) || getCastleRight(PieceColor::White) || getCastleLeft(PieceColor::Black) || getCastleRight(PieceColor::Black))){
		fen += '-';
	}

	fen += ' ';
	if (getEnPassantSquare().has_value()){
		fen += Utils::squareToAlgebraicNotation(getEnPassantSquare().value());
	}
	else{
		fen += '-';
	}

	return fen;
}

void Board::applyMove(Move const& move){
	// save the current state
	rewindStack.push(currentState);

	applyMoveStatic(move);

	// update State
	switchPerspective();
}

void Board::applyMoveStatic(Move const& move){
	if(move.startIndex == move.endIndex) return;

	removeCastlings(move.startIndex);
	removeCastlings(move.endIndex);

	// apply the move to the bitboards
	if (currentState.allPieceBitboard.isOccupied(move.endIndex)){
		// capture
		getBitboard(at(move.endIndex)).removePiece(move.endIndex);
	}
	currentState.allPieceBitboard.applyMove(move);
	getBitboard(at(move.startIndex)).applyMove(move);

	// apply the move to the square centric representation
	at(move.endIndex) = at(move.startIndex);
	at(move.startIndex).clear();

	// promotion
	if (move.promotionType != PieceType::None){
		removePiece(move.endIndex);
		placePiece(move.endIndex, Piece(move.promotionType, getColorToMove()));
	}

	// en passant
	if (move.isEnPassant){
		removePiece(getEnPassantSquareToCapture().value());
		currentState.enPassantSquare.reset();
		currentState.enPassantSquareToCapture.reset();
	}
	if (move.isDoublePawnMove){
		// get the "jumped" square
		currentState.enPassantSquare = Coordinate10x12((move.startIndex.pos + move.endIndex.pos) / 2);
		currentState.enPassantSquareToCapture = move.endIndex;
	}
	else{
		currentState.enPassantSquare.reset();
		currentState.enPassantSquareToCapture.reset();
	}

	// apply auxiliary moves
	if (move.auxiliaryMove)
		applyMoveStatic(*move.auxiliaryMove);
}

void Board::rewindMove(){
	if (rewindStack.empty())
		throw std::runtime_error("Tried to rewind move, but no moves were made.");
	
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
Bitboard<32>& Board::getAllPieceBitboard(){
	return currentState.allPieceBitboard;
}
Bitboard<32> const& Board::getAllPieceBitboard() const{
	return currentState.allPieceBitboard;
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

std::optional<Coordinate10x12> Board::getEnPassantSquare() const{
	return currentState.enPassantSquare;
}

std::optional<Coordinate10x12> Board::getEnPassantSquareToCapture() const{
	return currentState.enPassantSquareToCapture;
}

void Board::placePiece(Coordinate10x12 square, Piece piece){
	at(square) = piece;

	getBitboard(piece).placePiece(square);
	currentState.allPieceBitboard.placePiece(square);
}

void Board::removePiece(Coordinate10x12 square){
	Piece& piece = at(square);

	currentState.allPieceBitboard.removePiece(square);
	getBitboard(piece).removePiece(square);

	piece.clear();
}

void Board::removeCastlings(Coordinate8x8 movedSquare){
	switch(movedSquare.pos){
		// rook moves
		case Utils::coordToIndex(0,0).pos: currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Utils::coordToIndex(0,7).pos: currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false; break;
		case Utils::coordToIndex(7,0).pos: currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Utils::coordToIndex(7,7).pos: currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false; break;

		// king moves
		case Utils::coordToIndex(4, 0).pos:
			currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			break;
		case Utils::coordToIndex(4, 7).pos:
			currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false;
			currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false;
			break;
	}
}

void Board::switchPerspective(){
	currentState.colorToMove = getColorToMove().opposite();
}

}
