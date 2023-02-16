#include "Thera/perft.hpp"

#include "Thera/Utils/ScopeGuard.hpp"
#include "Thera/Utils/Coordinates.hpp"

#include "Thera/Board.hpp"
#include "Thera/Move.hpp"
#include "Thera/MoveGenerator.hpp"


namespace Thera{

namespace Detail{
// TODO: replace to use bitboards
bool isSquareAttacked(int8_t square, Thera::Board& board, Thera::MoveGenerator& generator){
    auto moves = generator.generateAllMoves(board);
    for (auto const& move : moves){
        if (move.endIndex == square)
            return true;
    }
    return false;
}

// TODO: replace to use bitboards
std::vector<int8_t> findPiece(Thera::PieceType type, Thera::PieceColor color, Thera::Board const& board){
    std::vector<int8_t> indices;
    indices.reserve(16);
    for (int index=0; index < 64; index++){
        if (board.at(index).getType() == type && board.at(index).getColor() == color){
            indices.push_back(Thera::Utils::to10x12Coords(index));
        }
    }
    return indices;
}

// TODO: replace to use bitboards
std::vector<Thera::Move> filterMoves(std::vector<Thera::Move> const& moves, Thera::Board& board, Thera::MoveGenerator& generator){
    std::vector<Thera::Move> newMoves;
    newMoves.reserve(moves.size());
    for (auto const& move : moves){
        {
            board.applyMove(move);
            Thera::Utils::ScopeGuard rewindBoard_guard([&](){board.rewindMove();});

            const auto kings = findPiece(Thera::PieceType::King, board.getColorToMove().opposite(), board);
            if (kings.size() == 0) continue;
            const bool isInCheck = isSquareAttacked(kings.at(0), board, generator);
            if (isInCheck) continue;
        }

        if (move.isCastling){
            bool isInvalid = false;

            const int8_t direction = Thera::Utils::sign(move.endIndex-move.startIndex);
            for (int8_t square = move.startIndex; square != move.endIndex; square += direction){
                board.applyMove(Thera::Move(move.startIndex, square));
                Thera::Utils::ScopeGuard rewindCastligCheckMove_guard([&](){board.rewindMove();});
                if (isSquareAttacked(square, board, generator)){
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