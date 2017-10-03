/*

MIT License

Copyright(c) 2016-2017 Judd Niemann

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
#include "fen.h"
#include "hashtable.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <cassert>

namespace juddperft {

//////////////////////////////////////
// Implementation of Class Move		//
//////////////////////////////////////

Move::Move(BitBoard From/*=0*/,BitBoard To/*=0*/,uint32_t Flags/*=0*/)
{
	Move::From=From;
	Move::To=To;
	Move::Flags=Flags;
}

bool Move::operator==(const Move& B) const
{
	return	(Move::From==B.From)	&&
		(Move::To==B.To)		&&
		(Move::Piece==B.Piece);
	
	// Note Move::Flags doesn't need to be the same 
	// for two moves to be considered equal.
	// This is because the equality test is primarily used
	// to verify inputted moves against a list of legal moves
	// and there is no need to go to all the trouble of setting the flags
	// on both sides.
}

Move& Move::Format(
		BitBoard From /*=0*/,
		BitBoard To /*=0*/,
		uint32_t BlackToMove /*=0*/,
		uint32_t Piece /*=0*/, 
		uint32_t Flags /*=0*/
		)
{
	Move::Flags=Flags;
	Move::From=From;
	Move::To=To;
	Move::BlackToMove=BlackToMove;
	Move::Piece=Piece;
	return *this;
}

//////////////////////////////////////////////
// Implementation of Class ChessPosition	//
//////////////////////////////////////////////

ChessPosition::ChessPosition()
{
	ChessPosition::A =0;
	ChessPosition::B =0;
	ChessPosition::C =0;
	ChessPosition::D =0;
	ChessPosition::Flags=0;
	ChessPosition::material = 0;
#ifdef _USE_HASH
	ChessPosition::HK = 0;
#endif
}

ChessPosition& ChessPosition::SetupStartPosition()
{
	ChessPosition::Flags = 0;
	ChessPosition::A = 0x4Aff00000000ff4A;
	ChessPosition::B = 0x3C0000000000003C;
	ChessPosition::C = 0xDB000000000000DB;
	ChessPosition::D = 0xffff000000000000;
	ChessPosition::BlackCanCastle=1;
	ChessPosition::WhiteCanCastle=1;
	ChessPosition::BlackCanCastleLong=1;
	ChessPosition::WhiteCanCastleLong=1;
	ChessPosition::BlackForfeitedCastle=0;
	ChessPosition::WhiteForfeitedCastle=0;
	ChessPosition::BlackForfeitedCastleLong=0;
	ChessPosition::WhiteForfeitedCastleLong=0;
	ChessPosition::BlackDidCastle=0;
	ChessPosition::WhiteDidCastle=0;
	ChessPosition::BlackDidCastleLong=0;
	ChessPosition::WhiteDidCastleLong=0;
	ChessPosition::MoveNumber=1;
	ChessPosition::HalfMoves=0;
	ChessPosition::CalculateMaterial();
#ifdef _USE_HASH
	ChessPosition::CalculateHash();
#endif
	return *this;
}

#ifdef _USE_HASH
ChessPosition & ChessPosition::CalculateHash()
{
	ChessPosition::HK = 0;
	
	// Scan the squares:
	for (int q = 0; q < 64; q++)
	{
		unsigned int piece = GetPieceAtSquare(1LL<<q);
		//assert(piece != 0x08);
		if(piece & 0x7)
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[piece][q];
	}
	if (ChessPosition::BlackToMove)	ChessPosition::HK ^= ZobristKeys.zkBlackToMove;
	if (ChessPosition::WhiteCanCastle) ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastle;
	if (ChessPosition::WhiteCanCastleLong) ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastleLong;
	if (ChessPosition::BlackCanCastle) ChessPosition::HK ^= ZobristKeys.zkBlackCanCastle;
	if (ChessPosition::BlackCanCastleLong) ChessPosition::HK ^= ZobristKeys.zkBlackCanCastleLong;  
	
	return *this;
}
#endif

ChessPosition& ChessPosition::SetPieceAtSquare(const unsigned int& piece /*=0*/, BitBoard square)
{
	// clear the square
	ChessPosition::A &= ~square;
	ChessPosition::B &= ~square;
	ChessPosition::C &= ~square;
	ChessPosition::D &= ~square;
	// 'install' the piece
	if (piece & 1)
	{
		ChessPosition::A |= square;
	}
	if (piece & 2)
	{
		ChessPosition::B |= square;
	}
	if (piece & 4)
	{
		ChessPosition::C |= square;
	}
	if (piece & 8)
	{
		ChessPosition::D |= square;
	}
	ChessPosition::CalculateMaterial();
#ifdef _USE_HASH
	ChessPosition::CalculateHash();
#endif
	return *this;
}

uint32_t ChessPosition::GetPieceAtSquare(const BitBoard& square) const
{
	uint32_t V;
	V = (ChessPosition::D & square)?8:0;
	V |=(ChessPosition::C & square)?4:0;
	V |=(ChessPosition::B & square)?2:0;
	V |=(ChessPosition::A & square)?1:0;
	return V;
}

