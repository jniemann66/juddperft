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

// winboard.cpp - Winboard Interface driver

#include "winboard.h"
#include "juddperft.h"
#include "diagnostics.h"
#include "engine.h"
#include "fen.h"
#include "hashtable.h"
#include "movegen.h"
#include "raiitimer.h"
#include "search.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace juddperft {

std::ofstream logfile("perft.txt");

WinboardInputCommandDefinition winboardInputCommands[]=
{
	{"xboard", parse_input_xboard, false},
	{"protover", parse_input_protover, false}, 						/* N */
	{"accepted", parse_input_accepted, false},
	{"rejected", parse_input_rejected, false},
	{"new", parse_input_new, false},
	{"variant", parse_input_variant, false}, 						/* VARNAME */
	{"quit", parse_input_quit, true},
	{"random", parse_input_random, false},
	{"force", parse_input_force, false},
	{"go", parse_input_go, false},
	{"playother", parse_input_playother, false},
	{"white", parse_input_white, false},
	{"black", parse_input_black, false},
	{"level", parse_input_level, false}, 							/* MPS BASE INC */
	{"st", parse_input_st, false},									/* TIME */
	{"sd", parse_input_sd, false}, 									/* DEPTH */
	{"nps", parse_input_nps, false}, 								/* NODE_RATE */
	{"time", parse_input_time, false},								/* N */
	{"otim", parse_input_otim, false},								/* N */
	{"MOVE", parse_input_move, false},
	{"usermove", parse_input_usermove, false}, 						/* MOVE */
	{"?", parse_input_movenow, false},
	{"ping", parse_input_ping, false},								/* N */
	{"draw", parse_input_draw, false},
	{"result", parse_input_result, false},							/* RESULT {COMMENT} */
	{"setboard", parse_input_setboard, true}, 						/* FEN */
	{"edit", parse_input_edit, false},
	{"hint", parse_input_hint, false},
	{"bk", parse_input_bk, false},
	{"undo", parse_input_undo, false},
	{"remove", parse_input_remove, false},
	{"hard", parse_input_hard, false},
	{"easy", parse_input_easy, false},
	{"post", parse_input_post, false},
	{"nopost", parse_input_nopost, false},
	{"analyze", parse_input_analyze, false},
	{"name", parse_input_name, false},								/* X */
	{"rating", parse_input_rating, false},
	{"ics", parse_input_ics, false}, 								/* HOSTNAME */
	{"computer", parse_input_computer, false},
	{"pause", parse_input_pause, false},
	{"resume", parse_input_resume, false},
	// New in WinBoard 4.4:
	{"memory", parse_input_memory, true },
	{"cores", parse_input_cores, true},
	{"egtpath", parse_input_egtpath, false},
	//
	{"option", parse_input_option, false},
	// extended commands
	{"movelist", parse_input_movelist, true},
	{"showposition", parse_input_showposition, true},
	{"showhash", parse_input_showhash, true},
	{"perft", parse_input_perft, true},
	{"perftfast", parse_input_perftfast, true},
	{"divide", parse_input_divide, true},
	{ "dividefast", parse_input_dividefast, true },
	{"writehash", parse_input_writehash, false},
	{"lookuphash", parse_input_lookuphash, false},
	{"test-external", parse_input_testExternal, true}
};

int winBoard(Engine* pE)
{
	// turn off output buffering
	setbuf(stdout, NULL);
	pE->currentPosition.setupStartPosition();
	printf("\nSupported Commands:\n");
	for (int i = 0; i < (sizeof(winboardInputCommands) / sizeof(WinboardInputCommandDefinition)); i++)
	{
		if (winboardInputCommands[i].implemented)
			printf("   %s\n", winboardInputCommands[i].pzCommandString);
	}
	printf("\n");
	while (waitForInput(pE));
	return 0;
}

/////////////////////////////////////////////////////
// waitForInput()
// Wait for an incoming command and then process it
/////////////////////////////////////////////////////

