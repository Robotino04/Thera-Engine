#include "Thera/TranspositionTable.hpp"

namespace Thera{

void TranspositionTable::addEntry(Board const& board, int eval, int depth, int alpha, int beta){
    Entry entry;
    entry.eval = eval;
    entry.depth = depth;

    if (entry.eval <= alpha){
        entry.flag = Entry::Flag::UpperBound;
    }
    else if (entry.eval >= beta){
        entry.flag = Entry::Flag::LowerBound;
    }
    else{
        entry.flag = Entry::Flag::Exact;
    }

    internalTable.insert_or_assign(board.getCurrentHash(), entry);
}

std::optional<int> TranspositionTable::readPotentialEntry(Board const& board, int depth, int& alpha, int& beta){
    if (internalTable.contains(board.getCurrentHash())){
        auto entry = internalTable.at(board.getCurrentHash());
        if (entry.depth >= depth){
            if (entry.flag == Entry::Flag::Exact){
                return entry.eval;
            }
            else if (entry.flag == Entry::Flag::LowerBound){
                alpha = std::max(alpha, entry.eval);
            }
            else if (entry.flag == Entry::Flag::UpperBound){
                beta = std::min(beta, entry.eval);
            }
            if (alpha > beta){
                return entry.eval;
            }
        }
    }

    return {};
}

}