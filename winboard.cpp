// winboard.cpp - Winboard Interface driver

#include "winboard.h"
#include "movegen.h"
#include "fen.h"
#include "search.h"
#include "engine.h"
#include "hashtable.h"
#include "Juddperft.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <cassert>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <numeric>

std::ofstream logfile("perft.txt");

#ifdef _USE_HASH
extern HashTable <std::atomic<PerftTableEntry> > PerftTable;
extern HashTable <std::atomic<LeafEntry>> LeafTable;
#endif

WINBOARD_INPUT_COMMAND_DEFINITION WinboardInputCommands[]=
{
	{"xboard",parse_input_xboard,false},
	{"protover",parse_input_protover,false},						/* N */
	{"accepted",parse_input_accepted,false},
	{"rejected",parse_input_rejected,false},
	{"new",parse_input_new,false},
	{"variant",parse_input_variant,false},						/* VARNAME */ 
	{"quit",parse_input_quit,true},
	{"random",parse_input_random,false},
	{"force",parse_input_force,false},
	{"go",parse_input_go,false},
	{"playother",parse_input_playother,false},
	{"white",parse_input_white,false},
	{"black",parse_input_black,false},
	{"level",parse_input_level,false},							/* MPS BASE INC */ 
	{"st",parse_input_st,false},								/* TIME */ 
	{"sd",parse_input_sd,false},									/* DEPTH */
	{"nps",parse_input_nps,false},								/* NODE_RATE */
	{"time",parse_input_time,false},							/* N */
	{"otim",parse_input_otim,false},							/* N */
	{"MOVE",parse_input_move,false},
	{"usermove",parse_input_usermove,false},						/* MOVE */ 
	{"?",parse_input_movenow,false}, 
	{"ping",parse_input_ping,false},							/* N */
	{"draw",parse_input_draw,false},
	{"result",parse_input_result,false},						/* RESULT {COMMENT} */ 
	{"setboard",parse_input_setboard,true},						/* FEN */ 
	{"edit",parse_input_edit,false},
	{"hint",parse_input_hint,false},
	{"bk",parse_input_bk,false},
	{"undo",parse_input_undo,false},
	{"remove",parse_input_remove,false},
	{"hard",parse_input_hard,false},
	{"easy",parse_input_easy,false},
	{"post",parse_input_post,false},
	{"nopost",parse_input_nopost,false},
	{"analyze",parse_input_analyze,false},
	{"name",parse_input_name,false},							/* X */ 
	{"rating",parse_input_rating,false},
	{"ics",parse_input_ics,false},								/* HOSTNAME */ 
	{"computer",parse_input_computer,false},
	{"pause",parse_input_pause,false},
	{"resume",parse_input_resume,false},
	// New in WinBoard 4.4:
	{"memory",parse_input_memory,true },
	{"cores",parse_input_cores,true},
	{"egtpath",parse_input_egtpath,false},
	//
	{"option",parse_input_option,false},
	// extended commands
	{"movelist",parse_input_movelist,true},
	{"showposition",parse_input_showposition,true},
	{"showhash",parse_input_showhash,true},
	{"perft",parse_input_perft,true},
	{"perftfast",parse_input_perftfast,true},
	{"divide",parse_input_divide,true},
	{ "dividefast",parse_input_dividefast,true },
	{"writehash",parse_input_writehash,false},
	{"lookuphash",parse_input_lookuphash,false}
};

int WinBoard(Engine* pE)
{
	// turn off output buffering
	setbuf(stdout,NULL);
	pE->CurrentPosition.SetupStartPosition();
	printf_s("\nSupported Commands:\n");
	for (int i = 0; i < (sizeof(WinboardInputCommands) / sizeof(WINBOARD_INPUT_COMMAND_DEFINITION)); i++)
	{
		if (WinboardInputCommands[i].implemented)
			printf_s("   %s\n", WinboardInputCommands[i].pzCommandString);
	}
	printf_s("\n");
	while(WaitForInput(pE));
	return 0;
}

/////////////////////////////////////////////////////
// WaitForInput()
// Wait for an incoming command and then process it
/////////////////////////////////////////////////////

