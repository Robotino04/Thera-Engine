#include "Thera/perft.hpp"

#include "Thera/Utils/ScopeGuard.hpp"
#include "Thera/Coordinate.hpp"

#include "Thera/Board.hpp"
#include "Thera/Move.hpp"
#include "Thera/MoveGenerator.hpp"

// TODO: remove
#include <iostream>
#include "Thera/Utils/ChessTerms.hpp"
#include "ANSI/ANSI.hpp"

namespace Thera{

namespace Detail{


static void printFilteredMove(Thera::Move const& move, Thera::Board const& board){
    // removed for performance evaluation
    std::cout
        << ANSI::set4BitColor(ANSI::Red) << "Filtered move " << ANSI::reset()
        << Utils::squareToAlgebraicNotation(move.startIndex)
        << Utils::squareToAlgebraicNotation(move.endIndex);
    switch (move.promotionType){
        case PieceType::Bishop: std::cout << "b"; break;
        case PieceType::Knight: std::cout << "n"; break;
        case PieceType::Rook:   std::cout << "r"; break;
        case PieceType::Queen:  std::cout << "q"; break;
        default: break;
    }
    std::cout << "     (" << board.storeToFEN() << ")\n";
}

// TODO: remove
std::vector<Thera::Move> filterMoves(std::vector<Thera::Move> const& moves, Thera::Board& board, Thera::MoveGenerator& generator){
    std::vector<Thera::Move> newMoves;
    newMoves.reserve(moves.size());
    for (auto const& move : moves){
        board.applyMove(move);
        Thera::Utils::ScopeGuard rewindBoard_guard([&](){board.rewindMove();});

        const auto kingBitboard = board.getBitboard({Thera::PieceType::King, board.getColorToNotMove()});
        if (!kingBitboard.hasPieces()) continue;

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
}

int perft(Board& board, MoveGenerator& generator, int depth, bool bulkCounting, std::function<void(Move const&, int)> printFn, int& filteredMoves, bool isInitialCall){
    if (depth == 0) return 1;

    int numNodes = 0;

    auto moves = generator.generateAllMoves(board);
    filteredMoves += moves.size();
    moves = Detail::filterMoves(moves, board, generator);
    filteredMoves -= moves.size();
    if (bulkCounting && depth == 1){
        if (isInitialCall){
            for (auto const& move : moves){
                printFn(move, 1);
            }
        }
        return moves.size();
    }

    for (auto const& move : moves){
        board.applyMove(move);
        Utils::ScopeGuard boardRestore([&](){
            board.rewindMove();
        });
        
        int tmp = perft(board, generator, depth-1, bulkCounting, [](auto, auto){}, filteredMoves, false);
        numNodes += tmp;

        printFn(move, tmp);
    }

    return numNodes;
}

}