bool waitForInput(Engine* pE)
{
	int nRecognizedCommands = sizeof(winboardInputCommands)/sizeof(winboardInputCommands[0]);

	char* input = NULL;
	std::string inputStr;
	std::getline(std::cin, inputStr);
	if (inputStr.empty())
		return true;

	input = strdup(inputStr.c_str()); // damn ugly. to-do: stop using strtok altogether

	if (input != NULL)
	{
		char* command;
		char* args;

		// log input
		logInput(logfile, input);
		command = strtok(input, " \n");
		if (command != NULL)
		{

			// search for command
			for (int i = 0; i < nRecognizedCommands; i++)
			{
				if (_stricmp(command, winboardInputCommands[i].pzCommandString) == 0)
				{
					if (_stricmp(command, "quit") == 0)
					{
						pE->stopSignal = true;
						return false;
					}
					// separate command from remainder of string
					args = strtok(NULL, "\n" /* note: deliberately ignore spaces */);
					winboardInputCommands[i].pF(args, pE); // invoke handler for function
					return true;
				}
			}
			// still here ? OK, we interpret input as a move:
			parse_input_usermove(command, pE);
		}
		free(input);
	}
	return true;
}

/////////////////////////////////////
// isImplemented() - returns true if
// ascii command pointed to by s
// is recognized AND implemented
/////////////////////////////////////

bool isImplemented(const char* s, Engine* pE)
{
	bool bRetVal = false;
	int nRecognizedCommands = sizeof(winboardInputCommands)/sizeof(winboardInputCommands[0]);
	if (s != NULL)
	{
		// search for command
		for (int i = 0; i<nRecognizedCommands; i++)
		{
			if (_stricmp(s, winboardInputCommands[i].pzCommandString) == 0)
			{
				if (winboardInputCommands[i].implemented)
					bRetVal = true;
				break;
			}
		}
	}
	return bRetVal;
}

// Handler functions for incoming commands:
void parse_input_xboard(const char* s, Engine* pE)
{
	winBoardOutput("\n");
}

void parse_input_protover(const char* s, Engine* pE)
{
	if (s != NULL)
	{
		// if protocol version is 2 or higher, send feature list,
		// by using feature command:
		if (atoi(s) >= 2)
			send_output_feature(pE);
		else
		{
			// to-do: prevent sending of Winboard-2 commands ?
		}
	}
}

void parse_input_accepted(const char* s, Engine* pE){}
void parse_input_rejected(const char* s, Engine* pE){}
void parse_input_new(const char* s, Engine* pE){}
void parse_input_variant(const char* s, Engine* pE){}
void parse_input_quit(const char* s, Engine* pE){}
void parse_input_random(const char* s, Engine* pE){}
void parse_input_force(const char* s, Engine* pE){}
void parse_input_go(const char* s, Engine* pE){}
void parse_input_playother(const char* s, Engine* pE){}
void parse_input_white(const char* s, Engine* pE){}
void parse_input_black(const char* s, Engine* pE){}
void parse_input_level(const char* s, Engine* pE){}
void parse_input_st(const char* s, Engine* pE){}
void parse_input_sd(const char* s, Engine* pE){}

void parse_input_nps(const char* s, Engine* pE) {}
void parse_input_time(const char* s, Engine* pE){}
void parse_input_otim(const char* s, Engine* pE){}
void parse_input_move(const char* s, Engine* pE){}
void parse_input_usermove(const char* s, Engine* const pE){}
void parse_input_movenow(const char* s, Engine* pE){}
void parse_input_ping(const char* s, Engine* pE){}
void parse_input_draw(const char* s, Engine* pE){}
void parse_input_result(const char* s, Engine* pE){}
void parse_input_setboard(const char* s, Engine* pE)
{
	if (s == NULL)
		return;

	if (!readFen(&pE->currentPosition, s))
		send_output_tellusererror("Illegal Position", pE);
}
void parse_input_edit(const char* s, Engine* pE){}
void parse_input_hint(const char* s, Engine* pE){}
void parse_input_bk(const char* s, Engine* pE){}
void parse_input_undo(const char* s, Engine* pE){}
void parse_input_remove(const char* s, Engine* pE){}
void parse_input_hard(const char* s, Engine* pE){ }
void parse_input_easy(const char* s, Engine* pE) {}
void parse_input_post(const char* s, Engine* pE){}
void parse_input_nopost(const char* s, Engine* pE){}
void parse_input_analyze(const char* s, Engine* pE){}
void parse_input_name(const char* s, Engine* pE){}
void parse_input_rating(const char* s, Engine* pE){}
void parse_input_ics(const char* s, Engine* pE){}
void parse_input_computer(const char* s, Engine* pE){}
void parse_input_pause(const char* s, Engine* pE){}
void parse_input_resume(const char* s, Engine* pE){}

