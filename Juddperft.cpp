#include "movegen.h"
#include "fen.h"
#include "search.h"
#include "winboard.h"
#include "Diagnostics.h"
#include "hashtable.h"

#include <stdio.h>
#include <intrin.h>

#include <atomic>

// Globals:
Engine TheEngine;
HashTable <std::atomic<PerftTableEntry> > PerftTable;
//

int main(int argc, char *argv[], char *envp[])
{

	printf("sizeof(PerftTableEntry) == %zd\n", sizeof(PerftTableEntry));
	printf("sizeof(std::atomic<PerftTableEntry>) == %zd\n", sizeof(std::atomic<PerftTableEntry>));
	printf("sizeof(ChessMove) == %zd\n", sizeof(ChessMove));
	
	//PerftTable.SetSize(MINIMAL_HASHTABLE_SIZE);
	PerftTable.SetSize(4000000000);
	
	
	ChessPosition P;
	//DumpChessPosition(P);
	P.SetupStartPosition();
	
	//DumpPerftScoreFfromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 25",5,193690690);	// Position 2: 'Kiwipete' position
	//DumpPerftScoreFfromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0",7, 178633661);								// Position 3
	//DumpPerftScoreFfromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",6, 706045033);		// Position 4
	//DumpPerftScoreFfromFEN("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 6, 706045033);		// Position 4 Mirrored (should be same score as previous)
	//DumpPerftScoreFfromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194);				// Position 5
	
	/*__int64 nNodes = 0;
	PerftFastMT(P, 7, nNodes);
	printf("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 %I64d", nNodes);*/

	//	DumpPerftScoreFfromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 7, 287188994746); // Position 6 28/1/2016: Correct (took 8454195 ms)
	//	Position 6 13/02/2016: Correct (took 4949149 ms on 4 cores)

	// FindPerftBug(&P, 5);

	WinBoard(&TheEngine);
	return 0;
}
