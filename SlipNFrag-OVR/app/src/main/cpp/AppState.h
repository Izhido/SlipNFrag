#pragma once

#include <android_native_app_glue.h>
#include "PermissionsGrantStatus.h"
#include "AppMode.h"
#include "Device.h"
#include "Context.h"
#include "Scene.h"
#include "View.h"
#include "Panel.h"
#include "Screen.h"
#include "Console.h"

struct AppState
{
	ovrJava Java;
	ANativeWindow *NativeWindow;
	AppMode Mode;
	AppMode PreviousMode;
	bool StartupButtonsPressed;
	bool Resumed;
	double PausedTime;
	Device Device;
	Context Context;
	ovrMobile *Ovr;
	int DefaultFOV;
	int FOV;
	Scene Scene;
	long long FrameIndex;
	double DisplayTime;
	int SwapInterval;
	int CpuLevel;
	int GpuLevel;
	int EyeTextureWidth;
	int EyeTextureHeight;
	int EyeTextureMaxDimension;
	VkDeviceSize NoOffset;
	int MainThreadTid;
	int RenderThreadTid;
	bool UseFragmentDensity;
	VkRenderPass RenderPass;
	std::vector<View> Views;
	ovrMatrix4f ViewMatrices[VRAPI_FRAME_LAYER_EYE_MAX];
	ovrMatrix4f ProjectionMatrices[VRAPI_FRAME_LAYER_EYE_MAX];
	float Yaw;
	float Pitch;
	float Roll;
	float Scale;
	int ScreenWidth;
	int ScreenHeight;
	int ConsoleWidth;
	int ConsoleHeight;
	Panel LeftArrows;
	Panel RightArrows;
	Screen Screen;
	Console Console;
	std::vector<float> ConsoleVertices;
	std::vector<uint16_t> ConsoleIndices;
	int FloorWidth;
	int FloorHeight;
	std::vector<uint32_t> FloorData;
	std::vector<uint32_t> NoGameDataData;
	double PreviousTime;
	double CurrentTime;
	uint32_t PreviousLeftButtons;
	uint32_t PreviousRightButtons;
	uint32_t LeftButtons;
	uint32_t RightButtons;
	ovrVector2f PreviousLeftJoystick;
	ovrVector2f PreviousRightJoystick;
	ovrVector2f LeftJoystick;
	ovrVector2f RightJoystick;
	bool NearViewModel;
	double TimeInWorldMode;
	bool ControlsMessageDisplayed;
	bool ControlsMessageClosed;

	void RenderScene(VkCommandBufferBeginInfo& commandBufferBeginInfo);
};
