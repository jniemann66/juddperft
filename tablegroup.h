#ifndef TABLEGROUP_H
#define TABLEGROUP_H

#include "hash_table.h"

// tablegroup.h : container for owning and managing a collection of various hash tables,
// and controlling how all the memory is divided-up and allocated

#define HT_PERFT_DEPTH_TALLY
#define HT_PERFT_LEAF_TABLE

namespace juddperft {

struct PerftRecord
{
	HashKey hk;

#ifdef HT_PERFT_DEPTH_TALLY
	// 60 bits of nodecount + 4 bits of depth
	union {
		struct {
			// warning: limitations are: max depth = 15, max count = 2^60 = 1,152,921,504,606,846,976
			// which only allows up to perft 12 from start position
			uint64_t depth : 4;
			uint64_t count : 60;
		};
		uint64_t data{0};
	};
#else
	// 64 bits of nodecount
	uint64_t count{0};
#endif

};

// for leaf nodes, we can simply cram the upper 56 bits of the hashkey and 8 bits of movecount into 64 bits
// limitation: cannot handle more than 255 legal moves, if that is even possible (accepted max seems to be 218)
using PerftLeafRecord = uint64_t;


class TableGroup
{
public:
	static bool setMemory(size_t requestedBytes);

	static HashTable <PerftRecord> perftTable;
	static HashTable <PerftLeafRecord> perftLeafTable;
};

} // namespace juddperft

#endif // TABLEGROUP_H
