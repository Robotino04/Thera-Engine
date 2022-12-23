#include "ChessBot/Board.hpp"
#include "ChessBot/Utils.hpp"
#include "ChessBot/Move.hpp"

#include "ANSI/ANSI.hpp"

#include <iostream>
#include <map>
#include <stdint.h>

struct RGB{
	uint8_t red, green, blue;
};

struct Options {
	bool invertedColors = false;
};

void printBoard(ChessBot::Board const& board, Options const& options){
	using namespace ChessBot;

	const RGB whiteBoardColor = {255, 210, 153};
	const RGB blackBoardColor = {130, 77, 39};
	const RGB whitePieceColor = {255, 255, 255};
	const RGB blackPieceColor = {0, 0, 0};
	const RGB pieceColor = options.invertedColors ? whitePieceColor : blackPieceColor;

	static const std::map<std::pair<PieceColor, PieceType>, std::string> pieces = {
		{{PieceColor::None, PieceType::None}, " "},
		{{PieceColor::White, PieceType::None}, " "},
		{{PieceColor::Black, PieceType::None}, " "},
		{{PieceColor::White, PieceType::Pawn}, "♙"},
		{{PieceColor::Black, PieceType::Pawn}, "♟︎"},
		{{PieceColor::White, PieceType::Bishop}, "♗"},
		{{PieceColor::Black, PieceType::Bishop}, "♝"},
		{{PieceColor::White, PieceType::Knight}, "♘"},
		{{PieceColor::Black, PieceType::Knight}, "♞"},
		{{PieceColor::White, PieceType::Rook}, "♖"},
		{{PieceColor::Black, PieceType::Rook}, "♜"},
		{{PieceColor::White, PieceType::Queen}, "♕"},
		{{PieceColor::Black, PieceType::Queen}, "♛"},
		{{PieceColor::White, PieceType::King}, "♔"},
		{{PieceColor::Black, PieceType::King}, "♚"},
	};

	std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << "  a b c d e f g h   " << ANSI::reset()  << "\n";	
	for(uint8_t y=0; y<8; y++){
		std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << static_cast<int>(8-y) << " ";
		for(uint8_t x=0; x<8; x++){
			const RGB boardColor = (x + y)%2 ? whiteBoardColor : blackBoardColor;

			// set the board color
			std::cout << ANSI::set24BitColor(boardColor.red, boardColor.green, boardColor.green, ANSI::Background);

			// set the piece color
			if (board.at(x, y).type != PieceType::None){
				std::cout << ANSI::set24BitColor(pieceColor.red, pieceColor.green, pieceColor.blue, ANSI::Foreground);
			}

			// print the piece
			std::cout
				<< pieces.at({board.at(x, y).color, board.at(x, y).type})
				<< " ";
		}
		std::cout
			<< ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << ANSI::reset(ANSI::Foreground) << static_cast<int>(8-y) << " "
			<< ANSI::reset() << " \n";
	}
	std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << "  a b c d e f g h   " << ANSI::reset()  << "\n";	
}

void printHelp(std::string const& argv0){
	std::cout
		<< ANSI::reset() << "Usage: " << argv0 << " [options]\n"
		<< "Options:"
		<< "\n\t-h/--help\tPrints this helping text"
		<< "\n\t-i/\t\tPrint pieces in inverted colors"
		<< "\n";		
}

int main(int argc, const char** argv){
	ChessBot::Board board;
	Options options{false};

	int i = 1;
	while (i < argc){
		std::string arg = argv[i++];
		if (arg == "-h" || arg == "--help"){
			printHelp(argv[0]);
			return 0;
		}
		else if (arg == "-i"){
			options.invertedColors = true;
		}
	}


	//board.loadFromFEN("8/2p2p2/1pb2k1p/4R3/p3pK2/7P/2P3P1/8 w - - 0 36");
	board.loadFromFEN(ChessBot::Utils::startingFEN);

	printBoard(board, options);
	std::cout << "Enter move or type \"exit\".\n";
	while (true){
		std::cout << "Move: " << std::flush;

		std::string moveStr;
		std::cin >> moveStr;

		if (moveStr == "exit") break;
		
		ChessBot::Move move;
		if (!move.fromString(moveStr)){
			std::cout << "Invalid Move!\n";
			continue;
		}

		board.applyMove(move);
		printBoard(board, options);
	}

	std::cout << ANSI::reset();
	return 0;
}
