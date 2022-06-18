#pragma once

#include "vid_oxr.h"
#include "d_lists.h"
#include "AppMode.h"
#include <vulkan/vulkan.h>
#include <common/xr_dependencies.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "Keyboard.h"
#include "Scene.h"
#include "FromEngine.h"
#include "PerFrame.h"
#include <thread>
#include "Controller.h"

struct AppState
{
	AppMode Mode;
	AppMode PreviousMode;
	bool StartupButtonsPressed;
	bool Resumed;
	double PausedTime;
	VkDevice Device;
	bool IndexTypeUInt8Enabled;
	XrSession Session;
	VkCommandPool CommandPool;
	VkQueue Queue;
	VkPhysicalDeviceMemoryProperties MemoryProperties;
	int DefaultFOV;
	int FOV;
	uint32_t SwapchainWidth;
	uint32_t SwapchainHeight;
	uint32_t SwapchainSampleCount;
	Screen Screen;
	std::vector<uint32_t> ScreenData;
	std::vector<Texture> ConsoleTextures;
	std::vector<Texture> StatusBarTextures;
	Keyboard Keyboard;
	std::vector<Texture> KeyboardTextures;
	float KeyboardHitOffsetY;
	XrSwapchain LeftArrowsSwapchain;
	XrSwapchain RightArrowsSwapchain;
	Scene Scene;
	FromEngine FromEngine;
	std::unordered_map<std::string, PerFrame> PerFrame;
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
	float ViewmodelForwardX;
	float ViewmodelForwardY;
	float ViewmodelForwardZ;
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
	bool NearViewmodel;
	double TimeInWorldMode;
	bool ControlsMessageDisplayed;
	bool ControlsMessageClosed;
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
	int CpuLevel;
	int GpuLevel;
	pid_t EngineThreadId;
	pid_t RenderThreadId;
	PFN_xrSetAndroidApplicationThreadKHR xrSetAndroidApplicationThreadKHR;
	VkImageMemoryBarrier copyBarrier;
	VkImageMemoryBarrier submitBarrier;

	void RenderScreen(uint32_t swapchainImageIndex);
	void RenderKeyboard(uint32_t swapchainImageIndex);
};
