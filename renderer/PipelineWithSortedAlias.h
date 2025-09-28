#pragma once

#include "Pipeline.h"

template <typename Loaded, typename Sorted>
struct PipelineWithSortedAlias : Pipeline
{
	int last;
	std::vector<Loaded> loaded;
	Sorted sorted;

	void Allocate(int last)
	{
		this->last = last;
		if (last >= loaded.size())
		{
			loaded.resize(last + 1);
		}
	}
};
