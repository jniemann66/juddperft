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

#include "fen.h"

/////////////////////////////////////////////////////////	
// readFen() - Reads a FEN string and sets up position
// Accordingly..
/////////////////////////////////////////////////////////

namespace juddperft {

	bool readFen(ChessPosition* pP, const char* pzFENString)
	{
		const char* s = pzFENString;
		char* token;
		char t[1024];
		unsigned int piece;
		unsigned int shift;
		pP->clear();
		// Read Field 1: Piece Positions
		int Square = 63;

		while ((s != NULL) && (Square >= 0))
		{
			switch (*s)
			{
			case '/':
				piece = 0;
				shift = 0;
				break;
			case '1':
				piece = 0;
				shift = 1;
				break;
			case '2':
				piece = 0;
				shift = 2;
				break;
			case '3':
				piece = 0;
				shift = 3;
				break;
			case '4':
				piece = 0;
				shift = 4;
				break;
			case '5':
				piece = 0;
				shift = 5;
				break;
			case '6':
				piece = 0;
				shift = 6;
				break;
			case '7':
				piece = 0;
				shift = 7;
				break;
			case '8':
				piece = 0;
				shift = 8;
				break;
			case 'P':
				piece = WPAWN;
				shift = 1;
				break;
			case 'N':
				piece = WKNIGHT;
				shift = 1;
				break;
			case 'B':
				piece = WBISHOP;
				shift = 1;
				break;
			case 'R':
				piece = WROOK;
				shift = 1;
				break;
			case 'Q':
				piece = WQUEEN;
				shift = 1;
				break;
			case 'K':
				piece = WKING;
				shift = 1;
				break;
			case 'p':
				piece = BPAWN;
				shift = 1;
				break;
			case 'n':
				piece = BKNIGHT;
				shift = 1;
				break;
			case 'b':
				piece = BBISHOP;
				shift = 1;
				break;
			case 'r':
				piece = BROOK;
				shift = 1;
				break;
			case 'q':
				piece = BQUEEN;
				shift = 1;
				break;
			case 'k':
				piece = BKING;
				shift = 1;
				break;
			default:
				piece = 0;
				shift = 0;
			}

			pP->setPieceAtSquare(piece, 1LL << Square);
			Square -= shift;
			s++;
		} // ends while ((s != NULL) && (Square >= 0))
		// Now, tokenize remainder of string:
		strncpy(t, s, 1023);
		token = strtok(t, " ");
		// Read Field 2: Active Color (w or b)
		if (token != NULL)
		{
			if (strstr(token, "b") != NULL)
			{
				// Black to Move
				pP->BlackToMove = 1;
			}
			token = strtok(NULL, " ");
		}
		// Read Field 3: Castling availability (combination of K,Q,k,q)
		if (token != NULL)
		{
			if (strstr(token, "K") != NULL)
			{
				// White Can castle short
				pP->WhiteCanCastle = 1;
			}
			if (strstr(token, "Q") != NULL)
			{
				// White can Castle long
				pP->WhiteCanCastleLong = 1;
			}
			if (strstr(token, "k") != NULL)
			{
				// Black can castle short
				pP->BlackCanCastle = 1;
			}
			if (strstr(token, "q") != NULL)
			{
				// Black can castle long
				pP->BlackCanCastleLong = 1;
			}
			token = strtok(NULL, " ");
		}
		// Read Field 4: En Passant square (- or name of square: a3 through g3 or a6 through g6)
		if (token != NULL)
		{
			unsigned int iSquare = 0;
			unsigned int piece = 0;
			if ((*token >= 'a') && (*token <= 'g'))
				iSquare += 7 - (*token - 'a');
			if (*(token + 1) == '3')
			{
				iSquare += 16;
				piece = WENPASSANT;
			}
			if (*(token + 1) == '6')
			{
				iSquare += 40;
				piece = BENPASSANT;
			}
			if (((iSquare >= 16) && (iSquare <= 24)) ||
				((iSquare >= 40) && (iSquare <= 48)))
				pP->setPieceAtSquare(piece, 1LL << (iSquare));
			token = strtok(NULL, " ");
		}
		// Read Field 5: Halfmove clock (number of half moves since last capture or pawn advance)
		if (token != NULL)
		{
			pP->HalfMoves = atoi(token);
			token = strtok(NULL, " ");
		}
		// Read Field 6: Fullmove Number
		if (token != NULL)
		{
			pP->MoveNumber = atoi(token);
		}

#ifdef _USE_HASH
		pP->calculateHash(); // Important: always do this after setting up a position ! 
#endif

		return true;
	}

