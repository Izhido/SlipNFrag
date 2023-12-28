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

void CachedLightmaps::Delete(AppState& appState)
{
	for (auto& entry : lightmaps)
	{
		for (Lightmap* l = entry.second, *next; l != nullptr; l = next)
		{
			next = l->next;
			l->Delete(appState);
			delete l;
		}
	}
	lightmaps.clear();
	for (Lightmap* l = oldLightmaps, *next; l != nullptr; l = next)
	{
		next = l->next;
		l->Delete(appState);
		delete l;
	}
	oldLightmaps = nullptr;
}

void CachedLightmaps::DeleteOld(AppState& appState)
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
