// fen.h
// Header file for FEN-manipulation 
// Routines.

#ifndef __FEN_H
#define __FEN_H 1

#include "movegen.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool ReadFen(ChessPosition* pP, const char* pzFENString);
bool WriteFen(char* pzFENBuffer, const ChessPosition* pP);

#endif
