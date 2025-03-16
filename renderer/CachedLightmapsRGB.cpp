#include "CachedLightmapsRGB.h"
#include "Constants.h"

void CachedLightmapsRGB::Setup(LoadedLightmapRGB& lightmap)
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

void CachedLightmapsRGB::Delete(AppState& appState)
{
	for (auto l : oldLightmaps)
	{
		l->Delete(appState);
	}
	oldLightmaps.clear();
}

void CachedLightmapsRGB::DeleteOld(AppState& appState)
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
