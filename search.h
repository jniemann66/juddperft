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

namespace juddperft {

#define PV_SIZE 64
#define MAX_THREADS 64 // Hard limit for number of threads to use

#define SEARCH_STOPPED 0x80000000

struct PerftInfo
{
	nodecount_t nMoves{0};
	nodecount_t nCapture{0};
	nodecount_t nEPCapture{0};
	nodecount_t nCastle{0};
	nodecount_t nCastleLong{0};
	nodecount_t nPromotion{0};
	nodecount_t nCheck{0};
	nodecount_t nCheckmate{0};
};

// perft() : single-threaded; doesn't use hashtable, but does collect stats (EP, capture, checks ... etc)
nodecount_t perft(ChessPosition P, int maxdepth, int depth, PerftInfo* pI);

// perfFast() : single-threaded; does use hashtable; doesn't collect stats
void perftFast(const ChessPosition& P, int depth, nodecount_t& nNodes);

// Multi-Threaded driver for perft()
void perftMT(ChessPosition P, int maxdepth, int depth, PerftInfo* pI);

// Multi-Threaded driver for perftFast()
void perftFastMT(ChessPosition P, int depth, nodecount_t& nNodes);

} //namespace juddperft

#endif // _SEARCH_H
