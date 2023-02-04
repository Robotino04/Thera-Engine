#include "CLI/perftMode.hpp"
#include "CLI/IO.hpp"
#include "CLI/perft.hpp"

#include "ANSI/ANSI.hpp"

#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"
#include "Thera/Utils/ScopeGuard.hpp"
#include "Thera/Utils/Coordinates.hpp"
#include "Thera/Utils/ChessTerms.hpp"

#include <iostream>
#include <cmath>
#include <chrono>


void printMove(Thera::Move const& move, int numSubmoves){
    std::cout
        << Thera::Utils::squareToAlgebraicNotation(Thera::Board::to8x8Coords(move.startIndex))
        << Thera::Utils::squareToAlgebraicNotation(Thera::Board::to8x8Coords(move.endIndex));
    switch (move.promotionType){
        case Thera::PieceType::Bishop: std::cout << "b"; break;
        case Thera::PieceType::Knight: std::cout << "n"; break;
        case Thera::PieceType::Rook: std::cout << "r"; break;
        case Thera::PieceType::Queen: std::cout << "q"; break;
        default: break;
    }
    std::cout << ": " << numSubmoves << "\n";
}


int perftMode(Options& options){
    std::string fen = options.fen;
    if (fen == "start"){
        fen = Thera::Utils::startingFEN;
    }

    std::cout << "Running perft(" << options.perftDepth << ") for \"" << fen << "\"";
    std::cout << " (bulk counting " << setConditionalColor(options.bulkCounting, ANSI::Foreground) << (options.bulkCounting? "enabled" : "disabled") << ANSI::reset(ANSI::Foreground) << ")\n";

    Thera::Board board;
    Thera::MoveGenerator generator;

    board.loadFromFEN(fen);

    auto start = std::chrono::high_resolution_clock::now();
    int numNodes = perft(options.perftDepth, options.bulkCounting, printMove, board, generator);
    auto stop = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = stop - start;

    std::cout << "perft(" << options.perftDepth << ") = " << numNodes << "\n";
    std::cout << "Completed in " << duration.count() << "s.\n";

    return 0;
}