ChessPosition& ChessPosition::CalculateMaterial()
{
	ChessPosition::material=0;
	BitBoard M;
	BitBoard V;

	for(int q=0;q<64;q++)
	{
		M=1LL << q;
		V = (ChessPosition::D & M) >> q;
		V <<= 1;
		V |=(ChessPosition::C & M) >> q;
		V <<= 1;
		V |=(ChessPosition::B & M) >> q;
		V <<= 1;
		V |=(ChessPosition::A & M) >> q;
		switch(V)
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
	return *this;
}

ChessPosition& ChessPosition::PerformMove(ChessMove M)
{
	BitBoard O, To;
	To = 1LL << M.ToSquare;
	O = ~((1LL << M.FromSquare) | To);
	unsigned long nFromSquare = M.FromSquare;
	unsigned long nToSquare = M.ToSquare;

	// CLEAR EP SQUARES :
	// clear any enpassant squares
	
	BitBoard EnPassant=A & B & (~C);
	ChessPosition::A &= ~EnPassant;
	ChessPosition::B &= ~EnPassant;
	ChessPosition::C &= ~EnPassant;
	ChessPosition::D &= ~EnPassant;

#ifdef _USE_HASH
	unsigned long nEPSquare;
	nEPSquare = GetSquareIndex(EnPassant);
	ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WENPASSANT][nEPSquare]; // Remove EP from nEPSquare
#endif // _USE_HASH

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
	// D &= 0xfffffffffffffff0 (ie Clear colour of affected squares from K to KR)

	// The following is for O-O-O:
	//
	//    R... K...                      ..KR ....
	// A: 0000 1000 ^ 0010 1000 (0x28) = 0010 0000
	// B: 0000 1000 ^ 0010 1000 (0x28) = 0010 0000
	// C: 1000 1000 ^ 1011 1000 (0xB8) = 0011 0000
	// For Black:
	// D: 1000 1000 ^ 1011 1000 (0xB8) = 0011 0000
	// For White:
	// D &= 0xffffffffffffff07 (ie Clear colour of affected squares from QR to K) 

	if(M.Piece == BKING)
	{
		if(M.Castle)
		{
			ChessPosition::A ^= 0x0a00000000000000;
			ChessPosition::B ^= 0x0a00000000000000;
			ChessPosition::C ^= 0x0f00000000000000;
			ChessPosition::D ^= 0x0f00000000000000;
#ifdef _USE_HASH
			HK ^= ZobristKeys.zkDoBlackCastle;
			if (ChessPosition::BlackCanCastleLong) ChessPosition::HK ^= ZobristKeys.zkBlackCanCastleLong;	// conditionally flip black castling long
#endif
			ChessPosition::BlackDidCastle = 1;
			ChessPosition::BlackCanCastle = 0;
			ChessPosition::BlackCanCastleLong = 0;
			return *this;
		}
		else if(M.CastleLong)
		{
			ChessPosition::A ^= 0x2800000000000000;
			ChessPosition::B ^= 0x2800000000000000;
			ChessPosition::C ^= 0xb800000000000000;
			ChessPosition::D ^= 0xb800000000000000;
#ifdef _USE_HASH
			HK ^= ZobristKeys.zkDoBlackCastleLong;
			if (ChessPosition::BlackCanCastle) ChessPosition::HK ^= ZobristKeys.zkBlackCanCastle;	// conditionally flip black castling
#endif
			ChessPosition::BlackDidCastleLong = 1;
			ChessPosition::BlackCanCastle = 0;
			ChessPosition::BlackCanCastleLong = 0;
			return *this;
		}
		else
		{
			// ordinary king move
			if (ChessPosition::BlackCanCastle || ChessPosition::BlackCanCastleLong)
			{
				// Black could have castled, but chose to move the King in a non-castling move
#ifdef _USE_HASH
				if(ChessPosition::BlackCanCastle) ChessPosition::HK ^= ZobristKeys.zkBlackCanCastle;	// conditionally flip black castling
				if(ChessPosition::BlackCanCastleLong) ChessPosition::HK ^= ZobristKeys.zkBlackCanCastleLong;	// conditionally flip black castling long
#endif		
				ChessPosition::BlackForfeitedCastle = 1;
				ChessPosition::BlackForfeitedCastleLong = 1;
				ChessPosition::BlackCanCastle = 0;
				ChessPosition::BlackCanCastleLong = 0;
			}		
		}
	}

	else if (M.Piece == WKING)
	{
		// White
		if(M.Castle)
		{
			ChessPosition::A ^= 0x000000000000000a;
			ChessPosition::B ^= 0x000000000000000a;
			ChessPosition::C ^= 0x000000000000000f;
			ChessPosition::D &= 0xfffffffffffffff0;	// clear colour of e1,f1,g1,h1 (make white)
#ifdef _USE_HASH
			HK^=ZobristKeys.zkDoWhiteCastle;
			if (ChessPosition::WhiteCanCastleLong) ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastleLong;	// conditionally flip white castling long
#endif
			ChessPosition::WhiteDidCastle = 1;
			ChessPosition::WhiteCanCastle = 0;
			ChessPosition::WhiteCanCastleLong = 0;
			return *this;
		}
		
		if(M.CastleLong)
		{
			
			ChessPosition::A ^= 0x0000000000000028;
			ChessPosition::B ^= 0x0000000000000028;
			ChessPosition::C ^= 0x00000000000000b8;
			ChessPosition::D &= 0xffffffffffffff07;	// clear colour of a1,b1,c1,d1,e1 (make white)
#ifdef _USE_HASH
			HK^=ZobristKeys.zkDoWhiteCastleLong;
			if (ChessPosition::WhiteCanCastle) ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastle;	// conditionally flip white castling
			
#endif
			ChessPosition::WhiteDidCastleLong = 1;
			ChessPosition::WhiteCanCastle = 0;
			ChessPosition::WhiteCanCastleLong=0;
			return *this;
		}
		
		else
		{
			// ordinary king move
			if (ChessPosition::WhiteCanCastle) {
#ifdef _USE_HASH
				ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastle;	// flip white castling
#endif					
				ChessPosition::WhiteForfeitedCastle = 1;
				ChessPosition::WhiteCanCastle = 0;

			}
			if (ChessPosition::WhiteCanCastleLong) {
#ifdef _USE_HASH
				ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastleLong;	// flip white castling long
#endif
				ChessPosition::WhiteForfeitedCastleLong = 1;
				ChessPosition::WhiteCanCastleLong = 0;
			}
		}
	}	
			
	// LOOK FOR FORFEITED CASTLING RIGHTS DUE to ROOK MOVES:
	else if(M.Piece == BROOK)
	{	
	//	if((1LL<<nFromSquare) & BLACKKRPOS)
		if(nFromSquare==56)
		{
			// Black moved K-side Rook and forfeits right to castle K-side
			if(ChessPosition::BlackCanCastle){
				ChessPosition::BlackForfeitedCastle=1;
#ifdef _USE_HASH
				ChessPosition::HK ^= ZobristKeys.zkBlackCanCastle;	// flip black castling		
#endif
				ChessPosition::BlackCanCastle=0;
			}
		}
		//else if ((1LL<<nFromSquare) & BLACKQRPOS)
		else if (nFromSquare==63)
		{
			// Black moved the QS Rook and forfeits right to castle Q-side
			if (ChessPosition::BlackCanCastleLong) {
				ChessPosition::BlackForfeitedCastleLong = 1;
#ifdef _USE_HASH
				ChessPosition::HK ^= ZobristKeys.zkBlackCanCastleLong;	// flip black castling long
#endif
				ChessPosition::BlackCanCastleLong=0;
			}
		}
	}
	
	else if(M.Piece == WROOK)
	{
		//if((1LL<<nFromSquare) & WHITEKRPOS)
		if (nFromSquare==0)
		{
			// White moved K-side Rook and forfeits right to castle K-side
			if (ChessPosition::WhiteCanCastle) {
				ChessPosition::WhiteForfeitedCastle = 1;
#ifdef _USE_HASH
				ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastle;	// flip white castling BROKEN !!!
#endif
				ChessPosition::WhiteCanCastle=0;
			}
		}
	//	else if((1LL<<nFromSquare) & WHITEQRPOS)
		else if (nFromSquare==7)
		{
			// White moved the QSide Rook and forfeits right to castle Q-side
			if (ChessPosition::WhiteCanCastleLong) {
				ChessPosition::WhiteForfeitedCastleLong = 1;
#ifdef _USE_HASH
				ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastleLong;	// flip white castling long
#endif
				ChessPosition::WhiteCanCastleLong=0;
			}
		}
	}
			
	// Ordinary Captures	////
	if (M.Capture)
	{
		int capturedpiece;

#if defined (_WIN64) && defined (_USE_BITTEST_INSTRUCTION)
		const long long d = ChessPosition::D;
		const long long c = ChessPosition::C;
		const long long b = ChessPosition::B;
		const long long a = ChessPosition::A;

		// using BitTest Intrinsic:
		capturedpiece = _bittest64(&d, nToSquare) << 3
			| _bittest64(&c, nToSquare) << 2
			| _bittest64(&b, nToSquare) << 1
			| _bittest64(&a, nToSquare);		
#else
		// find out which piece has been captured:

		// Branchy version (reliable, maybe a tiny bit slower):
		//capturedpiece = (ChessPosition::D & (1LL<<nToSquare))?8:0;
		//capturedpiece |=(ChessPosition::C & (1LL<<nToSquare))?4:0;
		//capturedpiece |=(ChessPosition::B & (1LL<<nToSquare))?2:0;
		//capturedpiece |=(ChessPosition::A & (1LL<<nToSquare))?1:0;

		// Branchless version:
		capturedpiece = ((ChessPosition::D & To) >> nToSquare) << 3
			| ((ChessPosition::C & To) >> nToSquare) << 2
			| ((ChessPosition::B & To) >> nToSquare) << 1
			| ((ChessPosition::A & To) >> nToSquare);
#endif

#ifdef _USE_HASH
		// Update Hash
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[capturedpiece][nToSquare]; // Remove captured Piece
#endif
	}
	
	// Render "ordinary" moves:
	ChessPosition::A &= O;
	ChessPosition::B &= O;
	ChessPosition::C &= O;
	ChessPosition::D &= O;

	// Populate new square (Branchless method):
	ChessPosition::A |= static_cast<int64_t>(M.Piece & 1) << M.ToSquare;
	ChessPosition::B |= static_cast<int64_t>((M.Piece & 2) >> 1) << M.ToSquare;
	ChessPosition::C |= static_cast<int64_t>((M.Piece & 4) >> 2) << M.ToSquare;
	ChessPosition::D |= static_cast<int64_t>((M.Piece & 8) >> 3) << M.ToSquare;
	
	// Populate new square (Branchy):
	//if (M.Piece & 1)
	//	ChessPosition::A |=  To;
	//if (M.Piece & 2)
	//	ChessPosition::B |=  To;
	//if (M.Piece & 4)
	//	ChessPosition::C |=  To;
	//if (M.Piece & 8)
	//	ChessPosition::D |=  To;
	
	//// Populate new square (Branchless method, BitBoard input):
	//ChessPosition::A |= To & -( static_cast<int64_t>(M.Piece) & 1);
	//ChessPosition::B |= To & -((static_cast<int64_t>(M.Piece) & 2) >> 1);
	//ChessPosition::C |= To & -((static_cast<int64_t>(M.Piece) & 4) >> 2);
	//ChessPosition::D |= To & -((static_cast<int64_t>(M.Piece) & 8) >> 3);

#ifdef _USE_HASH
	// Update Hash
	ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.Piece][nFromSquare]; // Remove piece at From square
	ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.Piece][nToSquare]; // Place piece at To Square
