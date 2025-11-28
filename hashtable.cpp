/*

MIT License

Copyright(c) 2016-2025 Judd Niemann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "hashtable.h"
#include "search.h"

#include <cmath>

#include <iostream>
#include <random>

namespace juddperft {

	ZobristKeySet::ZobristKeySet()
	{
		ZobristKeySet::generate();
	}

	bool ZobristKeySet::generate()
	{
		// Create a Random Number Generator, using the 64-bit Mersenne Twister Algorithm
		// with a uniform distribution of ints
		std::random_device rd;
		std::mt19937_64 rng(rd());
		std::uniform_int_distribution<unsigned long long int> dist(0, 0xffffffffffffffff);
		//
		for (int n = 0; n < 16; ++n) {
			for (int m = 0; m < 64; ++m) {
				ZobristKeySet::zkPieceOnSquare[n][m] = dist(rd);
			}
		}

		ZobristKeySet::zkBlackToMove = dist(rd);
		ZobristKeySet::zkWhiteCanCastle = dist(rd);
		ZobristKeySet::zkWhiteCanCastleLong = dist(rd);
		ZobristKeySet::zkBlackCanCastle = dist(rd);
		ZobristKeySet::zkBlackCanCastleLong = dist(rd);

		for (int m = 0; m < 24; ++m) {
			ZobristKeySet::zkPerftDepth[m] = dist(rd);
		}

		// generate pre-fabricated castling keys:
		ZobristKeySet::zkDoBlackCastle =
			ZobristKeySet::zkPieceOnSquare[BKING][59] ^ // Remove King from e8
			ZobristKeySet::zkPieceOnSquare[BKING][57] ^ // Place King on g8
			ZobristKeySet::zkPieceOnSquare[BROOK][56] ^ // Remove Rook from h8
			ZobristKeySet::zkPieceOnSquare[BROOK][58] ^ // Place Rook on f8
			ZobristKeySet::zkBlackCanCastle;			// (unconditionally) flip black castling

		ZobristKeySet::zkDoBlackCastleLong =
			ZobristKeySet::zkPieceOnSquare[BKING][59] ^ // Remove King from e8
			ZobristKeySet::zkPieceOnSquare[BKING][61] ^ // Place King on c8
			ZobristKeySet::zkPieceOnSquare[BROOK][63] ^ // Remove Rook from a8
			ZobristKeySet::zkPieceOnSquare[BROOK][60] ^ // Place Rook on d8
			ZobristKeySet::zkBlackCanCastleLong;		// (unconditionally) flip black castling long

		ZobristKeySet::zkDoWhiteCastle =
			ZobristKeySet::zkPieceOnSquare[WKING][3] ^ // Remove King from e1
			ZobristKeySet::zkPieceOnSquare[WKING][1] ^ // Place King on g1
			ZobristKeySet::zkPieceOnSquare[WROOK][0] ^ // Remove Rook from h1
			ZobristKeySet::zkPieceOnSquare[WROOK][2] ^ // Place Rook on f1
			ZobristKeySet::zkWhiteCanCastle;			// (unconditionally) flip white castling

		ZobristKeySet::zkDoWhiteCastleLong =
			ZobristKeySet::zkPieceOnSquare[WKING][3] ^ // Remove King from e1
			ZobristKeySet::zkPieceOnSquare[WKING][5] ^ // Place King on c1
			ZobristKeySet::zkPieceOnSquare[WROOK][7] ^ // Remove Rook from a1
			ZobristKeySet::zkPieceOnSquare[WROOK][4] ^ // Place Rook on d1
			ZobristKeySet::zkWhiteCanCastleLong;		// (unconditionally) flip white castling long

		return false;
	}

#ifdef _USE_HASH
	ZobristKeySet zobristKeys;
	HashTable <std::atomic<PerftTableEntry>> perftTable("Perft table");
#endif

} // namespace juddperft
