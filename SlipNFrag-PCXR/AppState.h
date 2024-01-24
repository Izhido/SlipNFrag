#pragma once

#include "vid_pcxr.h"
#include "d_lists.h"
#include "AppMode.h"
#include <vulkan/vulkan.h>
#include <common/xr_dependencies.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "Keyboard.h"
#include "Scene.h"
#include "FileLoader.h"
#include "FromEngine.h"
#include "PerFrame.h"
#include <thread>
#include "Controller.h"

struct AppState
{
	double PausedTime;
	double PreviousTime;
	double CurrentTime;
	AppMode Mode;
	AppMode PreviousMode;
	bool StartupButtonsPressed;
	bool Resumed;
	VkDevice Device;
	bool IndexTypeUInt8Enabled;
	XrSession Session;
	VkCommandPool CommandPool;
	std::vector<VkCommandBuffer> CommandBuffers;
	VkQueue Queue;
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	int DefaultFOV;
	int FOV;
	uint32_t SwapchainWidth;
	uint32_t SwapchainHeight;
	uint32_t SwapchainSampleCount;
	std::vector<XrSwapchainImageVulkan2KHR> SwapchainImages;
	Screen Screen;
	std::vector<uint32_t> ScreenData;
	std::vector<Texture> ConsoleTextures;
	std::vector<Texture> StatusBarTextures;
	Keyboard Keyboard;
	std::unordered_map<uint32_t, Texture> KeyboardTextures;
	float KeyboardHitOffsetY;
	XrSwapchain LeftArrowsSwapchain;
	XrSwapchain RightArrowsSwapchain;
    FileLoader* FileLoader;
	Scene Scene;
	FromEngine FromEngine;
	std::unordered_map<uint32_t, PerFrame> PerFrame;
	uint32_t EyeTextureWidth;
	uint32_t EyeTextureHeight;
	uint32_t EyeTextureMaxDimension;
	VkDeviceSize NoOffset;
	VkPipelineCache PipelineCache;
	VkRenderPass RenderPass;
	float Yaw;
	float Pitch;
	float Roll;
	float DistanceToFloor;
	float Scale;
	int ScreenWidth;
	int ScreenHeight;
	int ConsoleWidth;
	int ConsoleHeight;
	std::vector<uint32_t> NoGameDataData;
	Controller LeftController;
	Controller RightController;
	XrVector2f PreviousThumbstick;
	double TimeInWorldMode;
	bool ControlsMessageDisplayed;
	std::thread EngineThread;
	bool EngineThreadCreated;
	bool EngineThreadStarted;
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
	XrAction LeftKeyPressAction;
	XrAction RightKeyPressAction;
	std::vector<XrPath> SubactionPaths;
	std::vector<XrSpace> HandSpaces;
	std::vector<float> HandScales;
	std::vector<XrBool32> ActiveHands;
	std::vector<XrMatrix4x4f> ViewMatrices;
	std::vector<XrMatrix4x4f> ProjectionMatrices;
	XrSpaceLocation CameraLocation;
	bool CameraLocationIsValid;
	bool Focused;
	bool DestroyRequested;
	VkImageMemoryBarrier copyBarrier;
	VkImageMemoryBarrier submitBarrier;
	bool NoGameDataLoaded;

	void RenderScreen(ScreenPerFrame& perFrame);
	void RenderKeyboard(ScreenPerFrame& perFrame);
};
