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
	if (fen.at(++charIndex) == 'w') colorToMove = PieceColor::White;
	else colorToMove = PieceColor::Black;

	// TODO: Implement full FEN parsing (not just the piece positions)
	
}

void Board::applyMove(Move const& move){
	assert(move.startIndex != move.endIndex);

	at(move.endIndex) = at(move.startIndex);
	at(move.startIndex).setType(PieceType::None);
}

PieceColor Board::getColorToMove() const{
	return colorToMove;
}

}
