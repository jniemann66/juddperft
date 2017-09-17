#ifndef _SEARCH_H
#define _SEARCH_H 1

//#include <windows.h>

#include "movegen.h"
#include "hashtable.h"

#define PV_SIZE 64
#define MAX_THREADS 64 // Hard limit for number of threads to use
#define ZERO_64 0

#define SEARCH_STOPPED 0x80000000

struct PerftInfo{
	int64_t nMoves;
	int64_t nCapture;
	int64_t nEPCapture;
	int64_t nCastle;
	int64_t nCastleLong;
	int64_t nPromotion;
};

int64_t Perft(ChessPosition P, int maxdepth, int depth, PerftInfo* pI);		// Single-Threaded		
void PerftFast(const ChessPosition & P, int depth, int64_t & nNodes);		// Simple, Hash-Table-using perft
void PerftFastIterative(const ChessPosition & P, int depth, int64_t & nNodes);	// Iterative version - Hash Table functionality still broken
void PerftMT(ChessPosition P, int maxdepth, int depth, PerftInfo* pI);		// Multi-Threaded driver for Perft()
void PerftFastMT(ChessPosition P, int depth, int64_t & nNodes);				// Multi-Threaded driver for PerftFast()

#endif

#ifndef NULL
#define NULL 0
#endif