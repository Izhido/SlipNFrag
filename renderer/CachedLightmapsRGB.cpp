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
	for (LightmapRGB* l = oldLightmaps, *next; l != nullptr; l = next)
	{
		next = l->next;
		l->Delete(appState);
		delete l;
	}
	oldLightmaps = nullptr;
}

void CachedLightmapsRGB::DeleteOld(AppState& appState)
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
