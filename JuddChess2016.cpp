#include "movegen.h"
#include "fen.h"
#include "evaluate.h"
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
HashTable <std::atomic<TransTableEntry> > TransTable;
//

int main(int argc, char *argv[], char *envp[])
{

	//printf("sizeof(PerftTableEntry) == %zd\n", sizeof(PerftTableEntry));
	//printf("sizeof(std::atomic<PerftTableEntry>) == %zd\n", sizeof(std::atomic<PerftTableEntry>));

	//printf("sizeof(Move) == %zd\n", sizeof(Move));
	//printf("sizeof(Move2) == %zd\n", sizeof(Move2));
	//printf("sizeof(bool) == %zd\n", sizeof(bool));

	TransTable.SetSize(128000000);
	
//	TransTable.SetSize(2000000000);
	PerftTable.SetSize(MINIMAL_HASHTABLE_SIZE);

	SCORE x;
	x = MobilityBonus[WROOK][0];


	//ChessPosition P;
	//ReadFen(&P, "8/pp4pp/8/3PP3/8/8/P6P/8 w - - 0 1");
	//ReadFen(&P, "8/5ppp/8/P1p5/8/8/5PPP/8 w - - 0 1"); // Kmoch Diagram 5
	//DumpChessPosition(P);
	//EvaluatePawnStructure(P);
	/*ReadFen(&P, "r1b1k2r/pppp1ppp/2n5/3Pp3/1b5q/2P1n1P1/PP1KPP1P/RNBQ1BNR w kq e6 0 1");
	ReadFen(&P, "rnbqkbnr/ppPpp1pp/4N3/5p2/8/2B5/PPP1PPPP/RN1QKB1R w KQkq f6 0 1");
	ReadFen(&P, "rnb1kbnr/ppPpp1pp/4N3/4q3/5p2/4B3/PPPP1PPP/RN1QKB1R b KQkq - 0 1");*/
	/*DumpChessPosition(P);
	Move MoveList[MOVELIST_SIZE];
	GenWhiteCaptureMoves(P, MoveList);
	DumpMoveList(MoveList);*/
	//ReadFen(&P, "8/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/8 w KQkq - 0 25");
	//DumpChessPosition(P);
	//BitBoard First, Last;
	////
	//GetFirstAndLastPiece(P.A, First, Last);
	//DumpBitBoard(P.A);
	//DumpBitBoard(First);
	//DumpBitBoard(Last);
	////
	//GetFirstAndLastPiece(P.B, First, Last);
	//DumpBitBoard(P.B);
	//DumpBitBoard(First);
	//DumpBitBoard(Last);
	////
	//GetFirstAndLastPiece(P.C, First, Last);
	//DumpBitBoard(P.C);
	//DumpBitBoard(First);
	//DumpBitBoard(Last);
	////
	//GetFirstAndLastPiece(P.D, First, Last);
	//DumpBitBoard(P.D);
	//DumpBitBoard(First);
	//DumpBitBoard(Last);
	//
	
	//P.SetupStartPosition();
	/*PerftInfo I;
	PerftMT(P, 7,1, &I);*/
	
	//DumpPerftScoreFfromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 25",5,193690690);	// Position 2: 'Kiwipete' position
	//DumpPerftScoreFfromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0",7, 178633661);								// Position 3
	//DumpPerftScoreFfromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",6, 706045033);		// Position 4
	//DumpPerftScoreFfromFEN("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 6, 706045033);		// Position 4 Mirrored (should be same score as previous)
	//DumpPerftScoreFfromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194);				// Position 5

	/*ChessPosition P;
	ReadFen(&P, "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
	DumpChessPosition(P);
	EvaluateMobility(P);*/

	//DumpBitBoard(GenWhiteAttacks(P));

	/*__int64 nNodes = 0;
	PerftFastMT(P, 7, nNodes);
	printf("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10 %I64d", nNodes);*/

	//	DumpPerftScoreFfromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 7, 287188994746); // Position 6 28/1/2016: Correct (took 8454195 ms)
																															//	Position 6 13/02/2016: Correct (took 4949149 ms on 4 cores)
		
	// FindPerftBug(&P, 5);

	WinBoard(&TheEngine);
	return 0;
}