bool WaitForInput(Engine* pE)
{
	int nRecognizedCommands=sizeof(WinboardInputCommands)/sizeof(WinboardInputCommands[0]); 
	char input[2048];
	char* command;
	char* args;
	gets_s(input,2047);
	if(input != NULL)
	{
		// log input
		LogInput(logfile,input);
		command=strtok(input," \n");
		if(command!=NULL)
		{
			
			// search for command
			for(int i=0;i<nRecognizedCommands;i++)
			{
				if(_stricmp(command,WinboardInputCommands[i].pzCommandString)==NULL)
				{
					if(_stricmp(command,"quit")==NULL)
					{
						pE->StopSignal = true;
						return false;
					}
					// separate command from remainder of string
					args=strtok(NULL,"\n" /* note: deliberately ignore spaces */);
					WinboardInputCommands[i].pF(args,pE); // invoke handler for function
					return true;
				}
			}
			// still here ? OK, we interpret input as a move:
			parse_input_usermove(command,pE);
		}
	}
	return true;
}

/////////////////////////////////////
// IsImplemented() - returns true if 
// ascii command pointed to by s 
// is recognized AND implemented
/////////////////////////////////////

bool IsImplemented(const char* s,Engine* pE)
{
	bool bRetVal = false;
	int nRecognizedCommands=sizeof(WinboardInputCommands)/sizeof(WinboardInputCommands[0]); 
	if(s != NULL)
	{
		// search for command
		for(int i=0;i<nRecognizedCommands;i++)
		{
			if(_stricmp(s,WinboardInputCommands[i].pzCommandString)==NULL)
			{
				if(WinboardInputCommands[i].implemented)
					bRetVal=true;
				break;
			}
		}
	}
	return bRetVal;
}

// Handler functions for incoming commands:
void parse_input_xboard(const char* s,Engine* pE)
{
	WinBoardOutput("\n");
}
void parse_input_protover(const char* s,Engine* pE)
{	
	if (s != NULL)
	{
		// if protocol version is 2 or higher, send feature list,
		// by using feature command:
		if (atoi(s)>=2)
			send_output_feature(pE);
		else
		{
			// to-do: prevent sending of Winboard-2 commands ?
		}
	}
}		
void parse_input_accepted(const char* s,Engine* pE){}
void parse_input_rejected(const char* s,Engine* pE){}
void parse_input_new(const char* s,Engine* pE){} 
void parse_input_variant(const char* s,Engine* pE){}			
void parse_input_quit(const char* s,Engine* pE){} 
void parse_input_random(const char* s,Engine* pE){} 
void parse_input_force(const char* s,Engine* pE){} 
void parse_input_go(const char* s,Engine* pE){} 
void parse_input_playother(const char* s,Engine* pE){} 
void parse_input_white(const char* s,Engine* pE){} 
void parse_input_black(const char* s,Engine* pE){} 
void parse_input_level(const char* s,Engine* pE){}				
void parse_input_st(const char* s,Engine* pE){}					
void parse_input_sd(const char* s,Engine* pE){}	

void parse_input_nps(const char* s, Engine* pE) {}
void parse_input_time(const char* s,Engine* pE){}				
void parse_input_otim(const char* s,Engine* pE){} 
void parse_input_move(const char* s,Engine* pE){} 
void parse_input_usermove(const char* s,Engine* const pE){}		
void parse_input_movenow(const char* s,Engine* pE){} 
void parse_input_ping(const char* s,Engine* pE){}				
void parse_input_draw(const char* s,Engine* pE){} 
void parse_input_result(const char* s,Engine* pE){}				
void parse_input_setboard(const char* s,Engine* pE)
{
	if (s == NULL)
		return;

	if(!ReadFen(&pE->CurrentPosition,s))
		send_output_tellusererror("Illegal Position",pE);
}			
void parse_input_edit(const char* s,Engine* pE){} 
void parse_input_hint(const char* s,Engine* pE){} 
void parse_input_bk(const char* s,Engine* pE){} 
void parse_input_undo(const char* s,Engine* pE){} 
void parse_input_remove(const char* s,Engine* pE){} 
void parse_input_hard(const char* s,Engine* pE){ }
void parse_input_easy(const char* s, Engine* pE) {}
void parse_input_post(const char* s,Engine* pE){} 
void parse_input_nopost(const char* s,Engine* pE){}
void parse_input_analyze(const char* s,Engine* pE){} 
void parse_input_name(const char* s,Engine* pE){}				
void parse_input_rating(const char* s,Engine* pE){} 
void parse_input_ics(const char* s,Engine* pE){}				
void parse_input_computer(const char* s,Engine* pE){} 
void parse_input_pause(const char* s,Engine* pE){} 
void parse_input_resume(const char* s,Engine* pE){}

