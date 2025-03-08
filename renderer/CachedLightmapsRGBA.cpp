#include "CachedLightmapsRGBA.h"
#include "Constants.h"

void CachedLightmapsRGBA::Setup(LoadedLightmapRGBA& lightmap)
{
	lightmap.next = nullptr;
	if (current == nullptr)
	{
		first = &lightmap;
	}
	else
	{
		current->next = &lightmap;
	}
	current = &lightmap;
}

void CachedLightmapsRGBA::DisposeFront()
{
	for (auto& entry : lightmaps)
	{
		oldLightmaps.push_back(entry.second);
	}
	lightmaps.clear();
}

void CachedLightmapsRGBA::Delete(AppState& appState)
{
	for (auto l : oldLightmaps)
	{
		l->Delete(appState);
	}
	oldLightmaps.clear();
	for (auto& entry : lightmaps)
	{
		entry.second->Delete(appState);
	}
	lightmaps.clear();
}

void CachedLightmapsRGBA::DeleteOld(AppState& appState)
{
	for (auto l = oldLightmaps.begin(); l != oldLightmaps.end(); )
	{
		(*l)->unusedCount++;
		if ((*l)->unusedCount >= Constants::maxUnusedCount)
		{
			auto lightmap = *l;
			lightmap->Delete(appState);
			delete lightmap;
			l = oldLightmaps.erase(l);
		}
		else
		{
			l++;
		}
	}
}
