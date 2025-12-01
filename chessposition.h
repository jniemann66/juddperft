#ifndef CHESSPOSITION_H
#define CHESSPOSITION_H

//////////////////////////////////////////////
// chessposition.h							//
// Defines:									//
// Basic Types, type aliases, and enums		//
// class ChessPosition						//
//////////////////////////////////////////////

#include <cstdint>
#include <vector>

namespace juddperft {

using Bitboard = uint64_t;
using Hashkey = uint64_t;
using piece_t = uint32_t;

enum Piece : piece_t {
	WEMPTY = 0,
	WPAWN = 1,
	WBISHOP = 2,
	WENPASSANT = 3,
	WROOK = 4,
	WKNIGHT = 5,
	WQUEEN = 6,
	WKING = 7,
	BEMPTY = 8,
	BPAWN = 9,
	BBISHOP = 10,
	BENPASSANT = 11,
	BROOK = 12,
	BKNIGHT = 13,
	BQUEEN = 14,
	BKING = 15
};

// default material values of Pieces

const int pieceMaterialValue[16] =
{
	0,			// Empty Square
	100,		// White pawn
	300,		// White Bishop
	0,			// White En passant Square
	500,		// White Rook
	300,		// White Knight
	900,		// White Queen
	0,			// White King
	0,			// Unused
	-100,		// Black Pawn
	-300,		// Black Bishop
	0,			// Black En passant Square
	-500,		// Black Rook
	-300,		// Black Knight
	-900		// Black Queen
	,0			// Black King
};

class ChessMove;

// notes on naming conventions:
// ============================

// although I'm a big believer in using camelCase for variable names,
// I make an exception for Bitboard names.
// In here, (ie when writing code for generating moves etc),
// names of Bitboard variables are often represented using uppercase letters,
// typically to distinguish them from square indexes etc

class ChessPosition
{
	/* --------------------------------------------------------------

			Explanation of Bit-Representation used in positions:

			D (msb):	Colour (1=Black)
			C			1=Straight-Moving Piece (Q or R)
			B			1=Diagonal-moving piece (Q or B)
			A (lsb):	1=Pawn

			D	C	B	A

	(0)		0	0	0	0	Empty Square
	(1)		0	0	0	1	White Pawn
	(2)		0	0	1	0	White Bishop
	(3)		0	0	1	1	White En passant Square
	(4)		0	1	0	0	White Rook
	(5)		0	1	0	1	White Knight
	(6)		0	1	1	0	White Queen
	(7)		0	1	1	1	White King
	(8)		1	0	0	0	Reserved - Don't use (Black empty square)
	(9)		1	0	0	1	Black Pawn
	(10)	1	0	1	0	Black Bishop
	(11)	1	0	1	1	Black En passant Square
	(12)	1	1	0	0	Black Rook
	(13)	1	1	0	1	Black Knight
	(14)	1	1	1	0	Black Queen
	(15)	1	1	1	1	Black King

	---------------------------------------------------------------*/

public:
	Bitboard A;
	Bitboard B;
	Bitboard C;
	Bitboard D;

	union{
		struct{
			// move-generation options
			uint32_t dontGenerateAllMoves : 1; // used just to prove whether there is at least one legal move
			uint32_t dontDetectCheckmates : 1; // if set, generateMoves() will not test for 'IsCheckmated' flags
			uint32_t dontDetectChecks : 1; // if set, generateMoves() will not test for 'isInCheck' flags (implies dontDetectCheckmates)
			uint32_t unused : 10;
			// actual position flags
			uint32_t whiteCanCastle : 1;
			uint32_t whiteCanCastleLong : 1;
			uint32_t blackCanCastle : 1;
			uint32_t blackCanCastleLong : 1;
			uint32_t whiteForfeitedCastle : 1;
			uint32_t whiteForfeitedCastleLong : 1;
			uint32_t blackForfeitedCastle : 1;
			uint32_t blackForfeitedCastleLong : 1;
			uint32_t whiteDidCastle : 1;
			uint32_t whiteDidCastleLong : 1;
			uint32_t blackDidCastle : 1;
			uint32_t blackDidCastleLong : 1;
			uint32_t whiteIsInCheck : 1;
			uint32_t blackIsInCheck : 1;
			uint32_t whiteIsStalemated : 1;
			uint32_t blackIsStalemated : 1;
			uint32_t whiteIsCheckmated : 1;
			uint32_t blackIsCheckmated : 1;
			uint32_t blackToMove : 1;
		};
		uint32_t flags;
	};
	uint16_t moveNumber;
	uint16_t halfMoves;
	bool operator==(const ChessPosition& Q) {
		return((ChessPosition::A == Q.A) && (ChessPosition::B == Q.B) && (ChessPosition::C == Q.C) && (ChessPosition::D == Q.D) && (ChessPosition::flags == Q.flags));
	}

	Hashkey hk;

public:
	ChessPosition();
	ChessPosition& setupStartPosition();
	ChessPosition& calculateHash();

	ChessPosition& setPieceAtSquare(const piece_t& piece,  unsigned int s)
	{
		const Bitboard S = 1ull << s;

		// clear the square
		A &= ~S;
		B &= ~S;
		C &= ~S;
		D &= ~S;

		// populate the square
		A |= static_cast<int64_t>(piece & 1) << s;
		B |= static_cast<int64_t>((piece & 2) >> 1) << s;
		C |= static_cast<int64_t>((piece & 4) >> 2) << s;
		D |= static_cast<int64_t>((piece & 8) >> 3) << s;

		calculateHash();

		return *this;
	}

	piece_t getPieceAtSquare(unsigned int q) const
	{
		const Bitboard S = 1ull << q;

		Bitboard V = (D & S) >> q;
		V <<= 1;
		V |= (C & S) >> q;
		V <<= 1;
		V |= (B & S) >> q;
		V <<= 1;
		V |= (A & S) >> q;

		return static_cast<piece_t>(V);
	}

	// performMove() : applies a move to a position, including differential update to hash key
	ChessPosition& performMove(const ChessMove& M);

	// performMove() : applies a move to a position, without updating the kash key
	ChessPosition& performMoveNoHash(const ChessMove& M);

	std::vector<ChessMove> getLegalMoves() const;
	void getLegalMoves(ChessMove *movelist) const;

	void switchSides();
	void clear(void);
	int calculateMaterial() const;

	// print I/O functions
	void printPosition() const;
	static void printBitboard(Bitboard b);

private:
};

} // namespace juddperft

#endif // CHESSPOSITION_H
