#include "ChessBot/Board.hpp"
#include "ChessBot/Utils.hpp"
#include "ChessBot/Move.hpp"

#include <stdexcept>
#include <assert.h>

namespace ChessBot{

Piece& Board::at(int8_t x, int8_t y){
	return squares.at(mailboxSmallToBig[Utils::coordToIndex(x, y)]);
}
Piece& Board::at(int8_t x, int8_t y, int8_t offset){
	return squares.at(mailboxSmallToBig[Utils::coordToIndex(x, y) + offset]);
}

Piece& Board::at(int8_t index){
	return squares.at(mailboxSmallToBig[index]);
}


Piece const& Board::at(int8_t x, int8_t y) const{
	return squares.at(mailboxSmallToBig[Utils::coordToIndex(x, y)]);
}
Piece const& Board::at(int8_t x, int8_t y, int8_t offset) const{
	return squares.at(mailboxSmallToBig[Utils::coordToIndex(x, y) + offset]);
}

Piece const& Board::at(int8_t index) const{
	return squares.at(mailboxSmallToBig[index]);
}

void Board::loadFromFEN(std::string fen){
	for (auto& piece : squares){
		piece.setType(PieceType::None);
		piece.setColor(PieceColor::White);
	}
	int8_t x = 0, y = 0;
	int charIndex = -1;
	while(++charIndex < fen.size()){
		char c = fen.at(charIndex);
		switch (tolower(c)){
			case 'p':
				at(x, y).setType(PieceType::Pawn);
				at(x, y).setColor(tolower(c) == c ? PieceColor::Black : PieceColor::White);
				x++;
				break;
			case 'n':
				at(x, y).setType(PieceType::Knight);
				at(x, y).setColor(tolower(c) == c ? PieceColor::Black : PieceColor::White);
				x++;
				break;
			case 'b':
				at(x, y).setType(PieceType::Bishop);
				at(x, y).setColor(tolower(c) == c ? PieceColor::Black : PieceColor::White);
				x++;
				break;
			case 'r':
				at(x, y).setType(PieceType::Rook);
				at(x, y).setColor(tolower(c) == c ? PieceColor::Black : PieceColor::White);
				x++;
				break;
			case 'q':
				at(x, y).setType(PieceType::Queen);
				at(x, y).setColor(tolower(c) == c ? PieceColor::Black : PieceColor::White);
				x++;
				break;
			case 'k':
				at(x, y).setType(PieceType::King);
				at(x, y).setColor(tolower(c) == c ? PieceColor::Black : PieceColor::White);
				x++;
				break;
			case '/':
				y++;
				x = 0;
				break;
			case '0'...'8':
				x += c - '0';
				break;
			case ' ':
				goto parse_turn;
			default:
				throw std::invalid_argument(std::string("Invalid character '") + c + "' in FEN string!");
				break;
		}
	}
	parse_turn:
	charIndex++; // skip space


	if (fen.at(charIndex) == 'w') colorToMove = PieceColor::White;
	else if(fen.at(charIndex) == 'b') colorToMove = PieceColor::Black;
	else
		throw std::invalid_argument(std::string("Invalid character '") + fen.at(charIndex) + "' in FEN string!");
	charIndex ++; // consume side to move

	canCastleLeft.fill(false);
	canCastleRight.fill(false);

	while (fen.at(++charIndex) != ' '){
		switch(fen.at(charIndex)){
			case 'k': canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = true; break;
			case 'K': canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = true; break;
			case 'q': canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = true; break;
			case 'Q': canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = true; break;
			case '-': goto parse_en_passant;
			default:
				throw std::invalid_argument(std::string("Invalid character '") + fen.at(charIndex-1) + "' in FEN string!");
				break;
		}
	}
	parse_en_passant:
	charIndex++; // skip space

	// TODO: Implement full FEN parsing
	
}

void Board::applyMove(Move const& move){
	applyMoveStatic(move);

	// update State
	colorToMove = Utils::oppositeColor(colorToMove);
}

void Board::applyMoveStatic(Move const& move){
	assert(move.startIndex != move.endIndex);

	switch(move.startIndex){
		// rook moves
		case Utils::coordToIndex(0,0): canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Utils::coordToIndex(7,0): canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false; break;
		case Utils::coordToIndex(0,7): canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Utils::coordToIndex(7,7): canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false; break;
	
		// king moves
		case Utils::coordToIndex(4, 0):
			canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			break;
		case Utils::coordToIndex(4, 7):
			canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false;
			canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false;
			break;
	}
	switch(move.endIndex){
		// rook moves
		case Utils::coordToIndex(0,0): canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Utils::coordToIndex(7,0): canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false; break;
		case Utils::coordToIndex(0,7): canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Utils::coordToIndex(7,7): canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false; break;
	
		// king moves
		case Utils::coordToIndex(4, 0):
			canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			break;
		case Utils::coordToIndex(4, 7):
			canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false;
			canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false;
			break;
	}

	at(move.endIndex) = at(move.startIndex);
	at(move.startIndex).setType(PieceType::None);

	at(move.startIndex).setHasMoved(true);
	at(move.endIndex).setHasMoved(true);

	// apply auxiliary moves
	if (move.auxiliaryMove)
		applyMoveStatic(*move.auxiliaryMove);
}

PieceColor Board::getColorToMove() const{
	return colorToMove;
}

std::array<bool, 2> const& Board::getCastleLeft() const {
	return canCastleLeft;
}
bool Board::getCastleLeft(PieceColor color) const{
	return canCastleLeft.at(static_cast<uint8_t>(color));
}
std::array<bool, 2> const& Board::getCastleRight() const {
	return canCastleRight;
}
bool Board::getCastleRight(PieceColor color) const{
	return canCastleRight.at(static_cast<uint8_t>(color));
}

}
