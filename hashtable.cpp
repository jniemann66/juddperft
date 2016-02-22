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
	return false;
}

ZobristKeySet ZobristKeys;
