#include "chessposition.h"
#include "zobristkeyset.h"

#include "movegen.h"

namespace juddperft {

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
	blackCanCastle = 1;
	whiteCanCastle = 1;
	blackCanCastleLong = 1;
	whiteCanCastleLong = 1;
	blackForfeitedCastle = 0;
	whiteForfeitedCastle = 0;
	blackForfeitedCastleLong = 0;
	whiteForfeitedCastleLong = 0;
	blackDidCastle = 0;
	whiteDidCastle = 0;
	blackDidCastleLong = 0;
	whiteDidCastleLong = 0;
	moveNumber = 1;
	halfMoves = 0;
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

	if (blackToMove)
		ChessPosition::hk ^= zobristKeys.zkBlackToMove;

	if (whiteCanCastle)
		ChessPosition::hk ^= zobristKeys.zkWhiteCanCastle;

	if (whiteCanCastleLong)
		ChessPosition::hk ^= zobristKeys.zkWhiteCanCastleLong;

	if (blackCanCastle)
		ChessPosition::hk ^= zobristKeys.zkBlackCanCastle;

	if (blackCanCastleLong)
		ChessPosition::hk ^= zobristKeys.zkBlackCanCastleLong;

	return *this;
}

int ChessPosition::calculateMaterial() const
{
	int material = 0;
	for (int q = 0; q < 64; q++) {
		material += pieceMaterialValue[getPieceAtSquare(q)];
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
	if (M.checkmate && M.blackToMove == blackToMove) {

		if (M.blackToMove) {
			whiteIsCheckmated = 1;
		} else {
			blackIsCheckmated = 1;
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
			if (blackCanCastleLong) {
				hk ^= zobristKeys.zkBlackCanCastleLong;	// flip black castling long
			}

			blackDidCastle = 1;
			blackCanCastle = 0;
			blackCanCastleLong = 0;
			return *this;
		}

		else if (M.castleLong) {
			A ^= 0x2800000000000000;
			B ^= 0x2800000000000000;
			C ^= 0xb800000000000000;
			D ^= 0xb800000000000000;

			hk ^= zobristKeys.zkDoBlackCastleLong;
			if (blackCanCastle) {
				hk ^= zobristKeys.zkBlackCanCastle;	// conditionally flip black castling
			}

			blackDidCastleLong = 1;
			blackCanCastle = 0;
			blackCanCastleLong = 0;
			return *this;
		}

		// ordinary king move; Black could have castled,
		// but chose to move the King in a non-castling move
		if (blackCanCastle) {
			hk ^= zobristKeys.zkBlackCanCastle;	// flip black castling
		}

		if (blackCanCastleLong) {
			hk ^= zobristKeys.zkBlackCanCastleLong;	// flip black castling long
		}

		blackForfeitedCastle = 1;
		blackForfeitedCastleLong = 1;
		blackCanCastle = 0;
		blackCanCastleLong = 0;
		break;

	case WKING:
		if (M.castle) {
			A ^= 0x000000000000000a;
			B ^= 0x000000000000000a;
			C ^= 0x000000000000000f;
			D &= 0xfffffffffffffff0;	// clear colour of e1, f1, g1, h1 (make white)

			hk ^= zobristKeys.zkDoWhiteCastle;
			if (whiteCanCastleLong) {
				hk ^= zobristKeys.zkWhiteCanCastleLong;	// conditionally flip white castling long
			}

			whiteDidCastle = 1;
			whiteCanCastle = 0;
			whiteCanCastleLong = 0;
			return *this;
		}

		else if (M.castleLong) {
			A ^= 0x0000000000000028;
			B ^= 0x0000000000000028;
			C ^= 0x00000000000000b8;
			D &= 0xffffffffffffff07;	// clear colour of a1, b1, c1, d1, e1 (make white)

			hk ^= zobristKeys.zkDoWhiteCastleLong;
			if (whiteCanCastle) {
				hk ^= zobristKeys.zkWhiteCanCastle;	// conditionally flip white castling
			}

			whiteDidCastleLong = 1;
			whiteCanCastle = 0;
			whiteCanCastleLong = 0;
			return *this;
		}

		// ordinary king move; White could have castled,
		// but chose to move the King in a non-castling move
		if (whiteCanCastle) {
			hk ^= zobristKeys.zkWhiteCanCastle;	// flip white castling
		}

		if (whiteCanCastleLong) {
			hk ^= zobristKeys.zkWhiteCanCastleLong;	// flip white castling long
		}

		whiteForfeitedCastle = 1;
		whiteCanCastle = 0;
		whiteForfeitedCastleLong = 1;
		whiteCanCastleLong = 0;
		break;

	case BROOK:
		if (nFromSquare == SquareIndex::h8) {
			// Black moved K-side Rook and forfeits right to castle K-side
			if (blackCanCastle){
				blackForfeitedCastle = 1;
				hk ^= zobristKeys.zkBlackCanCastle;	// flip black castling
				blackCanCastle = 0;
			}
		} else if (nFromSquare == SquareIndex::a8) {
			// Black moved the QS Rook and forfeits right to castle Q-side
			if (blackCanCastleLong) {
				blackForfeitedCastleLong = 1;
				hk ^= zobristKeys.zkBlackCanCastleLong;	// flip black castling long
				blackCanCastleLong = 0;
			}
		}
		break;

	case WROOK:
		if (nFromSquare == SquareIndex::h1) {
			// White moved K-side Rook and forfeits right to castle K-side
			if (whiteCanCastle) {
				whiteForfeitedCastle = 1;
				hk ^= zobristKeys.zkWhiteCanCastle;	// flip white castling BROKEN !!!
				whiteCanCastle = 0;
			}
		} else if (nFromSquare == SquareIndex::a1) {
			// White moved the QSide Rook and forfeits right to castle Q-side
			if (whiteCanCastleLong) {
				whiteForfeitedCastleLong = 1;
				hk ^= zobristKeys.zkWhiteCanCastleLong;	// flip white castling long
				whiteCanCastleLong = 0;
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

		// if a rook was captured, it may take away castling rights
		if (To & CORNERS) {
			switch (capturedpiece) {
			case WROOK:
				if (whiteCanCastle && (To & H1)) {
					hk ^= zobristKeys.zkWhiteCanCastle;
					whiteCanCastle = 0;
				} else if (whiteCanCastleLong && (To & A1)) {
					hk ^= zobristKeys.zkWhiteCanCastleLong;
					whiteCanCastleLong = 0;
				}
				break;
			case BROOK:
				if (blackCanCastle && (To & H8)){
					hk ^= zobristKeys.zkBlackCanCastle;
					blackCanCastle = 0;
				} else if (blackCanCastleLong && (To & A8)) {
					hk ^= zobristKeys.zkBlackCanCastleLong;
					blackCanCastleLong = 0;
				}
			default:
				break;
			}
		}
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

	if ((M.piece & 7) == WPAWN) {
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
	}

	return *this;
}

ChessPosition& ChessPosition::performMoveNoHash(const ChessMove& M)
{
	Bitboard To = 1ull << M.destination;
	const Bitboard O = ~((1ull << M.origin) | To);
	const unsigned long nFromSquare = M.origin;

	// if move is known to be delivering checkmate, immediately flag checkmate in the position
	if (M.checkmate && M.blackToMove == blackToMove) {
		if (M.blackToMove) {
			whiteIsCheckmated = 1;
		} else {
			blackIsCheckmated = 1;
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

			blackDidCastle = 1;
			blackCanCastle = 0;
			blackCanCastleLong = 0;
			return *this;
		}

		else if (M.castleLong) {
			A ^= 0x2800000000000000;
			B ^= 0x2800000000000000;
			C ^= 0xb800000000000000;
			D ^= 0xb800000000000000;

			blackDidCastleLong = 1;
			blackCanCastle = 0;
			blackCanCastleLong = 0;
			return *this;
		}

		blackForfeitedCastle = 1;
		blackForfeitedCastleLong = 1;
		blackCanCastle = 0;
		blackCanCastleLong = 0;
		break;

	case WKING:
		if (M.castle) {
			A ^= 0x000000000000000a;
			B ^= 0x000000000000000a;
			C ^= 0x000000000000000f;
			D &= 0xfffffffffffffff0;	// clear colour of e1, f1, g1, h1 (make white)

			whiteDidCastle = 1;
			whiteCanCastle = 0;
			whiteCanCastleLong = 0;
			return *this;
		}

		else if (M.castleLong) {
			A ^= 0x0000000000000028;
			B ^= 0x0000000000000028;
			C ^= 0x00000000000000b8;
			D &= 0xffffffffffffff07;	// clear colour of a1, b1, c1, d1, e1 (make white)

			whiteDidCastleLong = 1;
			whiteCanCastle = 0;
			whiteCanCastleLong = 0;
			return *this;
		}

		whiteForfeitedCastle = 1;
		whiteCanCastle = 0;
		whiteForfeitedCastleLong = 1;
		whiteCanCastleLong = 0;
		break;

	case BROOK:
		if (nFromSquare == SquareIndex::h8) {
			// Black moved K-side Rook and forfeits right to castle K-side
			if (blackCanCastle){
				blackForfeitedCastle = 1;
				blackCanCastle = 0;
			}
		} else if (nFromSquare == SquareIndex::a8) {
			// Black moved the QS Rook and forfeits right to castle Q-side
			if (blackCanCastleLong) {
				blackForfeitedCastleLong = 1;
				blackCanCastleLong = 0;
			}
		}
		break;

	case WROOK:
		if (nFromSquare == SquareIndex::h1) {
			// White moved K-side Rook and forfeits right to castle K-side
			if (whiteCanCastle) {
				whiteForfeitedCastle = 1;
				whiteCanCastle = 0;
			}
		} else if (nFromSquare == SquareIndex::a1) {
			// White moved the QSide Rook and forfeits right to castle Q-side
			if (whiteCanCastleLong) {
				whiteForfeitedCastleLong = 1;
				whiteCanCastleLong = 0;
			}
		}
		break;

	default:
		break;
	} // ends switch (M.Piece)

	// Ordinary Captures
	if (M.capture) {
		// find out what was captured
		const unsigned int capturedpiece
				= ((D & To) >> M.destination) << 3
												 | ((C & To) >> M.destination) << 2
												 | ((B & To) >> M.destination) << 1
												 | ((A & To) >> M.destination);

		// if a rook was captured, it may take away castling rights
		if (To & CORNERS) {
			switch (capturedpiece) {
			case WROOK:
				if (whiteCanCastle && (To & H1)) {
					whiteCanCastle = 0;
				} else if (whiteCanCastleLong && (To & A1)) {
					whiteCanCastleLong = 0;
				}

				break;
			case BROOK:
				if (blackCanCastle && (To & H8)){
					blackCanCastle = 0;
				} else if (blackCanCastleLong && (To & A8)) {
					blackCanCastleLong = 0;
				}

			default:
				break;
			}
		}
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

	if ((M.piece & 7) == WPAWN) {
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
	}

	return *this;
}

std::vector<ChessMove> ChessPosition::getLegalMoves() const
{
	std::vector<ChessMove> movelist(MOVELIST_SIZE);
	MoveGenerator::generateMoves(*this, movelist.data());
	movelist.resize(movelist.at(0).moveCount);
	return movelist;
}

void ChessPosition::getLegalMoves(ChessMove *movelist) const
{
	if (movelist) {
		MoveGenerator::generateMoves(*this, movelist);
	}
}

void ChessPosition::switchSides()
{
	blackToMove ^= 1;
	hk ^= zobristKeys.zkBlackToMove;
	return;
}

void ChessPosition::clear(void)
{
	A = B = C = D = 0;
	flags = 0;
	hk = 0;
}

// Text Print functions //////////////////

void ChessPosition::printBitboard(Bitboard b)
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

void ChessPosition::printPosition() const
{
	printf("\n---------------------------------\n");
	for (int q = 63; q >= 0; q--)
	{
		switch(getPieceAtSquare(q))
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
			if ((q == 56) && (blackCanCastle))
				printf("Black can Castle");
			if ((q == 48) && (blackCanCastleLong))
				printf("Black can Castle Long");
			if ((q == 40) && (whiteCanCastle))
				printf("White can Castle");
			if ((q == 32) && (whiteCanCastleLong))
				printf("White can Castle Long");
			if (q == 8) printf("material= %d", calculateMaterial());
			if (q == 0) printf("%s to move", blackToMove ? "Black" : "White");
			printf("\n---------------------------------\n");
		}
	}
}


} // namespace juddperft
