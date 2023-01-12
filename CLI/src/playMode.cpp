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
static const RGB highlightBitboardPresent = {255, 242, 0};

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
			case 2:
				if (options.shownBitboard.getType() != ChessBot::PieceType::None){
					std::cout << "  Showing bitboard for " << ChessBot::Utils::pieceToString(options.shownBitboard, true);
				}
				break;
			default:
				break;
		}

		std::cout << ANSI::reset() << " \n";
	}
	std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << "  a b c d e f g h   " << ANSI::reset()  << "\n";	
}

struct MoveInputResult{
	enum OperationType{
		MakeMove,
		UndoMove,
		Continue,
		ForceMove,
		Exit,
	};

	ChessBot::Move move;
	OperationType op = OperationType::MakeMove;
};

void handleShowCommand(MoveInputResult& result, Options& options){
	static const std::map<std::string, ChessBot::PieceType> stringToPieceType = {
		{"p", ChessBot::PieceType::Pawn},
		{"b", ChessBot::PieceType::Bishop},
		{"n", ChessBot::PieceType::Knight},
		{"r", ChessBot::PieceType::Rook},
		{"q", ChessBot::PieceType::Queen},
		{"k", ChessBot::PieceType::King},

		{"pawn", 	ChessBot::PieceType::Pawn},
		{"bishop", 	ChessBot::PieceType::Bishop},
		{"knight", 	ChessBot::PieceType::Knight},
		{"rook", 	ChessBot::PieceType::Rook},
		{"queen", 	ChessBot::PieceType::Queen},
		{"king", 	ChessBot::PieceType::King},
	};
	static const std::map<std::string, ChessBot::PieceColor> stringToPieceColor = {
		{"white", 	ChessBot::PieceColor::White},
		{"black", 	ChessBot::PieceColor::Black},
		{"w", 		ChessBot::PieceColor::White},
		{"b", 		ChessBot::PieceColor::Black},
	};
	result.op = MoveInputResult::Continue;

	std::string buffer;

	// parse color
	std::cin >> buffer;
	if (buffer == "none"){
		options.shownBitboard.setType(ChessBot::PieceType::None);
		return;
	}

	try{
		options.shownBitboard.setColor(stringToPieceColor.at(buffer));
	}
	catch(std::out_of_range){
		std::cout << "Invalid piece \"" << buffer << "\"!\n";
	}

	// parse type
	std::cin >> buffer;
	try{
		options.shownBitboard.setType(stringToPieceType.at(buffer));
	}
	catch(std::out_of_range){
		std::cout << "Invalid piece \"" << buffer << "\"!\n";
	}
}

void getUserMoveStart(MoveInputResult& result, Options& options){
	std::string buffer;
	while(true){
		std::cout << "Move start: " << std::flush;
		std::cin >> buffer;
		if (buffer == "exit"){
			result.op = MoveInputResult::Exit;
			return;
		}
		else if (buffer == "undo"){
			result.op = MoveInputResult::UndoMove;
			return;
		}
		else if (buffer == "show"){
			handleShowCommand(result, options);
			return;
		}
		else{
			try{
				result.move.startIndex = ChessBot::Board::to10x12Coords(ChessBot::Utils::squareFromAlgebraicNotation(buffer.substr(0, 2)));
				break;
			}
			catch (std::invalid_argument){
				std::cout << "Invalid move!\n";
			}
		}
	}
}

void getUserMoveEnd(MoveInputResult& result, Options& options){
	std::string buffer;
	while(true){
		std::cout << "Move end: " << std::flush;
		std::cin >> buffer;
		if (buffer == "exit"){
			result.op = MoveInputResult::Exit;
			return;
		}
		else if (buffer == "change"){
			result.op = MoveInputResult::Continue;
			return;
		}
		else if (buffer == "undo"){
			result.op = MoveInputResult::UndoMove;
			return;
		}
		else if (buffer == "show"){
			handleShowCommand(result, options);
			return;
		}
		else{
			if (buffer.at(buffer.size() - 1) == 'F'){
				result.op = MoveInputResult::ForceMove;
			}
			try{
				result.move.endIndex = ChessBot::Board::to10x12Coords(ChessBot::Utils::squareFromAlgebraicNotation(buffer.substr(0, 2)));
				return;
			}
			catch (std::invalid_argument){
				std::cout << "Invalid move!\n";
			}
		}
	}
}

