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

#include "search.h"
#include "movegen.h"
#include "engine.h"

#include <thread>
#include <cassert>
#include <vector>
#include <queue>
#include <numeric>
#include <condition_variable>
#include <string>
#include <algorithm>
#include <chrono>

namespace juddperft {

uint64_t nodecount;
//

int64_t perft(const ChessPosition P, int maxdepth, int depth, PerftInfo* pI)
{
	ChessMove MoveList[MOVELIST_SIZE];
	ChessPosition Q = P;
	ChessMove* pM;

#ifdef _USE_HASH
	// Hash not used in this version (Table Entries with struct PerftInfo{} deemed too large ... )
#endif

	generateMoves(P, MoveList);
	int movecount = MoveList->MoveCount;

	if (depth == maxdepth)
	{
		pM = MoveList;
		for (int i = 0; i<movecount; i++, pM++)
		{
			pI->nMoves++;
			if (pM->Capture)
				pI->nCapture++;
			if (pM->Castle)
				pI->nCastle++;
			if (pM->CastleLong)
				pI->nCastleLong++;
			if (pM->EnPassantCapture)
				pI->nEPCapture++;
			if (pM->PromoteBishop ||
				pM->PromoteKnight ||
				pM->PromoteQueen ||
				pM->PromoteRook)
				pI->nPromotion++;
		}
	}

	else
	{
		pM = MoveList;
		for (int i = 0; i<movecount; i++, pM++)
		{
			Q.performMove(*pM).switchSides();
			//// ---------------------------------------------------
			//// Do this to trap bugs with incremental Hash updates:
			//
			//ChessPosition Q2;
			//Q2 = Q;
			//Q2.calculateHash();
			////assert (Q2.HK == Q.HK);
			//if (Q2.HK != Q.HK)
			//{
			//	printf("keys Don't match!\n ");
			//	printf("before:\n");
			//	dumpChessPosition(P);
			//	printf("after:\n");
			//	dumpMove(*pM);
			//	dumpChessPosition(Q);
			//	printf("\n\n");
			//	getchar();
			//	Q.calculateHash(); // Repair the bad hash Key
			//}
			//else
			//{
			//	//	printf( "keys Match: " );
			//	//	dumpMove(*pM);
			//	//	getchar();
			//}
			//// ---------------------------------------------------

			perft(Q, maxdepth, depth + 1, pI);
			Q = P; // unmake move
		}
	}
#ifdef _USE_HASH
//
#endif
	return pI->nMoves;
}

// perftFast() - stripped-down perft. Doesn't collect stats on Captures/castles/EPs etc.
void perftFast(const ChessPosition& P, int depth, int64_t& nNodes)
{
	ChessMove MoveList[MOVELIST_SIZE];
	ChessPosition Q = P;
	int64_t orig_nNodes = nNodes;

	if (depth == 1) { /* Leaf Node*/

#ifdef _USE_HASH
		// Consult the HashTable:
		HashKey HK = Q.HK^zobristKeys.zkPerftDepth[depth];
		std::atomic<LeafEntry> *pAtomicRecord = leafTable.getAddress(HK);		// get address of atomic record
		LeafEntry RetrievedRecord = pAtomicRecord->load();						// Load a non-atomic copy of the record
		if (RetrievedRecord.Hash == HK) {
			nNodes += RetrievedRecord.count;
			return;
		}
#endif // _USE_HASH

		generateMoves(P, MoveList);
		int movecount = MoveList->MoveCount;
		nNodes += movecount;

#ifdef _USE_HASH
		LeafEntry NewRecord;
		NewRecord.Hash = HK;
		NewRecord.count = movecount;
		do {
			// if (RetrievedRecord has changed) {} // do something (if we care)
		} while (!pAtomicRecord->compare_exchange_weak(RetrievedRecord, NewRecord)); // loop until successfully written;
#endif
	}

	else { /* Branch Node */

#ifdef _USE_HASH
		// Consult the HashTable:
		HashKey HK = Q.HK^zobristKeys.zkPerftDepth[depth];
		std::atomic<PerftTableEntry> *pAtomicRecord2;
		PerftTableEntry RetrievedRecord2;
		pAtomicRecord2 = perftTable.getAddress(HK);								// get address of atomic record
		RetrievedRecord2 = pAtomicRecord2->load();						// Load a non-atomic copy of the record
		if (RetrievedRecord2.Hash == HK) {
			if (RetrievedRecord2.depth == depth) {
				nNodes += RetrievedRecord2.count;
				return;
			}
		}
#endif
		generateMoves(P, MoveList);
		int movecount = MoveList->MoveCount;
		for (int i=0; i<movecount; i++) {

			Q = P;								// unmake move
			Q.performMove(MoveList[i]).switchSides();			// make move
			perftFast(Q, depth - 1, nNodes);
		}

#ifdef _USE_HASH
		PerftTableEntry NewRecord2;
		NewRecord2.Hash = HK;
		NewRecord2.depth = depth;
		NewRecord2.count = nNodes - orig_nNodes; // Record RELATIVE increase in nodes
		do {
			// if (RetrievedRecord has changed) {} // do something (if we care)
		} while (!pAtomicRecord2->compare_exchange_weak(RetrievedRecord2, NewRecord2)); // loop until successfully written;
#endif

	}
}

// perftFast() - stripped-down perft. Doesn't collect stats on Captures/castles/EPs etc.
//void perftFast(const ChessPosition& P, int depth, int64_t& nNodes)
//{
//	ChessMove MoveList[MOVELIST_SIZE];
//	ChessPosition Q = P;
//
//#ifdef _USE_HASH
//	// Consult the HashTable:
//	HashKey HK = Q.HK^zobristKeys.zkPerftDepth[depth];
//	int64_t orig_nNodes = nNodes;
//
//	std::atomic<PerftTableEntry> *pAtomicRecord = perftTable.getAddress(HK);								// get address of atomic record
//	PerftTableEntry RetrievedRecord = pAtomicRecord->load();						// Load a non-atomic copy of the record
//	if (RetrievedRecord.Hash == HK) {
//		if (RetrievedRecord.depth == depth) {
//			nNodes += RetrievedRecord.count;
//			return;
//		}
//	}
//#endif
//
//	generateMoves(P, MoveList);
//	int movecount = MoveList[0].MoveCount;
//	if (depth == 1)
//		nNodes += movecount;
//	else {
//
//		for (int i = 0; i<movecount; i++) {
//
//			Q = P;								// unmake move
//			Q.performMove(MoveList[i]).switchSides();			// make move
//			perftFast(Q, depth - 1, nNodes);
//		}
//	}
//
//#ifdef _USE_HASH
//
//	PerftTableEntry NewRecord;
//	NewRecord.Hash = HK;
//	NewRecord.depth = depth;
//	NewRecord.count = nNodes - orig_nNodes; // Record RELATIVE increase in nodes
//
//	do {
//		// if (RetrievedRecord has changed) {} // do something (if we care)
//	} while (!pAtomicRecord->compare_exchange_weak(RetrievedRecord, NewRecord)); // loop until successfully written;
//
//#endif
//}

// perftFastIterative() - Iterative version of perft.
void perftFastIterative(const ChessPosition& P, int depth, int64_t& nNodes)
{
	if (depth==0){
		nNodes = 1LL;
		return;
	}

	if (depth == 1) {
		ChessPosition Q;
		Q = P;
		ChessMove ML[MOVELIST_SIZE];
		generateMoves(Q, ML);
		nNodes = ML->MoveCount;
		return;
	}

	// The Following variables are "stack-less" equivalents to local vars in recursive version:
	std::vector<ChessMove[MOVELIST_SIZE]> MoveList(depth + 1);
	std::vector<ChessMove*> pMovePtr(depth + 1, nullptr);
	std::vector<ChessPosition> Q(depth + 1);
	Q[depth] = P;
	std::vector<int64_t> orig_nNodes(depth+1);

#ifdef _USE_HASH
	std::vector<HashKey> HK(depth+1);
	std::vector<std::atomic<PerftTableEntry>*> pAtomicRecord(depth + 1);
	std::vector<PerftTableEntry> RetrievedRecord(depth+1);
	std::vector<PerftTableEntry> NewRecord(depth + 1);
#endif

	int currentdepth = depth;

	for (;;) {

#ifdef _USE_HASH
		// Consult the HashTable:
		HK[currentdepth] = Q[currentdepth].HK^zobristKeys.zkPerftDepth[currentdepth];
		pAtomicRecord[currentdepth] = perftTable.getAddress(HK[currentdepth]);	// get address of atomic record
		RetrievedRecord[currentdepth] = pAtomicRecord[currentdepth]->load();	// Load a non-atomic copy of the record

		if (RetrievedRecord[currentdepth].Hash == HK[currentdepth]) {
			if (RetrievedRecord[currentdepth].depth == currentdepth) {
				nNodes += RetrievedRecord[currentdepth].count;
				if (currentdepth == depth)
					break; // finished
				else {
					++currentdepth; // step out
					continue;
				}
			}
		}
#endif
		// conditionally generate move list:
		if (pMovePtr[currentdepth] == nullptr) {
			orig_nNodes[currentdepth] = nNodes;
			generateMoves(Q[currentdepth], MoveList[currentdepth]);
			pMovePtr[currentdepth] = MoveList[currentdepth];
		}

		if (currentdepth == 1) {
			// Leaf Node
			nNodes += MoveList[currentdepth]->MoveCount;	// Accumulate

#ifdef _USE_HASH
			// writehash
			NewRecord[currentdepth].Hash = HK[currentdepth];
			NewRecord[currentdepth].depth = currentdepth;
			NewRecord[currentdepth].count = MoveList[currentdepth]->MoveCount;//nNodes - orig_nNodes[currentdepth]; // Record RELATIVE increase in nodes
			do {} while (!pAtomicRecord[currentdepth]->compare_exchange_weak(RetrievedRecord[currentdepth], NewRecord[currentdepth])); // loop until successfully written;
#endif
			++currentdepth; // step out
			continue;
		}

		if (!pMovePtr[currentdepth]->NoMoreMoves) {
			Q[currentdepth - 1] = Q[currentdepth];
			Q[currentdepth - 1].performMove(*pMovePtr[currentdepth]).switchSides();
			++pMovePtr[currentdepth];
			pMovePtr[currentdepth - 1] = nullptr;
			--currentdepth; // step into
			continue;
		}

#ifdef _USE_HASH

		//	Writehash - (2016-02-26 broken in multithreading)
			//NewRecord[currentdepth].Hash = HK[currentdepth];
			//NewRecord[currentdepth].depth = currentdepth;
			//NewRecord[currentdepth].count = nNodes-orig_nNodes[currentdepth]; // Record RELATIVE increase in nodes
			//do {
			//
			//} while (!pAtomicRecord[currentdepth]->compare_exchange_weak(RetrievedRecord[currentdepth], NewRecord[currentdepth])); // loop until successfully written;
#endif
			if (currentdepth == depth)
				break; // finished
			else
				++currentdepth; // step out
	}
}

// perftMT() : Multi-threaded version of perft(), which depends on perft()

void perftMT(ChessPosition P, int maxdepth, int depth, PerftInfo* pI)
{
	ChessMove MoveList[MOVELIST_SIZE];
	generateMoves(P, MoveList);

	if (depth == maxdepth) {
		for (ChessMove* pM = MoveList; pM->NoMoreMoves == 0; pM++)
		{
			pI->nMoves++;
			if (pM->Capture)
				pI->nCapture++;
			if (pM->Castle)
				pI->nCastle++;
			if (pM->CastleLong)
				pI->nCastleLong++;
			if (pM->EnPassantCapture)
				pI->nEPCapture++;
			if (pM->PromoteBishop ||
				pM->PromoteKnight ||
				pM->PromoteQueen ||
				pM->PromoteRook)
				pI->nPromotion++;
		}
		return;
	}

	// determine number of threads. Note:
	// MAX_THREADS is compile-time hard limit.
	// theEngine.nNumCores is how many cores user wants.
	// concurrency is what system is capable of.
	// App should only ever dispatch whichever is smallest of {concurrency, nNumCores, MAX_THREADS} threads:

	unsigned int nThreads = std::min(std::thread::hardware_concurrency(), std::min(theEngine.nNumCores, static_cast<unsigned int>(MAX_THREADS)));
	std::vector<std::thread> threads;
	std::vector<PerftInfo> PerftPartial(nThreads);
	std::queue<ChessMove> MoveQueue;
	std::mutex q_mutex;
	std::condition_variable cv;
	bool ready = false;

	// Set up a simple Thread Pool:
	for (unsigned int t = 0; t < std::min(nThreads, MoveList->MoveCount); t++) {
		threads.emplace_back([&, depth, P] {

			// Thread is to sleep until there is something to do ... and then wake up and do it.
			std::unique_lock<std::mutex> lock(q_mutex);
			std::chrono::milliseconds startDelay(500);
			cv.wait_for (lock, startDelay, [ready] { return ready; }); // sleep until something to do (note: lock will be auto-acquired on wake-up)

			// upon wake-up (lock acquired):
			while (!MoveQueue.empty()) {
				PerftInfo T;
				T.nCapture = T.nCastle = T.nCastleLong = T.nEPCapture = T.nMoves = T.nPromotion = ZERO_64;
				ChessPosition Q = P;							// Set up position
				ChessMove M = MoveQueue.front();				// Grab Move
				MoveQueue.pop();								// remove Move from queue:
				lock.unlock();									// yield usage of queue to other threads while busy processing perft
				Q.performMove(M).switchSides();					// make move
				perft(Q, maxdepth, depth + 1, &T);				// Invoke perft()
				std::cout << ".";								// show progress
				lock.lock();									// lock the queue again	for next iteration
				PerftPartial.push_back(T);						// record subtotals
			}
		});
	}

	// Put Moves into Queue for Thread pool to process:
	for (unsigned int i = 0; i < MoveList->MoveCount; ++i) {
		MoveQueue.push(MoveList[i]);
	}

	// start the ball rolling:
	ready = true;
	//cv.notify_all();

	//Join Threads:
	for (auto & th : threads) {
		th.join();
	}

	// add up total:
	std::for_each(PerftPartial.begin(), PerftPartial.end(), [pI](PerftInfo& t) {
		pI->nCapture += t.nCapture;
		pI->nCastle += t.nCastle;
		pI->nCastleLong += t.nCastleLong;
		pI->nEPCapture += t.nEPCapture;
		pI->nMoves += t.nMoves;
		pI->nPromotion += t.nPromotion;
	});
}

// perftFastMT() - Multi-threaded perftFast() driver, Thread Pool version - ensures cpu cores are always doing work. Depends on Perft3()
// 01/03/2016: (working ok)

void perftFastMT(ChessPosition P, int depth, int64_t& nNodes)
{
	nNodes = ZERO_64;

	if (depth == 0) {
		nNodes = 1;
		return;
	}

	ChessMove MoveList[MOVELIST_SIZE];
	generateMoves(P, MoveList);

	if (depth == 1) {
		nNodes = MoveList->MoveCount;
		return;
	}

	// determine number of threads. Note:
	// MAX_THREADS is compile-time hard limit.
	// theEngine.nNumCores is how many cores user wants.
	// concurrency is what system is capable of.
	// App should only ever dispatch whichever is smallest of {concurrency, nNumCores, MAX_THREADS} threads:

	unsigned int nThreads = std::min(std::thread::hardware_concurrency(), std::min(theEngine.nNumCores, static_cast<unsigned int>(MAX_THREADS)));
	std::vector<std::thread> threads;
	std::vector<int64_t> subTotal(nThreads, ZERO_64);
	std::queue<ChessMove> MoveQueue;
	std::mutex q_mutex;
	std::condition_variable cv;
	bool ready = false;

	// Set up a simple Thread Pool:
	for (unsigned int t = 0; t < std::min(nThreads, MoveList->MoveCount); t++) {
		threads.emplace_back([&, depth, P, t] {

			// Thread is to sleep until there is something to do ... and then wake up and do it.
			std::unique_lock<std::mutex> lock(q_mutex);
			std::chrono::milliseconds startDelay(500);
			cv.wait_for (lock, startDelay, [ready] { return ready; }); // sleep until something to do (note: lock will be auto-acquired on wake-up)

			// upon wake-up (lock acquired):
			while (!MoveQueue.empty()) {
				int64_t s = ZERO_64; 							// local accumulator for thread
				ChessPosition Q = P;							// Set up position
				ChessMove M = MoveQueue.front();				// Grab Move
				MoveQueue.pop();								// remove Move from queue:
				lock.unlock();									// yield usage of queue to other threads while busy processing perft
				Q.performMove(M).switchSides();					// make move
				perftFast(Q, depth - 1, s);						// Invoke perftFast()
				std::cout << ".";								// show progress
				lock.lock();									// lock the queue again	for next iteration
				subTotal.push_back(s);							// record subtotal
			}
		});
	}

	// Put Moves into Queue for Thread pool to process:
	for (unsigned int i = 0; i < MoveList->MoveCount; ++i) {
		MoveQueue.push(MoveList[i]);
	}

	// start the ball rolling:
	ready = true;
	//cv.notify_all();

	//Join Threads:
	for (auto & th : threads) {
		th.join();
	}

	// add up total:
	nNodes = std::accumulate(subTotal.begin(), subTotal.end(), ZERO_64);
}

} // namespace juddperft
