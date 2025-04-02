#include "LightmapsToDelete.h"
#include "Constants.h"

void LightmapsToDelete::Dispose(Lightmap *lightmap)
{
	lightmap->next = oldLightmaps;
	oldLightmaps = lightmap;
}

void LightmapsToDelete::DeleteOld(AppState& appState)
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

void LightmapsToDelete::Delete(AppState& appState)
{
	for (Lightmap* l = oldLightmaps, *next; l != nullptr; l = next)
	{
		next = l->next;
		l->Delete(appState);
		delete l;
	}
	oldLightmaps = nullptr;
}
