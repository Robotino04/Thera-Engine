#pragma once

namespace Thera::Utils{

enum class Architecture{
    x86_64,
    // x86_32,

#if defined(__x86_64__) || defined(_M_X64)
    Current = x86_64,
// #elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
//     Current = x86_32,
#else
    #warning Unimplemented CPU architecture. Performance might suffer.
#endif
};

}