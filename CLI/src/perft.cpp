#include "Thera/Board.hpp"
#include "Thera/MoveGenerator.hpp"
#include "Thera/Utils/ScopeGuard.hpp"
#include "Thera/Utils/Coordinates.hpp"
#include "Thera/Utils/ChessTerms.hpp"

bool isSquareAttacked(int8_t square, Thera::Board& board, Thera::MoveGenerator& generator){
    auto moves = generator.generateAllMoves(board);
    for (auto const& move : moves){
        if (move.endIndex == square)
            return true;
    }
    return false;
}

std::vector<int8_t> findPiece(Thera::PieceType type, Thera::PieceColor color, Thera::Board const& board){
    std::vector<int8_t> indices;
    indices.reserve(16);
    for (int index=0; index < 64; index++){
        if (board.at(index).getType() == type && board.at(index).getColor() == color){
            indices.push_back(Thera::Board::to10x12Coords(index));
        }
    }
    return indices;
}

std::vector<Thera::Move> filterMoves(std::vector<Thera::Move> const& moves, Thera::Board& board, Thera::MoveGenerator& generator){
    std::vector<Thera::Move> newMoves;
    newMoves.reserve(moves.size());
    for (auto const& move : moves){
        board.applyMove(move);
        Thera::Utils::ScopeGuard boardRestore([&](){board.rewindMove();});
        
        auto kings = findPiece(Thera::PieceType::King, board.getColorToMove().opposite(), board);
        if (move.isCastling){
            bool isInvalid = false;
            for (int8_t square = move.startIndex; square != move.endIndex; square += Thera::Utils::sign(move.endIndex-move.startIndex)){
                if (isSquareAttacked(square, board, generator)){
                    isInvalid = true;
                    break;
                }
            }
            if (isInvalid) continue;
        }
        if (kings.size() && !isSquareAttacked(kings.at(0), board, generator)){
            newMoves.push_back(move);
        }
    }
    return newMoves;
}

int perft(int depth, bool bulkCounting, std::function<void(Thera::Move const&, int)> printFn, Thera::Board& board, Thera::MoveGenerator& generator, bool isInitialCall){
    if (depth == 0) return 1;

    int numNodes = 0;

    auto moves = filterMoves(generator.generateAllMoves(board), board, generator);
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
        int tmp = perft(depth-1, bulkCounting, printFn, board, generator, false);
        numNodes += tmp;
        if (isInitialCall)
            printFn(move, tmp);
        board.rewindMove();
    }

    return numNodes;
}