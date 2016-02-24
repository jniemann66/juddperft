#ifndef _SEARCH_H
#define _SEARCH_H 1

#include <windows.h>

#include "movegen.h"
#include "hashtable.h"

#define PV_SIZE 64
#define MAX_THREADS 64 // Hard limit for number of threads to use

#define SEARCH_STOPPED 0x80000000

struct PerftInfo{
	__int64 nMoves;
	__int64 nCapture;
	__int64 nEPCapture;
	__int64 nCastle;
	__int64 nCastleLong;
	__int64 nPromotion;
};

void Perft(ChessPosition P, int maxdepth, int depth, PerftInfo* pI);		// Single-Threaded		
void PerftFast2(const ChessPosition P, int depth, __int64 * p_numNodes);	// Wrapper for PerftFast()- takes pointer instead of reference
void PerftFast(const ChessPosition & P, int depth, __int64 & nNodes);		// Simple, Hash-Table-using perft
void PerftFastIterative(const ChessPosition & P, int depth, __int64 & nNodes);
void PerftMT(ChessPosition P, int maxdepth, int depth, PerftInfo* pI);		// Multi-Threaded driver for Perft()
void PerftFastMT(ChessPosition P, int depth, __int64 & nNodes);				// Multi-Threaded driver for PerftFast()

#endif

#ifndef NULL
#define NULL 0
#endif