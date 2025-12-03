#ifndef ZOBRISTKEYSET_H
#define ZOBRISTKEYSET_H

#include <cstdint>
#include <optional>

namespace juddperft {

typedef uint64_t ZobristKey;

// class ZobristKeySet: a set of random 64-bit keys for generating a hash key from a given position

class ZobristKeySet
{
public:
	ZobristKeySet();

	// todo: if these are all static, will they be faster without a this pointer ??
	ZobristKey zkPieceOnSquare[16][64];
	ZobristKey zkBlackToMove;
	ZobristKey zkWhiteCanCastle;
	ZobristKey zkWhiteCanCastleLong;
	ZobristKey zkBlackCanCastle;
	ZobristKey zkBlackCanCastleLong;
	ZobristKey zkPerftDepth[24];

	// pre-fabricated combinations of keys for castling:
	ZobristKey zkDoBlackCastle;
	ZobristKey zkDoBlackCastleLong;
	ZobristKey zkDoWhiteCastle;
	ZobristKey zkDoWhiteCastleLong;

	// utility:
	static void findBestSeed(const std::optional<unsigned int>& prev_best_seed = {}, const std::optional<int>& maxAttempts = {});

private:
	uint64_t generate(std::optional<uint64_t> seed = {});

};

// Global instances:
extern ZobristKeySet zobristKeys;

} // namespace juddperft

#endif // ZOBRISTKEYSET_H
