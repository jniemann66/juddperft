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

#include "movegen.h"

#include "chessposition.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <set>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <bitset>
#include <cassert>

#include <iostream>
#include <type_traits>

namespace juddperft {

#ifdef COUNT_MOVEGEN_CPU_CYCLES
uint64_t movegen_call_count = 0;
uint64_t movegen_total_cycles = 0;
#endif

// --- begin definition of MoveGenerator ---

////////////////////////////////////////////
// Move Generation Functions
// generateMoves()
////////////////////////////////////////////

MoveGenerator moveGenerator; // instance, to ensure movetable is initialised

void MoveGenerator::generateMoves(const ChessPosition& P, ChessMove* pM)
{
	assert((~(P.A | P.B | P.C) & P.D) == 0); // Should not be any "black" empty squares

#ifdef COUNT_MOVEGEN_CPU_CYCLES
	uint64_t cycles = __rdtsc();
#endif

	if (P.blackToMove) {
		generateBlackMoves(P, pM);
	} else {
		generateWhiteMoves(P, pM);
	}

#ifdef COUNT_MOVEGEN_CPU_CYCLES
	cycles = __rdtsc() - cycles;
	static std::mutex m;
	std::lock_guard<std::mutex> lock(m);
	movegen_call_count++;
	movegen_total_cycles += cycles;
#endif

}

// isInCheck() - Given a position, determines if player is in check -
// set IsBlack to true to test if Black is in check
// set IsBlack to false to test if White is in check.

inline bool MoveGenerator::isInCheck(const ChessPosition& P, bool bIsBlack)
{
	return bIsBlack ? isBlackInCheck(P) != 0 : isWhiteInCheck(P) != 0;
}

//////////////////////////////////////////
// White Move Generation Functions:     //
// generateWhiteMoves(),                //
// isWhiteInCheck(),                    //
// scanWhiteMoveForChecks()             //
//////////////////////////////////////////

void MoveGenerator::generateWhiteMoves(const ChessPosition& P, ChessMove* pM)
{
	if (P.whiteIsCheckmated || P.whiteIsStalemated) {
		pM->moveCount = 0;
		pM->origin = 0;
		pM->destination = 0;
		pM->piece = 0;
		pM->endOfMoveList = 1;
		return;
	}

	const Bitboard& PA = P.A;
	const Bitboard& PB = P.B;
	const Bitboard& PC = P.C;
	const Bitboard& PD = P.D;
	const Bitboard PAB = PA & PB;	// Bitboard containing EnPassants and kings

	ChessMove* pFirstMove = pM;
	const Bitboard Occupied = PA | PB | PC;								// all squares occupied by something
	const Bitboard BlackOccupied = Occupied & PD;							// all squares occupied by B, including Black EP Squares
	const Bitboard WhiteFree // all squares where W is free to move
			= (PA & PB & ~PC)        // any EP square
			  |	~(Occupied)				// any vacant square
			|	(~PA & PD)			// Black Bishop, Rook or Queen
			|	(~PB & PD);			// Black Pawn or Knight

	const Bitboard BlackCapturables = BlackOccupied & ~PAB; // All black pieces except enpassants and black king
	const Bitboard EP = BlackOccupied & PAB & ~PC;  // Black E.P. capture target

	static constexpr int maxPieces = 17; // maximum per side; 16 actual pieces + E.P.
	int piecesFound = 0;

	for (int origin = h1; origin <= a8; origin++) { // start from white's side of board
		const Bitboard FROM = 1ull << origin; // Bitboard representation of origin square
		const piece_t piece = P.getPieceAtSquare(origin);

		Bitboard mask; // Bitboard representing all the squares where piece can go

		switch (piece) {

		case WKING:
		case WKNIGHT:
			mask = WhiteFree;
			break;

		case WPAWN:
			mask = fillUpOccluded(FROM, (WhiteFree & ~BlackOccupied))  // pawns cannot capture while advancing
					| moveUpLeftSingleOccluded(FROM, WhiteFree & BlackOccupied)
					| moveUpRightSingleOccluded(FROM, WhiteFree & BlackOccupied);
			break;

		case WBISHOP:
			mask = getDiagonalMoveSquares(FROM, WhiteFree, BlackCapturables);
			break;

		case WROOK:
			mask =  getStraightMoveSquares(FROM, WhiteFree, BlackCapturables);
			break;

		case WQUEEN:
			mask = getDiagonalMoveSquares(FROM, WhiteFree, BlackCapturables)
					| getStraightMoveSquares(FROM, WhiteFree, BlackCapturables);
			break;

		default:
			continue;

		} // ends switch (piece)

		// loop over potential moves, and test their legality
		for (int mv = 0; mv < 32; mv++) {
			squareindex_t dest = mvtable[piece][origin][mv];
			if (dest > a8) {
				break; // reached end of potential moves for this piece
			}

			const Bitboard TO = (1ull << dest) & mask; // Bitboard representation of destination square
			if (TO == 0) {
				continue; // piece cannot go that square; go on to next potential move
			}

			// start initialising the move
			pM->origin = origin;
			pM->destination = dest;
			pM->flags = 0;
			pM->blackToMove = 0;
			pM->piece = piece;

			// Test for capture:

			if (TO & BlackCapturables) {
				// Only considered a capture if dest is not an enpassant or king.
				pM->capture = 1;
			}

			// perform the move on the board (to test for check)
			ChessPosition Q = P;

			// clear old and new square:
			const Bitboard O = ~(FROM | TO);
			Q.A &= O;
			Q.B &= O;
			Q.C &= O;
			Q.D &= O;

			// Populate new square with piece
			Q.A |= static_cast<int64_t>(piece & 1) << dest;
			Q.B |= static_cast<int64_t>((piece & 2) >> 1) << dest;
			Q.C |= static_cast<int64_t>((piece & 4) >> 2) << dest;

			bool promote = false;
			if (piece == WPAWN) {
				if (FROM & RANK2 && TO & RANK4) {
					pM->doublePawnMove = 1;
					// e.p. square
					const Bitboard x = TO >> 8;
					Q.A |= x;
					Q.B |= x;
					Q.C &= ~x;
					Q.D &= ~x;
				} else if (TO & EP) {
					pM->enPassantCapture = 1;
					// remove the actual pawn (dest was EP square)
					const Bitboard x = TO >> 8;
					Q.A &= ~x;
					Q.B &= ~x;
					Q.C &= ~x;
					Q.D &= ~x;
				} else if (TO & RANK8) {
					promote = true;
				}
			}

			// test if doing all this puts white in check. If so, move isn't legal
			if (isWhiteInCheck(Q)) {
				pM->illegalMove = 1;
				continue; // go on to next potential move
			}

			if (promote) {
				// make an additional 3 copies for the underpromotions
				*(pM + 1) = *pM;
				*(pM + 2) = *pM;
				*(pM + 3) = *pM;

				// parsimonious ordering : P=> N, R, Q, B
				pM->promoteKnight = 1;
				Q.C |= TO;
				scanWhiteMoveForChecks(Q, pM);
				pM++;

				pM->promoteRook = 1;
				Q.A &= ~TO;
				scanWhiteMoveForChecks(Q, pM);
				pM++;

				pM->promoteQueen = 1;
				Q.B |= TO;
				scanWhiteMoveForChecks(Q, pM);
				pM++;

				pM->promoteBishop = 1;
				Q.C &= ~TO;
			}

			scanWhiteMoveForChecks(Q, pM);
			pM++; // Add to list (advance pointer)
			pM->flags = 0;
		} // ends loop over mv

		if (P.dontGenerateAllMoves && pM > pFirstMove) { // proved there is at least one legal move
			goto cleanup; // get the hell outa here ...
		}

		if (++piecesFound > maxPieces) {
			break;
		}

	} // ends loop over origin

	// castling
	if (PA & PB & PC & ~PD & E1) { // King still in original position

		// Conditionally generate O-O move:
		if (P.whiteCanCastle && // White still has castle rights
				(~PA & ~PB & PC & ~PD & H1) && // Kingside rook is in correct position
				(WHITECASTLEZONE & Occupied) == 0 && // Castle Zone (f1, g1) is clear
				!isWhiteInCheck(P, WHITECASTLECHECKZONE)) // King is not in Check (in e1, f1, g1)
		{
			ChessPosition Q = P;
			pM->piece = WKING;
			pM->origin = e1;
			pM->destination = g1;
			pM->flags = 0;
			pM->blackToMove = 0;
			pM->castle = 1;

			Q.A ^= 0x000000000000000a;
			Q.B ^= 0x000000000000000a;
			Q.C ^= 0x000000000000000f;
			Q.D &= 0xfffffffffffffff0;	// clear colour of e1, f1, g1, h1 (make white)

			scanWhiteMoveForChecks(Q, pM);
			pM++; // Add to list (advance pointer)
			pM->flags = 0;
		}

		// Conditionally generate O-O-O move:
		if (P.whiteCanCastleLong && // White still has castle-long rights
				(~PA & ~PB & PC & ~PD & A1) && // Queenside rook is in correct Position
				(WHITECASTLELONGZONE & Occupied) == 0 && // Castle-long zone (b1, c1, d1) is clear
				!isWhiteInCheck(P, WHITECASTLELONGCHECKZONE)) // King is not in check (in e1, d1, c1)
		{
			// Ok to Castle Long
			ChessPosition Q = P;
			pM->piece = WKING;
			pM->origin = e1;
			pM->destination = c1;
			pM->flags = 0;
			pM->blackToMove = 0;
			pM->castleLong = 1;

			Q.A ^= 0x0000000000000028;
			Q.B ^= 0x0000000000000028;
			Q.C ^= 0x00000000000000b8;
			Q.D &= 0xffffffffffffff07;	// clear colour of a1, b1, c1, d1, e1 (make white)


			scanWhiteMoveForChecks(Q, pM);
			pM++; // Add to list (advance pointer)
			pM->flags = 0;
		}
	} // ends castling

	cleanup:
	// put the move count into the first move
	pFirstMove->moveCount = pM - pFirstMove;

	// Create 'no more moves' move to mark end of list
	pM->origin = 0;
	pM->destination = 0;
	pM->piece = 0;
	pM->endOfMoveList = 1;
}

inline Bitboard MoveGenerator::isWhiteInCheck(const ChessPosition& Z, Bitboard extend)
{
	Bitboard WhiteKing = (Z.A & Z.B & Z.C & ~Z.D) | extend;
	Bitboard V = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
				 WhiteKing |			// White King
				 ~(Z.A | Z.B | Z.C);	// All Unoccupied squares

	Bitboard A = Z.A & Z.D;				// Black A-Plane
	Bitboard B = Z.B & Z.D;				// Black B-Plane
	Bitboard C = Z.C & Z.D;				// Black C-Plane

	Bitboard S = C & ~A;				// Straight-moving Pieces
	Bitboard D = B & ~A;				// Diagonal-moving Pieces
	Bitboard K = A & B & C;				// King
	Bitboard P = A & ~B & ~C;			// Pawns
	Bitboard N = A & ~B & C;			// Knights

	Bitboard X = fillStraightAttacksOccluded(S, V);
	X |= fillDiagonalAttacksOccluded(D, V);
	X |= fillKingAttacks(K);
	X |= fillKnightAttacks(N);
	X |= MoveDownLeftRightSingle(P);
	return X & WhiteKing;
}

inline void MoveGenerator::scanWhiteMoveForChecks(ChessPosition& Q, ChessMove* pM)
{
	if (Q.dontDetectChecks) {
		return;
	}

	// test if white's move will put black in check or checkmate
	if (isBlackInCheck(Q))	{
		pM->check = 1;
		if (Q.dontDetectCheckmates == 0) {
			Q.blackToMove = 1;
			ChessMove blacksMoves[MOVELIST_SIZE];
			Q.dontGenerateAllMoves = 1; // only need one or more moves to prove that black has at least one legal move
			generateBlackMoves(Q, blacksMoves);
			if (blacksMoves->moveCount == 0) { // black will be in check with no legal moves
				pM->checkmate = 1; // this move is a checkmating move
			}
		}
	} else {
		pM->check = 0;
		pM->checkmate = 0;
	}
}


//////////////////////////////////////////
// Black Move Generation Functions:     //
// generateBlackMoves(),                //
// isBlackInCheck()                     //
// scanBlackMoveForChecks()             //
//////////////////////////////////////////

void MoveGenerator::generateBlackMoves(const ChessPosition& P, ChessMove* pM)
{
	if (P.blackIsCheckmated || P.blackIsStalemated) {
		pM->moveCount = 0;
		pM->origin = 0;
		pM->destination = 0;
		pM->piece = 0;
		pM->endOfMoveList = 1;
		return;
	}

	const Bitboard& PA = P.A;
	const Bitboard& PB = P.B;
	const Bitboard& PC = P.C;
	const Bitboard& PD = P.D;
	const Bitboard PAB = PA & PB;	// Bitboard containing EnPassants and kings

	ChessMove* pFirstMove = pM;
	const Bitboard Occupied = PA | PB | PC; // all squares occupied by something
	const Bitboard WhiteOccupied = (Occupied & ~PD); // all squares occupied by W, including white EP Squares
	const Bitboard BlackFree // all squares where B is free to move
			= (PA & PB & ~PC)		// any EP square
			  | ~(Occupied)				// any vacant square
			| (~PA & ~PD)			// White Bishop, Rook or Queen
			| (~PB & ~PD);			// White Pawn or Knight

	const Bitboard WhiteCapturables = WhiteOccupied & ~PAB; // All white pieces except enpassants and white king
	const Bitboard EP = WhiteOccupied & PAB & ~PC; // White E.P. capture target

	static constexpr int maxPieces = 17; // maximum per side; 16 actual pieces + E.P.
	int piecesFound = 0;

	for (int origin = a8; origin >= h1; origin--) { // start from black's side of board
		const Bitboard FROM = 1ull << origin;  // Bitboard representation of origin square
		const piece_t piece = P.getPieceAtSquare(origin);

		Bitboard mask;  // Bitboard representing all the squares where piece can go

		switch (piece) {

		case BKING:
		case BKNIGHT:
			mask = BlackFree;
			break;

		case BPAWN:
			mask = fillDownOccluded(FROM, (BlackFree & ~WhiteOccupied))  // pawns cannot capture while advancing
					| moveDownLeftSingleOccluded(FROM, BlackFree & WhiteOccupied)
					| moveDownRightSingleOccluded(FROM, BlackFree & WhiteOccupied);

			break;

		case BBISHOP:
			mask = getDiagonalMoveSquares(FROM, BlackFree, WhiteCapturables);
			break;

		case BROOK:
			mask =  getStraightMoveSquares(FROM, BlackFree, WhiteCapturables);
			break;

		case BQUEEN:
			mask = getDiagonalMoveSquares(FROM, BlackFree, WhiteCapturables)
					| getStraightMoveSquares(FROM, BlackFree, WhiteCapturables);
			break;

		default:
			continue;

		} // ends switch (piece)

		// loop over potential moves, and test their legality
		for (int mv = 0; mv < 32; mv++) {
			squareindex_t dest = mvtable[piece][origin][mv];
			if (dest > a8) {
				break; // reached end of potential moves for this piece
			}

			const Bitboard TO = (1ull << dest) & mask;  // Bitboard representation of destination square
			if (TO == 0) {
				continue; // piece cannot go that square; go on to next potential move
			}

			// start initialising the move
			pM->origin = origin;
			pM->destination = dest;
			pM->flags = 0;
			pM->blackToMove = 1;
			pM->piece = piece;

			// Test for capture:
			if (TO & WhiteCapturables) {
				// Only considered a capture if dest is not an enpassant or king.
				pM->capture = 1;
			}

			ChessPosition Q = P;

			// clear old and new square
			const Bitboard O = ~(FROM | TO);
			Q.A &= O;
			Q.B &= O;
			Q.C &= O;
			Q.D &= O;

			// Populate new square with piece
			Q.A |= static_cast<int64_t>(piece & 1) << dest;
			Q.B |= static_cast<int64_t>((piece & 2) >> 1) << dest;
			Q.C |= static_cast<int64_t>((piece & 4) >> 2) << dest;
			Q.D |= TO;

			bool promote = false;
			if (piece == BPAWN) {
				if (FROM & RANK7 && TO & RANK5) {
					pM->doublePawnMove = 1;
					// e.p. square
					const Bitboard x = TO << 8;
					Q.A |= x;
					Q.B |= x;
					Q.C &= ~x;
					Q.D |= x;
				} else if (TO & EP) {
					pM->enPassantCapture = 1;
					// remove the actual pawn (dest was EP square)
					const Bitboard x = TO << 8;
					Q.A &= ~x;
					Q.B &= ~x;
					Q.C &= ~x;
					Q.D &= ~x;
				} else if (TO & RANK1) {
					promote = true;
				}
			}

			// test if doing all this puts black in check. If so, move isn't legal
			if (isBlackInCheck(Q)) {
				pM->illegalMove = 1;
				continue; // go on to next potential move
			}

			if (promote) {
				// make an additional 3 copies for underpromotions
				*(pM + 1) = *pM;
				*(pM + 2) = *pM;
				*(pM + 3) = *pM;

				// parsimonious ordering : P=> N, R, Q, B
				pM->promoteKnight = 1;
				Q.C |= TO;
				scanBlackMoveForChecks(Q, pM);
				pM++;

				pM->promoteRook = 1;
				Q.A &= ~TO;
				scanBlackMoveForChecks(Q, pM);
				pM++;

				pM->promoteQueen = 1;
				Q.B |= TO;
				scanBlackMoveForChecks(Q, pM);
				pM++;

				pM->promoteBishop = 1;
				Q.C &= ~TO;
			}

			scanBlackMoveForChecks(Q, pM);
			pM++; // Add to list (advance pointer)
			pM->flags = 0;

		} // ends loop over mv

		if (P.dontGenerateAllMoves && pM > pFirstMove) { // proved there is at least one legal move
			goto cleanup;  // get the hell outa here ...
		}

		if (++piecesFound > maxPieces) {
			break;
		}

	} // ends loop over origin;

	// castling
	if (PA & PB & PC & PD & E8) { // King still in original position

		// Conditionally generate O-O move:
		if (P.blackCanCastle && // Black still has castle rights
				(~PA & ~PB & PC & PD & H8) && // Kingside rook is in correct position
				(BLACKCASTLEZONE & Occupied) == 0 && // Castle Zone (f8, g8) is clear
				!isBlackInCheck(P, BLACKCASTLECHECKZONE)) // King is not in Check (in e8, f8, g8)
		{
			ChessPosition Q = P;
			pM->piece = BKING;
			pM->origin = e8;
			pM->destination = g8;
			pM->flags = 0;
			pM->blackToMove = 1;
			pM->castle = 1;

			Q.A ^= 0x0a00000000000000;
			Q.B ^= 0x0a00000000000000;
			Q.C ^= 0x0f00000000000000;
			Q.D ^= 0x0f00000000000000;

			scanBlackMoveForChecks(Q, pM);
			pM++; // Add to list (advance pointer)
			pM->flags = 0;
		}

		// Conditionally generate O-O-O move:
		if (P.blackCanCastleLong && // Black still has castle-long rights
				(~PA & ~PB & PC & PD & A8) && // Queenside rook is in correct Position
				(BLACKCASTLELONGZONE & Occupied) == 0 && // Castle Long Zone (b8, c8, d8) is clear
				!isBlackInCheck(P, BLACKCASTLELONGCHECKZONE)) // King is not in Check (e8, d8, c8)
		{
			ChessPosition Q = P;
			pM->piece = BKING;
			pM->origin = e8;
			pM->destination = c8;
			pM->flags = 0;
			pM->blackToMove = 1;
			pM->castleLong = 1;

			Q.A ^= 0x2800000000000000;
			Q.B ^= 0x2800000000000000;
			Q.C ^= 0xb800000000000000;
			Q.D ^= 0xb800000000000000;

			scanBlackMoveForChecks(Q, pM);
			pM++; // Add to list (advance pointer)
			pM->flags = 0;
		}

	} // ends castling

	cleanup:
	// put the move count into the first move
	pFirstMove->moveCount = pM - pFirstMove;

	// Create 'no more moves' move to mark end of list
	pM->origin = 0;
	pM->destination = 0;
	pM->piece = 0;
	pM->endOfMoveList = 1;
}

inline Bitboard MoveGenerator::isBlackInCheck(const ChessPosition& Z, Bitboard extend)
{
	Bitboard BlackKing = (Z.A & Z.B & Z.C & Z.D) | extend;
	Bitboard V = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
				 BlackKing |			// Black King
				 ~(Z.A | Z.B | Z.C);	// All Unoccupied squares

	Bitboard A = Z.A & ~Z.D;			// White A-Plane
	Bitboard B = Z.B & ~Z.D;			// White B-Plane
	Bitboard C = Z.C & ~Z.D;			// White C-Plane

	Bitboard S = C & ~A;				// Straight-moving Pieces
	Bitboard D = B & ~A;				// Diagonal-moving Pieces
	Bitboard K = A & B & C;				// King
	Bitboard P = A & ~B & ~C;			// Pawns
	Bitboard N = A & ~B & C;			// Knights

	Bitboard X = fillStraightAttacksOccluded(S, V);
	X |= fillDiagonalAttacksOccluded(D, V);
	X |= fillKingAttacks(K);
	X |= fillKnightAttacks(N);
	X |= MoveUpLeftRightSingle(P);

	return X & BlackKing;
}

inline void MoveGenerator::scanBlackMoveForChecks(ChessPosition& Q, ChessMove* pM)
{
	if (Q.dontDetectChecks) {
		return;
	}

	// test if black's move will put white in check or checkmate
	if (isWhiteInCheck(Q))	{
		pM->check = 1;
		if (Q.dontDetectCheckmates == 0) {
			Q.blackToMove = 0;
			ChessMove whitesMoves[MOVELIST_SIZE];
			Q.dontGenerateAllMoves = 1; // only need one or more moves to prove that white has at least one legal move
			generateWhiteMoves(Q, whitesMoves);
			if (whitesMoves->moveCount == 0) { // white will be in check with no legal moves
				pM->checkmate = 1; // this move is a checkmating move
			}
		}
	} else {
		pM->check = 0;
		pM->checkmate = 0;
	}
}

squareindex_t MoveGenerator::mvtable[16][64][32];

void MoveGenerator::populate_mvtable()
{
	memset(mvtable, xx, 16 * 64 * 32 * sizeof(squareindex_t));

	for (int p = 0; p < 16; p++) {
		for (int sq = 0; sq < 64; sq++) {
			switch(p) {
			case WPAWN:
			{
				int m = 0;

				// captures
				if (const auto captLeft = MoveUpLeftIndex[sq]; captLeft != xx) {
					mvtable[p][sq][m++] = captLeft;
				}

				if (const auto captRight = MoveUpRightIndex[sq]; captRight != xx) {
					mvtable[p][sq][m++] = captRight;
				}

				// advances
				if (const auto step1 = MoveUpIndex[sq]; step1 != xx) {
					mvtable[p][sq][m++] = step1;
					if (sq >= h2 && sq <= a2) { // double pawn advance only available from rank 2
						if (const auto step2 = MoveUpIndex[step1]; step2 != xx) {
							mvtable[p][sq][m++] = step2;
						}
					}
				}
			}
				break;

			case BPAWN:
			{
				int m = 0;

				// captures
				if (const auto captLeft = MoveDownLeftIndex[sq]; captLeft != xx) {
					mvtable[p][sq][m++] = captLeft;
				}

				if (const auto captRight = MoveDownRightIndex[sq]; captRight != xx) {
					mvtable[p][sq][m++] = captRight;
				}

				// advances
				if (const auto step1 = MoveDownIndex[sq]; step1 != xx) {
					mvtable[p][sq][m++] = step1;
					if (sq >= h7 && sq <= a7) { // double pawn advance only available from rank 7
						if (const auto step2 = MoveDownIndex[step1]; step2 != xx) {
							mvtable[p][sq][m++] = step2;
						}
					}
				}
			}
				break;

			case WBISHOP:
			case BBISHOP:
			{
				int m = 0;
				Bitboard D = fillDiagonalAttacksOccluded(1ull << sq, -1ll);
				for (squareindex_t dsq = 0; dsq < 64; dsq++) {
					if ((1ull << dsq) & D) {
						mvtable[p][sq][m++] = dsq;
					}
				}
			}
				break;

			case WROOK:
			case BROOK:
			{
				int m = 0;
				Bitboard S = fillStraightAttacksOccluded(1ull << sq, -1ll);
				for (squareindex_t dsq = 0; dsq < 64; dsq++) {
					if ((1ull << dsq) & S) {
						mvtable[p][sq][m++] = dsq;
					}
				}
			}
				break;

			case WKNIGHT:
			case BKNIGHT:
			{
				int m = 0;
				Bitboard N = fillKnightAttacks(1ull << sq);
				for (squareindex_t dsq = 0; dsq < 64; dsq++) {
					if ((1ull << dsq) & N) {
						mvtable[p][sq][m++] = dsq;
					}
				}
			}
				break;

			case WQUEEN:
			case BQUEEN:
			{
				int m = 0;
				Bitboard D = fillDiagonalAttacksOccluded(1ull << sq, -1ll);
				Bitboard S = fillStraightAttacksOccluded(1ull << sq, -1ll);
				for (squareindex_t dsq = 0; dsq < 64; dsq++) {
					if ((1ull << dsq) & D) {
						mvtable[p][sq][m++] = dsq;
					}
					if ((1ull << dsq) & S) {
						mvtable[p][sq][m++] = dsq;
					}
				}
			}
				break;

			case WKING:
			case BKING:
			{
				int m = 0;
				Bitboard K = fillKingAttacks(1ull << sq);
				for (squareindex_t dsq = 0; dsq < 64; dsq++) {
					if ((1ull << dsq) & K) {
						mvtable[p][sq][m++] = dsq;
					}
				}
			}
				break;

			default:
				break;
			}
		}
	}
}


inline Bitboard genWhiteAttacks(const ChessPosition& Z)
{
	Bitboard Occupied = Z.A | Z.B | Z.C;
	Bitboard Empty = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
					 ~Occupied;							// All Unoccupied squares

	Bitboard PotentialCapturesForWhite = Occupied & Z.D; // Black Pieces (including Kings)

	Bitboard A = Z.A & ~Z.D;			// White A-Plane
	Bitboard B = Z.B & ~Z.D;			// White B-Plane
	Bitboard C = Z.C & ~Z.D;			// White C-Plane

	Bitboard S = C & ~A;				// Straight-moving Pieces
	Bitboard D = B & ~A;				// Diagonal-moving Pieces
	Bitboard K = A & B & C;				// King
	Bitboard P = A & ~B & ~C;			// Pawns
	Bitboard N = A & ~B & C;			// Knights

	Bitboard StraightAttacks = moveUpSingleOccluded(fillUpOccluded(S, Empty), Empty | PotentialCapturesForWhite);
	StraightAttacks |= moveRightSingleOccluded(fillRightOccluded(S, Empty), Empty | PotentialCapturesForWhite);
	StraightAttacks |= moveDownSingleOccluded(fillDownOccluded(S, Empty), Empty | PotentialCapturesForWhite);
	StraightAttacks |= moveLeftSingleOccluded(fillLeftOccluded(S, Empty), Empty | PotentialCapturesForWhite);

	Bitboard DiagonalAttacks = moveUpRightSingleOccluded(fillUpRightOccluded(D, Empty), Empty | PotentialCapturesForWhite);
	DiagonalAttacks |= moveDownRightSingleOccluded(fillDownRightOccluded(D, Empty), Empty | PotentialCapturesForWhite);
	DiagonalAttacks |= moveDownLeftSingleOccluded(fillDownLeftOccluded(D, Empty), Empty | PotentialCapturesForWhite);
	DiagonalAttacks |= moveUpLeftSingleOccluded(fillUpLeftOccluded(D, Empty), Empty | PotentialCapturesForWhite);

	Bitboard KingAttacks = fillKingAttacksOccluded(K, Empty | PotentialCapturesForWhite);
	Bitboard KnightAttacks = fillKnightAttacksOccluded(N, Empty | PotentialCapturesForWhite);
	Bitboard PawnAttacks = MoveUpLeftRightSingle(P) & (Empty | PotentialCapturesForWhite);

	return (StraightAttacks | DiagonalAttacks | KingAttacks | KnightAttacks | PawnAttacks);
}

inline Bitboard genBlackAttacks(const ChessPosition& Z)
{
	Bitboard Occupied = Z.A | Z.B | Z.C;
	Bitboard Empty = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
					 ~Occupied;							// All Unoccupied squares

	Bitboard PotentialCapturesForBlack = Occupied & ~Z.D; // White Pieces (including Kings)

	Bitboard A = Z.A & Z.D;				// Black A-Plane
	Bitboard B = Z.B & Z.D;				// Black B-Plane
	Bitboard C = Z.C & Z.D;				// Black C-Plane

	Bitboard S = C & ~A;				// Straight-moving Pieces
	Bitboard D = B & ~A;				// Diagonal-moving Pieces
	Bitboard K = A & B & C;				// King
	Bitboard P = A & ~B & ~C;			// Pawns
	Bitboard N = A & ~B & C;			// Knights

	Bitboard StraightAttacks = moveUpSingleOccluded(fillUpOccluded(S, Empty), Empty | PotentialCapturesForBlack);
	StraightAttacks |= moveRightSingleOccluded(fillRightOccluded(S, Empty), Empty | PotentialCapturesForBlack);
	StraightAttacks |= moveDownSingleOccluded(fillDownOccluded(S, Empty), Empty | PotentialCapturesForBlack);
	StraightAttacks |= moveLeftSingleOccluded(fillLeftOccluded(S, Empty), Empty | PotentialCapturesForBlack);

	Bitboard DiagonalAttacks = moveUpRightSingleOccluded(fillUpRightOccluded(D, Empty), Empty | PotentialCapturesForBlack);
	DiagonalAttacks |= moveDownRightSingleOccluded(fillDownRightOccluded(D, Empty), Empty | PotentialCapturesForBlack);
	DiagonalAttacks |= moveDownLeftSingleOccluded(fillDownLeftOccluded(D, Empty), Empty | PotentialCapturesForBlack);
	DiagonalAttacks |= moveUpLeftSingleOccluded(fillUpLeftOccluded(D, Empty), Empty | PotentialCapturesForBlack);

	Bitboard KingAttacks = fillKingAttacksOccluded(K, Empty | PotentialCapturesForBlack);
	Bitboard KnightAttacks = fillKnightAttacksOccluded(N, Empty | PotentialCapturesForBlack);
	Bitboard PawnAttacks = MoveDownLeftRightSingle(P) & (Empty | PotentialCapturesForBlack);

	return (StraightAttacks | DiagonalAttacks | KingAttacks | KnightAttacks | PawnAttacks);
}

//////////////////////////////////////////////////////////////////////////////////
// printMove() : Converts a move to ascii and prints it to stdout, unless a character
// buffer is supplied, in which case the ascii is written to pBuffer instead.
// A number of notation styles are supported. Usually, CoOrdinate should be used
// for WinBoard.
//////////////////////////////////////////////////////////////////////////////////
void printMove(const ChessMove& mv, MoveNotationStyle style /* = LongAlgebraic */, char* pBuffer, const ChessMove *movelist)
{
	char ch1, ch2, p;
	const unsigned char p7 = mv.piece & 0x7;

	if (style != CoOrdinate)
	{
		if (mv.castle == 1)
		{
			if (pBuffer == nullptr)
				printf(" O-O\n");
			else
				sprintf (pBuffer, " O-O\n");
			return;
		}

		if (mv.castleLong == 1)
		{
			if (pBuffer == nullptr)
				printf(" O-O-O\n");
			else
				sprintf (pBuffer, " O-O-O\n");
			return;
		}
	}

	ch1 = 'h' - (mv.origin % 8);
	ch2 = 'h' - (mv.destination % 8);

	// Determine piece and assign character p accordingly:
	switch (p7)
	{
	case WPAWN:
		p = ' ';
		break;
	case WKNIGHT:
		p = 'N';
		break;
	case WBISHOP:
		p = 'B';
		break;
	case WROOK:
		p = 'R';
		break;
	case WQUEEN:
		p = 'Q';
		break;
	case WKING:
		p = 'K';
		break;
	default:
		p = '?';
	} // ends switch

	char s[SMALL_BUFFER_SIZE];  // output string
	*s = 0;

	switch(style)
	{
	case LongAlgebraic:
	case LongAlgebraicNoNewline:
	case Diagnostic:
	{
		// Determine if move is a capture
		if (mv.capture || mv.enPassantCapture) {
			sprintf(s, "%c%c%dx%c%d", p, ch1, 1 + (mv.origin >> 3), ch2, 1 + (mv.destination >> 3));
		} else {
			sprintf(s, "%c%c%d-%c%d", p, ch1, 1 + (mv.origin >> 3), ch2, 1 + (mv.destination >> 3));
		}

		// Promotions:
		if (mv.promoteQueen) {
			strcat(s, "=Q");
		} else if (mv.promoteBishop) {
			strcat(s, "=B");
		} else if (mv.promoteKnight) {
			strcat(s, "=N");
		} else if (mv.promoteRook) {
			strcat(s, "=R");
		}

		// checks
		if (mv.checkmate) {
			strcat(s, "#");
		} else if (mv.check) {
			strcat(s, "+");
		}

		if (style == LongAlgebraicNoNewline) {
			strcat(s, " ");
		} else {
			strcat(s, "\n");
		}

		if (pBuffer) {
			strcpy(pBuffer, s);
		} else {
			printf("%s", s);
		}
	}
		break;

	case StandardAlgebraic:
	{
		bool showOriginRank = false;
		bool showOriginFile = false;

		if (p7 == WPAWN) {
			// for pawns, piece type is not shown, and originating file is only shown in captures
			showOriginFile = (mv.capture || mv.enPassantCapture);
		} else {
			// print the piece type
			sprintf(s, "%c", p);

			if (p7 >= WBISHOP && p7 <= WQUEEN) { // potentialy ambiguous piece
				// detect if disambiguation is required
				if (movelist) {
					const ChessMove *mm = movelist;
					std::set<unsigned char> other_origin_ranks;
					std::set<unsigned char> other_origin_files;
					while (!mm->endOfMoveList && (mm - movelist < MOVELIST_SIZE)) {
						if (mm->piece == mv.piece && mm->destination == mv.destination && mm->origin != mv.origin) {
							// same type of piece, same destination, different origin
							other_origin_ranks.insert(mm->origin >> 3);
							other_origin_files.insert(mm->origin & 0x7);
						}
						mm++;
					}

					if (!other_origin_ranks.empty() || !other_origin_files.empty()) {
						const bool sharesOriginFile{other_origin_files.count(mv.origin & 0x7) > 0};
						const bool sharesOriginRank{other_origin_ranks.count(mv.origin >> 3) > 0};
						if (!sharesOriginFile && !sharesOriginRank) {
							showOriginFile = true;
						} else  {
							showOriginFile = sharesOriginRank;
							showOriginRank = sharesOriginFile;
						}
					}
				} else {
					// if there is no movelist supplied (not recommended for SAN), originating rank and file must both be shown
					showOriginRank = true;
					showOriginFile = true;
				}
			}
		}

		if (showOriginFile) {
			sprintf(s + strlen(s), "%c", ch1);
		}

		if (showOriginRank) {
			sprintf(s + strlen(s), "%d", 1 + (mv.origin >> 3));
		}

		// Format with 'x' for captures
		if (mv.capture || mv.enPassantCapture) {
			sprintf(s + strlen(s), "x%c%d", ch2, 1 + (mv.destination >> 3));
		} else {
			sprintf(s + strlen(s), "%c%d", ch2, 1 + (mv.destination >> 3));
		}

		// Promotions:
		if (mv.promoteQueen) {
			strcat(s, "=Q");
		} else if (mv.promoteBishop) {
			strcat(s, "=B");
		} else if (mv.promoteKnight) {
			strcat(s, "=N");
		} else if (mv.promoteRook) {
			strcat(s, "=R");
		}

		// checks
		if (mv.checkmate) {
			strcat(s, "#");
		} else if (mv.check) {
			strcat(s, "+");
		}

		strcat(s, "\n");

		if (pBuffer) {
			strcpy(pBuffer, s);
		} else {
			printf("%s", s);
		}
	}
		break;

	case CoOrdinate:
	{
		sprintf(s, "%c%d%c%d", ch1, 1 + (mv.origin >> 3), ch2, 1 + (mv.destination >> 3));
		// Promotions:
		if (mv.promoteBishop) {
			strcat(s, "b");
		} else if (mv.promoteKnight) {
			strcat(s, "n");
		} else if (mv.promoteRook) {
			strcat(s, "r");
		} else if (mv.promoteQueen) {
			strcat(s, "q");
		}

		strcat(s, "\n");

		if (pBuffer) {
			strcpy(pBuffer, s);
		} else {
			printf("%s", s);
		}
	}
		break;

	} // ends switch
}

void printMoveList(ChessMove* pMoveList, MoveNotationStyle style /* = LongAlgebraic */, char* pBuffer /*=nullptr*/)
{
	ChessMove* pM = pMoveList;
	unsigned int i = 0;
	do{
		if (pM->destination != pM->origin) {
			printMove(*pM, style, pBuffer, pMoveList);
		}

		if (pM->endOfMoveList != 0) {
			// No more moves
			break;
		}

		++pM;
	} while (++i < MOVELIST_SIZE);
}

// misc

/*
  // NQR !
	//// Populate new square (Branchless method, BitBoard input):
	//ChessPosition::A |= To & -( static_cast<int64_t>(M.Piece) & 1);
	//ChessPosition::B |= To & -((static_cast<int64_t>(M.Piece) & 2) >> 1);
	//ChessPosition::C |= To & -((static_cast<int64_t>(M.Piece) & 4) >> 2);
	//ChessPosition::D |= To & -((static_cast<int64_t>(M.Piece) & 8) >> 3);
*/

} // namespace juddperft
