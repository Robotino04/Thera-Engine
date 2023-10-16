#pragma once

#include "Thera/Board.hpp"

#include <unordered_map>
#include <optional>

namespace Thera{
    
class TranspositionTable{
    public:
        struct Entry{
            enum class Flag{
                Exact,
                LowerBound,
                UpperBound,
            } flag;
            int eval;
            int depth;
        };

        void addEntry(Board const& board, int eval, int depth, int alpha, int beta);

        std::optional<int> readPotentialEntry(Board const& board, int depth, int& alpha, int& beta);

    private:
        std::unordered_map<uint64_t, Entry> internalTable;
};

}
