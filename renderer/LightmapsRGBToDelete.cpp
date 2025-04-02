#include "LightmapsRGBToDelete.h"
#include "Constants.h"

void LightmapsRGBToDelete::Dispose(LightmapRGB *lightmap)
{
	lightmap->next = oldLightmaps;
	oldLightmaps = lightmap;
}

void LightmapsRGBToDelete::DeleteOld(AppState& appState)
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

void LightmapsRGBToDelete::Delete(AppState& appState)
{
	for (LightmapRGB* l = oldLightmaps, *next; l != nullptr; l = next)
	{
		next = l->next;
		l->Delete(appState);
		delete l;
	}
	oldLightmaps = nullptr;
}
