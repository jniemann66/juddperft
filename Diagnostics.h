#ifndef _DIAGNOSTICS_H
#define _DIAGNOSTICS_H 1
#include "movegen.h"

#define INCLUDE_DIAGNOSTICS 1
#define PERFTVALIDATE_TRUE 1
#define PERFTVALIDATE_FALSE 0
#define PERFTVALIDATE_FAILEDTOSTART -1

#ifdef INCLUDE_DIAGNOSTICS
// PerftValidatorPath[]: 
// Defines Path to external perft validation Program 
// (which is expected to take 3 arguments: <FEN String> <depth> <value>)
const char PerftValidatorPath[] = "c:\\bin\\PerftValidate.exe"; 

// PerftValidateWithExternal() - validates perft calculation against external engine
int PerftValidateWithExternal(const char* const pzFENString, int depth, __int64 value);
void FindPerftBug(const ChessPosition* pP, int depth);
void DumpPerftScoreFfromFEN(const char* pzFENstring, unsigned int depth, unsigned __int64 correctAnswer);

#endif // INCLUDE_DIAGNOSTICS
#endif //  _DIAGNOSTICS_H

