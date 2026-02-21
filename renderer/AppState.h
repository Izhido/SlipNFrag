#pragma once

#include "AppMode.h"
#include <common/xr_dependencies.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include "quakedef.h"
#include "d_lists.h"
#include "Keyboard.h"
#include "Scene.h"
#include "FromEngine.h"
#include "Controller.h"
#include "HandTracker.h"
#include "Actions.h"
#include <thread>
#include <vk_mem_alloc.h>

struct AppState
{
	double PausedTime;
	double PreviousTime;
	double CurrentTime;
	AppMode Mode;
	AppMode PreviousMode;
	bool StartupButtonsPressed;
	bool UncompressButtonsPressed;
	bool SharewareGameDataButtonsPressed;
	bool SharewareExists;
	bool IsRegistered;
	bool Resumed;
	VkDevice Device;
	bool CylinderCompositionLayerEnabled;
	bool IndexTypeUInt8Enabled;
	bool HandTrackingEnabled;
	bool SimultaneousHandsAndControllersEnabled;
	XrSession Session;
	VkCommandPool SetupCommandPool;
	VkQueue Queue;
	int DefaultFOV;
	int FOV;
	float SkyLeft;
	float SkyHorizontal;
	float SkyTop;
	float SkyVertical;
	VkRect2D SwapchainRect;
	uint32_t SwapchainSampleCount;
	std::vector<XrSwapchainImageVulkan2KHR> SwapchainImages;
	std::vector<XrSwapchainImageVulkan2KHR> DepthSwapchainImages;
	std::vector<VkImageView> DepthSwapchainImageViews;
	Screen Screen;
	std::vector<uint32_t> ScreenData;
	std::vector<Texture> ConsoleTextures;
	std::vector<Texture> StatusBarTextures;
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	std::vector<std::string> ConsoleTextureNames;
	std::vector<std::string> StatusBarTextureNames;
#endif
	Keyboard Keyboard;
	std::unordered_map<uint32_t, Texture> KeyboardTextures;
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	std::vector<std::string> KeyboardTextureNames;
#endif
	float KeyboardHitOffsetY;
	XrSwapchain LeftArrowsSwapchain;
	XrSwapchain RightArrowsSwapchain;
	struct FileLoader* FileLoader;
	struct Logger* Logger;
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
	XrMatrix4x4f VertexTransform;
	int ScreenWidth;
	int ScreenHeight;
	int ConsoleWidth;
	int ConsoleHeight;
	std::vector<uint32_t>* NoGameDataImageSource;
	std::vector<uint32_t>* NoGameDataUncompressImageSource;
	std::vector<uint32_t>* InvalidGameDataUncompressImageSource;
	std::vector<uint32_t>* SharewareGameDataImageSource;
	Controller LeftController;
	Controller RightController;
	XrVector2f PreviousThumbstick;
	std::thread EngineThread;
	bool EngineThreadCreated;
	bool EngineThreadStarted;
	bool EngineThreadStopped;
	Actions Actions;
	std::vector<XrPath> SubactionPaths;
	std::vector<XrSpace> HandSpaces;
	std::vector<XrBool32> ActiveHands;
	std::vector<XrMatrix4x4f> ViewMatrices;
	std::vector<XrMatrix4x4f> ProjectionMatrices;
	std::vector<HandTracker> HandTrackers;
	XrSpaceLocation CameraLocation;
	bool CameraLocationIsValid;
	bool Focused;
	VkImageMemoryBarrier copyBarrier;
	VkImageMemoryBarrier submitBarrier;
	VmaAllocator Allocator;
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT;
	PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT;
	PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT;
#endif

	static void AnglesFromQuaternion(XrQuaternionf& quat, float& yaw, float& pitch, float& roll);
	void RenderKeyboard(ScreenPerFrame& perFrame);
	void DestroyImageSources();
	void Destroy();
};
