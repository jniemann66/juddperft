#include "hashtable.h" 
#include "search.h"

#include <iostream>
#include <random>
#include <cmath>

ZobristKeySet::ZobristKeySet()
{
	ZobristKeySet::Generate();
}

bool ZobristKeySet::Generate()
{
	// Create a Random Number Generator, using the 64-bit Mersenne Twister Algorithm
	// with a uniform distribution of ints 
	std::random_device rd;
	std::mt19937_64 e2(rd());
	std::uniform_int_distribution<unsigned long long int> dist(0, 0xffffffffffffffff);
	//
	for (int n = 0; n < 16; ++n) {
		for (int m = 0; m<64; ++m) {
			ZobristKeySet::zkPieceOnSquare[n][m] = dist(rd);
		}
	}

	ZobristKeySet::zkBlackToMove = dist(rd);
	ZobristKeySet::zkWhiteCanCastle = dist(rd);
	ZobristKeySet::zkWhiteCanCastleLong = dist(rd);
	ZobristKeySet::zkBlackCanCastle = dist(rd);
	ZobristKeySet::zkBlackCanCastleLong = dist(rd);
	
	for (int m = 0; m<24; ++m) {
		ZobristKeySet::zkPerftDepth[m] = dist(rd);
	}	

	ZobristKeySet::zkDoBlackCastle=
		ZobristKeySet::zkPieceOnSquare[BKING][59]^ // Remove King from e8
		ZobristKeySet::zkPieceOnSquare[BKING][57]^ // Place King on g8
		ZobristKeySet::zkPieceOnSquare[BROOK][56]^ // Remove Rook from h8
		ZobristKeySet::zkPieceOnSquare[BROOK][58]^ // Place Rook on f8
		ZobristKeySet::zkBlackCanCastle;			// (unconditionally) flip black castling

	ZobristKeySet::zkDoBlackCastleLong=
		ZobristKeySet::zkPieceOnSquare[BKING][59]^ // Remove King from e8
		ZobristKeySet::zkPieceOnSquare[BKING][61]^ // Place King on c8
		ZobristKeySet::zkPieceOnSquare[BROOK][63]^ // Remove Rook from a8
		ZobristKeySet::zkPieceOnSquare[BROOK][60]^ // Place Rook on d8
		ZobristKeySet::zkBlackCanCastleLong;		// (unconditionally) flip black castling long

	ZobristKeySet::zkDoWhiteCastle=
		ZobristKeySet::zkPieceOnSquare[WKING][3]^ // Remove King from e1
		ZobristKeySet::zkPieceOnSquare[WKING][1]^ // Place King on g1
		ZobristKeySet::zkPieceOnSquare[WROOK][0]^ // Remove Rook from h1
		ZobristKeySet::zkPieceOnSquare[WROOK][2]^ // Place Rook on f1
		ZobristKeySet::zkWhiteCanCastle;			// (unconditionally) flip white castling

	ZobristKeySet::zkDoWhiteCastleLong=
		ZobristKeySet::zkPieceOnSquare[WKING][3]^ // Remove King from e1
		ZobristKeySet::zkPieceOnSquare[WKING][5]^ // Place King on c1
		ZobristKeySet::zkPieceOnSquare[WROOK][7]^ // Remove Rook from a1
		ZobristKeySet::zkPieceOnSquare[WROOK][4]^ // Place Rook on d1
		ZobristKeySet::zkWhiteCanCastleLong;		// (unconditionally) flip white castling long

	return false;
}

ZobristKeySet ZobristKeys;
