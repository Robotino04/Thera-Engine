#include "Thera/TranspositionTable.hpp"

namespace Thera {

void TranspositionTable::addEntry(Board const& board, int eval, NegamaxState nstate) {
    Entry entry;
    entry.eval = eval;
    entry.depth = nstate.depth;

    if (entry.eval <= nstate.alpha) {
        entry.flag = Entry::Flag::UpperBound;
    }
    else if (entry.eval >= nstate.beta) {
        entry.flag = Entry::Flag::LowerBound;
    }
    else {
        entry.flag = Entry::Flag::Exact;
    }

    internalTable.insert_or_assign(board.getCurrentHash(), entry);
}

std::optional<int> TranspositionTable::readPotentialEntry(Board const& board, NegamaxState& nstate) {
    if (internalTable.contains(board.getCurrentHash())) {
        auto entry = internalTable.at(board.getCurrentHash());
        if (entry.depth >= nstate.depth) {
            if (entry.flag == Entry::Flag::Exact) {
                return entry.eval;
            }
            else if (entry.flag == Entry::Flag::LowerBound) {
                nstate.alpha = std::max(nstate.alpha, entry.eval);
            }
            else if (entry.flag == Entry::Flag::UpperBound) {
                nstate.beta = std::min(nstate.beta, entry.eval);
            }
            if (nstate.alpha > nstate.beta) {
                return entry.eval;
            }
        }
    }

    return {};
}

}
