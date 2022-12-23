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

/**
 * @brief Return the opposite of the given color.
 * 
 * @param c the color to invert
 * @return PieceColor 
 */
static PieceColor opposite(PieceColor color){
	if(color == PieceColor::White) return PieceColor::Black;
	else if(color == PieceColor::Black) return PieceColor::White;
	else return PieceColor::None;
}

struct Piece{
	PieceType type = PieceType::None;
	PieceColor color = PieceColor::None;

	bool operator ==(Piece const& other) const{
		return this->type == other.type && this->color == other.color;
	}

	static const Piece Empty;
};

}
