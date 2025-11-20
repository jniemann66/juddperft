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

#ifndef _SEARCH_H
#define _SEARCH_H 1

#include "movegen.h"
#include "hashtable.h"

namespace juddperft {

#define PV_SIZE 64
#define MAX_THREADS 64 // Hard limit for number of threads to use
#define ZERO_64 0ULL

#define SEARCH_STOPPED 0x80000000

struct PerftInfo{
	int64_t nMoves{ZERO_64};
	int64_t nCapture{ZERO_64};
	int64_t nEPCapture{ZERO_64};
	int64_t nCastle{ZERO_64};
	int64_t nCastleLong{ZERO_64};
	int64_t nPromotion{ZERO_64};
	int64_t nCheck{ZERO_64};
	int64_t nCheckmate{ZERO_64};
};

int64_t perft(ChessPosition P, int maxdepth, int depth, PerftInfo* pI);		// Single-Threaded
void perftFast(const ChessPosition & P, int depth, int64_t & nNodes);		// Simple, Hash-Table-using perft
void perftFastIterative(const ChessPosition & P, int depth, int64_t & nNodes);	// Iterative version - Hash Table functionality still broken
void perftMT(ChessPosition P, int maxdepth, int depth, PerftInfo* pI);		// Multi-Threaded driver for perft()
void perftFastMT(ChessPosition P, int depth, int64_t & nNodes);				// Multi-Threaded driver for perftFast()

#ifndef NULL
#define NULL nullptr
#endif

} //namespace juddperft
#endif // _SEARCH_H
