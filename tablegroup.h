#ifndef TABLEGROUP_H
#define TABLEGROUP_H

#include "hash_table.h"

// tablegroup.h : container for owning and managing a collection of various hash tables,
// and controlling how all the memory is divided-up and allocated

namespace juddperft {

struct PerftRecord
{
	HashKey Hash;

	union {
		struct {
			// warning: limitations are: max depth = 15, max count = 2^60 = 1,152,921,504,606,846,976
			// which only allows up to perft 12 from start position
			uint64_t depth : 4;
			uint64_t count : 60;
		};
		uint64_t data{0};
	};
};

struct PerftLeafRecord
{
	uint64_t k : 56;
	uint64_t count : 8;
};

class TableGroup
{
public:
	static bool setMemory(size_t requestedBytes);

	static HashTable <PerftRecord> perftTable;
	static HashTable <PerftLeafRecord> perftLeafTable;
};

} // namespace juddperft

#endif // TABLEGROUP_H
