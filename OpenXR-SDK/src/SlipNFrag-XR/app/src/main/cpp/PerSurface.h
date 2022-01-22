#pragma once

#include "SharedMemoryTexturePositionsBuffer.h"
#include "IndexBuffer.h"

struct PerSurface
{
	SharedMemoryTexturePositionsBuffer texturePosition;
	IndexBuffer indices;
};
