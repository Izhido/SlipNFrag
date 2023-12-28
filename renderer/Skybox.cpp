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

void Skybox::MoveToPrevious(Scene& scene)
{
	if (scene.skybox != nullptr)
	{
		scene.skybox->next = scene.previousSkyboxes;
		scene.previousSkyboxes = scene.skybox;
		scene.skybox = nullptr;
	}
}

void Skybox::DeleteOld(AppState& appState)
{
	for (Skybox** s = &appState.Scene.previousSkyboxes; *s != nullptr; )
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
	MoveToPrevious(appState.Scene);
	for (Skybox** s = &appState.Scene.previousSkyboxes; *s != nullptr; )
	{
		auto next = (*s)->next;
		(*s)->Delete(appState);
		delete *s;
		*s = next;
	}
}
