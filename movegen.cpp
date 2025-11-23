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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <cassert>

namespace juddperft {

//////////////////////////////////////////////
// Implementation of Class ChessPosition	//
//////////////////////////////////////////////

ChessPosition::ChessPosition()
{
    ChessPosition::A = 0;
    ChessPosition::B = 0;
    ChessPosition::C = 0;
    ChessPosition::D = 0;
    ChessPosition::Flags = 0;

#ifdef _USE_HASH
    ChessPosition::HK = 0;
#endif

}

ChessPosition& ChessPosition::setupStartPosition()
{
    ChessPosition::Flags = 0;
    ChessPosition::A = 0x4Aff00000000ff4A;
    ChessPosition::B = 0x3C0000000000003C;
    ChessPosition::C = 0xDB000000000000DB;
    ChessPosition::D = 0xffff000000000000;
    ChessPosition::BlackCanCastle = 1;
    ChessPosition::WhiteCanCastle = 1;
    ChessPosition::BlackCanCastleLong = 1;
    ChessPosition::WhiteCanCastleLong = 1;
    ChessPosition::BlackForfeitedCastle = 0;
    ChessPosition::WhiteForfeitedCastle = 0;
    ChessPosition::BlackForfeitedCastleLong = 0;
    ChessPosition::WhiteForfeitedCastleLong = 0;
    ChessPosition::BlackDidCastle = 0;
    ChessPosition::WhiteDidCastle = 0;
    ChessPosition::BlackDidCastleLong = 0;
    ChessPosition::WhiteDidCastleLong = 0;
    ChessPosition::MoveNumber = 1;
    ChessPosition::HalfMoves = 0;
    ChessPosition::calculateMaterial();

#ifdef _USE_HASH
    ChessPosition::calculateHash();
#endif

    return *this;
}

#ifdef _USE_HASH
ChessPosition & ChessPosition::calculateHash()
{
    ChessPosition::HK = 0;

    // Scan the squares:
    for (unsigned int q = 0; q < 64; q++)
    {
        piece_t piece = getPieceAtSquare(q);
        //assert(piece != 0x08);
        if (piece & 0x7) {
            ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[piece][q];
        }
    }
    if (ChessPosition::BlackToMove)	ChessPosition::HK ^= zobristKeys.zkBlackToMove;
    if (ChessPosition::WhiteCanCastle) ChessPosition::HK ^= zobristKeys.zkWhiteCanCastle;
    if (ChessPosition::WhiteCanCastleLong) ChessPosition::HK ^= zobristKeys.zkWhiteCanCastleLong;
    if (ChessPosition::BlackCanCastle) ChessPosition::HK ^= zobristKeys.zkBlackCanCastle;
    if (ChessPosition::BlackCanCastleLong) ChessPosition::HK ^= zobristKeys.zkBlackCanCastleLong;

    return *this;
}
#endif

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

ChessPosition& ChessPosition::performMove(ChessMove M)
{
    BitBoard To = 1LL << M.ToSquare;
    const BitBoard O = ~((1LL << M.FromSquare) | To);
    unsigned long nFromSquare = M.FromSquare;
    unsigned long nToSquare = M.ToSquare;

    // if move is known to be delivering checkmate, immediately flag checkmate in the position
    if (M.Checkmate && M.BlackToMove == BlackToMove) {
        if (M.BlackToMove) {
            WhiteIsCheckmated = 1;
        } else {
            BlackIsCheckmated = 1;
        }
    }

    // CLEAR EP SQUARES :
    // clear any enpassant squares
    const BitBoard EnPassant = A & B & (~C);
    ChessPosition::A &= ~EnPassant;
    ChessPosition::B &= ~EnPassant;
    ChessPosition::C &= ~EnPassant;
    ChessPosition::D &= ~EnPassant;

#ifdef _USE_HASH
    unsigned long nEPSquare;
    nEPSquare = getSquareIndex(EnPassant);
    ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[WENPASSANT][nEPSquare]; // Remove EP from nEPSquare
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

    if (M.Piece == BKING)
    {
        if (M.Castle)
        {
            ChessPosition::A ^= 0x0a00000000000000;
            ChessPosition::B ^= 0x0a00000000000000;
            ChessPosition::C ^= 0x0f00000000000000;
            ChessPosition::D ^= 0x0f00000000000000;

#ifdef _USE_HASH
            HK ^= zobristKeys.zkDoBlackCastle;
            if (ChessPosition::BlackCanCastleLong) ChessPosition::HK ^= zobristKeys.zkBlackCanCastleLong;	// conditionally flip black castling long
#endif

            ChessPosition::BlackDidCastle = 1;
            ChessPosition::BlackCanCastle = 0;
            ChessPosition::BlackCanCastleLong = 0;
            return *this;
        }
        else if (M.CastleLong)
        {
            ChessPosition::A ^= 0x2800000000000000;
            ChessPosition::B ^= 0x2800000000000000;
            ChessPosition::C ^= 0xb800000000000000;
            ChessPosition::D ^= 0xb800000000000000;

#ifdef _USE_HASH
            HK ^= zobristKeys.zkDoBlackCastleLong;
            if (ChessPosition::BlackCanCastle) ChessPosition::HK ^= zobristKeys.zkBlackCanCastle;	// conditionally flip black castling
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
                if (ChessPosition::BlackCanCastle) ChessPosition::HK ^= zobristKeys.zkBlackCanCastle;	// conditionally flip black castling
                if (ChessPosition::BlackCanCastleLong) ChessPosition::HK ^= zobristKeys.zkBlackCanCastleLong;	// conditionally flip black castling long
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
        if (M.Castle)
        {
            ChessPosition::A ^= 0x000000000000000a;
            ChessPosition::B ^= 0x000000000000000a;
            ChessPosition::C ^= 0x000000000000000f;
            ChessPosition::D &= 0xfffffffffffffff0;	// clear colour of e1, f1, g1, h1 (make white)

#ifdef _USE_HASH
            HK ^= zobristKeys.zkDoWhiteCastle;
            if (ChessPosition::WhiteCanCastleLong) ChessPosition::HK ^= zobristKeys.zkWhiteCanCastleLong;	// conditionally flip white castling long
#endif

            ChessPosition::WhiteDidCastle = 1;
            ChessPosition::WhiteCanCastle = 0;
            ChessPosition::WhiteCanCastleLong = 0;
            return *this;
        }

        if (M.CastleLong)
        {

            ChessPosition::A ^= 0x0000000000000028;
            ChessPosition::B ^= 0x0000000000000028;
            ChessPosition::C ^= 0x00000000000000b8;
            ChessPosition::D &= 0xffffffffffffff07;	// clear colour of a1, b1, c1, d1, e1 (make white)

#ifdef _USE_HASH
            HK ^= zobristKeys.zkDoWhiteCastleLong;
            if (ChessPosition::WhiteCanCastle) ChessPosition::HK ^= zobristKeys.zkWhiteCanCastle;	// conditionally flip white castling

#endif

            ChessPosition::WhiteDidCastleLong = 1;
            ChessPosition::WhiteCanCastle = 0;
            ChessPosition::WhiteCanCastleLong = 0;
            return *this;
        }

        else
        {
            // ordinary king move
            if (ChessPosition::WhiteCanCastle) {

#ifdef _USE_HASH
                ChessPosition::HK ^= zobristKeys.zkWhiteCanCastle;	// flip white castling
#endif
                ChessPosition::WhiteForfeitedCastle = 1;
                ChessPosition::WhiteCanCastle = 0;

            }
            if (ChessPosition::WhiteCanCastleLong) {

#ifdef _USE_HASH
                ChessPosition::HK ^= zobristKeys.zkWhiteCanCastleLong;	// flip white castling long
#endif

                ChessPosition::WhiteForfeitedCastleLong = 1;
                ChessPosition::WhiteCanCastleLong = 0;
            }
        }
    }

    // LOOK FOR FORFEITED CASTLING RIGHTS DUE to ROOK MOVES:
    else if (M.Piece == BROOK)
    {
        if (nFromSquare == 56)
        {
            // Black moved K-side Rook and forfeits right to castle K-side
            if (ChessPosition::BlackCanCastle){
                ChessPosition::BlackForfeitedCastle = 1;

#ifdef _USE_HASH
                ChessPosition::HK ^= zobristKeys.zkBlackCanCastle;	// flip black castling
#endif

                ChessPosition::BlackCanCastle = 0;
            }
        } else if (nFromSquare == 63) {
            // Black moved the QS Rook and forfeits right to castle Q-side
            if (ChessPosition::BlackCanCastleLong) {
                ChessPosition::BlackForfeitedCastleLong = 1;

#ifdef _USE_HASH
                ChessPosition::HK ^= zobristKeys.zkBlackCanCastleLong;	// flip black castling long
#endif

                ChessPosition::BlackCanCastleLong = 0;
            }
        }
    }

    else if (M.Piece == WROOK)
    {
        if (nFromSquare == 0)
        {
            // White moved K-side Rook and forfeits right to castle K-side
            if (ChessPosition::WhiteCanCastle) {
                ChessPosition::WhiteForfeitedCastle = 1;

#ifdef _USE_HASH
                ChessPosition::HK ^= zobristKeys.zkWhiteCanCastle;	// flip white castling BROKEN !!!
#endif

                ChessPosition::WhiteCanCastle = 0;
            }
        } else if (nFromSquare == 7) {
            // White moved the QSide Rook and forfeits right to castle Q-side
            if (ChessPosition::WhiteCanCastleLong) {
                ChessPosition::WhiteForfeitedCastleLong = 1;

#ifdef _USE_HASH
                ChessPosition::HK ^= zobristKeys.zkWhiteCanCastleLong;	// flip white castling long
#endif

                ChessPosition::WhiteCanCastleLong = 0;
            }
        }
    }

    // Ordinary Captures	////
    if (M.Capture)
    {
        int capturedpiece;
        // find out which piece has been captured:
        capturedpiece = ((ChessPosition::D & To) >> nToSquare) << 3
                        | ((ChessPosition::C & To) >> nToSquare) << 2
                        | ((ChessPosition::B & To) >> nToSquare) << 1
                        | ((ChessPosition::A & To) >> nToSquare);

#ifdef _USE_HASH
        // Update Hash
        ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[capturedpiece][nToSquare]; // Remove captured Piece
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
    ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[M.Piece][nFromSquare]; // Remove piece at From square
    ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[M.Piece][nToSquare]; // Place piece at To Square
#endif

    // Promotions - Change the piece:

    if (M.PromoteBishop)
    {
        ChessPosition::A &= ~To;
        ChessPosition::B |= To;

#ifdef _USE_HASH
        ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[M.BlackToMove ? BPAWN : WPAWN][nToSquare]; // Remove pawn at To square
        ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[M.BlackToMove ? BBISHOP : WBISHOP][nToSquare]; // place Bishop at To square
#endif

        M.Piece = M.BlackToMove ? BBISHOP : WBISHOP;

        return *this;
    }

    else if (M.PromoteKnight)
    {
        ChessPosition::C |= To;

#ifdef _USE_HASH
        ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[M.BlackToMove ? BPAWN : WPAWN][nToSquare]; // Remove pawn at To square
        ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[M.BlackToMove ? BKNIGHT : WKNIGHT][nToSquare];// place Knight at To square
#endif
        M.Piece = M.BlackToMove ? BKNIGHT : WKNIGHT;

        return *this;
    }

    else if (M.PromoteRook)
    {
        ChessPosition::A &= ~To;
        ChessPosition::C |= To;

#ifdef _USE_HASH
        ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[M.BlackToMove ? BPAWN : WPAWN][nToSquare]; // Remove pawn at To square
        ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[M.BlackToMove ? BROOK : WROOK][nToSquare];	// place Rook at To square
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
        ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[M.BlackToMove ? BPAWN : WPAWN][nToSquare]; // Remove pawn at To square
        ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[M.BlackToMove ? BQUEEN : WQUEEN][nToSquare];	// place Queen at To square
#endif

        M.Piece = M.BlackToMove ? BQUEEN : WQUEEN;
        return *this;
    }

    // For Double-Pawn Moves, set EP square:
    else if (M.DoublePawnMove)
    {
        // Set EnPassant Square
        if (M.BlackToMove)
        {
            To <<= 8;
            ChessPosition::A |= To;
            ChessPosition::B |= To;
            ChessPosition::C &= ~To;
            ChessPosition::D |= To;

#ifdef _USE_HASH
            ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[BENPASSANT][nToSquare + 8];	// Place Black EP at (To+8)
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
            ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[WENPASSANT][nToSquare - 8];	// Place White EP at (To-8)
#endif
        }

        return *this;
    }

    // En-Passant Captures	////
    else if (M.EnPassantCapture)
    {
        // remove the actual pawn (it is different to the capture square)
        if (M.BlackToMove)
        {
            To <<= 8;
            ChessPosition::A &= ~To; // clear the pawn's square
            ChessPosition::B &= ~To;
            ChessPosition::C &= ~To;
            ChessPosition::D &= ~To;

#ifdef _USE_HASH
            ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[WPAWN][nToSquare + 8]; // Remove WHITE Pawn at (To+8)
#endif

        } else {
            To >>= 8;
            ChessPosition::A &= ~To;
            ChessPosition::B &= ~To;
            ChessPosition::C &= ~To;
            ChessPosition::D &= ~To;

#ifdef _USE_HASH
            ChessPosition::HK ^= zobristKeys.zkPieceOnSquare[BPAWN][nToSquare - 8]; // Remove BLACK Pawn at (To-8)
#endif
        }
    }

    return *this;
}

void ChessPosition::switchSides()
{
    ChessPosition::BlackToMove ^= 1;

#ifdef _USE_HASH
    ChessPosition::HK ^= zobristKeys.zkBlackToMove;
#endif

    return;
}

void ChessPosition::clear(void)
{
    A = B = C = D = 0;
    Flags = 0;

#ifdef _USE_HASH
    HK = 0;
#endif
}

////////////////////////////////////////////
// Move Generation Functions
// generateMoves()
////////////////////////////////////////////

void generateMoves(const ChessPosition& P, ChessMove* pM)
{
    assert((~(P.A | P.B | P.C) & P.D) == 0); // Should not be any "black" empty squares

    if (P.BlackToMove) {
        genBlackMoves(P, pM);
    } else {
        genWhiteMoves(P, pM);
    }
}

//////////////////////////////////////////
// White Move Generation Functions:		//
// genWhiteMoves(), 					//
// addWhiteMoveToListIfLegal(), 		//
// genBlackAttacks()					//
//////////////////////////////////////////

void genWhiteMoves(const ChessPosition& P, ChessMove* pM)
{
    if (P.WhiteIsCheckmated || P.WhiteIsStalemated) {
        pM->MoveCount = 0;
        pM->FromSquare = 0;
        pM->ToSquare = 0;
        pM->Piece = 0;
        pM->EndOfMoveList = 1;
        return;
    }

    ChessMove* pFirstMove = pM;
    const BitBoard Occupied = P.A | P.B | P.C;								// all squares occupied by something
    const BitBoard WhiteOccupied = (Occupied & ~P.D) & ~(P.A & P.B & ~P.C);	// all squares occupied by W, excluding EP Squares
    assert(WhiteOccupied != 0);
    const BitBoard BlackOccupied = Occupied & P.D;							// all squares occupied by B, including Black EP Squares
    const BitBoard WhiteFree // all squares where W is free to move
        = (P.A & P.B & ~P.C)        // any EP square
          |	~(Occupied)				// any vacant square
          |	(~P.A & P.D)			// Black Bishop, Rook or Queen
          |	(~P.B & P.D);			// Black Pawn or Knight

    const BitBoard SolidBlackPiece = P.D & ~(P.A & P.B); // All black pieces except enpassants and black king

    unsigned long firstSq = 0;
    unsigned long lastSq = 63;
    getFirstAndLastPiece(WhiteOccupied, firstSq, lastSq);

    for (unsigned int q = firstSq; q <= lastSq; q++) {
        const BitBoard fromSQ = 1LL << q;
        const piece_t piece = P.getPieceAtSquare(q);

        BitBoard toSq;
        ChessMove M; // Dummy Move object used for setting flags.

        switch (piece) {
        case WEMPTY:
        case BEMPTY:
            continue;
        case WPAWN:
        {
            // single move forward
            toSq = MoveUp[q] & WhiteFree & ~BlackOccupied /* pawns can't capture in forward moves */;
            if ((toSq & RANK8) != 0) {
                addWhitePromotionsToListIfLegal(P, pM, q, toSq);
            } else {
                // Ordinary Pawn Advance
                addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
                /* double move forward (only available from 2nd Rank */
                if ((fromSQ & RANK2) != 0) {
                    toSq = moveUpSingleOccluded(toSq, WhiteFree) & ~BlackOccupied;
                    M.ClearFlags();
                    M.DoublePawnMove = 1; // this flag will cause ChessPosition::performMove() to set an ep square in the position
                    addWhiteMoveToListIfLegal(P, pM, q, toSq, piece, M.Flags);
                }
            }

            // generate Pawn Captures:
            toSq = MoveUpLeft[q] & WhiteFree & BlackOccupied;
            if ((toSq & RANK8) != 0) {
                addWhitePromotionsToListIfLegal(P, pM, q, toSq);
            } else {
                // Ordinary Pawn Capture to Left
                addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            }

            toSq = MoveUpRight[q] & WhiteFree & BlackOccupied;
            if ((toSq & RANK8) != 0) {
                addWhitePromotionsToListIfLegal(P, pM, q, toSq);
            } else {
                // Ordinary Pawn Capture to right
                addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            }
        }
        break;

        case WENPASSANT:
            continue;
        case WKNIGHT:
        {
            toSq = MoveKnight8[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight7[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight1[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight2[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight3[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight4[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight5[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight6[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
        }
        break;
        case WKING:
        {
            toSq = MoveUp[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);

            toSq = MoveRight[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);

            // Conditionally generate O-O move:
            if ((P.WhiteCanCastle) &&								// White still has castle rights AND
                (fromSQ == WHITEKINGPOS) &&					// King is in correct Position AND
                ((~P.A & ~P.B & P.C & ~P.D & WHITEKRPOS) != 0) &&	// KRook is in correct Position AND
                (pM->IllegalMove == 0) &&							// Last generated move (1 step to right) was legal AND
                (WHITECASTLEZONE & Occupied) == 0 &&				// Castle Zone (f1, g1) is clear AND
                !isWhiteInCheck(P)									// King is not in Check
                )
            {
                // OK to Castle
                M.ClearFlags();
                M.Castle = 1;
                addWhiteCastlingMoveIfLegal(P, pM, piece, M.Flags);
            }

            toSq = MoveDown[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);

            toSq = MoveLeft[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);

            // Conditionally generate O-O-O move:
            if ((P.WhiteCanCastleLong)	&&							// White still has castle-long rights AND
                (fromSQ == WHITEKINGPOS) &&					// King is in correct Position AND
                ((~P.A & ~P.B & P.C & ~P.D & WHITEQRPOS) != 0) &&	// QRook is in correct Position AND
                (pM->IllegalMove == 0) &&							// Last generated move (1 step to left) was legal AND
                (WHITECASTLELONGZONE & Occupied) == 0 &&	 		// Castle Long Zone (b1, c1, d1) is clear AND
                !isWhiteInCheck(P)									// King is not in Check
                )
            {
                // Ok to Castle Long
                M.ClearFlags();
                M.CastleLong = 1;
                addWhiteCastlingMoveIfLegal(P, pM, piece, M.Flags);
            }
            toSq = MoveUpRight[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveDownRight[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveDownLeft[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveUpLeft[q] & WhiteFree;
            addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
        }
        break;
        case WBISHOP:
        case WROOK:
        case WQUEEN:
        {
            const BitBoard B = P.B & fromSQ;
            const BitBoard C = P.C & fromSQ;

            if (B != 0)
            {	/* diagonal slider (either B or Q) */

                toSq = fromSQ;
                do{ /* Diagonal UpRight */
                    toSq = moveUpRightSingleOccluded(toSq, WhiteFree);
                    addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidBlackPiece);

                toSq = fromSQ;
                do{ /* Diagonal DownRight */
                    toSq = moveDownRightSingleOccluded(toSq, WhiteFree);
                    addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidBlackPiece);

                toSq = fromSQ;
                do{ /* Diagonal DownLeft */
                    toSq = moveDownLeftSingleOccluded(toSq, WhiteFree);
                    addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidBlackPiece);

                toSq = fromSQ;
                do{ /* Diagonal UpLeft */
                    toSq = moveUpLeftSingleOccluded(toSq, WhiteFree);
                    addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidBlackPiece);
            }

            if (C != 0)
            {	/* vertical slider (either R or Q) */

                toSq = fromSQ;
                do{ /* Up */
                    toSq = moveUpSingleOccluded(toSq, WhiteFree);
                    addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidBlackPiece);

                toSq = fromSQ;
                do{ /* Right */
                    toSq = moveRightSingleOccluded(toSq, WhiteFree);
                    addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidBlackPiece);

                toSq = fromSQ;
                do{ /*Down */
                    toSq = moveDownSingleOccluded(toSq, WhiteFree);
                    addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidBlackPiece);

                toSq = fromSQ;
                do{ /*Left*/
                    toSq = moveLeftSingleOccluded(toSq, WhiteFree);
                    addWhiteMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidBlackPiece);
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

    // put the move count into the first move
    pFirstMove->MoveCount = pM - pFirstMove;

    // Create 'no more moves' move to mark end of list:
    pM->FromSquare = 0;
    pM->ToSquare = 0;
    pM->Piece = 0;
    pM->EndOfMoveList = 1;
}

inline void scanWhiteMoveForChecks(ChessPosition& Q, ChessMove* pM)
{
    // test if white's move will put black in check or checkmate
    if (isBlackInCheck(Q))	{
        pM->Check = 1;
        Q.BlackToMove = 1;
        ChessMove blacksMoves[MOVELIST_SIZE];
        Q.DontGenerateAllMoves = 1; // only need one or more moves to prove that black has at least one legal move
        genBlackMoves(Q, blacksMoves);
        if (blacksMoves->MoveCount == 0) { // black will be in check with no legal moves
            pM->Checkmate = 1; // this move is a checkmating move
        }
    } else {
        pM->Check = 0;
        pM->Checkmate = 0;
    }
}

inline void addWhiteCastlingMoveIfLegal(const ChessPosition& P, ChessMove*& pM, int32_t piece, int32_t flags)
{
    ChessPosition Q = P;
    pM->FromSquare = static_cast<unsigned char>(SquareIndex::e1);
    pM->Flags = flags;
    pM->Piece = piece;

    if (pM->CastleLong) {
        Q.A ^= 0x0000000000000028;
        Q.B ^= 0x0000000000000028;
        Q.C ^= 0x00000000000000b8;
        Q.D &= 0xffffffffffffff07;	// clear colour of a1, b1, c1, d1, e1 (make white)
        pM->ToSquare = static_cast<unsigned char>(SquareIndex::c1);
    } else {
        Q.A ^= 0x000000000000000a;
        Q.B ^= 0x000000000000000a;
        Q.C ^= 0x000000000000000f;
        Q.D &= 0xfffffffffffffff0;	// clear colour of e1, f1, g1, h1 (make white)
        pM->ToSquare = static_cast<unsigned char>(SquareIndex::g1);
    }

    if (isWhiteInCheck(Q)) {
        pM->IllegalMove = 1;
    } else {
        scanWhiteMoveForChecks(Q, pM);
        pM++; // Add to list (advance pointer)
        pM->Flags = 0;
    }
}

inline void addWhiteMoveToListIfLegal(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, BitBoard to, int32_t piece, int32_t flags)
{
    if (to != 0)
    {
        ChessPosition Q = P;
        pM->FromSquare = fromsquare;
        pM->ToSquare = static_cast<unsigned char>(getSquareIndex(to));
        assert((1ULL << pM->ToSquare) == to);
        pM->Flags = flags;
        pM->Piece = piece;

        const BitBoard O = ~((1LL << fromsquare) | to);

        // clear old and new square:
        Q.A &= O;
        Q.B &= O;
        Q.C &= O;
        Q.D &= O;

        //// Populate new square (Branchless method, BitBoard input):
        // Q.A |= to & -( static_cast<int64_t>(piece) & 1);
        // Q.B |= to & -((static_cast<int64_t>(piece) & 2) >> 1);
        // Q.C |= to & -((static_cast<int64_t>(piece) & 4) >> 2);
        // Q.D |= to & -((static_cast<int64_t>(piece) & 8) >> 3);

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

        if (isWhiteInCheck(Q)) {
            pM->IllegalMove = 1;
        } else {
            scanWhiteMoveForChecks(Q, pM);
            pM++; // Add to list (advance pointer)
            pM->Flags = 0;
        }
    }
}

inline void addWhitePromotionsToListIfLegal(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, BitBoard to, int32_t flags /*=0*/)
{
    if (to != 0) {
        pM->FromSquare = fromsquare;
        pM->ToSquare = static_cast<unsigned char>(getSquareIndex(to));
        assert((1ULL << pM->ToSquare) == to);
        pM->Flags = flags;
        pM->Piece = WPAWN;

        // Test for capture:
        BitBoard PAB = (P.A & P.B);	// Bitboard containing EnPassants and kings:
        pM->Capture = (to & P.D & ~PAB) ? 1 : 0;

        ChessPosition Q = P;
        BitBoard O = ~((1LL << fromsquare) | to);

        // clear old and new square:
        Q.A &= O;
        Q.B &= O;
        Q.C &= O;
        Q.D &= O;

        // promote to queen
        Q.B |= to;
        Q.C |= to;

        if (isWhiteInCheck(Q))	{
            pM->IllegalMove = 1;
        } else {
            // make an additional 3 copies for the underpromotions
            *(pM + 1) = *pM;
            *(pM + 2) = *pM;
            *(pM + 3) = *pM;

            pM->PromoteQueen = 1;
            scanWhiteMoveForChecks(Q, pM);

            // rook underpromotion
            pM++;
            pM->PromoteRook = 1;
            Q.B &= ~to;
            scanWhiteMoveForChecks(Q, pM);

            // bishop underpromotion
            pM++;
            pM->PromoteBishop = 1;
            Q.C &= ~to;
            Q.B |= to;
            scanWhiteMoveForChecks(Q, pM);

            // knight underpromotion
            pM++;
            pM->PromoteKnight = 1;
            Q.A |= to;
            Q.B &= ~to;
            Q.C |= to;
            scanWhiteMoveForChecks(Q, pM);

            pM++;
            pM->Flags = 0;
        }
    }
}

inline BitBoard genBlackAttacks(const ChessPosition& Z)
{
    BitBoard Occupied = Z.A | Z.B | Z.C;
    BitBoard Empty = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
                     ~Occupied;							// All Unoccupied squares

    BitBoard PotentialCapturesForBlack = Occupied & ~Z.D; // White Pieces (including Kings)

    BitBoard A = Z.A & Z.D;				// Black A-Plane
    BitBoard B = Z.B & Z.D;				// Black B-Plane
    BitBoard C = Z.C & Z.D;				// Black C-Plane

    BitBoard S = C & ~A;				// Straight-moving Pieces
    BitBoard D = B & ~A;				// Diagonal-moving Pieces
    BitBoard K = A & B & C;				// King
    BitBoard P = A & ~B & ~C;			// Pawns
    BitBoard N = A & ~B & C;			// Knights

    BitBoard StraightAttacks = moveUpSingleOccluded(fillUpOccluded(S, Empty), Empty | PotentialCapturesForBlack);
    StraightAttacks |= moveRightSingleOccluded(fillRightOccluded(S, Empty), Empty | PotentialCapturesForBlack);
    StraightAttacks |= moveDownSingleOccluded(fillDownOccluded(S, Empty), Empty | PotentialCapturesForBlack);
    StraightAttacks |= moveLeftSingleOccluded(fillLeftOccluded(S, Empty), Empty | PotentialCapturesForBlack);

    BitBoard DiagonalAttacks = moveUpRightSingleOccluded(fillUpRightOccluded(D, Empty), Empty | PotentialCapturesForBlack);
    DiagonalAttacks |= moveDownRightSingleOccluded(fillDownRightOccluded(D, Empty), Empty | PotentialCapturesForBlack);
    DiagonalAttacks |= moveDownLeftSingleOccluded(fillDownLeftOccluded(D, Empty), Empty | PotentialCapturesForBlack);
    DiagonalAttacks |= moveUpLeftSingleOccluded(fillUpLeftOccluded(D, Empty), Empty | PotentialCapturesForBlack);

    BitBoard KingAttacks = fillKingAttacksOccluded(K, Empty | PotentialCapturesForBlack);
    BitBoard KnightAttacks = fillKnightAttacksOccluded(N, Empty | PotentialCapturesForBlack);
    BitBoard PawnAttacks = MoveDownLeftRightSingle(P) & (Empty | PotentialCapturesForBlack);

    return (StraightAttacks | DiagonalAttacks | KingAttacks | KnightAttacks | PawnAttacks);
}

inline BitBoard isWhiteInCheck(const ChessPosition& Z)
{
    BitBoard WhiteKing = Z.A & Z.B & Z.C & ~Z.D;
    BitBoard V = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
                 WhiteKing |			// White King
                 ~(Z.A | Z.B | Z.C);	// All Unoccupied squares

    BitBoard A = Z.A & Z.D;				// Black A-Plane
    BitBoard B = Z.B & Z.D;				// Black B-Plane
    BitBoard C = Z.C & Z.D;				// Black C-Plane

    BitBoard S = C & ~A;				// Straight-moving Pieces
    BitBoard D = B & ~A;				// Diagonal-moving Pieces
    BitBoard K = A & B & C;				// King
    BitBoard P = A & ~B & ~C;			// Pawns
    BitBoard N = A & ~B & C;			// Knights

    BitBoard X = fillStraightAttacksOccluded(S, V);
    X |= fillDiagonalAttacksOccluded(D, V);
    X |= fillKingAttacks(K);
    X |= FillKnightAttacks(N);
    X |= MoveDownLeftRightSingle(P);
    return X & WhiteKing;
}

//////////////////////////////////////////
// Black Move Generation Functions:		//
// genBlackMoves(), 					//
// addBlackMoveToListIfLegal(), 		//
// genWhiteAttacks()					//
//////////////////////////////////////////

void genBlackMoves(const ChessPosition& P, ChessMove* pM)
{
    if (P.BlackIsCheckmated || P.BlackIsStalemated) {
        pM->MoveCount = 0;
        pM->FromSquare = 0;
        pM->ToSquare = 0;
        pM->Piece = 0;
        pM->EndOfMoveList = 1;
        return;
    }

    ChessMove* pFirstMove = pM;
    const BitBoard Occupied = P.A | P.B | P.C; // all squares occupied by something
    const BitBoard BlackOccupied = P.D & ~(P.A & P.B & ~P.C); // all squares occupied by B, excluding EP Squares
    assert(BlackOccupied != 0);
    const BitBoard WhiteOccupied = (Occupied & ~P.D); // all squares occupied by W, including white EP Squares
    const BitBoard BlackFree // all squares where B is free to move
        = (P.A & P.B & ~P.C)		// any EP square
          | ~(Occupied)				// any vacant square
          | (~P.A & ~P.D)				// White Bishop, Rook or Queen
          | (~P.B & ~P.D);			// White Pawn or Knight

    const BitBoard SolidWhitePiece = WhiteOccupied & ~(P.A & P.B); // All white pieces except enpassants and white king

    unsigned long firstSq = 0;
    unsigned long lastSq = 63;
    getFirstAndLastPiece(BlackOccupied, firstSq, lastSq);

    for (unsigned int q = firstSq; q <= lastSq; ++q) {
        const BitBoard fromSQ = 1LL << q;
        const piece_t piece = P.getPieceAtSquare(q);

        BitBoard toSq;
        ChessMove M; // Dummy Move object used for setting flags.

        switch (piece) {
        case WEMPTY:
        case BEMPTY:
            continue;
        case BPAWN:
        {
            // single move forward
            toSq = MoveDown[q] & BlackFree & ~WhiteOccupied /* pawns can't capture in forward moves */;
            if ((toSq & RANK1) != 0) {
                addBlackPromotionsToListIfLegal(P, pM, q, toSq);
            } else {
                // Ordinary Pawn Advance
                addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
                /* double move forward (only available from 7th Rank */
                if ((fromSQ & RANK7) != 0) {
                    toSq = moveDownSingleOccluded(toSq, BlackFree) & ~WhiteOccupied;
                    M.ClearFlags();
                    M.DoublePawnMove = 1; // this flag will cause ChessPosition::performMove() to set an ep square in the position
                    addBlackMoveToListIfLegal(P, pM, q, toSq, piece, M.Flags);
                }
            }

            // generate Pawn Captures:
            toSq = MoveDownLeft[q] & BlackFree & WhiteOccupied;
            if ((toSq & RANK1) != 0) {
                addBlackPromotionsToListIfLegal(P, pM, q, toSq);
            } else {
                // Ordinary Pawn Capture to Left
                addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            }

            toSq = MoveDownRight[q] & BlackFree & WhiteOccupied;
            if ((toSq & RANK1) != 0) {
                addBlackPromotionsToListIfLegal(P, pM, q, toSq);
            } else {
                // Ordinary Pawn Capture to right
                addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            }
        }
        break;
        case BENPASSANT:
            continue;
        case BKNIGHT:
        {
            toSq = MoveKnight1[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight2[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight3[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight4[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight5[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight6[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight7[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveKnight8[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
        }
        break;
        case BKING:
        {
            toSq = MoveUp[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);

            toSq = MoveRight[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);

            // Conditionally generate O-O move:
            if ((P.BlackCanCastle) &&								// Black still has castle rights AND
                (fromSQ == BLACKKINGPOS) &&					// King is in correct Position AND
                ((~P.A & ~P.B & P.C & P.D & BLACKKRPOS) != 0) &&	// KRook is in correct Position AND
                (pM->IllegalMove == 0) &&							// Last generated move (1 step to right) was legal AND
                (BLACKCASTLEZONE & Occupied) == 0 &&				// Castle Zone (f8, g8) is clear	AND
                !isBlackInCheck(P)									// King is not in Check
                )
            {
                // OK to Castle
                M.ClearFlags();
                M.Castle = 1;
                addBlackCastlingMoveToListIfLegal(P, pM, piece, M.Flags);
            }

            toSq = MoveDown[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);

            toSq = MoveLeft[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);

            // Conditionally generate O-O-O move:
            if ((P.BlackCanCastleLong) &&							// Black still has castle-long rights AND
                (fromSQ == BLACKKINGPOS) &&					// King is in correct Position AND
                ((~P.A & ~P.B & P.C & P.D & BLACKQRPOS) != 0) &&	// QRook is in correct Position AND
                (pM->IllegalMove == 0) &&							// Last generated move (1 step to left) was legal AND
                (BLACKCASTLELONGZONE & Occupied) == 0 &&			// Castle Long Zone (b8, c8, d8) is clear
                !isBlackInCheck(P)									// King is not in Check
                )
            {
                // OK to castle Long
                M.ClearFlags();
                M.CastleLong = 1;
                addBlackCastlingMoveToListIfLegal(P, pM, piece, M.Flags);
            }

            toSq = MoveUpRight[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveDownRight[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveDownLeft[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
            toSq = MoveUpLeft[q] & BlackFree;
            addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
        }
        break;

        case BBISHOP:
        case BROOK:
        case BQUEEN:
        {
            const BitBoard B = P.B & fromSQ;
            const BitBoard C = P.C & fromSQ;

            if (B != 0)
            { /* diagonal slider (either B or Q) */

                toSq = fromSQ;
                do{ /* Diagonal UpRight */
                    toSq = moveUpRightSingleOccluded(toSq, BlackFree);
                    addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidWhitePiece);

                toSq = fromSQ;
                do{ /* Diagonal DownRight */
                    toSq = moveDownRightSingleOccluded(toSq, BlackFree);
                    addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidWhitePiece);

                toSq = fromSQ;
                do{ /* Diagonal DownLeft */
                    toSq = moveDownLeftSingleOccluded(toSq, BlackFree);
                    addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidWhitePiece);

                toSq = fromSQ;
                do{ /* Diagonal UpLeft */
                    toSq = moveUpLeftSingleOccluded(toSq, BlackFree);
                    addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidWhitePiece);
            }

            if (C != 0)
            {   /* vertical slider (either R or Q) */

                toSq = fromSQ;
                do{ /* Up */
                    toSq = moveUpSingleOccluded(toSq, BlackFree);
                    addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidWhitePiece);

                toSq = fromSQ;
                do{ /* Right */
                    toSq = moveRightSingleOccluded(toSq, BlackFree);
                    addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidWhitePiece);

                toSq = fromSQ;
                do{ /*Down */
                    toSq = moveDownSingleOccluded(toSq, BlackFree);
                    addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidWhitePiece);

                toSq = fromSQ;
                do{ /*Left*/
                    toSq = moveLeftSingleOccluded(toSq, BlackFree);
                    addBlackMoveToListIfLegal(P, pM, q, toSq, piece);
                } while (toSq & ~SolidWhitePiece);
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

    // Hack to stuff the Move Count into the first Move:
    pFirstMove->MoveCount = pM - pFirstMove;

    // Create 'no more moves' move to mark end of list:
    pM->FromSquare = 0;
    pM->ToSquare = 0;
    pM->Piece = 0;
    pM->EndOfMoveList = 1;
}

inline void scanBlackMoveForChecks(ChessPosition& Q, ChessMove* pM)
{
    // test if black's move will put white in check or checkmate
    if (isWhiteInCheck(Q))	{
        pM->Check = 1;
        Q.BlackToMove = 0;
        ChessMove whitesMoves[MOVELIST_SIZE];
        Q.DontGenerateAllMoves = 1; // only need one or more moves to prove that white has at least one legal move
        genWhiteMoves(Q, whitesMoves);
        if (whitesMoves->MoveCount == 0) { // white will be in check with no legal moves
            pM->Checkmate = 1; // this move is a checkmating move
        }
    } else {
        pM->Check = 0;
        pM->Checkmate = 0;
    }
}

inline void addBlackCastlingMoveToListIfLegal(const ChessPosition& P, ChessMove*& pM, int32_t piece, int32_t flags/*=0*/)
{
    ChessPosition Q = P;
    pM->FromSquare = static_cast<unsigned char>(SquareIndex::e8);
    pM->Flags = flags;
    pM->BlackToMove = 1;
    pM->Piece = piece;

    if (pM->CastleLong) {
        Q.A ^= 0x2800000000000000;
        Q.B ^= 0x2800000000000000;
        Q.C ^= 0xb800000000000000;
        Q.D ^= 0xb800000000000000;
        pM->ToSquare = static_cast<unsigned char>(SquareIndex::c8);
    } else {
        Q.A ^= 0x0a00000000000000;
        Q.B ^= 0x0a00000000000000;
        Q.C ^= 0x0f00000000000000;
        Q.D ^= 0x0f00000000000000;
        pM->ToSquare = static_cast<unsigned char>(SquareIndex::g8);
    }

    if (isBlackInCheck(Q)) {
        pM->IllegalMove = 1;
    } else {
        scanBlackMoveForChecks(Q, pM);
        pM++; // Add to list (advance pointer)
        pM->Flags = 0;
    }
}

inline void addBlackMoveToListIfLegal(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, BitBoard to, int32_t piece, int32_t flags/*=0*/)
{
    if (to != 0)
    {
        ChessPosition Q = P;
        pM->FromSquare = fromsquare;
        pM->ToSquare = static_cast<unsigned char>(getSquareIndex(to));
        pM->Flags = flags;
        pM->BlackToMove = 1;
        pM->Piece = piece;

        const BitBoard O = ~((1LL << fromsquare) | to);

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

        if (isBlackInCheck(Q)) {
            pM->IllegalMove = 1;
        } else {
            scanBlackMoveForChecks(Q, pM);
            pM++; // Add to list (advance pointer)
            pM->Flags = 0;
        }
    }
}

inline void addBlackPromotionsToListIfLegal(const ChessPosition& P, ChessMove*& pM, unsigned char fromsquare, BitBoard to, int32_t flags/*=0*/)
{
    if (to != 0) {
        pM->FromSquare = fromsquare;
        pM->ToSquare = static_cast<unsigned char>(getSquareIndex(to));
        pM->Flags = flags;
        pM->BlackToMove = 1;
        pM->Piece = BPAWN;

        // Test for capture:
        BitBoard PAB = (P.A & P.B);	// Bitboard containing EnPassants and kings
        BitBoard WhiteOccupied = (P.A | P.B | P.C) & ~P.D;
        pM->Capture = (to & WhiteOccupied & ~PAB) ? 1 : 0;

        ChessPosition Q = P;
        const BitBoard O = ~((1LL << fromsquare) | to);

        // clear old and new square
        Q.A &= O;
        Q.B &= O;
        Q.C &= O;
        Q.D &= O;

        // promote to queen
        Q.D |= to;
        Q.B |= to;
        Q.C |= to;

        if (isBlackInCheck(Q)) {
            pM->IllegalMove = 1;
        } else {
            // make an additional 3 copies for underpromotions
            *(pM + 1) = *pM;
            *(pM + 2) = *pM;
            *(pM + 3) = *pM;

            pM->PromoteQueen = 1;
            scanBlackMoveForChecks(Q, pM);

            // rook underpromotion
            pM++;
            pM->PromoteRook = 1;
            Q.B &= ~to;
            scanBlackMoveForChecks(Q, pM);

            // bishop underpromotion
            pM++;
            pM->PromoteBishop = 1;
            Q.C &= ~to;
            Q.B |= to;
            scanBlackMoveForChecks(Q, pM);

            // knight underpromotion
            pM++;
            pM->PromoteKnight = 1;
            Q.A |= to;
            Q.B &= ~to;
            Q.C |= to;
            scanBlackMoveForChecks(Q, pM);

            pM++;
            pM->Flags = 0;
        }
    }
}

inline BitBoard genWhiteAttacks(const ChessPosition& Z)
{
    BitBoard Occupied = Z.A | Z.B | Z.C;
    BitBoard Empty = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
                     ~Occupied;							// All Unoccupied squares

    BitBoard PotentialCapturesForWhite = Occupied & Z.D; // Black Pieces (including Kings)

    BitBoard A = Z.A & ~Z.D;			// White A-Plane
    BitBoard B = Z.B & ~Z.D;			// White B-Plane
    BitBoard C = Z.C & ~Z.D;			// White C-Plane

    BitBoard S = C & ~A;				// Straight-moving Pieces
    BitBoard D = B & ~A;				// Diagonal-moving Pieces
    BitBoard K = A & B & C;				// King
    BitBoard P = A & ~B & ~C;			// Pawns
    BitBoard N = A & ~B & C;			// Knights

    BitBoard StraightAttacks = moveUpSingleOccluded(fillUpOccluded(S, Empty), Empty | PotentialCapturesForWhite);
    StraightAttacks |= moveRightSingleOccluded(fillRightOccluded(S, Empty), Empty | PotentialCapturesForWhite);
    StraightAttacks |= moveDownSingleOccluded(fillDownOccluded(S, Empty), Empty | PotentialCapturesForWhite);
    StraightAttacks |= moveLeftSingleOccluded(fillLeftOccluded(S, Empty), Empty | PotentialCapturesForWhite);

    BitBoard DiagonalAttacks = moveUpRightSingleOccluded(fillUpRightOccluded(D, Empty), Empty | PotentialCapturesForWhite);
    DiagonalAttacks |= moveDownRightSingleOccluded(fillDownRightOccluded(D, Empty), Empty | PotentialCapturesForWhite);
    DiagonalAttacks |= moveDownLeftSingleOccluded(fillDownLeftOccluded(D, Empty), Empty | PotentialCapturesForWhite);
    DiagonalAttacks |= moveUpLeftSingleOccluded(fillUpLeftOccluded(D, Empty), Empty | PotentialCapturesForWhite);

    BitBoard KingAttacks = fillKingAttacksOccluded(K, Empty | PotentialCapturesForWhite);
    BitBoard KnightAttacks = fillKnightAttacksOccluded(N, Empty | PotentialCapturesForWhite);
    BitBoard PawnAttacks = MoveUpLeftRightSingle(P) & (Empty | PotentialCapturesForWhite);

    return (StraightAttacks | DiagonalAttacks | KingAttacks | KnightAttacks | PawnAttacks);
}


inline BitBoard isBlackInCheck(const ChessPosition& Z)
{
    BitBoard BlackKing = Z.A & Z.B & Z.C & Z.D;
    BitBoard V = (Z.A & Z.B & ~Z.C) |	// All EP squares, regardless of colour
                 BlackKing |			// Black King
                 ~(Z.A | Z.B | Z.C);	// All Unoccupied squares

    BitBoard A = Z.A & ~Z.D;			// White A-Plane
    BitBoard B = Z.B & ~Z.D;			// White B-Plane
    BitBoard C = Z.C & ~Z.D;			// White C-Plane

    BitBoard S = C & ~A;				// Straight-moving Pieces
    BitBoard D = B & ~A;				// Diagonal-moving Pieces
    BitBoard K = A & B & C;				// King
    BitBoard P = A & ~B & ~C;			// Pawns
    BitBoard N = A & ~B & C;			// Knights

    BitBoard X = fillStraightAttacksOccluded(S, V);
    X |= fillDiagonalAttacksOccluded(D, V);
    X |= fillKingAttacks(K);
    X |= FillKnightAttacks(N);
    X |= MoveUpLeftRightSingle(P);

    return X & BlackKing;
}

// isInCheck() - Given a position, determines if player is in check -
// set IsBlack to true to test if Black is in check
// set IsBlack to false to test if White is in check.

inline bool isInCheck(const ChessPosition& P, bool bIsBlack)
{
    return bIsBlack ? isBlackInCheck(P) != 0 : isWhiteInCheck(P) != 0;
}

// Text Dump functions //////////////////

void dumpBitBoard(BitBoard b)
{
    printf("\n");
    for (int q = 63; q >= 0; q--)
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
// A number of notation styles are supported. Usually CoOrdinate should be used
// for WinBoard.
//////////////////////////////////////////////////////////////////////////////////
void dumpMove(ChessMove M, MoveNotationStyle style /* = LongAlgebraic */, char* pBuffer /*= NULL */)
{
    int from = 0, to = 0;
    BitBoard bbFrom, bbTo;
    bbFrom = 1LL << M.FromSquare;
    bbTo = 1LL << M.ToSquare;
    char ch1, ch2, p;

    if (style != CoOrdinate)
    {

        if (M.Castle == 1)
        {
            if (pBuffer == NULL)
                printf(" O-O\n");
            else
                sprintf (pBuffer, " O-O\n");
            return;
        }

        if (M.CastleLong == 1)
        {
            if (pBuffer == NULL)
                printf(" O-O-O\n");
            else
                sprintf (pBuffer, " O-O-O\n");
            return;
        }
    }

    for (int q = 0; q < 64; q++)
    {
        if (bbFrom & (1LL << q))
        {
            from = q;
            break;
        }
    }

    for (int q = 0; q < 64; q++)
    {
        if (bbTo & (1LL << q))
        {
            to = q;
            break;
        }
    }

    ch1 = 'h' - (from % 8);
    ch2 = 'h' - (to % 8);

    // Determine piece and assign character p accordingly:
    switch (M.Piece & 7)
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
    }
    //
    char s[SMALL_BUFFER_SIZE];  // output string
    *s = 0;

    if ((style == LongAlgebraic ) || (style == LongAlgebraicNoNewline))
    {
        // Determine if move is a capture
        if (M.Capture || M.EnPassantCapture)
            sprintf(s, "%c%c%dx%c%d", p, ch1, 1+(from >> 3), ch2, 1+(to >> 3));
        else
            sprintf(s, "%c%c%d-%c%d", p, ch1, 1+(from >> 3), ch2, 1+(to >> 3));
        // Promotions:
        if (M.PromoteBishop)
            strcat(s, "(B)");
        if (M.PromoteKnight)
            strcat(s, "(N)");
        if (M.PromoteRook)
            strcat(s, "(R)");
        if (M.PromoteQueen)
            strcat(s, "(Q)");
        //
        //
        if (style != LongAlgebraicNoNewline)
            strcat(s, "\n");
        else
            strcat(s, " ");
        //
        if (pBuffer == NULL)
            printf("%s", s);
        else
            strcpy(pBuffer, s);
    }

    if (style == CoOrdinate)
    {
        sprintf(s, "%c%d%c%d", ch1, 1 + (from >> 3), ch2, 1 + (to >> 3));
        // Promotions:
        if (M.PromoteBishop)
            strcat(s, "b");
        if (M.PromoteKnight)
            strcat(s, "n");
        if (M.PromoteRook)
            strcat(s, "r");
        if (M.PromoteQueen)
            strcat(s, "q");
        strcat(s, "\n");
        //
        if (pBuffer == NULL)
            printf("%s", s);
        else
            strcpy(pBuffer, s);
    }
}

void dumpMoveList(ChessMove* pMoveList, MoveNotationStyle style /* = LongAlgebraic */, char* pBuffer /*=NULL*/)
{
    ChessMove* pM = pMoveList;
    unsigned int i = 0;
    do{
        if (pM->ToSquare != pM->FromSquare)
            dumpMove(*pM, style, pBuffer);
        if (pM->EndOfMoveList != 0)
        {
            // No more moves
            break;
        }
        ++pM;
    } while (++i<MOVELIST_SIZE);
}

} // namespace juddperft