// extended input commands
void parse_input_movelist(const char* s,Engine* pE)
{
	ChessMove MoveList[MOVELIST_SIZE];
	GenerateMoves(pE->CurrentPosition,MoveList);
	DumpMoveList(MoveList,CoOrdinate);
}

void parse_input_showposition(const char* s, Engine* pE)
{
	DumpChessPosition(pE->CurrentPosition);
}
void parse_input_showhash(const char* s,Engine* pE)
{
#ifdef _USE_HASH
	printf_s("Leaf Node Table Size: %I64d bytes\n", LeafTable.GetSize());
	__int64 numEntries = LeafTable.GetNumEntries();
	__int64 nPopulatedLeafEntries = 0i64;
	std::atomic<LeafEntry> *pLeafTableBaseAddress= LeafTable.GetAddress(0i64);
	LeafEntry LE;
	for (__int64 w = 0; w < numEntries; w++) {
		LE = (pLeafTableBaseAddress + w)->load();
		if (LE.count != 0)
			nPopulatedLeafEntries++;
	}
	printf_s("%I64d entries occupied out of %I64d (%2.2f%%)\n\n", nPopulatedLeafEntries, numEntries, 100.0*static_cast<float>(nPopulatedLeafEntries) / static_cast<float>(numEntries));
	//
	printf_s("Perft Table Size: %I64d bytes\n", PerftTable.GetSize());
	numEntries = PerftTable.GetNumEntries();
	std::vector<__int64> depthTally(16,0);
	std::atomic<PerftTableEntry> *pBaseAddress = PerftTable.GetAddress(0i64);
	std::atomic<PerftTableEntry> *pAtomicRecord;
	
	for (__int64 x = 0; x < numEntries; x++) {
		pAtomicRecord = pBaseAddress + x;
		PerftTableEntry RetrievedRecord = pAtomicRecord->load();
		++depthTally[RetrievedRecord.depth];
	}

	for (unsigned int d = 0; d < 16; d++) {
		printf_s("Depth %d: %I64d (%2.1f%%)\n", d, depthTally[d],100.0*static_cast<float>(depthTally[d])/static_cast<float>(numEntries));
	}
	printf_s("Total: %I64d\n", std::accumulate(depthTally.begin(), depthTally.end(), 0i64));
#endif
}

void parse_input_perft(const char* s,Engine* pE)
{
	if (s == NULL)
		return;

	for(int q=1; q<=atoi(s); q++)
	{
		START_TIMER();
		PerftInfo T;
		T.nMoves = T.nCapture = T.nEPCapture = T.nCastle = T.nCastleLong = T.nPromotion = 0i64;
		PerftMT
		//Perft
			(pE->CurrentPosition,q,1,&T);
		printf_s("Perft %d: %I64d \nCaptures= %I64d Castles= %I64d CastleLongs= %I64d EPCaptures= %I64d Promotions= %I64d\n",
			q,
			T.nMoves,
			T.nCapture,
			T.nCastle,
			T.nCastleLong,
			T.nEPCapture,
			T.nPromotion
			);
		STOP_TIMER();
		printf_s("\n\n");
	}
}

void parse_input_perftfast(const char* s, Engine* pE) {
	
	if (s == NULL)
		return;

	for (int q = 1; q <= atoi(s); q++)
	{
		START_TIMER();
		__int64 nNumPositions = 0i64;
		
		PerftFastMT(pE->CurrentPosition, q, nNumPositions);
		printf_s("Perft %d: %I64d \n",
			q,nNumPositions
			);
		STOP_TIMER();
		printf_s("\n\n");
	}
}

