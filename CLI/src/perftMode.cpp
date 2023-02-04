#include "CLI/perftMode.hpp"
#include "CLI/IO.hpp"
#include "CLI/perft.hpp"

#include "ANSI/ANSI.hpp"

#include "ChessBot/Board.hpp"
#include "ChessBot/MoveGenerator.hpp"
#include "ChessBot/Utils/ScopeGuard.hpp"
#include "ChessBot/Utils/Coordinates.hpp"
#include "ChessBot/Utils/ChessTerms.hpp"

#include <iostream>
#include <cmath>
#include <chrono>


void printMove(ChessBot::Move const& move, int numSubmoves){
    std::cout
        << ChessBot::Utils::squareToAlgebraicNotation(ChessBot::Board::to8x8Coords(move.startIndex))
        << ChessBot::Utils::squareToAlgebraicNotation(ChessBot::Board::to8x8Coords(move.endIndex));
    switch (move.promotionType){
        case ChessBot::PieceType::Bishop: std::cout << "b"; break;
        case ChessBot::PieceType::Knight: std::cout << "n"; break;
        case ChessBot::PieceType::Rook: std::cout << "r"; break;
        case ChessBot::PieceType::Queen: std::cout << "q"; break;
        default: break;
    }
    std::cout << ": " << numSubmoves << "\n";
}


int perftMode(Options& options){
    std::string fen = options.fen;
    if (fen == "start"){
        fen = ChessBot::Utils::startingFEN;
    }

    std::cout << "Running perft(" << options.perftDepth << ") for \"" << fen << "\"";
    std::cout << " (bulk counting " << setConditionalColor(options.bulkCounting, ANSI::Foreground) << (options.bulkCounting? "enabled" : "disabled") << ANSI::reset(ANSI::Foreground) << ")\n";

    ChessBot::Board board;
    ChessBot::MoveGenerator generator;

    board.loadFromFEN(fen);

    auto start = std::chrono::high_resolution_clock::now();
    int numNodes = perft(options.perftDepth, options.bulkCounting, printMove, board, generator);
    auto stop = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = stop - start;

    std::cout << "perft(" << options.perftDepth << ") = " << numNodes << "\n";
    std::cout << "Completed in " << duration.count() << "s.\n";

    return 0;
}