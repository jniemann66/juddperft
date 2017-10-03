/*

MIT License

Copyright(c) 2016-2017 Judd Niemann

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

#ifndef _HASHTABLE_H
#define _HASHTABLE_H 1

#include "movegen.h"

#include <iostream>
#include <atomic>
#include <string>
#include <cstring> // for std::memset

namespace juddperft {

// HTs are "shrunk" to this size when not in use:
#define MINIMAL_HASHTABLE_SIZE 1000000
#define _SQUEEZE_PERFT_COUNT_60BITS 1 // squeeze depth and count into single 64-bit integer (4:60 bits respectively) to make hash entries smaller.

typedef uint64_t HashKey;
typedef uint64_t ZobristKey;

class ZobristKeySet
{
public:
	ZobristKeySet();
	ZobristKey zkPieceOnSquare[16][64];
	ZobristKey zkBlackToMove;
	ZobristKey zkWhiteCanCastle;
	ZobristKey zkWhiteCanCastleLong;
	ZobristKey zkBlackCanCastle;
	ZobristKey zkBlackCanCastleLong;
	ZobristKey zkPerftDepth[24];
	
	// pre-fabricated combinations of keys for castling:
	ZobristKey zkDoBlackCastle;
	ZobristKey zkDoBlackCastleLong;
	ZobristKey zkDoWhiteCastle;
	ZobristKey zkDoWhiteCastleLong;
	//

	bool Generate();
};

// generic Hashtable template:
template<class T> class HashTable
{
public:
	HashTable(const std::string& name = std::string("Hash Table"));
	~HashTable();
	bool SetSize(uint64_t nBytes);
	bool DeAllocate();
	T* GetAddress(const HashKey& SearchHK) const;
	uint64_t GetSize() const;			// return currently-allocated size in bytes
	uint64_t GetRequestedSize() const;	// return what was originally requested in bytes
	uint64_t GetNumEntries() const;
	double GetLoadFactor() const;
	void Clear();

private:
	T* m_pTable;
	uint64_t m_nEntries;
	uint64_t m_nIndexMask;
	uint64_t m_nRequestedSize;
	uint64_t m_nWrites;
	uint64_t m_nCollisions;
	std::string m_Name;
};

template<class T>
inline HashTable<T>::HashTable(const std::string& name) : m_Name(name)
{
	m_pTable = nullptr;
	m_nCollisions = 0;
	m_nEntries = 0;
	m_nIndexMask = 0;
	m_nWrites = 0;
}

template<class T>
inline HashTable<T>::~HashTable()
{
	if (m_pTable != nullptr) {
		std::cout << "deallocating " << m_Name << std::endl;
		delete[] m_pTable;
		m_pTable = nullptr;
	}
}

template<class T>
inline bool HashTable<T>::SetSize(uint64_t nBytes)
{
	m_nRequestedSize = nBytes;

	uint64_t nNewNumEntries = 1LL;
	// Make nNewSize a power of 2:
	while (nNewNumEntries*sizeof(T) < nBytes) {
		nNewNumEntries <<= 1;
	}

	nNewNumEntries >>= 1;

	if (nNewNumEntries == m_nEntries) {
		// No change in size
		return false;
	}

	m_nEntries = nNewNumEntries;
	// create a mask with all 1's (2^n - 1) for address calculation:
	HashTable::m_nIndexMask = m_nEntries - 1;
	HashTable::DeAllocate();

	m_pTable = new (std::nothrow) T[m_nEntries];

	if (m_pTable == nullptr) {
		std::cout << "Failed to allocate " << nBytes << " bytes for " << m_Name << std::endl;
		return false;
	}
	else {
		std::cout << "Allocated " << m_nEntries * sizeof(T) << " bytes for " << m_Name << " (" << m_nEntries << " entries at " << sizeof(T) << " bytes each)" << std::endl;
		m_nCollisions = 0;
		m_nWrites = 0;
		HashTable<T>::Clear();
		return true;
	}
}

template<class T>
inline bool HashTable<T>::DeAllocate()
{
	if (HashTable::m_pTable != nullptr) // to-do: do we need to do all this nullptr crap ?
	{
		std::cout << "deallocating " << m_Name << std::endl;
		delete[] m_pTable;
		m_pTable = nullptr;
		return true;
	}
	else {
		return false;
	}
}

template<class T>
inline T * HashTable<T>::GetAddress(const HashKey & SearchHK) const
{
	return m_pTable + (SearchHK & m_nIndexMask);
}

template<class T>
inline uint64_t HashTable<T>::GetSize() const
{
	return m_nEntries*sizeof(T);
}

template<class T>
inline uint64_t HashTable<T>::GetRequestedSize() const
{
	return m_nRequestedSize;
}

template<class T>
inline uint64_t HashTable<T>::GetNumEntries() const
{
	return m_nEntries;
}

template<class T>
inline double HashTable<T>::GetLoadFactor() const
{
	return
		static_cast<double>((1000 * m_nWrites) / m_nEntries) / 1000;
}

template<class T>
inline void HashTable<T>::Clear()
{
	if (m_pTable != nullptr)
		std::memset(m_pTable, 0, sizeof(T)*m_nEntries);
}

//

struct PerftTableEntry
{
	HashKey Hash;

#ifndef _SQUEEZE_PERFT_COUNT_60BITS
	int depth;
	unsigned long long count;
#else
	// more compact (squeeze count and depth into 64 bits):

	union {
		struct {
			unsigned long long count : 60;
			unsigned long long depth : 4;
	
	// warning: limitations are: max depth=15, max count = 2^60 = 1,152,921,504,606,846,976 
	// which only allows up to perft 12 from start position

		};
		unsigned long long Data;
	}; 
#endif 
	// Note: std::atomic<> version of this appears to add 8 bytes on msvc
};

struct LeafEntry
{
	HashKey Hash;
	unsigned char count;
};

#ifdef _USE_HASH
// Global instances:
extern ZobristKeySet ZobristKeys;
extern HashTable <std::atomic<PerftTableEntry>> perftTable;
extern HashTable <std::atomic<LeafEntry>> leafTable;
#endif

} // namespace juddperft
#endif // _HASHTABLE_H

