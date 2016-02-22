// Evaluate.cpp -  static evaluation of position
#include "evaluate.h"
#include <intrin.h>
#include <cstdio>

// Note: regarding the return value of Evaluate() ...
// In order for NegaMax algorithms (such as negascout) to work, 
// it is important for Evaluate() to return the score relative to the side being evaluated. ie:
// If white is to move: <Good for white> -> +ve , <Bad for White> -> -ve
// If Black is to move: <Good for Black> -> +ve , <bad for Black> -> -ve
//

SCORE Evaluate(const ChessPosition* pP)
{
	SCORE evaluation;
	SCORE material;
	SCORE positional=0;
	SCORE development=0;

	const BitBoard& A=pP->A;
	const BitBoard& B=pP->B;
	const BitBoard& C=pP->C;
	const BitBoard& D=pP->D;

	// Test for Checks
	bool colour = pP->BlackToMove;
	if (IsInCheck(*pP,!colour))
	{ 
		// Someone is in check : Test available moves
		Move MoveList[MOVELIST_SIZE];
		Move* pM = MoveList;
		GenerateMoves(*pP, pM);
		if (pM->NoMoreMoves)
			return CHECKMATE; // Checkmate		
	}
	
	// evaluate material:
	material=pP->material; // 
	
	// occupation of centre :
	if((A & ~B & ~C & D) & UPPER_CENTRE)
		positional -= BON_PAWN_IN_CENTRE;
	if((A & ~B & ~C & ~D) & LOWER_CENTRE)
		positional += BON_PAWN_IN_CENTRE;
	
	// Control of centre :
	BitBoard BlackAttacks = GenBlackAttacks(*pP);
	BitBoard WhiteAttacks = GenWhiteAttacks(*pP);
	//
	if (BlackAttacks & D4)
		positional -= 15;
	if (BlackAttacks & E4)
		positional -= 15;
	if (BlackAttacks & D5)
		positional -= 10;
	if (BlackAttacks & E5)
		positional -= 10;
	//
	if (WhiteAttacks & D4)
		positional += 10;
	if (WhiteAttacks & E4)
		positional += 10;
	if (WhiteAttacks & D5)
		positional += 15;
	if (WhiteAttacks & E5)
		positional += 15;

	// Knight Evaluation 
	BitBoard N = A & ~B & C; // Board containing all knights
	// "A Knight on the rim is grim !" :
	if(N & D & RIM)
		positional += PEN_KNIGHT_ON_RIM;
	if(N &~D & RIM)
		positional += PEN_KNIGHT_ON_RIM;
	
	// Black Knight Outpost :
	// ( black pawn -> DL1) && (black knight) +20
	
	if (N & D & BLACKOUTPOSTZONE & MoveDownLeftRightSingle(A & ~B & ~C &D))
		positional -= BON_KNIGHT_OUTPOST;

	//if(MoveDownLeftSingleOccluded((A & ~B & ~C & D),(N & D)))
	//	positional -= BON_KNIGHT_OUTPOST;
	//if(MoveDownRightSingleOccluded((A & ~B & ~C & D),(N & D)))
	//	positional -= BON_KNIGHT_OUTPOST;
	
	// white knight outpost
	if (N & ~D & WHITEOUTPOSTZONE & MoveUpLeftRightSingle(A & ~B & ~C &~D))
		positional += BON_KNIGHT_OUTPOST;

	//if(MoveUpRightSingleOccluded((A & ~B & ~C & ~D),(N & ~D)))
	//	positional += BON_KNIGHT_OUTPOST;
	//if(MoveUpLeftSingleOccluded((A & ~B & ~C & ~D),(N & ~D)))
	//	positional += BON_KNIGHT_OUTPOST;
	
	// King Safety
	// award points for each pawn in front of king
	if(MoveDownLeftSingleOccluded((A & B & C & D),(A & ~B & ~C & D)))
		positional -= 23;
	if(MoveDownSingleOccluded((A & B & C & D),(A & ~B & ~C & D)))
		positional -= 23;
	if(MoveDownRightSingleOccluded((A & B & C & D),(A & ~B & ~C & D)))
		positional -= 23;
	//
	if(MoveUpLeftSingleOccluded((A & B & C & ~D),(A & ~B & ~C & ~D)))
		positional += 23;
	if(MoveUpSingleOccluded((A & B & C & ~D),(A & ~B & ~C & ~D)))
		positional += 23;
	if(MoveUpRightSingleOccluded((A & B & C & ~D),(A & ~B & ~C & ~D)))
		positional += 23;

	// Penalize loss of castling
		if (pP->BlackForfeitedCastle || pP->BlackForfeitedCastleLong)
			positional += BON_CASTLING;
		if (pP->WhiteForfeitedCastle || pP->WhiteForfeitedCastleLong)
			positional -= BON_CASTLING;

	// give bonus for castling
		if (pP->BlackDidCastle==1)
			positional -= BON_CASTLING;
		if (pP->BlackDidCastleLong==1)
			positional -= BON_CASTLING;
		// give bonus for castling
		if (pP->WhiteDidCastle==1)
			positional += BON_CASTLING;
		if (pP->WhiteDidCastleLong==1)
			positional += BON_CASTLING;

	// Assess development until someone has castled
	if((pP->BlackCanCastle || pP->BlackCanCastleLong) || (pP->WhiteCanCastle || pP->WhiteCanCastleLong))
	{

		development=AssessDevelopment(*pP);

		// avoid moving queen if development is poor
		if (development >= THR_QUEEN_READINESS_THRESHOLD)	    // meaning: "if black is to far behind in development to move queen"
		{
			if((~A & B & C & D) & ~BLACKQUEENPOS)	// meaning: "if black queen is not in its orig position"
				positional += PEN_PREMATURE_QUEEN_DEVELOPMENT;	// favour white
		}

		if (development <= -THR_QUEEN_READINESS_THRESHOLD)	// meaning: "if white is to far behind in development to move queen"
		{
			if((~A & B & C & ~D) & ~WHITEQUEENPOS)	// meaning: "if white queen is not in its orig position"
				positional -= PEN_PREMATURE_QUEEN_DEVELOPMENT;	// favour black
		}
	
	}
		
	positional += development + EvaluatePawnStructure(*pP);
	evaluation = EvaluateMobility(*pP);
	evaluation = material + positional;
	//evaluation += EvaluatePawnStructure(*pP);

	
	//+EvaluateMobility(*pP);
	//+EvaluateVulnerabilities(*pP);

	// Important: Return score relative to side to play:
	return (pP->BlackToMove)? -evaluation : evaluation ;
}

