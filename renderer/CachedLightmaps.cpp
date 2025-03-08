#include "CachedLightmaps.h"
#include "Constants.h"

void CachedLightmaps::Setup(LoadedLightmap& lightmap)
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

void CachedLightmaps::DisposeFront()
{
	for (auto& entry : lightmaps)
	{
		oldLightmaps.push_back(entry.second);
	}
	lightmaps.clear();
}

void CachedLightmaps::Delete(AppState& appState)
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

void CachedLightmaps::DeleteOld(AppState& appState)
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
