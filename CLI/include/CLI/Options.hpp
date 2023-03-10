#pragma once

#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Piece.hpp"

#include <string>

enum class Mode{
	Play,
	Perft,
};

struct Options {
	bool invertedColors = false;
	Mode mode = Mode::Play;
	int perftDepth = 1;
	std::string fen = Thera::Utils::startingFEN;
	bool bulkCounting = false;

	Thera::Piece shownBitboard = {Thera::PieceType::None, Thera::PieceColor::White};
	bool showDebugBitboard = false;
};