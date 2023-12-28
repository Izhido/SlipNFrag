#ifdef PIPELINEWITHSORTED_CPP

template <typename Loaded, typename Sorted>
void PipelineWithSorted<Loaded, Sorted>::Allocate(int last)
{
	this->last = last;
	if (last >= loaded.size())
	{
		loaded.resize(last + 1);
	}
}

template <typename Loaded, typename Sorted>
void PipelineWithSorted<Loaded, Sorted>::SetBases(VkDeviceSize vertexBase, VkDeviceSize attributeBase, VkDeviceSize indexBase)
{
	this->vertexBase = vertexBase;
	this->attributeBase = attributeBase;
	this->indexBase = indexBase;
}

template <typename Loaded, typename Sorted>
void PipelineWithSorted<Loaded, Sorted>::ScaleIndexBase(int scale)
{
	indexBase *= scale;
}

#endif
