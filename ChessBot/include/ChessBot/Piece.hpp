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

struct Piece{
	PieceType type;
	PieceColor color;

	bool operator ==(Piece const& other) const{
		return this->type == other.type && this->color == other.color;
	}
};

}
