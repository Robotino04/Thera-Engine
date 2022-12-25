#include "ChessBot/Board.hpp"
#include "ChessBot/Utils.hpp"
#include "ChessBot/Move.hpp"
#include "ChessBot/MoveGenerator.hpp"

#include "ANSI/ANSI.hpp"

#include <iostream>
#include <map>
#include <stdint.h>
#include <stdexcept>

struct RGB{
	uint8_t red = 0, green = 0, blue = 0;

	bool operator ==(RGB const& other) const{
		return this->red == other.red && this->green == other.green && this->blue == other.blue;
	}

	bool operator !=(RGB const& other) const{
		return ! (*this == other);
	}

	static const RGB Black;
};
const RGB RGB::Black = {0,0,0};

struct Options {
	bool invertedColors = false;
};

RGB overlay(RGB base, RGB transparent, float opacity){
	return {
		static_cast<uint8_t>((1.0f-opacity)*static_cast<float>(base.red) + opacity*static_cast<float>(transparent.red)),
		static_cast<uint8_t>((1.0f-opacity)*static_cast<float>(base.green) + opacity*static_cast<float>(transparent.green)),
		static_cast<uint8_t>((1.0f-opacity)*static_cast<float>(base.blue) + opacity*static_cast<float>(transparent.blue))
	};
}

static const float highlightOpacity = 0.5;
static const RGB highlightMovePossible = {82, 255, 220};
static const RGB highlightSquareSelected = {247, 92, 255};

void printBoard(ChessBot::Board const& board, std::array<RGB, 64> const& squareHighlights, Options const& options){
	using namespace ChessBot;

	const RGB whiteBoardColor = {255, 210, 153};
	const RGB blackBoardColor = {130, 77, 39};
	const RGB whitePieceColor = {120, 120, 120};
	const RGB blackPieceColor = {0, 0, 0};

	static const std::map<std::pair<PieceColor, PieceType>, std::string> pieces = {
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
			RGB boardColor = (x + y)%2 ? blackBoardColor : whiteBoardColor;
			
			if (squareHighlights.at(Utils::coordToIndex(x, y)) != RGB::Black)
				boardColor = overlay(boardColor, squareHighlights.at(Utils::coordToIndex(x, y)), highlightOpacity);

			// set the board color
			std::cout << ANSI::set24BitColor(boardColor.red, boardColor.green, boardColor.blue, ANSI::Background);

			// set the piece color
			const RGB pieceColor = board.at(x, y).getColor() == PieceColor::White ? whitePieceColor : blackPieceColor;
			if (board.at(x, y).getType() != PieceType::None){
				std::cout << ANSI::set24BitColor(pieceColor.red, pieceColor.green, pieceColor.blue, ANSI::Foreground);
			}

			// print the piece
			std::cout
				<< pieces.at({board.at(x, y).getColor(), board.at(x, y).getType()})
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


	ChessBot::Board board;
	//board.loadFromFEN("8/2p2p2/1pb2k1p/4R3/p3pK2/7P/2P3P1/8 w - - 0 36");
	board.loadFromFEN("1qbnr1k1/7p/8/8/8/8/P7/1K1RNBQ1 w - - 0 1");

	std::array<RGB, 64> highlights;
	highlights.fill(RGB());

	printBoard(board, highlights, options);

	ChessBot::MoveGenerator generator;

	std::cout << "Enter move or type \"exit\".\n";
	std::cout << "Change your move by typing \"other\".\n";
	while (true){
		std::string moveStr;
		ChessBot::Move userMove;
		
		while(true){
			std::cout << "Move start: " << std::flush;
			std::cin >> moveStr;
			if (moveStr == "exit") goto exit;
			try{
				userMove.startIndex = ChessBot::Utils::squareFromString(moveStr);
				break;
			}
			catch (std::invalid_argument){
				std::cout << "Invalid Move!\n";
			}
		}


		auto possibleMoves = generator.generateMoves(board, userMove.startIndex);
		std::cout << "Number of moves: " << possibleMoves.size() << "\n";


		// highlight all moves
		highlights.at(userMove.startIndex) = highlightSquareSelected;
		for (auto const& move : possibleMoves){
			highlights.at(move.endIndex) = highlightMovePossible;
		}
		printBoard(board, highlights, options);

		while(true){
			std::cout << "Move end: " << std::flush;
			std::cin >> moveStr;
			if (moveStr == "exit") goto exit;
			if (moveStr == "other") goto skipMove;
			try{
				userMove.endIndex = ChessBot::Utils::squareFromString(moveStr);
				break;
			}
			catch (std::invalid_argument){
				std::cout << "Invalid Move!\n";
			}
		}

		board.applyMove(userMove);
	skipMove:
		highlights.fill(RGB());
		printBoard(board, highlights, options);
	}
exit:

	std::cout << "Bye...\n" << ANSI::reset();
	return 0;
}