inline SCORE AssessDevelopment( const ChessPosition& rP)
{
	SCORE development=0;
	const SCORE dev_value = BON_INDIVIDUAL_PIECE_DEVELOPMENT;
	
	const BitBoard& A=rP.A;
	const BitBoard& B=rP.B;
	const BitBoard& C=rP.C;
	const BitBoard& D=rP.D;
	
	// count number of white pieces still in orig position
	if((~A & ~B & C & ~D) & WHITEQRPOS)
		development -= dev_value;
	if((A & ~B & C & ~D) & WHITEQNPOS)
		development -= dev_value;
	if((~A & B & ~C & ~D) & WHITEQBPOS)
		development -= dev_value;
	if((~A & B & ~C & ~D) & WHITEKBPOS)
		development -= dev_value;
	if((A & ~B & C & ~D) & WHITEKNPOS)
		development -= dev_value;
	if((~A & ~B & C & ~D) & WHITEKRPOS)
		development -= dev_value;
		
	// count number of black pieces still in orig position
	if((~A & ~B & C & D) & BLACKQRPOS)
		development += dev_value;
	if((A & ~B & C & D) & BLACKQNPOS)
		development += dev_value;
	if((~A & B & ~C & D) & BLACKQBPOS)
		development += dev_value;
	if((~A & B & ~C & D) & BLACKKBPOS)
		development += dev_value;
	if((A & ~B & C & D) & BLACKKNPOS)
		development += dev_value;
	if((~A & ~B & C & D) & BLACKKRPOS)
		development += dev_value;
	
	return development;
}

