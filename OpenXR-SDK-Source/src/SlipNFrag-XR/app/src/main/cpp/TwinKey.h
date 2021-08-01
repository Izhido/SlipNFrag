#pragma once

#include <functional>

struct TwinKey
{
	void* first = nullptr;
	void* second = nullptr;
};

bool operator==(const TwinKey& first, const TwinKey& second);

namespace std
{
	template<> struct hash<TwinKey>
	{
		size_t operator()(const TwinKey& key) const;
	};
}
