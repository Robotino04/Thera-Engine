#pragma once

#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Piece.hpp"
#include "Thera/Coordinate.hpp"

#include <string>
#include <chrono>

enum class Mode {
    Play,
};

enum class BitboardSelection {
    None,
    SinglePiece,
    Debug,
    PinnedPieces,
    AllPieces,
    AttackedSquares,
    AttackedBySquares,
    AttackingSquares,
    PossibleMoveTargets,
};

struct Options {
    bool invertedColors = false;
    Mode mode = Mode::Play;
    int perftDepth = 1;
    std::string fen = Thera::Utils::startingFEN;
    bool bulkCounting = false;

    Thera::Piece shownPieceBitboard = {Thera::PieceType::None, Thera::PieceColor::White};
    BitboardSelection selectedBitboard = BitboardSelection::None;
    Thera::Coordinate squareSelection;

    int autoplayDepth = 0;
    std::chrono::milliseconds autoplaySearchTime;
};
