#include "CLI/playMode.hpp"
#include "CLI/IO.hpp"

#include "CLI/popen2.hpp"

#include "Thera/Board.hpp"
#include "Thera/Move.hpp"
#include "Thera/MoveGenerator.hpp"

#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Utils/exceptions.hpp"
#include "Thera/Utils/GitInfo.hpp"

#include "Thera/perft.hpp"
#include "Thera/search.hpp"

#include "ANSI/ANSI.hpp"

#include <array>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <functional>
#include <fstream>
#include <bitset>
#include <unordered_set>
#include <chrono>
#include <sstream>

static const float highlightOpacity = 0.5;
static const RGB highlightMovePossible = {82, 255, 220};
static const RGB highlightSquareSelected = {247, 92, 255};
static const RGB highlightBitboardPresent = {255, 242, 0};

static void printBoard(Thera::Board const& board, std::array<RGB, 64> const& squareHighlights, Options const& options){

	const RGB whiteBoardColor = {255, 210, 153};
	const RGB blackBoardColor = {130, 77, 39};
	const RGB whitePieceColorOnWhiteSquare = {80, 80, 80};
	const RGB whitePieceColorOnBlackSquare = {180, 180, 180};
	const RGB blackPieceColor = {0, 0, 0};

	using PC = Thera::PieceColor;
	using PT = Thera::PieceType;
	static const std::map<Thera::Piece, std::string> pieces = {
		{{PT::None, PC::White}, " "},
		{{PT::None, PC::Black}, " "},
		{{PT::Pawn, PC::White}, "♙"},
		{{PT::Pawn, PC::Black}, "♟"},
		{{PT::Bishop, PC::White}, "♗"},
		{{PT::Bishop, PC::Black}, "♝"},
		{{PT::Knight, PC::White}, "♘"},
		{{PT::Knight, PC::Black}, "♞"},
		{{PT::Rook, PC::White}, "♖"},
		{{PT::Rook, PC::Black}, "♜"},
		{{PT::Queen, PC::White}, "♕"},
		{{PT::Queen, PC::Black}, "♛"},
		{{PT::King, PC::White}, "♔"},
		{{PT::King, PC::Black}, "♚"},
	};

	std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << "  a b c d e f g h   " << ANSI::reset();	
	// print the current git commit
	std::cout << ANSI::set8BitColor(93) << "  ----------| Thera (Git ";
	std::cout << Thera::Utils::GitInfo::hash;
	if (Thera::Utils::GitInfo::isDirty)
		std::cout << " + local changes";
	std::cout << ")|----------\n";
	
	for(int y=7; y >= 0; y--){
		std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << y+1 << " ";
		for(int x=0; x<8; x++){
			RGB boardColor = (x + y)%2 ? whiteBoardColor : blackBoardColor;

			const Thera::Coordinate square(x, y);
			
			if (squareHighlights.at(square.getIndex64()) != RGB::Black)
				boardColor = overlay(boardColor, squareHighlights.at(square.getIndex64()), highlightOpacity);

			// set the board color
			std::cout << ANSI::set24BitColor(boardColor.red, boardColor.green, boardColor.blue, ANSI::Background);

			// set the piece color
			const RGB pieceColor = board.at(square).color == Thera::PieceColor::White
				? ((x + y)%2 ? whitePieceColorOnWhiteSquare : whitePieceColorOnBlackSquare)
				: blackPieceColor;
			if (board.at(square).type != Thera::PieceType::None){
				std::cout << ANSI::set24BitColor(pieceColor.red, pieceColor.green, pieceColor.blue, ANSI::Foreground);
			}

			// print the piece
			std::cout
				<< pieces.at(board.at(square))
				<< " " << ANSI::reset();
		}
		std::cout << ANSI::set4BitColor(ANSI::Gray, ANSI::Background) << y+1 << " " << ANSI::reset();

		// print board stats
		std::cout << "  ";
		switch(7-y){
			case 0:
				std::cout << (board.getColorToMove() == Thera::PieceColor::White ? "White" : "Black")
						  << " to move.";
				std::cout << "  (eval: " + std::to_string(Thera::evaluate(board)) + ")";
				break;
			case 1:
				std::cout << "Castling: [White] [Black]";
				break;
			case 2:
				std::cout << "          ";
				std::cout << setConditionalColor(board.getCurrentState().canWhiteCastleLeft, ANSI::Background) << "[Q]";
				std::cout << ANSI::reset() << " ";
				std::cout << setConditionalColor(board.getCurrentState().canWhiteCastleRight, ANSI::Background) << "[K]";
				std::cout << ANSI::reset() << " ";
				std::cout << setConditionalColor(board.getCurrentState().canBlackCastleLeft, ANSI::Background) << "[Q]";
				std::cout << ANSI::reset() << " ";
				std::cout << setConditionalColor(board.getCurrentState().canBlackCastleRight, ANSI::Background) << "[K]";

				break;
			case 3:
				switch(options.selectedBitboard){
					case BitboardSelection::AllPieces:
						std::cout << "Showing bitboard for all ";
						
						if (options.shownPieceBitboard.type != Thera::PieceType::None){
							std::cout << (options.shownPieceBitboard.color == Thera::PieceColor::White
											? "white "
											: "black ");
						}
						std::cout << "pieces";
						break;
					case BitboardSelection::Debug:
						std::cout << "Showing debug bitboard";
						break;
					case BitboardSelection::PinnedPieces:
						std::cout << "Showing pinned pieces";
						break;
					case BitboardSelection::SinglePiece:
						std::cout << "Showing bitboard for " << Thera::Utils::pieceToString(options.shownPieceBitboard, true);
						break;
					case BitboardSelection::AttackedSquares:
						std::cout << "Showing attacked squares";
						break;
					case BitboardSelection::None:
						std::cout << "Showing no bitboard";
						break;
					case BitboardSelection::AttackedBySquares:
						std::cout << "Showing squares attacked by " << ANSI::set4BitColor(ANSI::Blue) << Thera::Utils::squareToAlgebraicNotation(options.squareSelection) << ANSI::reset();
						break;
					case BitboardSelection::AttackingSquares:
						std::cout << "Showing squares attacking " << ANSI::set4BitColor(ANSI::Blue) << Thera::Utils::squareToAlgebraicNotation(options.squareSelection) << ANSI::reset();
						break;
					case BitboardSelection::PossibleMoveTargets:
						std::cout << "Showing possible move targets";
						break;

				}
				break;
			case 5:
				std::cout << "FEN: " << ANSI::set4BitColor(ANSI::Blue, ANSI::Foreground) << board.storeToFEN() << ANSI::reset();
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
		FlipColors,
		Search,
		NOP,
	};

	Thera::Move move;
	OperationType op = OperationType::MakeMove;
	int perftDepth=0;
	bool failed = false;
	std::string message;
	bool perftInstrumented;
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
		options.selectedBitboard = BitboardSelection::None;
		return;
	}
	else if (buffer == "all"){
		options.selectedBitboard = BitboardSelection::AllPieces;
		buffer = "";
		std::getline(std::cin, buffer);
		std::stringstream ss;
		ss << buffer;
		ss >> buffer;
		if (buffer == "white"){
			options.shownPieceBitboard.color = Thera::PieceColor::White;
			options.shownPieceBitboard.type = Thera::PieceType::Pawn;
		}
		else if (buffer == "black"){
			options.shownPieceBitboard.color = Thera::PieceColor::Black;
			options.shownPieceBitboard.type = Thera::PieceType::Pawn;
		}
		else{
			options.shownPieceBitboard.type = Thera::PieceType::None;
		}
		return;
	}
	else if (buffer == "debug"){
		options.selectedBitboard = BitboardSelection::Debug;
		return;
	}
	else if (buffer == "pin" || buffer == "pinned"){
		options.selectedBitboard = BitboardSelection::PinnedPieces;
		return;
	}
	else if (buffer == "attacked"){
		options.selectedBitboard = BitboardSelection::AttackedSquares;
		return;
	}
	else if (buffer == "attacked_by"){
		options.selectedBitboard = BitboardSelection::AttackedBySquares;
		std::cin >> buffer;
		try{
			options.squareSelection = Thera::Utils::squareFromAlgebraicNotation(buffer);
		}
		catch(std::invalid_argument){
			result.message = buffer + " isn't a valid square!";
			result.failed = true;
			return;
		}
		return;
	}
	else if (buffer == "attacking"){
		options.selectedBitboard = BitboardSelection::AttackingSquares;
		std::cin >> buffer;
		try{
			options.squareSelection = Thera::Utils::squareFromAlgebraicNotation(buffer);
		}
		catch(std::invalid_argument){
			result.message = buffer + " isn't a valid square!";
			result.failed = true;
			return;
		}
		return;
	}
	else if (buffer == "possible_targets"){
		options.selectedBitboard = BitboardSelection::PossibleMoveTargets;
		return;
	}

	try{
		options.shownPieceBitboard.color =stringToPieceColor.at(buffer);
	}
	catch(std::out_of_range){
		result.message = "Invalid color \"" + buffer + "\"!";
		result.failed = true;
		return;
	}

	// parse type
	std::cin >> buffer;
	try{
		options.shownPieceBitboard.type = stringToPieceType.at(buffer);
	}
	catch(std::out_of_range){
		result.message = "Invalid piece \"" + buffer + "\"!";
		result.failed = true;
		return;
	}
	options.selectedBitboard = BitboardSelection::SinglePiece;
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
		result.perftInstrumented = true;
		return;
	}
	else if (buffer == "fperft"){
		handlePerftCommand(result, options);
		result.perftInstrumented = false;
		return;
	}
	else if (buffer == "fen"){
		handleFenCommand(result, options);
		return;
	}
	else if (buffer == "analyze"){
		handlePerftCommand(result, options);
		result.op = MoveInputResult::Analyze;
		result.perftInstrumented = true;
		return;
	}
	else if (buffer == "fanalyze"){
		handlePerftCommand(result, options);
		result.op = MoveInputResult::Analyze;
		result.perftInstrumented = false;
		return;
	}
	else if (buffer == "search"){
		result.op = MoveInputResult::Search;
		std::cin >> buffer;
		try{
			result.perftDepth = std::stoi(buffer);
		}
		catch (std::invalid_argument){
			result.message = ANSI::set4BitColor(ANSI::Red) + "Invalid search depth!" + ANSI::reset();
			result.op = MoveInputResult::Continue;
		}
		return;
	}
	else if (buffer == "autoplay"){
		result.op = MoveInputResult::Continue;
		std::cin >> buffer;
		try{
			options.autoplayDepth = std::stoi(buffer);
		}
		catch (std::invalid_argument){
			result.message = ANSI::set4BitColor(ANSI::Red) + "Invalid search depth!" + ANSI::reset();
		}
		return;
	}
	else if (buffer == "flip"){
		result.op = MoveInputResult::FlipColors;
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

static void setBitboardHighlight(Options const& options, Thera::Board const& board, Thera::MoveGenerator& generator, std::array<RGB, 64>& highlights){
	Thera::Bitboard bitboard;

	generator.generateAttackData(board);
	switch(options.selectedBitboard){
		case BitboardSelection::AllPieces:
			if (options.shownPieceBitboard.type == Thera::PieceType::None)
				bitboard = board.getAllPieceBitboard();
			else
				bitboard = board.getPieceBitboardForOneColor(options.shownPieceBitboard.color);
			break;
		case BitboardSelection::Debug: 					bitboard = Thera::MoveGenerator::debugBitboard; break;
		case BitboardSelection::None: 					bitboard = 0; break;
		case BitboardSelection::PinnedPieces:  			bitboard = generator.getPinnedPieces();break;
		case BitboardSelection::SinglePiece: 			bitboard = board.getBitboard(options.shownPieceBitboard); break;
		case BitboardSelection::AttackedSquares: 		bitboard = generator.getAttackedSquares(); break;
		case BitboardSelection::AttackedBySquares:		bitboard = generator.getSquaresAttackedBy(options.squareSelection); break;
		case BitboardSelection::AttackingSquares:		bitboard = generator.getSquaresAttacking(options.squareSelection); break;
		case BitboardSelection::PossibleMoveTargets: 	bitboard = generator.getPossibleMoveTargets(); break;

		default:
			throw Thera::Utils::NotImplementedException();
	}
	
	while (bitboard.hasPieces()){
		highlights.at(bitboard.getLS1B()) = highlightBitboardPresent;
		bitboard.clearLS1B();
	}
}

static void redrawGUI(Options const& options, Thera::Board const& board, Thera::MoveGenerator& generator, std::array<RGB, 64>& highlights, std::string const& message){
	std::cout << ANSI::clearScreen() << ANSI::reset() << "-------------------\n" << message << ANSI::reset() << "\n";
	setBitboardHighlight(options, board, generator, highlights);
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
static void analyzePosition(MoveInputResult const& userInput, Thera::Board& board, Thera::MoveGenerator& generator, std::string& message, int originalDepth, bool instrumented){
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

	Thera::PerftResult stockfishResult;
	
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
			auto& movePair = stockfishResult.moves.emplace_back();

			while (buffer.at(i) != ':') i++;

			movePair.move = Thera::Move::fromString(buffer.substr(0, i));

			if (buffer.at(i++) != ':') continue;
			if (buffer.at(i++) != ' ') continue;
			movePair.numNodesSearched = stoi(buffer.substr(i));
		}
		else if (buffer.starts_with(NODES_SEARCHED_TEXT)){
			stockfishResult.numNodesSearched = stoi(buffer.substr(NODES_SEARCHED_TEXT.size()));
		}
	}


	Thera::PerftResult theraResult;
	if (instrumented) 	theraResult = Thera::perft_instrumented(board, generator, userInput.perftDepth, true);
	else 				theraResult = Thera::perft(board, generator, userInput.perftDepth, true);
	
	enum class MoveSource{
		Thera,
		Stockfish
	};

	std::vector<std::pair<Thera::PerftResult::SingleMove, MoveSource>> differentMoves;

	for (auto move : theraResult.moves){
		if (std::find(stockfishResult.moves.begin(), stockfishResult.moves.end(), move) == stockfishResult.moves.end()){
			differentMoves.emplace_back(move, MoveSource::Thera);
		}
	}
	
	for (auto move : stockfishResult.moves){
		if (std::find(theraResult.moves.begin(), theraResult.moves.end(), move) == theraResult.moves.end()){
			differentMoves.emplace_back(move, MoveSource::Stockfish);
		}
	}

	std::sort(differentMoves.begin(), differentMoves.end());

	for (auto [move, source] : differentMoves){
		message += indentation + std::string("[") + (source == MoveSource::Thera ? "Thera]     " : "Stockfish] ");
		message += Thera::Utils::squareToAlgebraicNotation(move.move.startIndex) + Thera::Utils::squareToAlgebraicNotation(move.move.endIndex);
		switch (move.move.promotionType){
			case Thera::PieceType::Bishop: message += "b"; break;
			case Thera::PieceType::Knight: message += "n"; break;
			case Thera::PieceType::Rook:   message += "r"; break;
			case Thera::PieceType::Queen:  message += "q"; break;
			default: break;
		}
		message += ": " + std::to_string(move.numNodesSearched) + "\n";
		if (userInput.perftDepth > 0){
			auto moveIt = std::find_if(theraResult.moves.begin(), theraResult.moves.end(), [move](auto other){return move.move == other.move;});
			if (moveIt == theraResult.moves.end()){
				message += indentation + "\tMove not found!\n";
			}
			else{
				MoveInputResult newUserInput = userInput;
				newUserInput.perftDepth--;
				board.applyMove(moveIt->move);
				analyzePosition(newUserInput, board, generator, message, originalDepth, instrumented);
				board.rewindMove();
			}
		}
	}

	std::vector<Thera::Move> theraRawMoves;
	for (auto move : theraResult.moves){
		theraRawMoves.push_back(move.move);
	}

	// find moves that got generated twice
	std::sort(theraRawMoves.begin(), theraRawMoves.end());
	auto duplicate = std::adjacent_find(theraRawMoves.begin(), theraRawMoves.end());
	while (duplicate != theraRawMoves.end()){
		message += indentation + std::string("[Thera]     ");
		message += Thera::Utils::squareToAlgebraicNotation(duplicate->startIndex) + Thera::Utils::squareToAlgebraicNotation(duplicate->endIndex);
		switch (duplicate->promotionType){
			case Thera::PieceType::Bishop: message += "b"; break;
			case Thera::PieceType::Knight: message += "n"; break;
			case Thera::PieceType::Rook:   message += "r"; break;
			case Thera::PieceType::Queen:  message += "q"; break;
			default: break;
		}
		message += ": " + ANSI::set4BitColor(ANSI::Red) + "Duplicate!" + ANSI::reset() + "\n";
		duplicate = std::adjacent_find(std::next(duplicate), theraRawMoves.end());
	}

	if (userInput.perftDepth != originalDepth) return;

	message += ANSI::set4BitColor(ANSI::Blue);
	message += indentation + "Stockfish searched " + std::to_string(stockfishResult.moves.size()) + " moves (" + std::to_string(stockfishResult.numNodesSearched) + " nodes)\n";
	message += indentation + "Thera searched " + std::to_string(theraResult.moves.size()) + " moves (" + std::to_string(theraResult.numNodesSearched) + " nodes)\n";

	message += "Filtered " + std::to_string(theraResult.numNodesFiltered) + " moves\n";

	message += indentation + "Results are ";
	if (differentMoves.size() || theraResult.numNodesSearched != stockfishResult.numNodesSearched)
		message += indentation + ANSI::set4BitColor(ANSI::Red) + "different" + ANSI::set4BitColor(ANSI::Blue) +".\n";
	else
		message += indentation + ANSI::set4BitColor(ANSI::Green) + "identical" + ANSI::set4BitColor(ANSI::Blue) +".\n";
}

