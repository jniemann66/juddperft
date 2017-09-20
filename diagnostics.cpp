#include "diagnostics.h"
#include "search.h"
#include "movegen.h"
#include "fen.h"
#include "raiitimer.h"

#include <stdio.h>

/////////////////////////////////////////////////////////
//
// Diagnostic Functions:
//
////////////////////////////////////////////////////////

#ifdef INCLUDE_DIAGNOSTICS

void DumpPerftScoreFfromFEN(const char* pzFENstring, unsigned int depth, uint64_t correctAnswer)
{
	RaiiTimer timer;
	ChessPosition P;
	ReadFen(&P, pzFENstring);
	DumpChessPosition(P);
	
	int64_t n=0;
	PerftFastMT(P, depth, n);
	printf("Perft %d: %lld (Correct Answer= %lld)\n", depth, n, correctAnswer);
		
	if (n != correctAnswer)
		printf("-== FAIL !!! ==-\n");

	printf("\n\n");
}

int PerftValidateWithExternal(const char* const pzFENString, int depth, int64_t value)
{
	char command[1024];
	//sprintf(command, "%s \"%s\" %d %lld >>NULL", perftValidatorPath.c_str(), pzFENString, depth, value); // Hide output of external program
	sprintf(command, "%s \"%s\" %d %lld", perftValidatorPath.c_str(), pzFENString, depth, value); // Show Output of external program
	return (system(command) == EXIT_SUCCESS) ? PERFTVALIDATE_TRUE : PERFTVALIDATE_FALSE;
}

void FindPerftBug(const ChessPosition* pP, int depth)
{
	char pzFENString[1024];

	// Generate Move List
	ChessMove MoveList[MOVELIST_SIZE];
	GenerateMoves(*pP, MoveList);

	if (depth <= 1)
	{
		printf("Found Bad Position! \n");
		DumpChessPosition(*pP);
		DumpMoveList(MoveList);
		printf("Hit enter to continue\n");
		getchar();
		return;
	}

	ChessMove* pM = MoveList;
	PerftInfo T;
	ChessPosition Q;

	while (pM->NoMoreMoves == 0)
	{
		// Set up position
		Q = *pP;
		Q.PerformMove(*pM);
		Q.SwitchSides();

		// generate FEN string
		WriteFen(pzFENString, &Q);

		printf("After Move: ");
		if (pP->BlackToMove)
			printf("... ");
		DumpMove(*pM);
		printf("Position: %s\n", pzFENString);

		T.nMoves = T.nCapture = T.nEPCapture = T.nCastle = T.nCastleLong = T.nPromotion = 0;
		PerftMT(Q, depth - 1, 1, &T);

		printf("Validating depth: %d positions: %lld ", depth - 1, T.nMoves);
		int nResult = PerftValidateWithExternal(pzFENString, depth - 1, T.nMoves);
		if (nResult == PERFTVALIDATE_FALSE)
		{
			// Go Deeper ...
			printf("WRONG !! Going deeper ...");
			FindPerftBug(&Q, depth - 1);
		}
		else
		{
			printf("ok");
		}
		printf("\n\n");

		pM++;
	}
	printf("FindPerftBug() finished\n\n");
}

void RunTestSuite() 
{
	// see https://chessprogramming.wikispaces.com/Perft+Results
	printf("Running Test Suite\n\n");
	DumpPerftScoreFfromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7, 3195901860);				// Posotion 1: Initial Position
	DumpPerftScoreFfromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 25",5,193690690);	// Position 2: 'Kiwipete' position
	DumpPerftScoreFfromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0",7, 178633661);								// Position 3
	DumpPerftScoreFfromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",6, 706045033);		// Position 4
	DumpPerftScoreFfromFEN("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 6, 706045033);		// Position 4 Mirrored (should be same score as previous)
	DumpPerftScoreFfromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194);				// Position 5
	DumpPerftScoreFfromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 7, 287188994746); // Position 6 28/1/2016: Correct (took 8454195 ms)
}

#endif // INCLUDE_DIAGNOSTICS
