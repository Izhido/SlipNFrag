#include "CachedLightmaps.h"
#include "Constants.h"

void CachedLightmaps::Setup(AppState& appState, LoadedLightmap& lightmap)
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
	for (auto entry = lightmaps.begin(); entry != lightmaps.end(); entry++)
	{
		for (auto l = &entry->second; *l != nullptr; )
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
	std::vector<TwinKey> toDelete;
	for (auto& entry : lightmaps)
	{
		auto total = 0;
		auto erased = 0;
		for (auto l = &entry.second; *l != nullptr; )
		{
			(*l)->unusedCount++;
			if ((*l)->unusedCount >= MAX_UNUSED_COUNT)
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
			if ((*l)->unusedCount >= MAX_UNUSED_COUNT)
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
