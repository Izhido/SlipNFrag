#include "Scene.h"

void Scene::Reset()
{
	viewmodelsPerKey.clear();
	aliasPerKey.clear();
	latestTextureSharedMemory = nullptr;
	usedInLatestTextureSharedMemory = 0;
	latestColormappedBuffer = nullptr;
	usedInLatestColormappedBuffer = 0;
	colormappedBufferList.clear();
	colormappedTexCoordsPerKey.clear();
	colormappedVerticesPerKey.clear();
	spritesPerKey.clear();
	viewmodelTextures.DisposeFront();
	aliasTextures.DisposeFront();
	spriteTextures.DisposeFront();
	for (auto entry = surfaces.begin(); entry != surfaces.end(); entry++)
	{
		for (TextureFromAllocation** t = &entry->second; *t != nullptr; )
		{
			TextureFromAllocation* next = (*t)->next;
			(*t)->next = oldSurfaces;
			oldSurfaces = *t;
			*t = next;
		}
	}
	surfaces.clear();
	colormappedBuffers.DisposeFront();
	viewmodelTextureCount = 0;
	aliasTextureCount = 0;
	spriteTextureCount = 0;
	resetDescriptorSetsCount++;
	vrapi_DestroyTextureSwapChain(skybox);
	skybox = VK_NULL_HANDLE;
}