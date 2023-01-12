#include "CLI/perftMode.hpp"
#include "CLI/IO.hpp"

#include "ANSI/ANSI.hpp"

#include "ChessBot/Board.hpp"
#include "ChessBot/MoveGenerator.hpp"
#include "ChessBot/Utils/ScopeGuard.hpp"
#include "ChessBot/Utils/Coordinates.hpp"
#include "ChessBot/Utils/ChessTerms.hpp"

#include <iostream>
#include <cmath>
#include <chrono>

bool isSquareAttacked(int8_t square, ChessBot::Board& board, ChessBot::MoveGenerator& generator){
    auto moves = generator.generateAllMoves(board);
    bool allKingsFound = true;
    for (auto const& move : moves){
        if (move.endIndex == square)
         return true;
    }
    return false;
}

std::vector<int8_t> findPiece(ChessBot::PieceType type, ChessBot::PieceColor color, ChessBot::Board const& board){
    std::vector<int8_t> indices;
    indices.reserve(16);
    for (int index=0; index < 64; index++){
        if (board.at(index).getType() == type && board.at(index).getColor() == color){
            indices.push_back(index);
        }
    }
    return indices;
}

std::vector<ChessBot::Move> filterMoves(std::vector<ChessBot::Move> const& moves, ChessBot::Board& board, ChessBot::MoveGenerator& generator){
    std::vector<ChessBot::Move> newMoves;
    newMoves.reserve(moves.size());
    for (auto const& move : moves){
        board.applyMove(move);
        ChessBot::Utils::ScopeGuard boardRestore([&](){board.rewindMove();});
        
        auto kings = findPiece(ChessBot::PieceType::King, board.getColorToMove().opposite(), board);
        if (move.isCastling){
            bool isInvalid = false;
            for (int8_t square = move.startIndex; square != move.endIndex; square += ChessBot::Utils::sign(move.endIndex-move.startIndex)){
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

int perft(int depth, bool bulkCounting, ChessBot::Board& board, ChessBot::MoveGenerator& generator, bool isInitialCall=true){
    if (depth == 0) return 1;

    int numNodes = 0;

    auto moves = filterMoves(generator.generateAllMoves(board), board, generator);
    if (bulkCounting && depth == 1)
        return moves.size();

    for (auto const& move : moves){
        board.applyMove(move);
        int tmp = perft(depth-1, bulkCounting, board, generator, false);
        numNodes += tmp;
        if (isInitialCall){
            std::cout
                << static_cast<char>('a' + ChessBot::Utils::getXCoord(move.startIndex)) << static_cast<char>('8' - ChessBot::Utils::getYCoord(move.startIndex))
                << static_cast<char>('a' + ChessBot::Utils::getXCoord(move.endIndex)) << static_cast<char>('8' - ChessBot::Utils::getYCoord(move.endIndex));
            switch (move.promotionType){
                case ChessBot::PieceType::Bishop: std::cout << "b"; break;
                case ChessBot::PieceType::Knight: std::cout << "n"; break;
                case ChessBot::PieceType::Rook: std::cout << "r"; break;
                case ChessBot::PieceType::Queen: std::cout << "q"; break;
                default: break;
            }
            
            std::cout << ": " << tmp << "\n";
        }
        board.rewindMove();
    }

    return numNodes;
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
    int numNodes = perft(options.perftDepth, options.bulkCounting, board, generator);
    auto stop = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = stop - start;

    std::cout << "perft(" << options.perftDepth << ") = " << numNodes << "\n";
    std::cout << "Completed in " << duration.count() << "s.\n";

    return 0;
}