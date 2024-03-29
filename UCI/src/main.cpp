#include "Thera/Board.hpp"
#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/MoveGenerator.hpp"
#include "Thera/search.hpp"
#include "Thera/Utils/GitInfo.hpp"

#include "TheraUCI/MultiStream.hpp"
#include "TheraUCI/stringUtils.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <fstream>
#include <chrono>
#include <atomic>
#include <thread>
#include <condition_variable>

static MultiStream out;
static std::ofstream logfile;
static std::chrono::high_resolution_clock::time_point search_start;

void iterationEndCallback(Thera::SearchResult const& result){
    const auto end = std::chrono::high_resolution_clock::now();
    out << "info depth " << result.depthReached << " ";
    if (result.isMate){
        int movesLeft = (result.depthReached+3)/2;
        if (result.maxEval < 0){
            movesLeft = -movesLeft;
        }
        out << "score mate " << movesLeft << " ";
    }
    else{
        out << "score cp " << static_cast<int>(result.maxEval) << " ";
    }
    out << "nodes " << result.nodesSearched << " ";
    std::chrono::milliseconds dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - search_start);
    out << "time " << dur.count() << " ";
    out << "\n";
    out.flush();
    std::cout.flush();
}

constexpr int infiniteDepth = 9999;

static Thera::Board board;
static Thera::MoveGenerator generator;

static std::condition_variable searchStartCond;
static std::mutex searchStartMutex;

static std::atomic<bool> searchShouldStop = false;
static std::atomic<bool> searchThreadShouldExit = false;
static struct SearchParameters{
    std::optional<std::chrono::milliseconds> maxSearchTime;
    int depth = infiniteDepth;
    bool silent = false;
} searchParameters;

void searchThreadFunction(){
    while (!searchThreadShouldExit){
        std::unique_lock lock(searchStartMutex);
        searchStartCond.wait(lock);
        if (searchThreadShouldExit){
            return;
        }

        search_start = std::chrono::high_resolution_clock::now();
        auto moves = Thera::search(board, generator, searchParameters.depth, searchParameters.maxSearchTime, searchShouldStop, iterationEndCallback);
        const auto end = std::chrono::high_resolution_clock::now();

        auto bestMove = getRandomBestMove(moves);
        std::chrono::duration<double> dur = end-search_start;
        if (searchParameters.silent){
            continue;
        }

        out << "bestmove " << bestMove.move.toString();
        if (bestMove.ponderMove.has_value()){
            out << " ponder " << bestMove.ponderMove.value().toString();
        }
        out << "\n";
        out.flush();
        std::cout.flush();
        logfile << "Search took " << dur.count() << "s.\n";
    }
}

