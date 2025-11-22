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

#include "Juddperft.h"
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

using namespace juddperft;

int main(int argc, char *argv[], char *envp[])
{

#ifdef _USE_HASH
	uint64_t nBytesToAllocate = 1000000000; // <-- Set how much RAM to use here (more RAM -> faster !!!)

	while (!setMemory(nBytesToAllocate)) {
		nBytesToAllocate >>= 1;	// Progressively halve until acceptable size found
		if (nBytesToAllocate < MINIMAL_HASHTABLE_SIZE)
			return EXIT_FAILURE;	// not going to end well ...
	}
#endif
	setProcessPriority();

	// runTestSuite();

	winBoard(&theEngine);
	return EXIT_SUCCESS;
}

namespace juddperft {

	bool setMemory(uint64_t nTotalBytes) {

#ifdef _USE_HASH

		std::cout << "\nAttempting to allocate up to " << nTotalBytes << " bytes of RAM ..." << std::endl;

		// constraint: Leaf Table should have 3 times as many Entries as perftTable (ie 3:1 ratio)

		uint64_t bytesForPerftTable = (nTotalBytes * sizeof(std::atomic<PerftTableEntry>)) /
			(sizeof(std::atomic<PerftTableEntry>) + 3 * sizeof(std::atomic<LeafEntry>));

		uint64_t bytesForLeafTable = (nTotalBytes * 3 * sizeof(std::atomic<LeafEntry>)) /
			(sizeof(std::atomic<PerftTableEntry>) + 3 * sizeof(std::atomic<LeafEntry>));

		return	(perftTable.setSize(bytesForPerftTable) && leafTable.setSize(bytesForLeafTable));
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
