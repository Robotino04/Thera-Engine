#pragma once

namespace ChessBot{

enum class PieceType{
	None = 0,
	Pawn,
	Knight,
	Bishop,
	Rook,
	Queen,
	King,
};

enum class PieceColor{
	None = 0,
	White,
	Black,
};

static PieceColor opposite(PieceColor c){
	if(c == PieceColor::White) return PieceColor::Black;
	else if(c == PieceColor::Black) return PieceColor::White;
	else return PieceColor::None;
}

struct Piece{
	PieceType type;
	PieceColor color;

	bool operator ==(Piece const& other) const{
		return this->type == other.type && this->color == other.color;
	}
};

}
