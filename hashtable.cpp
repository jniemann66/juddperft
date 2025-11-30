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

#include "hashtable.h"
#include "search.h"
#include "raiitimer.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <random>
#include <sstream>
#include <map>

namespace juddperft {

	ZobristKeySet::ZobristKeySet()
	{
		ZobristKeySet::generate();
	}

	uint64_t ZobristKeySet::generate(std::optional<uint64_t> seed)
	{
		if (!seed.has_value()) {
			// std::random_device rd;
			// seed = rd();

			seed = 0x4a1b5d94; // for consistency during dev. "best" seed yet to be found
		}

		// Create a Random Number Generator, using the 64-bit Mersenne Twister Algorithm
		// with a uniform distribution of ints;
		// avoid zero
		// avoid modulo-2 bias by making an even number of possibilities :-)

		std::mt19937_64 rng(seed.value());
		std::uniform_int_distribution<unsigned long long int> dist(2, 0xffffffffffffffff);
		for (int n = 0; n < 16; ++n) {
			for (int m = 0; m < 64; ++m) {
				ZobristKeySet::zkPieceOnSquare[n][m] = dist(rng);
			}
		}

		ZobristKeySet::zkBlackToMove = dist(rng);
		ZobristKeySet::zkWhiteCanCastle = dist(rng);
		ZobristKeySet::zkWhiteCanCastleLong = dist(rng);
		ZobristKeySet::zkBlackCanCastle = dist(rng);
		ZobristKeySet::zkBlackCanCastleLong = dist(rng);

		for (int m = 0; m < 24; ++m) {
			ZobristKeySet::zkPerftDepth[m] = dist(rng);
		}
		// ends random key generation


		// generate pre-fabricated castling keys:
		ZobristKeySet::zkDoBlackCastle =
			ZobristKeySet::zkPieceOnSquare[BKING][e8] ^ // Remove King from e8
			ZobristKeySet::zkPieceOnSquare[BKING][g8] ^ // Place King on g8
			ZobristKeySet::zkPieceOnSquare[BROOK][h8] ^ // Remove Rook from h8
			ZobristKeySet::zkPieceOnSquare[BROOK][f8] ^ // Place Rook on f8
			ZobristKeySet::zkBlackCanCastle;			// (unconditionally) flip black castling

		ZobristKeySet::zkDoBlackCastleLong =
			ZobristKeySet::zkPieceOnSquare[BKING][e8] ^ // Remove King from e8
			ZobristKeySet::zkPieceOnSquare[BKING][c8] ^ // Place King on c8
			ZobristKeySet::zkPieceOnSquare[BROOK][a8] ^ // Remove Rook from a8
			ZobristKeySet::zkPieceOnSquare[BROOK][d8] ^ // Place Rook on d8
			ZobristKeySet::zkBlackCanCastleLong;		// (unconditionally) flip black castling long

		ZobristKeySet::zkDoWhiteCastle =
			ZobristKeySet::zkPieceOnSquare[WKING][e1] ^ // Remove King from e1
			ZobristKeySet::zkPieceOnSquare[WKING][g1] ^ // Place King on g1
			ZobristKeySet::zkPieceOnSquare[WROOK][h1] ^ // Remove Rook from h1
			ZobristKeySet::zkPieceOnSquare[WROOK][f1] ^ // Place Rook on f1
			ZobristKeySet::zkWhiteCanCastle;			// (unconditionally) flip white castling

		ZobristKeySet::zkDoWhiteCastleLong =
			ZobristKeySet::zkPieceOnSquare[WKING][e1] ^ // Remove King from e1
			ZobristKeySet::zkPieceOnSquare[WKING][c1] ^ // Place King on c1
			ZobristKeySet::zkPieceOnSquare[WROOK][a1] ^ // Remove Rook from a1
			ZobristKeySet::zkPieceOnSquare[WROOK][d1] ^ // Place Rook on d1
			ZobristKeySet::zkWhiteCanCastleLong;		// (unconditionally) flip white castling long

		return seed.value();
	}

	void ZobristKeySet::findBestSeed(const std::optional<unsigned int>& prev_best_seed, const std::optional<int>& maxAttempts)
	{
		static std::map<unsigned int, double> results;

		perftTable.setQuiet(true);
		{
			std::cout << "warming up the CPU with a perft(8)" << std::endl;
			perftTable.setSize(perftTable.getSize());

			ChessPosition P;
			P.setupStartPosition();

			RaiiTimer timer;
			nodecount_t nNumPositions = 0;
			const int depth = 8;
			perftFastMT(P, depth, nNumPositions);
			printf("\nPerft %d: %" PRIu64 " \n",
				   depth, nNumPositions
				   );
			printf("\n");
		}

		int attempt = 0;

		while (attempt < maxAttempts.value_or(10000000)) {
			const unsigned int seed = (attempt ? zobristKeys.generate() : zobristKeys.generate(prev_best_seed));
			std::stringstream seed_s;
			seed_s << "seed=0x" << std::hex << seed;
			std::cout << seed_s.str() << std::endl;

			static constexpr int avgOf = 10;
			const int depth = 7;
			const nodecount_t expected = 3195901860ull;
			nodecount_t nNumPositions = 0;
			double tt = 0.0;

			for (int s = 0; s < avgOf; s++) {
				perftTable.setSize(perftTable.getSize());
				ChessPosition P;
				P.setupStartPosition();
				{
					RaiiTimer timer;
					nNumPositions = 0;
					perftFastMT(P, depth, nNumPositions);
					printf("\nPerft %d: %" PRIu64 " \n",
						   depth, nNumPositions
						   );
					printf("\n");

					timer.setNodes(nNumPositions);
					tt += timer.elapsed();
				}
			}

			if (nNumPositions == expected) {
				results.insert({seed, tt / avgOf});
			}

			auto itBest = std::min_element(std::begin(results), std::end(results), [](const auto& l, const auto& r) {
				return l.second < r.second;
			});

			std::cout << "best so far: seed=0x" << std::hex << itBest->first << " ms=" << std::dec << std::setw(1) << itBest->second << std::endl;

			auto itWorst = std::max_element(std::begin(results), std::end(results), [](const auto& l, const auto& r) {
				return l.second < r.second;
			});

			std::cout << "worst so far: seed=0x" << std::hex << itWorst->first << " ms=" << std::dec << std::setw(1) << itWorst->second << std::endl;

			attempt++;
		} // ends while (...)
	}

	ZobristKeySet zobristKeys;

#ifdef _USE_HASH
	HashTable <std::atomic<PerftTableEntry>> perftTable("Perft table");
#endif

} // namespace juddperft
