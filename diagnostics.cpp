#include "diagnostics.h"
#include "search.h"
#include "movegen.h"
#include "fen.h"

#include <stdio.h>

/////////////////////////////////////////////////////////
//
// Diagnostic Functions:
//
////////////////////////////////////////////////////////

#ifdef INCLUDE_DIAGNOSTICS

void DumpPerftScoreFfromFEN(const char* pzFENstring, unsigned int depth, unsigned __int64 correctAnswer)
{
	ChessPosition P;
	ReadFen(&P, pzFENstring);
	START_TIMER();
	
	__int64 n=0i64;
	PerftFastMTp(P, depth, n);
	printf_s("Perft %d: %I64d (Correct Answer= %I64d)\n", depth, n, correctAnswer);
		
	if (n != correctAnswer)
		printf_s("-== FAIL !!! ==-\n");
	STOP_TIMER();
	printf_s("\n\n");
}

int PerftValidateWithExternal(const char* const pzFENString, int depth, __int64 value)
{
	char command[1024];
	//sprintf_s(command, "%s \"%s\" %d %I64d >>NULL", PerftValidatorPath, pzFENString, depth, value); // Hide output of external program
	sprintf_s(command, "%s \"%s\" %d %I64d", PerftValidatorPath, pzFENString, depth, value); // Show Output of external program
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
		printf_s("Found Bad Position! \n");
		DumpChessPosition(*pP);
		DumpMoveList(MoveList);
		printf_s("Hit enter to continue\n");
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

		printf_s("After Move: ");
		if (pP->BlackToMove)
			printf_s("... ");
		DumpMove(*pM);
		printf_s("Position: %s\n", pzFENString);

		T.nMoves = T.nCapture = T.nEPCapture = T.nCastle = T.nCastleLong = T.nPromotion = 0i64;
		PerftMT(Q, depth - 1, 1, &T);

		printf_s("Validating depth: %d positions: %I64d ", depth - 1, T.nMoves);
		int nResult = PerftValidateWithExternal(pzFENString, depth - 1, T.nMoves);
		if (nResult == PERFTVALIDATE_FALSE)
		{
			// Go Deeper ...
			printf_s("WRONG !! Going deeper ...");
			FindPerftBug(&Q, depth - 1);
		}
		else
		{
			printf_s("ok");
		}
		printf_s("\n\n");

		pM++;
	}
	printf_s("FindPerftBug() finished\n\n");
}


#endif // INCLUDE_DIAGNOSTICS
