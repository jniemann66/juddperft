#include "tablegroup.h"

namespace juddperft {

bool TableGroup::setMemory(size_t requestedBytes)
{
	static constexpr size_t bits = 8 * sizeof (size_t); // hopefully 64

	do {
		size_t m = 0;
		for (size_t i = 0; i < bits; i++) {
			m = (1ull << (bits - i - 1));
			if (m & requestedBytes) {
				break;
			}
		}

#if defined(HT_PERFT_LEAF_TABLE)
		if (perftLeafTable.setSize(m / 2) && perftTable.setSize(m / 8)) { // 4:1 ratio
			return true;
		}
#else
		if (perftTable.setSize(m)) {
			return true;
		}
#endif

		requestedBytes >>= 1;

	} while (requestedBytes > 1024);

	return false;
}

HashTable <PerftRecord> TableGroup::perftTable("Perft table");
HashTable <PerftLeafRecord> TableGroup::perftLeafTable("Perft leaf node table");

}
