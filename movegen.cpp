// movegen.cpp

#include "movegen.h"
#include "fen.h"
#include "hashtable.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <windows.h>
#include <winnt.h>
#include <intrin.h>


#include <cassert>

extern ZobristKeySet ZobristKeys;

//////////////////////////////////////
// Implementation of Class Move		//
//////////////////////////////////////

Move::Move(BitBoard From/*=0*/,BitBoard To/*=0*/,unsigned __int32 Flags/*=0*/)
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

//Move& Move::operator=(const Move& M)
//{
//	Move::Flags = M.Flags;
//	Move::From = M.From;
//	Move::To = M.To;
//	return *this;
//}

Move& Move::Format(
		BitBoard From /*=0*/,
		BitBoard To /*=0*/,
		unsigned __int32 BlackToMove /*=0*/,
		unsigned __int32 Piece /*=0*/, 
		unsigned __int32 Flags /*=0*/
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
	ChessPosition::A =0i64;
	ChessPosition::B =0i64;
	ChessPosition::C =0i64;
	ChessPosition::D =0i64;
	ChessPosition::Flags=0;
	ChessPosition::material = 0;
#ifdef _USE_HASH
	ChessPosition::HK = 0i64;
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
	ChessPosition::HK = 0i64;
	
	// Scan the squares:
	for (int q = 0; q < 64; q++)
	{
		unsigned int piece = GetPieceAtSquare(1i64<<q);
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

unsigned __int32 ChessPosition::GetPieceAtSquare(const BitBoard& square) const
{
	unsigned __int32 V;
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
		M=1i64 << q;
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
	BitBoard O, P;
	P = 1i64 << M.ToSquare;
	O = ~((1i64 << M.FromSquare) | P);

#ifdef _USE_HASH
	unsigned long nFromSquare = M.FromSquare;
	unsigned long nToSquare = M.ToSquare;
	unsigned long nEPSquare;
#endif

	// CLEAR EP SQUARES :
	// clear any enpassant squares
	
	BitBoard EnPassant=A & B & (~C);
	ChessPosition::A &= ~EnPassant;
	ChessPosition::B &= ~EnPassant;
	ChessPosition::C &= ~EnPassant;
	ChessPosition::D &= ~EnPassant;

#ifdef _USE_HASH
	nEPSquare = GetSquareIndex(EnPassant);
	ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WENPASSANT][nEPSquare]; // Remove EP from nEPSquare
#endif // _USE_HASH



//	BitBoard BlackEnPassant=EnPassant & D;
//	BitBoard WhiteEnPassant=EnPassant & (~D);
//	
//
//
//	if (M.BlackToMove)
//	{
//		if (WhiteEnPassant != 0)
//		{
//			ChessPosition::A &= ~WhiteEnPassant;
//			ChessPosition::B &= ~WhiteEnPassant;
//			ChessPosition::C &= ~WhiteEnPassant;
//			ChessPosition::D &= ~WhiteEnPassant;
//#ifdef _USE_HASH
//			nEPSquare = GetSquareIndex(WhiteEnPassant);
//			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WENPASSANT][nEPSquare]; // Remove EP from nEPSquare
//#endif // _USE_HASH
//		}
//	}
//	else
//	{
//		if (BlackEnPassant != 0)
//		{
//			ChessPosition::A &= ~BlackEnPassant;
//			ChessPosition::B &= ~BlackEnPassant;
//			ChessPosition::C &= ~BlackEnPassant;
//			ChessPosition::D &= ~BlackEnPassant;
//#ifdef _USE_HASH
//			nEPSquare = GetSquareIndex(BlackEnPassant);
//			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BENPASSANT][nEPSquare]; // Remove EP from nEPSquare
//#endif // _USE_HASH
//
//		}
//	}

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
			ChessPosition::A ^= 0x0a00000000000000i64;
			ChessPosition::B ^= 0x0a00000000000000i64;
			ChessPosition::C ^= 0x0f00000000000000i64;
			ChessPosition::D ^= 0x0f00000000000000i64;
#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BKING][59]; // Remove King from e8
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BKING][57]; // Place King on g8
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BROOK][56]; // Remove Rook from h8
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BROOK][58]; // Place Rook on f8
			ChessPosition::HK ^= ZobristKeys.zkBlackCanCastle;	// (unconditionally) flip black castling
			if (ChessPosition::BlackCanCastleLong) ChessPosition::HK ^= ZobristKeys.zkBlackCanCastleLong;	// conditionally flip black castling long
