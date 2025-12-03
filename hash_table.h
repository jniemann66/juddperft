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

#ifndef _HASH_TABLE_H
#define _HASH_TABLE_H

#include "utils.h"

#include <cstring>

#include <atomic>
#include <iostream>
#include <optional>
#include <string>

namespace juddperft {

typedef uint64_t HashKey;

// generic Hashtable template:
template<class T>
class HashTable
{
public:
	HashTable(const std::string& name = std::string("Hash Table"));
	~HashTable();

	// getters
	std::atomic<T>* getAddress(const HashKey& SearchHK) const;
	std::string getName() const;
	size_t getSize() const;			// return currently-allocated size in bytes
	size_t getRequestedSize() const;	// return what was originally requested in bytes
	size_t getNumRecords() const;

	// setters
	bool setSize(size_t nBytes);
	void setName(const std::string &newName);
	bool deAllocate();
	void clear();

	void setQuiet(bool newQuiet);

private:
	std::atomic<T>* m_pTable{nullptr};
	size_t m_nEntries;
	size_t m_nIndexMask;
	size_t m_nRequestedSize;
	std::string m_Name;
	bool quiet{false};
};

template<class T>
inline HashTable<T>::HashTable(const std::string& name)
	: m_Name(name)
{
	m_pTable = nullptr;
	m_nEntries = 0;
	m_nIndexMask = 0;
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
inline bool HashTable<T>::setSize(size_t nBytes)
{
	m_nRequestedSize = nBytes;

	size_t nNewNumEntries = 1ull;
	// Make nNewSize a power of 2:
	while (nNewNumEntries * sizeof (std::atomic<T>) <= nBytes) {
		nNewNumEntries <<= 1;
	}

	nNewNumEntries >>= 1;
	m_nEntries = nNewNumEntries;

	// create a mask with all 1's (2^n - 1) for address calculation:
	m_nIndexMask = m_nEntries - 1;
	deAllocate();

	m_pTable = new (std::nothrow) std::atomic<T>[m_nEntries];

	if (m_pTable == nullptr) {
		std::cout << "Failed to allocate " << nBytes << " bytes for " << m_Name << std::endl;
		return false;
	} else {
		const size_t bytes = m_nEntries * sizeof(T);
		if (!quiet) {
			std::cout << "Allocated " << bytes << " bytes ("
					  << Utils::memorySizeWithBinaryPrefix(bytes) << ") for "
					  << m_Name << " (" << m_nEntries << " entries at " << sizeof(T) << " bytes each)" << std::endl;
		}

		// std::cout << "Is lock free ? " << m_pTable->is_lock_free() << std::endl;
		// note: on x86-64, gcc has a tendency to report this as false,
		// even when the expected cmpxchg16b instruction is actually being used (inside calls to libatomic)

		clear();
		return true;
	}
}

template<class T>
inline bool HashTable<T>::deAllocate()
{
	if (m_pTable) {
		if (!quiet) {
			std::cout << "deallocating " << m_Name << std::endl;
		}
		delete[] m_pTable;
		m_pTable = nullptr;
		return true;
	}

	return false;
}

template<class T>
inline std::atomic<T> *HashTable<T>::getAddress(const HashKey & SearchHK) const
{
	return m_pTable + (SearchHK & m_nIndexMask);
}

template<class T>
inline size_t HashTable<T>::getSize() const
{
	return m_nEntries * sizeof(std::atomic<T>);
}

template<class T>
inline size_t HashTable<T>::getNumRecords() const
{
	return m_nEntries;
}

template<class T>
inline void HashTable<T>::clear()
{
	static constexpr bool use_memset_ftw = true;

	if constexpr (use_memset_ftw) {
		// todo: find the "proper" way to clear these ...
		if (m_pTable != nullptr) {
			std::memset(m_pTable, 0, sizeof(std::atomic<T>) * m_nEntries);
		}
	} else {
		// seriously, this sucks ... takes more time to initialize than it does to do a perft(7) haha ...
		T t = T();
		for (size_t i = 0; i < m_nEntries; i++) {
			std::atomic_init<T>(m_pTable + i, t);
		}
	}
}

template<class T>
inline std::string HashTable<T>::getName() const
{
	return m_Name;
}

template<class T>
inline void HashTable<T>::setName(const std::string &newName)
{
	m_Name = newName;
}

template<class T>
void HashTable<T>::setQuiet(bool newQuiet)
{
	quiet = newQuiet;
}

} // namespace juddperft

#endif // _HASH_TABLE_H

