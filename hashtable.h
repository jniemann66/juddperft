#ifndef _HASHTABLE_H
#define _HASHTABLE_H 1

#include "movegen.h"
#include <atomic>
#include <string>

// HTs are "shrunk" to this size when not in use:
#define MINIMAL_HASHTABLE_SIZE 1000000
#define _SQUEEZE_PERFT_COUNT_60BITS 1 // squeeze depth and count into single 64-bit integer (4:60 bits respectively) to make hash entries smaller.

typedef unsigned __int64 HashKey;
typedef unsigned __int64 ZobristKey;

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
	HashTable(char* pName = "Hash Table");
	~HashTable();
	bool SetSize(unsigned __int64 nBytes);
	bool DeAllocate();
	T* GetAddress(const HashKey& SearchHK) const;
	unsigned __int64 GetSize() const;			// return currently-allocated size in bytes
	unsigned __int64 GetRequestedSize() const;	// return what was originally requested in bytes
	unsigned __int64 GetNumEntries() const;
	double GetLoadFactor() const;
	void Clear();
private:
	T* m_pTable;
	unsigned __int64 m_nEntries;
	unsigned __int64 m_nIndexMask;
	unsigned __int64 m_nRequestedSize;
	unsigned __int64 m_nWrites;
	unsigned __int64 m_nCollisions;
	std::string m_Name;
};

template<class T>
inline HashTable<T>::HashTable(char* pName) : m_Name(pName)
{
	m_pTable = nullptr;
	m_nCollisions = 0i64;
	m_nEntries = 0i64;
	m_nIndexMask = 0i64;
	m_nWrites = 0i64;
}

template<class T>
inline HashTable<T>::~HashTable()
{
	if (m_pTable != nullptr) {
		printf_s("deallocating %s\n",m_Name.c_str());
		delete[] m_pTable;
		m_pTable = nullptr;
	}
}

template<class T>
inline bool HashTable<T>::SetSize(unsigned __int64 nBytes)
{
	m_nRequestedSize = nBytes;

	unsigned __int64 nNewNumEntries = 1i64;
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
		printf_s("Failed to allocate %I64d bytes for %s !\n", nBytes,m_Name.c_str());
		return false;
	}
	else {
		printf_s("Allocated %I64d bytes for %s\n(%I64d Entries @ %zd bytes each)\n", m_nEntries*sizeof(T), m_Name.c_str(),m_nEntries, sizeof(T));
		m_nCollisions = 0i64;
		m_nWrites = 0i64;
		return true;
	}
}

template<class T>
inline bool HashTable<T>::DeAllocate()
{
	if (HashTable::m_pTable != nullptr) // to-do: do we need to do all this nullptr crap ?
	{
		printf_s("deallocating %s\n",m_Name.c_str());
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
inline unsigned __int64 HashTable<T>::GetSize() const
{
	return m_nEntries*sizeof(T);
}

template<class T>
inline unsigned __int64 HashTable<T>::GetRequestedSize() const
{
	return m_nRequestedSize;
}

template<class T>
inline unsigned __int64 HashTable<T>::GetNumEntries() const
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
	// Note: std::atomic<> version of this appears to add 8 bytes
};

struct LeafEntry
{
	HashKey Hash;
	unsigned char count;
};

//struct LeafEntry
//{
//	HashKey Hash;
//	bool hasDepth[3];
//	unsigned __int32 count[3];
//};


#endif // _HASHTABLE_H

