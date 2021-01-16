#include "TwinKey.h"

bool operator==(const TwinKey& first, const TwinKey& second)
{
	return (first.first == second.first && first.second == second.second);
}

size_t std::hash<TwinKey>::operator()(const TwinKey& key) const
{
	uint64_t h = hash<void*>()(key.first);
	uint64_t k = hash<void*>()(key.second);
	uint64_t m = 14313749767032793493;
	const int r = 47;
	k *= m;
	k ^= k >> r;
	k *= m;
	h ^= k;
	h *= m;
	return h + 11400714819323198485;
}
