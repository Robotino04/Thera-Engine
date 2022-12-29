#include "ChessBot/Board.hpp"
#include "ChessBot/Utils.hpp"
#include "ChessBot/Move.hpp"

#include <stdexcept>
#include <assert.h>

namespace ChessBot{

Piece& Board::at(int8_t x, int8_t y){
	return currentState.squares.at(mailboxSmallToBig[Utils::coordToIndex(x, y)]);
}
Piece& Board::at(int8_t x, int8_t y, int8_t offset){
	return currentState.squares.at(mailboxSmallToBig[Utils::coordToIndex(x, y) + offset]);
}

Piece& Board::at(int8_t index){
	return currentState.squares.at(mailboxSmallToBig[index]);
}


Piece const& Board::at(int8_t x, int8_t y) const{
	return currentState.squares.at(mailboxSmallToBig[Utils::coordToIndex(x, y)]);
}
Piece const& Board::at(int8_t x, int8_t y, int8_t offset) const{
	return currentState.squares.at(mailboxSmallToBig[Utils::coordToIndex(x, y) + offset]);
}

Piece const& Board::at(int8_t index) const{
	return currentState.squares.at(mailboxSmallToBig[index]);
}

bool Board::isEmpty(int8_t square) const{
	return at(square).getType() == PieceType::None;
}
bool Board::isFriendly(int8_t square) const{
	return at(square).getColor() == currentState.colorToMove;
}

void Board::loadFromFEN(std::string fen){
	for (auto& piece : currentState.squares){
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
		currentState.enPassantSquare = Utils::squareFromString(fen.substr(charIndex, 2));
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
	currentState.colorToMove = Utils::oppositeColor(currentState.colorToMove);
}

void Board::applyMoveStatic(Move const& move){
	assert(move.startIndex != move.endIndex);

	switch(move.startIndex){
		// rook moves
		case Utils::coordToIndex(0,0): currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Utils::coordToIndex(7,0): currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false; break;
		case Utils::coordToIndex(0,7): currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Utils::coordToIndex(7,7): currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false; break;
	
		// king moves
		case Utils::coordToIndex(4, 0):
			currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			break;
		case Utils::coordToIndex(4, 7):
			currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false;
			currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false;
			break;
	}
	switch(move.endIndex){
		// rook moves
		case Utils::coordToIndex(0,0): currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Utils::coordToIndex(7,0): currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false; break;
		case Utils::coordToIndex(0,7): currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false; break;
		case Utils::coordToIndex(7,7): currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false; break;
	
		// king moves
		case Utils::coordToIndex(4, 0):
			currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::Black)) = false;
			break;
		case Utils::coordToIndex(4, 7):
			currentState.canCastleLeft.at(static_cast<uint8_t>(PieceColor::White)) = false;
			currentState.canCastleRight.at(static_cast<uint8_t>(PieceColor::White)) = false;
			break;
	}

	at(move.endIndex) = at(move.startIndex);
	at(move.startIndex).setType(PieceType::None);

	// promotion
	if (move.promotionType != PieceType::None)
		at(move.endIndex).setType(move.promotionType);

	// en passant
	if (move.isEnPassant){
		at(currentState.enPassantSquare).setType(PieceType::None);
	}
	if (move.isDoublePawnMove){
		currentState.enPassantSquare = move.endIndex;
	}
	else{
		currentState.enPassantSquare = -1;
	}

	at(move.startIndex).setHasMoved(true);
	at(move.endIndex).setHasMoved(true);

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

}
