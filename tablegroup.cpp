/*

MIT License

Copyright(c) 2016-2026 Judd Niemann

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

#include "tablegroup.h"

namespace juddperft {

bool TableGroup::setMemory(size_t requestedBytes)
{
	static constexpr size_t bits = 8 * sizeof (size_t); // hopefully 64

	size_t m = 0;
	for (size_t i = 0; i < bits; i++) {
		m = (1ull << (bits - i - 1)); // 1 << 63 .. 1 << 0

#if defined(HT_PERFT_LEAF_TABLE)
		size_t t = m + m / 4; // 4:1 ratio
		if (t <= requestedBytes) {
			if (perftLeafTable.setSize(m) && perftTable.setSize(m / 4)) {
				return true;
			}
		}
#else
		if (m <= requestedBytes) {
			if (perftTable.setSize(m)) {
				return true;
			}
		}
#endif

		if (m <= 1024) {
			break;
		}
	}

	return false;
}

HashTable <PerftRecord> TableGroup::perftTable("Perft table");
HashTable <PerftLeafRecord> TableGroup::perftLeafTable("Perft leaf node table");

}