void setBitboardHighlight(Options const& options, ChessBot::Board const& board, std::array<RGB, 64>& highlights){
	if (options.shownBitboard.getType() != ChessBot::PieceType::None){
		// highlight bitboard moves
		for (auto square : board.getBitboard(options.shownBitboard)){
			if (ChessBot::Board::isOnBoard10x12(square))
				highlights.at(ChessBot::Board::to8x8Coords(square)) = highlightBitboardPresent;
		}
	}
}

void redrawGUI(Options const& options, ChessBot::Board const& board, std::array<RGB, 64>& highlights, std::string const& message){
	std::cout << ANSI::clearScreen() << message << ANSI::reset() << "\n";
	setBitboardHighlight(options, board, highlights);
	printBoard(board, highlights, options);
	highlights.fill(RGB());
}

int playMode(Options& options){
	ChessBot::Board board;
	board.loadFromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1");
	// board.loadFromFEN("1qbnr1k1/7p/8/8/8/8/P7/1K1RNBQ1 w - - 0 1");
	// board.loadFromFEN(ChessBot::Utils::startingFEN);

	std::array<RGB, 64> highlights;
	highlights.fill(RGB());
	options.shownBitboard = ChessBot::Piece(ChessBot::PieceType::Rook, ChessBot::PieceColor::White);

	ChessBot::MoveGenerator generator;

	std::string message =  	"Enter move or type 'exit'.\n"
							"Change your move by typing 'change'.\n"
							"Undo last move using 'undo'.";
	while (true){
		MoveInputResult userInput;

		redrawGUI(options, board, highlights, message);
		
		getUserMoveStart(userInput, options);
		if (userInput.op == MoveInputResult::Exit) break;
		else if (userInput.op == MoveInputResult::UndoMove){
			board.rewindMove();
			message = ANSI::set4BitColor(ANSI::Blue) + "Undone move." + ANSI::reset();
			continue;
		}
		else if (userInput.op == MoveInputResult::Continue)
			continue;

		auto possibleMoves = generator.generateMoves(board, ChessBot::Board::to8x8Coords(userInput.move.startIndex));
		message = ANSI::set4BitColor(ANSI::Blue) + "Number of moves: " + std::to_string(possibleMoves.size());

		if (options.shownBitboard.getType() == ChessBot::PieceType::None){
			// highlight all moves
			highlights.at(ChessBot::Board::to8x8Coords(userInput.move.startIndex)) = highlightSquareSelected;
			for (auto const& move : possibleMoves){
				highlights.at(ChessBot::Board::to8x8Coords(move.endIndex)) = highlightMovePossible;
			}
		}

		redrawGUI(options, board, highlights, message);
		
		getUserMoveEnd(userInput, options);
		if (userInput.op == MoveInputResult::Continue)
			continue;
		else if (userInput.op == MoveInputResult::ForceMove){
			if (board.getColorToMove() == board.at10x12(userInput.move.startIndex).getColor())
				board.applyMove(userInput.move);
			else
				board.applyMoveStatic(userInput.move);
			message = ANSI::set4BitColor(ANSI::Blue) + "Forced move." + ANSI::reset();
		}
		else if (userInput.op == MoveInputResult::UndoMove){
			board.rewindMove();
			message = ANSI::set4BitColor(ANSI::Blue) + "Undone move." + ANSI::reset();
		}
		else{
			auto moveIt = std::find_if(possibleMoves.begin(), possibleMoves.end(), [&](auto other){return ChessBot::Move::isSameBaseMove(userInput.move, other);});
			if (moveIt != possibleMoves.end()){
				// apply the found move since the user move
				// won't have any auxiliary moves attached
				board.applyMove(*moveIt);
			}
			else{
				message = ANSI::set4BitColor(ANSI::Red) + "Invalid move!" + ANSI::reset();
			}
		}
	}

	std::cout << ANSI::reset() << "Bye...\n";
	return 0;
}