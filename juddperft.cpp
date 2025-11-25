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
#include "movegen.h"
#include "fen.h"
#include "search.h"
#include "winboard.h"
#include "diagnostics.h"
#include "hashtable.h"

#ifdef _MSC_VER
#include <intrin.h>
#include <Windows.h>
#else
#include <x86intrin.h>
#endif

#include <cinttypes>
#include <iostream>
#include <atomic>
#include <bitset>

using namespace juddperft;

int main(int argc, char *argv[], char *envp[])
{
    // std::cout << "sizeof(ChessMove) == " << sizeof(ChessMove) << " sizeof(ChessPosition) == " << sizeof(ChessPosition) << std::endl;

#ifdef _USE_HASH
    uint64_t nBytesToAllocate = 8589934593; // <-- Set how much RAM to use here (more RAM -> faster !!!)

    while (!setMemory(nBytesToAllocate)) {
        nBytesToAllocate >>= 1;	// Progressively halve until acceptable size found
        if (nBytesToAllocate < MINIMAL_HASHTABLE_SIZE)
            return EXIT_FAILURE;	// not going to end well ...
    }
#endif
    setProcessPriority();

    ChessPosition X;
    // readFen(&X, "r3k2r/1bp2pP1/5n2/1P1Q4/1pPq4/5N2/1B1P2p1/R3K2R b KQkq c3 0 1");
    // readFen(&X, "1rb5/4r3/3p1npb/3kp1P1/1P3P1P/5nR1/2Q1BK2/bN4NR w - - 3 61");
    // dumpChessPosition(X);
    // ChessMove movelist[MOVELIST_SIZE];
    // X.getLegalMoves(movelist);
    // dumpMoveList(movelist, StandardAlgebraic, nullptr);

   //  runTestSuite();

    winBoard(&theEngine);

#ifdef COUNT_MOVEGEN_CPU_CYCLES
    const double avg_cpu_cycles = static_cast<double>(movegen_total_cycles) / movegen_call_count;
    std::cout << "Move generator average cpu cycles=" << avg_cpu_cycles << std::endl;

#endif

    return EXIT_SUCCESS;
}

namespace juddperft {

    bool setMemory(uint64_t nTotalBytes) {

#ifdef _USE_HASH

        std::cout << "\nAttempting to allocate up to " << nTotalBytes << " bytes of RAM ..." << std::endl;

        return perftTable.setSize(nTotalBytes);
#else
        return false;
#endif
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
