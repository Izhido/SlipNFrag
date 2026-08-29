#include "LightmapRGBPool.h"
#include "Constants.h"

LightmapRGB* LightmapRGBPool::Acquire()
{
	if (reusable != nullptr)
	{
		auto reused = reusable;
		reusable = reusable->next;
		memset(reused, 0, sizeof(LightmapRGB));
		return reused;
	}
	return new LightmapRGB { };
}

void LightmapRGBPool::Dispose(LightmapRGB *lightmap)
{
	lightmap->next = toDispose;
	toDispose = lightmap;
}

void LightmapRGBPool::DeleteOld(AppState& appState)
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

void LightmapRGBPool::Delete(AppState& appState)
{
	for (LightmapRGB* l = reusable, *next; l != nullptr; l = next)
	{
		next = l->next;
		delete l;
	}
	reusable = nullptr;
	for (LightmapRGB* l = toDispose, *next; l != nullptr; l = next)
	{
		next = l->next;
		l->Delete(appState);
		delete l;
	}
	toDispose = nullptr;
}
