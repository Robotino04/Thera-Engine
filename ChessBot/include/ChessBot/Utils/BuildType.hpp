#pragma once

namespace ChessBot::Utils{

/**
 * @brief An enum representing the different build types in the current translation unit.
 * 
 */
enum class BuildType{
    /**
     * @brief Represents compilation with -g and -O0 (maybe -O1).
     * 
     */
    Debug,

    /**
     * @brief Represents compilation withoug -g and with -O2 (maybe -O3).
     * 
     */
    Release,

#ifndef NDEBUG
    /**
     * @brief Represents the build type of this TU.
     * 
     * Probably debug.
     * 
     */
    Current = Debug,
#else
    /**
     * @brief Represents the build type of this TU.
     * 
     * Probably release.
     * 
     */
    Current = Release,
#endif
};

}