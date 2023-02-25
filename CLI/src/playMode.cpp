#include "CLI/playMode.hpp"
#include "CLI/IO.hpp"

#include "Thera/Board.hpp"
#include "Thera/Move.hpp"
#include "Thera/MoveGenerator.hpp"

#include "Thera/Utils/Coordinates.hpp"
#include "Thera/Utils/ChessTerms.hpp"

#include "Thera/perft.hpp"

#include "ANSI/ANSI.hpp"

#include <array>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <functional>
#include <fstream>

static const float highlightOpacity = 0.5;
static const RGB highlightMovePossible = {82, 255, 220};
static const RGB highlightSquareSelected = {247, 92, 255};
static const RGB highlightBitboardPresent = {255, 242, 0};

static void printBoard(Thera::Board const& board, std::array<RGB, 64> const& squareHighlights, Options const& options){

	const RGB whiteBoardColor = {255, 210, 153};
	const RGB blackBoardColor = {130, 77, 39};
	const RGB whitePieceColor = {120, 120, 120};
	const RGB blackPieceColor = {0, 0, 0};

	using PC = Thera::PieceColor;
	using PT = Thera::PieceType;
	static const std::map<std::pair<PC, PT>, std::string> pieces = {
		{{PC::White, PT::None}, " "},
		{{PC::Black, PT::None}, " "},
		{{PC::White, PT::Pawn}, "♙"},
		{{PC::Black, PT::Pawn}, "♟︎"},
		{{PC::White, PT::Bishop}, "♗"},
		{{PC::Black, PT::Bishop}, "♝"},
		{{PC::White, PT::Knight}, "♘"},
		{{PC::Black, PT::Knight}, "♞"},
		{{PC::White, PT::Rook}, "♖"},
		{{PC::Black, PT::Rook}, "♜"},
		{{PC::White, PT::Queen}, "♕"},
		{{PC::Black, PT::Queen}, "♛"},
		{{PC::White, PT::King}, "♔"},
		{{PC::Black, PT::King}, "♚"},
	};

	std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << "  a b c d e f g h   " << ANSI::reset()  << "\n";	
	for(uint8_t y=0; y<8; y++){
		std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << static_cast<int>(8-y) << " ";
		for(uint8_t x=0; x<8; x++){
			RGB boardColor = (x + y)%2 ? blackBoardColor : whiteBoardColor;
			
			if (squareHighlights.at(Thera::Utils::coordToIndex(x, y).pos) != RGB::Black)
				boardColor = overlay(boardColor, squareHighlights.at(Thera::Utils::coordToIndex(x, y).pos), highlightOpacity);

			// set the board color
			std::cout << ANSI::set24BitColor(boardColor.red, boardColor.green, boardColor.blue, ANSI::Background);

			// set the piece color
			const RGB pieceColor = board.at(Thera::Utils::coordToIndex(x, y)).getColor() == Thera::PieceColor::White ? whitePieceColor : blackPieceColor;
			if (board.at(Thera::Utils::coordToIndex(x, y)).getType() != Thera::PieceType::None){
				std::cout << ANSI::set24BitColor(pieceColor.red, pieceColor.green, pieceColor.blue, ANSI::Foreground);
			}

			// print the piece
			std::cout
				<< pieces.at({board.at(Thera::Utils::coordToIndex(x, y)).getColor(), board.at(Thera::Utils::coordToIndex(x, y)).getType()})
				<< " ";
		}
		std::cout
			<< ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << ANSI::reset(ANSI::Foreground) << static_cast<int>(8-y) << " " << ANSI::reset();

		// print board stats
		std::cout << "  ";
		switch(y){
			case 0:
				std::cout << "Castling: [White] [Black]";
				break;
			case 1:
				std::cout << "          ";
				std::cout << setConditionalColor(board.getCurrentState().canWhiteCastleLeft, ANSI::Background) << "[Q]";
				std::cout << ANSI::reset(ANSI::Background) << " ";
				std::cout << setConditionalColor(board.getCurrentState().canWhiteCastleRight, ANSI::Background) << "[K]";
				std::cout << ANSI::reset(ANSI::Background) << " ";
				std::cout << setConditionalColor(board.getCurrentState().canBlackCastleLeft, ANSI::Background) << "[Q]";
				std::cout << ANSI::reset(ANSI::Background) << " ";
				std::cout << setConditionalColor(board.getCurrentState().canBlackCastleRight, ANSI::Background) << "[K]";

				break;
			case 3:
				if (options.shownBitboard.getType() != Thera::PieceType::None){
					std::cout << "Showing bitboard for " << Thera::Utils::pieceToString(options.shownBitboard, true);
				}
				else{
					std::cout << "Showing no bitboard";
				}
				break;
			case 5:
				std::cout << "FEN: " << ANSI::set4BitColor(ANSI::Blue, ANSI::Foreground) << board.storeToFEN() << ANSI::reset(ANSI::Foreground);
			default:
				break;
		}

		std::cout << ANSI::reset() << "\n";
	}
	
	std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << "  a b c d e f g h   " << ANSI::reset()  << "\n";	
}

