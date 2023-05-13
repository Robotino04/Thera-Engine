#include <iostream>

#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"
#include "Thera/Coordinate.hpp"

#include "Thera/Utils/ScopeGuard.hpp"
#include "Thera/Utils/ChessTerms.hpp"

#include "Thera/perft.hpp"

#include "ANSI/ANSI.hpp"

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


int main(int argc, const char** argv){
    if (argc < 4){
        std::cout << "Invalid number if arguments given. Please give:\n"
                     "[depth] [fen] [bulk counting] [expected nodes]\n";
        return 1;
    }
    const int depth = std::atoi(argv[1]);
    const char* const fen = argv[2];
    const bool bulkCounting = std::atoi(argv[3]);
    const int expectedNodes = std::atoi(argv[4]);

    std::cout << "Running perft(" << depth << ") for \"" << fen << "\"";
    std::cout << " (bulk counting " << (bulkCounting ? ANSI::set4BitColor(ANSI::Green) + "enabled" : ANSI::set4BitColor(ANSI::Red) + "disabled") << ANSI::reset() << ")\n";

    Thera::Board board;
    Thera::MoveGenerator generator;

    board.loadFromFEN(fen);
    int filteredMoves = 0;

    const auto start = std::chrono::high_resolution_clock::now();
    auto result = Thera::perft(board, generator, depth, bulkCounting);
    const auto stop = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = stop - start;

    for (auto move : result.moves){
        printMove(move.move, move.numNodesSearched);
    }

    const bool passed = expectedNodes == result.numNodesSearched;

    std::cout << "perft(" << depth << ") = " << result.numNodesSearched << " (expected " << expectedNodes << ") " << (passed ? "✓" : "✗") << " \n";
    std::cout << "Filtered " << filteredMoves << " moves\n";
    std::cout << "Completed in " << duration.count() << "s. (" << (float(result.numNodesSearched)/duration.count())/1000'000 << "MN/s)\n";

    return passed ? 0 : 1;
}