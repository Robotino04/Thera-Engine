#pragma once

#include <functional>
#include <stdexcept>

namespace Thera::Utils{

class ScopeGuard{
    public:
        ScopeGuard(std::function<void()> end): end(end){
        }
        ~ScopeGuard(){
            if (!released)
                end();
            released = true;
        }
        void release(){
            if (released)
                throw std::logic_error("ScopeGuard released twice!");
            
            end();
            released = true;
        }
        ScopeGuard(ScopeGuard const&) = delete;
        ScopeGuard& operator =(ScopeGuard const&) = delete;

    private:
        const std::function<void()> end;
        bool released = false;
};

}