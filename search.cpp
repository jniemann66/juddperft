#include "search.h"
#include "movegen.h"
#include "engine.h"

#include <thread>
#include <cassert>
#include <vector>
#include <queue>
#include <numeric>
#include <condition_variable>


// Globals
extern Engine TheEngine;
extern ZobristKeySet ZobristKeys;

#ifdef _USE_HASH
extern HashTable <std::atomic<PerftTableEntry> > PerftTable;
extern HashTable <std::atomic<LeafEntry>> LeafTable;
#endif

unsigned __int64 nodecount;
//

__int64 Perft(const ChessPosition P, int maxdepth, int depth, PerftInfo* pI)
{	
	ChessMove MoveList[MOVELIST_SIZE];
	ChessPosition Q = P;
	ChessMove* pM;

#ifdef _USE_HASH
	// Hash not used in this version (Table Entries with struct PerftInfo{} deemed too large ... )
#endif
	
	GenerateMoves(P,MoveList);
	if (depth == maxdepth)
	{
		for (pM = MoveList; pM->NoMoreMoves == 0; pM++)
		{
			pI->nMoves++;
			if (pM->Capture != 0) {
				pI->nCapture++;
			}
				
			if (pM->Castle != 0)
				pI->nCastle++;
			if (pM->CastleLong != 0)
				pI->nCastleLong++;
			if (pM->EnPassantCapture != 0) {
				pI->nEPCapture++;
			}
			if (pM->PromoteBishop ||
				pM->PromoteKnight ||
				pM->PromoteQueen ||
				pM->PromoteRook)
				pI->nPromotion++;
		}
	}
	
	else
	{
		for( pM=MoveList ; pM->NoMoreMoves == 0 ; pM++ )
		{
			Q.PerformMove(*pM).SwitchSides();
			//// ---------------------------------------------------
			//// Do this to trap bugs with incremental Hash updates:
			//
			//ChessPosition Q2;
			//Q2 = Q;
			//Q2.CalculateHash();
			////assert (Q2.HK == Q.HK);
			//if (Q2.HK != Q.HK)
			//{
			//	printf_s("keys Don't match!\n ");
			//	printf_s("before:\n");
			//	DumpChessPosition(P);
			//	printf_s("after:\n");
			//	DumpMove(*pM);
			//	DumpChessPosition(Q);
			//	printf_s("\n\n");
			//	getchar();
			//	Q.CalculateHash(); // Repair the bad hash Key
			//}
			//else
			//{
			//	//	printf_s( "keys Match: " );
			//	//	DumpMove(*pM);
			//	//	getchar();
			//}
			//// ---------------------------------------------------

			Perft(Q, maxdepth, depth + 1, pI);
			Q = P; // unmake move
		}
	}
#ifdef _USE_HASH
//
#endif
	return pI->nMoves;
}