// extended input commands
void parse_input_movelist(const char* s, Engine* pE)
{
	ChessMove MoveList[MOVELIST_SIZE];
	generateMoves(pE->currentPosition, MoveList);
	dumpMoveList(MoveList, CoOrdinate);
}

void parse_input_showposition(const char* s, Engine* pE)
{
	dumpChessPosition(pE->currentPosition);
}
void parse_input_showhash(const char* s, Engine* pE)
{
#ifdef _USE_HASH
	printf("Perft Table Size: %lld bytes\n", perftTable.getSize());
	uint64_t numEntries = perftTable.getNumEntries();
	std::vector<int64_t> depthTally(16, 0);
	std::atomic<PerftTableEntry> *pBaseAddress = perftTable.getAddress(0);
	std::atomic<PerftTableEntry> *pAtomicRecord;

	for (uint64_t x = 0; x < numEntries; x++) {
		pAtomicRecord = pBaseAddress + x;
		PerftTableEntry RetrievedRecord = pAtomicRecord->load();
		++depthTally[RetrievedRecord.depth];
	}

	for (unsigned int d = 0; d < 16; d++) {
		printf("Depth %d: %lld (%2.1f%%)\n", d, depthTally[d], 100.0 * static_cast<float>(depthTally[d]) / static_cast<float>(numEntries));
	}
	printf("Total: %lld\n", std::accumulate(depthTally.begin(), depthTally.end(), 0));
#endif
}

void parse_input_perft(const char* s, Engine* pE)
{
	if (s == nullptr)
		return;

	for (int q = 1; q <= atoi(s); q++)
	{
		{
			RaiiTimer timer;
			PerftInfo T;
			perftMT(pE->currentPosition, q, 1, &T);
			printf("Perft %d: %lld \nTotal Captures= %lld Castles= %lld CastleLongs= %lld EPCaptures= %lld Promotions= %lld Checks= %lld Checkmates= %lld\n",
				   q,
				   T.nMoves,
				   T.nCapture + T.nEPCapture,
				   T.nCastle,
				   T.nCastleLong,
				   T.nEPCapture,
				   T.nPromotion,
				   T.nCheck,
				   T.nCheckmate
				   );
			printf("\n");
			timer.setNodes(T.nMoves);
		}
	}
}

void parse_input_perftfast(const char* s, Engine* pE) {

	if (s == NULL)
		return;

	for (int q = 1; q <= atoi(s); q++)
	{
		{
			RaiiTimer timer;
			nodecount_t nNumPositions = 0;
			perftFastMT(pE->currentPosition, q, nNumPositions);
			printf("\nPerft %d: %lld \n",
				q, nNumPositions
				);
			printf("\n");
			timer.setNodes(nNumPositions);
		}
	}
}

void parse_input_divide(const char* s, Engine* pE)
{
	if (s == NULL)
		return;

	int depth;
	depth = std::max(2, atoi(s));

	ChessMove MoveList[MOVELIST_SIZE];
	generateMoves(pE->currentPosition, MoveList);
	ChessMove* pM = MoveList;
	PerftInfo GT;
	ChessPosition Q;
	RaiiTimer timer;

	while (pM->EndOfMoveList == 0)
	{
		Q = pE->currentPosition;
		Q.performMove(*pM);
		Q.switchSides();
		dumpMove(*pM, LongAlgebraicNoNewline);
		PerftInfo T;
		perftMT(Q, depth-1, 1, &T);
		printf("Perft %d: %lld \nTotal Captures= %lld Castles= %lld CastleLongs= %lld EPCaptures= %lld Promotions= %lld Checks= %lld Checkmates= %lld\n",
			depth-1,
			T.nMoves,
			T.nCapture + T.nEPCapture,
			T.nCastle,
			T.nCastleLong,
			T.nEPCapture,
			T.nPromotion,
			T.nCheck,
			T.nCheckmate
			);

		printf("\n");

		GT.nMoves += T.nMoves;
		GT.nCapture += T.nCapture;
		GT.nCastle += T.nCastle;
		GT.nCastleLong += T.nCastleLong;
		GT.nEPCapture += T.nEPCapture;
		GT.nPromotion += T.nPromotion;
		GT.nCheck += T.nCheck;
		GT.nCheckmate += T.nCheckmate;

		pM++;
	}

	printf("Summary:\nPerft %d: %lld \nTotal Captures= %lld Castles= %lld CastleLongs= %lld EPCaptures= %lld Promotions= %lld Checks= %lld Checkmates= %lld\n",
		depth,
		GT.nMoves,
		GT.nCapture + GT.nEPCapture,
		GT.nCastle,
		GT.nCastleLong,
		GT.nEPCapture,
		GT.nPromotion,
		GT.nCheck,
		GT.nCheckmate
		);
}