	/////////////////////////////////////////////////////////	
	// writeFen() - Generates a FEN string from a Position
	/////////////////////////////////////////////////////////

	bool writeFen(char* pzFENBuffer, const ChessPosition* pP)
	{
		unsigned int piece;
		unsigned int nBlanks = 0;
		int EPSquare = 0;
		char s[1024];

		for (int square = 63; square >= 0; --square)
		{
			piece = pP->getPieceAtSquare(1LL << square);
			if ((piece & 0x07) == 0) // Blank
				nBlanks++;
			else
			{
				if ((piece == WENPASSANT) ||
					(piece == BENPASSANT))
				{
					nBlanks++;
					EPSquare = square;
				}

				else
				{
					if (nBlanks != 0) {
						sprintf(s, "%d", nBlanks);
						strncat(pzFENBuffer, s, 1023);
					}
					switch (piece)
					{
					case WPAWN:
						strcat(pzFENBuffer, "P");
						break;
					case WBISHOP:
						strcat(pzFENBuffer, "B");
						break;
					case WROOK:
						strcat(pzFENBuffer, "R");
						break;
					case WKNIGHT:
						strcat(pzFENBuffer, "N");
						break;
					case WQUEEN:
						strcat(pzFENBuffer, "Q");
						break;
					case WKING:
						strcat(pzFENBuffer, "K");
						break;
					case BPAWN:
						strcat(pzFENBuffer, "p");
						break;
					case BBISHOP:
						strcat(pzFENBuffer, "b");
						break;
					case BROOK:
						strcat(pzFENBuffer, "r");
						break;
					case BKNIGHT:
						strcat(pzFENBuffer, "n");
						break;
					case BQUEEN:
						strcat(pzFENBuffer, "q");
						break;
					case BKING:
						strcat(pzFENBuffer, "k");
						break;
					} // ends switch
					nBlanks = 0;
				} // ends else	
			}


			if (square % 8 == 0)
			{
				// Reached Last File ('h')
				if (nBlanks != 0) {
					sprintf(s, "%d", nBlanks);
					strncat(pzFENBuffer, s, 1023);
				}
				if (square != 0)
					strcat(pzFENBuffer, "/");

				nBlanks = 0;
			}
		} // ends loop over square
		strcat(pzFENBuffer, " ");

		// Field 2: Side to Move
		if (pP->BlackToMove)
			strcat(pzFENBuffer, "b ");
		else
			strcat(pzFENBuffer, "w ");

		// Field 3: Castling Rights
		if ((pP->WhiteCanCastle == 0) &&
			(pP->WhiteCanCastleLong == 0) &&
			(pP->BlackCanCastle == 0) &&
			(pP->BlackCanCastleLong == 0))
			strcat(pzFENBuffer, "-");
		else
		{
			if (pP->WhiteCanCastle)
				strcat(pzFENBuffer, "K");

			if (pP->WhiteCanCastleLong)
				strcat(pzFENBuffer, "Q");

			if (pP->BlackCanCastle)
				strcat(pzFENBuffer, "k");

			if (pP->BlackCanCastleLong)
				strcat(pzFENBuffer, "q");
		}
		strcat(pzFENBuffer, " ");

		// Field 4: En passant
		if (EPSquare != 0)
		{
			char f = 'h' - EPSquare % 8;
			char r = '1' + EPSquare / 8;
			sprintf(s, "%c%c ", f, r);
			strncat(pzFENBuffer, s, 1023);
		}
		else
			strcat(pzFENBuffer, "- ");

		// Field 5: Half-Move Clock
		sprintf(s, "%d ", pP->HalfMoves);
		strncat(pzFENBuffer, s, 1023);

		// Field 6: Fullmove number
		sprintf(s, "%d", pP->MoveNumber);
		strncat(pzFENBuffer, s, 1023);

		return true;
	}

} // namespace juddperft