void parse_input_divide(const char* s, Engine* pE)
{
	if (s == NULL)
		return;

	int depth;
	depth = max(2,atoi(s));

	ChessMove MoveList[MOVELIST_SIZE];
	GenerateMoves(pE->CurrentPosition, MoveList);
	ChessMove* pM = MoveList;
	PerftInfo GT;
	GT.nMoves = GT.nCapture = GT.nEPCapture = GT.nCastle = GT.nCastleLong = GT.nPromotion = 0i64;
	PerftInfo T;
	ChessPosition Q;
	START_TIMER();
	while(pM->NoMoreMoves==0)
	{ 
		Q = pE->CurrentPosition;
		Q.PerformMove(*pM);
		Q.SwitchSides();
	
		DumpMove(*pM,LongAlgebraicNoNewline);
	
		T.nMoves = T.nCapture = T.nEPCapture = T.nCastle = T.nCastleLong = T.nPromotion = 0i64;
		PerftMT(Q, depth-1, 1, &T);
		printf_s("Perft %d: %I64d \nCaptures= %I64d Castles= %I64d CastleLongs= %I64d EPCaptures= %I64d Promotions= %I64d\n",
			depth-1,
			T.nMoves,
			T.nCapture,
			T.nCastle,
			T.nCastleLong,
			T.nEPCapture,
			T.nPromotion
			);
		
		printf_s("\n");

		GT.nMoves += T.nMoves;
		GT.nCapture += T.nCapture;
		GT.nCastle += T.nCastle;
		GT.nCastleLong += T.nCastleLong;
		GT.nEPCapture += T.nEPCapture;
		GT.nPromotion += T.nPromotion;
		
		pM++;
	}
	STOP_TIMER();
	printf_s("Summary:\nPerft %d: %I64d \nCaptures= %I64d Castles= %I64d CastleLongs= %I64d EPCaptures= %I64d Promotions= %I64d\n",
		depth,
		GT.nMoves,
		GT.nCapture,
		GT.nCastle,
		GT.nCastleLong,
		GT.nEPCapture,
		GT.nPromotion
		);
}

void parse_input_dividefast(const char* s, Engine* pE) 
{
	if (s == NULL)
		return;

	int depth;
	depth = max(2, atoi(s));

	ChessMove MoveList[MOVELIST_SIZE];
	GenerateMoves(pE->CurrentPosition, MoveList);
	ChessMove* pM = MoveList;
	ChessPosition Q;
	__int64 GrandTotal = 0i64;
	START_TIMER();
	while (pM->NoMoreMoves == 0)
	{
		Q = pE->CurrentPosition;
		Q.PerformMove(*pM).SwitchSides();
		
		DumpMove(*pM, LongAlgebraicNoNewline);
		__int64 nNumPositions = 0i64;
		PerftFastMT(Q, depth-1, nNumPositions);
		printf_s("Perft %d: %I64d \n",
			depth-1, nNumPositions
			);
		GrandTotal += nNumPositions;
		pM++;
	}
	printf_s("\nPerft %d: %I64d\n",depth, GrandTotal);
	STOP_TIMER();
}

void parse_input_writehash(const char* s, Engine* pE){}

void parse_input_lookuphash(const char* s, Engine* pE)
{
#ifdef _USE_HASH
#endif
}

void parse_input_memory(const char* s, Engine* pE) {
	unsigned __int64 BytesRequested = _atoi64(s);
	if (s == NULL)
		return; 

	SetMemory(BytesRequested);
}
void parse_input_cores(const char* s, Engine* pE) {
	
	if (s == NULL)
		return;

	pE->nNumCores = max(1, min(atoi(s), MAX_THREADS));
}
void parse_input_egtpath(const char* s, Engine* pE) {}
void parse_input_option(const char* s, Engine* pE) {}

