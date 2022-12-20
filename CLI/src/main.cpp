#include "ChessBot/Board.hpp"
#include "ChessBot/Utils.hpp"

#include "ANSI/ANSI.hpp"

#include <iostream>
#include <map>
#include <stdint.h>

struct RGB{
	uint8_t red, green, blue;
};

void printBoard(ChessBot::Board const& board){
	using namespace ChessBot;

	static const RGB whiteBoardColor = {255, 210, 153};
	static const RGB blackBoardColor = {130, 77, 39};
	static const RGB whitePieceColor = {255, 255, 255};
	static const RGB blackPieceColor = {0, 0, 0};

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
	
	for(uint8_t y=0; y<8; y++){
		for(uint8_t x=0; x<8; x++){
			const RGB boardColor = (x + y)%2 ? whiteBoardColor : blackBoardColor;

			// set the board color
			std::cout << ANSI::set24BitColor(boardColor.red, boardColor.green, boardColor.green, ANSI::Background);

			// set the piece color
			if (board.at(x, y).type != PieceType::None){
				const RGB pieceColor = board.at(x, y).color == PieceColor::White ? blackPieceColor : blackPieceColor;
				std::cout << ANSI::set24BitColor(pieceColor.red, pieceColor.green, pieceColor.blue, ANSI::Foreground);
			}

			// print the piece
			std::cout
				<< pieces.at({board.at(x, y).color, board.at(x, y).type})
				<< " ";
		}
		std::cout << ANSI::reset() << " \n";
	}
}

int main(int argc, const char** argv){ ChessBot::Board board;
	board.loadFromFEN("8/2p2p2/1pb2k1p/4R3/p3pK2/7P/2P3P1/8 w - - 0 36");

	printBoard(board);

	// for (int i=0;i<8;i++){
	// 	std::cout << ANSI::set24BitColor(255, 210, 153, ANSI::Background) << "                " << ANSI::reset(ANSI::Background) << "\n";
	// }
	// for (int i=0;i<8;i++){
	// 	std::cout << ANSI::set24BitColor(130, 77, 39, ANSI::Background) << "                " << ANSI::reset(ANSI::Background) << "\n";
	// }

	std::cout << ANSI::reset();
	return 0;
}
