#ifdef PIPELINEWITHLOADED_CPP

template <typename Loaded>
void PipelineWithLoaded<Loaded>::Allocate(int last)
{
	this->last = last;
	if (last >= loaded.size())
	{
		loaded.resize(last + 1);
	}
}

#endif
