#pragma once

#include "ChessBot/Utils/ChessTerms.hpp"

#include <string>

enum class Mode{
	Play,
	Perft,
};

struct Options {
	bool invertedColors = false;
	Mode mode = Mode::Play;
	int perftDepth = 1;
	std::string fen = ChessBot::Utils::startingFEN;
	bool bulkCounting = false;
};