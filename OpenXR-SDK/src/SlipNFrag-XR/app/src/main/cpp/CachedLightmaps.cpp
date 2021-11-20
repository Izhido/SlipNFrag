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

void CachedLightmaps::DeleteOld(AppState& appState)
{
	std::vector<void*> toDelete;
	for (auto& entry : lightmaps)
	{
		auto total = 0;
		auto erased = 0;
		for (auto l = &entry.second; *l != nullptr; )
		{
			(*l)->unusedCount++;
			if ((*l)->unusedCount >= Constants::maxUnusedCount)
			{
				auto next = (*l)->next;
				(*l)->next = oldLightmaps;
				oldLightmaps = *l;
				*l = next;
				erased++;
			}
			else
			{
				l = &(*l)->next;
			}
			total++;
		}
		if (total == erased)
		{
			toDelete.push_back(entry.first);
		}
	}
	for (auto& entry : toDelete)
	{
		lightmaps.erase(entry);
	}
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
