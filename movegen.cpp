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
#include "hashtable.h"

#include "fen.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>



#include <set>
#include <fstream>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <bitset>
#include <cassert>

namespace juddperft {

#ifdef COUNT_MOVEGEN_CPU_CYCLES
uint64_t movegen_call_count = 0;
uint64_t movegen_total_cycles = 0;
#endif


//////////////////////////////////////////////
// Implementation of Class ChessPosition	//
//////////////////////////////////////////////

ChessPosition::ChessPosition()
{
	A = 0;
	B = 0;
	C = 0;
	D = 0;
	flags = 0;

	ChessPosition::hk = 0;
}

ChessPosition& ChessPosition::setupStartPosition()
{
	flags = 0;
	A = 0x4Aff00000000ff4A;
	B = 0x3C0000000000003C;
	C = 0xDB000000000000DB;
	D = 0xffff000000000000;
	BlackCanCastle = 1;
	WhiteCanCastle = 1;
	BlackCanCastleLong = 1;
	WhiteCanCastleLong = 1;
	BlackForfeitedCastle = 0;
	WhiteForfeitedCastle = 0;
	BlackForfeitedCastleLong = 0;
	WhiteForfeitedCastleLong = 0;
	BlackDidCastle = 0;
	WhiteDidCastle = 0;
	BlackDidCastleLong = 0;
	WhiteDidCastleLong = 0;
	MoveNumber = 1;
	HalfMoves = 0;
	calculateMaterial();

	ChessPosition::calculateHash();

	return *this;
}

ChessPosition & ChessPosition::calculateHash()
{
	hk = 0;

	for (unsigned int q = 0; q < 64; q++) {
		piece_t piece = getPieceAtSquare(q);
		if (piece & 0x7) {
			ChessPosition::hk ^= zobristKeys.zkPieceOnSquare[piece][q];
		}
	}

	if (BlackToMove)
		ChessPosition::hk ^= zobristKeys.zkBlackToMove;

	if (WhiteCanCastle)
		ChessPosition::hk ^= zobristKeys.zkWhiteCanCastle;

	if (WhiteCanCastleLong)
		ChessPosition::hk ^= zobristKeys.zkWhiteCanCastleLong;

	if (BlackCanCastle)
		ChessPosition::hk ^= zobristKeys.zkBlackCanCastle;

	if (BlackCanCastleLong)
		ChessPosition::hk ^= zobristKeys.zkBlackCanCastleLong;

	return *this;
}

int ChessPosition::calculateMaterial() const
{
	int material = 0;

	for (int q = 0; q < 64; q++)
	{
		switch (getPieceAtSquare(q))
		{
		case WPAWN:
			material += 100;
			break;
		case WBISHOP:
			material += 300;
			break;
		case WROOK:
			material += 500;
			break;
		case WQUEEN:
			material += 900;
			break;
		case WKNIGHT:
			material += 300;
			break;
		case BPAWN:
			material -= 100;
			break;
		case BBISHOP:
			material -= 300;
			break;
		case BROOK:
			material -= 500;
			break;
		case BQUEEN:
			material -= 900;
			break;
		case BKNIGHT:
			material -= 300;
			break;
		}
	}

	return material;
}

ChessPosition& ChessPosition::performMove(const ChessMove& M)
{
	Bitboard To = 1ull << M.destination;
	const Bitboard O = ~((1ull << M.origin) | To);
	const unsigned long nFromSquare = M.origin;
	const unsigned long nToSquare = M.destination;

	// if move is known to be delivering checkmate, immediately flag checkmate in the position
	if (M.checkmate && M.blackToMove == BlackToMove) {

		if (M.blackToMove) {
			WhiteIsCheckmated = 1;
		} else {
			BlackIsCheckmated = 1;
		}
	}

	// clear any enpassant squares
	const Bitboard EnPassant = A & B & ~C;
	A &= ~EnPassant;
	B &= ~EnPassant;
	C &= ~EnPassant;
	D &= ~EnPassant;

	const unsigned long epSquareIdx = getSquareIndex(EnPassant);
	hk ^= zobristKeys.zkPieceOnSquare[WENPASSANT][epSquareIdx]; // Remove EP from nEPSquare

	switch (M.piece) {
	case BKING:
		if (M.castle) {

			// APPLY CASTLING MOVES:
			// we use magic XOR-tricks to do the job ! :

			// The following is for O-O:
			//
			//    K..R          .RK.
			// A: 1000 ^ 1010 = 0010
			// B: 1000 ^ 1010 = 0010
			// C: 1001 ^ 1111 = 0110
			// For Black:
			// D: 1001 ^ 1111 = 0110
			// For White:
			// D &= 0xfffffffffffffff0 (ie clear colour of affected squares from K to KR)

			// The following is for O-O-O:
			//
			//    R... K...                      ..KR ....
			// A: 0000 1000 ^ 0010 1000 (0x28) = 0010 0000
			// B: 0000 1000 ^ 0010 1000 (0x28) = 0010 0000
			// C: 1000 1000 ^ 1011 1000 (0xB8) = 0011 0000
			// For Black:
			// D: 1000 1000 ^ 1011 1000 (0xB8) = 0011 0000
			// For White:
			// D &= 0xffffffffffffff07 (ie clear colour of affected squares from QR to K)

			A ^= 0x0a00000000000000;
			B ^= 0x0a00000000000000;
			C ^= 0x0f00000000000000;
			D ^= 0x0f00000000000000;

			hk ^= zobristKeys.zkDoBlackCastle;
			if (BlackCanCastleLong) {
				hk ^= zobristKeys.zkBlackCanCastleLong;	// flip black castling long
			}

			BlackDidCastle = 1;
			BlackCanCastle = 0;
			BlackCanCastleLong = 0;
			return *this;
		}

		if (M.castleLong) {
			A ^= 0x2800000000000000;
			B ^= 0x2800000000000000;
			C ^= 0xb800000000000000;
			D ^= 0xb800000000000000;

			hk ^= zobristKeys.zkDoBlackCastleLong;
			if (BlackCanCastle) {
				hk ^= zobristKeys.zkBlackCanCastle;	// conditionally flip black castling
			}

			BlackDidCastleLong = 1;
			BlackCanCastle = 0;
			BlackCanCastleLong = 0;
			return *this;
		}

		// ordinary king move; Black could have castled,
		// but chose to move the King in a non-castling move
		if (BlackCanCastle) {
			hk ^= zobristKeys.zkBlackCanCastle;	// flip black castling
		}

		if (BlackCanCastleLong) {
			hk ^= zobristKeys.zkBlackCanCastleLong;	// flip black castling long
		}

		BlackForfeitedCastle = 1;
		BlackForfeitedCastleLong = 1;
		BlackCanCastle = 0;
		BlackCanCastleLong = 0;
		break;

	case WKING:
		if (M.castle) {
			A ^= 0x000000000000000a;
			B ^= 0x000000000000000a;
			C ^= 0x000000000000000f;
			D &= 0xfffffffffffffff0;	// clear colour of e1, f1, g1, h1 (make white)

			hk ^= zobristKeys.zkDoWhiteCastle;
			if (WhiteCanCastleLong) {
				hk ^= zobristKeys.zkWhiteCanCastleLong;	// conditionally flip white castling long
			}

			WhiteDidCastle = 1;
			WhiteCanCastle = 0;
			WhiteCanCastleLong = 0;
			return *this;
		}

		if (M.castleLong) {
			A ^= 0x0000000000000028;
			B ^= 0x0000000000000028;
			C ^= 0x00000000000000b8;
			D &= 0xffffffffffffff07;	// clear colour of a1, b1, c1, d1, e1 (make white)

			hk ^= zobristKeys.zkDoWhiteCastleLong;
			if (WhiteCanCastle) {
				hk ^= zobristKeys.zkWhiteCanCastle;	// conditionally flip white castling
			}

			WhiteDidCastleLong = 1;
			WhiteCanCastle = 0;
			WhiteCanCastleLong = 0;
			return *this;
		}

		// ordinary king move; White could have castled,
		// but chose to move the King in a non-castling move
		if (WhiteCanCastle) {
			hk ^= zobristKeys.zkWhiteCanCastle;	// flip white castling
		}

		if (WhiteCanCastleLong) {
			hk ^= zobristKeys.zkWhiteCanCastleLong;	// flip white castling long
		}

		WhiteForfeitedCastle = 1;
		WhiteCanCastle = 0;
		WhiteForfeitedCastleLong = 1;
		WhiteCanCastleLong = 0;
		break;

	case BROOK:
		if (nFromSquare == SquareIndex::h8) {
			// Black moved K-side Rook and forfeits right to castle K-side
			if (BlackCanCastle){
				BlackForfeitedCastle = 1;
				hk ^= zobristKeys.zkBlackCanCastle;	// flip black castling
				BlackCanCastle = 0;
			}
		} else if (nFromSquare == SquareIndex::a8) {
			// Black moved the QS Rook and forfeits right to castle Q-side
			if (BlackCanCastleLong) {
				BlackForfeitedCastleLong = 1;
				hk ^= zobristKeys.zkBlackCanCastleLong;	// flip black castling long
				BlackCanCastleLong = 0;
			}
		}
		break;

	case WROOK:
		if (nFromSquare == SquareIndex::h1) {
			// White moved K-side Rook and forfeits right to castle K-side
			if (WhiteCanCastle) {
				WhiteForfeitedCastle = 1;
				hk ^= zobristKeys.zkWhiteCanCastle;	// flip white castling BROKEN !!!
				WhiteCanCastle = 0;
			}
		} else if (nFromSquare == SquareIndex::a1) {
			// White moved the QSide Rook and forfeits right to castle Q-side
			if (WhiteCanCastleLong) {
				WhiteForfeitedCastleLong = 1;
				hk ^= zobristKeys.zkWhiteCanCastleLong;	// flip white castling long
				WhiteCanCastleLong = 0;
			}
		}
		break;

	default:
		break;
	} // ends switch (M.Piece)

	// Ordinary Captures
	if (M.capture) {
		// find out what was captured
		const unsigned int capturedpiece =
				((D & To) >> nToSquare) << 3
														  | ((C & To) >> nToSquare) << 2
														  | ((B & To) >> nToSquare) << 1
														  | ((A & To) >> nToSquare);

		// Update Hash
		hk ^= zobristKeys.zkPieceOnSquare[capturedpiece][nToSquare]; // Remove captured Piece
	}

	// Render "ordinary" moves:
	A &= O;
	B &= O;
	C &= O;
	D &= O;

	// Populate new square (Branchless method):
	A |= static_cast<int64_t>(M.piece & 1) << M.destination;
	B |= static_cast<int64_t>((M.piece & 2) >> 1) << M.destination;
	C |= static_cast<int64_t>((M.piece & 4) >> 2) << M.destination;
	D |= static_cast<int64_t>((M.piece & 8) >> 3) << M.destination;

	// Update Hash
	hk ^= zobristKeys.zkPieceOnSquare[M.piece][nFromSquare]; // Remove piece at From square
	hk ^= zobristKeys.zkPieceOnSquare[M.piece][nToSquare]; // Place piece at To Square

	// For double-pawn moves, set EP square:
	if (M.doublePawnMove) {
		// Set EnPassant Square
		if (M.blackToMove) {
			To <<= 8;
			A |= To;
			B |= To;
			C &= ~To;
			D |= To;
			hk ^= zobristKeys.zkPieceOnSquare[BENPASSANT][nToSquare + 8]; // Place Black EP at (To+8)
		} else {
			To >>= 8;
			A |= To;
			B |= To;
			C &= ~To;
			D &= ~To;
			hk ^= zobristKeys.zkPieceOnSquare[WENPASSANT][nToSquare - 8]; // Place White EP at (To-8)
		}
		return *this;
	}

	// En-Passant Captures
	if (M.enPassantCapture) {
		// remove the actual pawn (it is different to the capture square)
		if (M.blackToMove) {
			To <<= 8;
			A &= ~To; // clear the pawn's square
			B &= ~To;
			C &= ~To;
			D &= ~To;
			hk ^= zobristKeys.zkPieceOnSquare[WPAWN][nToSquare + 8]; // Remove WHITE Pawn at (To+8)
		} else {
			To >>= 8;
			A &= ~To;
			B &= ~To;
			C &= ~To;
			D &= ~To;
			hk ^= zobristKeys.zkPieceOnSquare[BPAWN][nToSquare - 8]; // Remove BLACK Pawn at (To-8)
		}
		return *this;
	}

	// Promotions - Change the piece:
	if (M.promoteQueen) {
		A &= ~To;
		B |= To;
		C |= To;
		hk ^= zobristKeys.zkPieceOnSquare[(M.blackToMove ? BPAWN : WPAWN)][nToSquare]; // Remove pawn at To square
		hk ^= zobristKeys.zkPieceOnSquare[(M.blackToMove ? BQUEEN : WQUEEN)][nToSquare]; // place Queen at To square
		return *this;
	}

	if (M.promoteKnight) {
		C |= To;
		hk ^= zobristKeys.zkPieceOnSquare[(M.blackToMove ? BPAWN : WPAWN)][nToSquare]; // Remove pawn at To square
		hk ^= zobristKeys.zkPieceOnSquare[(M.blackToMove ? BKNIGHT : WKNIGHT)][nToSquare]; // place Knight at To square
		return *this;
	}

	if (M.promoteBishop) {
		A &= ~To;
		B |= To;
		hk ^= zobristKeys.zkPieceOnSquare[(M.blackToMove ? BPAWN : WPAWN)][nToSquare]; // Remove pawn at To square
		hk ^= zobristKeys.zkPieceOnSquare[(M.blackToMove ? BBISHOP : WBISHOP)][nToSquare]; // place Bishop at To square
		return *this;
	}

	if (M.promoteRook) {
		A &= ~To;
		C |= To;
		hk ^= zobristKeys.zkPieceOnSquare[(M.blackToMove ? BPAWN : WPAWN)][nToSquare]; // Remove pawn at To square
		hk ^= zobristKeys.zkPieceOnSquare[(M.blackToMove ? BROOK : WROOK)][nToSquare];	// place Rook at To square
		return *this;
	}

	return *this;
}

ChessPosition& ChessPosition::performMoveNoHash(const ChessMove& M)
{
	Bitboard To = 1ull << M.destination;
	const Bitboard O = ~((1ull << M.origin) | To);
	const unsigned long nFromSquare = M.origin;

	// if move is known to be delivering checkmate, immediately flag checkmate in the position
	if (M.checkmate && M.blackToMove == BlackToMove) {
		if (M.blackToMove) {
			WhiteIsCheckmated = 1;
		} else {
			BlackIsCheckmated = 1;
		}
	}

	// clear any enpassant squares
	const Bitboard EnPassant = A & B & ~C;
	A &= ~EnPassant;
	B &= ~EnPassant;
	C &= ~EnPassant;
	D &= ~EnPassant;

	switch (M.piece) {
	case BKING:
		if (M.castle) {

			A ^= 0x0a00000000000000;
			B ^= 0x0a00000000000000;
			C ^= 0x0f00000000000000;
			D ^= 0x0f00000000000000;

			BlackDidCastle = 1;
			BlackCanCastle = 0;
			BlackCanCastleLong = 0;
			return *this;
		}

		if (M.castleLong) {
			A ^= 0x2800000000000000;
			B ^= 0x2800000000000000;
			C ^= 0xb800000000000000;
			D ^= 0xb800000000000000;

			BlackDidCastleLong = 1;
			BlackCanCastle = 0;
			BlackCanCastleLong = 0;
			return *this;
		}

		BlackForfeitedCastle = 1;
		BlackForfeitedCastleLong = 1;
		BlackCanCastle = 0;
		BlackCanCastleLong = 0;
		break;

	case WKING:
		if (M.castle) {
			A ^= 0x000000000000000a;
			B ^= 0x000000000000000a;
			C ^= 0x000000000000000f;
			D &= 0xfffffffffffffff0;	// clear colour of e1, f1, g1, h1 (make white)

			WhiteDidCastle = 1;
			WhiteCanCastle = 0;
			WhiteCanCastleLong = 0;
			return *this;
		}

		if (M.castleLong) {
			A ^= 0x0000000000000028;
			B ^= 0x0000000000000028;
			C ^= 0x00000000000000b8;
			D &= 0xffffffffffffff07;	// clear colour of a1, b1, c1, d1, e1 (make white)

			WhiteDidCastleLong = 1;
			WhiteCanCastle = 0;
			WhiteCanCastleLong = 0;
			return *this;
		}

		WhiteForfeitedCastle = 1;
		WhiteCanCastle = 0;
		WhiteForfeitedCastleLong = 1;
		WhiteCanCastleLong = 0;
		break;

	case BROOK:
		if (nFromSquare == SquareIndex::h8) {
			// Black moved K-side Rook and forfeits right to castle K-side
			if (BlackCanCastle){
				BlackForfeitedCastle = 1;
				BlackCanCastle = 0;
			}
		} else if (nFromSquare == SquareIndex::a8) {
			// Black moved the QS Rook and forfeits right to castle Q-side
			if (BlackCanCastleLong) {
				BlackForfeitedCastleLong = 1;
				BlackCanCastleLong = 0;
			}
		}
		break;

	case WROOK:
		if (nFromSquare == SquareIndex::h1) {
			// White moved K-side Rook and forfeits right to castle K-side
			if (WhiteCanCastle) {
				WhiteForfeitedCastle = 1;
				WhiteCanCastle = 0;
			}
		} else if (nFromSquare == SquareIndex::a1) {
			// White moved the QSide Rook and forfeits right to castle Q-side
			if (WhiteCanCastleLong) {
				WhiteForfeitedCastleLong = 1;
				WhiteCanCastleLong = 0;
			}
		}
		break;

	default:
		break;
	} // ends switch (M.Piece)

	// Render "ordinary" moves:
	A &= O;
	B &= O;
	C &= O;
	D &= O;

	// Populate new square (Branchless method):
	A |= static_cast<int64_t>(M.piece & 1) << M.destination;
	B |= static_cast<int64_t>((M.piece & 2) >> 1) << M.destination;
	C |= static_cast<int64_t>((M.piece & 4) >> 2) << M.destination;
	D |= static_cast<int64_t>((M.piece & 8) >> 3) << M.destination;

	// For double-pawn moves, set EP square:
	if (M.doublePawnMove) {
		// Set EnPassant Square
		if (M.blackToMove) {
			To <<= 8;
			A |= To;
			B |= To;
			C &= ~To;
			D |= To;
		} else {
			To >>= 8;
			A |= To;
			B |= To;
			C &= ~To;
			D &= ~To;
		}
		return *this;
	}

	// En-Passant Captures
	if (M.enPassantCapture) {
		// remove the actual pawn (it is different to the capture square)
		if (M.blackToMove) {
			To <<= 8;
			A &= ~To; // clear the pawn's square
			B &= ~To;
			C &= ~To;
			D &= ~To;
		} else {
			To >>= 8;
			A &= ~To;
			B &= ~To;
			C &= ~To;
			D &= ~To;
		}
		return *this;
	}

	// Promotions - Change the piece:
	if (M.promoteQueen) {
		A &= ~To;
		B |= To;
		C |= To;
		return *this;
	}

	if (M.promoteKnight) {
		C |= To;
		return *this;
	}

	if (M.promoteBishop) {
		A &= ~To;
		B |= To;
		return *this;
	}

	if (M.promoteRook) {
		A &= ~To;
		C |= To;
		return *this;
	}

	return *this;
}


std::vector<ChessMove> ChessPosition::getLegalMoves() const
{
	std::vector<ChessMove> movelist(MOVELIST_SIZE);
	generateMoves(*this, movelist.data());
	movelist.resize(movelist.at(0).moveCount);
	return movelist;
}

void ChessPosition::getLegalMoves(ChessMove *movelist) const
{
	if (movelist) {
		generateMoves(*this, movelist);
	}
}

void ChessPosition::switchSides()
{
	BlackToMove ^= 1;
	hk ^= zobristKeys.zkBlackToMove;
	return;
}

void ChessPosition::clear(void)
{
	A = B = C = D = 0;
	flags = 0;
	hk = 0;
}

////////////////////////////////////////////
// Move Generation Functions
// generateMoves()
////////////////////////////////////////////

void generateMoves(const ChessPosition& P, ChessMove* pM)
{
	assert((~(P.A | P.B | P.C) & P.D) == 0); // Should not be any "black" empty squares

#ifdef COUNT_MOVEGEN_CPU_CYCLES
	uint64_t cycles = __rdtsc();
#endif

	if (P.BlackToMove) {
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

inline bool isInCheck(const ChessPosition& P, bool bIsBlack)
{
	return bIsBlack ? isBlackInCheck(P) != 0 : isWhiteInCheck(P) != 0;
}

//////////////////////////////////////////
// White Move Generation Functions:     //
// generateWhiteMoves(),                //
// addWhiteMove(),                      //
// isWhiteInCheck(),                    //
// genBlackAttacks()                    //
//////////////////////////////////////////

void generateWhiteMoves(const ChessPosition& P, ChessMove* pM)
{
	if (P.WhiteIsCheckmated || P.WhiteIsStalemated) {
		pM->moveCount = 0;
		pM->origin = 0;
		pM->destination = 0;
		pM->piece = 0;
		pM->endOfMoveList = 1;
		return;
	}

	ChessMove* pFirstMove = pM;
	const Bitboard Occupied = P.A | P.B | P.C;								// all squares occupied by something
	const Bitboard BlackOccupied = Occupied & P.D;							// all squares occupied by B, including Black EP Squares
	const Bitboard WhiteFree // all squares where W is free to move
			= (P.A & P.B & ~P.C)        // any EP square
			  |	~(Occupied)				// any vacant square
			  |	(~P.A & P.D)			// Black Bishop, Rook or Queen
			  |	(~P.B & P.D);			// Black Pawn or Knight

	const Bitboard SolidBlackPiece = P.D & ~(P.A & P.B); // All black pieces except enpassants and black king

	unsigned long firstSq = 0;
	unsigned long lastSq = 63;

	const Bitboard WhiteOccupied = (Occupied & ~P.D) & ~(P.A & P.B & ~P.C);	// all squares occupied by W, excluding EP Squares
	assert(WhiteOccupied != 0);
	getFirstAndLastPiece(WhiteOccupied, firstSq, lastSq);

	for (unsigned int q = firstSq; q <= lastSq; q++) {
		const Bitboard fromSQ = 1ull << q;
		const piece_t piece = P.getPieceAtSquare(q);

		switch (piece) {
		case WEMPTY:
		case BEMPTY:
			continue;

		case WPAWN:
		{
			const Bitboard marchZone =  WhiteFree & ~BlackOccupied;
			const int step1 = MoveUpIndex[q];
			addWhiteMove(P, pM, q, step1, marchZone, WPAWN);   // single pawn advance
			if (fromSQ & RANK2 && ((1ull << step1) & marchZone)) {
				const int step2 = MoveUpIndex[step1];
				addWhiteMove(P, pM, q, step2, marchZone, WPAWN); // double pawn advance
			}

			const Bitboard captureZone = WhiteFree & BlackOccupied;  // area into which pawns may capture
			addWhiteMove(P, pM, q, MoveUpLeftIndex[q], captureZone, WPAWN); // capture left
			addWhiteMove(P, pM, q, MoveUpRightIndex[q], captureZone, WPAWN); // capture right
		}
			break;

		case WENPASSANT:
			continue;

		case WKNIGHT:
		{
			addWhiteMove(P, pM, q, MoveKnight1Index[q], WhiteFree, WKNIGHT);
			addWhiteMove(P, pM, q, MoveKnight2Index[q], WhiteFree, WKNIGHT);
			addWhiteMove(P, pM, q, MoveKnight3Index[q], WhiteFree, WKNIGHT);
			addWhiteMove(P, pM, q, MoveKnight4Index[q], WhiteFree, WKNIGHT);
			addWhiteMove(P, pM, q, MoveKnight5Index[q], WhiteFree, WKNIGHT);
			addWhiteMove(P, pM, q, MoveKnight6Index[q], WhiteFree, WKNIGHT);
			addWhiteMove(P, pM, q, MoveKnight7Index[q], WhiteFree, WKNIGHT);
			addWhiteMove(P, pM, q, MoveKnight8Index[q], WhiteFree, WKNIGHT);
		}
			break;

		case WKING:
		{
			addWhiteMove(P, pM, q, MoveLeftIndex[q], WhiteFree, WKING);
			addWhiteMove(P, pM, q, MoveUpLeftIndex[q], WhiteFree, WKING);
			addWhiteMove(P, pM, q, MoveUpIndex[q], WhiteFree, WKING);
			addWhiteMove(P, pM, q, MoveUpRightIndex[q], WhiteFree, WKING);
			addWhiteMove(P, pM, q, MoveRightIndex[q],  WhiteFree, WKING);
			addWhiteMove(P, pM, q, MoveDownRightIndex[q], WhiteFree, WKING);
			addWhiteMove(P, pM, q, MoveDownIndex[q], WhiteFree, WKING);
			addWhiteMove(P, pM, q, MoveDownLeftIndex[q], WhiteFree, WKING);
		}
			break;

		case WBISHOP:
		case WROOK:
		case WQUEEN:
		{
			const Bitboard B = P.B & fromSQ;
			const Bitboard C = P.C & fromSQ;

			if (B != 0) {
				/* diagonal slider (either B or Q) */
				std::bitset<64> bs = getDiagonalMoveSquares(fromSQ, WhiteFree, SolidBlackPiece);
				BITSET_LOOP(addWhiteMove(P, pM, q, bit, WhiteFree, piece))
			}

			if (C != 0) {
				/* straight slider (either R or Q) */
				std::bitset<64> bs =  getStraightMoveSquares(fromSQ, WhiteFree, SolidBlackPiece);
				BITSET_LOOP(addWhiteMove(P, pM, q, bit, WhiteFree, piece))
			}
		}
			break;

		default:
			continue;

		} // ends switch

		if (P.DontGenerateAllMoves && pM > pFirstMove) {
			break;
		}

	} // ends loop over q

	// castling
	if (P.A & P.B & P.C & ~P.D & E1) { // King still in original position

		// Conditionally generate O-O move:
		if (P.WhiteCanCastle && // White still has castle rights
				(~P.A & ~P.B & P.C & ~P.D & H1) && // Kingside rook is in correct position
				(WHITECASTLEZONE & Occupied) == 0 && // Castle Zone (f1, g1) is clear
				!isWhiteInCheck(P, WHITECASTLECHECKZONE)) // King is not in Check (in e1, f1, g1)
		{
			ChessPosition Q = P;
			pM->piece = WKING;
			pM->origin = static_cast<unsigned char>(SquareIndex::e1);
			pM->destination = static_cast<unsigned char>(SquareIndex::g1);
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
		if (P.WhiteCanCastleLong && // White still has castle-long rights
				(~P.A & ~P.B & P.C & ~P.D & A1) && // Queenside rook is in correct Position
				(WHITECASTLELONGZONE & Occupied) == 0 && // Castle-long zone (b1, c1, d1) is clear
				!isWhiteInCheck(P, WHITECASTLELONGCHECKZONE)) // King is not in check (in e1, d1, c1)
		{
			// Ok to Castle Long
			ChessPosition Q = P;
			pM->piece = WKING;
			pM->origin = static_cast<unsigned char>(SquareIndex::e1);
			pM->destination = static_cast<unsigned char>(SquareIndex::c1);
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


	// put the move count into the first move
	pFirstMove->moveCount = pM - pFirstMove;

	// Create 'no more moves' move to mark end of list
	pM->origin = 0;
	pM->destination = 0;
	pM->piece = 0;
	pM->endOfMoveList = 1;
}

inline void addWhiteMove(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, unsigned char tosquare, Bitboard F, int32_t piece)
{
	if (tosquare > 63) {
		return;
	}

	const Bitboard to = (1ull << tosquare) & F;
	if (to == 0) {
		return;
	}

	const Bitboard from = (1ull << fromsquare);

	pM->origin = fromsquare;
	pM->destination = tosquare;
	pM->flags = 0;
	pM->blackToMove = 0;
	pM->piece = piece;

	// Test for capture:
	const Bitboard PAB = P.A & P.B;	// Bitboard containing EnPassants and kings:
	const Bitboard BlackOccupied = P.D;
	if (to & BlackOccupied & ~PAB) {
		// Only considered a capture if dest is not an enpassant or king.
		pM->capture = 1;
	}

	bool promote = false;
	if (piece == WPAWN) {
		if (from & RANK2 && to & RANK4) {
			pM->doublePawnMove = 1;
		} else if (to & BlackOccupied & PAB & ~P.C) {
			pM->enPassantCapture = 1;
		} else if (to & RANK8) {
			promote = true;
		}
	}

	ChessPosition Q = P;

	// clear old and new square:
	const Bitboard O = ~(from | to);
	Q.A &= O;
	Q.B &= O;
	Q.C &= O;
	Q.D &= O;

	// Populate new square with piece
	Q.A |= static_cast<int64_t>(piece & 1) << tosquare;
	Q.B |= static_cast<int64_t>((piece & 2) >> 1) << tosquare;
	Q.C |= static_cast<int64_t>((piece & 4) >> 2) << tosquare;
	Q.D |= static_cast<int64_t>((piece & 8) >> 3) << tosquare;

	if (pM->enPassantCapture) {
		// remove the actual pawn (dest was EP square)
		const Bitboard x = to >> 8;
		Q.A &= ~x;
		Q.B &= ~x;
		Q.C &= ~x;
		Q.D &= ~x;
	}

	// test if doing all this puts white in check. If so, move isn't legal
	if (isWhiteInCheck(Q)) {
		pM->illegalMove = 1;
		return;
	}

	if (promote) {
		// make an additional 3 copies for the underpromotions
		*(pM + 1) = *pM;
		*(pM + 2) = *pM;
		*(pM + 3) = *pM;

		// promote to queen
		pM->promoteQueen = 1;
		Q.A &= ~to;
		Q.B |= to;
		Q.C |= to;
		scanWhiteMoveForChecks(Q, pM);
		pM++;

		// rook underpromotion
		pM->promoteRook = 1;
		Q.B &= ~to;
		scanWhiteMoveForChecks(Q, pM);
		pM++;

		// bishop underpromotion
		pM->promoteBishop = 1;
		Q.C &= ~to;
		Q.B |= to;
		scanWhiteMoveForChecks(Q, pM);
		pM++;

		// knight underpromotion
		pM->promoteKnight = 1;
		Q.A |= to;
		Q.B &= ~to;
		Q.C |= to;
	}

	scanWhiteMoveForChecks(Q, pM);
	pM++; // Add to list (advance pointer)
	pM->flags = 0;
}


inline Bitboard isWhiteInCheck(const ChessPosition& Z, Bitboard extend)
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
	X |= FillKnightAttacks(N);
	X |= MoveDownLeftRightSingle(P);
	return X & WhiteKing;
}

inline void scanWhiteMoveForChecks(ChessPosition& Q, ChessMove* pM)
{
	// test if white's move will put black in check or checkmate
	if (isBlackInCheck(Q))	{
		pM->check = 1;
		Q.BlackToMove = 1;
		ChessMove blacksMoves[MOVELIST_SIZE];
		Q.DontGenerateAllMoves = 1; // only need one or more moves to prove that black has at least one legal move
		generateBlackMoves(Q, blacksMoves);
		if (blacksMoves->moveCount == 0) { // black will be in check with no legal moves
			pM->checkmate = 1; // this move is a checkmating move
		}
	} else {
		pM->check = 0;
		pM->checkmate = 0;
	}
}


//////////////////////////////////////////
// Black Move Generation Functions:     //
// generateBlackMoves(),                //
// addBlackMove(),                      //
// isBlackInCheck()                     //
// scanBlackMoveForChecks()             //
//////////////////////////////////////////

void generateBlackMoves(const ChessPosition& P, ChessMove* pM)
{
	if (P.BlackIsCheckmated || P.BlackIsStalemated) {
		pM->moveCount = 0;
		pM->origin = 0;
		pM->destination = 0;
		pM->piece = 0;
		pM->endOfMoveList = 1;
		return;
	}

	ChessMove* pFirstMove = pM;
	const Bitboard Occupied = P.A | P.B | P.C; // all squares occupied by something

	const Bitboard WhiteOccupied = (Occupied & ~P.D); // all squares occupied by W, including white EP Squares
	const Bitboard BlackFree // all squares where B is free to move
			= (P.A & P.B & ~P.C)		// any EP square
			  | ~(Occupied)				// any vacant square
			  | (~P.A & ~P.D)			// White Bishop, Rook or Queen
			  | (~P.B & ~P.D);			// White Pawn or Knight

	const Bitboard SolidWhitePiece = WhiteOccupied & ~(P.A & P.B); // All white pieces except enpassants and white king

	unsigned long firstSq = 0;
	unsigned long lastSq = 63;

	const Bitboard BlackOccupied = P.D & ~(P.A & P.B & ~P.C); // all squares occupied by B, excluding EP Squares
	assert(BlackOccupied != 0);
	getFirstAndLastPiece(BlackOccupied, firstSq, lastSq);

	for (unsigned int q = firstSq; q <= lastSq; q++) {
		const Bitboard fromSQ = 1ull << q;
		const piece_t piece = P.getPieceAtSquare(q);

		switch (piece) {
		case WEMPTY:
		case BEMPTY:
			continue;

		case BPAWN:
		{
			const Bitboard marchZone = BlackFree & ~WhiteOccupied; // area into which pawns may advance
			const int step1 = MoveDownIndex[q];
			addBlackMove(P, pM, q, step1, marchZone, BPAWN);   // single pawn advance
			if (fromSQ & RANK7 && ((1ull << step1) & marchZone)) {
				const int step2 = MoveDownIndex[step1];
				addBlackMove(P, pM, q, step2, marchZone, BPAWN); // double pawn advance
			}

			const Bitboard captureZone = BlackFree & WhiteOccupied;  // area into which pawns may capture
			addBlackMove(P, pM, q, MoveDownLeftIndex[q], captureZone, BPAWN); // capture left
			addBlackMove(P, pM, q, MoveDownRightIndex[q], captureZone, BPAWN); // capture right
		}
			break;

		case BENPASSANT:
			continue;

		case BKNIGHT:
		{
			addBlackMove(P, pM, q, MoveKnight1Index[q], BlackFree, BKNIGHT);
			addBlackMove(P, pM, q, MoveKnight2Index[q], BlackFree, BKNIGHT);
			addBlackMove(P, pM, q, MoveKnight3Index[q], BlackFree, BKNIGHT);
			addBlackMove(P, pM, q, MoveKnight4Index[q], BlackFree, BKNIGHT);
			addBlackMove(P, pM, q, MoveKnight5Index[q], BlackFree, BKNIGHT);
			addBlackMove(P, pM, q, MoveKnight6Index[q], BlackFree, BKNIGHT);
			addBlackMove(P, pM, q, MoveKnight7Index[q], BlackFree, BKNIGHT);
			addBlackMove(P, pM, q, MoveKnight8Index[q], BlackFree, BKNIGHT);
		}
			break;

		case BKING:
		{
			addBlackMove(P, pM, q, MoveLeftIndex[q], BlackFree, BKING);
			addBlackMove(P, pM, q, MoveUpLeftIndex[q], BlackFree, BKING);
			addBlackMove(P, pM, q, MoveUpIndex[q], BlackFree, BKING);
			addBlackMove(P, pM, q, MoveUpRightIndex[q], BlackFree, BKING);
			addBlackMove(P, pM, q, MoveRightIndex[q],  BlackFree, BKING);
			addBlackMove(P, pM, q, MoveDownRightIndex[q], BlackFree, BKING);
			addBlackMove(P, pM, q, MoveDownIndex[q], BlackFree, BKING);
			addBlackMove(P, pM, q, MoveDownLeftIndex[q], BlackFree, BKING);
		}
			break;

		case BBISHOP:
		case BROOK:
		case BQUEEN:
		{
			const Bitboard B = P.B & fromSQ;
			const Bitboard C = P.C & fromSQ;

			if (B != 0) {
				/* diagonal slider (either B or Q) */
				std::bitset<64> bs = getDiagonalMoveSquares(fromSQ, BlackFree, SolidWhitePiece);
				BITSET_LOOP(addBlackMove(P, pM, q, bit, BlackFree, piece))
			}

			if (C != 0) {
				/* straight slider (either R or Q) */
				std::bitset<64> bs = getStraightMoveSquares(fromSQ, BlackFree, SolidWhitePiece);
				BITSET_LOOP(addBlackMove(P, pM, q, bit, BlackFree, piece))
			}
		}
			break;

		default:
			continue;

		} // ends switch

		if (P.DontGenerateAllMoves && pM > pFirstMove) {
			break;
		}

	} // ends loop over q;

	// castling
	if (P.A & P.B & P.C & P.D & E8) { // King still in original position

		// Conditionally generate O-O move:
		if (P.BlackCanCastle && // Black still has castle rights
				(~P.A & ~P.B & P.C & P.D & H8) && // Kingside rook is in correct position
				(BLACKCASTLEZONE & Occupied) == 0 && // Castle Zone (f8, g8) is clear
				!isBlackInCheck(P, BLACKCASTLECHECKZONE)) // King is not in Check (in e8, f8, g8)
		{
			ChessPosition Q = P;
			pM->piece = BKING;
			pM->origin = static_cast<unsigned char>(SquareIndex::e8);
			pM->destination = static_cast<unsigned char>(SquareIndex::g8);
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
		if (P.BlackCanCastleLong && // Black still has castle-long rights
				(~P.A & ~P.B & P.C & P.D & A8) && // Queenside rook is in correct Position
				(BLACKCASTLELONGZONE & Occupied) == 0 && // Castle Long Zone (b8, c8, d8) is clear
				!isBlackInCheck(P, BLACKCASTLELONGCHECKZONE)) // King is not in Check (e8, d8, c8)
		{
			ChessPosition Q = P;
			pM->piece = BKING;
			pM->origin = static_cast<unsigned char>(SquareIndex::e8);
			pM->destination = static_cast<unsigned char>(SquareIndex::c8);
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

	// put the move count into the first move
	pFirstMove->moveCount = pM - pFirstMove;

	// Create 'no more moves' move to mark end of list
	pM->origin = 0;
	pM->destination = 0;
	pM->piece = 0;
	pM->endOfMoveList = 1;
}

inline void addBlackMove(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, unsigned char tosquare, Bitboard F, int32_t piece)
{
	if (tosquare > 63) {
		return;
	}

	const Bitboard to = (1ull << tosquare) & F;
	if (to == 0) {
		return;
	}

	const Bitboard from = (1ull << fromsquare);

	pM->origin = fromsquare;
	pM->destination = tosquare;
	pM->flags = 0;
	pM->blackToMove = 1;
	pM->piece = piece;

	// Test for capture:
	const Bitboard PAB = P.A & P.B;	// Bitboard containing EnPassants and kings
	Bitboard WhiteOccupied = (P.A | P.B | P.C) & ~P.D;
	if (to & WhiteOccupied & ~PAB) {
		// Only considered a capture if dest is not an enpassant or king.
		pM->capture = 1;
	}

	bool promote = false;
	if (piece == BPAWN) {
		if (from & RANK7 && to & RANK5) {
			pM->doublePawnMove = 1;
		} else if (to & WhiteOccupied & PAB & ~P.C) {
			pM->enPassantCapture = 1;
		} else if (to & RANK1) {
			promote = true;
		}
	}

	ChessPosition Q = P;

	// clear old and new square
	const Bitboard O = ~(from | to);
	Q.A &= O;
	Q.B &= O;
	Q.C &= O;
	Q.D &= O;

	// Populate new square with piece
	Q.A |= static_cast<int64_t>(piece & 1) << tosquare;
	Q.B |= static_cast<int64_t>((piece & 2) >> 1) << tosquare;
	Q.C |= static_cast<int64_t>((piece & 4) >> 2) << tosquare;
	Q.D |= static_cast<int64_t>((piece & 8) >> 3) << tosquare;

	if (pM->enPassantCapture) {
		// remove the actual pawn (dest was EP square)
		const Bitboard x = to << 8;
		Q.A &= ~x;
		Q.B &= ~x;
		Q.C &= ~x;
		Q.D &= ~x;
	}

	// test if doing all this puts black in check. If so, move isn't legal
	if (isBlackInCheck(Q)) {
		pM->illegalMove = 1;
		return;
	}

	if (promote) {
		// make an additional 3 copies for underpromotions
		*(pM + 1) = *pM;
		*(pM + 2) = *pM;
		*(pM + 3) = *pM;

		// promote to queen
		pM->promoteQueen = 1;
		Q.A &= ~to;
		Q.B |= to;
		Q.C |= to;
		Q.D |= to;
		scanBlackMoveForChecks(Q, pM);
		pM++;

		// rook underpromotion
		pM->promoteRook = 1;
		Q.B &= ~to;
		scanBlackMoveForChecks(Q, pM);
		pM++;

		// bishop underpromotion
		pM->promoteBishop = 1;
		Q.C &= ~to;
		Q.B |= to;
		scanBlackMoveForChecks(Q, pM);
		pM++;

		// knight underpromotion
		pM->promoteKnight = 1;
		Q.A |= to;
		Q.B &= ~to;
		Q.C |= to;
	}

	scanBlackMoveForChecks(Q, pM);
	pM++; // Add to list (advance pointer)
	pM->flags = 0;
}

inline Bitboard isBlackInCheck(const ChessPosition& Z, Bitboard extend)
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
	X |= FillKnightAttacks(N);
	X |= MoveUpLeftRightSingle(P);

	return X & BlackKing;
}

inline void scanBlackMoveForChecks(ChessPosition& Q, ChessMove* pM)
{
	// test if black's move will put white in check or checkmate
	if (isWhiteInCheck(Q))	{
		pM->check = 1;
		Q.BlackToMove = 0;
		ChessMove whitesMoves[MOVELIST_SIZE];
		Q.DontGenerateAllMoves = 1; // only need one or more moves to prove that white has at least one legal move
		generateWhiteMoves(Q, whitesMoves);
		if (whitesMoves->moveCount == 0) { // white will be in check with no legal moves
			pM->checkmate = 1; // this move is a checkmating move
		}
	} else {
		pM->check = 0;
		pM->checkmate = 0;
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

// Text Dump functions //////////////////

void dumpBitBoard(Bitboard b)
{
	std::cout << "\n";
	for (int q = 63; q >= 0; q--)
	{
		if ((b & (1ull << q)) != 0)
		{
			std::cout << ("o ");
		}
		else
		{
			std::cout << (". ");
		}
		if ((q % 8) == 0)
		{
			std::cout << ("\n");
		}
	}
	std::cout << std::flush;
}

void dumpChessPosition(ChessPosition p)
{
	printf("\n---------------------------------\n");
	for (int q = 63; q >= 0; q--)
	{
		switch(p.getPieceAtSquare(q))
		{
		case WPAWN:
			printf("| P ");
			break;
		case WBISHOP:
			printf("| B ");
			break;
		case WROOK:
			printf("| R ");
			break;
		case WQUEEN:
			printf("| Q ");
			break;
		case WKING:
			printf("| K ");
			break;
		case WKNIGHT:
			printf("| N ");
			break;
		case WENPASSANT:
			printf("| EP");
			break;
		case BPAWN:
			printf("| p ");
			break;
		case BBISHOP:
			printf("| b ");
			break;
		case BROOK:
			printf("| r ");
			break;
		case BQUEEN:
			printf("| q ");
			break;
		case BKING:
			printf("| k ");
			break;
		case BKNIGHT:
			printf("| n ");
			break;
		case BENPASSANT:
			printf("| ep");
			break;
		default:
			printf("|   ");
		}

		if ((q % 8) == 0)
		{
			printf("|   ");
			if ((q == 56) && (p.BlackCanCastle))
				printf("Black can Castle");
			if ((q == 48) && (p.BlackCanCastleLong))
				printf("Black can Castle Long");
			if ((q == 40) && (p.WhiteCanCastle))
				printf("White can Castle");
			if ((q == 32) && (p.WhiteCanCastleLong))
				printf("White can Castle Long");
			if (q == 8) printf("material= %d", p.calculateMaterial());
			if (q == 0) printf("%s to move", p.BlackToMove ? "Black" : "White");
			printf("\n---------------------------------\n");
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////
// dumpMove() - Converts a move to ascii and dumps it to stdout, unless a character
// buffer is supplied, in which case the ascii is written to pBuffer instead.
// A number of notation styles are supported. Usually, CoOrdinate should be used
// for WinBoard.
//////////////////////////////////////////////////////////////////////////////////
void dumpMove(const ChessMove& mv, MoveNotationStyle style /* = LongAlgebraic */, char* pBuffer, const ChessMove *movelist)
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

void dumpMoveList(ChessMove* pMoveList, MoveNotationStyle style /* = LongAlgebraic */, char* pBuffer /*=nullptr*/)
{
	ChessMove* pM = pMoveList;
	unsigned int i = 0;
	do{
		if (pM->destination != pM->origin) {
			dumpMove(*pM, style, pBuffer, pMoveList);
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
