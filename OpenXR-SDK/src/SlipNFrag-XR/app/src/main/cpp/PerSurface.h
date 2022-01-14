#pragma once

#include "SharedMemoryTexturePositionsBuffer.h"
#include "SharedMemoryIndexBuffer.h"

struct PerSurface
{
	SharedMemoryTexturePositionsBuffer texturePosition;
	SharedMemoryIndexBuffer indices;
};
