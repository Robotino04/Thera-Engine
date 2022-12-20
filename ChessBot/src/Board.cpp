#include "ChessBot/Board.hpp"

#include <stdexcept>

namespace ChessBot{

static uint8_t coordToIndex(uint8_t x, uint8_t y){
	return x + y * 8;
}

Piece& Board::at(uint8_t x, uint8_t y){
	return squares.at(coordToIndex(x, y));
}

Piece& Board::at(uint8_t index){
	return squares.at(index);
}


Piece const& Board::at(uint8_t x, uint8_t y) const{
	return squares.at(coordToIndex(x, y));
}

Piece const& Board::at(uint8_t index) const{
	return squares.at(index);
}

void Board::loadFromFEN(std::string fen){
	for (auto& piece : squares){
		piece.type = PieceType::None;
		piece.color = PieceColor::None;
	}
	uint8_t x = 0, y = 0;
	int charIndex = -1;
	while(++charIndex < fen.size()){
		char c = fen.at(charIndex);
		switch (tolower(c)){
			case 'p':
				at(x, y).type = PieceType::Pawn;
				at(x, y).color = tolower(c) == c ? PieceColor::Black : PieceColor::White;
				x++;
				break;
			case 'n':
				at(x, y).type = PieceType::Knight;
				at(x, y).color = tolower(c) == c ? PieceColor::Black : PieceColor::White;
				x++;
				break;
			case 'b':
				at(x, y).type = PieceType::Bishop;
				at(x, y).color = tolower(c) == c ? PieceColor::Black : PieceColor::White;
				x++;
				break;
			case 'r':
				at(x, y).type = PieceType::Rook;
				at(x, y).color = tolower(c) == c ? PieceColor::Black : PieceColor::White;
				x++;
				break;
			case 'q':
				at(x, y).type = PieceType::Queen;
				at(x, y).color = tolower(c) == c ? PieceColor::Black : PieceColor::White;
				x++;
				break;
			case 'k':
				at(x, y).type = PieceType::King;
				at(x, y).color = tolower(c) == c ? PieceColor::Black : PieceColor::White;
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

}