SCORE EvaluateMobility(const ChessPosition& P)
{
	BitBoard Occupied = P.A | P.B | P.C;
	BitBoard AvailBBishop1Squares = 0i64;
	BitBoard AvailBBishop2Squares = 0i64;
	BitBoard AvailWBishop1Squares = 0i64;
	BitBoard AvailWBishop2Squares = 0i64;
	BitBoard AvailBKnight1Squares = 0i64;
	BitBoard AvailBKnight2Squares = 0i64;
	BitBoard AvailWKnight1Squares = 0i64;
	BitBoard AvailWKnight2Squares = 0i64;
	BitBoard AvailBRook1Squares = 0i64;
	BitBoard AvailBRook2Squares = 0i64;
	BitBoard AvailWRook1Squares = 0i64;
	BitBoard AvailWRook2Squares = 0i64;
	BitBoard AvailBQueenSquares = 0i64;
	BitBoard AvailWQueenSquares = 0i64;

	BitBoard Empty = ~Occupied |			// All Unoccupied squares
		(P.A & P.B & ~P.C);					// All EP squares, regardless of colour

	BitBoard PotentialCapturesForBlack = Occupied & ~P.D & ~(P.A & P.B & P.C);	// White Pieces except Kings
	BitBoard PotentialCapturesForWhite = Occupied & P.D & ~(P.A & P.B & P.C);	// Black Pieces except Kings

	// Find relevant pieces:
	// Bishops - paths don't overlap (unless more than 2 on board); calculated simultaneously:
	BitBoard BlackBishop1;
	BitBoard BlackBishop2;
	GetFirstAndLastPiece(~P.A & P.B & ~P.C & P.D, BlackBishop1, BlackBishop2);

	BitBoard WhiteBishop1;
	BitBoard WhiteBishop2;
	GetFirstAndLastPiece(~P.A & P.B & ~P.C & ~P.D, WhiteBishop1, WhiteBishop2);
	
	// Knights
	BitBoard BlackKnight1;
	BitBoard BlackKnight2;
	GetFirstAndLastPiece(P.A & ~P.B & P.C & P.D, BlackKnight1, BlackKnight2);

	BitBoard WhiteKnight1;
	BitBoard WhiteKnight2;
	GetFirstAndLastPiece(P.A & ~P.B & P.C & ~P.D, WhiteKnight1, WhiteKnight2);

	// Rooks
	BitBoard BlackRook1;
	BitBoard BlackRook2;
	GetFirstAndLastPiece(~P.A & ~P.B & P.C & P.D, BlackRook1, BlackRook2);

	BitBoard WhiteRook1;
	BitBoard WhiteRook2;
	GetFirstAndLastPiece(~P.A & ~P.B & P.C & ~P.D, WhiteRook1, WhiteRook2);

	// Queen (assume 1; there may be more than 1 on the board, though !)
	BitBoard BlackQueen;
	BitBoard WhiteQueen;
	BlackQueen = ~P.A & P.B & P.C & P.D;
	WhiteQueen = ~P.A & P.B & P.C & ~P.D;
	//

	// Find Available Squares for each piece:
	AvailBBishop1Squares = MoveUpRightSingleOccluded(FillUpRightOccluded(BlackBishop1, Empty), Empty | PotentialCapturesForBlack);
	AvailBBishop1Squares |= MoveDownRightSingleOccluded(FillDownRightOccluded(BlackBishop1, Empty), Empty | PotentialCapturesForBlack);
	AvailBBishop1Squares |= MoveDownLeftSingleOccluded(FillDownLeftOccluded(BlackBishop1, Empty), Empty | PotentialCapturesForBlack);
	AvailBBishop1Squares |= MoveUpLeftSingleOccluded(FillUpLeftOccluded(BlackBishop1, Empty), Empty | PotentialCapturesForBlack);

	if (BlackBishop2 != BlackBishop1) {
		AvailBBishop2Squares = MoveUpRightSingleOccluded(FillUpRightOccluded(BlackBishop2, Empty), Empty | PotentialCapturesForBlack);
		AvailBBishop2Squares |= MoveDownRightSingleOccluded(FillDownRightOccluded(BlackBishop2, Empty), Empty | PotentialCapturesForBlack);
		AvailBBishop2Squares |= MoveDownLeftSingleOccluded(FillDownLeftOccluded(BlackBishop2, Empty), Empty | PotentialCapturesForBlack);
		AvailBBishop2Squares |= MoveUpLeftSingleOccluded(FillUpLeftOccluded(BlackBishop2, Empty), Empty | PotentialCapturesForBlack);
	}

	AvailWBishop1Squares = MoveUpRightSingleOccluded(FillUpRightOccluded(WhiteBishop1, Empty), Empty | PotentialCapturesForWhite);
	AvailWBishop1Squares |= MoveDownRightSingleOccluded(FillDownRightOccluded(WhiteBishop1, Empty), Empty | PotentialCapturesForWhite);
	AvailWBishop1Squares |= MoveDownLeftSingleOccluded(FillDownLeftOccluded(WhiteBishop1, Empty), Empty | PotentialCapturesForWhite);
	AvailWBishop1Squares |= MoveUpLeftSingleOccluded(FillUpLeftOccluded(WhiteBishop1, Empty), Empty | PotentialCapturesForWhite);

	if (WhiteBishop2 != WhiteBishop1) {
		AvailWBishop2Squares = MoveUpRightSingleOccluded(FillUpRightOccluded(WhiteBishop2, Empty), Empty | PotentialCapturesForWhite);
		AvailWBishop2Squares |= MoveDownRightSingleOccluded(FillDownRightOccluded(WhiteBishop2, Empty), Empty | PotentialCapturesForWhite);
		AvailWBishop2Squares |= MoveDownLeftSingleOccluded(FillDownLeftOccluded(WhiteBishop2, Empty), Empty | PotentialCapturesForWhite);
		AvailWBishop2Squares |= MoveUpLeftSingleOccluded(FillUpLeftOccluded(WhiteBishop2, Empty), Empty | PotentialCapturesForWhite);
	}

	AvailBKnight1Squares = FillKnightAttacksOccluded(BlackKnight1, (Empty | PotentialCapturesForBlack));

	if (BlackKnight2 != BlackKnight1) {
		AvailBKnight2Squares = FillKnightAttacksOccluded(BlackKnight2, (Empty | PotentialCapturesForBlack));
	}

	AvailWKnight1Squares = FillKnightAttacksOccluded(WhiteKnight1, (Empty | PotentialCapturesForWhite));

	if (WhiteKnight2 != WhiteKnight1) {
		AvailWKnight2Squares = FillKnightAttacksOccluded(WhiteKnight2, (Empty | PotentialCapturesForWhite));
	}

	// Rooks
	AvailBRook1Squares = MoveUpSingleOccluded(FillUpOccluded(BlackRook1, Empty), Empty | PotentialCapturesForBlack);
	AvailBRook1Squares |= MoveRightSingleOccluded(FillRightOccluded(BlackRook1, Empty), Empty | PotentialCapturesForBlack);
	AvailBRook1Squares |= MoveDownSingleOccluded(FillDownOccluded(BlackRook1, Empty), Empty | PotentialCapturesForBlack);
	AvailBRook1Squares |= MoveLeftSingleOccluded(FillLeftOccluded(BlackRook1, Empty), Empty | PotentialCapturesForBlack);


	if (BlackRook2 != BlackRook1) {
		AvailBRook2Squares = MoveUpSingleOccluded(FillUpOccluded(BlackRook2, Empty), Empty | PotentialCapturesForBlack);
		AvailBRook2Squares |= MoveRightSingleOccluded(FillRightOccluded(BlackRook2, Empty), Empty | PotentialCapturesForBlack);
		AvailBRook2Squares |= MoveDownSingleOccluded(FillDownOccluded(BlackRook2, Empty), Empty | PotentialCapturesForBlack);
		AvailBRook2Squares |= MoveLeftSingleOccluded(FillLeftOccluded(BlackRook2, Empty), Empty | PotentialCapturesForBlack);
	}

	AvailWRook1Squares = MoveUpSingleOccluded(FillUpOccluded(WhiteRook1, Empty), Empty | PotentialCapturesForWhite);
	AvailWRook1Squares |= MoveRightSingleOccluded(FillRightOccluded(WhiteRook1, Empty), Empty | PotentialCapturesForWhite);
	AvailWRook1Squares |= MoveDownSingleOccluded(FillDownOccluded(WhiteRook1, Empty), Empty | PotentialCapturesForWhite);
	AvailWRook1Squares |= MoveLeftSingleOccluded(FillLeftOccluded(WhiteRook1, Empty), Empty | PotentialCapturesForWhite);


	if (WhiteRook2 != WhiteRook1) {
		AvailWRook2Squares = MoveUpSingleOccluded(FillUpOccluded(WhiteRook2, Empty), Empty | PotentialCapturesForWhite);
		AvailWRook2Squares |= MoveRightSingleOccluded(FillRightOccluded(WhiteRook2, Empty), Empty | PotentialCapturesForWhite);
		AvailWRook2Squares |= MoveDownSingleOccluded(FillDownOccluded(WhiteRook2, Empty), Empty | PotentialCapturesForWhite);
		AvailWRook2Squares |= MoveLeftSingleOccluded(FillLeftOccluded(WhiteRook2, Empty), Empty | PotentialCapturesForWhite);
	}

	//if (BlackQueen) {
	//	AvailBQueenSquares = MoveUpRightSingleOccluded(FillUpRightOccluded(BlackQueen, Empty), Empty | PotentialCapturesForBlack);
	//	AvailBQueenSquares |= MoveDownRightSingleOccluded(FillDownRightOccluded(BlackQueen, Empty), Empty | PotentialCapturesForBlack);
	//	AvailBQueenSquares |= MoveDownLeftSingleOccluded(FillDownLeftOccluded(BlackQueen, Empty), Empty | PotentialCapturesForBlack);
	//	AvailBQueenSquares |= MoveUpLeftSingleOccluded(FillUpLeftOccluded(BlackQueen, Empty), Empty | PotentialCapturesForBlack);

	//	AvailBQueenSquares |= MoveUpSingleOccluded(FillUpOccluded(BlackQueen, Empty), Empty | PotentialCapturesForBlack);
	//	AvailBQueenSquares |= MoveRightSingleOccluded(FillRightOccluded(BlackQueen, Empty), Empty | PotentialCapturesForBlack);
	//	AvailBQueenSquares |= MoveDownSingleOccluded(FillDownOccluded(BlackQueen, Empty), Empty | PotentialCapturesForBlack);
	//	AvailBQueenSquares |= MoveLeftSingleOccluded(FillLeftOccluded(BlackQueen, Empty), Empty | PotentialCapturesForBlack);
	//}

	//if (WhiteQueen) {
	//	AvailWQueenSquares = MoveUpRightSingleOccluded(FillUpRightOccluded(WhiteQueen, Empty), Empty | PotentialCapturesForWhite);
	//	AvailWQueenSquares |= MoveDownRightSingleOccluded(FillDownRightOccluded(WhiteQueen, Empty), Empty | PotentialCapturesForWhite);
	//	AvailWQueenSquares |= MoveDownLeftSingleOccluded(FillDownLeftOccluded(WhiteQueen, Empty), Empty | PotentialCapturesForWhite);
	//	AvailWQueenSquares |= MoveUpLeftSingleOccluded(FillUpLeftOccluded(WhiteQueen, Empty), Empty | PotentialCapturesForWhite);

	//	AvailWQueenSquares |= MoveUpSingleOccluded(FillUpOccluded(WhiteQueen, Empty), Empty | PotentialCapturesForWhite);
	//	AvailWQueenSquares |= MoveRightSingleOccluded(FillRightOccluded(WhiteQueen, Empty), Empty | PotentialCapturesForWhite);
	//	AvailWQueenSquares |= MoveDownSingleOccluded(FillDownOccluded(WhiteQueen, Empty), Empty | PotentialCapturesForWhite);
	//	AvailWQueenSquares |= MoveLeftSingleOccluded(FillLeftOccluded(WhiteQueen, Empty), Empty | PotentialCapturesForWhite);
	//}

	// for debugging:

	//DumpBitBoard(BlackBishops);
	//DumpBitBoard(WhiteBishops);
	//DumpBitBoard(BlackKnight1);
	//DumpBitBoard(BlackKnight2);
	//DumpBitBoard(WhiteKnight1);
	//DumpBitBoard(WhiteKnight2);
	//DumpBitBoard(BlackRook1);
	//DumpBitBoard(BlackRook2);
	//DumpBitBoard(WhiteRook1);
	//DumpBitBoard(WhiteRook2);
	//DumpBitBoard(BlackQueen);
	//DumpBitBoard(WhiteQueen);


	/*DumpBitBoard(AvailBBishop1Squares);
	DumpBitBoard(AvailBBishop2Squares);
	DumpBitBoard(AvailWBishop1Squares);
	DumpBitBoard(AvailWBishop2Squares);
	DumpBitBoard(AvailBKnight1Squares);
	DumpBitBoard(AvailBKnight2Squares);
	DumpBitBoard(AvailWKnight1Squares);
	DumpBitBoard(AvailWKnight2Squares);
	DumpBitBoard(AvailBRook1Squares);
	DumpBitBoard(AvailBRook2Squares);
	DumpBitBoard(AvailWRook1Squares);
	DumpBitBoard(AvailWRook2Squares);
	DumpBitBoard(AvailBQueenSquares);
	DumpBitBoard(AvailWQueenSquares);*/

	//return /*MOBILITY_AVAILABLE_SQUARE_VALUE * */(
	//	-PopCount(AvailBBishopSquares)
	//	+ PopCount(AvailWBishopSquares)
	//	- PopCount(AvailBKnight1Squares)
	//	- PopCount(AvailBKnight2Squares)
	//	+ PopCount(AvailWKnight1Squares)
	//	+ PopCount(AvailWKnight2Squares)
	//	- PopCount(AvailBRook1Squares)
	//	- PopCount(AvailBRook2Squares)
	//	+ PopCount(AvailWRook1Squares)
	//	+ PopCount(AvailWRook2Squares)
	//	/*- PopCount(AvailBQueenSquares)
	//	+ PopCount(AvailWQueenSquares)*/
	
	SCORE bb1 = MobilityBonus[BBISHOP][PopCount(AvailBBishop1Squares)];
	SCORE bb2 = MobilityBonus[BBISHOP][PopCount(AvailBBishop2Squares)];
	SCORE wb1 = MobilityBonus[WBISHOP][PopCount(AvailWBishop1Squares)];
	SCORE wb2 = MobilityBonus[WBISHOP][PopCount(AvailWBishop2Squares)];
	SCORE bn1 = MobilityBonus[BKNIGHT][PopCount(AvailBKnight1Squares)];
	SCORE bn2 = MobilityBonus[BKNIGHT][PopCount(AvailBKnight2Squares)];
	SCORE wn1 = MobilityBonus[WKNIGHT][PopCount(AvailWKnight1Squares)];
	SCORE wn2 = MobilityBonus[WKNIGHT][PopCount(AvailWKnight2Squares)];
	SCORE br1 = MobilityBonus[BROOK][PopCount(AvailBRook1Squares)];
	SCORE br2 = MobilityBonus[BROOK][PopCount(AvailBRook2Squares)];
	SCORE wr1 = MobilityBonus[WROOK][PopCount(AvailWRook1Squares)];
	SCORE wr2 = MobilityBonus[WROOK][PopCount(AvailWRook2Squares)];
	return bb1 + bb2 + wb1 + wb2 + bn1 + bn2 + wn1 + wn2 + br1 + br2 + wr1 + wr2;
		/*- PopCount(AvailBQueenSquares)
		+ PopCount(AvailWQueenSquares)*/
}