// ---------------- Output Functions -------------------------------
void  send_output_feature(Engine* pE)
{
	char t[4096],s[1024];
	
	sprintf_s(t,"feature ");
	sprintf_s(s, "done=0 "); strcat(t, s);
	//
	sprintf_s(s, "\nfeature "); strcat(t, s);
	sprintf_s(s,"ping=%d ",IsImplemented("ping",pE)? 1:0); strcat(t,s);					// recommended:	1	
	sprintf_s(s,"setboard=%d ",IsImplemented("setboard",pE)? 1:0); strcat(t,s);			// recommended: 1
	sprintf_s(s,"playother=%d ",IsImplemented("playother",pE)? 1:0); strcat(t,s);		// recommended: 1
	sprintf_s(s,"san=0 "); strcat(t,s);													// no recommendation
	sprintf_s(s,"usermove=%d ",IsImplemented("usermove",pE)? 1:0); strcat(t,s);			// no recommendation
	sprintf_s(s,"time=%d ",IsImplemented("time",pE)? 1:0); strcat(t,s);					// recommended: 1	
	sprintf_s(s,"draw=%d ",IsImplemented("draw",pE)? 1:0); strcat(t,s);					// recommended: 1
	//
	sprintf_s(s,"\nfeature "); strcat(t,s);						
	sprintf_s(s,"sigint=0 "); strcat(t,s);												// recommended: 1
	sprintf_s(s,"sigterm=0 "); strcat(t,s);												// recommended: 1
	sprintf_s(s,"reuse=1 "); strcat(t,s);												// recommended: 1
	sprintf_s(s,"analyze=%d ",IsImplemented("analyze",pE)? 1:0); strcat(t,s);			// recommended: 1
	sprintf_s(s,"myname=\"JuddChess v1.0\" "); strcat(t,s);								// no recommendation
	//
	sprintf_s(s,"\nfeature "); strcat(t,s);												
	sprintf_s(s,"variants=\"normal\" "); strcat(t,s);									// recommended: "normal"
	sprintf_s(s,"colors=0 "); strcat(t,s);												// recommended: 0
	sprintf_s(s,"ics=0 "); strcat(t,s);													// no recommendation
	sprintf_s(s,"name=%d ",IsImplemented("name",pE)? 1:0); strcat(t,s);					
	sprintf_s(s,"pause=%d ",IsImplemented("pause",pE)? 1:0); strcat(t,s);
	//
	sprintf_s(s, "\nfeature "); strcat(t, s);											
	sprintf_s(s, "nps=%d ", IsImplemented("nps", pE) ? 1 : 0); strcat(t, s);
	sprintf_s(s, "debug=0 "); strcat(t, s);
	sprintf_s(s, "memory=%d ", IsImplemented("memory", pE) ? 1 : 0); strcat(t, s);
	sprintf_s(s, "smp=%d ", IsImplemented("cores", pE) ? 1 : 0); strcat(t, s);
	sprintf_s(s, "smp=0 "); strcat(t, s);	
	sprintf(s, "\nfeature done=1 "); strcat(t, s);
	sprintf_s(s,"\n"); strcat(t,s);
	WinBoardOutput(t);
}
void  send_output_illegalmove(const char* s,Engine* pE){}
void  send_output_error(const char* s,Engine* pE){}
void  send_output_move(Move M){}
void  send_output_hint(Move M){}
void  send_output_result(const char* s,Engine* pE){}
void  send_output_resign(Engine* pE){}
void  send_output_offerdraw(Engine* pE){}
void  send_output_tellopponent(const char* s,Engine* pE){}
void  send_output_tellothers(const char* s,Engine* pE){}
void  send_output_tellall(const char* s,Engine* pE){}
void  send_output_telluser(const char* s,Engine* pE){}
void  send_output_tellusererror(const char* s,Engine* pE){}
void  send_output_askuser(char* r, const char* s,Engine* pE){}
void  send_output_tellics(const char* s,Engine* pE){}
void  send_output_tellicsnoalias(const char* s,Engine* pE){}

void WinBoardOutput(const char* s)
{
	printf_s(s);
	LogOutput(logfile,s);
}

void LogInput(std::ofstream& logfile, const char* s)
{
	if(logfile)
		logfile << "Received command: " << s << std::endl;
}

void LogOutput(std::ofstream& logfile,const char* s)
{
	if(logfile)
		logfile << "Sent command: " << s << std::endl;
}

void SendReplyMove(const char* s,Engine* pE){}	


void SendReplyMoveAndPonder(const char* s, Engine* pE) {}