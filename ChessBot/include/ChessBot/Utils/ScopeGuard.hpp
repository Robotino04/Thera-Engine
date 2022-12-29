#pragma once

#include <functional>

namespace ChessBot::Utils{

class ScopeGuard{
    public:
        ScopeGuard(std::function<void()> begin, std::function<void()> end): end(end){
            begin();
        }
        ~ScopeGuard(){
            end();
        }
        ScopeGuard(ScopeGuard const&) = delete;
        ScopeGuard& operator =(ScopeGuard const&) = delete;

    private:
        std::function<void()> end;
};

}