int main(){
    std::string line;
    std::stringstream lineStream;
    std::string buffer;

    std::thread searchThread(searchThreadFunction);

    logfile.open("/tmp/TheraUCI.log");
    if (!logfile.is_open()){
        std::cerr << "Unable to open logfile. Exiting...\n";
        return 1;
    }
    out.linkStream(logfile);
    out.linkStream(std::cout);

    // expect the UCI command
    std::getline(std::cin, line);
    lineStream.clear();
    lineStream << line;
    lineStream >> buffer;
    if (buffer != "uci"){
        out << "Non UCI compliant GUI detected! Exiting...\n";
        return 0;
    }

    // construct the current version
    std::string version = Thera::Utils::GitInfo::hash;
    if (Thera::Utils::GitInfo::isDirty){
        version += " + local changes";
    }

    // send ids
    out << "id name Thera (Git " + version + ")\n";
    out << "id author Robotino\n";

    out << "uciok\n";

    int numMoves;

    while (true){
        out.flush();
        logfile.flush();
        std::getline(std::cin, line);
        logfile << "[cin] " + line + "\n";
        logfile.flush();
        lineStream = std::stringstream();
        lineStream << line;
        
        lineStream >> buffer;
        if (buffer == "position"){
            lineStream >> buffer;
            if (buffer == "startpos"){
                board.loadFromFEN(Thera::Utils::startingFEN);

                // remove the "moves" subcommand
                buffer = "";
                lineStream >> buffer;
                buffer = trim(buffer);
                if (buffer.length() && buffer != "moves"){
                    logfile << "Invalid subcommand to 'position': '" + buffer + "'\n";
                    break;
                }
            }
            else if (buffer == "fen"){
                std::string fen;
                lineStream >> buffer;
                while (lineStream.rdbuf()->in_avail() && buffer != "moves"){
                    fen += buffer + " ";
                    lineStream >> buffer;
                }

                try{
                    board.loadFromFEN(fen);
                }
                catch (std::invalid_argument){
                    logfile << "Invalid FEN string: \"" + lineStream.str() + "\"\n";
                }
            }
            else{
                logfile << "Invalid subcommand for 'position'\n";
                continue;
            }

            numMoves = 0;
            // apply the moves
            while (lineStream.rdbuf()->in_avail()){
                const auto possibleMoves = generator.generateAllMoves(board);
                lineStream >> buffer;
                Thera::Move inputMove = Thera::Move::fromString(trim(buffer));
                
                auto moveIt = std::find_if(possibleMoves.begin(), possibleMoves.end(), [&](auto other){return Thera::Move::isSameBaseMove(inputMove, other);});
                if (moveIt != possibleMoves.end()){
                    // apply the found move since the input move
                    // won't have any stats attached
                    board.applyMove(*moveIt);
                    logfile << "Made move: " << moveIt->toString() << "\n";
                }
                else{
                    logfile << "Invalid move detected.\n";
                    break;
                }
                numMoves++;
            }

            // log the resulting position
            logfile << board.storeToFEN() << "\n";
        }
        else if (buffer == "isready"){
            out << "readyok\n";
        }
        else if (buffer == "quit"){
            searchParameters.silent = true;
            searchShouldStop = true;            
            searchThreadShouldExit = true;
            searchStartCond.notify_one();
            searchThread.join();
            return 0;
        }
        else if (buffer == "stop"){
            searchShouldStop = true;            
            searchStartCond.notify_one();
        }
        else if (buffer == "go"){
            std::chrono::milliseconds wtime = std::chrono::milliseconds::zero();
            std::chrono::milliseconds btime = std::chrono::milliseconds::zero();
            std::chrono::milliseconds winc = std::chrono::milliseconds::zero();
            std::chrono::milliseconds binc = std::chrono::milliseconds::zero();
            std::optional<std::chrono::milliseconds> movetime;
            searchParameters.depth = infiniteDepth;
            while (lineStream.rdbuf()->in_avail()){
                lineStream >> buffer;
                if (buffer == "wtime"){
                    lineStream >> buffer;
                    wtime = std::chrono::milliseconds(std::stoi(buffer));
                }
                else if (buffer == "btime"){
                    lineStream >> buffer;
                    btime = std::chrono::milliseconds(std::stoi(buffer));
                }
                else if (buffer == "winc"){
                    lineStream >> buffer;
                    winc = std::chrono::milliseconds(std::stoi(buffer));
                }
                else if (buffer == "binc"){
                    lineStream >> buffer;
                    binc = std::chrono::milliseconds(std::stoi(buffer));
                }
                else if (buffer == "movetime"){
                    lineStream >> buffer;
                    movetime = std::chrono::milliseconds(std::stoi(buffer));
                }
                else if (buffer == "depth"){
                    lineStream >> searchParameters.depth;
                }
            }
            
            if (movetime.has_value()) searchParameters.maxSearchTime = movetime.value();
            else if ((wtime + btime + winc + binc).count() != 0){
                auto inc = board.getColorToMove() == Thera::PieceColor::White ? winc : binc;
                auto time = (board.getColorToMove() == Thera::PieceColor::White ? wtime : btime) - std::chrono::seconds(2);

                // assume a game lasts max. 60 moves.
                int movesLeft = 80*2-numMoves;
                auto maxTimePerMoveLeft = std::max(time / movesLeft, std::chrono::milliseconds(10));
                searchParameters.maxSearchTime = inc + maxTimePerMoveLeft;
                logfile << "Searching for " << searchParameters.maxSearchTime.value().count() << "ms.\n"; 
            }
            if (searchParameters.depth < infiniteDepth){
                logfile << "Searching to depth " << searchParameters.depth << ".\n"; 
            }
            searchParameters.silent = false;

            searchShouldStop = false;
            searchStartCond.notify_one();
        }

        if (lineStream.rdbuf()->in_avail()){
            std::string remainder(lineStream.str().substr(lineStream.tellg()));
            logfile << "Not completely processed line: \"" + remainder + "\"\n";
        }
    }

    return 0;
}