void EvaluateSquareControl(const ChessPosition& P) {
	int Square[64];

}

SCORE EvaluateVulnerabilities (const ChessPosition& P){
	BitBoard BlackAttacks, WhiteAttacks, BlackFutureAttacks, WhiteFutureAttacks;
	
	BlackAttacks=GenBlackAttacks(P);
	// BlackFutureAttacks=GenFutureBlackAttacks();
	WhiteAttacks=GenWhiteAttacks(P);
	// WhiteFutureAttacks=GenFutureWhiteAttacks();
	int Piece;
	
	// debug:
	//printf_s("Threatened Black Pieces:\n");
	//DumpBitBoard((P.A | P.B | P.C) & P.D & WhiteAttacks);
	//printf_s("Threatened White Pieces:\n");
	//DumpBitBoard((P.A | P.B | P.C) & ~P.D & BlackAttacks);
	// end debug

	SCORE ThreatBalance = 0;
	for (int q = 0; q < 64; q++) {
		BitBoard Mask = 1i64 << q;
		Piece=P.GetPieceAtSquare(Mask);
		if ((Piece & 8) /*black*/) 
		{ // to-do: EP ??
			if (Mask & WhiteAttacks) {

				/*printf_s("Threatened Black Piece: %d, Square: %d, value: %d\n", Piece, q, PieceMaterialValue[Piece]);*/
				// Penalty for black
				ThreatBalance -= PieceMaterialValue[Piece]/10; // 1/8th of piece value ?
			}
			// is piece hanging or protected ? by what ??
			//if(PieceisHanging){
				// attackervalue > attackedvalue // bad
				// attackervalue == attackedvalue // bad
				// attackervaule < attackedvalue /bad
			//}
			//	else{
			// attackervalue > attackedvalue // good
			// attackervalue == attackedvalue // evaluateexchange()
			// attackervaule < attackedvalue // bad	
			//}
			//if (Mask & WhiteFutureAttacks) {
			//	// less severe Penalty for black
			//	// ThreatBalance += something /2 ;

			//}
			
			
		}
		else if (Piece & 7 /* white */){
			if (Mask & BlackAttacks) {
				/*printf_s("Threatened White Piece: %d, Square: %d, value: %d\n", Piece, q, PieceMaterialValue[Piece]);*/
				// Penalty for White
				ThreatBalance -= PieceMaterialValue[Piece]/10; // 1/8th of piece value ?
			}
			//if (Mask & BlackFutureAttacks) {
			//	// less severe Penalty for White
			//	ThreatBalance = something /2 ;
			//}
			// is piece hanging or protected ? by what ??
		}
	}
	return ThreatBalance;
}

