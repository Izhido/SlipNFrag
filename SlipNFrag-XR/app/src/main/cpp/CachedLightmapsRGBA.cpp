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
		for (auto l = &entry.second; *l != nullptr; )
		{
			auto next = (*l)->next;
			(*l)->next = oldLightmaps;
			oldLightmaps = *l;
			*l = next;
		}
	}
	lightmaps.clear();
}

void CachedLightmapsRGBA::Delete(AppState& appState)
{
	for (auto& entry : lightmaps)
	{
		for (LightmapRGBA* l = entry.second, *next; l != nullptr; l = next)
		{
			next = l->next;
			l->Delete(appState);
			delete l;
		}
	}
	lightmaps.clear();
	for (LightmapRGBA* l = oldLightmaps, *next; l != nullptr; l = next)
	{
		next = l->next;
		l->Delete(appState);
		delete l;
	}
	oldLightmaps = nullptr;
}

void CachedLightmapsRGBA::DeleteOld(AppState& appState)
{
	if (oldLightmaps != nullptr)
	{
		for (auto l = &oldLightmaps; *l != nullptr; )
		{
			(*l)->unusedCount++;
			if ((*l)->unusedCount >= Constants::maxUnusedCount)
			{
				auto next = (*l)->next;
				(*l)->Delete(appState);
				delete *l;
				*l = next;
			}
			else
			{
				l = &(*l)->next;
			}
		}
	}
}
