// evaluate.h
#ifndef _EVALUATE_H
#define _EVALUATE_H 1
#include "movegen.h"

typedef int SCORE;

SCORE Evaluate(const ChessPosition* pP);
SCORE AssessDevelopment( const ChessPosition& rP);
SCORE EvaluateMobility(const ChessPosition & P);
SCORE EvaluateVulnerabilities(const ChessPosition & P);
SCORE EvaluatePawnStructure(const ChessPosition & P);

// BON=bonus, PEN=penalty, THR=Threshold

#define MOBILITY_AVAILABLE_SQUARE_VALUE 2

#define BON_INDIVIDUAL_PIECE_DEVELOPMENT 10
#define PEN_PREMATURE_QUEEN_DEVELOPMENT 12
#define PEN_KNIGHT_ON_RIM 5
#define THR_QUEEN_READINESS_THRESHOLD 40
#define BON_KNIGHT_OUTPOST 19
#define BON_PAWN_IN_CENTRE 33

#define BON_CASTLING 20

//(0)		0	0	0	0	Empty Square
//(1)		0	0	0	1	White Pawn
//(2)		0	0	1	0	White Bishop
//(3)		0	0	1	1	White En passant Square
//(4)		0	1	0	0	White Rook
//(5)		0	1	0	1	White Knight
//(6)		0	1	1	0	White Queen
//(7)		0	1	1	1	White King
//(8)		1	0	0	0	Reserved - Don't use (Black empty square)
//(9)		1	0	0	1	Black Pawn
//(10)	1	0	1	0	Black Bishop
//(11)	1	0	1	1	Black En passant Square
//(12)	1	1	0	0	Black Rook
//(13)	1	1	0	1	Black Knight
//(14)	1	1	1	0	Black Queen
//(15)	1	1	1	1	Black King

const SCORE PassedPawnValue[8] = { 0,51,34,23,15,10,10,0 }; // in order of distance-to-queening
const SCORE MobilityBonus[16][28] = {
	{},
	{},
	{ -21, -2,	9,	17, 23, 28, 32, 36,	39,	42,	45,	47,	49,	52 },
	{},
	{ -25, -10, -2,	4,	9,	13,	16,	19,	22,	24,	26,	28,	29,	31,	33 },
	{ -35, -17, -7,	1,	6,	11,	15,	18,	21 },
	{ -17,	0,	11,	18,	24,	28,	32,	35,	38,	41,	43,	46,	48,	50,	51,	53,	54,	56,	57,	59,	60,	61,	62 },
	{},
	{},
	{},
	{ 21, 2,-9,-17,-23,-28,-32,-36,-39,-42,-45,-47,-49,-52},
	{},
	{ 25,10,2,-4,-9,-13,-16,-19,-22,-24,-26,-28,-29,-31,-33},
	{ 35,17,7,-1,-6,-11,-15,-18,-21 },
	{ 17,0,-11,-18,-24,-28,-32,-35,-38,-41,-43,-46,-48,-50,-51,-53,-54,-56,-57,-59,-60,-61,-62 },
	{}
};

#endif // _EVALUATE_H


