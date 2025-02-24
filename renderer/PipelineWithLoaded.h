#pragma once

#include "Pipeline.h"

template <typename Loaded>
struct PipelineWithLoaded : Pipeline
{
	int last;
	std::vector<Loaded> loaded;

	void Allocate(int last)
	{
		this->last = last;
		if (last >= loaded.size())
		{
			loaded.resize(last + 1);
		}
	}
};
