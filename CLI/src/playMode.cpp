#include "CLI/playMode.hpp"
#include "CLI/IO.hpp"

#include "ChessBot/Board.hpp"
#include "ChessBot/Move.hpp"
#include "ChessBot/MoveGenerator.hpp"

#include "ChessBot/Utils/Coordinates.hpp"
#include "ChessBot/Utils/ChessTerms.hpp"

#include "ANSI/ANSI.hpp"

#include <array>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <functional>

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
			<< ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << ANSI::reset(ANSI::Foreground) << static_cast<int>(8-y) << " " << ANSI::reset();

		// print board stats
		switch(y){
			case 0:
				std::cout << "  Castling: [White] [Black]";
				break;
			case 1:
				std::cout << "            ";
				std::cout << setConditionalColor(board.getCastleLeft(ChessBot::PieceColor::White), ANSI::Background) << "[Q]";
				std::cout << ANSI::reset(ANSI::Background) << " ";
				std::cout << setConditionalColor(board.getCastleRight(ChessBot::PieceColor::White), ANSI::Background) << "[K]";
				std::cout << ANSI::reset(ANSI::Background) << " ";
				std::cout << setConditionalColor(board.getCastleLeft(ChessBot::PieceColor::Black), ANSI::Background) << "[Q]";
				std::cout << ANSI::reset(ANSI::Background) << " ";
				std::cout << setConditionalColor(board.getCastleRight(ChessBot::PieceColor::Black), ANSI::Background) << "[K]";

				break;
			default:
				break;
		}

		std::cout << ANSI::reset() << " \n";
	}
	std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << "  a b c d e f g h   " << ANSI::reset()  << "\n";	
}

struct MoveInputResult{
	ChessBot::Move move;
	bool force = false;
	bool redo = false;
	bool exit = false;
};

void getUserMoveStart(MoveInputResult& result){
	std::string buffer;
	while(true){
		std::cout << "Move start: " << std::flush;
		std::cin >> buffer;
		if (buffer == "exit"){
			result.exit = true;
			return;
		}
		else{
			try{
				result.move.startIndex = ChessBot::Utils::squareFromAlgebraicNotation(buffer.substr(0, 4));
				break;
			}
			catch (std::invalid_argument){
				std::cout << "Invalid move!\n";
			}
		}
	}
}

void getUserMoveEnd(MoveInputResult& result){
	std::string buffer;
	while(true){
		std::cout << "Move end: " << std::flush;
		std::cin >> buffer;
		if (buffer == "exit"){
			result.exit = true;
			return;
		}
		else if (buffer == "redo"){
			result.redo = true;
			return;
		}
		else{
			if (buffer.at(buffer.size() - 1) == 'F'){
				result.force = true;
			}
			try{
				result.move.endIndex = ChessBot::Utils::squareFromAlgebraicNotation(buffer.substr(0, 4));
				return;
			}
			catch (std::invalid_argument){
				std::cout << "Invalid move!\n";
			}
		}
	}
}

int playMode(Options& options){
	ChessBot::Board board;
	board.loadFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
	// board.loadFromFEN("1qbnr1k1/7p/8/8/8/8/P7/1K1RNBQ1 w - - 0 1");
	// board.loadFromFEN(ChessBot::Utils::startingFEN);

	std::array<RGB, 64> highlights;
	highlights.fill(RGB());

	printBoard(board, highlights, options);

	ChessBot::MoveGenerator generator;

	std::cout << "Enter move or type \"exit\".\n";
	std::cout << "Change your move by typing \"other\".\n";
	while (true){
		MoveInputResult userInput;
		
		getUserMoveStart(userInput);
		if (userInput.exit) break;

		auto possibleMoves = generator.generateMoves(board, userInput.move.startIndex);
		std::cout << "Number of moves: " << possibleMoves.size() << "\n";


		// highlight all moves
			highlights.at(userInput.move.startIndex) = highlightSquareSelected;
		for (auto const& move : possibleMoves){
			highlights.at(move.endIndex) = highlightMovePossible;
		}
		printBoard(board, highlights, options);


		getUserMoveEnd(userInput);

		if (userInput.force){
			if (board.getColorToMove() == board.at(userInput.move.startIndex).getColor())
				board.applyMove(userInput.move);
			else
				board.applyMoveStatic(userInput.move);
		}
		else{
			auto moveIt = std::find_if(possibleMoves.begin(), possibleMoves.end(), [&](auto other){return ChessBot::Move::isSameBaseMove(userInput.move, other);});
			if (moveIt != possibleMoves.end()){
				// apply the found move since the user move
				// won't have any auxiliary moves attached
				board.applyMove(*moveIt);
			}
			else{
				std::cout << "Impossible move!\n";
			}
		}
		
		highlights.fill(RGB());
		printBoard(board, highlights, options);
	}

	std::cout << "Bye...\n" << ANSI::reset();
	return 0;
}