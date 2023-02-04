#pragma once

#include <functional>

namespace Thera::Utils{

class ScopeGuard{
    public:
        ScopeGuard(std::function<void()> end): end(end){
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