void parse_input_dividefast(const char* s, Engine* pE)
{
	if (s == NULL)
		return;

	int depth;
	depth = std::max(2, atoi(s));

	ChessMove MoveList[MOVELIST_SIZE];
	generateMoves(pE->currentPosition, MoveList);
	ChessMove* pM = MoveList;
	ChessPosition Q;
	int64_t grandtotal = 0;
	RaiiTimer timer;

	while (pM->EndOfMoveList == 0)
	{
		Q = pE->currentPosition;
		Q.performMove(*pM).switchSides();

		dumpMove(*pM, LongAlgebraicNoNewline);
		nodecount_t nNumPositions = 0;
		perftFastMT(Q, depth - 1, nNumPositions);
		printf(" %lld \n",
			nNumPositions
			);
		grandtotal += nNumPositions;
		pM++;
	}
	timer.setNodes(grandtotal);
	printf("\nPerft %d: %lld\n", depth, grandtotal);
}

void parse_input_writehash(const char* s, Engine* pE){}

void parse_input_lookuphash(const char* s, Engine* pE)
{
#ifdef _USE_HASH
#endif
}

void parse_input_memory(const char* s, Engine* pE) {
	uint64_t BytesRequested = _atoi64(s);
	if (s == NULL)
		return;

	setMemory(BytesRequested);
}
void parse_input_cores(const char* s, Engine* pE) {

	if (s == NULL)
		return;

	pE->nNumCores = std::max(1, std::min(atoi(s), MAX_THREADS));
}
void parse_input_egtpath(const char* s, Engine* pE) {}
void parse_input_option(const char* s, Engine* pE) {}

void parse_input_testExternal(const char* s, Engine* pE) {
#ifdef INCLUDE_DIAGNOSTICS
	bool badArgs = true; // anticipate the worst :-)
	if (s != nullptr && strlen(s) != 0) {
		std::string str(s);
		std::istringstream iss(str);
		std::vector<std::string> args;
		std::string arg;
		while (std::getline(iss, arg, ' ')) {
			args.push_back(arg);
		}

		if (args.size() == 2) {
			badArgs = false;
			std::string perftValidatorPath(args.at(0));
			int depth = std::stoi(args.at(1));
			std::cout << "\nTesting against external engine: " << perftValidatorPath;
			std::cout << " to depth of " << depth;
			std::cout << "\nPosition:\n" << std::endl;
			ChessPosition currentPosition = pE->currentPosition;
			dumpChessPosition(currentPosition);
			std::cout << "Off we go ... " << std::endl;
			findPerftBug(perftValidatorPath, &currentPosition, depth);
		}
	}

	if (badArgs) {
		std::cout << "\nUsage: text-external <path to external app> <depth>\n\n";
		std::cout << "This will issue the following system command for each test position\n";
		std::cout << "<external app> \"<Fen String>\" <depth> <perft value>\n";
		std::cout << "external app is expected to terminate with EXIT_SUCCESS (0) if it agrees with\n";
		std::cout << "the perft value for the given position\n" << std::endl;
	}

#endif
}

