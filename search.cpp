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

nodecount_t perft(const ChessPosition P, int maxdepth, int depth, PerftInfo* pI)
{
	ChessMove MoveList[MOVELIST_SIZE];
	ChessPosition Q = P;
	ChessMove* pM;

	generateMoves(P, MoveList);
	int movecount = MoveList->MoveCount;

	if (depth == maxdepth)
	{
		pM = MoveList;
        for (int i = 0; i < movecount; i++, pM++)
		{
			pI->nMoves++;
			if (pM->Capture) {
				pI->nCapture++;
			}

			if (pM->Castle) {
				pI->nCastle++;
			}

			if (pM->CastleLong) {
				pI->nCastleLong++;
			}

			if (pM->EnPassantCapture) {
				pI->nEPCapture++;
			}

			if (pM->PromoteBishop ||
				pM->PromoteKnight ||
				pM->PromoteQueen ||
				pM->PromoteRook) {
				pI->nPromotion++;
			}

			if (pM->Check) {
				pI->nCheck++;
			}

            if (pM->Checkmate) {
                pI->nCheckmate++;
            }
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

	return pI->nMoves;
}


void perftFast(const ChessPosition& P, int depth, nodecount_t& nNodes)
{
    ChessMove moveList[MOVELIST_SIZE];
    ChessPosition Q = P;
    nodecount_t orig_nNodes = nNodes;

#ifdef _USE_HASH
    // Consult the HashTable:
    HashKey hk = Q.HK^zobristKeys.zkPerftDepth[depth];
    std::atomic<PerftTableEntry> *pAtomicRecord = perftTable.getAddress(hk);		// get address of atomic record
    PerftTableEntry retrievedRecord = pAtomicRecord->load();						// Load a copy of the record
    if (retrievedRecord.Hash == hk) {
        nNodes += retrievedRecord.count;
        return;
    }

    PerftTableEntry newRecord;
    newRecord.Hash = hk;
    newRecord.depth = depth;
#endif // _USE_HASH

    if (depth == 1) { /* Leaf Node*/
        generateMoves(P, moveList);
        int movecount = moveList->MoveCount;
        newRecord.count = movecount;
        nNodes += movecount;
    } else { /* Branch Node */
        generateMoves(P, moveList);
        int movecount = moveList->MoveCount;
        for (int i = 0; i < movecount; i++) {
            Q = P; // unmake move
            Q.performMove(moveList[i]).switchSides(); // make move
            perftFast(Q, depth - 1, nNodes);
        }
        newRecord.count = nNodes - orig_nNodes; // record RELATIVE increase in nodecount
    }

#ifdef _USE_HASH
    do {
        // if (RetrievedRecord has changed) {} // do something (if we care)
    } while (!pAtomicRecord->compare_exchange_weak(retrievedRecord, newRecord)); // loop until successfully written;
#endif

}

void perftMT(ChessPosition P, int maxdepth, int depth, PerftInfo* pI)
{
	ChessMove MoveList[MOVELIST_SIZE];
	generateMoves(P, MoveList);

	if (depth == maxdepth) {
		for (ChessMove* pM = MoveList; pM->EndOfMoveList == 0; pM++)
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
	std::for_each(PerftPartial.begin(), PerftPartial.end(), [pI](const PerftInfo& t) {
		pI->nCapture += t.nCapture;
		pI->nCastle += t.nCastle;
		pI->nCastleLong += t.nCastleLong;
		pI->nEPCapture += t.nEPCapture;
		pI->nMoves += t.nMoves;
		pI->nPromotion += t.nPromotion;
		pI->nCheck += t.nCheck;
		pI->nCheckmate += t.nCheckmate;
	});
}

// perftFastMT() - Multi-threaded perftFast() driver, Thread Pool version - ensures cpu cores are always doing work. Depends on Perft3()
// 01/03/2016: (working ok)

void perftFastMT(ChessPosition P, int depth, nodecount_t& nNodes)
{
    nNodes = 0;

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
    std::vector<int64_t> subTotal(nThreads, 0);
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
                nodecount_t s = 0; 							// local accumulator for thread
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
    nNodes = std::accumulate(subTotal.begin(), subTotal.end(), 0ull);
}

} // namespace juddperft