#endif

	// Promotions - Change the piece:

	if (M.PromoteBishop)
	{
		ChessPosition::A &= ~To;
		ChessPosition::B |= To;	
#ifdef _USE_HASH
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.BlackToMove ? BPAWN : WPAWN][nToSquare]; // Remove pawn at To square
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.BlackToMove ? BBISHOP : WBISHOP][nToSquare]; // place Bishop at To square
#endif
		M.Piece = M.BlackToMove ? BBISHOP : WBISHOP;
	//
		return *this;
	}

	else if (M.PromoteKnight)
	{
		ChessPosition::C |= To;
#ifdef _USE_HASH
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.BlackToMove ? BPAWN : WPAWN][nToSquare]; // Remove pawn at To square
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.BlackToMove ? BKNIGHT : WKNIGHT][nToSquare];// place Knight at To square
#endif
		M.Piece = M.BlackToMove ? BKNIGHT : WKNIGHT;
	//
		return *this;
	}

	else if (M.PromoteRook)
	{		
		ChessPosition::A &= ~To;
		ChessPosition::C |= To;
#ifdef _USE_HASH
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.BlackToMove ? BPAWN : WPAWN][nToSquare]; // Remove pawn at To square
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.BlackToMove ? BROOK : WROOK][nToSquare];	// place Rook at To square
#endif
		M.Piece = M.BlackToMove ? BROOK : WROOK;
		//
		return *this;
	}

	else if (M.PromoteQueen)
	{
		ChessPosition::A &= ~To;
		ChessPosition::B |= To;
		ChessPosition::C |= To;
#ifdef _USE_HASH
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.BlackToMove ? BPAWN : WPAWN][nToSquare]; // Remove pawn at To square
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.BlackToMove ? BQUEEN : WQUEEN][nToSquare];	// place Queen at To square
#endif
		M.Piece = M.BlackToMove ? BQUEEN : WQUEEN;
		//
		return *this;
	}

	// For Double-Pawn Moves, set EP square:	////
	else if(M.DoublePawnMove)
	{
		// Set EnPassant Square
		if(M.BlackToMove)
		{
			To <<= 8;
			ChessPosition::A |= To;
			ChessPosition::B |= To;
			ChessPosition::C &= ~To;
			ChessPosition::D |= To;

#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BENPASSANT][nToSquare + 8];	// Place Black EP at (To+8)
#endif
		}
		else
		{	
			To >>= 8;
			ChessPosition::A |= To;
			ChessPosition::B |= To;
			ChessPosition::C &= ~To;
			ChessPosition::D &= ~To;
		
#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WENPASSANT][nToSquare - 8];	// Place White EP at (To-8)
#endif
		}
		//
		return *this;
	}
	
	// En-Passant Captures	////
	else if(M.EnPassantCapture)
	{
		// remove the actual pawn (it is different to the capture square)
		if(M.BlackToMove)
		{
			To <<= 8;
			ChessPosition::A &= ~To; // clear the pawn's square
			ChessPosition::B &= ~To;
			ChessPosition::C &= ~To;
			ChessPosition::D &= ~To;
		//	ChessPosition::material -= 100; // perft doesn't care
#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WPAWN][nToSquare + 8]; // Remove WHITE Pawn at (To+8)
#endif
		}
		else
		{	
			To >>= 8;
			ChessPosition::A &= ~To;
			ChessPosition::B &= ~To;
			ChessPosition::C &= ~To;
			ChessPosition::D &= ~To;
		//	ChessPosition::material += 100; // perft doesn't care
#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BPAWN][nToSquare - 8]; // Remove BLACK Pawn at (To-8)
#endif
		}
	}
	//
	return *this;
}

void ChessPosition::SwitchSides()
{
	ChessPosition::BlackToMove ^= 1;
#ifdef _USE_HASH
	ChessPosition::HK ^= ZobristKeys.zkBlackToMove;
#endif
	return;
}

void ChessPosition::Clear(void)
{
	A=B=C=D=0;
	Flags=0;
	material = 0;
#ifdef _USE_HASH
	HK = 0;
#endif
}

////////////////////////////////////////////
// Move Generation Functions 
// GenerateMoves()
////////////////////////////////////////////

void GenerateMoves(const ChessPosition& P, ChessMove* pM)
{
	
	assert((~(P.A | P.B | P.C) & P.D)==0); // Should not be any "black" empty squares
		
	if(P.BlackToMove)
	{
		GenBlackMoves(P,pM);
	}
	else
	{
		GenWhiteMoves(P,pM);
	}
}

//////////////////////////////////////////
// White Move Generation Functions:		//	
// GenWhiteMoves(),						//
// AddWhiteMoveToListIfLegal(),			//
// GenBlackAttacks()					//
//////////////////////////////////////////

