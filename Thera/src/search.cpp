#include "Thera/search.hpp"
#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Utils/ScopeGuard.hpp"
#include "Thera/Utils/ChessTerms.hpp"

#include <numeric>
#include <cstdlib>
#include <algorithm>
#include <unordered_map>
#include <array>

namespace Thera{

namespace EvaluationValues{
static const std::unordered_map<PieceType, int> pieceValues = {
    {PieceType::Pawn, 100},
    {PieceType::Bishop, 300},
    {PieceType::Knight, 300},
    {PieceType::Rook, 500},
    {PieceType::Queen, 900},
    {PieceType::King, 20000},
};

// https://www.chessprogramming.org/Simplified_Evaluation_Function
static const std::array<std::array<int, 64>, 7> simplifiedEvalScores{
    std::array<int, 64>{  // no piece / placeholder
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
    },
    { // pawn
         0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
         5,  5, 10, 25, 25, 10,  5,  5,
         0,  0,  0, 20, 20,  0,  0,  0,
         5, -5,-10,  0,  0,-10, -5,  5,
         5, 10, 10,-30,-30, 10, 10,  5,
         0,  0,  0,  0,  0,  0,  0,  0
    },
    {  // knight
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-35,-30,-30,-30,-30,-35,-50,
    },
    {  // bishop
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-10,-10,-10,-10,-10,-20,
    },
    {  // rook
         0,  0,  0,  0,  0,  0,  0,  0,
         5, 10, 10, 10, 10, 10, 10,  5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
         0,  0,  0,  5,  5,  0,  0,  0
    },
    {  // queen
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
         -5,  0,  5,  5,  5,  5,  0, -5,
          0,  0,  5,  5,  5,  5,  0, -5,
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    },
    {  // king
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -10,-20,-20,-20,-20,-20,-20,-10,
         20, 20,  0,  0,  0,  0, 20, 20,
         20, 30, 10,  0,  0, 10, 30, 20
    }
};

}

struct TranspositionTableEntry{
    enum class Flag{
        Exact,
        LowerBound,
        UpperBound,
    } flag;
    int eval;
    int depth;
};


std::vector<Move> preorderMoves(std::vector<Move> const&& moves, Board& board, MoveGenerator& generator){
    struct ScoredMove{
        Move move;
        int score = 0;

        bool operator <(ScoredMove const& other) const{
            return score < other.score;
        }
    };

    static const int million = 1'000'000;
    static const int promotionScore = 6 * million;
    static const int winningCaptureScore = 8 * million;
    static const int loosingCaptureScore = 2 * million;

    std::vector<ScoredMove> scoredMoves;
    scoredMoves.reserve(moves.size());

    board.switchPerspective();
    generator.generateAttackData(board);
    board.switchPerspective();

    for (auto move : moves){
        ScoredMove& scoredMove = scoredMoves.emplace_back();
        scoredMove.move = move;

        Piece capturedPiece = board.at(move.endIndex);
        if (capturedPiece.type != PieceType::None){
            int pieceValueDifference = EvaluationValues::pieceValues.at(capturedPiece.type) - EvaluationValues::pieceValues.at(move.piece.type);
            if (generator.getAttackedSquares()[move.endIndex]){
                scoredMove.score += (pieceValueDifference >= 0 ? winningCaptureScore : loosingCaptureScore) + pieceValueDifference;
            }
            else{
                scoredMove.score += pieceValueDifference + winningCaptureScore;
            }
        }

        if (move.promotionType != PieceType::None){
            scoredMove.score += promotionScore + EvaluationValues::pieceValues.at(move.promotionType);
        }
    }

    std::sort(scoredMoves.rbegin(), scoredMoves.rend());

    std::vector<Move> sortedMoves;
    sortedMoves.reserve(moves.size());
    for (auto move : scoredMoves){
        sortedMoves.push_back(move.move);
    }
    return sortedMoves;
}

int getMaterial(PieceColor color, Board const& board){
    int score = 0;
    for (auto type : Utils::allPieceTypes){
        score += board.getBitboard({type, color}).getNumPieces() * EvaluationValues::pieceValues.at(type);
    }
    return score;
}

int getPiecePositionValue(PieceType piece, Bitboard positions){
    int score = 0;
    while (positions.hasPieces()){
        auto pos = positions.getLS1B();
        positions.clearLS1B();
        score += EvaluationValues::simplifiedEvalScores.at(static_cast<int>(piece)).at(pos);
    }
    return score;
}

int endgameKingEval(Board const& board, float endgameProgress, PieceColor otherColor, float gameDirection){
    int eval = 0;

    if (gameDirection > 0.0f){
        Coordinate enemyKingPos = Coordinate(board.getBitboard({PieceType::King, otherColor}).getLS1B());
        int enemyKingDistanceFromCenter = std::max(3 - int(enemyKingPos.x), int(enemyKingPos.x) - 4) + std::max(3 - int(enemyKingPos.y), int(enemyKingPos.y) - 4);
        eval += enemyKingDistanceFromCenter;

        int kingDistance = Utils::manhattanDistance(
            Coordinate(board.getBitboard({PieceType::King, PieceColor::White}).getLS1B()),
            Coordinate(board.getBitboard({PieceType::King, PieceColor::Black}).getLS1B())
        );
        eval += 14 - kingDistance;
    }

    return eval * 10 * endgameProgress;
}

int evaluate(Board& board, MoveGenerator& generator){
    PieceColor color = board.getColorToMove();
    PieceColor otherColor = board.getColorToNotMove();

    if (board.is3FoldRepetition()){
        return 0;
    }

    const int maxMaterial = 2*EvaluationValues::pieceValues.at(PieceType::Rook) + EvaluationValues::pieceValues.at(PieceType::Knight) + EvaluationValues::pieceValues.at(PieceType::Bishop);

    int eval = 0;
    const int whiteMaterial = getMaterial(color, board);
    const int blackMaterial = getMaterial(otherColor, board);
    eval += whiteMaterial;
    eval -= blackMaterial;
    const int materialLeft = whiteMaterial + blackMaterial - 2 * EvaluationValues::pieceValues.at(PieceType::King);
    const float gameDirection = (eval >= 0) ? 1.f : -1.f;
    const float endgameProgress = 1.f - (std::min(1.0f, float(materialLeft) / float(maxMaterial)));

    for (auto pieceType : Utils::allPieceTypes){
        int whiteMaterial = getPiecePositionValue(pieceType, board.getBitboard({pieceType, PieceColor::White}));
        int blackMaterial = getPiecePositionValue(pieceType, board.getBitboard({pieceType, PieceColor::Black}).flipped());
        if (board.getColorToMove() == PieceColor::White){
            eval += float(whiteMaterial) * (1.0f - endgameProgress);
            eval -= float(blackMaterial) * (1.0f - endgameProgress);
        }
        else{
            eval -= float(whiteMaterial) * (1.0f - endgameProgress);
            eval += float(blackMaterial) * (1.0f - endgameProgress);
        }
    }

    eval += endgameKingEval(board, endgameProgress, otherColor, gameDirection);
    eval -= endgameKingEval(board, endgameProgress, color, -gameDirection);

    return eval;
}

int getSearchExtensionDepth(Move const& lastMove, Board const& board, MoveGenerator& generator){
    generator.generateAttackData(board);

    int searchExtensions = 0;

    // extend checks
    if (generator.isInCheck(board)){
        searchExtensions++;
    }

    // extend promotions
    if (lastMove.promotionType != PieceType::None){
        searchExtensions++;
    }

    return searchExtensions;
}

bool negamaxStep(int newEval, int& bestEval, int& alpha, int beta){
    alpha = std::max(alpha, newEval);
    bestEval = std::max(bestEval, newEval);
    return alpha > beta;
}


int capturesOnlyNegamax(Board& board, MoveGenerator& generator, int alpha, int beta, std::optional<std::chrono::steady_clock::time_point> searchStop, SearchResult& searchResult){
    if (searchStop.has_value() && std::chrono::steady_clock::now() >= searchStop.value()) throw SearchStopException();

    if (board.is3FoldRepetition()){
        return 0;
    }

    int bestEvaluation = -evalInfinity;
    if (negamaxStep(evaluate(board, generator), bestEvaluation, alpha, beta))
        return bestEvaluation;

    generator.capturesOnly = true;
    auto moves = generator.generateAllMoves(board);
    generator.capturesOnly = false;

    moves = preorderMoves(std::move(moves), board, generator);
    for (auto move : moves){
        board.applyMove(move);
        Utils::ScopeGuard moveRewind_guard([&](){board.rewindMove();});
        int eval = -capturesOnlyNegamax(board, generator, -beta, -alpha, searchStop, searchResult);
        if (negamaxStep(eval, bestEvaluation, alpha, beta))
            break;
    }

    return bestEvaluation;
}

int negamax(Board& board, MoveGenerator& generator, int depth, int alpha, int beta, std::optional<std::chrono::steady_clock::time_point> searchStop, std::unordered_map<uint64_t, TranspositionTableEntry>& transpositionTable, SearchResult& searchResult){
    if (searchStop.has_value() && std::chrono::steady_clock::now() >= searchStop.value()) throw SearchStopException();

    if (board.is3FoldRepetition()){
        return 0;
    }

    if (depth == 0){
        searchResult.nodesSearched++;
        return capturesOnlyNegamax(board, generator, alpha, beta, searchStop, searchResult);
    }

    if (board.is3FoldRepetition()){
        return 0;
    }

    if (transpositionTable.contains(board.getCurrentHash())){
        auto entry = transpositionTable.at(board.getCurrentHash());
        if (entry.depth >= depth){
            if (entry.flag == TranspositionTableEntry::Flag::Exact){
                return entry.eval;
            }
            else if (entry.flag == TranspositionTableEntry::Flag::LowerBound){
                alpha = std::max(alpha, entry.eval);
            }
            else if (entry.flag == TranspositionTableEntry::Flag::UpperBound){
                beta = std::min(beta, entry.eval);
            }
            if (alpha > beta){
                return entry.eval;
            }
        }
    }
    
    int bestEvaluation = -evalInfinity;

    auto moves = generator.generateAllMoves(board);

    if (moves.size() == 0){
        if (generator.isInCheck(board)){
            bestEvaluation = -evalInfinity;
        }
        else{
            bestEvaluation = 0.0f;
        }
    }
    else{
        moves = preorderMoves(std::move(moves), board, generator);
        for (auto move : moves){
            board.applyMove(move);

            int searchExtensions = getSearchExtensionDepth(move, board, generator);

            Utils::ScopeGuard moveRewind_guard([&](){board.rewindMove();});
            int eval = -negamax(board, generator, depth-1 + searchExtensions, -beta, -alpha, searchStop, transpositionTable, searchResult);
            if (negamaxStep(eval, bestEvaluation, alpha, beta))
                break;
        }
    }

    TranspositionTableEntry entry;
    entry.eval = bestEvaluation;
    entry.depth = depth;

    if (entry.eval <= alpha){
        entry.flag = TranspositionTableEntry::Flag::UpperBound;
    }
    else if (entry.eval >= beta){
        entry.flag = TranspositionTableEntry::Flag::LowerBound;
    }
    else{
        entry.flag = TranspositionTableEntry::Flag::Exact;
    }

    transpositionTable.insert_or_assign(board.getCurrentHash(), entry);

    return entry.eval;
}

SearchResult search(Board& board, MoveGenerator& generator, int depth, std::optional<std::chrono::milliseconds> maxSearchTime, std::function<void(SearchResult const&)> iterationEndCallback){
    if (depth == 0) throw std::invalid_argument("Depth may not be 0");

    auto moves = generator.generateAllMoves(board);
    // move preordering
    SearchResult result;
    result.depthReached = 0;
    for (auto move : moves){
        result.moves.emplace_back(move);
    }
    SearchResult resultTmp = result;

    std::chrono::steady_clock::time_point searchStopTP;
    if (maxSearchTime.has_value()){
        searchStopTP = std::chrono::steady_clock::now() + maxSearchTime.value();
    }
    else{
        searchStopTP = std::chrono::steady_clock::time_point::max();
    }


    // iterative deepening
    for (int currentDepth=1; currentDepth <= depth; currentDepth++){
        int alpha = -evalInfinity;
        int beta = evalInfinity;

        // sort in reverse to first search the best moves
        std::sort(resultTmp.moves.rbegin(), resultTmp.moves.rend());
        try{
            for (auto& move : resultTmp.moves){
                std::unordered_map<uint64_t, TranspositionTableEntry> transpositionTable;
                board.applyMove(move.move);
                Utils::ScopeGuard moveRewind_guard([&](){board.rewindMove();});
                move.eval = -negamax(board, generator, currentDepth-1, -beta, -alpha, searchStopTP, transpositionTable, resultTmp);

                alpha = std::max(alpha, move.eval);
                if (alpha > beta)
                    break;
            }
        }
        catch(SearchStopException){
            return resultTmp;
        }
        resultTmp.depthReached = currentDepth;
        result = resultTmp;
        resultTmp.nodesSearched = 0;
        // exit early if a checkmate is found
        result.maxEval = -evalInfinity;
        for (auto move : result.moves){
            result.maxEval = std::max(result.maxEval, move.eval);
        }
        result.isMate = std::abs(result.maxEval) == evalInfinity;

        iterationEndCallback(result);

        if (result.isMate){
            return result;
        }
    }

    return result;
}

EvaluatedMove getRandomBestMove(SearchResult const& moves){
    int bestEval = moves.moves.front().eval;
        std::vector<EvaluatedMove> bestMoves;
    for (auto move : moves.moves){
        if (move.eval > bestEval){
            bestMoves.clear();
            bestEval = move.eval;
        }
        if (move.eval == bestEval){
            bestMoves.push_back(move);
        }
    }

    const int selectedMoveIndex = rand() % bestMoves.size();
    return bestMoves.at(selectedMoveIndex);
}

}