// PerftFast() - stripped-down perft. Doesn't collect stats on Captures/castles/EPs etc.
void PerftFast(const ChessPosition& P, int depth, __int64& nNodes)
{
	ChessMove MoveList[MOVELIST_SIZE];
	ChessPosition Q = P;
	__int64 orig_nNodes = nNodes;

	if (depth == 1) { /* Leaf Node*/

#ifdef _USE_HASH
		// Consult the HashTable:
		HashKey HK = Q.HK^ZobristKeys.zkPerftDepth[depth];
		std::atomic<LeafEntry> *pAtomicRecord = LeafTable.GetAddress(HK);		// get address of atomic record
		LeafEntry RetrievedRecord = pAtomicRecord->load();						// Load a non-atomic copy of the record
		if (RetrievedRecord.Hash == HK) {
			nNodes += RetrievedRecord.count;
			return;
		}
#endif // _USE_HASH

		GenerateMoves(P, MoveList);
		int movecount = MoveList[0].MoveCount;
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
		HashKey HK = Q.HK^ZobristKeys.zkPerftDepth[depth];
		std::atomic<PerftTableEntry> *pAtomicRecord2;
		PerftTableEntry RetrievedRecord2;
		pAtomicRecord2 = PerftTable.GetAddress(HK);								// get address of atomic record
		RetrievedRecord2 = pAtomicRecord2->load();						// Load a non-atomic copy of the record
		if (RetrievedRecord2.Hash == HK) {	
			if (RetrievedRecord2.depth == depth) {
				nNodes += RetrievedRecord2.count;
				return;
			}
		}
#endif
		GenerateMoves(P, MoveList);
		int movecount = MoveList[0].MoveCount;
		for (int i=0; i<movecount; i++) {

			Q = P;								// unmake move
			Q.PerformMove(MoveList[i]).SwitchSides();			// make move
			PerftFast(Q, depth - 1, nNodes);
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

// PerftFast() - stripped-down perft. Doesn't collect stats on Captures/castles/EPs etc.
//void PerftFast(const ChessPosition& P, int depth, __int64& nNodes)
//{
//	ChessMove MoveList[MOVELIST_SIZE];
//	ChessPosition Q = P;
//
//#ifdef _USE_HASH
//	// Consult the HashTable:
//	HashKey HK = Q.HK^ZobristKeys.zkPerftDepth[depth];
//	__int64 orig_nNodes = nNodes;
//
//	std::atomic<PerftTableEntry> *pAtomicRecord = PerftTable.GetAddress(HK);								// get address of atomic record
//	PerftTableEntry RetrievedRecord = pAtomicRecord->load();						// Load a non-atomic copy of the record
//	if (RetrievedRecord.Hash == HK) {
//		if (RetrievedRecord.depth == depth) {
//			nNodes += RetrievedRecord.count;
//			return;
//		}
//	}
//#endif
//
//	GenerateMoves(P, MoveList);
//	int movecount = MoveList[0].MoveCount;
//	if (depth == 1)
//		nNodes += movecount;
//	else {
//
//		for (int i = 0; i<movecount; i++) {
//
//			Q = P;								// unmake move
//			Q.PerformMove(MoveList[i]).SwitchSides();			// make move
//			PerftFast(Q, depth - 1, nNodes);
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

// PerftFastIterative() - Iterative version of perft.
void PerftFastIterative(const ChessPosition& P, int depth, __int64& nNodes)
{
	if (depth==0){
		nNodes = 1i64;
		return;
	}

	if (depth == 1) {
		ChessPosition Q;
		Q = P;
		ChessMove ML[MOVELIST_SIZE];
		GenerateMoves(Q, ML);
		nNodes = ML->MoveCount;
		return;
	}

	// The Following variables are "stack-less" equivalents to local vars in recursive version:
	std::vector<ChessMove[MOVELIST_SIZE]> MoveList(depth + 1);
	std::vector<ChessMove*> pMovePtr(depth + 1,nullptr);
	std::vector<ChessPosition> Q(depth + 1);
	Q[depth] = P;
	
#ifdef _USE_HASH
	std::vector<HashKey> HK(depth+1);
	std::vector<__int64> orig_nNodes(depth+1);
	std::vector<std::atomic<PerftTableEntry>*> pAtomicRecord(depth + 1);
	std::vector<PerftTableEntry> RetrievedRecord(depth+1);
	std::vector<PerftTableEntry> NewRecord(depth + 1);
#endif
	
	int currentdepth = depth;
	
	for (;;) {

#ifdef _USE_HASH
		// Consult the HashTable:
		HK[currentdepth] = Q[currentdepth].HK^ZobristKeys.zkPerftDepth[currentdepth];	
		pAtomicRecord[currentdepth] = PerftTable.GetAddress(HK[currentdepth]);	// get address of atomic record
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
			GenerateMoves(Q[currentdepth], MoveList[currentdepth]);
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
			Q[currentdepth - 1].PerformMove(*pMovePtr[currentdepth]).SwitchSides();
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


// PerftMT() : Multi-threaded version of Perft(), which depends on Perft()

void PerftMT(ChessPosition P, int maxdepth, int depth, PerftInfo* pI)
{
	ChessMove MoveList[MOVELIST_SIZE];
	GenerateMoves(P, MoveList);

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
	// TheEngine.nNumCores is how many cores user wants. 
	// concurrency is what system is capable of.
	// App should only ever dispatch whichever is smallest of {concurrency, nNumCores, MAX_THREADS} threads:

	unsigned int nThreads = min(std::thread::hardware_concurrency(), min(TheEngine.nNumCores, MAX_THREADS));
	std::vector<std::thread> threads;
	std::vector<PerftInfo> PerftPartial(nThreads);
	std::queue<ChessMove> MoveQueue;
	std::mutex q_mutex;
	std::condition_variable cv;
	bool bStart = false;

	// Set up a simple Thread Pool:
	for (unsigned int t = 0; t < min(nThreads, MoveList->MoveCount); t++) {
		threads.emplace_back([&, depth, P] {
			// Thread's job is to sleep until there is something to do ... and then wake up and do it. (Repeat until nothing left to do)

			std::unique_lock<std::mutex> lock(q_mutex);
			cv.wait(lock, [&MoveQueue, bStart] {return (!MoveQueue.empty() || bStart); }); // sleep until something to do (note: lock will be auto-acquired on wake-up)

																						   // upon wake-up:
			PerftInfo T;
			T.nCapture = T.nCastle = T.nCastleLong = T.nEPCapture = T.nMoves = T.nPromotion = 0i64;
			for (;;) // thread's event loop
			{
				if (!MoveQueue.empty()) {
					ChessPosition Q = P;							// Set up position
					ChessMove M = MoveQueue.front();
					MoveQueue.pop();								// remove ChessMove from queue:
					lock.unlock();									// yield usage of queue to other threads while busy processing perft												
					Q.PerformMove(M).SwitchSides();					// make move
					Perft(Q, maxdepth, depth + 1, &T);
					printf_s(".");									// show progress
					lock.lock();									// lock the queue again
				}

				if (MoveQueue.empty())
					break;
			}
			PerftPartial.push_back(T);
		});
	}

	// Put Moves into Queue for Thread pool to process:
	for (unsigned int i = 0; i < MoveList->MoveCount; ++i) {
		MoveQueue.push(MoveList[i]);
	}

	// start the ball rolling:
	bStart = true;
	cv.notify_all();

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

// PerftFastMT() - Multi-threaded PerftFast() driver, Thread Pool version - ensures cpu cores are always doing work. Depends on Perft3()
// 01/03/2016: (working ok)

void PerftFastMT(ChessPosition P, int depth, __int64& nNodes)
{
	nNodes = 0i64;

	if (depth == 0) {
		nNodes = 1;
		return;
	}

	ChessMove MoveList[MOVELIST_SIZE];
	GenerateMoves(P, MoveList);

	if (depth == 1) {
		nNodes = MoveList->MoveCount;
		return;
	}
	
	// determine number of threads. Note:
	// MAX_THREADS is compile-time hard limit. 
	// TheEngine.nNumCores is how many cores user wants. 
	// concurrency is what system is capable of.
	// App should only ever dispatch whichever is smallest of {concurrency, nNumCores, MAX_THREADS} threads:

	unsigned int nThreads = min(std::thread::hardware_concurrency(), min(TheEngine.nNumCores, MAX_THREADS));
	std::vector<std::thread> threads;
	std::vector<__int64> subTotal(nThreads, 0i64);
	std::queue<ChessMove> MoveQueue;
	std::mutex q_mutex;
	std::condition_variable cv;
	bool bStart = false;

	// Set up a simple Thread Pool:
	for (unsigned int t = 0; t < min(nThreads,MoveList->MoveCount); t++) {
		threads.emplace_back([&, depth, P] {
			// Thread's job is to sleep until there is something to do ... and then wake up and do it. (Repeat until nothing left to do)

			std::unique_lock<std::mutex> lock(q_mutex);
			cv.wait(lock, [&MoveQueue, bStart] {return (!MoveQueue.empty() || bStart); }); // sleep until something to do (note: lock will be auto-acquired on wake-up)

			// upon wake-up:
			__int64 s = 0i64; // local accumulator for thread
			for (;;) // thread's event loop
			{
				if (!MoveQueue.empty()) {
					ChessPosition Q = P;							// Set up position
					ChessMove M = MoveQueue.front();
					MoveQueue.pop();								// remove ChessMove from queue:
					lock.unlock();									// yield usage of queue to other threads while busy processing perft												
					Q.PerformMove(M).SwitchSides();					// make move
					PerftFast(Q, depth - 1, s);						// Invoke PerftFast()		
					printf_s(".");									// show progress
					lock.lock();									// lock the queue again
				}

				if (MoveQueue.empty())
					break;
			}
			subTotal.push_back(s);
		});
	}

	// Put Moves into Queue for Thread pool to process:
	for (unsigned int i = 0; i < MoveList->MoveCount; ++i) {
		MoveQueue.push(MoveList[i]);
	}

	// start the ball rolling:
	bStart = true;
	cv.notify_all();

	//Join Threads:
	for (auto & th : threads) {
		th.join();
	}

	// add up total:
	nNodes = std::accumulate(subTotal.begin(), subTotal.end(), 0i64);
}