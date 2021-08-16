#include "Skybox.h"
#include "AppState.h"
#include "Constants.h"

void Skybox::Delete(AppState& appState) const
{
	if (swapchain != XR_NULL_HANDLE)
	{
		xrDestroySwapchain(swapchain);
	}
}

void Skybox::DeleteOld(AppState& appState)
{
	for (Skybox** s = &appState.Scene.skybox; *s != nullptr; )
	{
		(*s)->unusedCount++;
		if ((*s)->unusedCount >= Constants::maxUnusedCount)
		{
			auto next = (*s)->next;
			(*s)->Delete(appState);
			delete *s;
			*s = next;
		}
		else
		{
			s = &(*s)->next;
		}
	}
	if (appState.Scene.skybox != nullptr)
	{
		appState.Scene.skybox->unusedCount = 0;
	}
}

void Skybox::DeleteAll(AppState& appState)
{
	for (Skybox** s = &appState.Scene.skybox; *s != nullptr; )
	{
		auto next = (*s)->next;
		(*s)->Delete(appState);
		delete *s;
		*s = next;
	}
}
