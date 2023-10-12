#include "Thera/Board.hpp"
#include "Thera/Move.hpp"

#include "Thera/Utils/Math.hpp"
#include "Thera/Utils/ChessTerms.hpp"
#include "Thera/Utils/BuildType.hpp"
#include "Thera/Utils/ScopeGuard.hpp"

#include "Thera/Coordinate.hpp"

#include <stdexcept>
#include <map>
#include <sstream>
#include <random>

namespace Thera{

Piece Board::at(Coordinate index) const{
	for (auto piece : Utils::allPieces){
		if (getBitboard(piece).isOccupied(index)){
			return piece;
		}
	}
	return {PieceType::None, PieceColor::White};
}

static std::string generateFenErrorText(std::string const& fen, int charIndex){
	std::stringstream ss;
	ss << "Invalid character '" << fen.at(charIndex) << "' in FEN string!\n";
	ss << " \"" << fen << "\"\n  ";
	for (int i=0; i<charIndex;i++){
		ss << " ";
	}
	ss << "Ʌ";
	return ss.str();
}

void Board::loadFromFEN(std::string fen){
	static const std::map<char, PieceType> fenCharsToPieceType = {
		{'p', PieceType::Pawn},
		{'b', PieceType::Bishop},
		{'n', PieceType::Knight},
		{'r', PieceType::Rook},
		{'q', PieceType::Queen},
		{'k', PieceType::King},
	};

	std::default_random_engine randomGenerator;
	std::uniform_int_distribution<uint64_t> distribution;
	randomGenerator.seed(0);
	for (auto& tableEntry : zobristTable){
		for (uint64_t& entry : tableEntry)
			entry = 0;
		for (auto piece : Utils::allPieces){
			tableEntry.at(piece.getRaw()) = distribution(randomGenerator);
		}
	}
	zobristBlackToMove = distribution(randomGenerator);
	currentState.zobristHash = 0;

	currentState.allPieceBitboard = Bitboard();
	for (auto& bitboard : currentState.pieceBitboards){
		bitboard = Bitboard();
	}

	uint8_t x = 0, y = 7;
	int charIndex = -1;
	while(++charIndex < fen.size()){
		char c = fen.at(charIndex);
		switch (tolower(c)){
			case '/':
				y--;
				x = 0;
				break;
			case '0'...'8':
				x += c - '0';
				break;
			case ' ':
				goto parse_turn;
			default:{
				Piece piece;
				try{
					piece = Piece(fenCharsToPieceType.at(tolower(c)), islower(c) ? PieceColor::Black : PieceColor::White);
				}
				catch(std::out_of_range){
					throw std::invalid_argument(generateFenErrorText(fen, charIndex));
				}

				placePiece(Coordinate(x, y), piece);
				x++;
				break;
			}
		}
	}
	parse_turn:
	charIndex++; // skip space

	currentState.isWhiteToMove = true;
	if (fen.at(charIndex) == 'w') ;
	else if(fen.at(charIndex) == 'b') switchPerspective();
	else
		throw std::invalid_argument(generateFenErrorText(fen, charIndex));
	charIndex += 2; // consume side to move and space

	currentState.canBlackCastleLeft = false;
	currentState.canBlackCastleRight = false;
	currentState.canWhiteCastleLeft = false;
	currentState.canWhiteCastleRight = false;

	while (fen.at(charIndex) != ' '){
		switch(fen.at(charIndex)){
			case 'k': currentState.canBlackCastleRight = true; break;
			case 'K': currentState.canWhiteCastleRight = true; break;
			case 'q': currentState.canBlackCastleLeft = true; break;
			case 'Q': currentState.canWhiteCastleLeft = true; break;
			case '-': charIndex++; goto parse_en_passant; // skip char since loop would normaly do that
			default:
				throw std::invalid_argument(generateFenErrorText(fen, charIndex));
				break;
		}
		charIndex++;
	}
	parse_en_passant:
	charIndex++; // skip space

	if (!(Utils::isInRange(fen.at(charIndex), 'a', 'h') || fen.at(charIndex) == '-')){
		throw std::invalid_argument(generateFenErrorText(fen, charIndex));
	}
	if (fen.at(charIndex) == '-'){
		currentState.hasEnPassant = false;
	}
	else{
		currentState.enPassantSquareForFEN = Utils::squareFromAlgebraicNotation(fen.substr(charIndex, 2));
		currentState.enPassantSquareToCapture = currentState.enPassantSquareForFEN;
		currentState.enPassantSquareToCapture.y += (currentState.isWhiteToMove ? -1 : 1);
		currentState.hasEnPassant = true;
		charIndex += 1;
	}
	charIndex += 2; // skip sth. and space
	

	// TODO: Implement move counters

	while (!rewindStack.empty()) rewindStack.pop();

	numberOfPositionRepetitions.clear();
	numberOfPositionRepetitions.insert({getCurrentHash(), 1});
}
std::string Board::storeToFEN() const{
	static const std::map<PieceType, char> pieceTypeToFenChars = {
		{PieceType::Pawn, 'p'},
		{PieceType::Bishop, 'b'},
		{PieceType::Knight, 'n'},
		{PieceType::Rook, 'r'},
		{PieceType::Queen, 'q'},
		{PieceType::King, 'k'},
	};

	std::string fen = "";

	for (int y=7; y >= 0; y--){
		for (int x=0; x<8; x++){
		try{
			char c = pieceTypeToFenChars.at(at(Coordinate(x, y)).type);
			if (at(Coordinate(x, y)).color == PieceColor::White){
				c = std::toupper(c);
			}

			fen += c;
		}
		catch(std::out_of_range){
			// square is empty
			if (Utils::isInRange(fen.back(), '0', '7')){
				fen.back()++;
			}
			else{
				fen += '1';
			}
		}

		if (x == 7 && y > 0){
			fen += '/';
		}
		}
	}

	fen += ' ';
	fen += currentState.isWhiteToMove ? 'w' : 'b';

	fen += ' ';
	if (currentState.canWhiteCastleLeft) fen += 'Q';
	if (currentState.canWhiteCastleRight) fen += 'K';
	if (currentState.canBlackCastleLeft) fen += 'q';
	if (currentState.canBlackCastleRight) fen += 'k';

	if (!(currentState.canWhiteCastleLeft || currentState.canWhiteCastleRight || currentState.canBlackCastleLeft || currentState.canBlackCastleRight)){
		fen += '-';
	}

	fen += ' ';
	if (currentState.hasEnPassant){
		fen += Utils::squareToAlgebraicNotation(getEnPassantSquareForFEN());
	}
	else{
		fen += '-';
	}

	// TODO: implement move counters
	fen += " 0 1";

	return fen;
}

void Board::applyMove(Move const& move){
	// save the current state
	rewindStack.push(currentState);

	applyMoveStatic(move);

	// update State
	switchPerspective();

	uint64_t hash = getCurrentHash();
	if (numberOfPositionRepetitions.contains(hash)){
		numberOfPositionRepetitions.at(hash)++;
	}
	else{
		numberOfPositionRepetitions.insert({hash, 1});
	}
}

void Board::applyMoveStatic(Move const& move){
	if(move.startIndex == move.endIndex) return;

	removeCastlings(move.startIndex);
	removeCastlings(move.endIndex);
	
	// remove pieces from bitboards
	getPieceBitboardForOneColor(getColorToNotMove()).removePiece(move.endIndex);
	removePiece(move.endIndex);
	

	// apply the move to the bitboards
	currentState.allPieceBitboard.applyMove(move);
	getPieceBitboardForOneColor(getColorToMove()).applyMove(move);
	getBitboard(move.piece).applyMove(move);
	currentState.zobristHash ^= zobristTable.at(move.startIndex.getIndex64()).at(move.piece.getRaw());
	currentState.zobristHash ^= zobristTable.at(move.endIndex.getIndex64()).at(move.piece.getRaw());

	// promotion
	if (move.promotionType != PieceType::None){
		removePiece(move.endIndex);
		placePiece(move.endIndex, Piece(move.promotionType, getColorToMove()));
	}

	// en passant
	if (move.isEnPassant){
		removePiece(Coordinate(move.endIndex.x, move.startIndex.y));
		currentState.hasEnPassant = false;
	}
	if (move.isCastling){
		Move castlingMove = Move(move.castlingStart, move.castlingEnd);
		castlingMove.piece = {PieceType::Rook, move.piece.color};
		// apply the rook move to the bitboards
		getPieceBitboardForOneColor(castlingMove.piece.color).applyMove(castlingMove);
		currentState.allPieceBitboard.applyMove(castlingMove);
		getBitboard(castlingMove.piece).applyMove(castlingMove);
		currentState.zobristHash ^= zobristTable.at(castlingMove.startIndex.getIndex64()).at(castlingMove.piece.getRaw());
		currentState.zobristHash ^= zobristTable.at(castlingMove.endIndex.getIndex64()).at(castlingMove.piece.getRaw());
	}
	if (move.isDoublePawnMove){
		// get the "jumped" square
		currentState.enPassantSquareForFEN = Coordinate(move.startIndex.x, (move.startIndex.y + move.endIndex.y) / 2);
		currentState.enPassantSquareToCapture = move.endIndex;
		currentState.hasEnPassant = true;
	}
	else{
		currentState.hasEnPassant = false;
	}
}

void Board::rewindMove(){
	if (rewindStack.empty())
		throw std::runtime_error("Tried to rewind move, but no moves were made.");
	
	const int newCount = --numberOfPositionRepetitions.at(getCurrentHash());
	if (newCount == 0){
		numberOfPositionRepetitions.erase(getCurrentHash());
	}

	currentState = rewindStack.top();
	rewindStack.pop();
}


void Board::placePiece(Coordinate square, Piece piece){
	currentState.zobristHash ^= zobristTable.at(square.getIndex64()).at(piece.getRaw());
	getBitboard(piece).placePiece(square);
	currentState.allPieceBitboard.placePiece(square);
	getPieceBitboardForOneColor(piece.color).placePiece(square);
}

void Board::removePiece(Coordinate square){
	currentState.zobristHash ^= zobristTable.at(square.getIndex64()).at(at(square).getRaw());
	currentState.allPieceBitboard.removePiece(square);
	for (auto& bb : currentState.pieceBitboards){
		bb.removePiece(square);
	}
}

void Board::removeCastlings(Coordinate movedSquare){
	switch(movedSquare.getIndex64()){
		// rook moves
		case SquareIndex64::a8: currentState.canBlackCastleLeft = false; break;
		case SquareIndex64::h8: currentState.canBlackCastleRight = false; break;
		case SquareIndex64::a1: currentState.canWhiteCastleLeft = false; break;
		case SquareIndex64::h1: currentState.canWhiteCastleRight = false; break;

		// king moves
		case SquareIndex64::e8:
			currentState.canBlackCastleLeft = false;
			currentState.canBlackCastleRight = false;
			break;
		case SquareIndex64::e1:
			currentState.canWhiteCastleLeft = false;
			currentState.canWhiteCastleRight = false;
			break;
	}
}
}
