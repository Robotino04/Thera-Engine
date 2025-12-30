#include "Thera/perft.hpp"

#include "Thera/Utils/ScopeGuard.hpp"

#include "Thera/Board.hpp"
#include "Thera/Move.hpp"
#include "Thera/MoveGenerator.hpp"

#include <iostream>
#include "ANSI/ANSI.hpp"

namespace Thera {

static void printFilteredMove(Thera::Move const& move, Thera::Board const& board) {
    // removed for performance evaluation
    std::cout << ANSI::set4BitColor(ANSI::Red) << "Filtered move " << ANSI::reset() << move.toString();
    std::cout << "     (" << board.storeToFEN() << ")\n";
}

static std::vector<Thera::Move> filterMoves(std::vector<Thera::Move> const& moves, Thera::Board& board, Thera::MoveGenerator& generator) {
    std::vector<Thera::Move> newMoves;
    newMoves.reserve(moves.size());
    for (auto const& move : moves) {
        board.applyMove(move);
        Thera::Utils::ScopeGuard rewindBoard_guard([&]() { board.rewindMove(); });

        const auto kingBitboard = board.getBitboard({Thera::PieceType::King, board.getColorToNotMove()});
        if (!kingBitboard.hasPieces())
            continue;

        board.switchPerspective();
        generator.generateAttackData(board);
        board.switchPerspective();

        if ((generator.getAttackedSquares() & kingBitboard).hasPieces()) {
            rewindBoard_guard.release();
            printFilteredMove(move, board);
            continue;
        }

        newMoves.push_back(move);
    }
    return newMoves;
}


PerftResult perft_instrumented(Board& board, MoveGenerator& generator, int depth, bool bulkCounting, bool isInitialCall) {
    if (depth == 0)
        return {1, {}};

    PerftResult result;

    auto moves = generator.generateAllMoves(board);
    result.numNodesFiltered += moves.size();
    // filtering only tests for, but no longer removes invalid moves
    // this is to have test cases fail in case illegal moves get generated
    result.numNodesFiltered -= filterMoves(moves, board, generator).size();
    if (bulkCounting && depth == 1) {
        result.numNodesSearched = moves.size();
        for (auto move : moves) {
            result.moves.emplace_back(move, 1);
        }
        return result;
    }

    for (auto const& move : moves) {
        board.applyMove(move);
        Utils::ScopeGuard boardRestore([&]() { board.rewindMove(); });

        auto tmp = perft_instrumented(board, generator, depth - 1, bulkCounting, false);
        result.moves.emplace_back(move, tmp.numNodesSearched);
        result.numNodesFiltered += tmp.numNodesFiltered;
        result.numNodesSearched += tmp.numNodesSearched;
    }

    return result;
}

template <bool bulkCounting>
static uint64_t perftHelper(Board& board, MoveGenerator& generator, int depth) {
    if (depth == 0)
        return 1;

    auto moves = generator.generateAllMoves(board);
    if (bulkCounting && depth == 1) {
        return moves.size();
    }

    uint64_t numNodes = 0;
    for (auto const& move : moves) {
        board.applyMove(move);
        numNodes += perftHelper<bulkCounting>(board, generator, depth - 1);
        board.rewindMove();
    }

    return numNodes;
};

PerftResult perft(Board& board, MoveGenerator& generator, int depth, bool bulkCounting) {
    if (depth == 0)
        return {1, {}};

    PerftResult result;

    auto moves = generator.generateAllMoves(board);
    if (bulkCounting && depth == 1) {
        result.numNodesSearched = moves.size();
        for (auto move : moves) {
            result.moves.emplace_back(move, 1);
        }
        return result;
    }

    for (auto move : moves) {
        board.applyMove(move);
        Utils::ScopeGuard boardRestore([&]() { board.rewindMove(); });

        uint64_t tmp;

        if (bulkCounting)
            tmp = perftHelper<true>(board, generator, depth - 1);
        else
            tmp = perftHelper<false>(board, generator, depth - 1);

        result.numNodesSearched += tmp;
        result.moves.emplace_back(move, tmp);
    }

    return result;
}

}