SCORE EvaluatePawnStructure(const ChessPosition& P) {
	// Note: Hans Kmoch's terminology (from "Pawn Power In Chess") used throughout.

	SCORE s = 0;
	BitBoard Pawns = P.A & ~P.B & ~P.C;
	BitBoard WhitePawns = Pawns & ~P.D;
	BitBoard BlackPawns = Pawns & P.D;
	BitBoard Empty = 
		~(P.A | P.B | P.C) |	// Not Occupied 
		~(P.A & P.B &~P.C);		// Not EP

	BitBoard WhiteFrontSpans =
		FillUpOccluded(WhitePawns, -1i64);	// Fill all the way to edge of board
	BitBoard BlackFrontSpans =
		FillDownOccluded(BlackPawns, -1i64);	// Fill all the way to edge of board

	// get White pawns on adjacent files to black pawns
	BitBoard WAdjacentToB =
		(MoveLeftSingleOccluded(WhiteFrontSpans, -1i64) | MoveRightSingleOccluded(WhiteFrontSpans, -1i64)) &BlackFrontSpans; 

	// get Black pawns on adjacent files to white pawns
	BitBoard BAdjacentToW =
		(MoveLeftSingleOccluded(BlackFrontSpans, -1i64) | MoveRightSingleOccluded(BlackFrontSpans, -1i64)) &WhiteFrontSpans;

	// Find white Passed Pawns
	// Where there are NO adjacencies with Black, Fill up until we hit a Pawn:
	BitBoard WhitePassedPawnPaths = FillUpOccluded(WhitePawns, ~(Pawns|BAdjacentToW)) & ~WhitePawns;
//	DumpBitBoard(WhitePassedPawnPaths);

	// Find Black passed pawns
	// Where there are NO adjacencies with White, Fill down until we hit a Pawn:
	BitBoard BlackPassedPawnPaths = FillDownOccluded(BlackPawns, ~(Pawns | WAdjacentToB)) & ~BlackPawns;
//	DumpBitBoard(BlackPassedPawnPaths);
	int x = 0; 
	for (int f = 0; f < 8; f++) { // h-file to a-file
		if ((H8 << f) & WhitePassedPawnPaths)
		{
			// white Passed Pawn (can make it to H8)
			x= PopCount(WhitePassedPawnPaths & (FILEH << f)); // Distance to queening 
			s+=PassedPawnValue[x];
	//		printf("white passed pawn: file=%d distance= %d bonus=%d\n", f, x, PassedPawnValue[x]);	// Debug
		}
		if ((H1 << f) & BlackPassedPawnPaths)
		{
			// Black Passed Pawn (can make it to H1)
			x= PopCount(BlackPassedPawnPaths & (FILEH << f)); // Distance to queening
			s-=PassedPawnValue[x];
	//		printf("black passed pawn: file=%d distance= %d bonus=%d\n", f, x, -PassedPawnValue[x]);	// Debug
		}
	}
	return 0;
}