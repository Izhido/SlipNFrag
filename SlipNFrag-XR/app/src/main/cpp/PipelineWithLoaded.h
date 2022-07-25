#pragma once

#include "Pipeline.h"

template <typename Loaded>
struct PipelineWithLoaded : Pipeline
{
	int last;
	std::vector<Loaded> loaded;

	void Allocate(int last);
};

#define PIPELINEWITHLOADED_CPP
#include "PipelineWithLoaded.cpp"
#undef PIPELINEWITHLOADED_CPP