#endif
			ChessPosition::BlackDidCastle = 1;
			ChessPosition::BlackCanCastle = 0;
			ChessPosition::BlackCanCastleLong = 0;
			return *this;
		}
		else if(M.CastleLong)
		{
			ChessPosition::A ^= 0x2800000000000000i64;
			ChessPosition::B ^= 0x2800000000000000i64;
			ChessPosition::C ^= 0xb800000000000000i64;
			ChessPosition::D ^= 0xb800000000000000i64;
#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BKING][59]; // Remove King from e8
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BKING][61]; // Place King on c8
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BROOK][63]; // Remove Rook from a8
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BROOK][60]; // Place Rook on d8
			if (ChessPosition::BlackCanCastle) ChessPosition::HK ^= ZobristKeys.zkBlackCanCastle;	// conditionally flip black castling
			ChessPosition::HK ^= ZobristKeys.zkBlackCanCastleLong;	// (unconditionally) flip black castling long
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

	if (M.Piece == WKING)
	{
		// White
		if(M.Castle)
		{
			ChessPosition::A ^= 0x000000000000000ai64;
			ChessPosition::B ^= 0x000000000000000ai64;
			ChessPosition::C ^= 0x000000000000000fi64;
			ChessPosition::D &= 0xfffffffffffffff0i64;	// clear colour of e1,f1,g1,h1 (make white)
#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WKING][3]; // Remove King from e1
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WKING][1]; // Place King on g1
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WROOK][0]; // Remove Rook from h1
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WROOK][2]; // Place Rook on f1
			ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastle;	// (unconditionally) flip white castling
			if (ChessPosition::WhiteCanCastleLong) ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastleLong;	// conditionally flip white castling long
#endif
			ChessPosition::WhiteDidCastle = 1;
			ChessPosition::WhiteCanCastle = 0;
			ChessPosition::WhiteCanCastleLong = 0;
			return *this;
		}
		
		if(M.CastleLong)
		{
			ChessPosition::A ^= 0x0000000000000028i64;
			ChessPosition::B ^= 0x0000000000000028i64;
			ChessPosition::C ^= 0x00000000000000b8i64;
			ChessPosition::D &= 0xffffffffffffff07i64;	// clear colour of a1,b1,c1,d1,e1 (make white)
#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WKING][3]; // Remove King from e1
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WKING][5]; // Place King on c1
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WROOK][7]; // Remove Rook from a1
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WROOK][4]; // Place Rook on d1
			if (ChessPosition::WhiteCanCastle) ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastle;	// conditionally flip white castling
			ChessPosition::HK ^= ZobristKeys.zkWhiteCanCastleLong;	// (unconditionally) flip white castling long
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
	if(M.Piece == BROOK)
	{	
		if((1i64<<nFromSquare) & BLACKKRPOS)
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
		else if ((1i64<<nFromSquare) & BLACKQRPOS)
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
	
	if(M.Piece == WROOK)
	{
		if((1i64<<nFromSquare) & WHITEKRPOS)
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
		else if((1i64<<nFromSquare) & WHITEQRPOS)
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
		// find out which piece has been captured:
		capturedpiece = (ChessPosition::D & (1i64<<nToSquare))?8:0;
		capturedpiece |=(ChessPosition::C & (1i64<<nToSquare))?4:0;
		capturedpiece |=(ChessPosition::B & (1i64<<nToSquare))?2:0;
		capturedpiece |=(ChessPosition::A & (1i64<<nToSquare))?1:0;

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
			
	//if (M.Piece & 1)
	//	ChessPosition::A |=  P;
	//if (M.Piece & 2)
	//	ChessPosition::B |=  P;
	//if (M.Piece & 4)
	//	ChessPosition::C |=  P;
	//if (M.Piece & 8)
	//	ChessPosition::D |=  P;

	// Populate new square (Branchless method):
	ChessPosition::A |= P & -( static_cast<__int64>(M.Piece) & 1);
	ChessPosition::B |= P & -((static_cast<__int64>(M.Piece) & 2) >> 1);
	ChessPosition::C |= P & -((static_cast<__int64>(M.Piece) & 4) >> 2);
	ChessPosition::D |= P & -((static_cast<__int64>(M.Piece) & 8) >> 3);


#ifdef _USE_HASH
	// Update Hash
	ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.Piece][nFromSquare]; // Remove piece at From square
	ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.Piece][nToSquare]; // Place piece at To Square