int playMode(Options& options){
	Thera::Board board;
	board.loadFromFEN(options.fen);

	std::array<RGB, 64> highlights;
	highlights.fill(RGB());
	options.shownPieceBitboard = Thera::Piece(Thera::PieceType::None, Thera::PieceColor::White);
	options.selectedBitboard = BitboardSelection::None;

	Thera::MoveGenerator generator;

	std::string message =  	"Enter move or type 'exit'.\n"
							"Change your move by typing 'change'.\n"
							"Undo last move using 'undo'.";

	std::optional<Thera::PieceColor> computerColor;
	auto lastOp = MoveInputResult::NOP;
	std::stack<Thera::Move> moveStack;
	while (true){
		// make a computer move if requested
		if (options.autoplayDepth){
			if (!computerColor.has_value()){
				computerColor = board.getColorToMove();
			}
			if (computerColor == board.getColorToMove() && lastOp != MoveInputResult::UndoMove){
				auto [move, _] = Thera::search(board, generator, options.autoplayDepth);
				board.applyMove(move);
				moveStack.push(move);
				continue;
			}
		}
		else
			computerColor.reset();

		// highlight the last move
		if (!moveStack.empty()){
			highlights.at(moveStack.top().startIndex.getIndex64()) = RGB(255, 0, 0);
			highlights.at(moveStack.top().endIndex.getIndex64()) = RGB(255, 0, 0);
		}
		
		MoveInputResult userInput;

		redrawGUI(options, board, generator, highlights, message);
		message = "";
		
		getUserMoveStart(userInput, options);
		
		if (userInput.failed){
			message = ANSI::set4BitColor(ANSI::Red) + userInput.message + ANSI::reset();
			continue;
		}

		if (userInput.op == MoveInputResult::Exit) break;
		else if (userInput.op == MoveInputResult::UndoMove){
			try{
				board.rewindMove();
				moveStack.pop();
				message = ANSI::set4BitColor(ANSI::Blue) + "Undid move." + ANSI::reset();
			}
			catch(std::runtime_error){
				message = ANSI::set4BitColor(ANSI::Red) + "No move to undo." + ANSI::reset();
			}
			lastOp = userInput.op;
			continue;
		}
		else if (userInput.op == MoveInputResult::LoadFEN){
			board.loadFromFEN(options.fen);
			message = ANSI::set4BitColor(ANSI::Blue) + "Loaded position from FEN." + ANSI::reset();
			lastOp = userInput.op;
			moveStack = std::stack<Thera::Move>();
			continue;
		}
		else if (userInput.op == MoveInputResult::Analyze){
			analyzePosition(userInput, board, generator, message, userInput.perftDepth, userInput.perftInstrumented);
			if (!userInput.perftInstrumented) message += ANSI::set8BitColor(208) + "Performed fast analysis. No filtering was performed!";
			lastOp = userInput.op;
			continue;
		}
		else if (userInput.op == MoveInputResult::FlipColors){
			board.switchPerspective();
			message = ANSI::set4BitColor(ANSI::Blue) + "Flipped color to move." + ANSI::reset();
			lastOp = userInput.op;
			continue;
		}
		else if (userInput.op == MoveInputResult::Search){
			auto [move, eval] = Thera::search(board, generator, userInput.perftDepth);
			message = ANSI::set4BitColor(ANSI::Blue) + "Best move: ";
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
			message += " (Eval: " + std::to_string(eval) + ")" + ANSI::reset();
		
			lastOp = userInput.op;
			continue;
		}
		else if (userInput.op == MoveInputResult::NOP){
			lastOp = userInput.op;
		}
		else if (userInput.op == MoveInputResult::Perft){
			message = "";

			Thera::PerftResult result;
    		const auto start = std::chrono::high_resolution_clock::now();
			if (userInput.perftInstrumented) result = Thera::perft_instrumented(board, generator, userInput.perftDepth, true);
			else result = Thera::perft(board, generator, userInput.perftDepth, true);
    		const auto stop = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> duration = stop-start;

			for (auto move : result.moves){
				message
					+= Thera::Utils::squareToAlgebraicNotation(move.move.startIndex)
					+  Thera::Utils::squareToAlgebraicNotation(move.move.endIndex);
				switch (move.move.promotionType){
					case Thera::PieceType::Bishop: message += "b"; break;
					case Thera::PieceType::Knight: message += "n"; break;
					case Thera::PieceType::Rook:   message += "r"; break;
					case Thera::PieceType::Queen:  message += "q"; break;
					default: break;
				}
				message += ": " + std::to_string(move.numNodesSearched) + "\n";
			}


			// write the perft output to a file for easier debugging
			std::ofstream logFile("/tmp/thera.txt", std::ofstream::trunc);
			if (!logFile.is_open()){
				message += ANSI::set4BitColor(ANSI::Red) + "Unable to open logfile! Ignoring." + ANSI::reset() + "\n";
			}
			else{
				logFile << message;
				logFile.close();
			}

			message += "Filtered " + std::to_string(result.numNodesFiltered) + " moves\n";
			message += "Nodes searched: " + std::to_string(result.numNodesSearched) + "\n";
			message += "Time spent: " + std::to_string(duration.count()) + "s (" + std::to_string((float(result.numNodesSearched)/duration.count())/1000'000) + "MN/s)\n";
			if (!userInput.perftInstrumented) message += ANSI::set8BitColor(208) + "Performed fast analysis. No filtering was performed!";

			lastOp = userInput.op;
			continue;
		}
		else if (userInput.op == MoveInputResult::Continue){
			lastOp = userInput.op;
			continue;
		}

		auto const isCorrectStart = [&](auto const& move){
			return move.startIndex == userInput.move.startIndex;
		};

		std::vector<Thera::Move> allMoves = generator.generateAllMoves(board);
		std::vector<Thera::Move> possibleMoves;
		std::copy_if(allMoves.begin(), allMoves.end(), std::back_inserter(possibleMoves), isCorrectStart);

		message = "";

		for (auto move : possibleMoves){
			message	+= ""
					+  Thera::Utils::squareToAlgebraicNotation(move.startIndex)
					+  Thera::Utils::squareToAlgebraicNotation(move.endIndex);
			switch (move.promotionType){
				case Thera::PieceType::Bishop: message += "b"; break;
				case Thera::PieceType::Knight: message += "n"; break;
				case Thera::PieceType::Rook:   message += "r"; break;
				case Thera::PieceType::Queen:  message += "q"; break;
				default: break;
			}	
			message += "\n";
		}
		message += ANSI::set4BitColor(ANSI::Blue) + "Number of moves: " + std::to_string(possibleMoves.size());
		

		if (options.selectedBitboard == BitboardSelection::None){
			// highlight all moves
			highlights.at(userInput.move.startIndex.getIndex64()) = highlightSquareSelected;
			for (auto const& move : possibleMoves){
				highlights.at(move.endIndex.getIndex64()) = highlightMovePossible;
			}
		}

		redrawGUI(options, board, generator, highlights, message);
		
		getUserMoveEnd(userInput, options);
		if (userInput.op == MoveInputResult::Exit) break;
		else if (userInput.op == MoveInputResult::Continue) continue;
		else if (userInput.op == MoveInputResult::ForceMove){
			if (board.getColorToMove() == board.at(userInput.move.startIndex).color){
				board.applyMove(userInput.move);
				moveStack.push(userInput.move);
			}
			else{
				board.applyMoveStatic(userInput.move);
				moveStack.push(userInput.move);
				message = ANSI::set4BitColor(ANSI::Blue) + "Forced move." + ANSI::reset();
			}
		}
		else if (userInput.op == MoveInputResult::UndoMove){
			board.rewindMove();
			moveStack.pop();
			message = ANSI::set4BitColor(ANSI::Blue) + "Undone move." + ANSI::reset();
		}
		else if (userInput.op == MoveInputResult::NOP);
		else{
			auto moveIt = std::find_if(possibleMoves.begin(), possibleMoves.end(), [&](auto other){return Thera::Move::isSameBaseMove(userInput.move, other);});
			if (moveIt != possibleMoves.end()){
				// apply the found move since the user move
				// won't have any stats attached
				board.applyMove(*moveIt);
				moveStack.push(*moveIt);
			}
			else{
				message = ANSI::set4BitColor(ANSI::Red) + "Invalid move!" + ANSI::reset();
			}
			lastOp = userInput.op;
		}
	}

	std::cout << ANSI::reset() << "Bye...\n";
	return 0;
}