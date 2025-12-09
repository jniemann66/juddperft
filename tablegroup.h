#ifndef TABLEGROUP_H
#define TABLEGROUP_H

#include "hash_table.h"

// tablegroup.h : container for owning and managing a collection of various hash tables,
// and controlling how all the memory is divided-up and allocated

#define HT_PERFT_DEPTH_TALLY
// #define HT_PERFT_LEAF_TABLE // performs worse - IDK why - cache effects ? atomic shenanigans ?

namespace juddperft {

struct PerftRecord
{
	HashKey Hash;

#ifdef HT_PERFT_DEPTH_TALLY
	// 60 bits of count + 4 bits of depth
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
	// 64 bits of count
	uint64_t count{0};
#endif

};


struct PerftLeafRecord
{
	uint64_t k : 56;
	uint64_t count : 8;
};

using PerftRecordType = PerftRecord;

template <typename T, typename = void>
struct has_depth : std::false_type {};

template <typename T>
struct has_depth<T, std::void_t<decltype(std::declval<T>().depth)> > : std::true_type {};

class TableGroup
{
public:
	static bool setMemory(size_t requestedBytes);

	static HashTable <PerftRecord> perftTable;
	static HashTable <PerftLeafRecord> perftLeafTable;
};

} // namespace juddperft

#endif // TABLEGROUP_H