// ---------------- Output Functions -------------------------------
void  send_output_feature(Engine* pE)
{
	char t[4096], s[1024];

	sprintf(t, "feature ");
	sprintf(s, "done=0 "); strcat(t, s);
	//
	sprintf(s, "\nfeature "); strcat(t, s);
	sprintf(s, "ping=%d ", isImplemented("ping", pE)? 1:0); strcat(t, s);					// recommended:	1
	sprintf(s, "setboard=%d ", isImplemented("setboard", pE)? 1:0); strcat(t, s);			// recommended: 1
	sprintf(s, "playother=%d ", isImplemented("playother", pE)? 1:0); strcat(t, s);		// recommended: 1
	sprintf(s, "san=0 "); strcat(t, s);													// no recommendation
	sprintf(s, "usermove=%d ", isImplemented("usermove", pE)? 1:0); strcat(t, s);			// no recommendation
	sprintf(s, "time=%d ", isImplemented("time", pE)? 1:0); strcat(t, s);					// recommended: 1
	sprintf(s, "draw=%d ", isImplemented("draw", pE)? 1:0); strcat(t, s);					// recommended: 1
	//
	sprintf(s, "\nfeature "); strcat(t, s);
	sprintf(s, "sigint=0 "); strcat(t, s);												// recommended: 1
	sprintf(s, "sigterm=0 "); strcat(t, s);												// recommended: 1
	sprintf(s, "reuse=1 "); strcat(t, s);												// recommended: 1
	sprintf(s, "analyze=%d ", isImplemented("analyze", pE)? 1:0); strcat(t, s);			// recommended: 1
	sprintf(s, "myname=\"JuddChess v1.0\" "); strcat(t, s);								// no recommendation
	//
	sprintf(s, "\nfeature "); strcat(t, s);
	sprintf(s, "variants=\"normal\" "); strcat(t, s);									// recommended: "normal"
	sprintf(s, "colors=0 "); strcat(t, s);												// recommended: 0
	sprintf(s, "ics=0 "); strcat(t, s);													// no recommendation
	sprintf(s, "name=%d ", isImplemented("name", pE)? 1:0); strcat(t, s);
	sprintf(s, "pause=%d ", isImplemented("pause", pE)? 1:0); strcat(t, s);
	//
	sprintf(s, "\nfeature "); strcat(t, s);
	sprintf(s, "nps=%d ", isImplemented("nps", pE) ? 1 : 0); strcat(t, s);
	sprintf(s, "debug=0 "); strcat(t, s);
	sprintf(s, "memory=%d ", isImplemented("memory", pE) ? 1 : 0); strcat(t, s);
	sprintf(s, "smp=%d ", isImplemented("cores", pE) ? 1 : 0); strcat(t, s);
	sprintf(s, "smp=0 "); strcat(t, s);
	sprintf(s, "\nfeature done=1 "); strcat(t, s);
	sprintf(s, "\n"); strcat(t, s);
	winBoardOutput(t);
}
void  send_output_illegalmove(const char* s, Engine* pE){}
void  send_output_error(const char* s, Engine* pE){}
void  send_output_move(const ChessMove& M){}
void  send_output_hint(const ChessMove& M){}
void  send_output_result(const char* s, Engine* pE){}
void  send_output_resign(Engine* pE){}
void  send_output_offerdraw(Engine* pE){}
void  send_output_tellopponent(const char* s, Engine* pE){}
void  send_output_tellothers(const char* s, Engine* pE){}
void  send_output_tellall(const char* s, Engine* pE){}
void  send_output_telluser(const char* s, Engine* pE){}
void  send_output_tellusererror(const char* s, Engine* pE){}
void  send_output_askuser(char* r, const char* s, Engine* pE){}
void  send_output_tellics(const char* s, Engine* pE){}
void  send_output_tellicsnoalias(const char* s, Engine* pE){}

void winBoardOutput(const char* s)
{
	printf("%s", s);
	logOutput(logfile, s);
}

void logInput(std::ofstream& logfile, const char* s)
{
	if (logfile)
		logfile << "Received command: " << s << std::endl;
}

void logOutput(std::ofstream& logfile, const char* s)
{
	if (logfile)
		logfile << "Sent command: " << s << std::endl;
}

void sendReplyMove(const char* s, Engine* pE){}


void sendReplyMoveAndPonder(const char* s, Engine* pE) {}

} // namespace juddperft
