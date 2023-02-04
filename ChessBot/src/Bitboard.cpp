#include "ChessBot/Bitboard.hpp"
#include "ChessBot/Board.hpp"
#include "ChessBot/Utils/Bits.hpp"

namespace ChessBot{

/**
 * @brief Forces the compiler to generate Bitboard implementations from 1 -> 64 pieces.
 */

#include "BitboardForceCompile.ipp"

template<int N>
void Bitboard<N>::removePiece(int8_t square){
    if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
        if (!isOccupied(square))
            throw std::invalid_argument("Tried to remove piece from empty square.");
        if (!Board::isOnBoard10x12(square))
            throw std::invalid_argument("Tried to remove piece from outside the board.");
    }
    
    (*this)[square] = false;

    const int pieceIndex = reverseOccupiedSquares.at(square);

    // move last entry in occupiedSquares to the empty spot
    occupiedSquares.at(pieceIndex) = occupiedSquares.at(numPieces-1);
    reverseOccupiedSquares.at(occupiedSquares.at(pieceIndex)) = pieceIndex;

    reverseOccupiedSquares.at(square) = -1;
    numPieces--;
}

template<int N>
uint64_t Bitboard<N>::getBoard8x8() const{
    uint64_t board = 0;
    for (int i=0; i<64; i++){
        board = Utils::setBit<uint64_t>(board, i, (*this)[Board::to10x12Coords(i)]);
    }
    return board;
}

template<int N>
std::array<int8_t, N> Bitboard<N>::getPieces8x8() const{

    uint64_t x = 0;
    for (int8_t i=0; i<64; i++){
        x = (x << 1) | isOccupied(Board::to10x12Coords(i));
    }
    x = Utils::reverseBits(x);

    //TODO: change to all 8x8 coords

    if constexpr (Utils::BuildType::Current == Utils::BuildType::Debug){
        if (std::popcount(x) != numPieces)
            throw std::runtime_error("Desync between bitboard and numPieces detected.");
    }

    std::array<int8_t, N> result;
    auto list = result.begin();



    if (x) do {
        int8_t idx = bitScanForward(x); // square index from 0..63
        *list++ = idx;
    } while (x &= x-1); // reset LS1B

    return result;
}

}