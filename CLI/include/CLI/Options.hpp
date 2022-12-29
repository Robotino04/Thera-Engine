#pragma once

#include "ChessBot/Utils.hpp"

#include <string>

enum class Mode{
	None,
	Play,
	Perft,
};

struct Options {
	bool invertedColors = false;
	Mode mode = Mode::None;
	int perftDepth = 1;
	std::string fen = ChessBot::Utils::startingFEN;
	bool bulkCounting = false;
};