void GenWhiteMoves(const ChessPosition& P, ChessMove* pM)
{
	ChessMove* pFirstMove = pM;
	BitBoard Occupied = P.A | P.B | P.C;								// all squares occupied by something
	BitBoard WhiteOccupied = (Occupied & ~P.D) & ~(P.A & P.B & ~P.C);	// all squares occupied by W, excluding EP Squares
	BitBoard BlackOccupied = Occupied & P.D;							// all squares occupied by B, including Black EP Squares
	BitBoard WhiteFree;				// all squares where W is free to move
	WhiteFree = (P.A & P.B & ~P.C)	// any EP square 
		|	~(Occupied)				// any vacant square
		|	(~P.A & P.D)			// Black Bishop, Rook or Queen
		|	(~P.B & P.D);			// Black Pawn or Knight
	
	assert(WhiteOccupied != 0);

	BitBoard Square;
	BitBoard CurrentSquare;
	BitBoard A,B,C;
	Move M; // Dummy Move object used for setting flags.
	uint32_t piece=0;
	
	unsigned long a = 63;
	unsigned long b = 0;

#if defined(_USE_BITSCAN_INSTRUCTIONS) && defined(_WIN64) 
	// perform Bitscans to determine start and finish squares;
	// Important: a and b must be initialised first !
	_BitScanReverse64(&a, WhiteOccupied);
	_BitScanForward64(&b, WhiteOccupied);
#else
	// (just scan all 64 squares: 0-63):
#endif

	for (unsigned int q=b; q<=a; q++)
	{
		CurrentSquare = 1LL << q;
		if ((WhiteOccupied & CurrentSquare) == 0)
			continue; // square empty - nothing to do
		//
		A = P.A & CurrentSquare;
		B = P.B & CurrentSquare;
		C = P.C & CurrentSquare;
		
		if (A !=0)
		{
			if (B == 0)
			{
				if (C == 0)
				{
					piece=WPAWN;
					// single move forward 
					Square = MoveUp[q] & WhiteFree & ~BlackOccupied /* pawns can't capture in forward moves */;
					if((Square & RANK8) != 0)
						AddWhitePromotionsToListIfLegal(P, pM, q, Square, piece);
					else
					{
						// Ordinary Pawn Advance
						AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
						/* double move forward (only available from 2nd Rank */
						if ((CurrentSquare & RANK2)!=0)
						{
							Square = MoveUpSingleOccluded(Square,WhiteFree) & ~BlackOccupied;
							M.ClearFlags();
							M.DoublePawnMove=1; // this flag will cause ChessPosition::PerformMove() to set an ep square in the position
							AddWhiteMoveToListIfLegal(P,pM,q,Square,piece,M.Flags);
						}
					}
					// Generate Pawn Captures:
					Square=MoveUpLeft[q] & WhiteFree & BlackOccupied;
					if((Square & RANK8) != 0)
						AddWhitePromotionsToListIfLegal(P, pM, q, Square, piece);
					else
					{
						// Ordinary Pawn Capture to Left
						AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					}
					Square = MoveUpRight[q] & WhiteFree & BlackOccupied;
					if((Square & RANK8) != 0)
						AddWhitePromotionsToListIfLegal(P, pM, q, Square, piece);
					else
					{
						// Ordinary Pawn Capture to right
						AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					}
					continue;
				}
				else
				{
					piece = WKNIGHT;
					Square=MoveKnight8[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight7[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight1[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight2[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight3[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight4[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight5[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight6[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);		
					continue;
				}
			}	// Ends if (B == 0)

			else /* B != 0 */
			{
				if (C != 0)
				{
					piece = WKING;
					Square=MoveUp[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
						
					Square=MoveRight[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);

					// Conditionally Generate O-O move:
					if ((P.WhiteCanCastle) &&								// White still has castle rights AND
						(CurrentSquare==WHITEKINGPOS) &&					// King is in correct Position AND
						((~P.A & ~P.B & P.C & ~P.D & WHITEKRPOS) != 0) &&	// KRook is in correct Position AND
						(pM->IllegalMove == 0) &&							// Last generated move (1 step to right) was legal AND
						(WHITECASTLEZONE & Occupied) == 0 &&				// Castle Zone (f1,g1) is clear AND
						!IsWhiteInCheck(P)									// King is not in Check
						)
					{
						// OK to Castle
						Square = G1;										// Move King to g1
						M.ClearFlags();
						M.Castle=1;
						AddWhiteMoveToListIfLegal(P,pM,q,Square,piece,M.Flags);
					}

					Square=MoveDown[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					
					Square=MoveLeft[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);

					// Conditionally Generate O-O-O move:
					if((P.WhiteCanCastleLong)	&&							// White still has castle-long rights AND
						(CurrentSquare==WHITEKINGPOS) &&					// King is in correct Position AND
						((~P.A & ~P.B & P.C & ~P.D & WHITEQRPOS) != 0) &&	// QRook is in correct Position AND
						(pM->IllegalMove == 0) &&							// Last generated move (1 step to left) was legal AND
						(WHITECASTLELONGZONE & Occupied) == 0 &&	 		// Castle Long Zone (b1,c1,d1) is clear AND	
						!IsWhiteInCheck(P)									// King is not in Check
						)
					{
						// Ok to Castle Long
						Square = C1;										// Move King to c1
						M.ClearFlags();
						M.CastleLong=1;
						AddWhiteMoveToListIfLegal(P,pM,q,Square,piece,M.Flags);
					}

					Square=MoveUpRight[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveDownRight[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveDownLeft[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveUpLeft[q] & WhiteFree;
					AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);

					continue;
				} // ENDS if (C != 0)
				else
				{
					// en passant square - no action to be taken, but continue loop
					continue;
				}
			} // Ends else
		} // ENDS if (A !=0)
			
		BitBoard SolidBlackPiece=P.D & ~(P.A & P.B); // All black pieces except enpassants and black king
			
		if (B != 0)
		{
			// Piece can do diagonal moves (it's either a B or Q)
			piece=(C == 0)? WBISHOP : WQUEEN;

			Square=CurrentSquare;
			do{ /* Diagonal UpRight */
				Square=MoveUpRightSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidBlackPiece);
			
			Square=CurrentSquare;
			do{ /* Diagonal DownRight */
				Square=MoveDownRightSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidBlackPiece);

			Square=CurrentSquare;
			do{ /* Diagonal DownLeft */
				Square=MoveDownLeftSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidBlackPiece);
			
			Square=CurrentSquare;
			do{ /* Diagonal UpLeft */
				Square=MoveUpLeftSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidBlackPiece);
		}

		if (C != 0)
		{
			// Piece can do straight moves (it's either a R or Q)
			piece=( B == 0)? WROOK : WQUEEN;

			Square=CurrentSquare;
			do{ /* Up */
				Square=MoveUpSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidBlackPiece);

			Square=CurrentSquare;
			do{ /* Right */
				Square=MoveRightSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidBlackPiece);

			Square=CurrentSquare;
			do{ /*Down */
				Square=MoveDownSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidBlackPiece);

			Square=CurrentSquare;
			do{ /*Left*/
				Square=MoveLeftSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidBlackPiece);
		}
	} // ends loop over q;

	// Hack to stuff the Move Count into the first Move:
	pFirstMove->MoveCount = pM - pFirstMove;
	
	// Create 'no more moves' move to mark end of list:
	pM->FromSquare=0;
	pM->ToSquare=0;
	pM->Piece=0;
	pM->NoMoreMoves=1;
}

inline void AddWhiteMoveToListIfLegal(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, BitBoard to, int32_t piece, int32_t flags)
{
	if (to != 0)
	{
		ChessPosition Q = P;
		pM->FromSquare = fromsquare;
		pM->ToSquare = static_cast<unsigned char>(GetSquareIndex(to));
		assert((1LL << pM->ToSquare) == to);
		pM->Flags = flags;
		pM->Piece = piece;

		BitBoard O = ~((1LL << fromsquare) | to);

		if (pM->Castle)
		{
			Q.A ^= 0x000000000000000a;
			Q.B ^= 0x000000000000000a;
			Q.C ^= 0x000000000000000f;
			//			Q.D &= 0xfffffffffffffff0;	// clear colour of e1,f1,g1,h1 (make white)
		}
		else if (pM->CastleLong)
		{
			Q.A ^= 0x0000000000000028;
			Q.B ^= 0x0000000000000028;
			Q.C ^= 0x00000000000000b8;
			//			Q.D &= 0xffffffffffffff07;	// clear colour of a1,b1,c1,d1,e1 (make white)
		}
		else {

			// clear old and new square:  
			Q.A &= O;
			Q.B &= O;
			Q.C &= O;
			Q.D &= O;

			// Populate new square (Branchless method):
			Q.A |= static_cast<int64_t>(piece & 1) << pM->ToSquare;
			Q.B |= static_cast<int64_t>((piece & 2) >> 1) << pM->ToSquare;
			Q.C |= static_cast<int64_t>((piece & 4) >> 2) << pM->ToSquare;
			Q.D |= static_cast<int64_t>((piece & 8) >> 3) << pM->ToSquare;

			// Test for capture:
			BitBoard PAB = (P.A & P.B);	// Bitboard containing EnPassants and kings:
			if (to & P.D) {

				if (to & ~PAB) // Only considered a capture if dest is not an enpassant or king.
					pM->Capture = 1;

				else if ((piece == WPAWN) && (to & P.D & PAB & ~P.C)) {
					pM->EnPassantCapture = 1;
					// remove the actual pawn (as well as the ep square)
					to >>= 8;
					Q.A &= ~to;
					Q.B &= ~to;
					Q.C &= ~to;
					Q.D &= ~to;
				}
			}
		}
			
		if (!IsWhiteInCheck(Q))										// Does proposed move put our (white) king in Check ?
		{
#ifdef _FLAG_CHECKS_IN_MOVE_GENERATION		
			if (IsBlackInCheck(Q))									// Does the move put enemy (black) king in Check ?
				pM->Check = 1;
#endif
			pM++;			// Advancing the pointer means that the 
							// move is now added to the list.
							// (pointer is ready for next move)
			pM->Flags = 0;

		}
		else
			pM->IllegalMove = 1;
	}
}

inline void AddWhitePromotionsToListIfLegal(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, BitBoard to, int32_t piece, int32_t flags /*=0*/)
{
	if (to != 0)
	{
		ChessPosition Q = P;
		pM->FromSquare = fromsquare;
		pM->ToSquare = static_cast<unsigned char>(GetSquareIndex(to));
		assert((1LL << pM->ToSquare) == to);
		pM->Flags = flags;
		pM->Piece = piece;

		BitBoard O = ~((1LL << fromsquare) | to);
		
		// clear old and new square:  
		Q.A &= O;
		Q.B &= O;
		Q.C &= O;
		Q.D &= O;

		// Populate new square with Queen:
		Q.B |= to;
		Q.C |= to;

		// Test for capture:
		BitBoard PAB = (P.A & P.B);	// Bitboard containing EnPassants and kings:
		pM->Capture = (to & P.D & ~PAB) ? 1 : 0;

		if (!IsWhiteInCheck(Q))										// Does proposed move put our (white) king in Check ?
		{
#ifdef _FLAG_CHECKS_IN_MOVE_GENERATION	
			// To-do: Promote to new piece (can potentially check opponent)
			if (IsBlackInCheck(Q))									// Does the move put enemy (black) king in Check ?
				pM->Check = 1;
#endif
			// make an additional 3 copies (there are four promotions)
			*(pM + 1) = *pM;
			*(pM + 2) = *pM;
			*(pM + 3) = *pM;

			// set Promotion flags accordingly:
			pM->PromoteKnight = 1;
			pM++;
			pM->PromoteBishop = 1;
			pM++;
			pM->PromoteRook = 1;
			pM++;
			pM->PromoteQueen = 1;
			pM++;
			pM->Flags = 0;
		}
		else
			pM->IllegalMove = 1;
	}
}

inline BitBoard GenBlackAttacks(const ChessPosition& Z)
{	
	BitBoard Occupied = Z.A | Z.B | Z.C;
	BitBoard Empty = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
		~Occupied;							// All Unoccupied squares
	
	BitBoard PotentialCapturesForBlack = Occupied & ~Z.D; // White Pieces (including Kings)
	
	BitBoard A=Z.A & Z.D;				// Black A-Plane
	BitBoard B=Z.B & Z.D;				// Black B-Plane 
	BitBoard C=Z.C & Z.D;				// Black C-Plane
	
	BitBoard S = C & ~A;				// Straight-moving Pieces
	BitBoard D = B & ~A;				// Diagonal-moving Pieces
	BitBoard K = A & B & C;				// King
	BitBoard P = A & ~B & ~C;			// Pawns
	BitBoard N = A & ~B & C;			// Knights

	BitBoard StraightAttacks = MoveUpSingleOccluded(FillUpOccluded(S, Empty), Empty | PotentialCapturesForBlack);
	StraightAttacks |= MoveRightSingleOccluded(FillRightOccluded(S, Empty), Empty | PotentialCapturesForBlack);
	StraightAttacks |= MoveDownSingleOccluded(FillDownOccluded(S, Empty), Empty | PotentialCapturesForBlack);
	StraightAttacks |= MoveLeftSingleOccluded(FillLeftOccluded(S, Empty), Empty | PotentialCapturesForBlack);
	
	BitBoard DiagonalAttacks = MoveUpRightSingleOccluded(FillUpRightOccluded(D, Empty), Empty | PotentialCapturesForBlack);
	DiagonalAttacks |= MoveDownRightSingleOccluded(FillDownRightOccluded(D, Empty), Empty | PotentialCapturesForBlack);
	DiagonalAttacks |= MoveDownLeftSingleOccluded(FillDownLeftOccluded(D, Empty), Empty | PotentialCapturesForBlack);
	DiagonalAttacks |= MoveUpLeftSingleOccluded(FillUpLeftOccluded(D, Empty), Empty | PotentialCapturesForBlack);
	
	BitBoard KingAttacks = FillKingAttacksOccluded(K, Empty | PotentialCapturesForBlack);
	BitBoard KnightAttacks = FillKnightAttacksOccluded(N, Empty | PotentialCapturesForBlack);
	BitBoard PawnAttacks = MoveDownLeftRightSingle(P) & (Empty | PotentialCapturesForBlack);

	return (StraightAttacks | DiagonalAttacks | KingAttacks | KnightAttacks | PawnAttacks);
}

inline BitBoard IsWhiteInCheck(const ChessPosition& Z)
{
	BitBoard WhiteKing = Z.A & Z.B & Z.C & ~Z.D;
	BitBoard V = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
		WhiteKing |						// White King
		~(Z.A | Z.B | Z.C);				// All Unoccupied squares

	BitBoard A = Z.A & Z.D;				// Black A-Plane
	BitBoard B = Z.B & Z.D;				// Black B-Plane 
	BitBoard C = Z.C & Z.D;				// Black C-Plane

	BitBoard S = C & ~A;				// Straight-moving Pieces
	BitBoard D = B & ~A;				// Diagonal-moving Pieces
	BitBoard K = A & B & C;				// King
	BitBoard P = A & ~B & ~C;			// Pawns
	BitBoard N = A & ~B & C;			// Knights

	BitBoard X = FillStraightAttacksOccluded(S, V);
	X |= FillDiagonalAttacksOccluded(D, V);
	X |= FillKingAttacks(K);
	X |= FillKnightAttacks(N);
	X |= MoveDownLeftRightSingle(P);
	return X & WhiteKing;
}

//////////////////////////////////////////
// Black Move Generation Functions:		//	
// GenBlackMoves(),						//
// AddBlackMoveToListIfLegal(),			//
// GenWhiteAttacks()					//
//////////////////////////////////////////

void GenBlackMoves(const ChessPosition& P, ChessMove* pM)
{
	ChessMove* pFirstMove = pM;
	BitBoard Occupied = P.A | P.B | P.C;								// all squares occupied by something
	BitBoard BlackOccupied = P.D & ~(P.A & P.B & ~P.C);					// all squares occupied by B, excluding EP Squares
	BitBoard WhiteOccupied = (Occupied & ~P.D);							// all squares occupied by W, including white EP Squares
	BitBoard BlackFree;				// all squares where B is free to move
	BlackFree = (P.A & P.B & ~P.C)	// any EP square 
		| ~(Occupied)				// any vacant square
		| (~P.A & ~P.D)				// White Bishop, Rook or Queen
		| (~P.B & ~P.D);			// White Pawn or Knight
	
	assert(BlackOccupied != 0);
		
	BitBoard Square;
	BitBoard CurrentSquare;
	BitBoard A,B,C;
	Move M; // Dummy Move object used for setting flags.
	uint32_t piece=0;
	
	unsigned long a = 63;
	unsigned long b = 0;

#if defined(_USE_BITSCAN_INSTRUCTIONS) && defined(_WIN64)
	// perform Bitscans to determine start and finish squares;
	// Important: a and b must be initialised first !
	_BitScanReverse64(&a, BlackOccupied);
	_BitScanForward64(&b, BlackOccupied);
#else
	// (just scan all 64 squares: 0-63)
#endif
	
	for (unsigned int q = b; q <= a; ++q)
	{
		assert(q >= 0);
		assert(q <= 63);

		CurrentSquare = 1LL << q;
		if ((BlackOccupied & CurrentSquare) == 0)
			continue; // square empty - nothing to do
		//
		A = P.A & CurrentSquare;
		B = P.B & CurrentSquare;
		C = P.C & CurrentSquare;

		if (A !=0)
		{
			if (B == 0)
			{
				if (C == 0)
				{
					piece=BPAWN;
					// single move forward 
					Square = MoveDown[q] & BlackFree & ~WhiteOccupied /* pawns can't capture in forward moves */;
					if((Square & RANK1) != 0)
						AddBlackPromotionsToListIfLegal(P, pM, q, Square, piece);
					else
					{
						// Ordinary Pawn Advance
						AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
						/* double move forward (only available from 7th Rank */
						if ((CurrentSquare & RANK7)!=0)
						{
							Square=MoveDownSingleOccluded(Square,BlackFree)&~WhiteOccupied;
							M.ClearFlags();
							M.DoublePawnMove=1; // this flag will cause ChessPosition::PerformMove() to set an ep square in the position
							AddBlackMoveToListIfLegal(P,pM,q,Square,piece,M.Flags);
						}
					}
					// Generate Pawn Captures:
					Square=MoveDownLeft[q] & BlackFree & WhiteOccupied;
					if((Square & RANK1) != 0)
						AddBlackPromotionsToListIfLegal(P, pM, q, Square, piece);
					else
					{
						// Ordinary Pawn Capture to Left
						AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					}
					Square = MoveDownRight[q] & BlackFree & WhiteOccupied;
					if((Square & RANK1) != 0)
						AddBlackPromotionsToListIfLegal(P, pM, q, Square, piece);
					else
					{
						// Ordinary Pawn Capture to right
						AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					}
					continue;
				}
				else
				{
					piece=BKNIGHT;
					Square=MoveKnight1[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight2[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight3[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight4[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight5[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight6[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight7[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveKnight8[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					continue;
				}
			}	// ENDS if (B ==0)

			else /* B != 0 */
			{
				if (C != 0)
				{
					piece = BKING;
					Square=MoveUp[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
						
					Square=MoveRight[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);

					// Conditionally Generate O-O move:
					if((P.BlackCanCastle) &&								// Black still has castle rights AND
						(CurrentSquare==BLACKKINGPOS) &&					// King is in correct Position AND
						((~P.A & ~P.B & P.C & P.D & BLACKKRPOS) != 0) &&	// KRook is in correct Position AND
						(pM->IllegalMove == 0) &&							// Last generated move (1 step to right) was legal AND
						(BLACKCASTLEZONE & Occupied) == 0 &&				// Castle Zone (f8,g8) is clear	AND
						!IsBlackInCheck(P)									// King is not in Check
						)
					{
						// OK to Castle
						Square = G8;										// Move King to g8
						M.ClearFlags();
						M.Castle=1;
						AddBlackMoveToListIfLegal(P,pM,q,Square,piece,M.Flags);	
					}

					Square=MoveDown[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					
					Square=MoveLeft[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);

					// Conditionally Generate O-O-O move:
					if((P.BlackCanCastleLong) &&							// Black still has castle-long rights AND
						(CurrentSquare==BLACKKINGPOS) &&					// King is in correct Position AND
						((~P.A & ~P.B & P.C & P.D & BLACKQRPOS) != 0) &&	// QRook is in correct Position AND
						(pM->IllegalMove == 0) &&							// Last generated move (1 step to left) was legal AND
						(BLACKCASTLELONGZONE & Occupied) == 0 &&			// Castle Long Zone (b8,c8,d8) is clear
						!IsBlackInCheck(P)									// King is not in Check
						)
					{
						// OK to castle Long
						Square = C8;										// Move King to c8
						M.ClearFlags();
						M.CastleLong=1;
						AddBlackMoveToListIfLegal(P,pM,q,Square,piece,M.Flags);
					}

					Square=MoveUpRight[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveDownRight[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveDownLeft[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					Square=MoveUpLeft[q] & BlackFree;
					AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
					continue;
				} // ENDS if (C != 0)
				else
				{
					// en passant square - no action to be taken, but continue loop
					continue;
				}
			} // Ends else
		} // ENDS if (A !=0)
			
		BitBoard SolidWhitePiece=WhiteOccupied & ~(P.A & P.B); // All white pieces except enpassants and white king
			
		if (B != 0)
		{
			// Piece can do diagonal moves (it's either a B or Q)
			piece=(C == 0)? BBISHOP : BQUEEN;

			Square=CurrentSquare;
			do{ /* Diagonal UpRight */
				Square=MoveUpRightSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal(P, pM, q, Square, piece);
			} while (Square & ~SolidWhitePiece);

			Square=CurrentSquare;
			do{ /* Diagonal DownRight */
				Square=MoveDownRightSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidWhitePiece);

			Square=CurrentSquare;
			do{ /* Diagonal DownLeft */
				Square=MoveDownLeftSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidWhitePiece);

			Square=CurrentSquare;
			do{ /* Diagonal UpLeft */
				Square=MoveUpLeftSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidWhitePiece);
		}

		if (C != 0)
		{
			// Piece can do straight moves (it's either a R or Q)
			piece=( B == 0)? BROOK : BQUEEN;

			Square=CurrentSquare;
			do{ /* Up */
				Square=MoveUpSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidWhitePiece);

			Square=CurrentSquare;
			do{ /* Right */
				Square=MoveRightSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidWhitePiece);

			Square=CurrentSquare;
			do{ /*Down */
				Square=MoveDownSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidWhitePiece);

			Square=CurrentSquare;
			do{ /*Left*/
				Square=MoveLeftSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal(P,pM,q,Square,piece);
			} while (Square & ~SolidWhitePiece);
		}
	} // ends loop over q;

	// Hack to stuff the Move Count into the first Move:
	pFirstMove->MoveCount = pM - pFirstMove;

	// Create 'no more moves' move to mark end of list:	
	pM->FromSquare=0;
	pM->ToSquare=0;
	pM->Piece=0;
	pM->NoMoreMoves=1;
}

inline void AddBlackMoveToListIfLegal(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, BitBoard to, int32_t piece, int32_t flags/*=0*/)
{
	if (to != 0)
	{
		ChessPosition Q = P;
		pM->FromSquare = fromsquare;
		pM->ToSquare = static_cast<unsigned char>(GetSquareIndex(to));
		pM->Flags = flags;
		pM->BlackToMove = 1;
		pM->Piece = piece;
		
		BitBoard O = ~((1LL << fromsquare) | to); 
				
		if (pM->Castle)
		{
			Q.A ^= 0x0a00000000000000;
			Q.B ^= 0x0a00000000000000;
			Q.C ^= 0x0f00000000000000;
			Q.D ^= 0x0f00000000000000;

		}
		else if (pM->CastleLong)
		{
			Q.A ^= 0x2800000000000000;
			Q.B ^= 0x2800000000000000;
			Q.C ^= 0xb800000000000000;
			Q.D ^= 0xb800000000000000;
		}
		else {

			// clear old and new square  
			Q.A &= O;
			Q.B &= O;
			Q.C &= O;
			Q.D &= O;

			// Populate new square (Branchless method):
			Q.A |= static_cast<int64_t>(piece & 1) << pM->ToSquare;
			Q.B |= static_cast<int64_t>((piece & 2) >> 1) << pM->ToSquare;
			Q.C |= static_cast<int64_t>((piece & 4) >> 2) << pM->ToSquare;
			Q.D |= static_cast<int64_t>((piece & 8) >> 3) << pM->ToSquare;
		
			// Test for capture:
			BitBoard PAB = (P.A & P.B);	// Bitboard containing EnPassants and kings
			BitBoard WhiteOccupied = (P.A | P.B | P.C) & ~P.D;
			if (to &  WhiteOccupied) {

				if (to & ~PAB) // Only considered a capture if dest is not an enpassant or king.
					pM->Capture = 1;

				else if ((piece == BPAWN) && (to & WhiteOccupied & PAB & ~P.C)) {
					pM->EnPassantCapture = 1;
					// remove the actual pawn (as well as the ep square)
					to <<= 8;
					Q.A &= ~to; // clear the pawn's square
					Q.B &= ~to;
					Q.C &= ~to;
					Q.D &= ~to;
				}
			}
		}
	
		if (!IsBlackInCheck(Q))	// Does proposed move put our (black) king in Check ?
		{
#ifdef _FLAG_CHECKS_IN_MOVE_GENERATION
			if (IsWhiteInCheck(Q))	// Does the move put enemy (white) king in Check ?
				pM->Check = 1;
#endif

			pM++;			// Advancing the pointer means that the 
							// move is now added to the list.
							// (pointer is ready for next move)
			pM->Flags = 0;
		}
		else
			pM->IllegalMove = 1;
	}
}

inline void AddBlackPromotionsToListIfLegal(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, BitBoard to, int32_t piece, int32_t flags/*=0*/)
{
	if (to != 0)
	{
		ChessPosition Q = P;
		pM->FromSquare = fromsquare;
		pM->ToSquare = static_cast<unsigned char>(GetSquareIndex(to));
		pM->Flags = flags;
		pM->BlackToMove = 1;
		pM->Piece = piece;
		
		BitBoard O = ~((1LL << fromsquare) | to);

		// clear old and new square  
		Q.A &= O;
		Q.B &= O;
		Q.C &= O;
		Q.D &= O;

		// Populate new square with Queen:
		Q.B |= to;
		Q.C |= to;
		Q.D |= to;

		// Test for capture:
		BitBoard PAB = (P.A & P.B);	// Bitboard containing EnPassants and kings
		BitBoard WhiteOccupied = (P.A | P.B | P.C) & ~P.D;
		pM->Capture = (to & WhiteOccupied & ~PAB) ? 1 : 0;

		if (!IsBlackInCheck(Q))	// Does proposed move put our (black) king in Check ?
		{
			// make an additional 3 copies for underpromotions
			*(pM + 1) = *pM;
			*(pM + 2) = *pM;
			*(pM + 3) = *pM;

			// set Promotion flags accordingly:
			pM->PromoteQueen = 1;
#ifdef _FLAG_CHECKS_IN_MOVE_GENERATION
			if (IsWhiteInCheck(Q))	// Does the move put enemy (white) king in Check ?
				pM->Check = 1;
#endif
			pM++;
			//
			pM->PromoteRook = 1;
#ifdef _FLAG_CHECKS_IN_MOVE_GENERATION
			// Promote to Rook (take away B)
			if (IsWhiteInCheck(Q))	// Does the move put enemy (white) king in Check ?
				pM->Check = 1;
#endif
			pM++;
			//
			pM->PromoteBishop = 1;
#ifdef _FLAG_CHECKS_IN_MOVE_GENERATION
			// Promote to Bishop (take away C, add B)
			if (IsWhiteInCheck(Q))	// Does the move put enemy (white) king in Check ?
				pM->Check = 1;
#endif
			pM++;
			//
			pM->PromoteKnight = 1;
#ifdef _FLAG_CHECKS_IN_MOVE_GENERATION
			// Promote to Knight (take away B, add C, add A)
			if (IsWhiteInCheck(Q))	// Does the move put enemy (white) king in Check ?
				pM->Check = 1;
#endif
			pM++;
			//
			pM->Flags = 0;
		}
		else
			pM->IllegalMove = 1;
	}
}

inline BitBoard GenWhiteAttacks(const ChessPosition& Z)
{
	BitBoard Occupied = Z.A | Z.B | Z.C;
	BitBoard Empty = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
		~Occupied;							// All Unoccupied squares
	
	BitBoard PotentialCapturesForWhite = Occupied & Z.D; // Black Pieces (including Kings)
	
	BitBoard A=Z.A & ~Z.D;				// White A-Plane
	BitBoard B=Z.B & ~Z.D;				// White B-Plane 
	BitBoard C=Z.C & ~Z.D;				// White C-Plane
	
	BitBoard S = C & ~A;				// Straight-moving Pieces
	BitBoard D = B & ~A;				// Diagonal-moving Pieces
	BitBoard K = A & B & C;				// King
	BitBoard P = A & ~B & ~C;			// Pawns
	BitBoard N = A & ~B & C;			// Knights
	
	BitBoard StraightAttacks = MoveUpSingleOccluded(FillUpOccluded(S, Empty), Empty | PotentialCapturesForWhite);
	StraightAttacks |= MoveRightSingleOccluded(FillRightOccluded(S, Empty), Empty | PotentialCapturesForWhite);
	StraightAttacks |= MoveDownSingleOccluded(FillDownOccluded(S, Empty), Empty | PotentialCapturesForWhite);
	StraightAttacks |= MoveLeftSingleOccluded(FillLeftOccluded(S, Empty), Empty | PotentialCapturesForWhite);

	BitBoard DiagonalAttacks = MoveUpRightSingleOccluded(FillUpRightOccluded(D, Empty), Empty | PotentialCapturesForWhite);
	DiagonalAttacks |= MoveDownRightSingleOccluded(FillDownRightOccluded(D, Empty), Empty | PotentialCapturesForWhite);
	DiagonalAttacks |= MoveDownLeftSingleOccluded(FillDownLeftOccluded(D, Empty), Empty | PotentialCapturesForWhite);
	DiagonalAttacks |= MoveUpLeftSingleOccluded(FillUpLeftOccluded(D, Empty), Empty | PotentialCapturesForWhite);

	BitBoard KingAttacks = FillKingAttacksOccluded(K, Empty | PotentialCapturesForWhite);
	BitBoard KnightAttacks = FillKnightAttacksOccluded(N, Empty | PotentialCapturesForWhite);
	BitBoard PawnAttacks = MoveUpLeftRightSingle(P) & (Empty | PotentialCapturesForWhite);

	return (StraightAttacks | DiagonalAttacks | KingAttacks | KnightAttacks | PawnAttacks);
}


inline BitBoard IsBlackInCheck(const ChessPosition& Z)
{
	BitBoard BlackKing = Z.A & Z.B & Z.C & Z.D;
	BitBoard V = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
		BlackKing |						// Black King
		~(Z.A | Z.B | Z.C);				// All Unoccupied squares

	BitBoard A = Z.A & ~Z.D;			// White A-Plane
	BitBoard B = Z.B & ~Z.D;			// White B-Plane 
	BitBoard C = Z.C & ~Z.D;			// White C-Plane

	BitBoard S = C & ~A;				// Straight-moving Pieces
	BitBoard D = B & ~A;				// Diagonal-moving Pieces
	BitBoard K = A & B & C;				// King
	BitBoard P = A & ~B & ~C;			// Pawns
	BitBoard N = A & ~B & C;			// Knights

	BitBoard X = FillStraightAttacksOccluded(S, V);
	X |= FillDiagonalAttacksOccluded(D, V);
	X |= FillKingAttacks(K);
	X |= FillKnightAttacks(N);
	X |= MoveUpLeftRightSingle(P);
	
	return X & BlackKing;
}

// IsInCheck() - Given a position, determines if player is in check - 
// set IsBlack to true to test if Black is in check
// set IsBlack to false to test if White is in check.

inline bool IsInCheck(const ChessPosition& P, bool bIsBlack)
{
	return bIsBlack ? IsBlackInCheck(P)!=0 : IsWhiteInCheck(P)!=0;
}

// Text Dump functions //////////////////

void DumpBitBoard(BitBoard b)
{
	printf("\n");
	for (int q=63;q>=0;q--)
	{
		if ((b & (1LL << q)) != 0)
		{
			printf("* ");
		}
		else 
		{
			printf(". ");
		}
		if ((q % 8) == 0)
		{
			printf ("\n");
		}
	}
}

void DumpChessPosition(ChessPosition p)
{
	printf("\n---------------------------------\n");
	int64_t M;
	int64_t V;
	for (int q=63;q>=0;q--)
	{
		M=1LL << q;
		V = ((p.D) & M) >> q;
		V <<= 1;
		V |=((p.C) & M) >> q;
		V <<= 1;
		V |=((p.B) & M) >> q;
		V <<= 1;
		V |=((p.A) & M) >> q;
		switch(V)
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
			if (q == 8) printf("material= %d", p.material);
			if (q == 0) printf("%s to move", p.BlackToMove ? "Black" : "White");
			printf("\n---------------------------------\n");
		}
	}	
}

//////////////////////////////////////////////////////////////////////////////////
// DumpMove() - Converts a move to ascii and dumps it to stdout, unless a character 
// buffer is supplied, in which case the ascii is written to pBuffer instead.
// A number of notation styles are supported. Usually CoOrdinate should be used
// for WinBoard.
//////////////////////////////////////////////////////////////////////////////////
void DumpMove(ChessMove M, MoveNotationStyle style /* = LongAlgebraic */, char* pBuffer /*= NULL */)
{
	int from=0, to=0;
	BitBoard bbFrom,bbTo;
	bbFrom = 1LL << M.FromSquare;
	bbTo = 1LL << M.ToSquare;
	char c1,c2,p;
		
	if(style != CoOrdinate)
	{
		
		if(M.Castle==1)	
		{
			if(pBuffer==NULL)
				printf(" O-O\n");
			else
				sprintf (pBuffer," O-O\n");
			return; 
		}

		if(M.CastleLong==1)	
		{
			if(pBuffer==NULL)
				printf(" O-O-O\n");
			else
				sprintf (pBuffer," O-O-O\n");
			return; 
		}
	}

	for (int q=0;q<64;q++)
	{
		if (bbFrom & (1LL << q))
		{
			from = q; 
			break;
		}
	}

	for (int q=0;q<64;q++)
	{
		if (bbTo & (1LL << q))
		{
			to=q; 
			break; 
		}
	}

	c1='h'-(from % 8);
	c2='h'-(to % 8);

	// Determine piece and assign character p accordingly:
	switch (M.Piece & 7)
	{
	case WPAWN:
		p=' ';
		break;
	case WKNIGHT:
		p='N';
		break;
	case WBISHOP:
		p='B';
		break;
	case WROOK:
		p='R';
		break;
	case WQUEEN:
		p='Q';
		break;
	case WKING:
		p='K';
		break;
	default:
		p='?';
	}
	//
	char s[SMALL_BUFFER_SIZE];  // output string
	*s = 0;
	
	if((style == LongAlgebraic ) || (style == LongAlgebraicNoNewline))
	{
		// Determine if move is a capture
		if (M.Capture || M.EnPassantCapture)
			sprintf(s,"%c%c%dx%c%d",p,c1,1+(from >> 3),c2,1+(to >> 3));
		else
			sprintf(s,"%c%c%d-%c%d",p,c1,1+(from >> 3),c2,1+(to >> 3));
		// Promotions:
		if (M.PromoteBishop)
			strcat(s,"(B)");
		if (M.PromoteKnight)
			strcat(s,"(N)");
		if (M.PromoteRook)
			strcat(s,"(R)");
		if (M.PromoteQueen)
			strcat(s,"(Q)");
		//
		//
		if(style != LongAlgebraicNoNewline)
			strcat(s,"\n");
		else
			strcat(s," ");
		//
		if(pBuffer == NULL)
			printf("%s", s);
		else
			strcpy(pBuffer,s);
	}

	if(style == CoOrdinate)
	{
		sprintf(s,"%c%d%c%d",c1,1+(from >> 3),c2,1+(to >> 3));
		// Promotions:
		if (M.PromoteBishop)
			strcat(s,"b");
		if (M.PromoteKnight)
			strcat(s,"n");
		if (M.PromoteRook)
			strcat(s,"r");
		if (M.PromoteQueen)
			strcat(s,"q");
		strcat(s,"\n");
		//
		if(pBuffer == NULL)
			printf("%s", s);
		else
			strcpy(pBuffer,s);
	}
}

void DumpMoveList(ChessMove* pMoveList, MoveNotationStyle style /* = LongAlgebraic */, char* pBuffer /*=NULL*/)
{
	ChessMove* pM=pMoveList;
	unsigned int i=0;
	do{
		if (pM->ToSquare != pM->FromSquare)
			DumpMove(*pM,style,pBuffer);
		if (pM->NoMoreMoves!=0)
		{
			// No more moves
			break;
		}
		++pM;
	}while (++i<MOVELIST_SIZE);
}

} // namespace juddperft