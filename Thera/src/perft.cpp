#include "Thera/perft.hpp"

#include "Thera/Utils/ScopeGuard.hpp"
#include "Thera/Coordinate.hpp"

#include "Thera/Board.hpp"
#include "Thera/Move.hpp"
#include "Thera/MoveGenerator.hpp"


namespace Thera{

namespace Detail{
// TODO: replace to use bitboards
bool isSquareAttacked(Coordinate square, Thera::Board& board, Thera::MoveGenerator& generator){
    auto moves = generator.generateAllMoves(board);
    for (auto const& move : moves){
        if (move.endIndex == square)
            return true;
    }
    return false;
}

// TODO: replace to use bitboards
std::vector<Thera::Move> filterMoves(std::vector<Thera::Move> const& moves, Thera::Board& board, Thera::MoveGenerator& generator){
    std::vector<Thera::Move> newMoves;
    newMoves.reserve(moves.size());
    for (auto const& move : moves){
        {
            board.applyMove(move);
            Thera::Utils::ScopeGuard rewindBoard_guard([&](){board.rewindMove();});

            const auto kingBitboard = board.getBitboard({Thera::PieceType::King, board.getColorToNotMove()});
            if (!kingBitboard.hasPieces()) continue;
            const auto kingSquare = kingBitboard.getPieces().at(0);
            const bool isInCheck = isSquareAttacked(kingSquare, board, generator);
            if (isInCheck) continue;
        }

        if (move.isCastling){
            bool isInvalid = false;

            const uint8_t direction = Thera::Utils::sign(move.endIndex.x - move.startIndex.x);
            for (uint8_t newX = move.startIndex.x; newX != move.endIndex.x; newX += direction){
                const Thera::Coordinate target(newX, move.startIndex.y);

                board.applyMove(Thera::Move(move.startIndex, target));
                Thera::Utils::ScopeGuard rewindCastligCheckMove_guard([&](){board.rewindMove();});

                if (isSquareAttacked(target, board, generator)){
                    isInvalid = true;
                    break;
                }
            }
            if (isInvalid) continue;
        }

        newMoves.push_back(move);
    }
    return newMoves;
}
}

int perft(Board& board, MoveGenerator& generator, int depth, bool bulkCounting, std::function<void(Move const&, int)> printFn, bool isInitialCall){
    if (depth == 0) return 1;

    int numNodes = 0;

    auto moves = generator.generateAllMoves(board);
    moves = Detail::filterMoves(moves, board, generator);
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
        
        int tmp = perft(board, generator, depth-1, bulkCounting, [](auto, auto){}, false);
        numNodes += tmp;

        printFn(move, tmp);
    }

    return numNodes;
}

}