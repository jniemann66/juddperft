#include "zobristkeyset.h"

#include "hash_table.h"
#include "tablegroup.h"
#include "chessposition.h"
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
	generate();
}

uint64_t ZobristKeySet::generate(std::optional<uint64_t> seed)
{
	if (!seed.has_value()) {
		std::random_device rd;
		seed = rd();

		// seed = 0x4a1b5d94; // for consistency during dev. "best" seed yet to be found
	}

	// Create a Random Number Generator, using the 64-bit Mersenne Twister Algorithm
	// with a uniform distribution of ints;
	// avoid zero
	// avoid modulo-2 bias by making an even number of possibilities :-)

	std::mt19937_64 rng(seed.value());
	std::uniform_int_distribution<unsigned long long int> dist(2, 0xffffffffffffffff);
	for (int n = 0; n < 16; ++n) {
		for (int m = 0; m < 64; ++m) {
			zkPieceOnSquare[n][m] = dist(rng);
		}
	}

	zkBlackToMove = dist(rng);
	zkWhiteCanCastle = dist(rng);
	zkWhiteCanCastleLong = dist(rng);
	zkBlackCanCastle = dist(rng);
	zkBlackCanCastleLong = dist(rng);

	for (int m = 0; m < 24; ++m) {
		zkPerftDepth[m] = dist(rng);
	}
	// ends random key generation


	// generate pre-fabricated castling keys:
	zkDoBlackCastle =
		zkPieceOnSquare[BKING][e8] ^ // Remove King from e8
		zkPieceOnSquare[BKING][g8] ^ // Place King on g8
		zkPieceOnSquare[BROOK][h8] ^ // Remove Rook from h8
		zkPieceOnSquare[BROOK][f8] ^ // Place Rook on f8
		zkBlackCanCastle;			// (unconditionally) flip black castling

	zkDoBlackCastleLong =
		zkPieceOnSquare[BKING][e8] ^ // Remove King from e8
		zkPieceOnSquare[BKING][c8] ^ // Place King on c8
		zkPieceOnSquare[BROOK][a8] ^ // Remove Rook from a8
		zkPieceOnSquare[BROOK][d8] ^ // Place Rook on d8
		zkBlackCanCastleLong;		// (unconditionally) flip black castling long

	zkDoWhiteCastle =
		zkPieceOnSquare[WKING][e1] ^ // Remove King from e1
		zkPieceOnSquare[WKING][g1] ^ // Place King on g1
		zkPieceOnSquare[WROOK][h1] ^ // Remove Rook from h1
		zkPieceOnSquare[WROOK][f1] ^ // Place Rook on f1
		zkWhiteCanCastle;			// (unconditionally) flip white castling

	zkDoWhiteCastleLong =
		zkPieceOnSquare[WKING][e1] ^ // Remove King from e1
		zkPieceOnSquare[WKING][c1] ^ // Place King on c1
		zkPieceOnSquare[WROOK][a1] ^ // Remove Rook from a1
		zkPieceOnSquare[WROOK][d1] ^ // Place Rook on d1
		zkWhiteCanCastleLong;		// (unconditionally) flip white castling long

	return seed.value();
}

void ZobristKeySet::findBestSeed(const std::optional<unsigned int>& prev_best_seed, const std::optional<int>& maxAttempts)
{
	static std::map<unsigned int, double> results;

	TableGroup::perftTable.setQuiet(true);
	{
		std::cout << "warming up the CPU with a perft(8)" << std::endl;
		TableGroup::perftTable.clear();

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
			TableGroup::perftTable.clear();
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

} // namespace juddperft
