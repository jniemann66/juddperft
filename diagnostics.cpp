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

#include "diagnostics.h"
#include "chessposition.h"
#include "search.h"
#include "movegen.h"
#include "fen.h"
#include "raiitimer.h"

#include <cstring>
#include <cinttypes>
#include <cstdio>

////////////////////////////////////////////////////////
//
// Diagnostic Functions:
//
////////////////////////////////////////////////////////

#ifdef INCLUDE_DIAGNOSTICS

namespace juddperft {

void printPerftScoreFfromFEN(const char* pzFENstring, unsigned int depth, uint64_t correctAnswer)
{
	RaiiTimer timer;
	ChessPosition P;
	readFen(&P, pzFENstring);
	P.printPosition();

	nodecount_t n = 0;
	perftFastMT(P, depth, n);
	printf("Perft %d: %" PRIu64 " (Correct! Answer= %" PRIu64 ")\n", depth, n, correctAnswer);

	if (n != correctAnswer)
		printf("-== FAIL !!! ==-\n");

	printf("\n\n");
}

int perftValidateWithExternal(const std::string& validatorPath, const std::string& fenString, int depth, int64_t value)
{
	std::string command = validatorPath + " \"" + fenString + "\" " + std::to_string(depth) + " " + std::to_string(value);

	/* // mock
	std::cout << command << std::endl;
	return PERFTVALIDATE_TRUE;
	*/

	return (system(command.c_str()) == EXIT_SUCCESS) ? PERFTVALIDATE_TRUE : PERFTVALIDATE_FALSE;
}

void findPerftBug(const std::string& validatorPath, const ChessPosition* pP, int depth)
{
	// allocate fen buffer;
	char fenBuffer[1024];
	std::string fenString;

	// generate Move List
	ChessMove MoveList[MOVELIST_SIZE];
	MoveGenerator::generateMoves(*pP, MoveList);

	PerftInfo T;

	if (depth <= 1) { // terminal node

		// generate FEN string
		memset(fenBuffer, 0, 1024);
		writeFen(fenBuffer, pP);
		fenString.assign(fenBuffer);

		std::cout << "Reached Single Position!\n";

		pP->printPosition();
		printMoveList(MoveList);
		perftMT(*pP, 1, 1, &T);
		int nResult = perftValidateWithExternal(validatorPath, fenString, 1, T.nMoves);
		if (nResult == PERFTVALIDATE_FALSE) {
			std::cout << "Engines disagree on Number of moves from this position" << std::endl;
		} else {
			std::cout << "ok" << std::endl;
		}

		std::cout << "Hit enter to continue" << std::endl;
		getchar();

		return;
	}

	ChessMove* pM = MoveList;
	ChessPosition Q;

	while (pM->endOfMoveList == 0)
	{
		// Set up position
		Q = *pP;
		Q.performMove(*pM);
		Q.switchSides();

		// generate FEN string
		memset(fenBuffer, 0, 1024);
		writeFen(fenBuffer, &Q);
		fenString.assign(fenBuffer);

		std::cout << "After Move: ";
		if (pP->blackToMove)
			std::cout << "... ";
		printMove(*pM);
		std::cout << "Position: " << fenString << std::endl;

		T.nMoves = T.nCapture = T.nEPCapture = T.nCastle = T.nCastleLong = T.nPromotion = T.nCheck = T.nCheckmate = 0LL;
		perftMT(Q, depth - 1, 1, &T);

		std::cout << "\nValidating depth: " << depth - 1 << " perft: " << T.nMoves << std::endl;
		int nResult = perftValidateWithExternal(validatorPath, fenString, depth - 1, T.nMoves);
		if (nResult == PERFTVALIDATE_FALSE)
		{
			// Investigate further ...
			std::cout << "WRONG !! Taking a closer look ..." << std::endl;
			findPerftBug(validatorPath, &Q, depth - 1);
		}
		else
		{
			std::cout << "ok";
		}
		std::cout << "\n" << std::endl;

		pM++;
	}
	std::cout << "findPerftBug() finished\n\n";
}

void runTestSuite()
{
// 	// see https://chessprogramming.wikispaces.com/perft+Results
// 	printf("Running Test Suite\n\n");
// //	printPerftScoreFfromFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 7, 3195901860);				// Position 1: Initial Position
// 	printPerftScoreFfromFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 25", 6, 8031647685);	// Position 2: 'Kiwipete' position
// 	printPerftScoreFfromFEN("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0", 8, 3009794393);								// Position 3
// 	printPerftScoreFfromFEN("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6, 706045033);		// Position 4
// 	printPerftScoreFfromFEN("r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 6, 706045033);		// Position 4 Mirrored (should be same score as previous)
// 	printPerftScoreFfromFEN("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194);				// Position 5
// 	printPerftScoreFfromFEN("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 7, 287188994746); // Position 6 28/1/2016: Correct (took 8454195 ms)
// 	// (Position 6 21/11/25 : correct, 211945 ms - but on more modern hardware)

// 	// this next position helped to find an embarassing castling bug, in which a captured, unmoved rook was not resulting in loss of castle rights -
// 	// so if player could get their "other" rook (eg from a1) into position (h1), and didn't move their king, then they were still able to castle, which is illegal.
// 	// It's a weird and rare scenario, but can happen in this position (eg: if 1 ... g2xh1=B):

// 	printPerftScoreFfromFEN("r3k2r/1bp2pP1/5n2/1P1Q4/1pPq4/5N2/1B1P2p1/R3K2R b KQkq c3 0 1", 6, 8419356881); // https://www.talkchess.com/forum3/viewtopic.php?f=2&t=70543

// 	// was getting 8419356941 (+60) when bug was in effect

	// https://www.talkchess.com/forum/viewtopic.php?t=59781
	printPerftScoreFfromFEN("rnb1kbnr/pp1pp1pp/1qp2p2/8/Q1P5/N7/PP1PPPPP/1RB1KBNR b Kkq - 2 4 18", 7, 14'794'751'816);



}

} // namespace juddperft
#endif // INCLUDE_DIAGNOSTICS
