#pragma once

#include <android_native_app_glue.h>
#include "AppMode.h"
#include <EGL/egl.h>
#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "Scene.h"
#include "PerImage.h"
#include <mutex>
#include <thread>
#include "Controller.h"

struct AppState
{
	ANativeWindow* NativeWindow;
	AppMode Mode;
	AppMode PreviousMode;
	bool StartupButtonsPressed;
	bool Resumed;
	double PausedTime;
	VkDevice Device;
	XrSession Session;
	VkCommandPool CommandPool;
	VkQueue Queue;
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	int DefaultFOV;
	int FOV;
	uint32_t SwapchainWidth;
	uint32_t SwapchainHeight;
	uint32_t SwapchainSampleCount;
	XrSwapchain ScreenSwapchain;
	std::vector<XrSwapchainImageVulkan2KHR> ScreenVulkanImages;
	std::vector<XrSwapchainImageBaseHeader*> ScreenImages;
	std::vector<uint32_t> ScreenData;
	Buffer ScreenStagingBuffer;
	std::vector<VkCommandBuffer> ScreenCommandBuffers;
	std::vector<VkSubmitInfo> ScreenSubmitInfo;
	Scene Scene;
	std::vector<PerImage> PerImage;
	int EyeTextureWidth;
	int EyeTextureHeight;
	int EyeTextureMaxDimension;
	VkDeviceSize NoOffset;
	VkPipelineCache PipelineCache;
	VkRenderPass RenderPass;
	float Yaw;
	float Pitch;
	float Roll;
	float PositionX;
	float PositionY;
	float PositionZ;
	float DistanceToFloor;
	float Scale;
	float OriginX;
	float OriginY;
	float OriginZ;
	int ScreenWidth;
	int ScreenHeight;
	int ConsoleWidth;
	int ConsoleHeight;
	std::vector<uint32_t> NoGameDataData;
	double PreviousTime;
	double CurrentTime;
	Controller LeftController;
	Controller RightController;
	XrVector2f PreviousThumbstick;
	bool NearViewModel;
	double TimeInWorldMode;
	bool ControlsMessageDisplayed;
	bool ControlsMessageClosed;
	std::mutex ModeChangeMutex;
	std::mutex InputMutex;
	std::mutex RenderInputMutex;
	std::mutex RenderMutex;
	std::thread EngineThread;
	bool EngineThreadStopped;
	XrActionSet ActionSet;
	XrAction Play1Action;
	XrAction Play2Action;
	XrAction JumpAction;
	XrAction SwimDownAction;
	XrAction RunAction;
	XrAction FireAction;
	XrAction MoveXAction;
	XrAction MoveYAction;
	XrAction SwitchWeaponAction;
	XrAction MenuAction;
	XrAction EnterTriggerAction;
	XrAction EnterNonTriggerAction;
	XrAction EscapeYAction;
	XrAction EscapeNonYAction;
	XrAction QuitAction;
	XrAction PoseAction;
	std::vector<XrPath> SubactionPaths;
	std::vector<XrSpace> HandSpaces;
	std::vector<float> HandScales;
	std::vector<XrBool32> ActiveHands;
};
