#include <iostream>

#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"

#include "Thera/Utils/ScopeGuard.hpp"
#include "Thera/Utils/Coordinates.hpp"
#include "Thera/Utils/ChessTerms.hpp"

#include "Thera/perft.hpp"

#include "ANSI/ANSI.hpp"

#include <iostream>
#include <cmath>
#include <chrono>


static void printMove(Thera::Move const& move, int numSubmoves){
    std::cout
        << Thera::Utils::squareToAlgebraicNotation(Thera::Utils::to8x8Coords(move.startIndex))
        << Thera::Utils::squareToAlgebraicNotation(Thera::Utils::to8x8Coords(move.endIndex));
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
    std::cout << " (bulk counting " << (bulkCounting ? ANSI::set4BitColor(ANSI::Green) + "enabled" : ANSI::set4BitColor(ANSI::Red) + "disabled") << ANSI::reset(ANSI::Foreground) << ")\n";

    Thera::Board board;
    Thera::MoveGenerator generator;

    board.loadFromFEN(fen);

    const auto start = std::chrono::high_resolution_clock::now();
    const int numNodes = Thera::perft(board, generator, depth, bulkCounting, printMove);
    const auto stop = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = stop - start;

    const bool passed = expectedNodes == numNodes;

    std::cout << "perft(" << depth << ") = " << numNodes << " (expected " << expectedNodes << ") " << (passed ? "✓" : "✗") << " \n";
    std::cout << "Completed in " << duration.count() << "s.\n";

    return passed ? 0 : 1;
}