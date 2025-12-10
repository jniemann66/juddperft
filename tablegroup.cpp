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