#endif

	// Promotions - Change the piece:

	if (M.PromoteBishop)
	{
		ChessPosition::A &= ~P;
		ChessPosition::B |= P;	
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
		ChessPosition::C |= P;
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
		ChessPosition::A &= ~P;
		ChessPosition::C |= P;
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
		ChessPosition::A &= ~P;
		ChessPosition::B |= P;
		ChessPosition::C |= P;
#ifdef _USE_HASH
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.BlackToMove ? BPAWN : WPAWN][nToSquare]; // Remove pawn at To square
		ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[M.BlackToMove ? BQUEEN : WQUEEN][nToSquare];	// place Queen at To square
#endif
		M.Piece = M.BlackToMove ? BQUEEN : WQUEEN;
		//
		return *this;
	}

	// For Double-Pawn Moves, set EP square:	////
	if(M.DoublePawnMove)
	{
		// Set EnPassant Square
		if(M.BlackToMove)
		{
			P <<= 8;
			ChessPosition::A &= ~P;
			ChessPosition::B &= ~P;
			ChessPosition::C &= ~P;
			ChessPosition::D &= ~P;
			ChessPosition::A |= P;
			ChessPosition::B |= P;
			ChessPosition::D |= P;
#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[BENPASSANT][nToSquare + 8];	// Place Black EP at (To+8)
#endif
		}
		else
		{	
			P >>= 8;
			ChessPosition::A &= ~P;
			ChessPosition::B &= ~P;
			ChessPosition::C &= ~P;
			ChessPosition::D &= ~P;
			ChessPosition::A |= P;
			ChessPosition::B |= P;
#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WENPASSANT][nToSquare - 8];	// Place White EP at (To-8)
#endif
		}
		//
		return *this;
	}
	
	// En-Passant Captures	////
	if(M.EnPassantCapture)
	{
		// remove the actual pawn (it is different to the capture square)
		if(M.BlackToMove)
		{
			P <<= 8;
			ChessPosition::A &= ~P; // clear the pawn's square
			ChessPosition::B &= ~P; 
			ChessPosition::C &= ~P;
			ChessPosition::D &= ~P;
		//	ChessPosition::material -= 100; // perft doesn't care
#ifdef _USE_HASH
			ChessPosition::HK ^= ZobristKeys.zkPieceOnSquare[WPAWN][nToSquare + 8]; // Remove WHITE Pawn at (To+8)
#endif
		}
		else
		{	
			P >>= 8;
			ChessPosition::A &= ~P;
			ChessPosition::B &= ~P;
			ChessPosition::C &= ~P;
			ChessPosition::D &= ~P;
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
	//return *this;
	return;
}

// PrepareMove() - cut-down version of PerformMove()
// doesn't update flags, material, or HashKey
// doesn't set or clear EP squares
// It doesn't change the Piece for Promotions
// This function should only be used by the Move generator !!

inline void ChessPosition::PrepareChessMove(const ChessMove& M)
{
	BitBoard O,P;
	P = 1i64 << M.ToSquare;
	O = ~((1i64 << M.FromSquare) | P);
	

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


	if (M.BlackToMove)
	{
		if (M.Castle)
		{
			ChessPosition::A ^= 0x0a00000000000000i64;
			ChessPosition::B ^= 0x0a00000000000000i64;
			ChessPosition::C ^= 0x0f00000000000000i64;
			ChessPosition::D ^= 0x0f00000000000000i64;
			return;

		}
		if (M.CastleLong)
		{
			ChessPosition::A ^= 0x2800000000000000i64;
			ChessPosition::B ^= 0x2800000000000000i64;
			ChessPosition::C ^= 0xb800000000000000i64;
			ChessPosition::D ^= 0xb800000000000000i64;
			return;
		}
	}

	else
	{
		// White
		if (M.Castle)
		{
			ChessPosition::A ^= 0x000000000000000ai64;
			ChessPosition::B ^= 0x000000000000000ai64;
			ChessPosition::C ^= 0x000000000000000fi64;
//			ChessPosition::D &= 0xfffffffffffffff0i64;	// clear colour of e1,f1,g1,h1 (make white)
			return;
		}
		if (M.CastleLong)
		{
			ChessPosition::A ^= 0x0000000000000028i64;
			ChessPosition::B ^= 0x0000000000000028i64;
			ChessPosition::C ^= 0x00000000000000b8i64;
//			ChessPosition::D &= 0xffffffffffffff07i64;	// clear colour of a1,b1,c1,d1,e1 (make white)
			return;
		}
	}

	// clear old and new square  
	ChessPosition::A &= O;
	ChessPosition::B &= O;
	ChessPosition::C &= O;
	ChessPosition::D &= O;

	// Populate new square:
	if (M.Piece & 1)
		ChessPosition::A |=  P;
	if (M.Piece & 2)
		ChessPosition::B |=  P;
	if (M.Piece & 4)
		ChessPosition::C |=  P;
	if (M.Piece & 8)
		ChessPosition::D |=  P;

		
	//// Populate new square (Branchless method):
	//ChessPosition::A |= P & -( static_cast<__int64>(M.Piece) & 1);
	//ChessPosition::B |= P & -((static_cast<__int64>(M.Piece) & 2) >> 1);
	//ChessPosition::C |= P & -((static_cast<__int64>(M.Piece) & 4) >> 2);
	//ChessPosition::D |= P & -((static_cast<__int64>(M.Piece) & 8) >> 3);

	//// Populate new square (Branchless method - no casts):
	//ChessPosition::A |= P & -((M.Piece) & 1);
	//ChessPosition::B |= P & -(((M.Piece) & 2) >> 1);
	//ChessPosition::C |= P & -(((M.Piece) & 4) >> 2);
	//ChessPosition::D |= P & -(((M.Piece) & 8) >> 3);
	
	// En-Passant Captures	////
	if (M.EnPassantCapture)
	{
		// remove the actual pawn (as well as the ep square)
		if (M.BlackToMove)
		{
			P <<= 8;
			ChessPosition::A &= ~P; // clear the pawn's square
			ChessPosition::B &= ~P;
			ChessPosition::C &= ~P;
			ChessPosition::D &= ~P;
		}
		else
		{
			P >>= 8;
			ChessPosition::A &= ~P;
			ChessPosition::B &= ~P;
			ChessPosition::C &= ~P;
			ChessPosition::D &= ~P;
		}
	}
	return;
}

void ChessPosition::Clear(void)
{
	A=B=C=D=0i64;
	Flags=0;
	material = 0;
#ifdef _USE_HASH
	HK = 0i64;
#endif
}

////////////////////////////////////////////
// Move Generation Functions 
// GenerateMoves()
////////////////////////////////////////////

void GenerateMoves(const ChessPosition& P, ChessMove* pM)
{
	
	assert((~(P.A | P.B | P.C) & P.D)==0i64); // Should not be any "black" empty squares
		
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
	// To-do: identify Checks !
	// To-Do Prioritze checks & captures.
	BitBoard Occupied;		/* all squares occupied by something */
	BitBoard WhiteOccupied; /* all squares occupied by W */
	BitBoard BlackOccupied; /* all squares occupied by B */
	BitBoard King,BlackKing;
	BitBoard WhiteFree;		/* all squares where W is free to move */
	
	Occupied = P.A | P.B | P.C;
	BlackOccupied = Occupied & P.D;								/* Note : Also includes Black EP squares !*/
	WhiteOccupied = (Occupied & ~P.D) & ~(P.A & P.B & ~P.C);	/* Note : excludes White EP squares ! */
	
	assert(WhiteOccupied != 0);
	
	King=P.A & P.B & P.C;	
	BlackKing=(King & P.D);
	WhiteFree= ~WhiteOccupied & ~BlackKing; 
	//
	BitBoard Square;
	BitBoard CurrentSquare;
	BitBoard A,B,C;
	Move M; // Dummy Move object used for setting flags.
	unsigned __int32 piece=0;
	
	unsigned long a = 63;
	unsigned long b = 0;

#if defined(_USE_BITSCAN_INSTRUCTIONS) && defined(_WIN64) 
	// perform Bitscans to determine start and finish squares;
	// Important: a and b must be initialised first !
	_BitScanReverse64(&a, WhiteOccupied);
	_BitScanForward64(&b, WhiteOccupied);
#elif
	// (just scan all 64 squares: 0-63):
#endif

	for (int q=b; q<=a; q++)
	{
		CurrentSquare = 1i64 << q;
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
					piece=WPAWN; /* Piece is a Pawn */
					// single move forward 
					Square = MoveUp[q] & WhiteFree & ~BlackOccupied /* pawns can't capture in forward moves */;
					if((Square & RANK8) != 0)
					{
						// Pawn Promotion
						M.ClearFlags();
						M.PromoteKnight=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteBishop=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteRook=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteQueen=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
					}
					else
					{
						// Ordinary Pawn Advance
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
						/* double move forward (only available from 2nd Rank */
						if ((CurrentSquare & RANK2)!=0)
						{
							Square = MoveUpSingleOccluded(Square,WhiteFree) & ~BlackOccupied;
							M.ClearFlags();
							M.DoublePawnMove=1; // note: ep is handled by ChessPosition class
							AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						}
					}
					// Generate Pawn Captures:
					Square=MoveUpLeft[q] & WhiteFree & BlackOccupied;
					if((Square & RANK8) != 0)
					{
						// Pawn Promotion
						M.ClearFlags();
						M.PromoteKnight=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteBishop=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteRook=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteQueen=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
					}
					else
					{
						// Ordinary Pawn Capture to Left
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					}
					Square = MoveUpRight[q] & WhiteFree & BlackOccupied;
					if((Square & RANK8) != 0)
					{
						// Pawn Promotion
						M.ClearFlags();
						M.PromoteKnight=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteBishop=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteRook=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteQueen=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
					}
					else
					{
						// Ordinary Pawn Capture to right
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					}
					continue;
				}
				else
				{
					piece = WKNIGHT;
						
					Square=MoveKnight8[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight7[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight1[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight2[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight3[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight4[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight5[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight6[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);

					continue;
				}
			}	// Ends if (B == 0)

			else /* B != 0 */
			{
				if (C != 0)
				{
					piece = WKING;
					Square=MoveUp[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
						
					Square=MoveRight[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);

					// Conditionally Generate O-O move:
					if ((P.WhiteCanCastle) &&								// White still has castle rights AND
						(CurrentSquare==WHITEKINGPOS) &&					// King is in correct Position AND
						((~P.A & ~P.B & P.C & ~P.D & WHITEKRPOS) != 0) &&	// KRook is in correct Position AND
						(pM->IllegalMove == 0) &&							// Last generated move (1 step to right) was legal AND
						(WHITECASTLEZONE & Occupied) == 0i64 &&				// Castle Zone (f1,g1) is clear AN
						!IsWhiteInCheck(P)									// King is not in Check
						)
					{
						// OK to Castle
						Square = G1;										// Move King to g1
						M.ClearFlags();
						M.Castle=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
					}

					Square=MoveDown[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					
					Square=MoveLeft[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);

					// Conditionally Generate O-O-O move:
					if((P.WhiteCanCastleLong)	&&							// White still has castle-long rights AND
						(CurrentSquare==WHITEKINGPOS) &&					// King is in correct Position AND
						((~P.A & ~P.B & P.C & ~P.D & WHITEQRPOS) != 0) &&	// QRook is in correct Position AND
						(pM->IllegalMove == 0) &&							// Last generated move (1 step to left) was legal AND
						(WHITECASTLELONGZONE & Occupied) == 0i64 &&	 		// Castle Long Zone (b1,c1,d1) is clear AND	
						!IsWhiteInCheck(P)									// King is not in Check
						)
					{
						// Ok to Castle Long
						Square = C1;										// Move King to c1
						M.ClearFlags();
						M.CastleLong=1;
						AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
					}

					Square=MoveUpRight[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveDownRight[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveDownLeft[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveUpLeft[q] & WhiteFree;
					AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);

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
				AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidBlackPiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /* Diagonal DownRight */
				Square=MoveDownRightSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidBlackPiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /* Diagonal DownLeft */
				Square=MoveDownLeftSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidBlackPiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /* Diagonal UpLeft */
				Square=MoveUpLeftSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidBlackPiece)
					break;
			} while (Square != 0);
		}

		if (C != 0)
		{
			// Piece can do straight moves (it's either a R or Q)
			piece=( B == 0)? WROOK : WQUEEN;	
			Square=CurrentSquare;
			do{ /* Up */
				Square=MoveUpSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidBlackPiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /* Right */
				Square=MoveRightSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidBlackPiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /*Down */
				Square=MoveDownSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidBlackPiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /*Left*/
				Square=MoveLeftSingleOccluded(Square,WhiteFree);
				AddWhiteMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidBlackPiece)
					break;
			} while (Square != 0);
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

inline void AddWhiteMoveToListIfLegal2(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, BitBoard to, __int32 piece, __int32 flags /*=0*/)
{
	if (to != 0i64)
	{
		ChessPosition Q = P;
		// lets start by building the move structure:
		pM->FromSquare = fromsquare;
		pM->ToSquare = GetSquareIndex(to);
		assert((1i64 << pM->ToSquare) == to);
		pM->Flags = flags;
		pM->BlackToMove = 0;
		pM->Piece = piece;

		// Bitboard containing EnPassants and kings:
		BitBoard QAB = (Q.A & Q.B);

		// Test for capture. Note: 
		// Only considered a capture if dest is not an enpassant or king.
		pM->Capture = (to & Q.D & ~QAB) ? 1 : 0;
		pM->EnPassantCapture = ((pM->Piece == WPAWN) && (to & Q.D & QAB & ~Q.C)) ? 1 : 0;

		// Now, lets 'try out' the move:
		Q.PrepareChessMove(*pM);
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
	// To-do: identify Checks !
	// To-Do Prioritze checks & captures.
	BitBoard Occupied;		/* all squares occupied by something */
	BitBoard WhiteOccupied; /* all squares occupied by W */
	BitBoard BlackOccupied; /* all squares occupied by B */
	BitBoard King, WhiteKing;
	BitBoard BlackFree;		/* all squares where B is free to move */

	Occupied = P.A | P.B | P.C;
	WhiteOccupied = Occupied & ~P.D;					/* Note : Also includes White EP squares !*/
	BlackOccupied = P.D & ~(P.A & P.B & ~P.C & P.D);	/* Note : excludes Black EP squares ! */
	
	assert(BlackOccupied != 0);

	King=P.A & P.B & P.C;
	WhiteKing=(King & ~P.D); 
	//BlackKingInCheck=King & P.D & GenWhiteAttacks(P);
	BlackFree= ~BlackOccupied & ~WhiteKing;
	//
	BitBoard Square;
	BitBoard CurrentSquare;
	BitBoard A,B,C;
	Move M; // Dummy Move object used for setting flags.
	unsigned __int32 piece=0;
	
	unsigned long a = 63;
	unsigned long b = 0;

#if defined(_USE_BITSCAN_INSTRUCTIONS) && defined(_WIN64)
	// perform Bitscans to determine start and finish squares;
	// Important: a and b must be initialised first !
	_BitScanReverse64(&a, BlackOccupied);
	_BitScanForward64(&b, BlackOccupied);
#elif
	// (just scan all 64 squares: 0-63)
#endif
	
	for (int q = b; q <= a; q++)
	{
		assert(q >= 0);
		assert(q <= 63);

		CurrentSquare = 1i64 << q;
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
					piece=BPAWN; /* Piece is a Pawn */
					// single move forward 
					Square = MoveDown[q] & BlackFree & ~WhiteOccupied /* pawns can't capture in forward moves */;
					if((Square & RANK1) != 0)
					{
						// Pawn Promotion
						M.ClearFlags();
						M.PromoteKnight=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteBishop=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteRook=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteQueen=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
					}
					else
					{
						// Ordinary Pawn Advance
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
						/* double move forward (only available from 7th Rank */
						if ((CurrentSquare & RANK7)!=0)
						{
							Square=MoveDownSingleOccluded(Square,BlackFree)&~WhiteOccupied;
							M.ClearFlags();
							M.DoublePawnMove=1; // note: ep is handled by ChessPosition class
							AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						}
					}
					// Generate Pawn Captures:
					Square=MoveDownLeft[q] & BlackFree & WhiteOccupied;
					if((Square & RANK1) != 0)
					{
						// Pawn Promotion
						M.ClearFlags();
						M.PromoteKnight=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteBishop=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteRook=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteQueen=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
					}
					else
					{
						// Ordinary Pawn Capture to Left
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					}
					Square = MoveDownRight[q] & BlackFree & WhiteOccupied;
					if((Square & RANK1) != 0)
					{
						// Pawn Promotion
						M.ClearFlags();
						M.PromoteKnight=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteBishop=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteRook=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
						M.ClearFlags();
						M.PromoteQueen=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
					}
					else
					{
						// Ordinary Pawn Capture to right
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					}
					continue;
				}
				else
				{
					piece=BKNIGHT;

					Square=MoveKnight1[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight2[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight3[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight4[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight5[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight6[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight7[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveKnight8[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);

					continue;
				}
			}	// ENDS if (B ==0)

			else /* B != 0 */
			{
				if (C != 0)
				{
					piece = BKING;
					Square=MoveUp[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
						
					Square=MoveRight[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);

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
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);	
					}

					Square=MoveDown[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					
					Square=MoveLeft[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);

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
						//Square=MoveLeftSingleOccluded(Square,BlackFree);	// Move another (2nd) square to the left
						Square = C8;										// Move King to c8
						M.ClearFlags();
						M.CastleLong=1;
						AddBlackMoveToListIfLegal2(P,pM,q,Square,piece,M.Flags);
					}

					Square=MoveUpRight[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveDownRight[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveDownLeft[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
					Square=MoveUpLeft[q] & BlackFree;
					AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
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
				AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidWhitePiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /* Diagonal DownRight */
				Square=MoveDownRightSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidWhitePiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /* Diagonal DownLeft */
				Square=MoveDownLeftSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidWhitePiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /* Diagonal UpLeft */
				Square=MoveUpLeftSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidWhitePiece)
					break;
			} while (Square != 0);
		}

		if (C != 0)
		{
			// Piece can do straight moves (it's either a R or Q)
			piece=( B == 0)? BROOK : BQUEEN;	
			Square=CurrentSquare;
			do{ /* Up */
				Square=MoveUpSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidWhitePiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /* Right */
				Square=MoveRightSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidWhitePiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /*Down */
				Square=MoveDownSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidWhitePiece)
					break;
			} while (Square != 0);
			Square=CurrentSquare;
			do{ /*Left*/
				Square=MoveLeftSingleOccluded(Square,BlackFree);
				AddBlackMoveToListIfLegal2(P,pM,q,Square,piece);
				if(Square & SolidWhitePiece)
					break;
			} while (Square != 0);
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

inline void AddBlackMoveToListIfLegal2(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, BitBoard to, __int32 piece, __int32 flags/*=0*/)
{
	if (to != 0i64)
	{
		ChessPosition Q = P;
		// lets start by building the move structure:
		/*pM->From = from;
		pM->To = to;*/
		//BitBoard from = 1i64 << fromsquare;
		pM->FromSquare = fromsquare;
		pM->ToSquare = GetSquareIndex(to);
		pM->Flags = flags;
		pM->BlackToMove = 1;
		pM->Piece = piece;

		// Bitboard containing EnPassants and kings:
		BitBoard QAB = (Q.A & Q.B);
		BitBoard WhiteOccupied = (Q.A | Q.B | Q.C)&~Q.D;

		// Test for capture. Note: 
		// Only considered a capture if dest is not an enpassant or king.
		pM->Capture = (to & WhiteOccupied & ~QAB) ? 1 : 0;
		pM->EnPassantCapture = ((pM->Piece == BPAWN) && (to & WhiteOccupied & QAB & ~Q.C)) ? 1 : 0;

		// Now, lets 'try out' the move:
		Q.PrepareChessMove(*pM);
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

bool IsInCheck(const ChessPosition& P, bool bIsBlack)
{
	return bIsBlack ? IsBlackInCheck(P) : IsWhiteInCheck(P);
}

// Text Dump functions //////////////////

void DumpBitBoard(BitBoard b)
{
	printf_s("\n");
	for (int q=63;q>=0;q--)
	{
		if ((b & (1i64 << q)) != 0)
		{
			printf_s("* ");
		}
		else 
		{
			printf_s(". ");
		}
		if ((q % 8) == 0)
		{
			printf ("\n");
		}
	}
}

void DumpChessPosition(ChessPosition p)
{
	printf_s("\n---------------------------------\n");
	__int64 M;
	__int64 V;
	for (int q=63;q>=0;q--)
	{
		M=1i64 << q;
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
			printf_s("| P ");
			break;
		case WBISHOP:
			printf_s("| B ");
			break;
		case WROOK:
			printf_s("| R ");
			break;
		case WQUEEN:
			printf_s("| Q ");
			break;
		case WKING:
			printf_s("| K ");
			break;
		case WKNIGHT:
			printf_s("| N ");
			break;
		case WENPASSANT:
			printf_s("| EP");
			break;
		case BPAWN:
			printf_s("| p ");
			break;
		case BBISHOP:
			printf_s("| b ");
			break;
		case BROOK:
			printf_s("| r ");
			break;
		case BQUEEN:
			printf_s("| q ");
			break;
		case BKING:
			printf_s("| k ");
			break;
		case BKNIGHT:
			printf_s("| n ");
			break;
		case BENPASSANT:
			printf_s("| ep");
			break;
		default:
			printf_s("|   ");
		}

		if ((q % 8) == 0)
		{
			printf_s("|   ");
			if ((q == 56) && (p.BlackCanCastle))
				printf_s("Black can Castle");
			if ((q == 48) && (p.BlackCanCastleLong))
				printf_s("Black can Castle Long");
			if ((q == 40) && (p.WhiteCanCastle))
				printf_s("White can Castle");
			if ((q == 32) && (p.WhiteCanCastleLong))
				printf_s("White can Castle Long");
			if (q == 8) printf_s("material= %d", p.material);
			if (q == 0) printf_s("%s to move", p.BlackToMove ? "Black" : "White");
			printf_s("\n---------------------------------\n");
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
	bbFrom = 1i64 << M.FromSquare;
	bbTo = 1i64 << M.ToSquare;
	char c1,c2,p;
		
	if(style != CoOrdinate)
	{
		
		if(M.Castle==1)	
		{
			if(pBuffer==NULL)
				printf_s(" O-O\n");
			else
				sprintf (pBuffer," O-O\n");
			return; 
		}

		if(M.CastleLong==1)	
		{
			if(pBuffer==NULL)
				printf_s(" O-O-O\n");
			else
				sprintf (pBuffer," O-O-O\n");
			return; 
		}
	}

	for (int q=0;q<64;q++)
	{
		if (bbFrom & (1i64 << q))
		{
			from = q; 
			break;
		}
	}

	for (int q=0;q<64;q++)
	{
		if (bbTo & (1i64 << q))
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
	sprintf_s(s, "");
	if((style == LongAlgebraic ) || (style == LongAlgebraicNoNewline))
	{
		// Determine if move is a capture
		if (M.Capture || M.EnPassantCapture)
			sprintf_s(s,"%c%c%dx%c%d",p,c1,1+(from >> 3),c2,1+(to >> 3));
		else
			sprintf_s(s,"%c%c%d-%c%d",p,c1,1+(from >> 3),c2,1+(to >> 3));
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
		if(pBuffer==NULL)
			printf_s(s);
		else
			strcpy(pBuffer,s);
	}

	if(style == CoOrdinate)
	{
		sprintf_s(s,"%c%d%c%d",c1,1+(from >> 3),c2,1+(to >> 3));
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
		if(pBuffer==NULL)
			printf_s(s);
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
	}while (++i<256);
}

