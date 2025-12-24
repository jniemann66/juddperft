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

#include "juddperft.h"
//#include "chessposition.h"
#include "diagnostics.h"
#include "movegen.h"
//#include "fen.h"
//#include "hash_table.h"
#include "search.h"

#include "tablegroup.h"

#include "engine.h"
#include "winboard.h"

#ifdef _MSC_VER
#include <intrin.h>
#include <Windows.h>
#else
#if defined(__x86_64__)
#include <x86intrin.h>
#elif defined(__aarch64__)
#endif
#endif

#include <iostream>

using namespace juddperft;

int main(int argc, char *argv[], char *envp[])
{
	// std::cout << "sizeof(ChessMove) == " << sizeof(ChessMove) << " sizeof(ChessPosition) == " << sizeof(ChessPosition) << std::endl;

	// size_t nBytesToAllocate = 1ull << 32; // 4GiB


#if !defined(__EMSCRIPTEN__ )
	size_t nBytesToAllocate = 8589934592; // <-- Set how much RAM to use here (more RAM -> faster ... until you start hitting the page file then it gets significantly worse !!!)

	// size_t nBytesToAllocate = 1ull << 34; // 16GiB

	if (!setMemory(nBytesToAllocate)) {
		return EXIT_FAILURE;	// not going to end well ...
	}

	setProcessPriority();


	// testMoveTables();

	// ChessPosition X;
	// readFen(&X, "r3k2r/1bp2pP1/5n2/1P1Q4/1pPq4/5N2/1B1P2p1/R3K2R b KQkq c3 0 1");
	// readFen(&X, "1rb5/4r3/3p1npb/3kp1P1/1P3P1P/5nR1/2Q1BK2/bN4NR w - - 3 61");
	// readFen(&X, "6k1/8/8/Q1Q1Q3/8/Q1q1Q3/8/Q1Q1Q1K1 w - -");
	// printChessPosition(X);
	// ChessMove movelist[MOVELIST_SIZE];
	// X.getLegalMoves(movelist);
	// printMoveList(movelist, StandardAlgebraic, nullptr);

	// ZobristKeySet::findBestSeed(0x4a1b5d94);

	//runTestSuite();


	winBoard(&theEngine);

#ifdef COUNT_MOVEGEN_CPU_CYCLES
	const double avg_cpu_cycles = static_cast<double>(movegen_total_cycles) / movegen_call_count;
	std::cout << "Move generator average cpu cycles=" << avg_cpu_cycles << std::endl;
#endif

#else
	// experimental ...
	ChessPosition P;
	P.setupStartPosition();
	theEngine.currentPosition = P;
	bool keepgoing = true;
//	do {
		keepgoing = waitForInput(&theEngine);
		std::cout << "ready" << std::endl;
//	} while (keepgoing);

#endif

	return EXIT_SUCCESS;
}

namespace juddperft {

	bool setMemory(size_t nTotalBytes)
	{
		std::cout << "\nAttempting to allocate up to " << nTotalBytes << " bytes of RAM ..." << std::endl;
		return TableGroup::setMemory(nTotalBytes);
		return false;
	}

	void setProcessPriority()
	{

#ifdef _MSC_VER
		DWORD dwError;

		if (!SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS)) {
			dwError = GetLastError();
			std::cout << "Failed to set Process priority: " << dwError << std::endl;
		}
#endif

	}

} // namespace juddperft
