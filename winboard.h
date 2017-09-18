// Winboard.h
// Winboard Protocol Interface

#ifndef _WINBOARD_H
#define _WINBOARD_H 1

#define _CRT_SECURE_NO_WARNINGS 1

#include "engine.h"
#include <fstream>

#ifndef MSC_VER
#include <strings.h>
#define _stricmp strcasecmp
#include <stdlib.h>
#define _atoi64 atoll
#endif

typedef struct WinboardInputCommandDefinition{
	char* pzCommandString;
	void (*pF)(const char*,Engine* pE);		// pointer to handler function. NULL if not implemented
	bool implemented;						// set to true if command is implemented
} WINBOARD_INPUT_COMMAND_DEFINITION;

//typedef struct ThinkInfo
//{
//	ChessPosition P;
//	Variation* pPV;
//	unsigned int maxdepth;
//} THINKINFO;

// Winboard Input/Output ///////////////
// WinBoardOutput
// -- all Winboard output should be via this function !! --
void WinBoardOutput(const char* s);

// functions for dumping output to log file
void LogInput(std::ofstream& logfile,const char* s);
void LogOutput(std::ofstream& logfile,const char* s);

// functions for parsing input commands
void parse_input_xboard(const char* s,Engine* pE);
void parse_input_protover(const char* s,Engine* pE);		
void parse_input_accepted(const char* s,Engine* pE);
void parse_input_rejected(const char* s,Engine* pE);
void parse_input_new(const char* s,Engine* pE); 
void parse_input_variant(const char* s,Engine* pE);			
void parse_input_quit(const char* s,Engine* pE); 
void parse_input_random(const char* s,Engine* pE); 
void parse_input_force(const char* s,Engine* pE); 
void parse_input_go(const char* s,Engine* pE); 
void parse_input_playother(const char* s,Engine* pE); 
void parse_input_white(const char* s,Engine* pE); 
void parse_input_black(const char* s,Engine* pE); 
void parse_input_level(const char* s,Engine* pE);				
void parse_input_st(const char* s,Engine* pE);					
void parse_input_sd(const char* s,Engine* pE);
void parse_input_nps(const char* s, Engine* pE);
void parse_input_time(const char* s,Engine* pE);				
void parse_input_otim(const char* s,Engine* pE); 
void parse_input_move(const char* s,Engine* pE); 
void parse_input_usermove(const char* s,Engine* const pE);			
void parse_input_movenow(const char* s,Engine* pE); 
void parse_input_ping(const char* s,Engine* pE);				
void parse_input_draw(const char* s,Engine* pE); 
void parse_input_result(const char* s,Engine* pE);				
void parse_input_setboard(const char* s,Engine* pE);			
void parse_input_edit(const char* s,Engine* pE); 
void parse_input_hint(const char* s,Engine* pE); 
void parse_input_bk(const char* s,Engine* pE); 
void parse_input_undo(const char* s,Engine* pE); 
void parse_input_remove(const char* s,Engine* pE); 
void parse_input_hard(const char* s,Engine* pE); 
void parse_input_easy(const char* s,Engine* pE); 
void parse_input_post(const char* s,Engine* pE); 
void parse_input_nopost(const char* s,Engine* pE);
void parse_input_analyze(const char* s,Engine* pE); 
void parse_input_name(const char* s,Engine* pE);				
void parse_input_rating(const char* s,Engine* pE); 
void parse_input_ics(const char* s,Engine* pE);				
void parse_input_computer(const char* s,Engine* pE); 
void parse_input_pause(const char* s,Engine* pE); 
void parse_input_resume(const char* s,Engine* pE);
void parse_input_memory(const char* s, Engine* pE);
void parse_input_cores(const char* s, Engine* pE);
void parse_input_egtpath(const char* s, Engine* pE);
void parse_input_option(const char* s, Engine* pE);

// extended input commands (not part of winboard protocol)
void parse_input_movelist(const char* s,Engine* pE);
void parse_input_showposition(const char * s, Engine * pE);
void parse_input_showhash(const char* s,Engine* pE);
void parse_input_perft(const char* s,Engine* pE);
void parse_input_perftfast(const char * s, Engine * pE);
void parse_input_divide(const char* s, Engine* pE);
void parse_input_dividefast(const char * s, Engine * pE);
void parse_input_writehash(const char* s, Engine* pE);
void parse_input_lookuphash(const char* s, Engine* pE);

// functions for sending output commands
void send_output_feature(Engine* pE);
void send_output_illegalmove(const char* s,Engine* pE);
void send_output_error(const char* s,Engine* pE);
void send_output_move(Move M);
void send_output_hint(Move M);
void send_output_result(const char* s,Engine* pE);
void send_output_resign(Engine* pE);
void send_output_offerdraw(Engine* pE);
void send_output_tellopponent(const char* s,Engine* pE);
void send_output_tellothers(const char* s,Engine* pE);
void send_output_tellall(const char* s,Engine* pE);
void send_output_telluser(const char* s,Engine* pE);
void send_output_tellusererror(const char* s,Engine* pE);
void send_output_askuser(char* r, const char* s,Engine* pE);
void send_output_tellics(const char* s,Engine* pE);
void send_output_tellicsnoalias(const char* s,Engine* pE);
//
int WinBoard(Engine* pE);
bool WaitForInput(Engine* pE);
bool IsImplemented(const char* s,Engine* pE);
void SendReplyMove(const char* s,Engine* pE);
void SendReplyMoveAndPonder(const char * s, Engine * pE);
#endif
