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

// Winboard.h : defines a Winboard Protocol Interface

#ifndef _WINBOARD_H
#define _WINBOARD_H 1

#define _CRT_SECURE_NO_WARNINGS 1

#include "engine.h"
#include <fstream>

#ifndef _MSC_VER
#include <strings.h>
#define _stricmp strcasecmp
#include <stdlib.h>
#define _atoi64 atoll
#endif

namespace juddperft {

struct WinboardInputCommandDefinition {
	WinboardInputCommandDefinition(const char*commandString, void(*pF)(const char*, Engine*), bool isImplemented)
		: pzCommandString(nullptr), pF(pF), implemented(isImplemented) {
		if (commandString != nullptr) {
			pzCommandString = strdup(commandString);
		}
	};

	~WinboardInputCommandDefinition() {
		free(pzCommandString);
	}

	char* pzCommandString;
	void(*pF)(const char*, Engine*);		// pointer to handler function. NULL if not implemented
	bool implemented;						// set to true if command is implemented
};

//typedef struct ThinkInfo
//{
//	ChessPosition P;
//	Variation* pPV;
//	unsigned int maxdepth;
//} THINKINFO;

// Winboard Input/Output ///////////////
// winBoardOutput
// -- all Winboard output should be via this function !! --
void winBoardOutput(const char* s);

// functions for dumping output to log file
void logInput(std::ofstream& logfile, const char* s);
void logOutput(std::ofstream& logfile, const char* s);

// functions for parsing input commands
void parse_input_xboard(const char* s, Engine* pE);
void parse_input_protover(const char* s, Engine* pE);
void parse_input_accepted(const char* s, Engine* pE);
void parse_input_rejected(const char* s, Engine* pE);
void parse_input_new(const char* s, Engine* pE);
void parse_input_variant(const char* s, Engine* pE);
void parse_input_quit(const char* s, Engine* pE);
void parse_input_random(const char* s, Engine* pE);
void parse_input_force(const char* s, Engine* pE);
void parse_input_go(const char* s, Engine* pE);
void parse_input_playother(const char* s, Engine* pE);
void parse_input_white(const char* s, Engine* pE);
void parse_input_black(const char* s, Engine* pE);
void parse_input_level(const char* s, Engine* pE);
void parse_input_st(const char* s, Engine* pE);
void parse_input_sd(const char* s, Engine* pE);
void parse_input_nps(const char* s, Engine* pE);
void parse_input_time(const char* s, Engine* pE);
void parse_input_otim(const char* s, Engine* pE);
void parse_input_move(const char* s, Engine* pE);
void parse_input_usermove(const char* s, Engine* const pE);
void parse_input_movenow(const char* s, Engine* pE);
void parse_input_ping(const char* s, Engine* pE);
void parse_input_draw(const char* s, Engine* pE);
void parse_input_result(const char* s, Engine* pE);
void parse_input_setboard(const char* s, Engine* pE);
void parse_input_edit(const char* s, Engine* pE);
void parse_input_hint(const char* s, Engine* pE);
void parse_input_bk(const char* s, Engine* pE);
void parse_input_undo(const char* s, Engine* pE);
void parse_input_remove(const char* s, Engine* pE);
void parse_input_hard(const char* s, Engine* pE);
void parse_input_easy(const char* s, Engine* pE);
void parse_input_post(const char* s, Engine* pE);
void parse_input_nopost(const char* s, Engine* pE);
void parse_input_analyze(const char* s, Engine* pE);
void parse_input_name(const char* s, Engine* pE);
void parse_input_rating(const char* s, Engine* pE);
void parse_input_ics(const char* s, Engine* pE);
void parse_input_computer(const char* s, Engine* pE);
void parse_input_pause(const char* s, Engine* pE);
void parse_input_resume(const char* s, Engine* pE);
void parse_input_memory(const char* s, Engine* pE);
void parse_input_cores(const char* s, Engine* pE);
void parse_input_egtpath(const char* s, Engine* pE);
void parse_input_option(const char* s, Engine* pE);

// extended input commands (not part of winboard protocol)
void parse_input_movelist(const char* s, Engine* pE);
void parse_input_showposition(const char * s, Engine * pE);
void parse_input_showhash(const char* s, Engine* pE);
void parse_input_perft(const char* s, Engine* pE);
void parse_input_perftfast(const char * s, Engine * pE);
void parse_input_divide(const char* s, Engine* pE);
void parse_input_dividefast(const char * s, Engine * pE);
void parse_input_writehash(const char* s, Engine* pE);
void parse_input_lookuphash(const char* s, Engine* pE);
void parse_input_testExternal(const char * s, Engine * pE);

// functions for sending output commands
void send_output_feature(Engine* pE);
void send_output_illegalmove(const char* s, Engine* pE);
void send_output_error(const char* s, Engine* pE);
void send_output_move(const ChessMove& M);
void send_output_hint(const ChessMove& M);
void send_output_result(const char* s, Engine* pE);
void send_output_resign(Engine* pE);
void send_output_offerdraw(Engine* pE);
void send_output_tellopponent(const char* s, Engine* pE);
void send_output_tellothers(const char* s, Engine* pE);
void send_output_tellall(const char* s, Engine* pE);
void send_output_telluser(const char* s, Engine* pE);
void send_output_tellusererror(const char* s, Engine* pE);
void send_output_askuser(char* r, const char* s, Engine* pE);
void send_output_tellics(const char* s, Engine* pE);
void send_output_tellicsnoalias(const char* s, Engine* pE);
//
int winBoard(Engine* pE);
bool waitForInput(Engine* pE);
bool isImplemented(const char* s, Engine* pE);
void sendReplyMove(const char* s, Engine* pE);
void sendReplyMoveAndPonder(const char * s, Engine * pE);

} // namespace juddperft
#endif // _WINBOARD_H
