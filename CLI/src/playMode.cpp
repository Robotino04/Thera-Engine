#include "CLI/playMode.hpp"
#include "CLI/IO.hpp"

#include "CLI/popen2.hpp"

#include "Thera/Board.hpp"
#include "Thera/Move.hpp"
#include "Thera/MoveGenerator.hpp"

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
		{{PC::Black, PT::Pawn}, "♟"},
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

			const Thera::Coordinate square(x, y);
			
			if (squareHighlights.at(square.getIndex64()) != RGB::Black)
				boardColor = overlay(boardColor, squareHighlights.at(square.getIndex64()), highlightOpacity);

			// set the board color
			std::cout << ANSI::set24BitColor(boardColor.red, boardColor.green, boardColor.blue, ANSI::Background);

			// set the piece color
			const RGB pieceColor = board.at(square).getColor() == Thera::PieceColor::White ? whitePieceColor : blackPieceColor;
			if (board.at(square).getType() != Thera::PieceType::None){
				std::cout << ANSI::set24BitColor(pieceColor.red, pieceColor.green, pieceColor.blue, ANSI::Foreground);
			}

			// print the piece
			std::cout
				<< pieces.at({board.at(square).getColor(), board.at(square).getType()})
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
		Analyze,
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
	else if (buffer == "analyze"){
		handlePerftCommand(result, options);
		result.op = MoveInputResult::Analyze;
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

static void setBitboardHighlight(Thera::Bitboard const& bitboard, std::array<RGB, 64>& highlights){
	for (int8_t i=0; i<64; i++){
		if (bitboard[i])
			highlights.at(i) = highlightBitboardPresent;
	}
}

static void setBitboardHighlight(Options const& options, Thera::Board const& board, std::array<RGB, 64>& highlights){
	Thera::Bitboard bitboard;

	if (options.shownBitboard == Thera::Piece(Thera::PieceType::None, Thera::PieceColor::Black)){
		bitboard = board.getAllPieceBitboard();
	}
	else{
		bitboard = board.getBitboard(options.shownBitboard);
	}
	
	int i=0;
	for (Thera::Coordinate square : bitboard.getPieces()){
		if (i++ < bitboard.getNumPieces()){
			highlights.at(square.getIndex64()) = highlightBitboardPresent;
		}
	}
}

static void redrawGUI(Options const& options, Thera::Board const& board, std::array<RGB, 64>& highlights, std::string const& message){
	std::cout << ANSI::clearScreen() << ANSI::reset() << "-------------------\n" << message << ANSI::reset() << "\n";
	setBitboardHighlight(options, board, highlights);
	printBoard(board, highlights, options);
	highlights.fill(RGB());
}

/**
 * @brief Run perft and compare the result with stockfish.
 * 
 * @param userInput 
 * @param options 
 * @param message 
 */
static void analyzePosition(MoveInputResult const& userInput, Thera::Board& board, Thera::MoveGenerator& generator, std::string& message, int originalDepth){
	if (userInput.perftDepth == 0) return;
	if (userInput.perftDepth == originalDepth) message = "";

	std::string indentation;
	for (int i=0; i<originalDepth-userInput.perftDepth; i++)
		indentation += "\t";
	
	auto [in, out] = popen2("stockfish");

	const std::string positionCMD = "position fen " + board.storeToFEN() + "\n";
	const std::string perftCMD = "go perft " + std::to_string(userInput.perftDepth) + "\n";

	fputs(positionCMD.c_str(), in.get());
	fputs(perftCMD.c_str(), in.get());
	fputs("quit\n", in.get());
	fflush(in.get());

	std::vector<std::pair<Thera::Move, int>> stockfishMoves;
	int stockfishNodesSearched = 0;
	
	while (true){
		char c_buffer[1024];
		if (fgets(c_buffer, sizeof(c_buffer), out.get()) == nullptr) break;
		
		const std::string buffer = c_buffer;

		const std::string NODES_SEARCHED_TEXT = "Nodes searched: ";

		int i=0;
		if (Thera::Utils::isInRange(buffer.at(i++), 'a', 'h') &&
			Thera::Utils::isInRange(buffer.at(i++), '1', '8') && 
			Thera::Utils::isInRange(buffer.at(i++), 'a', 'h') &&
			Thera::Utils::isInRange(buffer.at(i++), '1', '8')
		){
			auto& movePair = stockfishMoves.emplace_back();
			auto& move = movePair.first;
			auto& numSubmoves = movePair.second;

			move.startIndex = Thera::Utils::squareFromAlgebraicNotation(buffer.substr(0, 2));
			move.endIndex = Thera::Utils::squareFromAlgebraicNotation(buffer.substr(2, 2));

			char promotion = buffer.at(i);
			switch(tolower(promotion)){
				case 'b': move.promotionType = Thera::PieceType::Bishop; i++; break;
				case 'n': move.promotionType = Thera::PieceType::Knight; i++; break;
				case 'r': move.promotionType = Thera::PieceType::Rook; i++; break;
				case 'q': move.promotionType = Thera::PieceType::Queen; i++; break;
			}

			if (buffer.at(i++) != ':') continue;
			if (buffer.at(i++) != ' ') continue;
			numSubmoves = stoi(buffer.substr(i));
		}
		else if (buffer.starts_with(NODES_SEARCHED_TEXT)){
			stockfishNodesSearched = stoi(buffer.substr(NODES_SEARCHED_TEXT.size()));
		}
	}

	std::vector<std::pair<Thera::Move, int>> theraMoves;
	const auto storeMove = [&](Thera::Move const& move, int numSubmoves){
		theraMoves.emplace_back(move, numSubmoves);
	};

	int theraNodesSearched = Thera::perft(board, generator, userInput.perftDepth, true, storeMove);
	
	enum class MoveSource{
		Thera,
		Stockfish
	};

	std::vector<std::tuple<Thera::Move, int, MoveSource>> differentMoves;

	

	for (auto movePair : theraMoves){
		auto const movePairCompare = [&](auto const& otherPair){
			return Thera::Move::isSameBaseMove(movePair.first, otherPair.first) && movePair.second == otherPair.second;
		};
		if (std::find_if(stockfishMoves.begin(), stockfishMoves.end(), movePairCompare) == stockfishMoves.end()){
			differentMoves.push_back(std::make_tuple(movePair.first, movePair.second, MoveSource::Thera));
		}
	}
	
	for (auto movePair : stockfishMoves){
		auto const movePairCompare = [&](auto const& otherPair){
			return Thera::Move::isSameBaseMove(movePair.first, otherPair.first) && movePair.second == otherPair.second;
		};
		if (std::find_if(theraMoves.begin(), theraMoves.end(), movePairCompare) == theraMoves.end()){
			differentMoves.push_back(std::make_tuple(movePair.first, movePair.second, MoveSource::Stockfish));
		}
	}

	std::sort(differentMoves.begin(), differentMoves.end(), [&](auto const& a, auto const& b){
		if (std::get<0>(a).startIndex != std::get<0>(b).startIndex)
			return std::get<0>(a).startIndex.getRaw() < std::get<0>(b).startIndex.getRaw();
		if (std::get<0>(a).endIndex != std::get<0>(b).endIndex)
			return std::get<0>(a).endIndex.getRaw() < std::get<0>(b).endIndex.getRaw();
		if (std::get<0>(a).promotionType != std::get<0>(b).promotionType)
			return static_cast<int>(std::get<0>(a).promotionType) < static_cast<int>(std::get<0>(b).promotionType);
		
		return std::get<1>(a) < std::get<1>(b);
	});

	for (auto [move, numSubmoves, source] : differentMoves){
		message += indentation + std::string("[") + (source == MoveSource::Thera ? "Thera]     " : "Stockfish] ");
		message += Thera::Utils::squareToAlgebraicNotation(move.startIndex) + Thera::Utils::squareToAlgebraicNotation(move.endIndex);
		switch (move.promotionType){
			case Thera::PieceType::Bishop: message += "b"; break;
			case Thera::PieceType::Knight: message += "n"; break;
			case Thera::PieceType::Rook:   message += "r"; break;
			case Thera::PieceType::Queen:  message += "q"; break;
			default: break;
		}
		message += ": " + std::to_string(numSubmoves) + "\n";
		if (userInput.perftDepth > 0){
			auto moveIt = std::find_if(theraMoves.begin(), theraMoves.end(), [&](auto other){return Thera::Move::isSameBaseMove(move, other.first);});
			if (moveIt == theraMoves.end()){
				message += indentation + "\tMove not found!\n";
			}
			else{
				MoveInputResult newUserInput = userInput;
				newUserInput.perftDepth--;
				board.applyMove(moveIt->first);
				analyzePosition(newUserInput, board, generator, message, originalDepth);
				board.rewindMove();
			}
		}
	}

	if (userInput.perftDepth != originalDepth) return;

	message += ANSI::set4BitColor(ANSI::Blue);
	message += indentation + "Stockfish searched " + std::to_string(stockfishMoves.size()) + " moves (" + std::to_string(stockfishNodesSearched) + " nodes)\n";
	message += indentation + "Thera searched " + std::to_string(theraMoves.size()) + " moves (" + std::to_string(theraNodesSearched) + " nodes)\n";

	message += indentation + "Results are ";
	if (differentMoves.size())
		message += indentation + ANSI::set4BitColor(ANSI::Red) + "different" + ANSI::set4BitColor(ANSI::Blue) +".\n";
	else
		message += indentation + ANSI::set4BitColor(ANSI::Green) + "identical" + ANSI::set4BitColor(ANSI::Blue) +".\n";
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
		else if (userInput.op == MoveInputResult::Analyze){
			analyzePosition(userInput, board, generator, message, userInput.perftDepth);
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

			message = "Nodes searched: " + std::to_string(nodesSearched) + "\n";

			continue;
		}
		else if (userInput.op == MoveInputResult::Continue)
			continue;

		auto possibleMoves = generator.generateMoves(board, userInput.move.startIndex);
		message = ANSI::set4BitColor(ANSI::Blue) + "Number of moves: " + std::to_string(possibleMoves.size());

		if (options.shownBitboard == Thera::Piece(Thera::PieceType::None, Thera::PieceColor::White)){
			// highlight all moves
			highlights.at(userInput.move.startIndex.getIndex64()) = highlightSquareSelected;
			for (auto const& move : possibleMoves){
				highlights.at(move.endIndex.getIndex64()) = highlightMovePossible;
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
				// won't have any stats attached
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