struct MoveInputResult{
	enum OperationType{
		MakeMove,
		UndoMove,
		Continue,
		ForceMove,
		Perft,
		Exit,
		LoadFEN,
	};

	Thera::Move move;
	OperationType op = OperationType::MakeMove;
	int perftDepth=0;
	bool failed = false;
	std::string message;
};

static void handleShowCommand(MoveInputResult& result, Options& options){
	static const std::map<std::string, Thera::PieceType> stringToPieceType = {
		{"p", Thera::PieceType::Pawn},
		{"b", Thera::PieceType::Bishop},
		{"n", Thera::PieceType::Knight},
		{"r", Thera::PieceType::Rook},
		{"q", Thera::PieceType::Queen},
		{"k", Thera::PieceType::King},

		{"pawn", 	Thera::PieceType::Pawn},
		{"bishop", 	Thera::PieceType::Bishop},
		{"knight", 	Thera::PieceType::Knight},
		{"rook", 	Thera::PieceType::Rook},
		{"queen", 	Thera::PieceType::Queen},
		{"king", 	Thera::PieceType::King},
	};
	static const std::map<std::string, Thera::PieceColor> stringToPieceColor = {
		{"white", 	Thera::PieceColor::White},
		{"black", 	Thera::PieceColor::Black},
		{"w", 		Thera::PieceColor::White},
		{"b", 		Thera::PieceColor::Black},
	};
	result.op = MoveInputResult::Continue;

	std::string buffer;

	// parse color
	std::cin >> buffer;
	if (buffer == "none"){
		options.shownBitboard.setType(Thera::PieceType::None);
		options.shownBitboard.setColor(Thera::PieceColor::White);
		return;
	}
	else if (buffer == "all"){
		options.shownBitboard.setType(Thera::PieceType::None);
		options.shownBitboard.setColor(Thera::PieceColor::Black);
		return;
	}

	try{
		options.shownBitboard.setColor(stringToPieceColor.at(buffer));
	}
	catch(std::out_of_range){
		result.message = "Invalid color \"" + buffer + "\"!";
		result.failed = true;
		return;
	}

	// parse type
	std::cin >> buffer;
	try{
		options.shownBitboard.setType(stringToPieceType.at(buffer));
	}
	catch(std::out_of_range){
		result.message = "Invalid piece \"" + buffer + "\"!";
		result.failed = true;
		return;
	}
}

static void handlePerftCommand(MoveInputResult& result, Options& options){
	std::string buffer;

	result.op = MoveInputResult::Perft;

	// parse depth
	try{
		std::cin >> buffer;
		result.perftDepth = std::stoi(buffer);
	}
	catch(std::invalid_argument){
		result.message = "Invalid depth \"" + buffer + "\"!";
		result.op = MoveInputResult::Continue;
		result.failed = true;
		return;
	}
}

static void handleFenCommand(MoveInputResult& result, Options& options){
	result.op = MoveInputResult::LoadFEN;

	std::string buffer;
	std::getline(std::cin, buffer);
	buffer = buffer.substr(1); // remove the leading space

	try{
		Thera::Board testBoard;
		testBoard.loadFromFEN(buffer);
		options.fen = buffer;
	}
	catch(std::invalid_argument){
		result.message = "Invalid FEN string: \"" + buffer + "\"";
		result.op = MoveInputResult::Continue;
		result.failed = true;
		return;
	}
	
}

static void getUserMoveStart(MoveInputResult& result, Options& options){
	std::string buffer;
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
	else if (buffer == "perft"){
		handlePerftCommand(result, options);
		return;
	}
	else if (buffer == "fen"){
		handleFenCommand(result, options);
		return;
	}
	else{
		try{
			result.move.startIndex = Thera::Utils::squareFromAlgebraicNotation(buffer.substr(0, 2));
			return;
		}
		catch (std::invalid_argument){
			result.message = "Invalid command or move!";
			result.failed = true;
			return;
		}
	}
	result.failed = true;
	result.message = "Invalid command or move!";
}

static void getUserMoveEnd(MoveInputResult& result, Options& options){
	std::string buffer;
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
	else{
		if (buffer.at(buffer.size() - 1) == 'F'){
			result.op = MoveInputResult::ForceMove;
		}
		try{
			result.move.endIndex = Thera::Utils::squareFromAlgebraicNotation(buffer.substr(0, 2));
			return;
		}
		catch (std::invalid_argument){
			result.message = "Invalid command or move!";
			result.failed = true;
			return;
		}
	}
	result.failed = true;
	result.message = "Invalid command or move!";
}

template<int N>
static void setBitboardHighlight(Thera::Bitboard<N> const& bitboard, std::array<RGB, 64>& highlights){
	for (int8_t i=0; i<64; i++){
		if (bitboard[Thera::Coordinate8x8(i)] && Thera::Utils::isOnBoard(Thera::Coordinate8x8(i)))
			highlights.at(i) = highlightBitboardPresent;
	}
}

