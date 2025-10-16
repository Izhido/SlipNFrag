#include "Floor.h"
#include "AppState.h"

VkDeviceSize Floor::VerticesSize()
{
	return 3 * 4 * sizeof(float);
}

VkDeviceSize Floor::AttributesSize()
{
	return 2 * 4 * sizeof(float);
}

VkDeviceSize Floor::IndicesSize()
{
	return 6;
}

void Floor::WriteVertices(AppState& appState, float* vertices)
{
	*vertices++ = -0.5;
	*vertices++ = appState.DistanceToFloor;
	*vertices++ = -0.5;
	*vertices++ = 0.5;
	*vertices++ = appState.DistanceToFloor;
	*vertices++ = -0.5;
	*vertices++ = 0.5;
	*vertices++ = appState.DistanceToFloor;
	*vertices++ = 0.5;
	*vertices++ = -0.5;
	*vertices++ = appState.DistanceToFloor;
	*vertices++ = 0.5;
}

void Floor::WriteAttributes(float* attributes)
{
	*attributes++ = 0;
	*attributes++ = 0;
	*attributes++ = 1;
	*attributes++ = 0;
	*attributes++ = 1;
	*attributes++ = 1;
	*attributes++ = 0;
	*attributes++ = 1;
}

void Floor::WriteIndices8(unsigned char* indices)
{
	*indices++ = 0;
	*indices++ = 1;
	*indices++ = 2;
	*indices++ = 2;
	*indices++ = 3;
	*indices++ = 0;
}

void Floor::WriteIndices16(uint16_t* indices)
{
	*indices++ = 0;
	*indices++ = 1;
	*indices++ = 2;
	*indices++ = 2;
	*indices++ = 3;
	*indices++ = 0;
}
