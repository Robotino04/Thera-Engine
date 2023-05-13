#include "CLI/perftMode.hpp"
#include "CLI/IO.hpp"

#include "ANSI/ANSI.hpp"

#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"

#include "Thera/Utils/ScopeGuard.hpp"
#include "Thera/Coordinate.hpp"
#include "Thera/Utils/ChessTerms.hpp"

#include "Thera/perft.hpp"

#include <iostream>
#include <cmath>
#include <chrono>


static void printMove(Thera::Move const& move, int numSubmoves){
    std::cout
        << Thera::Utils::squareToAlgebraicNotation(move.startIndex)
        << Thera::Utils::squareToAlgebraicNotation(move.endIndex);
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
    std::cout << " (bulk counting " << setConditionalColor(options.bulkCounting, ANSI::Foreground) << (options.bulkCounting? "enabled" : "disabled") << ANSI::reset() << ")\n";

    Thera::Board board;
    Thera::MoveGenerator generator;


    board.loadFromFEN(fen);

    auto start = std::chrono::high_resolution_clock::now();
    auto result = Thera::perft_instrumented(board, generator, options.perftDepth, options.bulkCounting);
    auto stop = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = stop - start;

    std::cout << "perft(" << options.perftDepth << ") = " << result.numNodesSearched << "\n";
    std::cout << "Filtered " << result.numNodesFiltered << " moves\n";
    std::cout << "Completed in " << duration.count() << "s.\n";

    return 0;
}