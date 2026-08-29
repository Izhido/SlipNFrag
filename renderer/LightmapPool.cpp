#include "LightmapPool.h"
#include "Constants.h"

Lightmap* LightmapPool::Acquire()
{
	if (reusable != nullptr)
	{
		auto reused = reusable;
		reusable = reusable->next;
		memset(reused, 0, sizeof(Lightmap));
		return reused;
	}
	return new Lightmap { };
}

void LightmapPool::Dispose(Lightmap *lightmap)
{
	lightmap->next = toDispose;
	toDispose = lightmap;
}

void LightmapPool::DeleteOld(AppState& appState)
{
	if (toDispose != nullptr)
	{
		for (auto l = &toDispose; *l != nullptr; )
		{
			(*l)->unusedCount++;
			if ((*l)->unusedCount >= Constants::framesToLive)
			{
				auto next = (*l)->next;
				(*l)->Delete(appState);
				(*l)->next = reusable;
				reusable = *l;
				*l = next;
			}
			else
			{
				l = &(*l)->next;
			}
		}
	}
}

void LightmapPool::Delete(AppState& appState)
{
	for (Lightmap* l = reusable, *next; l != nullptr; l = next)
	{
		next = l->next;
		delete l;
	}
	reusable = nullptr;
	for (Lightmap* l = toDispose, *next; l != nullptr; l = next)
	{
		next = l->next;
		l->Delete(appState);
		delete l;
	}
	toDispose = nullptr;
}