static void setBitboardHighlight(Options const& options, Thera::Board const& board, std::array<RGB, 64>& highlights){
	if (options.shownBitboard.getType() != Thera::PieceType::None || options.shownBitboard.getColor() != Thera::PieceColor::Black){
		setBitboardHighlight(board.getBitboard(options.shownBitboard), highlights);
	}
	else if (options.shownBitboard == Thera::Piece(Thera::PieceType::None, Thera::PieceColor::Black)){
		setBitboardHighlight(board.getAllPieceBitboard(), highlights);
	}
}

static void redrawGUI(Options const& options, Thera::Board const& board, std::array<RGB, 64>& highlights, std::string const& message){
	std::cout << ANSI::clearScreen() << message << ANSI::reset() << "\n";
	setBitboardHighlight(options, board, highlights);
	printBoard(board, highlights, options);
	highlights.fill(RGB());
}

int playMode(Options& options){
	Thera::Board board;
	board.loadFromFEN(options.fen);

	std::array<RGB, 64> highlights;
	highlights.fill(RGB());
	options.shownBitboard = Thera::Piece(Thera::PieceType::None, Thera::PieceColor::White);

	Thera::MoveGenerator generator;

	std::string message =  	"Enter move or type 'exit'.\n"
							"Change your move by typing 'change'.\n"
							"Undo last move using 'undo'.";
	while (true){
		MoveInputResult userInput;

		redrawGUI(options, board, highlights, message);
		
		getUserMoveStart(userInput, options);
		
		if (userInput.failed){
			message = ANSI::set4BitColor(ANSI::Red) + userInput.message + ANSI::reset();
			continue;
		}

		if (userInput.op == MoveInputResult::Exit) break;
		else if (userInput.op == MoveInputResult::UndoMove){
			try{
				board.rewindMove();
				message = ANSI::set4BitColor(ANSI::Blue) + "Undid move." + ANSI::reset();
			}
			catch(std::runtime_error){
				message = ANSI::set4BitColor(ANSI::Red) + "No move to undo." + ANSI::reset();
			}
			continue;
		}
		else if (userInput.op == MoveInputResult::LoadFEN){
			board.loadFromFEN(options.fen);
			message = ANSI::set4BitColor(ANSI::Blue) + "Loaded position from FEN." + ANSI::reset();
			continue;
		}
		else if (userInput.op == MoveInputResult::Perft){
			const auto messageLoggingMovePrint = [&](Thera::Move const& move, int numSubmoves){
				message
					+= Thera::Utils::squareToAlgebraicNotation(move.startIndex)
					+  Thera::Utils::squareToAlgebraicNotation(move.endIndex);
				switch (move.promotionType){
					case Thera::PieceType::Bishop: message += "b"; break;
					case Thera::PieceType::Knight: message += "n"; break;
					case Thera::PieceType::Rook:   message += "r"; break;
					case Thera::PieceType::Queen:  message += "q"; break;
					default: break;
				}
				message += ": " + std::to_string(numSubmoves) + "\n";
			};

			message = "";

			int nodesSearched = Thera::perft(board, generator, userInput.perftDepth, true, messageLoggingMovePrint);

			// write the perft output to a file for easier debugging
			std::ofstream logFile("/tmp/thera.txt", std::ofstream::trunc);
			if (!logFile.is_open()){
				message += ANSI::set4BitColor(ANSI::Red) + "Unable to open logfile! Ignoring." + ANSI::reset(ANSI::Foreground) + "\n";
			}
			else{
				logFile << message;
				logFile.close();
			}

			message = "-------------------\n" + message + "Nodes searched: " + std::to_string(nodesSearched) + "\n";

			continue;
		}
		else if (userInput.op == MoveInputResult::Continue)
			continue;

		auto possibleMoves = generator.generateMoves(board, userInput.move.startIndex);
		message = ANSI::set4BitColor(ANSI::Blue) + "Number of moves: " + std::to_string(possibleMoves.size());

		if (options.shownBitboard == Thera::Piece(Thera::PieceType::None, Thera::PieceColor::White)){
			// highlight all moves
			highlights.at(((Thera::Coordinate8x8)userInput.move.startIndex).pos) = highlightSquareSelected;
			for (auto const& move : possibleMoves){
				highlights.at(((Thera::Coordinate8x8)move.endIndex).pos) = highlightMovePossible;
			}
		}

		redrawGUI(options, board, highlights, message);
		
		getUserMoveEnd(userInput, options);
		if (userInput.op == MoveInputResult::Continue)
			continue;
		else if (userInput.op == MoveInputResult::ForceMove){
			if (board.getColorToMove() == board.at(userInput.move.startIndex).getColor())
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
			auto moveIt = std::find_if(possibleMoves.begin(), possibleMoves.end(), [&](auto other){return Thera::Move::isSameBaseMove(userInput.move, other);});
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