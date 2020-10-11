#define VK_USE_PLATFORM_ANDROID_KHR
#include <vulkan/vulkan.h>
#include <vector>
#include "VrApi_Helpers.h"
#include <android/log.h>
#include "sys_ovr.h"
#include <android_native_app_glue.h>
#include <android/window.h>
#include <sys/prctl.h>
#include "VrApi.h"
#include "VrApi_Vulkan.h"
#include <dlfcn.h>
#include <cerrno>
#include <unistd.h>
#include "VrApi_Input.h"
#include "VrApi_SystemUtils.h"
#include "vid_ovr.h"
#include "in_ovr.h"
#include "stb_image.h"
#include "d_lists.h"

#define DEBUG 1
#define _DEBUG 1
#define USE_API_DUMP 1

#define VK(func) checkErrors(func, #func);
#define VC(func) func;
#define MAX_UNUSED_COUNT 16

enum PermissionsGrantStatus
{
	PermissionsPending,
	PermissionsGranted,
	PermissionsDenied
};

enum AppMode
{
	AppStartupMode,
	AppScreenMode,
	AppWorldMode,
	AppNoGameDataMode
};

struct Instance
{
	void *loader;
	VkInstance instance;
	VkBool32 validate;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
	PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
	PFN_vkCreateInstance vkCreateInstance;
	PFN_vkDestroyInstance vkDestroyInstance;
	PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
	PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
	PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR;
	PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
	PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR;
	PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
	PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
	PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties;
	PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties;
	PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
	PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties;
	PFN_vkCreateDevice vkCreateDevice;
	PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
	VkDebugReportCallbackEXT debugReportCallback;
};

struct Device
{
	std::vector<const char *> enabledExtensionNames;
	uint32_t enabledLayerCount;
	const char *enabledLayerNames[32];
	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceFeatures2 physicalDeviceFeatures;
	VkPhysicalDeviceProperties2 physicalDeviceProperties;
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	uint32_t queueFamilyCount;
	VkQueueFamilyProperties *queueFamilyProperties;
	std::vector<uint32_t> queueFamilyUsedQueues;
	pthread_mutex_t queueFamilyMutex;
	int workQueueFamilyIndex;
	int presentQueueFamilyIndex;
	bool supportsMultiview;
	bool supportsFragmentDensity;
	VkDevice device;
	PFN_vkDestroyDevice vkDestroyDevice;
	PFN_vkGetDeviceQueue vkGetDeviceQueue;
	PFN_vkQueueSubmit vkQueueSubmit;
	PFN_vkQueueWaitIdle vkQueueWaitIdle;
	PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
	PFN_vkAllocateMemory vkAllocateMemory;
	PFN_vkFreeMemory vkFreeMemory;
	PFN_vkMapMemory vkMapMemory;
	PFN_vkUnmapMemory vkUnmapMemory;
	PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges;
	PFN_vkBindBufferMemory vkBindBufferMemory;
	PFN_vkBindImageMemory vkBindImageMemory;
	PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
	PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
	PFN_vkCreateFence vkCreateFence;
	PFN_vkDestroyFence vkDestroyFence;
	PFN_vkResetFences vkResetFences;
	PFN_vkGetFenceStatus vkGetFenceStatus;
	PFN_vkWaitForFences vkWaitForFences;
	PFN_vkCreateBuffer vkCreateBuffer;
	PFN_vkDestroyBuffer vkDestroyBuffer;
	PFN_vkCreateImage vkCreateImage;
	PFN_vkDestroyImage vkDestroyImage;
	PFN_vkCreateImageView vkCreateImageView;
	PFN_vkDestroyImageView vkDestroyImageView;
	PFN_vkCreateShaderModule vkCreateShaderModule;
	PFN_vkDestroyShaderModule vkDestroyShaderModule;
	PFN_vkCreatePipelineCache vkCreatePipelineCache;
	PFN_vkDestroyPipelineCache vkDestroyPipelineCache;
	PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
	PFN_vkDestroyPipeline vkDestroyPipeline;
	PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
	PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
	PFN_vkCreateSampler vkCreateSampler;
	PFN_vkDestroySampler vkDestroySampler;
	PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
	PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
	PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
	PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
	PFN_vkResetDescriptorPool vkResetDescriptorPool;
	PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
	PFN_vkFreeDescriptorSets vkFreeDescriptorSets;
	PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
	PFN_vkCreateFramebuffer vkCreateFramebuffer;
	PFN_vkDestroyFramebuffer vkDestroyFramebuffer;
	PFN_vkCreateRenderPass vkCreateRenderPass;
	PFN_vkDestroyRenderPass vkDestroyRenderPass;
	PFN_vkCreateCommandPool vkCreateCommandPool;
	PFN_vkDestroyCommandPool vkDestroyCommandPool;
	PFN_vkResetCommandPool vkResetCommandPool;
	PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
	PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
	PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
	PFN_vkEndCommandBuffer vkEndCommandBuffer;
	PFN_vkResetCommandBuffer vkResetCommandBuffer;
	PFN_vkCmdBindPipeline vkCmdBindPipeline;
	PFN_vkCmdSetViewport vkCmdSetViewport;
	PFN_vkCmdSetScissor vkCmdSetScissor;
	PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets;
	PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer;
	PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
	PFN_vkCmdBlitImage vkCmdBlitImage;
	PFN_vkCmdDraw vkCmdDraw;
	PFN_vkCmdDrawIndexed vkCmdDrawIndexed;
	PFN_vkCmdDispatch vkCmdDispatch;
	PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
	PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage;
	PFN_vkCmdResolveImage vkCmdResolveImage;
	PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
	PFN_vkCmdPushConstants vkCmdPushConstants;
	PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass;
	PFN_vkCmdEndRenderPass vkCmdEndRenderPass;
};

struct Context
{
	Device* device;
	uint32_t queueFamilyIndex;
	uint32_t queueIndex;
	VkQueue queue;
	VkCommandPool commandPool;
	VkPipelineCache pipelineCache;
};

struct Buffer
{
	Buffer* next;
	int unusedCount;
	size_t size;
	VkMemoryPropertyFlags flags;
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* mapped;
};

struct BufferWithOffset
{
	Buffer* buffer;
	VkDeviceSize offset;
};

struct Texture
{
	Texture* next;
	void* key;
	void* key2;
	int frameCount;
	int unusedCount;
	int width;
	int height;
	int mipCount;
	int layerCount;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;
};

struct PipelineDescriptorResources
{
	bool created;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
};

struct PipelineResources
{
	PipelineResources *next;
	int unusedCount;
	PipelineDescriptorResources palette;
	PipelineDescriptorResources textured;
	PipelineDescriptorResources sprites;
	PipelineDescriptorResources turbulent;
	PipelineDescriptorResources alias;
	PipelineDescriptorResources viewmodels;
	PipelineDescriptorResources colored;
	PipelineDescriptorResources sky;
	PipelineDescriptorResources floor;
};

struct CachedBuffers
{
	Buffer* buffers;
	Buffer* oldBuffers;
};

struct CachedTextures
{
	Texture* textures;
	Texture* oldTextures;
};

struct Pipeline
{
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
};

struct PipelineAttributes
{
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	std::vector<VkVertexInputBindingDescription> vertexBindings;
	VkPipelineVertexInputStateCreateInfo vertexInputState;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
};

struct Scene
{
	bool createdScene;
	VkShaderModule texturedVertex;
	VkShaderModule texturedFragment;
	VkShaderModule surfaceVertex;
	VkShaderModule spritesFragment;
	VkShaderModule turbulentFragment;
	VkShaderModule aliasVertex;
	VkShaderModule aliasFragment;
	VkShaderModule viewmodelVertex;
	VkShaderModule viewmodelFragment;
	VkShaderModule coloredVertex;
	VkShaderModule coloredFragment;
	VkShaderModule skyVertex;
	VkShaderModule skyFragment;
	VkShaderModule floorVertex;
	VkShaderModule floorFragment;
	VkShaderModule consoleVertex;
	VkShaderModule consoleFragment;
	VkDescriptorSetLayout palette;
	Pipeline textured;
	Pipeline sprites;
	Pipeline turbulent;
	Pipeline alias;
	Pipeline viewmodel;
	Pipeline colored;
	Pipeline sky;
	Pipeline floor;
	Pipeline console;
	PipelineAttributes texturedAttributes;
	PipelineAttributes colormappedAttributes;
	PipelineAttributes coloredAttributes;
	PipelineAttributes skyAttributes;
	PipelineAttributes floorAttributes;
	PipelineAttributes consoleAttributes;
	Buffer matrices;
	int numBuffers;
	int hostClearCount;
	std::unordered_map<void*, std::unordered_map<void*, int>> texturedFrameCountsPerKeys;
	CachedBuffers colormappedVertices;
	CachedBuffers colormappedTexCoords;
	CachedTextures aliasTextures;
	CachedTextures viewmodelTextures;
	std::unordered_map<void*, BufferWithOffset> colormappedVerticesPerKey;
	std::unordered_map<void*, BufferWithOffset> colormappedTexCoordsPerKey;
	std::unordered_map<void*, Texture*> aliasPerKey;
	std::unordered_map<void*, Texture*> viewmodelsPerKey;
	std::vector<int> surfaceOffsets;
	std::vector<int> spriteOffsets;
	std::vector<int> turbulentOffsets;
	std::vector<int> aliasOffsets;
	std::vector<int> aliasColormapOffsets;
	std::vector<int> viewmodelOffsets;
	std::vector<int> viewmodelColormapOffsets;
	std::vector<int> newVertices;
	std::vector<int> newTexCoords;
	std::vector<BufferWithOffset*> aliasVerticesList;
	std::vector<BufferWithOffset*> aliasTexCoordsList;
	std::vector<BufferWithOffset*> viewmodelVerticesList;
	std::vector<BufferWithOffset*> viewmodelTexCoordsList;
	std::vector<Texture*> surfaceList;
	std::vector<Texture*> spriteList;
	std::unordered_map<void*, Texture*> turbulentPerKey;
	std::vector<Texture*> turbulentList;
	std::vector<Texture*> aliasList;
	std::vector<Texture*> aliasColormapList;
	std::vector<Texture*> viewmodelList;
	std::vector<Texture*> viewmodelColormapList;
};

struct PerImage
{
	CachedBuffers sceneMatricesStagingBuffers;
	CachedBuffers vertices;
	CachedBuffers attributes;
	CachedBuffers indices16;
	CachedBuffers indices32;
	CachedBuffers uniforms;
	CachedBuffers stagingBuffers;
	CachedTextures surfaces;
	CachedTextures sprites;
	CachedTextures turbulent;
	CachedTextures colormaps;
	PipelineResources* pipelineResources;
	int paletteOffset;
	int host_colormapOffset;
	int skyOffset;
	int floorOffset;
	int paletteChanged;
	Texture* palette;
	Texture* host_colormap;
	int hostClearCount;
	Texture* sky;
	Texture* floor;
	VkDeviceSize texturedVertexBase;
	VkDeviceSize coloredVertexBase;
	VkDeviceSize texturedAttributeBase;
	VkDeviceSize colormappedAttributeBase;
	VkDeviceSize vertexTransformBase;
	VkDeviceSize colormappedIndex16Base;
	VkDeviceSize coloredIndex16Base;
	VkDeviceSize coloredIndex32Base;
	VkCommandBuffer commandBuffer;
	VkFence fence;
	bool submitted;
};

struct Framebuffer
{
	int swapChainLength;
	ovrTextureSwapChain *colorTextureSwapChain;
	std::vector<Texture> colorTextures;
	std::vector<Texture> fragmentDensityTextures;
	std::vector<VkImageMemoryBarrier> startBarriers;
	std::vector<VkImageMemoryBarrier> endBarriers;
	VkImage depthImage;
	VkDeviceMemory depthMemory;
	VkImageView depthImageView;
	Texture renderTexture;
	std::vector<VkFramebuffer> framebuffers;
	int width;
	int height;
};

struct ColorSwapChain
{
	int SwapChainLength;
	ovrTextureSwapChain *SwapChain;
	std::vector<VkImage> ColorTextures;
	std::vector<VkImage> FragmentDensityTextures;
	std::vector<VkExtent2D> FragmentDensityTextureSizes;
};

struct View
{
	Framebuffer framebuffer;
	ColorSwapChain colorSwapChain;
	int index;
	std::vector<PerImage> perImage;
};

struct ConsolePipelineResources
{
	ConsolePipelineResources *next;
	int unusedCount;
	PipelineDescriptorResources descriptorResources;
};

struct ConsolePerImage
{
	CachedBuffers vertices;
	CachedBuffers indices;
	CachedBuffers stagingBuffers;
	ConsolePipelineResources* pipelineResources;
	int paletteOffset;
	int textureOffset;
	int paletteChanged;
	Texture* palette;
	Texture* texture;
	VkCommandBuffer commandBuffer;
	VkFence fence;
	bool submitted;
};

struct ConsoleFramebuffer
{
	int swapChainLength;
	ovrTextureSwapChain *colorTextureSwapChain;
	std::vector<Texture> colorTextures;
	std::vector<VkImageMemoryBarrier> startBarriers;
	std::vector<VkImageMemoryBarrier> endBarriers;
	Texture renderTexture;
	std::vector<VkFramebuffer> framebuffers;
	int width;
	int height;
};

struct ConsoleColorSwapChain
{
	int SwapChainLength;
	ovrTextureSwapChain *SwapChain;
	std::vector<VkImage> ColorTextures;
};

struct ConsoleView
{
	ConsoleFramebuffer framebuffer;
	ConsoleColorSwapChain colorSwapChain;
	int index;
	std::vector<ConsolePerImage> perImage;
};

struct Console
{
	ovrTextureSwapChain* SwapChain;
	VkCommandBuffer CommandBuffer;
	VkRenderPass RenderPass;
	ConsoleView View;
};

struct Panel
{
	ovrTextureSwapChain* SwapChain;
	std::vector<uint32_t> Data;
	VkImage Image;
	Buffer Buffer;
};

struct Screen
{
	ovrTextureSwapChain* SwapChain;
	std::vector<uint32_t> Data;
	VkImage Image;
	Buffer Buffer;
	VkCommandBuffer CommandBuffer;
	VkSubmitInfo SubmitInfo;
};

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
};

static const int queueCount = 1;
static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;

extern m_state_t m_state;
extern qboolean r_skip_fov_check;
extern vec3_t r_modelorg_delta;

PermissionsGrantStatus PermissionsGrantStatus = PermissionsPending;

extern "C" void Java_com_heribertodelgado_slipnfrag_MainActivity_notifyPermissionsGrantStatus(JNIEnv* jni, jclass clazz, int permissionsGranted)
{
	if (permissionsGranted != 0)
	{
		PermissionsGrantStatus = PermissionsGranted;
	}
	else
	{
		PermissionsGrantStatus = PermissionsDenied;
	}
}

void checkErrors(VkResult result, const char *function)
{
	if (result != VK_SUCCESS)
	{
		const char* errorString;
		switch (result)
		{
			case VK_NOT_READY:
				errorString = "VK_NOT_READY";
				break;
			case VK_TIMEOUT:
				errorString = "VK_TIMEOUT";
				break;
			case VK_EVENT_SET:
				errorString = "VK_EVENT_SET";
				break;
			case VK_EVENT_RESET:
				errorString = "VK_EVENT_RESET";
				break;
			case VK_INCOMPLETE:
				errorString = "VK_INCOMPLETE";
				break;
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				errorString = "VK_ERROR_OUT_OF_HOST_MEMORY";
				break;
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				errorString = "VK_ERROR_OUT_OF_DEVICE_MEMORY";
				break;
			case VK_ERROR_INITIALIZATION_FAILED:
				errorString = "VK_ERROR_INITIALIZATION_FAILED";
				break;
			case VK_ERROR_DEVICE_LOST:
				errorString = "VK_ERROR_DEVICE_LOST";
				break;
			case VK_ERROR_MEMORY_MAP_FAILED:
				errorString = "VK_ERROR_MEMORY_MAP_FAILED";
				break;
			case VK_ERROR_LAYER_NOT_PRESENT:
				errorString = "VK_ERROR_LAYER_NOT_PRESENT";
				break;
			case VK_ERROR_EXTENSION_NOT_PRESENT:
				errorString = "VK_ERROR_EXTENSION_NOT_PRESENT";
				break;
			case VK_ERROR_FEATURE_NOT_PRESENT:
				errorString = "VK_ERROR_FEATURE_NOT_PRESENT";
				break;
			case VK_ERROR_INCOMPATIBLE_DRIVER:
				errorString = "VK_ERROR_INCOMPATIBLE_DRIVER";
				break;
			case VK_ERROR_TOO_MANY_OBJECTS:
				errorString = "VK_ERROR_TOO_MANY_OBJECTS";
				break;
			case VK_ERROR_FORMAT_NOT_SUPPORTED:
				errorString = "VK_ERROR_FORMAT_NOT_SUPPORTED";
				break;
			case VK_ERROR_SURFACE_LOST_KHR:
				errorString = "VK_ERROR_SURFACE_LOST_KHR";
				break;
			case VK_SUBOPTIMAL_KHR:
				errorString = "VK_SUBOPTIMAL_KHR";
				break;
			case VK_ERROR_OUT_OF_DATE_KHR:
				errorString = "VK_ERROR_OUT_OF_DATE_KHR";
				break;
			case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
				errorString = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
				break;
			case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
				errorString = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
				break;
			case VK_ERROR_VALIDATION_FAILED_EXT:
				errorString = "VK_ERROR_VALIDATION_FAILED_EXT";
				break;
			case VK_ERROR_INVALID_SHADER_NV:
				errorString = "VK_ERROR_INVALID_SHADER_NV";
				break;
			default:
				errorString = "unknown";
		}
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Vulkan error: %s: %s\n", function, errorString);
		exit(0);
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject, size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *pUserData)
{
	const char *reportType = "Unknown";
	if ((msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0)
	{
		reportType = "Error";
	}
	else if ((msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0)
	{
		reportType = "Warning";
	}
	else if ((msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) != 0)
	{
		reportType = "Performance Warning";
	}
	else if ((msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) != 0)
	{
		reportType = "Information";
	}
	else if ((msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) != 0)
	{
		reportType = "Debug";
	}
	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s: [%s] Code %d : %s", reportType, pLayerPrefix, msgCode, pMsg);
	return VK_FALSE;
}

double getTime()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec * 1e9 + now.tv_nsec) * 0.000000001;
}

void createMemoryAllocateInfo(AppState& appState, VkMemoryRequirements& memoryRequirements, VkMemoryPropertyFlags properties, VkMemoryAllocateInfo& memoryAllocateInfo)
{
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	auto found = false;
	for (auto i = 0; i < appState.Context.device->physicalDeviceMemoryProperties.memoryTypeCount; i++)
	{
		if ((memoryRequirements.memoryTypeBits & (1 << i)) != 0)
		{
			const VkFlags propertyFlags = appState.Context.device->physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags;
			if ((propertyFlags & properties) == properties)
			{
				found = true;
				memoryAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
	}
	if (!found)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "createMemoryAllocateInfo(): Memory type %d with properties %d not found.", memoryRequirements.memoryTypeBits, properties);
		vrapi_Shutdown();
		exit(0);
	}
}

void createBuffer(AppState& appState, VkDeviceSize size, VkBufferUsageFlags usage, Buffer*& buffer)
{
	buffer = new Buffer();
	memset(buffer, 0, sizeof(Buffer));
	buffer->size = size;
	VkBufferCreateInfo bufferCreateInfo { };
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = buffer->size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK(appState.Device.vkCreateBuffer(appState.Device.device, &bufferCreateInfo, nullptr, &buffer->buffer));
	buffer->flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	VkMemoryRequirements memoryRequirements;
	VC(appState.Device.vkGetBufferMemoryRequirements(appState.Device.device, buffer->buffer, &memoryRequirements));
	VkMemoryAllocateInfo memoryAllocateInfo { };
	createMemoryAllocateInfo(appState, memoryRequirements, buffer->flags, memoryAllocateInfo);
	VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &buffer->memory));
	VK(appState.Device.vkBindBufferMemory(appState.Device.device, buffer->buffer, buffer->memory, 0));
}

void createVertexBuffer(AppState& appState, VkDeviceSize size, Buffer*& buffer)
{
	createBuffer(appState, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, buffer);
}

void createIndexBuffer(AppState& appState, VkDeviceSize size, Buffer*& buffer)
{
	createBuffer(appState, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, buffer);
}

void createStagingBuffer(AppState& appState, VkDeviceSize size, Buffer*& buffer)
{
	createBuffer(appState, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, buffer);
}

void createStagingStorageBuffer(AppState& appState, VkDeviceSize size, Buffer*& buffer)
{
	createBuffer(appState, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, buffer);
}

void createUniformBuffer(AppState& appState, VkDeviceSize size, Buffer*& buffer)
{
	createBuffer(appState, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, buffer);
}

void createShaderModule(AppState& appState, struct android_app* app, const char* filename, VkShaderModule* shaderModule)
{
	auto file = AAssetManager_open(app->activity->assetManager, filename, AASSET_MODE_BUFFER);
	size_t length = AAsset_getLength(file);
	if ((length % 4) != 0)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "createShaderModule(): %s is not 4-byte aligned.", filename);
		exit(0);
	}
	std::vector<unsigned char> buffer(length);
	AAsset_read(file, buffer.data(), length);
	VkShaderModuleCreateInfo moduleCreateInfo { };
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.pCode = (uint32_t *)buffer.data();
	moduleCreateInfo.codeSize = length;
	VK(appState.Device.vkCreateShaderModule(appState.Device.device, &moduleCreateInfo, nullptr, shaderModule));
}

void createTexture(AppState& appState, VkCommandBuffer commandBuffer, uint32_t width, uint32_t height, VkFormat format, uint32_t mipCount, VkImageUsageFlags usage, Texture*& texture)
{
	texture = new Texture();
	memset(texture, 0, sizeof(Texture));
	texture->width = width;
	texture->height = height;
	texture->mipCount = mipCount;
	texture->layerCount = 1;
	VkImageCreateInfo imageCreateInfo { };
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = format;
	imageCreateInfo.extent.width = texture->width;
	imageCreateInfo.extent.height = texture->height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = texture->mipCount;
	imageCreateInfo.arrayLayers = texture->layerCount;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK(appState.Device.vkCreateImage(appState.Device.device, &imageCreateInfo, nullptr, &texture->image));
	VkMemoryRequirements memoryRequirements;
	VC(appState.Device.vkGetImageMemoryRequirements(appState.Device.device, texture->image, &memoryRequirements));
	VkMemoryAllocateInfo memoryAllocateInfo { };
	createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo);
	VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &texture->memory));
	VK(appState.Device.vkBindImageMemory(appState.Device.device, texture->image, texture->memory, 0));
	VkImageViewCreateInfo imageViewCreateInfo { };
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = texture->image;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = imageCreateInfo.format;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.levelCount = texture->mipCount;
	imageViewCreateInfo.subresourceRange.layerCount = texture->layerCount;
	VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &texture->view));
	VkImageMemoryBarrier imageMemoryBarrier { };
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrier.image = texture->image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.levelCount = texture->mipCount;
	imageMemoryBarrier.subresourceRange.layerCount = texture->layerCount;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	VkSamplerCreateInfo samplerCreateInfo { };
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.maxLod = texture->mipCount;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
	VK(appState.Device.vkCreateSampler(appState.Device.device, &samplerCreateInfo, nullptr, &texture->sampler));
}

void deleteOldBuffers(AppState& appState, CachedBuffers& cachedBuffers)
{
	for (Buffer** b = &cachedBuffers.oldBuffers; *b != nullptr; )
	{
		(*b)->unusedCount++;
		if ((*b)->unusedCount >= MAX_UNUSED_COUNT)
		{
			Buffer* next = (*b)->next;
			if ((*b)->mapped != nullptr)
			{
				VC(appState.Device.vkUnmapMemory(appState.Device.device, (*b)->memory));
			}
			VC(appState.Device.vkDestroyBuffer(appState.Device.device, (*b)->buffer, nullptr));
			VC(appState.Device.vkFreeMemory(appState.Device.device, (*b)->memory, nullptr));
			delete *b;
			*b = next;
		}
		else
		{
			b = &(*b)->next;
		}
	}
}

void disposeFrontBuffers(AppState& appState, CachedBuffers& cachedBuffers)
{
	for (Buffer* b = cachedBuffers.buffers, *next = nullptr; b != nullptr; b = next)
	{
		next = b->next;
		b->next = cachedBuffers.oldBuffers;
		cachedBuffers.oldBuffers = b;
	}
	cachedBuffers.buffers = nullptr;
}

void forcedDisposeFrontBuffers(AppState& appState, CachedBuffers& cachedBuffers)
{
	for (Buffer* b = cachedBuffers.buffers, *next = nullptr; b != nullptr; b = next)
	{
		next = b->next;
		b->size = 0;
		b->next = cachedBuffers.oldBuffers;
		cachedBuffers.oldBuffers = b;
	}
	cachedBuffers.buffers = nullptr;
}

void resetCachedBuffers(AppState& appState, CachedBuffers& cachedBuffers)
{
	deleteOldBuffers(appState, cachedBuffers);
	disposeFrontBuffers(appState, cachedBuffers);
}

void deleteTexture(AppState& appState, Texture* texture)
{
	VC(appState.Device.vkDestroySampler(appState.Device.device, texture->sampler, nullptr));
	if (texture->memory != VK_NULL_HANDLE)
	{
		VC(appState.Device.vkDestroyImageView(appState.Device.device, texture->view, nullptr));
		VC(appState.Device.vkDestroyImage(appState.Device.device, texture->image, nullptr));
		VC(appState.Device.vkFreeMemory(appState.Device.device, texture->memory, nullptr));
	}
}

void deleteOldTextures(AppState& appState, CachedTextures& cachedTextures)
{
	for (Texture** t = &cachedTextures.oldTextures; *t != nullptr; )
	{
		(*t)->unusedCount++;
		if ((*t)->unusedCount >= MAX_UNUSED_COUNT)
		{
			Texture* next = (*t)->next;
			deleteTexture(appState, *t);
			delete *t;
			*t = next;
		}
		else
		{
			t = &(*t)->next;
		}
	}
}

void disposeFrontTextures(AppState& appState, CachedTextures& cachedTextures)
{
	for (Texture* t = cachedTextures.textures, *next = nullptr; t != nullptr; t = next)
	{
		next = t->next;
		t->next = cachedTextures.oldTextures;
		cachedTextures.oldTextures = t;
	}
	cachedTextures.textures = nullptr;
}

void forcedDisposeFrontTextures(AppState& appState, CachedTextures& cachedTextures)
{
	for (Texture* t = cachedTextures.textures, *next = nullptr; t != nullptr; t = next)
	{
		next = t->next;
		t->next = cachedTextures.oldTextures;
		t->width = 0;
		t->height = 0;
		cachedTextures.oldTextures = t;
	}
	cachedTextures.textures = nullptr;
}

void resetCachedTextures(AppState& appState, CachedTextures& cachedTextures)
{
	deleteOldTextures(appState, cachedTextures);
	disposeFrontTextures(appState, cachedTextures);
}

void fillTexture(AppState& appState, Texture* texture, Buffer* buffer, VkDeviceSize offset, VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier imageMemoryBarrier { };
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = texture->image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	VkBufferImageCopy region { };
	region.bufferOffset = offset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = texture->width;
	region.imageExtent.height = texture->height;
	region.imageExtent.depth = 1;
	VC(appState.Device.vkCmdCopyBufferToImage(commandBuffer, buffer->buffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
}

void fillMipmappedTexture(AppState& appState, Texture* texture, Buffer* buffer, VkDeviceSize offset, VkCommandBuffer commandBuffer)
{
	VkImageMemoryBarrier imageMemoryBarrier { };
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = texture->image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	VkBufferImageCopy region { };
	region.bufferOffset = offset;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent.width = texture->width;
	region.imageExtent.height = texture->height;
	region.imageExtent.depth = 1;
	VC(appState.Device.vkCmdCopyBufferToImage(commandBuffer, buffer->buffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	auto width = texture->width;
	auto height = texture->height;
	for (auto i = 1; i < texture->mipCount; i++)
	{
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.subresourceRange.baseMipLevel = i;
		VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
		VkImageBlit blit { };
		blit.srcOffsets[1].x = width;
		blit.srcOffsets[1].y = height;
		blit.srcOffsets[1].z = 1;
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.layerCount = 1;
		width /= 2;
		if (width < 1)
		{
			width = 1;
		}
		height /= 2;
		if (height < 1)
		{
			height = 1;
		}
		blit.dstOffsets[1].x = width;
		blit.dstOffsets[1].y = height;
		blit.dstOffsets[1].z = 1;
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.layerCount = 1;
		VC(appState.Device.vkCmdBlitImage(commandBuffer, texture->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST));
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
	}
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = texture->mipCount;
	VC(appState.Device.vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
}

void moveBufferToFront(Buffer* buffer, CachedBuffers& cachedBuffers)
{
	buffer->unusedCount = 0;
	buffer->next = cachedBuffers.buffers;
	cachedBuffers.buffers = buffer;
}

void moveTextureToFront(Texture* texture, CachedTextures& cachedTextures)
{
	texture->unusedCount = 0;
	texture->next = cachedTextures.textures;
	cachedTextures.textures = texture;
}

void deleteCachedBuffers(AppState& appState, CachedBuffers& cachedBuffers)
{
	for (Buffer* b = cachedBuffers.buffers, *next = nullptr; b != nullptr; b = next)
	{
		next = b->next;
		if (b->mapped != nullptr)
		{
			VC(appState.Device.vkUnmapMemory(appState.Device.device, b->memory));
		}
		VC(appState.Device.vkDestroyBuffer(appState.Device.device, b->buffer, nullptr));
		VC(appState.Device.vkFreeMemory(appState.Device.device, b->memory, nullptr));
		delete b;
	}
	for (Buffer* b = cachedBuffers.oldBuffers, *next = nullptr; b != nullptr; b = next)
	{
		next = b->next;
		if (b->mapped != nullptr)
		{
			VC(appState.Device.vkUnmapMemory(appState.Device.device, b->memory));
		}
		VC(appState.Device.vkDestroyBuffer(appState.Device.device, b->buffer, nullptr));
		VC(appState.Device.vkFreeMemory(appState.Device.device, b->memory, nullptr));
		delete b;
	}
}

void deleteCachedTextures(AppState& appState, CachedTextures& cachedTextures)
{
	for (Texture* t = cachedTextures.textures, *next = nullptr; t != nullptr; t = next)
	{
		next = t->next;
		deleteTexture(appState, t);
		delete t;
	}
	for (Texture* t = cachedTextures.oldTextures, *next = nullptr; t != nullptr; t = next)
	{
		next = t->next;
		deleteTexture(appState, t);
		delete t;
	}
}

void deletePipelineDescriptorResources(AppState& appState, PipelineDescriptorResources& pipelineDescriptorResources)
{
	if (pipelineDescriptorResources.created)
	{
		VC(appState.Device.vkFreeDescriptorSets(appState.Device.device, pipelineDescriptorResources.descriptorPool, 1, &pipelineDescriptorResources.descriptorSet));
		VC(appState.Device.vkDestroyDescriptorPool(appState.Device.device, pipelineDescriptorResources.descriptorPool, nullptr));
	}
}

void deletePipeline(AppState& appState, Pipeline& pipeline)
{
	VC(appState.Device.vkDestroyPipeline(appState.Device.device, pipeline.pipeline, nullptr));
	VC(appState.Device.vkDestroyPipelineLayout(appState.Device.device, pipeline.pipelineLayout, nullptr));
	VC(appState.Device.vkDestroyDescriptorSetLayout(appState.Device.device, pipeline.descriptorSetLayout, nullptr));
}

void appHandleCommands(struct android_app *app, int32_t cmd)
{
	auto appState = (AppState*)app->userData;
	double delta;
	switch (cmd)
	{
		case APP_CMD_PAUSE:
			appState->Resumed = false;
			appState->PausedTime = getTime();
			break;
		case APP_CMD_RESUME:
			appState->Resumed = true;
			delta = getTime() - appState->PausedTime;
			if (appState->PreviousTime > 0)
			{
				appState->PreviousTime += delta;
			}
			if (appState->CurrentTime > 0)
			{
				appState->CurrentTime += delta;
			}
			break;
		case APP_CMD_INIT_WINDOW:
			appState->NativeWindow = app->window;
			break;
		case APP_CMD_TERM_WINDOW:
			appState->NativeWindow = nullptr;
			break;
		case APP_CMD_DESTROY:
			exit(0);
	}
}

bool leftButtonIsDown(AppState& appState, uint32_t button)
{
	return ((appState.LeftButtons & button) != 0 && (appState.PreviousLeftButtons & button) == 0);
}

bool leftButtonIsUp(AppState& appState, uint32_t button)
{
	return ((appState.LeftButtons & button) == 0 && (appState.PreviousLeftButtons & button) != 0);
}

bool rightButtonIsDown(AppState& appState, uint32_t button)
{
	return ((appState.RightButtons & button) != 0 && (appState.PreviousRightButtons & button) == 0);
}

bool rightButtonIsUp(AppState& appState, uint32_t button)
{
	return ((appState.RightButtons & button) == 0 && (appState.PreviousRightButtons & button) != 0);
}

void android_main(struct android_app *app)
{
	ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);
	ovrJava java;
	java.Vm = app->activity->vm;
	java.Vm->AttachCurrentThread(&java.Env, nullptr);
	java.ActivityObject = app->activity->clazz;
	prctl(PR_SET_NAME, (long) "SlipNFrag", 0, 0, 0);
	ovrInitParms initParms = vrapi_DefaultInitParms(&java);
	initParms.GraphicsAPI = VRAPI_GRAPHICS_API_VULKAN_1;
	int32_t initResult = vrapi_Initialize(&initParms);
	if (initResult != VRAPI_INITIALIZE_SUCCESS)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vrapi_Initialize() failed: %i", initResult);
		exit(0);
	}
	AppState appState { };
	appState.FrameIndex = 1;
	appState.SwapInterval = 1;
	appState.CpuLevel = 2;
	appState.GpuLevel = 2;
	appState.Java = java;
	appState.PausedTime = -1;
	appState.PreviousTime = -1;
	appState.CurrentTime = -1;
	uint32_t instanceExtensionNamesSize;
	if (vrapi_GetInstanceExtensionsVulkan(nullptr, &instanceExtensionNamesSize))
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vrapi_GetInstanceExtensionsVulkan() failed to get size of extensions string.");
		vrapi_Shutdown();
		exit(0);
	}
	std::vector<char> instanceExtensionNames(instanceExtensionNamesSize);
	if (vrapi_GetInstanceExtensionsVulkan(instanceExtensionNames.data(), &instanceExtensionNamesSize))
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vrapi_GetInstanceExtensionsVulkan() failed to get extensions string.");
		vrapi_Shutdown();
		exit(0);
	}
	Instance instance { };
#if defined(_DEBUG)
	instance.validate = VK_TRUE;
#else
	instance.validate = VK_FALSE;
#endif
	auto libraryFilename = "libvulkan.so";
	instance.loader = dlopen(libraryFilename, RTLD_NOW | RTLD_LOCAL);
	if (instance.loader == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): %s not available: %s", libraryFilename, dlerror());
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr) dlsym(instance.loader, "vkGetInstanceProcAddr");
	instance.vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties) dlsym(instance.loader, "vkEnumerateInstanceLayerProperties");
	instance.vkCreateInstance = (PFN_vkCreateInstance) dlsym(instance.loader, "vkCreateInstance");
	if (instance.validate)
	{
		instanceExtensionNames.resize(instanceExtensionNamesSize + 1 + strlen(VK_EXT_DEBUG_REPORT_EXTENSION_NAME));
		strcpy(instanceExtensionNames.data() + instanceExtensionNamesSize, " ");
		strcpy(instanceExtensionNames.data() + instanceExtensionNamesSize + 1, VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		instanceExtensionNamesSize = instanceExtensionNames.size();
	}
	instanceExtensionNames.resize(instanceExtensionNamesSize + 1 + strlen(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME));
	strcpy(instanceExtensionNames.data() + instanceExtensionNamesSize, " ");
	strcpy(instanceExtensionNames.data() + instanceExtensionNamesSize + 1, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
	std::vector<const char *> enabledExtensionNames;
	std::string extensionName;
	for (auto c : instanceExtensionNames)
	{
		if (c > ' ')
		{
			extensionName += c;
		}
		else if (extensionName.length() > 0)
		{
			char *extensionNameToAdd = new char[extensionName.length() + 1];
			strcpy(extensionNameToAdd, extensionName.c_str());
			enabledExtensionNames.push_back(extensionNameToAdd);
			extensionName = "";
		}
	}
	if (extensionName.length() > 0)
	{
		char *extensionNameToAdd = new char[extensionName.length() + 1];
		strcpy(extensionNameToAdd, extensionName.c_str());
		enabledExtensionNames.push_back(extensionNameToAdd);
	}
#if defined(_DEBUG)
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Enabled Extensions: ");
	for (uint32_t i = 0; i < enabledExtensionNames.size(); i++)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "\t(%d):%s", i, enabledExtensionNames[i]);
	}
#endif
	static const char *requestedLayers[]
	{
#if defined(_DEBUG)
		"VK_LAYER_LUNARG_core_validation",
		"VK_LAYER_LUNARG_parameter_validation",
#endif
		"VK_LAYER_LUNARG_object_tracker",
		"VK_LAYER_GOOGLE_threading",
		"VK_LAYER_GOOGLE_unique_objects"
#if USE_API_DUMP == 1
		,"VK_LAYER_LUNARG_api_dump"
#endif
	};
	const uint32_t requestedCount = sizeof(requestedLayers) / sizeof(requestedLayers[0]);
	std::vector<const char *> enabledLayerNames;
	if (instance.validate)
	{
		uint32_t availableLayerCount = 0;
		VK(instance.vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr));
		std::vector<VkLayerProperties> availableLayers(availableLayerCount);
		VK(instance.vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data()));
		for (uint32_t i = 0; i < requestedCount; i++)
		{
			for (uint32_t j = 0; j < availableLayerCount; j++)
			{
				if (strcmp(requestedLayers[i], availableLayers[j].layerName) == 0)
				{
					enabledLayerNames.push_back(requestedLayers[i]);
					break;
				}
			}
		}
	}
#if defined(_DEBUG)
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Enabled Layers ");
	for (uint32_t i = 0; i < enabledLayerNames.size(); i++)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "\t(%d):%s", i, enabledLayerNames[i]);
	}
#endif
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "--------------------------------\n");
	const uint32_t apiMajor = VK_VERSION_MAJOR(VK_API_VERSION_1_0);
	const uint32_t apiMinor = VK_VERSION_MINOR(VK_API_VERSION_1_0);
	const uint32_t apiPatch = VK_VERSION_PATCH(VK_API_VERSION_1_0);
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Instance API version : %d.%d.%d\n", apiMajor, apiMinor, apiPatch);
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "--------------------------------\n");
	VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo { };
	debugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	debugReportCallbackCreateInfo.pfnCallback = debugReportCallback;
	VkApplicationInfo appInfo { };
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Slip & Frag";
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "Oculus Mobile SDK";
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_API_VERSION_1_0;
	VkInstanceCreateInfo instanceCreateInfo { };
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = (instance.validate) ? &debugReportCallbackCreateInfo : nullptr;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledLayerCount = enabledLayerNames.size();
	instanceCreateInfo.ppEnabledLayerNames = (const char *const *) (enabledLayerNames.size() != 0 ? enabledLayerNames.data() : nullptr);
	instanceCreateInfo.enabledExtensionCount = enabledExtensionNames.size();
	instanceCreateInfo.ppEnabledExtensionNames = (const char *const *) (enabledExtensionNames.size() != 0 ? enabledExtensionNames.data() : nullptr);
	VK(instance.vkCreateInstance(&instanceCreateInfo, nullptr, &instance.instance));
	instance.vkDestroyInstance = (PFN_vkDestroyInstance) (instance.vkGetInstanceProcAddr(instance.instance, "vkDestroyInstance"));
	if (instance.vkDestroyInstance == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkDestroyInstance.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) (instance.vkGetInstanceProcAddr(instance.instance, "vkEnumeratePhysicalDevices"));
	if (instance.vkEnumeratePhysicalDevices == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkEnumeratePhysicalDevices.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures) (instance.vkGetInstanceProcAddr(instance.instance, "vkGetPhysicalDeviceFeatures"));
	if (instance.vkGetPhysicalDeviceFeatures == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceFeatures.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkGetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR) (instance.vkGetInstanceProcAddr(instance.instance, "vkGetPhysicalDeviceFeatures2KHR"));
	if (instance.vkGetPhysicalDeviceFeatures2KHR == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceFeatures2KHR.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties) (instance.vkGetInstanceProcAddr(instance.instance, "vkGetPhysicalDeviceProperties"));
	if (instance.vkGetPhysicalDeviceProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR) (instance.vkGetInstanceProcAddr(instance.instance, "vkGetPhysicalDeviceProperties2KHR"));
	if (instance.vkGetPhysicalDeviceProperties2KHR == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceProperties2KHR.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties) (instance.vkGetInstanceProcAddr(instance.instance, "vkGetPhysicalDeviceMemoryProperties"));
	if (instance.vkGetPhysicalDeviceMemoryProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceMemoryProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties) (instance.vkGetInstanceProcAddr(instance.instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
	if (instance.vkGetPhysicalDeviceQueueFamilyProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceQueueFamilyProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties) (instance.vkGetInstanceProcAddr(instance.instance, "vkGetPhysicalDeviceFormatProperties"));
	if (instance.vkGetPhysicalDeviceFormatProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceFormatProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties) (instance.vkGetInstanceProcAddr(instance.instance, "vkGetPhysicalDeviceImageFormatProperties"));
	if (instance.vkGetPhysicalDeviceImageFormatProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceImageFormatProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties) (instance.vkGetInstanceProcAddr(instance.instance, "vkEnumerateDeviceExtensionProperties"));
	if (instance.vkEnumerateDeviceExtensionProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkEnumerateDeviceExtensionProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties) (instance.vkGetInstanceProcAddr(instance.instance, "vkEnumerateDeviceLayerProperties"));
	if (instance.vkEnumerateDeviceLayerProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkEnumerateDeviceLayerProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkCreateDevice = (PFN_vkCreateDevice) (instance.vkGetInstanceProcAddr(instance.instance, "vkCreateDevice"));
	if (instance.vkCreateDevice == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkCreateDevice.");
		vrapi_Shutdown();
		exit(0);
	}
	instance.vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) (instance.vkGetInstanceProcAddr(instance.instance, "vkGetDeviceProcAddr"));
	if (instance.vkGetDeviceProcAddr == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkGetDeviceProcAddr.");
		vrapi_Shutdown();
		exit(0);
	}
	if (instance.validate)
	{
		instance.vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT) (instance.vkGetInstanceProcAddr(instance.instance, "vkCreateDebugReportCallbackEXT"));
		if (instance.vkCreateDebugReportCallbackEXT == nullptr)
		{
			__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkCreateDebugReportCallbackEXT.");
			vrapi_Shutdown();
			exit(0);
		}
		instance.vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT) (instance.vkGetInstanceProcAddr(instance.instance, "vkDestroyDebugReportCallbackEXT"));
		if (instance.vkDestroyDebugReportCallbackEXT == nullptr)
		{
			__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetInstanceProcAddr() could not find vkDestroyDebugReportCallbackEXT.");
			vrapi_Shutdown();
			exit(0);
		}
		VK(instance.vkCreateDebugReportCallbackEXT(instance.instance, &debugReportCallbackCreateInfo, nullptr, &instance.debugReportCallback));
	}
	uint32_t deviceExtensionNamesSize;
	if (vrapi_GetDeviceExtensionsVulkan(nullptr, &deviceExtensionNamesSize))
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vrapi_GetDeviceExtensionsVulkan() failed to get size of extensions string.");
		vrapi_Shutdown();
		exit(0);
	}
	std::vector<char> deviceExtensionNames(deviceExtensionNamesSize);
	if (vrapi_GetDeviceExtensionsVulkan(deviceExtensionNames.data(), &deviceExtensionNamesSize))
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vrapi_GetDeviceExtensionsVulkan() failed to get extensions string.");
		vrapi_Shutdown();
		exit(0);
	}
	extensionName.clear();
	for (auto c : deviceExtensionNames)
	{
		if (c > ' ')
		{
			extensionName += c;
		}
		else if (extensionName.length() > 0)
		{
			char *extensionNameToAdd = new char[extensionName.length() + 1];
			strcpy(extensionNameToAdd, extensionName.c_str());
			appState.Device.enabledExtensionNames.push_back(extensionNameToAdd);
			extensionName = "";
		}
	}
	if (extensionName.length() > 0)
	{
		char *extensionNameToAdd = new char[extensionName.length() + 1];
		strcpy(extensionNameToAdd, extensionName.c_str());
		appState.Device.enabledExtensionNames.push_back(extensionNameToAdd);
	}
#if defined(_DEBUG)
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Requested Extensions: ");
	for (uint32_t i = 0; i < appState.Device.enabledExtensionNames.size(); i++)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "\t(%d):%s", i, appState.Device.enabledExtensionNames[i]);
	}
#endif
	uint32_t physicalDeviceCount = 0;
	VK(instance.vkEnumeratePhysicalDevices(instance.instance, &physicalDeviceCount, nullptr));
	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	VK(instance.vkEnumeratePhysicalDevices(instance.instance, &physicalDeviceCount, physicalDevices.data()));
	for (uint32_t physicalDeviceIndex = 0; physicalDeviceIndex < physicalDeviceCount; physicalDeviceIndex++)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		VC(instance.vkGetPhysicalDeviceProperties(physicalDevices[physicalDeviceIndex], &physicalDeviceProperties));
		const uint32_t driverMajor = VK_VERSION_MAJOR(physicalDeviceProperties.driverVersion);
		const uint32_t driverMinor = VK_VERSION_MINOR(physicalDeviceProperties.driverVersion);
		const uint32_t driverPatch = VK_VERSION_PATCH(physicalDeviceProperties.driverVersion);
		const uint32_t apiMajor = VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion);
		const uint32_t apiMinor = VK_VERSION_MINOR(physicalDeviceProperties.apiVersion);
		const uint32_t apiPatch = VK_VERSION_PATCH(physicalDeviceProperties.apiVersion);
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "--------------------------------\n");
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Device Name          : %s\n", physicalDeviceProperties.deviceName);
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Device Type          : %s\n", ((physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) ? "integrated GPU" : ((physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? "discrete GPU" : ((physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) ? "virtual GPU" : ((physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) ? "CPU" : "unknown")))));
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Vendor ID            : 0x%04X\n", physicalDeviceProperties.vendorID);
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Device ID            : 0x%04X\n", physicalDeviceProperties.deviceID);
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Driver Version       : %d.%d.%d\n", driverMajor, driverMinor, driverPatch);
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "API Version          : %d.%d.%d\n", apiMajor, apiMinor, apiPatch);
		uint32_t queueFamilyCount = 0;
		VC(instance.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[physicalDeviceIndex], &queueFamilyCount, nullptr));
		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		VC(instance.vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[physicalDeviceIndex], &queueFamilyCount, queueFamilyProperties.data()));
		for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
		{
			const VkQueueFlags queueFlags = queueFamilyProperties[queueFamilyIndex].queueFlags;
			__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "%-21s%c %d =%s%s (%d queues, %d priorities)\n", (queueFamilyIndex == 0 ? "Queue Families" : ""), (queueFamilyIndex == 0 ? ':' : ' '), queueFamilyIndex, (queueFlags & VK_QUEUE_GRAPHICS_BIT) ? " graphics" : "", (queueFlags & VK_QUEUE_TRANSFER_BIT) ? " transfer" : "", queueFamilyProperties[queueFamilyIndex].queueCount, physicalDeviceProperties.limits.discreteQueuePriorities);
		}
		int workQueueFamilyIndex = -1;
		int presentQueueFamilyIndex = -1;
		auto queueFlags = VK_QUEUE_GRAPHICS_BIT;
		for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++)
		{
			if ((queueFamilyProperties[queueFamilyIndex].queueFlags & queueFlags) == queueFlags)
			{
				if ((int) queueFamilyProperties[queueFamilyIndex].queueCount >= queueCount)
				{
					workQueueFamilyIndex = queueFamilyIndex;
				}
			}
			if (workQueueFamilyIndex != -1 && presentQueueFamilyIndex != -1)
			{
				break;
			}
		}
		presentQueueFamilyIndex = workQueueFamilyIndex;
		if (workQueueFamilyIndex == -1)
		{
			__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Required work queue family not supported.\n");
			continue;
		}
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Work Queue Family    : %d\n", workQueueFamilyIndex);
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Present Queue Family : %d\n", presentQueueFamilyIndex);
		uint32_t availableExtensionCount = 0;
		VK(instance.vkEnumerateDeviceExtensionProperties(physicalDevices[physicalDeviceIndex], nullptr, &availableExtensionCount, nullptr));
		std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
		VK(instance.vkEnumerateDeviceExtensionProperties(physicalDevices[physicalDeviceIndex], nullptr, &availableExtensionCount, availableExtensions.data()));
		for (int extensionIdx = 0; extensionIdx < availableExtensionCount; extensionIdx++)
		{
			if (strcmp(availableExtensions[extensionIdx].extensionName, VK_KHR_MULTIVIEW_EXTENSION_NAME) == 0)
			{
				appState.Device.supportsMultiview = true;
			}
			if (strcmp(availableExtensions[extensionIdx].extensionName, VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME) == 0)
			{
				appState.Device.supportsFragmentDensity = true;
			}
		}
		if (instance.validate)
		{
			uint32_t availableLayerCount = 0;
			VK(instance.vkEnumerateDeviceLayerProperties(physicalDevices[physicalDeviceIndex], &availableLayerCount, nullptr));
			std::vector<VkLayerProperties> availableLayers(availableLayerCount);
			VK(instance.vkEnumerateDeviceLayerProperties(physicalDevices[physicalDeviceIndex], &availableLayerCount, availableLayers.data()));
			for (uint32_t i = 0; i < requestedCount; i++)
			{
				for (uint32_t j = 0; j < availableLayerCount; j++)
				{
					if (strcmp(requestedLayers[i], availableLayers[j].layerName) == 0)
					{
						appState.Device.enabledLayerNames[appState.Device.enabledLayerCount++] = requestedLayers[i];
						break;
					}
				}
			}
#if defined(_DEBUG)
			__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Enabled Layers ");
			for (uint32_t i = 0; i < appState.Device.enabledLayerCount; i++)
			{
				__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "\t(%d):%s", i, appState.Device.enabledLayerNames[i]);
			}
#endif
		}
		appState.Device.physicalDevice = physicalDevices[physicalDeviceIndex];
		appState.Device.queueFamilyCount = queueFamilyCount;
		appState.Device.queueFamilyProperties = queueFamilyProperties.data();
		appState.Device.workQueueFamilyIndex = workQueueFamilyIndex;
		appState.Device.presentQueueFamilyIndex = presentQueueFamilyIndex;
		appState.Device.physicalDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		appState.Device.physicalDeviceFeatures.pNext = nullptr;
		appState.Device.physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		appState.Device.physicalDeviceProperties.pNext = nullptr;
		VkPhysicalDeviceMultiviewFeatures deviceMultiviewFeatures { };
		VkPhysicalDeviceMultiviewProperties deviceMultiviewProperties { };
		if (appState.Device.supportsMultiview)
		{
			deviceMultiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
			appState.Device.physicalDeviceFeatures.pNext = &deviceMultiviewFeatures;
			deviceMultiviewProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;
			appState.Device.physicalDeviceProperties.pNext = &deviceMultiviewProperties;
		}
		VC(instance.vkGetPhysicalDeviceFeatures2KHR(physicalDevices[physicalDeviceIndex], &appState.Device.physicalDeviceFeatures));
		VC(instance.vkGetPhysicalDeviceProperties2KHR(physicalDevices[physicalDeviceIndex], &appState.Device.physicalDeviceProperties));
		VC(instance.vkGetPhysicalDeviceMemoryProperties(physicalDevices[physicalDeviceIndex], &appState.Device.physicalDeviceMemoryProperties));
		if (appState.Device.supportsMultiview)
		{
			__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Device %s multiview rendering, with %d views and %u max instances", deviceMultiviewFeatures.multiview ? "supports" : "does not support", deviceMultiviewProperties.maxMultiviewViewCount, deviceMultiviewProperties.maxMultiviewInstanceIndex);
			appState.Device.supportsMultiview = deviceMultiviewFeatures.multiview;
		}
		if (appState.Device.supportsMultiview)
		{
			char *extensionNameToAdd = new char[strlen(VK_KHR_MULTIVIEW_EXTENSION_NAME) + 1];
			strcpy(extensionNameToAdd, VK_KHR_MULTIVIEW_EXTENSION_NAME);
			appState.Device.enabledExtensionNames.push_back(extensionNameToAdd);
		}
		if (appState.Device.supportsFragmentDensity)
		{
			char *extensionNameToAdd = new char[strlen(VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME) + 1];
			strcpy(extensionNameToAdd, VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME);
			appState.Device.enabledExtensionNames.push_back(extensionNameToAdd);
		}
		break;
	}
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "--------------------------------\n");
	if (appState.Device.physicalDevice == VK_NULL_HANDLE)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): No capable Vulkan physical device found.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.queueFamilyUsedQueues.resize(appState.Device.queueFamilyCount);
	for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < appState.Device.queueFamilyCount; queueFamilyIndex++)
	{
		appState.Device.queueFamilyUsedQueues[queueFamilyIndex] = 0xFFFFFFFF << appState.Device.queueFamilyProperties[queueFamilyIndex].queueCount;
	}
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&appState.Device.queueFamilyMutex, &attr);
	const uint32_t discreteQueuePriorities = appState.Device.physicalDeviceProperties.properties.limits.discreteQueuePriorities;
	const float queuePriority = (discreteQueuePriorities <= 2) ? 0.0f : 0.5f;
	VkDeviceQueueCreateInfo deviceQueueCreateInfo[2] { };
	deviceQueueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo[0].queueFamilyIndex = appState.Device.workQueueFamilyIndex;
	deviceQueueCreateInfo[0].queueCount = queueCount;
	deviceQueueCreateInfo[0].pQueuePriorities = &queuePriority;
	deviceQueueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfo[1].queueFamilyIndex = appState.Device.presentQueueFamilyIndex;
	deviceQueueCreateInfo[1].queueCount = 1;
	VkDeviceCreateInfo deviceCreateInfo { };
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	if (appState.Device.presentQueueFamilyIndex == -1 || appState.Device.presentQueueFamilyIndex == appState.Device.workQueueFamilyIndex)
	{
		deviceCreateInfo.queueCreateInfoCount = 1;
	}
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo;
	deviceCreateInfo.enabledLayerCount = appState.Device.enabledLayerCount;
	deviceCreateInfo.ppEnabledLayerNames = (const char *const *) (appState.Device.enabledLayerCount != 0 ? appState.Device.enabledLayerNames : nullptr);
	deviceCreateInfo.enabledExtensionCount = appState.Device.enabledExtensionNames.size();
	deviceCreateInfo.ppEnabledExtensionNames = (const char *const *) (appState.Device.enabledExtensionNames.size() != 0 ? appState.Device.enabledExtensionNames.data() : nullptr);
	VK(instance.vkCreateDevice(appState.Device.physicalDevice, &deviceCreateInfo, nullptr, &appState.Device.device));
	appState.Device.vkDestroyDevice = (PFN_vkDestroyDevice)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyDevice"));
	if (appState.Device.vkDestroyDevice == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyDevice.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkGetDeviceQueue = (PFN_vkGetDeviceQueue)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkGetDeviceQueue"));
	if (appState.Device.vkGetDeviceQueue == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkGetDeviceQueue.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkQueueSubmit = (PFN_vkQueueSubmit)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkQueueSubmit"));
	if (appState.Device.vkQueueSubmit == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkQueueSubmit.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkQueueWaitIdle = (PFN_vkQueueWaitIdle)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkQueueWaitIdle"));
	if (appState.Device.vkQueueWaitIdle == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkQueueWaitIdle.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDeviceWaitIdle"));
	if (appState.Device.vkDeviceWaitIdle == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDeviceWaitIdle.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkAllocateMemory = (PFN_vkAllocateMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkAllocateMemory"));
	if (appState.Device.vkAllocateMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkAllocateMemory.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkFreeMemory = (PFN_vkFreeMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkFreeMemory"));
	if (appState.Device.vkFreeMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkFreeMemory.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkMapMemory = (PFN_vkMapMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkMapMemory"));
	if (appState.Device.vkMapMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkMapMemory.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkUnmapMemory = (PFN_vkUnmapMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkUnmapMemory"));
	if (appState.Device.vkUnmapMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkUnmapMemory.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkFlushMappedMemoryRanges = (PFN_vkFlushMappedMemoryRanges)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkFlushMappedMemoryRanges"));
	if (appState.Device.vkFlushMappedMemoryRanges == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkFlushMappedMemoryRanges.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkBindBufferMemory = (PFN_vkBindBufferMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkBindBufferMemory"));
	if (appState.Device.vkBindBufferMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkBindBufferMemory.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkBindImageMemory = (PFN_vkBindImageMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkBindImageMemory"));
	if (appState.Device.vkBindImageMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkBindImageMemory.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkGetBufferMemoryRequirements"));
	if (appState.Device.vkGetBufferMemoryRequirements == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkGetBufferMemoryRequirements.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkGetImageMemoryRequirements"));
	if (appState.Device.vkGetImageMemoryRequirements == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkGetImageMemoryRequirements.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateFence = (PFN_vkCreateFence)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateFence"));
	if (appState.Device.vkCreateFence == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateFence.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyFence = (PFN_vkDestroyFence)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyFence"));
	if (appState.Device.vkDestroyFence == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyFence.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkResetFences = (PFN_vkResetFences)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkResetFences"));
	if (appState.Device.vkResetFences == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkResetFences.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkGetFenceStatus = (PFN_vkGetFenceStatus)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkGetFenceStatus"));
	if (appState.Device.vkGetFenceStatus == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkGetFenceStatus.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkWaitForFences = (PFN_vkWaitForFences)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkWaitForFences"));
	if (appState.Device.vkWaitForFences == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkWaitForFences.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateBuffer = (PFN_vkCreateBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateBuffer"));
	if (appState.Device.vkCreateBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateBuffer.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyBuffer = (PFN_vkDestroyBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyBuffer"));
	if (appState.Device.vkDestroyBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyBuffer.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateImage = (PFN_vkCreateImage)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateImage"));
	if (appState.Device.vkCreateImage == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateImage.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyImage = (PFN_vkDestroyImage)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyImage"));
	if (appState.Device.vkDestroyImage == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyImage.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateImageView = (PFN_vkCreateImageView)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateImageView"));
	if (appState.Device.vkCreateImageView == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateImageView.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyImageView = (PFN_vkDestroyImageView)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyImageView"));
	if (appState.Device.vkDestroyImageView == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyImageView.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateShaderModule = (PFN_vkCreateShaderModule)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateShaderModule"));
	if (appState.Device.vkCreateShaderModule == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateShaderModule.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyShaderModule = (PFN_vkDestroyShaderModule)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyShaderModule"));
	if (appState.Device.vkDestroyShaderModule == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyShaderModule.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreatePipelineCache = (PFN_vkCreatePipelineCache)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreatePipelineCache"));
	if (appState.Device.vkCreatePipelineCache == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreatePipelineCache.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyPipelineCache"));
	if (appState.Device.vkDestroyPipelineCache == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyPipelineCache.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateGraphicsPipelines"));
	if (appState.Device.vkCreateGraphicsPipelines == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateGraphicsPipelines.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyPipeline = (PFN_vkDestroyPipeline)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyPipeline"));
	if (appState.Device.vkDestroyPipeline == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyPipeline.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreatePipelineLayout"));
	if (appState.Device.vkCreatePipelineLayout == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreatePipelineLayout.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyPipelineLayout"));
	if (appState.Device.vkDestroyPipelineLayout == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyPipelineLayout.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateSampler = (PFN_vkCreateSampler)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateSampler"));
	if (appState.Device.vkCreateSampler == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateSampler.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroySampler = (PFN_vkDestroySampler)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroySampler"));
	if (appState.Device.vkDestroySampler == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroySampler.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateDescriptorSetLayout"));
	if (appState.Device.vkCreateDescriptorSetLayout == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateDescriptorSetLayout.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyDescriptorSetLayout"));
	if (appState.Device.vkDestroyDescriptorSetLayout == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyDescriptorSetLayout.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateDescriptorPool"));
	if (appState.Device.vkCreateDescriptorPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateDescriptorPool.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyDescriptorPool"));
	if (appState.Device.vkDestroyDescriptorPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyDescriptorPool.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkResetDescriptorPool = (PFN_vkResetDescriptorPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkResetDescriptorPool"));
	if (appState.Device.vkResetDescriptorPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkResetDescriptorPool.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkAllocateDescriptorSets"));
	if (appState.Device.vkAllocateDescriptorSets == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkAllocateDescriptorSets.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkFreeDescriptorSets = (PFN_vkFreeDescriptorSets)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkFreeDescriptorSets"));
	if (appState.Device.vkFreeDescriptorSets == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkFreeDescriptorSets.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkUpdateDescriptorSets"));
	if (appState.Device.vkUpdateDescriptorSets == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkUpdateDescriptorSets.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateFramebuffer = (PFN_vkCreateFramebuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateFramebuffer"));
	if (appState.Device.vkCreateFramebuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateFramebuffer.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyFramebuffer"));
	if (appState.Device.vkDestroyFramebuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyFramebuffer.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateRenderPass = (PFN_vkCreateRenderPass)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateRenderPass"));
	if (appState.Device.vkCreateRenderPass == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateRenderPass.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyRenderPass = (PFN_vkDestroyRenderPass)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyRenderPass"));
	if (appState.Device.vkDestroyRenderPass == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyRenderPass.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCreateCommandPool = (PFN_vkCreateCommandPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateCommandPool"));
	if (appState.Device.vkCreateCommandPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCreateCommandPool.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkDestroyCommandPool = (PFN_vkDestroyCommandPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyCommandPool"));
	if (appState.Device.vkDestroyCommandPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkDestroyCommandPool.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkResetCommandPool = (PFN_vkResetCommandPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkResetCommandPool"));
	if (appState.Device.vkResetCommandPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkResetCommandPool.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkAllocateCommandBuffers"));
	if (appState.Device.vkAllocateCommandBuffers == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkAllocateCommandBuffers.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkFreeCommandBuffers"));
	if (appState.Device.vkFreeCommandBuffers == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkFreeCommandBuffers.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkBeginCommandBuffer"));
	if (appState.Device.vkBeginCommandBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkBeginCommandBuffer.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkEndCommandBuffer = (PFN_vkEndCommandBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkEndCommandBuffer"));
	if (appState.Device.vkEndCommandBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkEndCommandBuffer.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkResetCommandBuffer = (PFN_vkResetCommandBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkResetCommandBuffer"));
	if (appState.Device.vkResetCommandBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkResetCommandBuffer.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdBindPipeline = (PFN_vkCmdBindPipeline)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBindPipeline"));
	if (appState.Device.vkCmdBindPipeline == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdBindPipeline.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdSetViewport = (PFN_vkCmdSetViewport)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdSetViewport"));
	if (appState.Device.vkCmdSetViewport == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdSetViewport.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdSetScissor = (PFN_vkCmdSetScissor)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdSetScissor"));
	if (appState.Device.vkCmdSetScissor == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdSetScissor.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBindDescriptorSets"));
	if (appState.Device.vkCmdBindDescriptorSets == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdBindDescriptorSets.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBindIndexBuffer"));
	if (appState.Device.vkCmdBindIndexBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdBindIndexBuffer.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBindVertexBuffers"));
	if (appState.Device.vkCmdBindVertexBuffers == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdBindVertexBuffers.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdBlitImage = (PFN_vkCmdBlitImage)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBlitImage"));
	if (appState.Device.vkCmdBlitImage == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdBlitImage.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdDraw = (PFN_vkCmdDraw)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdDraw"));
	if (appState.Device.vkCmdDraw == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdDraw.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdDrawIndexed"));
	if (appState.Device.vkCmdDrawIndexed == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdDrawIndexed.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdDispatch = (PFN_vkCmdDispatch)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdDispatch"));
	if (appState.Device.vkCmdDispatch == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdDispatch.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdCopyBuffer"));
	if (appState.Device.vkCmdCopyBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdCopyBuffer.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdCopyBufferToImage"));
	if (appState.Device.vkCmdCopyBufferToImage == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdCopyBufferToImage.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdResolveImage = (PFN_vkCmdResolveImage)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdResolveImage"));
	if (appState.Device.vkCmdResolveImage == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdResolveImage.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdPipelineBarrier"));
	if (appState.Device.vkCmdPipelineBarrier == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdPipelineBarrier.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdPushConstants = (PFN_vkCmdPushConstants)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdPushConstants"));
	if (appState.Device.vkCmdPushConstants == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdPushConstants.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBeginRenderPass"));
	if (appState.Device.vkCmdBeginRenderPass == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdBeginRenderPass.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdEndRenderPass"));
	if (appState.Device.vkCmdEndRenderPass == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vkGetDeviceProcAddr() could not find vkCmdEndRenderPass.");
		vrapi_Shutdown();
		exit(0);
	}
	if (pthread_mutex_trylock(&appState.Device.queueFamilyMutex) == EBUSY)
	{
		pthread_mutex_lock(&appState.Device.queueFamilyMutex);
	}
	if ((appState.Device.queueFamilyUsedQueues[appState.Device.workQueueFamilyIndex] & 1) != 0)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): queue 0 in queueFamilyUsedQueues is already in use.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.Device.queueFamilyUsedQueues[appState.Device.workQueueFamilyIndex] |= 1;
	pthread_mutex_unlock(&appState.Device.queueFamilyMutex);
	appState.Context.device = &appState.Device;
	appState.Context.queueFamilyIndex = appState.Device.workQueueFamilyIndex;
	appState.Context.queueIndex = 0;
	VC(appState.Device.vkGetDeviceQueue(appState.Device.device, appState.Context.queueFamilyIndex, appState.Context.queueIndex, &appState.Context.queue));
	VkCommandPoolCreateInfo commandPoolCreateInfo { };
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = appState.Context.queueFamilyIndex;
	VK(appState.Device.vkCreateCommandPool(appState.Device.device, &commandPoolCreateInfo, nullptr, &appState.Context.commandPool));
	VkFenceCreateInfo fenceCreateInfo { };
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkCommandBufferAllocateInfo commandBufferAllocateInfo { };
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = appState.Context.commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	VkCommandBufferBeginInfo commandBufferBeginInfo { };
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VkCommandBuffer setupCommandBuffer;
	VkSubmitInfo setupSubmitInfo { };
	setupSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	setupSubmitInfo.commandBufferCount = 1;
	setupSubmitInfo.pCommandBuffers = &setupCommandBuffer;
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo { };
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK(appState.Device.vkCreatePipelineCache(appState.Device.device, &pipelineCacheCreateInfo, nullptr, &appState.Context.pipelineCache));
	ovrSystemCreateInfoVulkan systemInfo;
	systemInfo.Instance = instance.instance;
	systemInfo.Device = appState.Device.device;
	systemInfo.PhysicalDevice = appState.Device.physicalDevice;
	initResult = vrapi_CreateSystemVulkan(&systemInfo);
	if (initResult != ovrSuccess)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): Failed to create VrApi Vulkan system.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.CpuLevel = CPU_LEVEL;
	appState.GpuLevel = GPU_LEVEL;
	appState.MainThreadTid = gettid();
	auto isMultiview = appState.Device.supportsMultiview;
	auto useFragmentDensity = appState.Device.supportsFragmentDensity;
	appState.Views.resize(isMultiview ? 1 : VRAPI_FRAME_LAYER_EYE_MAX);
	auto eyeTextureWidth = vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
	auto eyeTextureHeight = vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);
	if (eyeTextureWidth > appState.Device.physicalDeviceProperties.properties.limits.maxFramebufferWidth)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): Eye texture width exceeds the physical device's limits.");
		vrapi_Shutdown();
		exit(0);
	}
	if (eyeTextureHeight > appState.Device.physicalDeviceProperties.properties.limits.maxFramebufferHeight)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): Eye texture height exceeds the physical device's limits.");
		vrapi_Shutdown();
		exit(0);
	}
	auto horizontalFOV = vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X);
	auto verticalFOV = vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y);
	appState.FOV = std::max(horizontalFOV, verticalFOV);
	for (auto& view : appState.Views)
	{
		view.colorSwapChain.SwapChain = vrapi_CreateTextureSwapChain3(isMultiview ? VRAPI_TEXTURE_TYPE_2D_ARRAY : VRAPI_TEXTURE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, eyeTextureWidth, eyeTextureHeight, 1, 3);
		view.colorSwapChain.SwapChainLength = vrapi_GetTextureSwapChainLength(view.colorSwapChain.SwapChain);
		view.colorSwapChain.ColorTextures.resize(view.colorSwapChain.SwapChainLength);
		view.colorSwapChain.FragmentDensityTextures.resize(view.colorSwapChain.SwapChainLength);
		view.colorSwapChain.FragmentDensityTextureSizes.resize(view.colorSwapChain.SwapChainLength);
		for (auto i = 0; i < view.colorSwapChain.SwapChainLength; i++)
		{
			view.colorSwapChain.ColorTextures[i] = vrapi_GetTextureSwapChainBufferVulkan(view.colorSwapChain.SwapChain, i);
			if (view.colorSwapChain.FragmentDensityTextures.size() > 0)
			{
				auto result = vrapi_GetTextureSwapChainBufferFoveationVulkan(view.colorSwapChain.SwapChain, i, &view.colorSwapChain.FragmentDensityTextures[i], &view.colorSwapChain.FragmentDensityTextureSizes[i].width, &view.colorSwapChain.FragmentDensityTextureSizes[i].height);
				if (result != ovrSuccess)
				{
					view.colorSwapChain.FragmentDensityTextures.clear();
					view.colorSwapChain.FragmentDensityTextureSizes.clear();
				}
			}
		}
		useFragmentDensity = useFragmentDensity && (view.colorSwapChain.FragmentDensityTextures.size() > 0);
	}
	if ((appState.Device.physicalDeviceProperties.properties.limits.framebufferColorSampleCounts & VK_SAMPLE_COUNT_4_BIT) == 0)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): Requested color buffer sample count not available.");
		vrapi_Shutdown();
		exit(0);
	}
	if ((appState.Device.physicalDeviceProperties.properties.limits.framebufferDepthSampleCounts & VK_SAMPLE_COUNT_4_BIT) == 0)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): Requested depth buffer sample count not available.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.UseFragmentDensity = useFragmentDensity;
	uint32_t attachmentCount = 0;
	VkAttachmentDescription attachments[4] { };
	attachments[attachmentCount].format = VK_FORMAT_R8G8B8A8_UNORM;
	attachments[attachmentCount].samples = VK_SAMPLE_COUNT_4_BIT;
	attachments[attachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[attachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentCount++;
	attachments[attachmentCount].format = VK_FORMAT_R8G8B8A8_UNORM;
	attachments[attachmentCount].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[attachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[attachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentCount++;
	attachments[attachmentCount].format = VK_FORMAT_D24_UNORM_S8_UINT;
	attachments[attachmentCount].samples = VK_SAMPLE_COUNT_4_BIT;
	attachments[attachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[attachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentCount++;
	uint32_t sampleMapAttachment = 0;
	if (appState.UseFragmentDensity)
	{
		sampleMapAttachment = attachmentCount;
		attachments[attachmentCount].format = VK_FORMAT_R8G8_UNORM;
		attachments[attachmentCount].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[attachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[attachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
		attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
		attachmentCount++;
	}
	VkAttachmentReference colorAttachmentReference;
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference resolveAttachmentReference;
	resolveAttachmentReference.attachment = 1;
	resolveAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthAttachmentReference;
	depthAttachmentReference.attachment = 2;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentReference fragmentDensityAttachmentReference;
	if (appState.UseFragmentDensity)
	{
		fragmentDensityAttachmentReference.attachment = sampleMapAttachment;
		fragmentDensityAttachmentReference.layout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
	}
	VkSubpassDescription subpassDescription { };
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.pResolveAttachments = &resolveAttachmentReference;
	subpassDescription.pDepthStencilAttachment = &depthAttachmentReference;
	VkRenderPassCreateInfo renderPassCreateInfo { };
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = attachmentCount;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	VkRenderPassMultiviewCreateInfo multiviewCreateInfo { };
	const uint32_t viewMask = 3;
	if (isMultiview)
	{
		multiviewCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
		multiviewCreateInfo.subpassCount = 1;
		multiviewCreateInfo.pViewMasks = &viewMask;
		multiviewCreateInfo.correlationMaskCount = 1;
		multiviewCreateInfo.pCorrelationMasks = &viewMask;
		renderPassCreateInfo.pNext = &multiviewCreateInfo;
	}
	VkRenderPassFragmentDensityMapCreateInfoEXT fragmentDensityMapCreateInfoEXT;
	if (appState.UseFragmentDensity)
	{
		fragmentDensityMapCreateInfoEXT.sType = VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT;
		fragmentDensityMapCreateInfoEXT.fragmentDensityMapAttachment = fragmentDensityAttachmentReference;
		fragmentDensityMapCreateInfoEXT.pNext = renderPassCreateInfo.pNext;
		renderPassCreateInfo.pNext = &fragmentDensityMapCreateInfoEXT;
	}
	VK(appState.Device.vkCreateRenderPass(appState.Device.device, &renderPassCreateInfo, nullptr, &appState.RenderPass));
	for (auto& view : appState.Views)
	{
		view.framebuffer.colorTextureSwapChain = view.colorSwapChain.SwapChain;
		view.framebuffer.swapChainLength = view.colorSwapChain.SwapChainLength;
		view.framebuffer.colorTextures.resize(view.framebuffer.swapChainLength);
		if (view.colorSwapChain.FragmentDensityTextures.size() > 0)
		{
			view.framebuffer.fragmentDensityTextures.resize(view.framebuffer.swapChainLength);
		}
		view.framebuffer.startBarriers.resize(view.framebuffer.swapChainLength);
		view.framebuffer.endBarriers.resize(view.framebuffer.swapChainLength);
		view.framebuffer.framebuffers.resize(view.framebuffer.swapChainLength);
		view.framebuffer.width = eyeTextureWidth;
		view.framebuffer.height = eyeTextureHeight;
		for (auto i = 0; i < view.framebuffer.swapChainLength; i++)
		{
			auto& texture = view.framebuffer.colorTextures[i];
			texture.width = eyeTextureWidth;
			texture.height = eyeTextureHeight;
			texture.layerCount = isMultiview ? 2 : 1;
			texture.image = view.colorSwapChain.ColorTextures[i];
			VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
			VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
			VkImageMemoryBarrier imageMemoryBarrier { };
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.image = view.colorSwapChain.ColorTextures[i];
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageMemoryBarrier.subresourceRange.levelCount = 1;
			imageMemoryBarrier.subresourceRange.layerCount = texture.layerCount;
			VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
			VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
			VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
			VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
			VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
			VkImageViewCreateInfo imageViewCreateInfo { };
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = texture.image;
			imageViewCreateInfo.viewType = isMultiview ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.layerCount = texture.layerCount;
			VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &texture.view));
			VkSamplerCreateInfo samplerCreateInfo { };
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			samplerCreateInfo.maxLod = 1;
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			VK(appState.Device.vkCreateSampler(appState.Device.device, &samplerCreateInfo, nullptr, &texture.sampler));
			if (view.framebuffer.fragmentDensityTextures.size() > 0)
			{
				auto& texture = view.framebuffer.fragmentDensityTextures[i];
				texture.width = view.colorSwapChain.FragmentDensityTextureSizes[i].width;
				texture.height = view.colorSwapChain.FragmentDensityTextureSizes[i].height;
				texture.layerCount = isMultiview ? 2 : 1;
				texture.image = view.colorSwapChain.FragmentDensityTextures[i];
				VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
				VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
				imageMemoryBarrier.image = view.colorSwapChain.FragmentDensityTextures[i];
				VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
				VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
				VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
				VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
				VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
				VkImageViewCreateInfo imageViewCreateInfoFragmentDensity { };
				imageViewCreateInfoFragmentDensity.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewCreateInfoFragmentDensity.image = texture.image;
				imageViewCreateInfoFragmentDensity.viewType = isMultiview ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
				imageViewCreateInfoFragmentDensity.format = VK_FORMAT_R8G8_UNORM;
				imageViewCreateInfoFragmentDensity.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageViewCreateInfoFragmentDensity.subresourceRange.levelCount = 1;
				imageViewCreateInfoFragmentDensity.subresourceRange.layerCount = texture.layerCount;
				VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfoFragmentDensity, nullptr, &texture.view));
				VkSamplerCreateInfo samplerCreateInfo { };
				samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				samplerCreateInfo.maxLod = 1;
				samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
				VK(appState.Device.vkCreateSampler(appState.Device.device, &samplerCreateInfo, nullptr, &texture.sampler));
			}
			auto& startBarrier = view.framebuffer.startBarriers[i];
			startBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			startBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			startBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			startBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			startBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			startBarrier.image = texture.image;
			startBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			startBarrier.subresourceRange.levelCount = 1;
			startBarrier.subresourceRange.layerCount = texture.layerCount;
			auto& endBarrier = view.framebuffer.endBarriers[i];
			endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			endBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			endBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			endBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			endBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			endBarrier.image = texture.image;
			endBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			endBarrier.subresourceRange.levelCount = 1;
			endBarrier.subresourceRange.layerCount = texture.layerCount;
		}
		VkFormatProperties props;
		VC(instance.vkGetPhysicalDeviceFormatProperties(appState.Device.physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &props));
		if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)
		{
			__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): Color attachment bit in texture is not defined.");
			vrapi_Shutdown();
			exit(0);
		}
		auto& texture = view.framebuffer.renderTexture;
		texture.width = view.framebuffer.width;
		texture.height = view.framebuffer.height;
		texture.layerCount = (isMultiview ? 2 : 1);
		VkImageCreateInfo imageCreateInfo { };
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageCreateInfo.extent.width = texture.width;
		imageCreateInfo.extent.height = texture.height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = texture.layerCount;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_4_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK(appState.Device.vkCreateImage(appState.Device.device, &imageCreateInfo, nullptr, &texture.image));
		VkMemoryRequirements memoryRequirements;
		VC(appState.Device.vkGetImageMemoryRequirements(appState.Device.device, texture.image, &memoryRequirements));
		VkMemoryAllocateInfo memoryAllocateInfo { };
		createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo);
		VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &texture.memory));
		VK(appState.Device.vkBindImageMemory(appState.Device.device, texture.image, texture.memory, 0));
		VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
		VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
		VkImageMemoryBarrier imageMemoryBarrier { };
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier.image = texture.image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.layerCount = texture.layerCount;
		VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
		VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
		VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
		VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
		VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
		VkImageViewCreateInfo imageViewCreateInfo { };
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = texture.image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.layerCount = texture.layerCount;
		VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &texture.view));
		VkSamplerCreateInfo samplerCreateInfo { };
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
		samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.maxLod = 1;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		VK(appState.Device.vkCreateSampler(appState.Device.device, &samplerCreateInfo, nullptr, &texture.sampler));
		VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
		VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.image = texture.image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.layerCount = texture.layerCount;
		VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
		VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
		VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
		VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
		VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
		const auto numLayers = (isMultiview ? 2 : 1);
		imageCreateInfo.format = VK_FORMAT_D24_UNORM_S8_UINT;
		imageCreateInfo.extent.width = view.framebuffer.width;
		imageCreateInfo.extent.height = view.framebuffer.height;
		imageCreateInfo.arrayLayers = numLayers;
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		VK(appState.Device.vkCreateImage(appState.Device.device, &imageCreateInfo, nullptr, &view.framebuffer.depthImage));
		VC(appState.Device.vkGetImageMemoryRequirements(appState.Device.device, view.framebuffer.depthImage, &memoryRequirements));
		createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo);
		VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &view.framebuffer.depthMemory));
		VK(appState.Device.vkBindImageMemory(appState.Device.device, view.framebuffer.depthImage, view.framebuffer.depthMemory, 0));
		imageViewCreateInfo.image = view.framebuffer.depthImage;
		imageViewCreateInfo.viewType = (numLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D);
		imageViewCreateInfo.format = imageCreateInfo.format;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageViewCreateInfo.subresourceRange.layerCount = numLayers;
		VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &view.framebuffer.depthImageView));
		VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
		VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.image = view.framebuffer.depthImage;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageMemoryBarrier.subresourceRange.layerCount = numLayers;
		VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
		VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
		VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
		VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
		VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
		for (auto i = 0; i < view.framebuffer.swapChainLength; i++)
		{
			uint32_t attachmentCount = 0;
			VkImageView attachments[4];
			attachments[attachmentCount++] = view.framebuffer.renderTexture.view;
			attachments[attachmentCount++] = view.framebuffer.colorTextures[i].view;
			attachments[attachmentCount++] = view.framebuffer.depthImageView;
			if (view.framebuffer.fragmentDensityTextures.size() > 0 && appState.UseFragmentDensity)
			{
				attachments[attachmentCount++] = view.framebuffer.fragmentDensityTextures[i].view;
			}
			VkFramebufferCreateInfo framebufferCreateInfo { };
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = appState.RenderPass;
			framebufferCreateInfo.attachmentCount = attachmentCount;
			framebufferCreateInfo.pAttachments = attachments;
			framebufferCreateInfo.width = view.framebuffer.width;
			framebufferCreateInfo.height = view.framebuffer.height;
			framebufferCreateInfo.layers = 1;
			VK(appState.Device.vkCreateFramebuffer(appState.Device.device, &framebufferCreateInfo, nullptr, &view.framebuffer.framebuffers[i]));
		}
		view.perImage.resize(view.framebuffer.swapChainLength);
		for (auto& perImage : view.perImage)
		{
			VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &perImage.commandBuffer));
			VK(appState.Device.vkCreateFence(appState.Device.device, &fenceCreateInfo, nullptr, &perImage.fence));
		}
	}
	app->userData = &appState;
	app->onAppCmd = appHandleCommands;
	while (app->destroyRequested == 0)
	{
		for (;;)
		{
			int events;
			struct android_poll_source *source;
			const int timeoutMilliseconds = (appState.Ovr == nullptr && app->destroyRequested == 0) ? -1 : 0;
			if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void**)&source) < 0)
			{
				break;
			}
			if (source != nullptr)
			{
				source->process(app, source);
			}
			if (appState.Resumed && appState.NativeWindow != nullptr)
			{
				if (appState.Ovr == nullptr)
				{
					ovrModeParmsVulkan parms = vrapi_DefaultModeParmsVulkan(&appState.Java, (unsigned long long)appState.Context.queue);
					parms.ModeParms.Flags &= ~VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;
					parms.ModeParms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
					parms.ModeParms.WindowSurface = (size_t)appState.NativeWindow;
					parms.ModeParms.Display = 0;
					parms.ModeParms.ShareContext = 0;
					appState.Ovr = vrapi_EnterVrMode((ovrModeParms*)&parms);
					if (appState.Ovr == nullptr)
					{
						__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main: vrapi_EnterVRMode() failed: invalid ANativeWindow");
						appState.NativeWindow = nullptr;
					}
					if (appState.Ovr != nullptr)
					{
						vrapi_SetClockLevels(appState.Ovr, appState.CpuLevel, appState.GpuLevel);
						vrapi_SetPerfThread(appState.Ovr, VRAPI_PERF_THREAD_TYPE_MAIN, appState.MainThreadTid);
						vrapi_SetPerfThread(appState.Ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, appState.RenderThreadTid);
					}
				}
			}
			else if (appState.Ovr != nullptr)
			{
				vrapi_LeaveVrMode(appState.Ovr);
				appState.Ovr = nullptr;
			}
		}
		auto leftRemoteFound = false;
		auto rightRemoteFound = false;
		appState.LeftButtons = 0;
		appState.RightButtons = 0;
		appState.LeftJoystick.x = 0;
		appState.LeftJoystick.y = 0;
		appState.RightJoystick.x = 0;
		appState.RightJoystick.y = 0;
		for (auto i = 0; ; i++)
		{
			ovrInputCapabilityHeader cap;
			ovrResult result = vrapi_EnumerateInputDevices(appState.Ovr, i, &cap);
			if (result < 0)
			{
				break;
			}
			if (cap.Type == ovrControllerType_TrackedRemote)
			{
				ovrInputStateTrackedRemote trackedRemoteState;
				trackedRemoteState.Header.ControllerType = ovrControllerType_TrackedRemote;
				result = vrapi_GetCurrentInputState(appState.Ovr, cap.DeviceID, &trackedRemoteState.Header);
				if (result == ovrSuccess)
				{
					result = vrapi_GetInputDeviceCapabilities(appState.Ovr, &cap);
					if (result == ovrSuccess)
					{
						auto trCap = (ovrInputTrackedRemoteCapabilities*) &cap;
						if ((trCap->ControllerCapabilities & ovrControllerCaps_LeftHand) != 0)
						{
							leftRemoteFound = true;
							appState.LeftButtons = trackedRemoteState.Buttons;
							appState.LeftJoystick = trackedRemoteState.Joystick;
						}
						else if ((trCap->ControllerCapabilities & ovrControllerCaps_RightHand) != 0)
						{
							rightRemoteFound = true;
							appState.RightButtons = trackedRemoteState.Buttons;
							appState.RightJoystick = trackedRemoteState.Joystick;
						}
					}
				}
			}
		}
		if (leftRemoteFound && rightRemoteFound)
		{
			if (appState.Mode == AppStartupMode)
			{
				if (appState.StartupButtonsPressed)
				{
					if ((appState.LeftButtons & ovrButton_X) == 0 && (appState.RightButtons & ovrButton_A) == 0)
					{
						appState.Mode = AppWorldMode;
						appState.StartupButtonsPressed = false;
					}
				}
				else if ((appState.LeftButtons & ovrButton_X) != 0 && (appState.RightButtons & ovrButton_A) != 0)
				{
					appState.StartupButtonsPressed = true;
				}
			}
			else if (appState.Mode == AppScreenMode)
			{
				if (host_initialized)
				{
					if (leftButtonIsDown(appState, ovrButton_Enter))
					{
						Key_Event(K_ESCAPE, true);
					}
					if (leftButtonIsUp(appState, ovrButton_Enter))
					{
						Key_Event(K_ESCAPE, false);
					}
					if (key_dest == key_game)
					{
						if (joy_initialized)
						{
							joy_avail = true;
							pdwRawValue[JOY_AXIS_X] = -appState.LeftJoystick.x;
							pdwRawValue[JOY_AXIS_Y] = -appState.LeftJoystick.y;
							pdwRawValue[JOY_AXIS_Z] = appState.RightJoystick.x;
							pdwRawValue[JOY_AXIS_R] = appState.RightJoystick.y;
						}
						if (leftButtonIsDown(appState, ovrButton_Trigger) || rightButtonIsDown(appState, ovrButton_Trigger))
						{
							Cmd_ExecuteString("+attack", src_command);
						}
						if (leftButtonIsUp(appState, ovrButton_Trigger) || rightButtonIsUp(appState, ovrButton_Trigger))
						{
							Cmd_ExecuteString("-attack", src_command);
						}
						if (leftButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_GripTrigger))
						{
							Cmd_ExecuteString("+speed", src_command);
						}
						if (leftButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_GripTrigger))
						{
							Cmd_ExecuteString("-speed", src_command);
						}
						if (leftButtonIsDown(appState, ovrButton_Joystick))
						{
							Cmd_ExecuteString("impulse 10", src_command);
						}
						if (m_state == m_quit)
						{
							if (rightButtonIsDown(appState, ovrButton_B))
							{
								Cmd_ExecuteString("+jump", src_command);
							}
							if (rightButtonIsUp(appState, ovrButton_B))
							{
								Cmd_ExecuteString("-jump", src_command);
							}
							if (leftButtonIsDown(appState, ovrButton_Y))
							{
								Key_Event('y', true);
							}
							if (leftButtonIsUp(appState, ovrButton_Y))
							{
								Key_Event('y', false);
							}
						}
						else
						{
							if (leftButtonIsDown(appState, ovrButton_Y) || rightButtonIsDown(appState, ovrButton_B))
							{
								Cmd_ExecuteString("+jump", src_command);
							}
							if (leftButtonIsUp(appState, ovrButton_Y) || rightButtonIsUp(appState, ovrButton_B))
							{
								Cmd_ExecuteString("-jump", src_command);
							}
						}
						if (leftButtonIsDown(appState, ovrButton_X) || rightButtonIsDown(appState, ovrButton_A))
						{
							Cmd_ExecuteString("+movedown", src_command);
						}
						if (leftButtonIsUp(appState, ovrButton_X) || rightButtonIsUp(appState, ovrButton_A))
						{
							Cmd_ExecuteString("-movedown", src_command);
						}
						if (rightButtonIsDown(appState, ovrButton_Joystick))
						{
							Cmd_ExecuteString("+mlook", src_command);
						}
						if (rightButtonIsUp(appState, ovrButton_Joystick))
						{
							Cmd_ExecuteString("-mlook", src_command);
						}
					}
					else
					{
						if ((appState.LeftJoystick.y > 0.5 && appState.PreviousLeftJoystick.y <= 0.5) || (appState.RightJoystick.y > 0.5 && appState.PreviousRightJoystick.y <= 0.5))
						{
							Key_Event(K_UPARROW, true);
						}
						if ((appState.LeftJoystick.y <= 0.5 && appState.PreviousLeftJoystick.y > 0.5) || (appState.RightJoystick.y <= 0.5 && appState.PreviousRightJoystick.y > 0.5))
						{
							Key_Event(K_UPARROW, false);
						}
						if ((appState.LeftJoystick.y < -0.5 && appState.PreviousLeftJoystick.y >= -0.5) || (appState.RightJoystick.y < -0.5 && appState.PreviousRightJoystick.y >= -0.5))
						{
							Key_Event(K_DOWNARROW, true);
						}
						if ((appState.LeftJoystick.y >= -0.5 && appState.PreviousLeftJoystick.y < -0.5) || (appState.RightJoystick.y >= -0.5 && appState.PreviousRightJoystick.y < -0.5))
						{
							Key_Event(K_DOWNARROW, false);
						}
						if ((appState.LeftJoystick.x > 0.5 && appState.PreviousLeftJoystick.x <= 0.5) || (appState.RightJoystick.x > 0.5 && appState.PreviousRightJoystick.x <= 0.5))
						{
							Key_Event(K_RIGHTARROW, true);
						}
						if ((appState.LeftJoystick.x <= 0.5 && appState.PreviousLeftJoystick.x > 0.5) || (appState.RightJoystick.x <= 0.5 && appState.PreviousRightJoystick.x > 0.5))
						{
							Key_Event(K_RIGHTARROW, false);
						}
						if ((appState.LeftJoystick.x < -0.5 && appState.PreviousLeftJoystick.x >= -0.5) || (appState.RightJoystick.x < -0.5 && appState.PreviousRightJoystick.x >= -0.5))
						{
							Key_Event(K_LEFTARROW, true);
						}
						if ((appState.LeftJoystick.x >= -0.5 && appState.PreviousLeftJoystick.x < -0.5) || (appState.RightJoystick.x >= -0.5 && appState.PreviousRightJoystick.x < -0.5))
						{
							Key_Event(K_LEFTARROW, false);
						}
						if (leftButtonIsDown(appState, ovrButton_Trigger) || leftButtonIsDown(appState, ovrButton_X) || rightButtonIsDown(appState, ovrButton_Trigger) || rightButtonIsDown(appState, ovrButton_A))
						{
							Key_Event(K_ENTER, true);
						}
						if (leftButtonIsUp(appState, ovrButton_Trigger) || leftButtonIsUp(appState, ovrButton_X) || rightButtonIsUp(appState, ovrButton_Trigger) || rightButtonIsUp(appState, ovrButton_A))
						{
							Key_Event(K_ENTER, false);
						}
						if (leftButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_B))
						{
							Key_Event(K_ESCAPE, true);
						}
						if (leftButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_B))
						{
							Key_Event(K_ESCAPE, false);
						}
						if (m_state == m_quit)
						{
							if (leftButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_B))
							{
								Key_Event(K_ESCAPE, true);
							}
							if (leftButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_B))
							{
								Key_Event(K_ESCAPE, false);
							}
							if (leftButtonIsDown(appState, ovrButton_Y))
							{
								Key_Event('y', true);
							}
							if (leftButtonIsUp(appState, ovrButton_Y))
							{
								Key_Event('y', false);
							}
						}
						else
						{
							if (leftButtonIsDown(appState, ovrButton_GripTrigger) || leftButtonIsDown(appState, ovrButton_Y) || rightButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_B))
							{
								Key_Event(K_ESCAPE, true);
							}
							if (leftButtonIsUp(appState, ovrButton_GripTrigger) || leftButtonIsUp(appState, ovrButton_Y) || rightButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_B))
							{
								Key_Event(K_ESCAPE, false);
							}
						}
					}
				}
			}
			else if (appState.Mode == AppWorldMode)
			{
				if (host_initialized)
				{
					if (leftButtonIsDown(appState, ovrButton_Enter))
					{
						Key_Event(K_ESCAPE, true);
					}
					if (leftButtonIsUp(appState, ovrButton_Enter))
					{
						Key_Event(K_ESCAPE, false);
					}
					if (key_dest == key_game)
					{
						if (joy_initialized)
						{
							joy_avail = true;
							pdwRawValue[JOY_AXIS_X] = -appState.LeftJoystick.x - appState.RightJoystick.x;
							pdwRawValue[JOY_AXIS_Y] = -appState.LeftJoystick.y - appState.RightJoystick.y;
						}
						if (leftButtonIsDown(appState, ovrButton_Trigger) || rightButtonIsDown(appState, ovrButton_Trigger))
						{
							if (!appState.ControlsMessageClosed && appState.ControlsMessageDisplayed && (appState.NearViewModel || d_lists.last_viewmodel < 0))
							{
								SCR_Interrupt();
								appState.ControlsMessageClosed = true;
							}
							else if (appState.NearViewModel || d_lists.last_viewmodel < 0)
							{
								Cmd_ExecuteString("+attack", src_command);
							}
						}
						if (leftButtonIsUp(appState, ovrButton_Trigger) || rightButtonIsUp(appState, ovrButton_Trigger))
						{
							Cmd_ExecuteString("-attack", src_command);
						}
						if (leftButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_GripTrigger))
						{
							Cmd_ExecuteString("+speed", src_command);
						}
						if (leftButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_GripTrigger))
						{
							Cmd_ExecuteString("-speed", src_command);
						}
						if (leftButtonIsDown(appState, ovrButton_Joystick) || rightButtonIsDown(appState, ovrButton_Joystick))
						{
							Cmd_ExecuteString("impulse 10", src_command);
						}
						if (m_state == m_quit)
						{
							if (rightButtonIsDown(appState, ovrButton_B))
							{
								Cmd_ExecuteString("+jump", src_command);
							}
							if (rightButtonIsUp(appState, ovrButton_B))
							{
								Cmd_ExecuteString("-jump", src_command);
							}
							if (leftButtonIsDown(appState, ovrButton_Y))
							{
								Key_Event('y', true);
							}
							if (leftButtonIsUp(appState, ovrButton_Y))
							{
								Key_Event('y', false);
							}
						}
						else
						{
							if (leftButtonIsDown(appState, ovrButton_Y) || rightButtonIsDown(appState, ovrButton_B))
							{
								Cmd_ExecuteString("+jump", src_command);
							}
							if (leftButtonIsUp(appState, ovrButton_Y) || rightButtonIsUp(appState, ovrButton_B))
							{
								Cmd_ExecuteString("-jump", src_command);
							}
						}
						if (leftButtonIsDown(appState, ovrButton_X) || rightButtonIsDown(appState, ovrButton_A))
						{
							Cmd_ExecuteString("+movedown", src_command);
						}
						if (leftButtonIsUp(appState, ovrButton_X) || rightButtonIsUp(appState, ovrButton_A))
						{
							Cmd_ExecuteString("-movedown", src_command);
						}
					}
					else
					{
						if ((appState.LeftJoystick.y > 0.5 && appState.PreviousLeftJoystick.y <= 0.5) || (appState.RightJoystick.y > 0.5 && appState.PreviousRightJoystick.y <= 0.5))
						{
							Key_Event(K_UPARROW, true);
						}
						if ((appState.LeftJoystick.y <= 0.5 && appState.PreviousLeftJoystick.y > 0.5) || (appState.RightJoystick.y <= 0.5 && appState.PreviousRightJoystick.y > 0.5))
						{
							Key_Event(K_UPARROW, false);
						}
						if ((appState.LeftJoystick.y < -0.5 && appState.PreviousLeftJoystick.y >= -0.5) || (appState.RightJoystick.y < -0.5 && appState.PreviousRightJoystick.y >= -0.5))
						{
							Key_Event(K_DOWNARROW, true);
						}
						if ((appState.LeftJoystick.y >= -0.5 && appState.PreviousLeftJoystick.y < -0.5) || (appState.RightJoystick.y >= -0.5 && appState.PreviousRightJoystick.y < -0.5))
						{
							Key_Event(K_DOWNARROW, false);
						}
						if ((appState.LeftJoystick.x > 0.5 && appState.PreviousLeftJoystick.x <= 0.5) || (appState.RightJoystick.x > 0.5 && appState.PreviousRightJoystick.x <= 0.5))
						{
							Key_Event(K_RIGHTARROW, true);
						}
						if ((appState.LeftJoystick.x <= 0.5 && appState.PreviousLeftJoystick.x > 0.5) || (appState.RightJoystick.x <= 0.5 && appState.PreviousRightJoystick.x > 0.5))
						{
							Key_Event(K_RIGHTARROW, false);
						}
						if ((appState.LeftJoystick.x < -0.5 && appState.PreviousLeftJoystick.x >= -0.5) || (appState.RightJoystick.x < -0.5 && appState.PreviousRightJoystick.x >= -0.5))
						{
							Key_Event(K_LEFTARROW, true);
						}
						if ((appState.LeftJoystick.x >= -0.5 && appState.PreviousLeftJoystick.x < -0.5) || (appState.RightJoystick.x >= -0.5 && appState.PreviousRightJoystick.x < -0.5))
						{
							Key_Event(K_LEFTARROW, false);
						}
						if (leftButtonIsDown(appState, ovrButton_Trigger) || leftButtonIsDown(appState, ovrButton_X) || rightButtonIsDown(appState, ovrButton_Trigger) || rightButtonIsDown(appState, ovrButton_A))
						{
							Key_Event(K_ENTER, true);
						}
						if (leftButtonIsUp(appState, ovrButton_Trigger) || leftButtonIsUp(appState, ovrButton_X) || rightButtonIsUp(appState, ovrButton_Trigger) || rightButtonIsUp(appState, ovrButton_A))
						{
							Key_Event(K_ENTER, false);
						}
						if (m_state == m_quit)
						{
							if (leftButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_B))
							{
								Key_Event(K_ESCAPE, true);
							}
							if (leftButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_B))
							{
								Key_Event(K_ESCAPE, false);
							}
							if (leftButtonIsDown(appState, ovrButton_Y))
							{
								Key_Event('y', true);
							}
							if (leftButtonIsUp(appState, ovrButton_Y))
							{
								Key_Event('y', false);
							}
						}
						else
						{
							if (leftButtonIsDown(appState, ovrButton_GripTrigger) || leftButtonIsDown(appState, ovrButton_Y) || rightButtonIsDown(appState, ovrButton_GripTrigger) || rightButtonIsDown(appState, ovrButton_B))
							{
								Key_Event(K_ESCAPE, true);
							}
							if (leftButtonIsUp(appState, ovrButton_GripTrigger) || leftButtonIsUp(appState, ovrButton_Y) || rightButtonIsUp(appState, ovrButton_GripTrigger) || rightButtonIsUp(appState, ovrButton_B))
							{
								Key_Event(K_ESCAPE, false);
							}
						}
					}
				}
			}
		}
		else
		{
			joy_avail = false;
		}
		if (leftRemoteFound)
		{
			appState.PreviousLeftButtons = appState.LeftButtons;
			appState.PreviousLeftJoystick = appState.LeftJoystick;
		}
		else
		{
			appState.PreviousLeftButtons = 0;
			appState.PreviousLeftJoystick.x = 0;
			appState.PreviousLeftJoystick.y = 0;
		}
		if (rightRemoteFound)
		{
			appState.PreviousRightButtons = appState.RightButtons;
			appState.PreviousRightJoystick = appState.RightJoystick;
		}
		else
		{
			appState.PreviousRightButtons = 0;
			appState.PreviousRightJoystick.x = 0;
			appState.PreviousRightJoystick.y = 0;
		}
		if (appState.Ovr == nullptr)
		{
			continue;
		}
		if (PermissionsGrantStatus == PermissionsDenied)
		{
			ANativeActivity_finish(app->activity);
		}
		if (!appState.Scene.createdScene)
		{
			int frameFlags = 0;
			frameFlags |= VRAPI_FRAME_FLAG_FLUSH;
			ovrLayerProjection2 blackLayer = vrapi_DefaultLayerBlackProjection2();
			blackLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
			ovrLayerLoadingIcon2 iconLayer = vrapi_DefaultLayerLoadingIcon2();
			iconLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
			const ovrLayerHeader2* layers[] { &blackLayer.Header, &iconLayer.Header };
			ovrSubmitFrameDescription2 frameDesc { };
			frameDesc.Flags = frameFlags;
			frameDesc.SwapInterval = 1;
			frameDesc.FrameIndex = appState.FrameIndex;
			frameDesc.DisplayTime = appState.DisplayTime;
			frameDesc.LayerCount = 2;
			frameDesc.Layers = layers;
			vrapi_SubmitFrame2(appState.Ovr, &frameDesc);
			appState.ScreenWidth = 960;
			appState.ScreenHeight = 600;
			appState.ConsoleWidth = 320;
			appState.ConsoleHeight = 200;
			appState.Console.SwapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, appState.ScreenWidth, appState.ScreenHeight, 1, 3);
			appState.Console.View.colorSwapChain.SwapChain = appState.Console.SwapChain;
			appState.Console.View.colorSwapChain.SwapChainLength = vrapi_GetTextureSwapChainLength(appState.Console.View.colorSwapChain.SwapChain);
			appState.Console.View.colorSwapChain.ColorTextures.resize(appState.Console.View.colorSwapChain.SwapChainLength);
			for (auto i = 0; i < appState.Console.View.colorSwapChain.SwapChainLength; i++)
			{
				appState.Console.View.colorSwapChain.ColorTextures[i] = vrapi_GetTextureSwapChainBufferVulkan(appState.Console.View.colorSwapChain.SwapChain, i);
			}
			uint32_t attachmentCount = 0;
			VkAttachmentDescription attachments[2] { };
			attachments[attachmentCount].format = VK_FORMAT_R8G8B8A8_UNORM;
			attachments[attachmentCount].samples = VK_SAMPLE_COUNT_4_BIT;
			attachments[attachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[attachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentCount++;
			attachments[attachmentCount].format = VK_FORMAT_R8G8B8A8_UNORM;
			attachments[attachmentCount].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[attachmentCount].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[attachmentCount].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[attachmentCount].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[attachmentCount].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[attachmentCount].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachments[attachmentCount].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentCount++;
			VkAttachmentReference colorAttachmentReference;
			colorAttachmentReference.attachment = 0;
			colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkAttachmentReference resolveAttachmentReference;
			resolveAttachmentReference.attachment = 1;
			resolveAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkSubpassDescription subpassDescription { };
			subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescription.colorAttachmentCount = 1;
			subpassDescription.pColorAttachments = &colorAttachmentReference;
			subpassDescription.pResolveAttachments = &resolveAttachmentReference;
			VkRenderPassCreateInfo renderPassCreateInfo { };
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = attachmentCount;
			renderPassCreateInfo.pAttachments = attachments;
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpassDescription;
			VK(appState.Device.vkCreateRenderPass(appState.Device.device, &renderPassCreateInfo, nullptr, &appState.Console.RenderPass));
			appState.Console.View.framebuffer.colorTextureSwapChain = appState.Console.View.colorSwapChain.SwapChain;
			appState.Console.View.framebuffer.swapChainLength = appState.Console.View.colorSwapChain.SwapChainLength;
			appState.Console.View.framebuffer.colorTextures.resize(appState.Console.View.framebuffer.swapChainLength);
			appState.Console.View.framebuffer.startBarriers.resize(appState.Console.View.framebuffer.swapChainLength);
			appState.Console.View.framebuffer.endBarriers.resize(appState.Console.View.framebuffer.swapChainLength);
			appState.Console.View.framebuffer.framebuffers.resize(appState.Console.View.framebuffer.swapChainLength);
			appState.Console.View.framebuffer.width = appState.ScreenWidth;
			appState.Console.View.framebuffer.height = appState.ScreenHeight;
			for (auto i = 0; i < appState.Console.View.framebuffer.swapChainLength; i++)
			{
				auto& texture = appState.Console.View.framebuffer.colorTextures[i];
				texture.width = appState.ScreenWidth;
				texture.height = appState.ScreenHeight;
				texture.layerCount = 1;
				texture.image = appState.Console.View.colorSwapChain.ColorTextures[i];
				VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
				VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
				VkImageMemoryBarrier imageMemoryBarrier { };
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageMemoryBarrier.image = appState.Console.View.colorSwapChain.ColorTextures[i];
				imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageMemoryBarrier.subresourceRange.levelCount = 1;
				imageMemoryBarrier.subresourceRange.layerCount = texture.layerCount;
				VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
				VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
				VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
				VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
				VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
				VkImageViewCreateInfo imageViewCreateInfo { };
				imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				imageViewCreateInfo.image = texture.image;
				imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
				imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageViewCreateInfo.subresourceRange.levelCount = 1;
				imageViewCreateInfo.subresourceRange.layerCount = texture.layerCount;
				VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &texture.view));
				VkSamplerCreateInfo samplerCreateInfo { };
				samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
				samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
				samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
				samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				samplerCreateInfo.maxLod = 1;
				samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
				VK(appState.Device.vkCreateSampler(appState.Device.device, &samplerCreateInfo, nullptr, &texture.sampler));
				auto& startBarrier = appState.Console.View.framebuffer.startBarriers[i];
				startBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				startBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				startBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				startBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				startBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				startBarrier.image = texture.image;
				startBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				startBarrier.subresourceRange.levelCount = 1;
				startBarrier.subresourceRange.layerCount = texture.layerCount;
				auto& endBarrier = appState.Console.View.framebuffer.endBarriers[i];
				endBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				endBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				endBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				endBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				endBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				endBarrier.image = texture.image;
				endBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				endBarrier.subresourceRange.levelCount = 1;
				endBarrier.subresourceRange.layerCount = texture.layerCount;
			}
			VkFormatProperties props;
			VC(instance.vkGetPhysicalDeviceFormatProperties(appState.Device.physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &props));
			if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)
			{
				__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): Color attachment bit in texture is not defined.");
				vrapi_Shutdown();
				exit(0);
			}
			auto& texture = appState.Console.View.framebuffer.renderTexture;
			texture.width = appState.Console.View.framebuffer.width;
			texture.height = appState.Console.View.framebuffer.height;
			texture.layerCount = 1;
			VkImageCreateInfo imageCreateInfo { };
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			imageCreateInfo.extent.width = texture.width;
			imageCreateInfo.extent.height = texture.height;
			imageCreateInfo.extent.depth = 1;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = texture.layerCount;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_4_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VK(appState.Device.vkCreateImage(appState.Device.device, &imageCreateInfo, nullptr, &texture.image));
			VkMemoryRequirements memoryRequirements;
			VC(appState.Device.vkGetImageMemoryRequirements(appState.Device.device, texture.image, &memoryRequirements));
			VkMemoryAllocateInfo memoryAllocateInfo { };
			createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo);
			VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &texture.memory));
			VK(appState.Device.vkBindImageMemory(appState.Device.device, texture.image, texture.memory, 0));
			VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
			VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
			VkImageMemoryBarrier imageMemoryBarrier { };
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.image = texture.image;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageMemoryBarrier.subresourceRange.levelCount = 1;
			imageMemoryBarrier.subresourceRange.layerCount = texture.layerCount;
			VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
			VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
			VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
			VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
			VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
			VkImageViewCreateInfo imageViewCreateInfo { };
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = texture.image;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			imageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.layerCount = texture.layerCount;
			VK(appState.Device.vkCreateImageView(appState.Device.device, &imageViewCreateInfo, nullptr, &texture.view));
			VkSamplerCreateInfo samplerCreateInfo { };
			samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
			samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerCreateInfo.maxLod = 1;
			samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			VK(appState.Device.vkCreateSampler(appState.Device.device, &samplerCreateInfo, nullptr, &texture.sampler));
			VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
			VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			imageMemoryBarrier.image = texture.image;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageMemoryBarrier.subresourceRange.levelCount = 1;
			imageMemoryBarrier.subresourceRange.layerCount = texture.layerCount;
			VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
			VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
			VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
			VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
			VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
			for (auto i = 0; i < appState.Console.View.framebuffer.swapChainLength; i++)
			{
				uint32_t attachmentCount = 0;
				VkImageView attachments[2];
				attachments[attachmentCount++] = appState.Console.View.framebuffer.renderTexture.view;
				attachments[attachmentCount++] = appState.Console.View.framebuffer.colorTextures[i].view;
				VkFramebufferCreateInfo framebufferCreateInfo { };
				framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				framebufferCreateInfo.renderPass = appState.Console.RenderPass;
				framebufferCreateInfo.attachmentCount = attachmentCount;
				framebufferCreateInfo.pAttachments = attachments;
				framebufferCreateInfo.width = appState.Console.View.framebuffer.width;
				framebufferCreateInfo.height = appState.Console.View.framebuffer.height;
				framebufferCreateInfo.layers = 1;
				VK(appState.Device.vkCreateFramebuffer(appState.Device.device, &framebufferCreateInfo, nullptr, &appState.Console.View.framebuffer.framebuffers[i]));
			}
			appState.Console.View.perImage.resize(appState.Console.View.framebuffer.swapChainLength);
			for (auto& perImage : appState.Console.View.perImage)
			{
				VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &perImage.commandBuffer));
				VK(appState.Device.vkCreateFence(appState.Device.device, &fenceCreateInfo, nullptr, &perImage.fence));
			}
			appState.Screen.SwapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, appState.ScreenWidth, appState.ScreenHeight, 1, 1);
			appState.Screen.Data.resize(appState.ScreenWidth * appState.ScreenHeight, 255 << 24);
			appState.Screen.Image = vrapi_GetTextureSwapChainBufferVulkan(appState.Screen.SwapChain, 0);
			auto imageFile = AAssetManager_open(app->activity->assetManager, "play.png", AASSET_MODE_BUFFER);
			auto imageFileLength = AAsset_getLength(imageFile);
			std::vector<stbi_uc> imageSource(imageFileLength);
			AAsset_read(imageFile, imageSource.data(), imageFileLength);
			int imageWidth;
			int imageHeight;
			int imageComponents;
			auto image = stbi_load_from_memory(imageSource.data(), imageFileLength, &imageWidth, &imageHeight, &imageComponents, 4);
			auto texIndex = ((appState.ScreenHeight - imageHeight) * appState.ScreenWidth + appState.ScreenWidth - imageWidth) / 2;
			auto index = 0;
			for (auto y = 0; y < imageHeight; y++)
			{
				for (auto x = 0; x < imageWidth; x++)
				{
					auto r = image[index];
					index++;
					auto g = image[index];
					index++;
					auto b = image[index];
					index++;
					auto a = image[index];
					index++;
					auto factor = (double)a / 255;
					r = (unsigned char)((double)r * factor);
					g = (unsigned char)((double)g * factor);
					b = (unsigned char)((double)b * factor);
					appState.Screen.Data[texIndex] = ((uint32_t)255 << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
					texIndex++;
				}
				texIndex += appState.ScreenWidth - imageWidth;
			}
			stbi_image_free(image);
			for (auto b = 0; b < 5; b++)
			{
				auto i = (unsigned char)(192.0 * sin(M_PI / (double)(b - 1)));
				auto color = ((uint32_t)255 << 24) | ((uint32_t)i << 16) | ((uint32_t)i << 8) | i;
				auto texTopIndex = b * appState.ScreenWidth + b;
				auto texBottomIndex = (appState.ScreenHeight - 1 - b) * appState.ScreenWidth + b;
				for (auto x = 0; x < appState.ScreenWidth - b - b; x++)
				{
					appState.Screen.Data[texTopIndex] = color;
					texTopIndex++;
					appState.Screen.Data[texBottomIndex] = color;
					texBottomIndex++;
				}
				auto texLeftIndex = (b + 1) * appState.ScreenWidth + b;
				auto texRightIndex = (b + 1) * appState.ScreenWidth + appState.ScreenWidth - 1 - b;
				for (auto y = 0; y < appState.ScreenHeight - b - 1 - b - 1; y++)
				{
					appState.Screen.Data[texLeftIndex] = color;
					texLeftIndex += appState.ScreenWidth;
					appState.Screen.Data[texRightIndex] = color;
					texRightIndex += appState.ScreenWidth;
				}
			}
			appState.Screen.Buffer.size = appState.Screen.Data.size() * sizeof(uint32_t);
			VkBufferCreateInfo bufferCreateInfo { };
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = appState.Screen.Buffer.size;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			VK(appState.Device.vkCreateBuffer(appState.Device.device, &bufferCreateInfo, nullptr, &appState.Screen.Buffer.buffer));
			appState.Screen.Buffer.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			VC(appState.Device.vkGetBufferMemoryRequirements(appState.Device.device, appState.Screen.Buffer.buffer, &memoryRequirements));
			createMemoryAllocateInfo(appState, memoryRequirements, appState.Screen.Buffer.flags, memoryAllocateInfo);
			VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &appState.Screen.Buffer.memory));
			VK(appState.Device.vkBindBufferMemory(appState.Device.device, appState.Screen.Buffer.buffer, appState.Screen.Buffer.memory, 0));
			VK(appState.Device.vkMapMemory(appState.Device.device, appState.Screen.Buffer.memory, 0, memoryRequirements.size, 0, &appState.Screen.Buffer.mapped));
			memcpy(appState.Screen.Buffer.mapped, appState.Screen.Data.data(), appState.Screen.Buffer.size);
			VC(appState.Device.vkUnmapMemory(appState.Device.device, appState.Screen.Buffer.memory));
			appState.Screen.Buffer.mapped = nullptr;
			VkMappedMemoryRange mappedMemoryRange { };
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedMemoryRange.memory = appState.Screen.Buffer.memory;
			VC(appState.Device.vkFlushMappedMemoryRanges(appState.Device.device, 1, &mappedMemoryRange));
			VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
			VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
			VkBufferImageCopy region { };
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.width = appState.ScreenWidth;
			region.imageExtent.height = appState.ScreenHeight;
			region.imageExtent.depth = 1;
			VC(appState.Device.vkCmdCopyBufferToImage(setupCommandBuffer, appState.Screen.Buffer.buffer, appState.Screen.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
			VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
			VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
			VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
			VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
			VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &appState.Screen.CommandBuffer));
			appState.Screen.SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			appState.Screen.SubmitInfo.commandBufferCount = 1;
			appState.Screen.SubmitInfo.pCommandBuffers = &appState.Screen.CommandBuffer;
			appState.LeftArrows.SwapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, appState.ScreenWidth, appState.ScreenHeight, 1, 1);
			appState.LeftArrows.Data.resize(appState.ScreenWidth * appState.ScreenHeight, 255 << 24);
			appState.LeftArrows.Image = vrapi_GetTextureSwapChainBufferVulkan(appState.LeftArrows.SwapChain, 0);
			imageFile = AAssetManager_open(app->activity->assetManager, "leftarrows.png", AASSET_MODE_BUFFER);
			imageFileLength = AAsset_getLength(imageFile);
			imageSource.resize(imageFileLength);
			AAsset_read(imageFile, imageSource.data(), imageFileLength);
			image = stbi_load_from_memory(imageSource.data(), imageFileLength, &imageWidth, &imageHeight, &imageComponents, 4);
			texIndex = ((appState.ScreenHeight - imageHeight) * appState.ScreenWidth + appState.ScreenWidth - imageWidth) / 2;
			index = 0;
			for (auto y = 0; y < imageHeight; y++)
			{
				for (auto x = 0; x < imageWidth; x++)
				{
					auto r = image[index];
					index++;
					auto g = image[index];
					index++;
					auto b = image[index];
					index++;
					auto a = image[index];
					index++;
					auto factor = (double)a / 255;
					r = (unsigned char)((double)r * factor);
					g = (unsigned char)((double)g * factor);
					b = (unsigned char)((double)b * factor);
					appState.LeftArrows.Data[texIndex] = ((uint32_t)255 << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
					texIndex++;
				}
				texIndex += appState.ScreenWidth - imageWidth;
			}
			stbi_image_free(image);
			appState.LeftArrows.Buffer.size = appState.LeftArrows.Data.size() * sizeof(uint32_t);
			bufferCreateInfo.size = appState.LeftArrows.Buffer.size;
			VK(appState.Device.vkCreateBuffer(appState.Device.device, &bufferCreateInfo, nullptr, &appState.LeftArrows.Buffer.buffer));
			appState.LeftArrows.Buffer.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			VC(appState.Device.vkGetBufferMemoryRequirements(appState.Device.device, appState.LeftArrows.Buffer.buffer, &memoryRequirements));
			createMemoryAllocateInfo(appState, memoryRequirements, appState.LeftArrows.Buffer.flags, memoryAllocateInfo);
			VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &appState.LeftArrows.Buffer.memory));
			VK(appState.Device.vkBindBufferMemory(appState.Device.device, appState.LeftArrows.Buffer.buffer, appState.LeftArrows.Buffer.memory, 0));
			VK(appState.Device.vkMapMemory(appState.Device.device, appState.LeftArrows.Buffer.memory, 0, memoryRequirements.size, 0, &appState.LeftArrows.Buffer.mapped));
			memcpy(appState.LeftArrows.Buffer.mapped, appState.LeftArrows.Data.data(), appState.LeftArrows.Buffer.size);
			VC(appState.Device.vkUnmapMemory(appState.Device.device, appState.LeftArrows.Buffer.memory));
			appState.LeftArrows.Buffer.mapped = nullptr;
			mappedMemoryRange.memory = appState.LeftArrows.Buffer.memory;
			VC(appState.Device.vkFlushMappedMemoryRanges(appState.Device.device, 1, &mappedMemoryRange));
			VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
			VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
			VC(appState.Device.vkCmdCopyBufferToImage(setupCommandBuffer, appState.LeftArrows.Buffer.buffer, appState.LeftArrows.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
			VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
			VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
			VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
			VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
			appState.RightArrows.SwapChain = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, appState.ScreenWidth, appState.ScreenHeight, 1, 1);
			appState.RightArrows.Data.resize(appState.ScreenWidth * appState.ScreenHeight, 255 << 24);
			appState.RightArrows.Image = vrapi_GetTextureSwapChainBufferVulkan(appState.RightArrows.SwapChain, 0);
			imageFile = AAssetManager_open(app->activity->assetManager, "rightarrows.png", AASSET_MODE_BUFFER);
			imageFileLength = AAsset_getLength(imageFile);
			imageSource.resize(imageFileLength);
			AAsset_read(imageFile, imageSource.data(), imageFileLength);
			image = stbi_load_from_memory(imageSource.data(), imageFileLength, &imageWidth, &imageHeight, &imageComponents, 4);
			texIndex = ((appState.ScreenHeight - imageHeight) * appState.ScreenWidth + appState.ScreenWidth - imageWidth) / 2;
			index = 0;
			for (auto y = 0; y < imageHeight; y++)
			{
				for (auto x = 0; x < imageWidth; x++)
				{
					auto r = image[index];
					index++;
					auto g = image[index];
					index++;
					auto b = image[index];
					index++;
					auto a = image[index];
					index++;
					auto factor = (double)a / 255;
					r = (unsigned char)((double)r * factor);
					g = (unsigned char)((double)g * factor);
					b = (unsigned char)((double)b * factor);
					appState.RightArrows.Data[texIndex] = ((uint32_t)255 << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
					texIndex++;
				}
				texIndex += appState.ScreenWidth - imageWidth;
			}
			stbi_image_free(image);
			appState.RightArrows.Buffer.size = appState.RightArrows.Data.size() * sizeof(uint32_t);
			bufferCreateInfo.size = appState.RightArrows.Buffer.size;
			VK(appState.Device.vkCreateBuffer(appState.Device.device, &bufferCreateInfo, nullptr, &appState.RightArrows.Buffer.buffer));
			appState.RightArrows.Buffer.flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			VC(appState.Device.vkGetBufferMemoryRequirements(appState.Device.device, appState.RightArrows.Buffer.buffer, &memoryRequirements));
			createMemoryAllocateInfo(appState, memoryRequirements, appState.RightArrows.Buffer.flags, memoryAllocateInfo);
			VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &appState.RightArrows.Buffer.memory));
			VK(appState.Device.vkBindBufferMemory(appState.Device.device, appState.RightArrows.Buffer.buffer, appState.RightArrows.Buffer.memory, 0));
			VK(appState.Device.vkMapMemory(appState.Device.device, appState.RightArrows.Buffer.memory, 0, memoryRequirements.size, 0, &appState.RightArrows.Buffer.mapped));
			memcpy(appState.RightArrows.Buffer.mapped, appState.RightArrows.Data.data(), appState.RightArrows.Buffer.size);
			VC(appState.Device.vkUnmapMemory(appState.Device.device, appState.RightArrows.Buffer.memory));
			appState.RightArrows.Buffer.mapped = nullptr;
			mappedMemoryRange.memory = appState.RightArrows.Buffer.memory;
			VC(appState.Device.vkFlushMappedMemoryRanges(appState.Device.device, 1, &mappedMemoryRange));
			VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
			VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
			VC(appState.Device.vkCmdCopyBufferToImage(setupCommandBuffer, appState.RightArrows.Buffer.buffer, appState.RightArrows.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
			VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
			VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
			VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
			VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
			auto consoleBottom = (float)(appState.ConsoleHeight - SBAR_HEIGHT - 24) / appState.ConsoleHeight;
			auto statusBarTop = (float)(appState.ScreenHeight - SBAR_HEIGHT - 24) / appState.ScreenHeight;
			appState.ConsoleVertices.assign({ -1, consoleBottom * 2 - 1, 0, 0, consoleBottom, 1, consoleBottom * 2 - 1, 0, 1, consoleBottom, 1, -1, 0, 1, 0, -1, -1, 0, 0, 0, -1, 1, 0, 0, 1, -1.0 / 3.0, 1, 0, 1, 1, -1.0 / 3.0, statusBarTop * 2 - 1, 0, 1, consoleBottom, -1, statusBarTop * 2 - 1, 0, 0, consoleBottom });
			appState.ConsoleIndices.assign({ 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4 });
			imageFile = AAssetManager_open(app->activity->assetManager, "floor.png", AASSET_MODE_BUFFER);
			imageFileLength = AAsset_getLength(imageFile);
			imageSource.resize(imageFileLength);
			AAsset_read(imageFile, imageSource.data(), imageFileLength);
			image = stbi_load_from_memory(imageSource.data(), imageFileLength, &imageWidth, &imageHeight, &imageComponents, 4);
			appState.FloorWidth = imageWidth;
			appState.FloorHeight = imageHeight;
			appState.FloorData.resize(imageWidth * imageHeight);
			memcpy(appState.FloorData.data(), image, appState.FloorData.size() * sizeof(uint32_t));
			stbi_image_free(image);
			appState.NoGameDataData.resize(appState.ScreenWidth * appState.ScreenHeight, 255 << 24);
			imageFile = AAssetManager_open(app->activity->assetManager, "nogamedata.png", AASSET_MODE_BUFFER);
			imageFileLength = AAsset_getLength(imageFile);
			imageSource.resize(imageFileLength);
			AAsset_read(imageFile, imageSource.data(), imageFileLength);
			image = stbi_load_from_memory(imageSource.data(), imageFileLength, &imageWidth, &imageHeight, &imageComponents, 4);
			texIndex = ((appState.ScreenHeight - imageHeight) * appState.ScreenWidth + appState.ScreenWidth - imageWidth) / 2;
			index = 0;
			for (auto y = 0; y < imageHeight; y++)
			{
				for (auto x = 0; x < imageWidth; x++)
				{
					auto r = image[index];
					index++;
					auto g = image[index];
					index++;
					auto b = image[index];
					index++;
					auto a = image[index];
					index++;
					auto factor = (double)a / 255;
					r = (unsigned char)((double)r * factor);
					g = (unsigned char)((double)g * factor);
					b = (unsigned char)((double)b * factor);
					appState.NoGameDataData[texIndex] = ((uint32_t)255 << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
					texIndex++;
				}
				texIndex += appState.ScreenWidth - imageWidth;
			}
			stbi_image_free(image);
			for (auto b = 0; b < 5; b++)
			{
				auto i = (unsigned char)(192.0 * sin(M_PI / (double)(b - 1)));
				auto color = ((uint32_t)255 << 24) | ((uint32_t)i << 16) | ((uint32_t)i << 8) | i;
				auto texTopIndex = b * appState.ScreenWidth + b;
				auto texBottomIndex = (appState.ScreenHeight - 1 - b) * appState.ScreenWidth + b;
				for (auto x = 0; x < appState.ScreenWidth - b - b; x++)
				{
					appState.NoGameDataData[texTopIndex] = color;
					texTopIndex++;
					appState.NoGameDataData[texBottomIndex] = color;
					texBottomIndex++;
				}
				auto texLeftIndex = (b + 1) * appState.ScreenWidth + b;
				auto texRightIndex = (b + 1) * appState.ScreenWidth + appState.ScreenWidth - 1 - b;
				for (auto y = 0; y < appState.ScreenHeight - b - 1 - b - 1; y++)
				{
					appState.NoGameDataData[texLeftIndex] = color;
					texLeftIndex += appState.ScreenWidth;
					appState.NoGameDataData[texRightIndex] = color;
					texRightIndex += appState.ScreenWidth;
				}
			}
			appState.Scene.numBuffers = (isMultiview) ? VRAPI_FRAME_LAYER_EYE_MAX : 1;
			createShaderModule(appState, app, (isMultiview ? "shaders/textured_multiview.vert.spv" : "shaders/textured.vert.spv"), &appState.Scene.texturedVertex);
			createShaderModule(appState, app, "shaders/textured.frag.spv", &appState.Scene.texturedFragment);
			createShaderModule(appState, app, (isMultiview ? "shaders/surface_multiview.vert.spv" : "shaders/surface.vert.spv"), &appState.Scene.surfaceVertex);
			createShaderModule(appState, app, "shaders/sprite.frag.spv", &appState.Scene.spritesFragment);
			createShaderModule(appState, app, "shaders/turbulent.frag.spv", &appState.Scene.turbulentFragment);
			createShaderModule(appState, app, (isMultiview ? "shaders/alias_multiview.vert.spv" : "shaders/alias.vert.spv"), &appState.Scene.aliasVertex);
			createShaderModule(appState, app, "shaders/alias.frag.spv", &appState.Scene.aliasFragment);
			createShaderModule(appState, app, (isMultiview ? "shaders/viewmodel_multiview.vert.spv" : "shaders/viewmodel.vert.spv"), &appState.Scene.viewmodelVertex);
			createShaderModule(appState, app, "shaders/viewmodel.frag.spv", &appState.Scene.viewmodelFragment);
			createShaderModule(appState, app, (isMultiview ? "shaders/colored_multiview.vert.spv" : "shaders/colored.vert.spv"), &appState.Scene.coloredVertex);
			createShaderModule(appState, app, "shaders/colored.frag.spv", &appState.Scene.coloredFragment);
			createShaderModule(appState, app, (isMultiview ? "shaders/sky_multiview.vert.spv" : "shaders/sky.vert.spv"), &appState.Scene.skyVertex);
			createShaderModule(appState, app, "shaders/sky.frag.spv", &appState.Scene.skyFragment);
			createShaderModule(appState, app, (isMultiview ? "shaders/floor_multiview.vert.spv" : "shaders/floor.vert.spv"), &appState.Scene.floorVertex);
			createShaderModule(appState, app, "shaders/floor.frag.spv", &appState.Scene.floorFragment);
			createShaderModule(appState, app, "shaders/console.vert.spv", &appState.Scene.consoleVertex);
			createShaderModule(appState, app, "shaders/console.frag.spv", &appState.Scene.consoleFragment);
			VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo { };
			tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			VkPipelineViewportStateCreateInfo viewportStateCreateInfo { };
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.viewportCount = 1;
			viewportStateCreateInfo.scissorCount = 1;
			VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo { };
			rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizationStateCreateInfo.lineWidth = 1.0f;
			VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo { };
			multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_4_BIT;
			multisampleStateCreateInfo.minSampleShading = 1.0f;
			VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo { };
			depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
			depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
			depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencilStateCreateInfo.front.failOp = VK_STENCIL_OP_KEEP;
			depthStencilStateCreateInfo.front.passOp = VK_STENCIL_OP_KEEP;
			depthStencilStateCreateInfo.front.depthFailOp = VK_STENCIL_OP_KEEP;
			depthStencilStateCreateInfo.front.compareOp = VK_COMPARE_OP_ALWAYS;
			depthStencilStateCreateInfo.back.failOp = VK_STENCIL_OP_KEEP;
			depthStencilStateCreateInfo.back.passOp = VK_STENCIL_OP_KEEP;
			depthStencilStateCreateInfo.back.depthFailOp = VK_STENCIL_OP_KEEP;
			depthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
			depthStencilStateCreateInfo.minDepthBounds = 0.0f;
			depthStencilStateCreateInfo.maxDepthBounds = 1.0f;
			VkPipelineColorBlendAttachmentState colorBlendAttachmentState { };
			colorBlendAttachmentState.blendEnable = VK_TRUE;
			colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo { };
			colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_CLEAR;
			colorBlendStateCreateInfo.attachmentCount = 1;
			colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
			VkDynamicState dynamicStateEnables[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo { };
			pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			pipelineDynamicStateCreateInfo.dynamicStateCount = 2;
			pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables;
			appState.Scene.texturedAttributes.vertexAttributes.resize(6);
			appState.Scene.texturedAttributes.vertexBindings.resize(3);
			appState.Scene.texturedAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			appState.Scene.texturedAttributes.vertexBindings[0].stride = 3 * sizeof(float);
			appState.Scene.texturedAttributes.vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.texturedAttributes.vertexAttributes[1].location = 1;
			appState.Scene.texturedAttributes.vertexAttributes[1].binding = 1;
			appState.Scene.texturedAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
			appState.Scene.texturedAttributes.vertexBindings[1].binding = 1;
			appState.Scene.texturedAttributes.vertexBindings[1].stride = 2 * sizeof(float);
			appState.Scene.texturedAttributes.vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.texturedAttributes.vertexAttributes[2].location = 2;
			appState.Scene.texturedAttributes.vertexAttributes[2].binding = 2;
			appState.Scene.texturedAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.texturedAttributes.vertexAttributes[3].location = 3;
			appState.Scene.texturedAttributes.vertexAttributes[3].binding = 2;
			appState.Scene.texturedAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.texturedAttributes.vertexAttributes[3].offset = 4 * sizeof(float);
			appState.Scene.texturedAttributes.vertexAttributes[4].location = 4;
			appState.Scene.texturedAttributes.vertexAttributes[4].binding = 2;
			appState.Scene.texturedAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.texturedAttributes.vertexAttributes[4].offset = 8 * sizeof(float);
			appState.Scene.texturedAttributes.vertexAttributes[5].location = 5;
			appState.Scene.texturedAttributes.vertexAttributes[5].binding = 2;
			appState.Scene.texturedAttributes.vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.texturedAttributes.vertexAttributes[5].offset = 12 * sizeof(float);
			appState.Scene.texturedAttributes.vertexBindings[2].binding = 2;
			appState.Scene.texturedAttributes.vertexBindings[2].stride = 16 * sizeof(float);
			appState.Scene.texturedAttributes.vertexBindings[2].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
			appState.Scene.texturedAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			appState.Scene.texturedAttributes.vertexInputState.vertexBindingDescriptionCount = appState.Scene.texturedAttributes.vertexBindings.size();
			appState.Scene.texturedAttributes.vertexInputState.pVertexBindingDescriptions = appState.Scene.texturedAttributes.vertexBindings.data();
			appState.Scene.texturedAttributes.vertexInputState.vertexAttributeDescriptionCount = appState.Scene.texturedAttributes.vertexAttributes.size();
			appState.Scene.texturedAttributes.vertexInputState.pVertexAttributeDescriptions = appState.Scene.texturedAttributes.vertexAttributes.data();
			appState.Scene.texturedAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			appState.Scene.texturedAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			appState.Scene.colormappedAttributes.vertexAttributes.resize(7);
			appState.Scene.colormappedAttributes.vertexBindings.resize(4);
			appState.Scene.colormappedAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			appState.Scene.colormappedAttributes.vertexBindings[0].stride = 3 * sizeof(float);
			appState.Scene.colormappedAttributes.vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.colormappedAttributes.vertexAttributes[1].location = 1;
			appState.Scene.colormappedAttributes.vertexAttributes[1].binding = 1;
			appState.Scene.colormappedAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
			appState.Scene.colormappedAttributes.vertexBindings[1].binding = 1;
			appState.Scene.colormappedAttributes.vertexBindings[1].stride = 2 * sizeof(float);
			appState.Scene.colormappedAttributes.vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.colormappedAttributes.vertexAttributes[2].location = 2;
			appState.Scene.colormappedAttributes.vertexAttributes[2].binding = 2;
			appState.Scene.colormappedAttributes.vertexAttributes[2].format = VK_FORMAT_R32_SFLOAT;
			appState.Scene.colormappedAttributes.vertexBindings[2].binding = 2;
			appState.Scene.colormappedAttributes.vertexBindings[2].stride = sizeof(float);
			appState.Scene.colormappedAttributes.vertexBindings[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.colormappedAttributes.vertexAttributes[3].location = 3;
			appState.Scene.colormappedAttributes.vertexAttributes[3].binding = 3;
			appState.Scene.colormappedAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.colormappedAttributes.vertexAttributes[4].location = 4;
			appState.Scene.colormappedAttributes.vertexAttributes[4].binding = 3;
			appState.Scene.colormappedAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.colormappedAttributes.vertexAttributes[4].offset = 4 * sizeof(float);
			appState.Scene.colormappedAttributes.vertexAttributes[5].location = 5;
			appState.Scene.colormappedAttributes.vertexAttributes[5].binding = 3;
			appState.Scene.colormappedAttributes.vertexAttributes[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.colormappedAttributes.vertexAttributes[5].offset = 8 * sizeof(float);
			appState.Scene.colormappedAttributes.vertexAttributes[6].location = 6;
			appState.Scene.colormappedAttributes.vertexAttributes[6].binding = 3;
			appState.Scene.colormappedAttributes.vertexAttributes[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.colormappedAttributes.vertexAttributes[6].offset = 12 * sizeof(float);
			appState.Scene.colormappedAttributes.vertexBindings[3].binding = 3;
			appState.Scene.colormappedAttributes.vertexBindings[3].stride = 16 * sizeof(float);
			appState.Scene.colormappedAttributes.vertexBindings[3].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
			appState.Scene.colormappedAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			appState.Scene.colormappedAttributes.vertexInputState.vertexBindingDescriptionCount = appState.Scene.colormappedAttributes.vertexBindings.size();
			appState.Scene.colormappedAttributes.vertexInputState.pVertexBindingDescriptions = appState.Scene.colormappedAttributes.vertexBindings.data();
			appState.Scene.colormappedAttributes.vertexInputState.vertexAttributeDescriptionCount = appState.Scene.colormappedAttributes.vertexAttributes.size();
			appState.Scene.colormappedAttributes.vertexInputState.pVertexAttributeDescriptions = appState.Scene.colormappedAttributes.vertexAttributes.data();
			appState.Scene.colormappedAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			appState.Scene.colormappedAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			appState.Scene.coloredAttributes.vertexAttributes.resize(5);
			appState.Scene.coloredAttributes.vertexBindings.resize(2);
			appState.Scene.coloredAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			appState.Scene.coloredAttributes.vertexBindings[0].stride = 3 * sizeof(float);
			appState.Scene.coloredAttributes.vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.coloredAttributes.vertexAttributes[1].location = 1;
			appState.Scene.coloredAttributes.vertexAttributes[1].binding = 1;
			appState.Scene.coloredAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.coloredAttributes.vertexAttributes[2].location = 2;
			appState.Scene.coloredAttributes.vertexAttributes[2].binding = 1;
			appState.Scene.coloredAttributes.vertexAttributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.coloredAttributes.vertexAttributes[2].offset = 4 * sizeof(float);
			appState.Scene.coloredAttributes.vertexAttributes[3].location = 3;
			appState.Scene.coloredAttributes.vertexAttributes[3].binding = 1;
			appState.Scene.coloredAttributes.vertexAttributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.coloredAttributes.vertexAttributes[3].offset = 8 * sizeof(float);
			appState.Scene.coloredAttributes.vertexAttributes[4].location = 4;
			appState.Scene.coloredAttributes.vertexAttributes[4].binding = 1;
			appState.Scene.coloredAttributes.vertexAttributes[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			appState.Scene.coloredAttributes.vertexAttributes[4].offset = 12 * sizeof(float);
			appState.Scene.coloredAttributes.vertexBindings[1].binding = 1;
			appState.Scene.coloredAttributes.vertexBindings[1].stride = 16 * sizeof(float);
			appState.Scene.coloredAttributes.vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
			appState.Scene.coloredAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			appState.Scene.coloredAttributes.vertexInputState.vertexBindingDescriptionCount = appState.Scene.coloredAttributes.vertexBindings.size();
			appState.Scene.coloredAttributes.vertexInputState.pVertexBindingDescriptions = appState.Scene.coloredAttributes.vertexBindings.data();
			appState.Scene.coloredAttributes.vertexInputState.vertexAttributeDescriptionCount = appState.Scene.coloredAttributes.vertexAttributes.size();
			appState.Scene.coloredAttributes.vertexInputState.pVertexAttributeDescriptions = appState.Scene.coloredAttributes.vertexAttributes.data();
			appState.Scene.coloredAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			appState.Scene.coloredAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			appState.Scene.skyAttributes.vertexAttributes.resize(2);
			appState.Scene.skyAttributes.vertexBindings.resize(2);
			appState.Scene.skyAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			appState.Scene.skyAttributes.vertexBindings[0].stride = 3 * sizeof(float);
			appState.Scene.skyAttributes.vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.skyAttributes.vertexAttributes[1].location = 1;
			appState.Scene.skyAttributes.vertexAttributes[1].binding = 1;
			appState.Scene.skyAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
			appState.Scene.skyAttributes.vertexBindings[1].binding = 1;
			appState.Scene.skyAttributes.vertexBindings[1].stride = 2 * sizeof(float);
			appState.Scene.skyAttributes.vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.skyAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			appState.Scene.skyAttributes.vertexInputState.vertexBindingDescriptionCount = appState.Scene.skyAttributes.vertexBindings.size();
			appState.Scene.skyAttributes.vertexInputState.pVertexBindingDescriptions = appState.Scene.skyAttributes.vertexBindings.data();
			appState.Scene.skyAttributes.vertexInputState.vertexAttributeDescriptionCount = appState.Scene.skyAttributes.vertexAttributes.size();
			appState.Scene.skyAttributes.vertexInputState.pVertexAttributeDescriptions = appState.Scene.skyAttributes.vertexAttributes.data();
			appState.Scene.skyAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			appState.Scene.skyAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			appState.Scene.floorAttributes.vertexAttributes.resize(2);
			appState.Scene.floorAttributes.vertexBindings.resize(2);
			appState.Scene.floorAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			appState.Scene.floorAttributes.vertexBindings[0].stride = 3 * sizeof(float);
			appState.Scene.floorAttributes.vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.floorAttributes.vertexAttributes[1].location = 1;
			appState.Scene.floorAttributes.vertexAttributes[1].binding = 1;
			appState.Scene.floorAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
			appState.Scene.floorAttributes.vertexBindings[1].binding = 1;
			appState.Scene.floorAttributes.vertexBindings[1].stride = 2 * sizeof(float);
			appState.Scene.floorAttributes.vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.floorAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			appState.Scene.floorAttributes.vertexInputState.vertexBindingDescriptionCount = appState.Scene.floorAttributes.vertexBindings.size();
			appState.Scene.floorAttributes.vertexInputState.pVertexBindingDescriptions = appState.Scene.floorAttributes.vertexBindings.data();
			appState.Scene.floorAttributes.vertexInputState.vertexAttributeDescriptionCount = appState.Scene.floorAttributes.vertexAttributes.size();
			appState.Scene.floorAttributes.vertexInputState.pVertexAttributeDescriptions = appState.Scene.floorAttributes.vertexAttributes.data();
			appState.Scene.floorAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			appState.Scene.floorAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			appState.Scene.consoleAttributes.vertexAttributes.resize(2);
			appState.Scene.consoleAttributes.vertexBindings.resize(2);
			appState.Scene.consoleAttributes.vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			appState.Scene.consoleAttributes.vertexBindings[0].stride = 5 * sizeof(float);
			appState.Scene.consoleAttributes.vertexBindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.consoleAttributes.vertexAttributes[1].location = 1;
			appState.Scene.consoleAttributes.vertexAttributes[1].binding = 1;
			appState.Scene.consoleAttributes.vertexAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
			appState.Scene.consoleAttributes.vertexAttributes[1].offset = 3 * sizeof(float);
			appState.Scene.consoleAttributes.vertexBindings[1].binding = 1;
			appState.Scene.consoleAttributes.vertexBindings[1].stride = 5 * sizeof(float);
			appState.Scene.consoleAttributes.vertexBindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			appState.Scene.consoleAttributes.vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			appState.Scene.consoleAttributes.vertexInputState.vertexBindingDescriptionCount = appState.Scene.consoleAttributes.vertexBindings.size();
			appState.Scene.consoleAttributes.vertexInputState.pVertexBindingDescriptions = appState.Scene.consoleAttributes.vertexBindings.data();
			appState.Scene.consoleAttributes.vertexInputState.vertexAttributeDescriptionCount = appState.Scene.consoleAttributes.vertexAttributes.size();
			appState.Scene.consoleAttributes.vertexInputState.pVertexAttributeDescriptions = appState.Scene.consoleAttributes.vertexAttributes.data();
			appState.Scene.consoleAttributes.inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			appState.Scene.consoleAttributes.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			appState.Scene.textured.stages.resize(2);
			appState.Scene.textured.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.textured.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			appState.Scene.textured.stages[0].module = appState.Scene.surfaceVertex;
			appState.Scene.textured.stages[0].pName = "main";
			appState.Scene.textured.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.textured.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			appState.Scene.textured.stages[1].module = appState.Scene.texturedFragment;
			appState.Scene.textured.stages[1].pName = "main";
			VkDescriptorSetLayoutBinding descriptorSetBindings[3] { };
			descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorSetBindings[0].descriptorCount = 1;
			descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			descriptorSetBindings[1].binding = 1;
			descriptorSetBindings[2].binding = 2;
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo { };
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.pBindings = descriptorSetBindings;
			descriptorSetLayoutCreateInfo.bindingCount = 1;
			VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &appState.Scene.palette));
			descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorSetBindings[0].descriptorCount = 1;
			descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorSetBindings[1].descriptorCount = 1;
			descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			descriptorSetLayoutCreateInfo.bindingCount = 2;
			VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &appState.Scene.textured.descriptorSetLayout));
			VkDescriptorSetLayout descriptorSetLayouts[2] { };
			descriptorSetLayouts[0] = appState.Scene.palette;
			descriptorSetLayouts[1] = appState.Scene.textured.descriptorSetLayout;
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo { };
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = 2;
			pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;
			VkPushConstantRange pushConstantInfo { };
			pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			pushConstantInfo.size = 3 * sizeof(float);
			pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
			VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &appState.Scene.textured.pipelineLayout));
			VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo { };
			graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCreateInfo.stageCount = appState.Scene.textured.stages.size();
			graphicsPipelineCreateInfo.pStages = appState.Scene.textured.stages.data();
			graphicsPipelineCreateInfo.pVertexInputState = &appState.Scene.texturedAttributes.vertexInputState;
			graphicsPipelineCreateInfo.pInputAssemblyState = &appState.Scene.texturedAttributes.inputAssemblyState;
			graphicsPipelineCreateInfo.pTessellationState = &tessellationStateCreateInfo;
			graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
			graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
			graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
			graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
			graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
			graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
			graphicsPipelineCreateInfo.layout = appState.Scene.textured.pipelineLayout;
			graphicsPipelineCreateInfo.renderPass = appState.RenderPass;
			VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &appState.Scene.textured.pipeline));
			appState.Scene.sprites.stages.resize(2);
			appState.Scene.sprites.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.sprites.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			appState.Scene.sprites.stages[0].module = appState.Scene.texturedVertex;
			appState.Scene.sprites.stages[0].pName = "main";
			appState.Scene.sprites.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.sprites.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			appState.Scene.sprites.stages[1].module = appState.Scene.spritesFragment;
			appState.Scene.sprites.stages[1].pName = "main";
			VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &appState.Scene.sprites.descriptorSetLayout));
			pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
			pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
			descriptorSetLayouts[1] = appState.Scene.sprites.descriptorSetLayout;
			VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &appState.Scene.sprites.pipelineLayout));
			graphicsPipelineCreateInfo.stageCount = appState.Scene.sprites.stages.size();
			graphicsPipelineCreateInfo.pStages = appState.Scene.sprites.stages.data();
			graphicsPipelineCreateInfo.layout = appState.Scene.sprites.pipelineLayout;
			VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &appState.Scene.sprites.pipeline));
			appState.Scene.turbulent.stages.resize(2);
			appState.Scene.turbulent.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.turbulent.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			appState.Scene.turbulent.stages[0].module = appState.Scene.surfaceVertex;
			appState.Scene.turbulent.stages[0].pName = "main";
			appState.Scene.turbulent.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.turbulent.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			appState.Scene.turbulent.stages[1].module = appState.Scene.turbulentFragment;
			appState.Scene.turbulent.stages[1].pName = "main";
			VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &appState.Scene.turbulent.descriptorSetLayout));
			pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			pushConstantInfo.size = 4 * sizeof(float);
			pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
			descriptorSetLayouts[1] = appState.Scene.turbulent.descriptorSetLayout;
			VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &appState.Scene.turbulent.pipelineLayout));
			graphicsPipelineCreateInfo.stageCount = appState.Scene.turbulent.stages.size();
			graphicsPipelineCreateInfo.pStages = appState.Scene.turbulent.stages.data();
			graphicsPipelineCreateInfo.layout = appState.Scene.turbulent.pipelineLayout;
			VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &appState.Scene.turbulent.pipeline));
			appState.Scene.alias.stages.resize(2);
			appState.Scene.alias.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.alias.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			appState.Scene.alias.stages[0].module = appState.Scene.aliasVertex;
			appState.Scene.alias.stages[0].pName = "main";
			appState.Scene.alias.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.alias.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			appState.Scene.alias.stages[1].module = appState.Scene.aliasFragment;
			appState.Scene.alias.stages[1].pName = "main";
			descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorSetBindings[0].descriptorCount = 1;
			descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorSetBindings[1].descriptorCount = 1;
			descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			descriptorSetBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorSetBindings[2].descriptorCount = 1;
			descriptorSetBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			descriptorSetLayoutCreateInfo.bindingCount = 3;
			VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &appState.Scene.alias.descriptorSetLayout));
			pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			pushConstantInfo.size = 16 * sizeof(float);
			pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantInfo;
			descriptorSetLayouts[1] = appState.Scene.alias.descriptorSetLayout;
			VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &appState.Scene.alias.pipelineLayout));
			graphicsPipelineCreateInfo.stageCount = appState.Scene.alias.stages.size();
			graphicsPipelineCreateInfo.pStages = appState.Scene.alias.stages.data();
			graphicsPipelineCreateInfo.pVertexInputState = &appState.Scene.colormappedAttributes.vertexInputState;
			graphicsPipelineCreateInfo.pInputAssemblyState = &appState.Scene.colormappedAttributes.inputAssemblyState;
			graphicsPipelineCreateInfo.layout = appState.Scene.alias.pipelineLayout;
			VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &appState.Scene.alias.pipeline));
			appState.Scene.viewmodel.stages.resize(2);
			appState.Scene.viewmodel.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.viewmodel.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			appState.Scene.viewmodel.stages[0].module = appState.Scene.viewmodelVertex;
			appState.Scene.viewmodel.stages[0].pName = "main";
			appState.Scene.viewmodel.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.viewmodel.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			appState.Scene.viewmodel.stages[1].module = appState.Scene.viewmodelFragment;
			appState.Scene.viewmodel.stages[1].pName = "main";
			VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &appState.Scene.viewmodel.descriptorSetLayout));
			pushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			pushConstantInfo.size = 24 * sizeof(float);
			descriptorSetLayouts[1] = appState.Scene.viewmodel.descriptorSetLayout;
			VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &appState.Scene.viewmodel.pipelineLayout));
			graphicsPipelineCreateInfo.stageCount = appState.Scene.viewmodel.stages.size();
			graphicsPipelineCreateInfo.pStages = appState.Scene.viewmodel.stages.data();
			graphicsPipelineCreateInfo.layout = appState.Scene.viewmodel.pipelineLayout;
			VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &appState.Scene.viewmodel.pipeline));
			appState.Scene.colored.stages.resize(2);
			appState.Scene.colored.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.colored.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			appState.Scene.colored.stages[0].module = appState.Scene.coloredVertex;
			appState.Scene.colored.stages[0].pName = "main";
			appState.Scene.colored.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.colored.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			appState.Scene.colored.stages[1].module = appState.Scene.coloredFragment;
			appState.Scene.colored.stages[1].pName = "main";
			descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorSetBindings[0].descriptorCount = 1;
			descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			descriptorSetLayoutCreateInfo.bindingCount = 1;
			VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &appState.Scene.colored.descriptorSetLayout));
			pushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			pushConstantInfo.size = sizeof(float);
			descriptorSetLayouts[1] = appState.Scene.colored.descriptorSetLayout;
			VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &appState.Scene.colored.pipelineLayout));
			graphicsPipelineCreateInfo.stageCount = appState.Scene.colored.stages.size();
			graphicsPipelineCreateInfo.pStages = appState.Scene.colored.stages.data();
			graphicsPipelineCreateInfo.pVertexInputState = &appState.Scene.coloredAttributes.vertexInputState;
			graphicsPipelineCreateInfo.pInputAssemblyState = &appState.Scene.coloredAttributes.inputAssemblyState;
			graphicsPipelineCreateInfo.layout = appState.Scene.colored.pipelineLayout;
			VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &appState.Scene.colored.pipeline));
			appState.Scene.sky.stages.resize(2);
			appState.Scene.sky.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.sky.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			appState.Scene.sky.stages[0].module = appState.Scene.skyVertex;
			appState.Scene.sky.stages[0].pName = "main";
			appState.Scene.sky.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.sky.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			appState.Scene.sky.stages[1].module = appState.Scene.skyFragment;
			appState.Scene.sky.stages[1].pName = "main";
			descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorSetBindings[0].descriptorCount = 1;
			descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorSetBindings[1].descriptorCount = 1;
			descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			descriptorSetBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorSetBindings[2].descriptorCount = 1;
			descriptorSetBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			descriptorSetLayoutCreateInfo.bindingCount = 3;
			VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &appState.Scene.sky.descriptorSetLayout));
			pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
			pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
			descriptorSetLayouts[1] = appState.Scene.sky.descriptorSetLayout;
			VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &appState.Scene.sky.pipelineLayout));
			graphicsPipelineCreateInfo.stageCount = appState.Scene.sky.stages.size();
			graphicsPipelineCreateInfo.pStages = appState.Scene.sky.stages.data();
			graphicsPipelineCreateInfo.pVertexInputState = &appState.Scene.skyAttributes.vertexInputState;
			graphicsPipelineCreateInfo.pInputAssemblyState = &appState.Scene.skyAttributes.inputAssemblyState;
			graphicsPipelineCreateInfo.layout = appState.Scene.sky.pipelineLayout;
			VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &appState.Scene.sky.pipeline));
			appState.Scene.floor.stages.resize(2);
			appState.Scene.floor.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.floor.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			appState.Scene.floor.stages[0].module = appState.Scene.floorVertex;
			appState.Scene.floor.stages[0].pName = "main";
			appState.Scene.floor.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.floor.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			appState.Scene.floor.stages[1].module = appState.Scene.floorFragment;
			appState.Scene.floor.stages[1].pName = "main";
			descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorSetBindings[0].descriptorCount = 1;
			descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorSetBindings[1].descriptorCount = 1;
			descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			descriptorSetLayoutCreateInfo.bindingCount = 2;
			VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &appState.Scene.floor.descriptorSetLayout));
			pipelineLayoutCreateInfo.setLayoutCount = 1;
			pipelineLayoutCreateInfo.pSetLayouts = &appState.Scene.floor.descriptorSetLayout;
			VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &appState.Scene.floor.pipelineLayout));
			graphicsPipelineCreateInfo.stageCount = appState.Scene.floor.stages.size();
			graphicsPipelineCreateInfo.pStages = appState.Scene.floor.stages.data();
			graphicsPipelineCreateInfo.pVertexInputState = &appState.Scene.floorAttributes.vertexInputState;
			graphicsPipelineCreateInfo.pInputAssemblyState = &appState.Scene.floorAttributes.inputAssemblyState;
			graphicsPipelineCreateInfo.layout = appState.Scene.floor.pipelineLayout;
			VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &appState.Scene.floor.pipeline));
			appState.Scene.console.stages.resize(2);
			appState.Scene.console.stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.console.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
			appState.Scene.console.stages[0].module = appState.Scene.consoleVertex;
			appState.Scene.console.stages[0].pName = "main";
			appState.Scene.console.stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			appState.Scene.console.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			appState.Scene.console.stages[1].module = appState.Scene.consoleFragment;
			appState.Scene.console.stages[1].pName = "main";
			descriptorSetBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorSetBindings[0].descriptorCount = 1;
			descriptorSetBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			descriptorSetBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorSetBindings[1].descriptorCount = 1;
			descriptorSetBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			descriptorSetLayoutCreateInfo.bindingCount = 2;
			VK(appState.Device.vkCreateDescriptorSetLayout(appState.Device.device, &descriptorSetLayoutCreateInfo, nullptr, &appState.Scene.console.descriptorSetLayout));
			pipelineLayoutCreateInfo.pSetLayouts = &appState.Scene.console.descriptorSetLayout;
			VK(appState.Device.vkCreatePipelineLayout(appState.Device.device, &pipelineLayoutCreateInfo, nullptr, &appState.Scene.console.pipelineLayout));
			graphicsPipelineCreateInfo.stageCount = appState.Scene.console.stages.size();
			graphicsPipelineCreateInfo.pStages = appState.Scene.console.stages.data();
			graphicsPipelineCreateInfo.pVertexInputState = &appState.Scene.consoleAttributes.vertexInputState;
			graphicsPipelineCreateInfo.pInputAssemblyState = &appState.Scene.consoleAttributes.inputAssemblyState;
			graphicsPipelineCreateInfo.layout = appState.Scene.console.pipelineLayout;
			graphicsPipelineCreateInfo.renderPass = appState.Console.RenderPass;
			VK(appState.Device.vkCreateGraphicsPipelines(appState.Device.device, appState.Context.pipelineCache, 1, &graphicsPipelineCreateInfo, nullptr, &appState.Scene.console.pipeline));
			appState.Scene.matrices.size = appState.Scene.numBuffers * 2 * sizeof(ovrMatrix4f);
			bufferCreateInfo.size = appState.Scene.matrices.size;
			bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			VK(appState.Device.vkCreateBuffer(appState.Device.device, &bufferCreateInfo, nullptr, &appState.Scene.matrices.buffer));
			appState.Scene.matrices.flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			VC(appState.Device.vkGetBufferMemoryRequirements(appState.Device.device, appState.Scene.matrices.buffer, &memoryRequirements));
			createMemoryAllocateInfo(appState, memoryRequirements, appState.Scene.matrices.flags, memoryAllocateInfo);
			VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &appState.Scene.matrices.memory));
			VK(appState.Device.vkBindBufferMemory(appState.Device.device, appState.Scene.matrices.buffer, appState.Scene.matrices.memory, 0));
			appState.Scene.createdScene = true;
		}
		appState.FrameIndex++;
		VkDeviceSize noOffset = 0;
		const double predictedDisplayTime = vrapi_GetPredictedDisplayTime(appState.Ovr, appState.FrameIndex);
		const ovrTracking2 tracking = vrapi_GetPredictedTracking2(appState.Ovr, predictedDisplayTime);
		appState.DisplayTime = predictedDisplayTime;
		if (appState.Mode != AppStartupMode && appState.Mode != AppNoGameDataMode)
		{
			if (cls.demoplayback || cl.intermission)
			{
				appState.Mode = AppScreenMode;
			}
			else
			{
				appState.Mode = AppWorldMode;
			}
		}
		if (appState.PreviousMode != appState.Mode)
		{
			if (appState.PreviousMode == AppStartupMode)
			{
				sys_version = "OVR 1.0.3";
				const char* basedir = "/sdcard/android/data/com.heribertodelgado.slipnfrag/files";
				std::vector<std::string> arguments;
				arguments.emplace_back("SlipNFrag");
				arguments.emplace_back("-basedir");
				arguments.emplace_back(basedir);
				std::vector<unsigned char> commandline;
				auto file = fopen((std::string(basedir) + "/slipnfrag.commandline").c_str(), "rb");
				if (file != nullptr)
				{
					fseek(file, 0, SEEK_END);
					auto length = ftell(file);
					if (length > 0)
					{
						fseek(file, 0, SEEK_SET);
						commandline.resize(length);
						fread(commandline.data(), length, 1, file);
					}
					fclose(file);
				}
				arguments.emplace_back();
				for (auto c : commandline)
				{
					if (c <= ' ')
					{
						if (arguments[arguments.size() - 1].size() != 0)
						{
							arguments.emplace_back();
						}
					}
					else
					{
						arguments[arguments.size() - 1] += c;
					}
				}
				if (arguments[arguments.size() - 1].size() == 0)
				{
					arguments.pop_back();
				}
				sys_argc = (int)arguments.size();
				sys_argv = new char*[sys_argc];
				for (auto i = 0; i < arguments.size(); i++)
				{
					sys_argv[i] = new char[arguments[i].length() + 1];
					strcpy(sys_argv[i], arguments[i].c_str());
				}
				Sys_Init(sys_argc, sys_argv);
				if (sys_errormessage.length() > 0)
				{
					if (sys_nogamedata)
					{
						appState.Mode = AppNoGameDataMode;
					}
					else
					{
						exit(0);
					}
				}
				appState.PreviousLeftButtons = 0;
				appState.PreviousRightButtons = 0;
				appState.DefaultFOV = (int)Cvar_VariableValue("fov");
			}
			if (appState.Mode == AppScreenMode)
			{
				d_uselists = false;
				r_skip_fov_check = false;
				sb_onconsole = false;
				Cvar_SetValue("joystick", 1);
				Cvar_SetValue("joyadvanced", 1);
				Cvar_SetValue("joyadvaxisx", AxisSide);
				Cvar_SetValue("joyadvaxisy", AxisForward);
				Cvar_SetValue("joyadvaxisz", AxisTurn);
				Cvar_SetValue("joyadvaxisr", AxisLook);
				Joy_AdvancedUpdate_f();
				vid_width = appState.ScreenWidth;
				vid_height = appState.ScreenHeight;
				con_width = appState.ConsoleWidth;
				con_height = appState.ConsoleHeight;
				Cvar_SetValue("fov", appState.DefaultFOV);
				VID_Resize(320.0 / 240.0);
			}
			else if (appState.Mode == AppWorldMode)
			{
				d_uselists = true;
				r_skip_fov_check = true;
				sb_onconsole = true;
				Cvar_SetValue("joystick", 1);
				Cvar_SetValue("joyadvanced", 1);
				Cvar_SetValue("joyadvaxisx", AxisSide);
				Cvar_SetValue("joyadvaxisy", AxisForward);
				Cvar_SetValue("joyadvaxisz", AxisNada);
				Cvar_SetValue("joyadvaxisr", AxisNada);
				Joy_AdvancedUpdate_f();
				vid_width = appState.ScreenWidth;
				vid_height = appState.ScreenWidth; // to force a square screen for VR, and to get better results for FOV angle calculations
				con_width = appState.ConsoleWidth;
				con_height = appState.ConsoleHeight;
				Cvar_SetValue("fov", appState.FOV);
				VID_Resize(1);
			}
			else if (appState.Mode == AppNoGameDataMode)
			{
				d_uselists = false;
				r_skip_fov_check = false;
				sb_onconsole = false;
			}
			appState.PreviousMode = appState.Mode;
		}
		deleteOldBuffers(appState, appState.Scene.colormappedVertices);
		deleteOldBuffers(appState, appState.Scene.colormappedTexCoords);
		for (auto i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; i++)
		{
			appState.ViewMatrices[i] = ovrMatrix4f_Transpose(&tracking.Eye[i].ViewMatrix);
			appState.ProjectionMatrices[i] = ovrMatrix4f_Transpose(&tracking.Eye[i].ProjectionMatrix);
		}
		auto& orientation = tracking.HeadPose.Pose.Orientation;
		auto x = orientation.x;
		auto y = orientation.y;
		auto z = orientation.z;
		auto w = orientation.w;
		float Q[3] = { x, y, z };
		float ww = w * w;
		float Q11 = Q[1] * Q[1];
		float Q22 = Q[0] * Q[0];
		float Q33 = Q[2] * Q[2];
		const float psign = -1;
		float s2 = psign * 2 * (psign * w * Q[0] + Q[1] * Q[2]);
		const float singularityRadius = 1e-12;
		if (s2 < singularityRadius - 1)
		{
			appState.Yaw = 0;
			appState.Pitch = -M_PI / 2;
			appState.Roll = atan2(2 * (psign * Q[1] * Q[0] + w * Q[2]), ww + Q22 - Q11 - Q33);
		}
		else if (s2 > 1 - singularityRadius)
		{
			appState.Yaw = 0;
			appState.Pitch = M_PI / 2;
			appState.Roll = atan2(2 * (psign * Q[1] * Q[0] + w * Q[2]), ww + Q22 - Q11 - Q33);
		}
		else
		{
			appState.Yaw = -(atan2(-2 * (w * Q[1] - psign * Q[0] * Q[2]), ww + Q33 - Q11 - Q22));
			appState.Pitch = asin(s2);
			appState.Roll = atan2(2 * (w * Q[2] - psign * Q[1] * Q[0]), ww + Q11 - Q22 - Q33);
		}
		auto pose = vrapi_LocateTrackingSpace(appState.Ovr, VRAPI_TRACKING_SPACE_LOCAL_FLOOR);
		auto playerHeight = 32;
		if (host_initialized && cl.viewentity >= 0 && cl.viewentity < cl_entities.size())
		{
			auto player = &cl_entities[cl.viewentity];
			if (player != nullptr)
			{
				auto model = player->model;
				if (model != nullptr)
				{
					playerHeight = model->maxs[1] - model->mins[1];
				}
			}
		}
		auto scale = -pose.Position.y / playerHeight;
		bool host_updated = false;
		if (host_initialized)
		{
			if (appState.Mode == AppWorldMode)
			{
				cl.viewangles[YAW] = appState.Yaw * 180 / M_PI + 90;
				cl.viewangles[PITCH] = -appState.Pitch * 180 / M_PI;
				cl.viewangles[ROLL] = -appState.Roll * 180 / M_PI;
			}
			if (appState.PreviousTime < 0)
			{
				appState.PreviousTime = getTime();
			}
			else if (appState.CurrentTime < 0)
			{
				appState.CurrentTime = getTime();
			}
			else
			{
				appState.PreviousTime = appState.CurrentTime;
				appState.CurrentTime = getTime();
				frame_lapse = (float) (appState.CurrentTime - appState.PreviousTime);
				if (appState.Mode == AppWorldMode)
				{
					appState.TimeInWorldMode += frame_lapse;
					if (!appState.ControlsMessageDisplayed && appState.TimeInWorldMode > 4)
					{
						SCR_InterruptableCenterPrint("Controls:\n\nLeft or Right Joysticks:\nWalk Forward / Backpedal, \n   Step Left / Step Right \n\n[B] / [Y]: Jump     \n[A] / [X]: Swim down\nTriggers: Attack  \nGrip Triggers: Run          \nClick Joysticks: Change Weapon  \n\nApproach and fire weapon to close");
						appState.ControlsMessageDisplayed = true;
					}
				}
			}
			if (r_cache_thrash)
			{
				VID_ReallocSurfCache();
			}
			host_updated = Host_FrameUpdate(frame_lapse);
			if (sys_quitcalled)
			{
				ANativeActivity_finish(app->activity);
			}
			if (sys_errormessage.length() > 0)
			{
				exit(0);
			}
			if (host_updated)
			{
				if (d_uselists)
				{
					r_modelorg_delta[0] = tracking.HeadPose.Pose.Position.x / scale;
					r_modelorg_delta[1] = -tracking.HeadPose.Pose.Position.z / scale;
					r_modelorg_delta[2] = tracking.HeadPose.Pose.Position.y / scale;
					auto distanceSquared = r_modelorg_delta[0] * r_modelorg_delta[0] + r_modelorg_delta[1] * r_modelorg_delta[1] + r_modelorg_delta[2] * r_modelorg_delta[2];
					appState.NearViewModel = (distanceSquared < 12 * 12);
					d_awayfromviewmodel = !appState.NearViewModel;
					d_lists.last_surface = -1;
					d_lists.last_sprite = -1;
					d_lists.last_turbulent = -1;
					d_lists.last_alias = -1;
					d_lists.last_viewmodel = -1;
					d_lists.last_particle = -1;
					d_lists.last_sky = -1;
					d_lists.last_textured_vertex = -1;
					d_lists.last_textured_attribute = -1;
					d_lists.last_colormapped_attribute = -1;
					d_lists.last_colormapped_index16 = -1;
					d_lists.last_colormapped_index32 = -1;
					d_lists.last_colored_vertex = -1;
					d_lists.last_colored_index16 = -1;
					d_lists.last_colored_index32 = -1;
					d_lists.clear_color = -1;
					auto nodrift = cl.nodrift;
					cl.nodrift = true;
					Host_FrameRender();
					cl.nodrift = nodrift;
					if (appState.Scene.hostClearCount != host_clearcount)
					{
						appState.Scene.viewmodelsPerKey.clear();
						appState.Scene.aliasPerKey.clear();
						appState.Scene.colormappedTexCoordsPerKey.clear();
						appState.Scene.colormappedVerticesPerKey.clear();
						forcedDisposeFrontTextures(appState, appState.Scene.viewmodelTextures);
						forcedDisposeFrontTextures(appState, appState.Scene.aliasTextures);
						forcedDisposeFrontBuffers(appState, appState.Scene.colormappedTexCoords);
						forcedDisposeFrontBuffers(appState, appState.Scene.colormappedVertices);
						appState.Scene.texturedFrameCountsPerKeys.clear();
						appState.Scene.hostClearCount = host_clearcount;
					}
					for (auto i = 0; i <= d_lists.last_surface; i++)
					{
						auto& surface = d_lists.surfaces[i];
						auto firstEntry = appState.Scene.texturedFrameCountsPerKeys.find(surface.surface);
						if (firstEntry == appState.Scene.texturedFrameCountsPerKeys.end())
						{
							appState.Scene.texturedFrameCountsPerKeys[surface.surface][surface.entity] = r_framecount;
						}
						else
						{
							auto secondEntry = firstEntry->second.find(surface.entity);
							if (secondEntry == firstEntry->second.end() || surface.created)
							{
								firstEntry->second[surface.entity] = r_framecount;
							}
						}
					}
				}
				else
				{
					r_modelorg_delta[0] = 0;
					r_modelorg_delta[1] = 0;
					r_modelorg_delta[2] = 0;
					Host_FrameRender();
				}
			}
			Host_FrameFinish(host_updated);
		}
		auto matrixIndex = 0;
		for (auto& view : appState.Views)
		{
			VkRect2D screenRect { };
			screenRect.extent.width = view.framebuffer.width;
			screenRect.extent.height = view.framebuffer.height;
			view.index = (view.index + 1) % view.framebuffer.swapChainLength;
			auto& perImage = view.perImage[view.index];
			if (perImage.submitted)
			{
				VK(appState.Device.vkWaitForFences(appState.Device.device, 1, &perImage.fence, VK_TRUE, 1ULL * 1000 * 1000 * 1000));
				VK(appState.Device.vkResetFences(appState.Device.device, 1, &perImage.fence));
				perImage.submitted = false;
			}
			resetCachedBuffers(appState, perImage.sceneMatricesStagingBuffers);
			resetCachedBuffers(appState, perImage.vertices);
			resetCachedBuffers(appState, perImage.attributes);
			resetCachedBuffers(appState, perImage.indices16);
			resetCachedBuffers(appState, perImage.indices32);
			resetCachedBuffers(appState, perImage.uniforms);
			resetCachedBuffers(appState, perImage.stagingBuffers);
			resetCachedTextures(appState, perImage.surfaces);
			resetCachedTextures(appState, perImage.sprites);
			resetCachedTextures(appState, perImage.turbulent);
			resetCachedTextures(appState, perImage.colormaps);
			for (PipelineResources** r = &perImage.pipelineResources; *r != nullptr; )
			{
				(*r)->unusedCount++;
				if ((*r)->unusedCount >= MAX_UNUSED_COUNT)
				{
					PipelineResources *next = (*r)->next;
					deletePipelineDescriptorResources(appState, (*r)->floor);
					deletePipelineDescriptorResources(appState, (*r)->sky);
					deletePipelineDescriptorResources(appState, (*r)->colored);
					deletePipelineDescriptorResources(appState, (*r)->viewmodels);
					deletePipelineDescriptorResources(appState, (*r)->alias);
					deletePipelineDescriptorResources(appState, (*r)->turbulent);
					deletePipelineDescriptorResources(appState, (*r)->sprites);
					deletePipelineDescriptorResources(appState, (*r)->textured);
					deletePipelineDescriptorResources(appState, (*r)->palette);
					delete *r;
					*r = next;
				}
				else
				{
					r = &(*r)->next;
				}
			}
			VK(appState.Device.vkResetCommandBuffer(perImage.commandBuffer, 0));
			VK(appState.Device.vkBeginCommandBuffer(perImage.commandBuffer, &commandBufferBeginInfo));
			VkMemoryBarrier memoryBarrier { };
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr));
			VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &view.framebuffer.startBarriers[view.index]));
			Buffer* stagingBuffer;
			if (perImage.sceneMatricesStagingBuffers.oldBuffers != nullptr)
			{
				stagingBuffer = perImage.sceneMatricesStagingBuffers.oldBuffers;
				perImage.sceneMatricesStagingBuffers.oldBuffers = perImage.sceneMatricesStagingBuffers.oldBuffers->next;
			}
			else
			{
				createStagingBuffer(appState, appState.Scene.matrices.size, stagingBuffer);
			}
			moveBufferToFront(stagingBuffer, perImage.sceneMatricesStagingBuffers);
			VK(appState.Device.vkMapMemory(appState.Device.device, stagingBuffer->memory, 0, stagingBuffer->size, 0, &stagingBuffer->mapped));
			ovrMatrix4f *sceneMatrices = nullptr;
			*((void**)&sceneMatrices) = stagingBuffer->mapped;
			memcpy(sceneMatrices, &appState.ViewMatrices[matrixIndex], appState.Scene.numBuffers * sizeof(ovrMatrix4f));
			memcpy(sceneMatrices + appState.Scene.numBuffers, &appState.ProjectionMatrices[matrixIndex], appState.Scene.numBuffers * sizeof(ovrMatrix4f));
			VC(appState.Device.vkUnmapMemory(appState.Device.device, stagingBuffer->memory));
			stagingBuffer->mapped = nullptr;
			VkBufferMemoryBarrier bufferMemoryBarrier { };
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			bufferMemoryBarrier.buffer = stagingBuffer->buffer;
			bufferMemoryBarrier.size = stagingBuffer->size;
			VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			VkBufferCopy bufferCopy { };
			bufferCopy.size = appState.Scene.matrices.size;
			VC(appState.Device.vkCmdCopyBuffer(perImage.commandBuffer, stagingBuffer->buffer, appState.Scene.matrices.buffer, 1, &bufferCopy));
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			bufferMemoryBarrier.buffer = appState.Scene.matrices.buffer;
			bufferMemoryBarrier.size = appState.Scene.matrices.size;
			VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			auto floorVerticesSize = 0;
			auto texturedVerticesSize = 0;
			auto coloredVerticesSize = 0;
			auto verticesSize = 0;
			auto colormappedVerticesSize = 0;
			auto colormappedTexCoordsSize = 0;
			auto floorAttributesSize = 0;
			auto texturedAttributesSize = 0;
			auto colormappedLightsSize = 0;
			auto vertexTransformSize = 0;
			auto attributesSize = 0;
			auto floorIndicesSize = 0;
			auto colormappedIndices16Size = 0;
			auto coloredIndices16Size = 0;
			auto indices16Size = 0;
			auto colormappedIndices32Size = 0;
			auto coloredIndices32Size = 0;
			auto indices32Size = 0;
			Buffer* vertices = nullptr;
			Buffer* attributes = nullptr;
			Buffer* indices16 = nullptr;
			Buffer* indices32 = nullptr;
			Buffer* uniforms = nullptr;
			if (appState.Mode != AppWorldMode)
			{
				floorVerticesSize = 3 * 4 * sizeof(float);
			}
			texturedVerticesSize = (d_lists.last_textured_vertex + 1) * sizeof(float);
			coloredVerticesSize = (d_lists.last_colored_vertex + 1) * sizeof(float);
			verticesSize = texturedVerticesSize + coloredVerticesSize + floorVerticesSize;
			if (verticesSize > 0 || d_lists.last_alias >= 0 || d_lists.last_viewmodel >= 0)
			{
				for (Buffer** b = &perImage.vertices.oldBuffers; *b != nullptr; b = &(*b)->next)
				{
					if ((*b)->size >= verticesSize && (*b)->size < verticesSize * 2)
					{
						vertices = *b;
						*b = (*b)->next;
						break;
					}
				}
				if (vertices == nullptr)
				{
					createVertexBuffer(appState, verticesSize, vertices);
				}
				moveBufferToFront(vertices, perImage.vertices);
				VK(appState.Device.vkMapMemory(appState.Device.device, vertices->memory, 0, verticesSize, 0, &vertices->mapped));
				if (floorVerticesSize > 0)
				{
					auto mapped = (float*)vertices->mapped;
					(*mapped) = -0.5;
					mapped++;
					(*mapped) = pose.Position.y;
					mapped++;
					(*mapped) = -0.5;
					mapped++;
					(*mapped) = 0.5;
					mapped++;
					(*mapped) = pose.Position.y;
					mapped++;
					(*mapped) = -0.5;
					mapped++;
					(*mapped) = 0.5;
					mapped++;
					(*mapped) = pose.Position.y;
					mapped++;
					(*mapped) = 0.5;
					mapped++;
					(*mapped) = -0.5;
					mapped++;
					(*mapped) = pose.Position.y;
					mapped++;
					(*mapped) = 0.5;
				}
				perImage.texturedVertexBase = floorVerticesSize;
				memcpy((unsigned char*)vertices->mapped + perImage.texturedVertexBase, d_lists.textured_vertices.data(), texturedVerticesSize);
				perImage.coloredVertexBase = perImage.texturedVertexBase + texturedVerticesSize;
				memcpy((unsigned char*)vertices->mapped + perImage.coloredVertexBase, d_lists.colored_vertices.data(), coloredVerticesSize);
				VC(appState.Device.vkUnmapMemory(appState.Device.device, vertices->memory));
				vertices->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				bufferMemoryBarrier.buffer = vertices->buffer;
				bufferMemoryBarrier.size = vertices->size;
				VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				if (d_lists.last_alias >= appState.Scene.aliasVerticesList.size())
				{
					appState.Scene.aliasVerticesList.resize(d_lists.last_alias + 1);
					appState.Scene.aliasTexCoordsList.resize(d_lists.last_alias + 1);
				}
				appState.Scene.newVertices.clear();
				appState.Scene.newTexCoords.clear();
				auto verticesOffset = 0;
				auto texCoordsOffset = 0;
				for (auto i = 0; i <= d_lists.last_alias; i++)
				{
					auto& alias = d_lists.alias[i];
					auto verticesEntry = appState.Scene.colormappedVerticesPerKey.find(alias.vertices);
					if (verticesEntry == appState.Scene.colormappedVerticesPerKey.end())
					{
						auto newEntry = appState.Scene.colormappedVerticesPerKey.emplace(alias.vertices, BufferWithOffset());
						newEntry.first->second.offset = verticesOffset;
						appState.Scene.newVertices.push_back(i);
						appState.Scene.aliasVerticesList[i] = &newEntry.first->second;
						verticesOffset += alias.vertex_count * 2 * 3 * sizeof(float);
					}
					else
					{
						appState.Scene.aliasVerticesList[i] = &verticesEntry->second;
					}
					auto texCoordsEntry = appState.Scene.colormappedTexCoordsPerKey.find(alias.texture_coordinates);
					if (texCoordsEntry == appState.Scene.colormappedTexCoordsPerKey.end())
					{
						auto newEntry = appState.Scene.colormappedTexCoordsPerKey.emplace(alias.texture_coordinates, BufferWithOffset());
						newEntry.first->second.offset = texCoordsOffset;
						appState.Scene.newTexCoords.push_back(i);
						appState.Scene.aliasTexCoordsList[i] = &newEntry.first->second;
						texCoordsOffset += alias.vertex_count * 2 * 2 * sizeof(float);
					}
					else
					{
						appState.Scene.aliasTexCoordsList[i] = &texCoordsEntry->second;
					}
				}
				if (verticesOffset > 0)
				{
					Buffer* buffer = nullptr;
					createVertexBuffer(appState, verticesOffset, buffer);
					moveBufferToFront(buffer, appState.Scene.colormappedVertices);
					colormappedVerticesSize += verticesOffset;
					VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, 0, buffer->size, 0, &buffer->mapped));
					auto mapped = (float*)buffer->mapped;
					for (auto i : appState.Scene.newVertices)
					{
						auto& alias = d_lists.alias[i];
						auto entry = appState.Scene.aliasVerticesList[i];
						auto vertexFromModel = alias.vertices;
						for (auto j = 0; j < alias.vertex_count; j++)
						{
							auto x = (float)(vertexFromModel->v[0]);
							auto y = (float)(vertexFromModel->v[1]);
							auto z = (float)(vertexFromModel->v[2]);
							(*mapped) = x;
							mapped++;
							(*mapped) = z;
							mapped++;
							(*mapped) = -y;
							mapped++;
							(*mapped) = x;
							mapped++;
							(*mapped) = z;
							mapped++;
							(*mapped) = -y;
							mapped++;
							vertexFromModel++;
						}
						entry->buffer = buffer;
					}
					VC(appState.Device.vkUnmapMemory(appState.Device.device, buffer->memory));
					buffer->mapped = nullptr;
					bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
					bufferMemoryBarrier.buffer = buffer->buffer;
					bufferMemoryBarrier.size = buffer->size;
					VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				}
				if (texCoordsOffset > 0)
				{
					Buffer* buffer = nullptr;
					createVertexBuffer(appState, texCoordsOffset, buffer);
					moveBufferToFront(buffer, appState.Scene.colormappedTexCoords);
					colormappedTexCoordsSize += texCoordsOffset;
					VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, 0, buffer->size, 0, &buffer->mapped));
					auto mapped = (float*)buffer->mapped;
					for (auto i : appState.Scene.newTexCoords)
					{
						auto& alias = d_lists.alias[i];
						auto entry = appState.Scene.aliasTexCoordsList[i];
						auto texCoords = alias.texture_coordinates;
						for (auto j = 0; j < alias.vertex_count; j++)
						{
							auto s = (float)(texCoords->s >> 16);
							auto t = (float)(texCoords->t >> 16);
							s /= alias.width;
							t /= alias.height;
							(*mapped) = s;
							mapped++;
							(*mapped) = t;
							mapped++;
							(*mapped) = s + 0.5;
							mapped++;
							(*mapped) = t;
							mapped++;
							texCoords++;
						}
						entry->buffer = buffer;
					}
				}
				if (d_lists.last_viewmodel >= appState.Scene.viewmodelVerticesList.size())
				{
					appState.Scene.viewmodelVerticesList.resize(d_lists.last_viewmodel + 1);
					appState.Scene.viewmodelTexCoordsList.resize(d_lists.last_viewmodel + 1);
				}
				appState.Scene.newVertices.clear();
				appState.Scene.newTexCoords.clear();
				verticesOffset = 0;
				texCoordsOffset = 0;
				for (auto i = 0; i <= d_lists.last_viewmodel; i++)
				{
					auto& viewmodel = d_lists.viewmodel[i];
					auto verticesEntry = appState.Scene.colormappedVerticesPerKey.find(viewmodel.vertices);
					if (verticesEntry == appState.Scene.colormappedVerticesPerKey.end())
					{
						auto newEntry = appState.Scene.colormappedVerticesPerKey.emplace(viewmodel.vertices, BufferWithOffset());
						newEntry.first->second.offset = verticesOffset;
						appState.Scene.newVertices.push_back(i);
						appState.Scene.viewmodelVerticesList[i] = &newEntry.first->second;
						verticesOffset += viewmodel.vertex_count * 2 * 3 * sizeof(float);
					}
					else
					{
						appState.Scene.viewmodelVerticesList[i] = &verticesEntry->second;
					}
					auto texCoordsEntry = appState.Scene.colormappedTexCoordsPerKey.find(viewmodel.texture_coordinates);
					if (texCoordsEntry == appState.Scene.colormappedTexCoordsPerKey.end())
					{
						auto newEntry = appState.Scene.colormappedTexCoordsPerKey.emplace(viewmodel.texture_coordinates, BufferWithOffset());
						newEntry.first->second.offset = texCoordsOffset;
						appState.Scene.newTexCoords.push_back(i);
						appState.Scene.viewmodelTexCoordsList[i] = &newEntry.first->second;
						texCoordsOffset += viewmodel.vertex_count * 2 * 2 * sizeof(float);
					}
					else
					{
						appState.Scene.viewmodelTexCoordsList[i] = &texCoordsEntry->second;
					}
				}
				if (verticesOffset > 0)
				{
					Buffer* buffer = nullptr;
					createVertexBuffer(appState, verticesOffset, buffer);
					moveBufferToFront(buffer, appState.Scene.colormappedVertices);
					colormappedVerticesSize += verticesOffset;
					VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, 0, buffer->size, 0, &buffer->mapped));
					auto mapped = (float*)buffer->mapped;
					for (auto i : appState.Scene.newVertices)
					{
						auto& viewmodel = d_lists.viewmodel[i];
						auto entry = appState.Scene.viewmodelVerticesList[i];
						auto vertexFromModel = viewmodel.vertices;
						for (auto j = 0; j < viewmodel.vertex_count; j++)
						{
							auto x = (float)(vertexFromModel->v[0]);
							auto y = (float)(vertexFromModel->v[1]);
							auto z = (float)(vertexFromModel->v[2]);
							(*mapped) = x;
							mapped++;
							(*mapped) = z;
							mapped++;
							(*mapped) = -y;
							mapped++;
							(*mapped) = x;
							mapped++;
							(*mapped) = z;
							mapped++;
							(*mapped) = -y;
							mapped++;
							vertexFromModel++;
						}
						entry->buffer = buffer;
					}
					VC(appState.Device.vkUnmapMemory(appState.Device.device, buffer->memory));
					buffer->mapped = nullptr;
					bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
					bufferMemoryBarrier.buffer = buffer->buffer;
					bufferMemoryBarrier.size = buffer->size;
					VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				}
				if (texCoordsOffset > 0)
				{
					Buffer* buffer = nullptr;
					createVertexBuffer(appState, texCoordsOffset, buffer);
					moveBufferToFront(buffer, appState.Scene.colormappedTexCoords);
					colormappedTexCoordsSize += texCoordsOffset;
					VK(appState.Device.vkMapMemory(appState.Device.device, buffer->memory, 0, buffer->size, 0, &buffer->mapped));
					auto mapped = (float*)buffer->mapped;
					for (auto i : appState.Scene.newTexCoords)
					{
						auto& viewmodel = d_lists.viewmodel[i];
						auto entry = appState.Scene.viewmodelTexCoordsList[i];
						auto texCoords = viewmodel.texture_coordinates;
						for (auto j = 0; j < viewmodel.vertex_count; j++)
						{
							auto s = (float)(texCoords->s >> 16);
							auto t = (float)(texCoords->t >> 16);
							s /= viewmodel.width;
							t /= viewmodel.height;
							(*mapped) = s;
							mapped++;
							(*mapped) = t;
							mapped++;
							(*mapped) = s + 0.5;
							mapped++;
							(*mapped) = t;
							mapped++;
							texCoords++;
						}
						entry->buffer = buffer;
					}
				}
				if (appState.Mode != AppWorldMode)
				{
					floorAttributesSize = 2 * 4 * sizeof(float);
				}
				texturedAttributesSize = (d_lists.last_textured_attribute + 1) * sizeof(float);
				colormappedLightsSize = (d_lists.last_colormapped_attribute + 1) * sizeof(float);
				vertexTransformSize = 16 * sizeof(float);
				attributesSize = floorAttributesSize + texturedAttributesSize + colormappedLightsSize + vertexTransformSize;
				for (Buffer** b = &perImage.attributes.oldBuffers; *b != nullptr; b = &(*b)->next)
				{
					if ((*b)->size >= attributesSize && (*b)->size < attributesSize * 2)
					{
						attributes = *b;
						*b = (*b)->next;
						break;
					}
				}
				if (attributes == nullptr)
				{
					createVertexBuffer(appState, attributesSize, attributes);
				}
				moveBufferToFront(attributes, perImage.attributes);
				VK(appState.Device.vkMapMemory(appState.Device.device, attributes->memory, 0, attributesSize, 0, &attributes->mapped));
				if (floorAttributesSize > 0)
				{
					auto mapped = (float*)attributes->mapped;
					(*mapped) = 0;
					mapped++;
					(*mapped) = 0;
					mapped++;
					(*mapped) = 1;
					mapped++;
					(*mapped) = 0;
					mapped++;
					(*mapped) = 1;
					mapped++;
					(*mapped) = 1;
					mapped++;
					(*mapped) = 0;
					mapped++;
					(*mapped) = 1;
				}
				perImage.texturedAttributeBase = floorAttributesSize;
				memcpy((unsigned char*)attributes->mapped + perImage.texturedAttributeBase, d_lists.textured_attributes.data(), texturedAttributesSize);
				perImage.colormappedAttributeBase = perImage.texturedAttributeBase + texturedAttributesSize;
				memcpy((unsigned char*)attributes->mapped + perImage.colormappedAttributeBase, d_lists.colormapped_attributes.data(), colormappedLightsSize);
				perImage.vertexTransformBase = perImage.colormappedAttributeBase + colormappedLightsSize;
				auto mapped = (float*)attributes->mapped + perImage.vertexTransformBase / sizeof(float);
				(*mapped) = scale;
				mapped++;
				(*mapped) = 0;
				mapped++;
				(*mapped) = 0;
				mapped++;
				(*mapped) = 0;
				mapped++;
				(*mapped) = 0;
				mapped++;
				(*mapped) = scale;
				mapped++;
				(*mapped) = 0;
				mapped++;
				(*mapped) = 0;
				mapped++;
				(*mapped) = 0;
				mapped++;
				(*mapped) = 0;
				mapped++;
				(*mapped) = scale;
				mapped++;
				(*mapped) = 0;
				mapped++;
				(*mapped) = -r_refdef.vieworg[0] * scale;
				mapped++;
				(*mapped) = -r_refdef.vieworg[2] * scale;
				mapped++;
				(*mapped) = r_refdef.vieworg[1] * scale;
				mapped++;
				(*mapped) = 1;
				VC(appState.Device.vkUnmapMemory(appState.Device.device, attributes->memory));
				attributes->mapped = nullptr;
				bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
				bufferMemoryBarrier.buffer = attributes->buffer;
				bufferMemoryBarrier.size = attributes->size;
				VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				if (appState.Mode != AppWorldMode)
				{
					floorIndicesSize = 6 * sizeof(uint16_t);
				}
				colormappedIndices16Size = (d_lists.last_colormapped_index16 + 1) * sizeof(uint16_t);
				coloredIndices16Size = (d_lists.last_colored_index16 + 1) * sizeof(uint16_t);
				indices16Size = floorIndicesSize + colormappedIndices16Size + coloredIndices16Size;
				if (indices16Size > 0)
				{
					for (Buffer** b = &perImage.indices16.oldBuffers; *b != nullptr; b = &(*b)->next)
					{
						if ((*b)->size >= indices16Size && (*b)->size < indices16Size * 2)
						{
							indices16 = *b;
							*b = (*b)->next;
							break;
						}
					}
					if (indices16 == nullptr)
					{
						createIndexBuffer(appState, indices16Size, indices16);
					}
					moveBufferToFront(indices16, perImage.indices16);
					VK(appState.Device.vkMapMemory(appState.Device.device, indices16->memory, 0, indices16Size, 0, &indices16->mapped));
					if (floorIndicesSize > 0)
					{
						auto mapped = (uint16_t*)indices16->mapped;
						(*mapped) = 0;
						mapped++;
						(*mapped) = 1;
						mapped++;
						(*mapped) = 2;
						mapped++;
						(*mapped) = 2;
						mapped++;
						(*mapped) = 3;
						mapped++;
						(*mapped) = 0;
					}
					perImage.colormappedIndex16Base = floorIndicesSize;
					memcpy((unsigned char*)indices16->mapped + perImage.colormappedIndex16Base, d_lists.colormapped_indices16.data(), colormappedIndices16Size);
					perImage.coloredIndex16Base = perImage.colormappedIndex16Base + colormappedIndices16Size;
					memcpy((unsigned char*)indices16->mapped + perImage.coloredIndex16Base, d_lists.colored_indices16.data(), coloredIndices16Size);
					VC(appState.Device.vkUnmapMemory(appState.Device.device, indices16->memory));
					indices16->mapped = nullptr;
					bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
					bufferMemoryBarrier.buffer = indices16->buffer;
					bufferMemoryBarrier.size = indices16->size;
					VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				}
				colormappedIndices32Size = (d_lists.last_colormapped_index32 + 1) * sizeof(uint32_t);
				coloredIndices32Size = (d_lists.last_colored_index32 + 1) * sizeof(uint32_t);
				indices32Size = colormappedIndices32Size + coloredIndices32Size;
				if (indices32Size > 0)
				{
					for (Buffer** b = &perImage.indices32.oldBuffers; *b != nullptr; b = &(*b)->next)
					{
						if ((*b)->size >= indices32Size && (*b)->size < indices32Size * 2)
						{
							indices32 = *b;
							*b = (*b)->next;
							break;
						}
					}
					if (indices32 == nullptr)
					{
						createIndexBuffer(appState, indices32Size, indices32);
					}
					moveBufferToFront(indices32, perImage.indices32);
					VK(appState.Device.vkMapMemory(appState.Device.device, indices32->memory, 0, indices32Size, 0, &indices32->mapped));
					memcpy(indices32->mapped, d_lists.colormapped_indices32.data(), colormappedIndices32Size);
					perImage.coloredIndex32Base = colormappedIndices32Size;
					memcpy((unsigned char*)indices32->mapped + perImage.coloredIndex32Base, d_lists.colored_indices32.data(), coloredIndices32Size);
					VC(appState.Device.vkUnmapMemory(appState.Device.device, indices32->memory));
					indices32->mapped = nullptr;
					bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
					bufferMemoryBarrier.buffer = indices32->buffer;
					bufferMemoryBarrier.size = indices32->size;
					VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				}
				if (d_lists.last_sky >= 0)
				{
					if (perImage.uniforms.oldBuffers != nullptr)
					{
						uniforms = perImage.uniforms.oldBuffers;
						perImage.uniforms.oldBuffers = perImage.uniforms.oldBuffers->next;
					}
					else
					{
						createUniformBuffer(appState, 13 * sizeof(float), uniforms);
					}
					moveBufferToFront(uniforms, perImage.uniforms);
					VK(appState.Device.vkMapMemory(appState.Device.device, uniforms->memory, 0, uniforms->size, 0, &uniforms->mapped));
					auto mapped = (float*)uniforms->mapped;
					auto rotation = ovrMatrix4f_CreateFromQuaternion(&orientation);
					(*mapped) = -rotation.M[0][2];
					mapped++;
					(*mapped) = rotation.M[2][2];
					mapped++;
					(*mapped) = -rotation.M[1][2];
					mapped++;
					(*mapped) = rotation.M[0][0];
					mapped++;
					(*mapped) = -rotation.M[2][0];
					mapped++;
					(*mapped) = rotation.M[1][0];
					mapped++;
					(*mapped) = rotation.M[0][1];
					mapped++;
					(*mapped) = -rotation.M[2][1];
					mapped++;
					(*mapped) = rotation.M[1][1];
					mapped++;
					(*mapped) = eyeTextureWidth;
					mapped++;
					(*mapped) = eyeTextureHeight;
					mapped++;
					(*mapped) = std::max(eyeTextureWidth, eyeTextureHeight);
					mapped++;
					(*mapped) = skytime*skyspeed;
					VC(appState.Device.vkUnmapMemory(appState.Device.device, uniforms->memory));
					uniforms->mapped = nullptr;
					bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
					bufferMemoryBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
					bufferMemoryBarrier.buffer = uniforms->buffer;
					bufferMemoryBarrier.size = uniforms->size;
					VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
				}
			}
			auto stagingBufferSize = 0;
			perImage.paletteOffset = -1;
			if (perImage.palette == nullptr || perImage.paletteChanged != pal_changed)
			{
				if (perImage.palette == nullptr)
				{
					createTexture(appState, perImage.commandBuffer, 256, 1, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, perImage.palette);
				}
				perImage.paletteOffset = stagingBufferSize;
				stagingBufferSize += 1024;
				perImage.paletteChanged = pal_changed;
			}
			perImage.host_colormapOffset = -1;
			if (host_colormap.size() > 0 && perImage.host_colormap == nullptr)
			{
				createTexture(appState, perImage.commandBuffer, 256, 64, VK_FORMAT_R8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, perImage.host_colormap);
				perImage.host_colormapOffset = stagingBufferSize;
				stagingBufferSize += 16384;
			}
			if (d_lists.last_surface >= appState.Scene.surfaceOffsets.size())
			{
				appState.Scene.surfaceOffsets.resize(d_lists.last_surface + 1);
				appState.Scene.surfaceList.resize(d_lists.last_surface + 1);
			}
			for (auto i = 0; i <= d_lists.last_surface; i++)
			{
				auto& surface = d_lists.surfaces[i];
				Texture* texture = nullptr;
				for (Texture** t = &perImage.surfaces.oldTextures; *t != nullptr; t = &(*t)->next)
				{
					if ((*t)->key == surface.surface && (*t)->key2 == surface.entity)
					{
						texture = *t;
						*t = (*t)->next;
						break;
					}
				}
				if (texture == nullptr)
				{
					auto mipCount = (int)(std::floor(std::log2(std::max(surface.width, surface.height)))) + 1;
					createTexture(appState, perImage.commandBuffer, surface.width, surface.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, texture);
					texture->key = surface.surface;
					texture->key2 = surface.entity;
					texture->frameCount = r_framecount;
					appState.Scene.surfaceOffsets[i] = stagingBufferSize;
					stagingBufferSize += surface.size;
				}
				else
				{
					auto firstEntry = appState.Scene.texturedFrameCountsPerKeys.find(surface.surface);
					auto secondEntry = firstEntry->second.find(surface.entity);
					if (texture->frameCount == secondEntry->second)
					{
						appState.Scene.surfaceOffsets[i] = -1;
					}
					else
					{
						texture->frameCount = secondEntry->second;
						appState.Scene.surfaceOffsets[i] = stagingBufferSize;
						stagingBufferSize += surface.size;
					}
				}
				appState.Scene.surfaceList[i] = texture;
				moveTextureToFront(texture, perImage.surfaces);
			}
			if (d_lists.last_sprite >= appState.Scene.spriteOffsets.size())
			{
				appState.Scene.spriteOffsets.resize(d_lists.last_sprite + 1);
				appState.Scene.spriteList.resize(d_lists.last_sprite + 1);
			}
			for (auto i = 0; i <= d_lists.last_sprite; i++)
			{
				auto& sprite = d_lists.sprites[i];
				Texture* texture = nullptr;
				for (Texture** t = &perImage.sprites.oldTextures; *t != nullptr; t = &(*t)->next)
				{
					if ((*t)->width == sprite.width && (*t)->height == sprite.height)
					{
						texture = *t;
						*t = (*t)->next;
						break;
					}
				}
				if (texture == nullptr)
				{
					auto mipCount = (int)(std::floor(std::log2(std::max(sprite.width, sprite.height)))) + 1;
					createTexture(appState, perImage.commandBuffer, sprite.width, sprite.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, texture);
				}
				appState.Scene.spriteOffsets[i] = stagingBufferSize;
				stagingBufferSize += sprite.size;
				appState.Scene.spriteList[i] = texture;
				moveTextureToFront(texture, perImage.sprites);
			}
			if (d_lists.last_turbulent >= appState.Scene.turbulentOffsets.size())
			{
				appState.Scene.turbulentOffsets.resize(d_lists.last_turbulent + 1);
				appState.Scene.turbulentList.resize(d_lists.last_turbulent + 1);
			}
			appState.Scene.turbulentPerKey.clear();
			for (auto i = 0; i <= d_lists.last_turbulent; i++)
			{
				auto& turbulent = d_lists.turbulent[i];
				auto entry = appState.Scene.turbulentPerKey.find(turbulent.texture);
				if (perImage.hostClearCount == host_clearcount && entry != appState.Scene.turbulentPerKey.end())
				{
					appState.Scene.turbulentOffsets[i] = -1;
					appState.Scene.turbulentList[i] = entry->second;
					entry->second->unusedCount = 0;
				}
				else if (perImage.hostClearCount == host_clearcount)
				{
					Texture* texture = nullptr;
					for (Texture** t = &perImage.turbulent.oldTextures; *t != nullptr; t = &(*t)->next)
					{
						if ((*t)->key == turbulent.texture)
						{
							texture = *t;
							*t = (*t)->next;
							break;
						}
					}
					if (texture == nullptr)
					{
						auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
						createTexture(appState, perImage.commandBuffer, turbulent.width, turbulent.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, texture);
						texture->key = turbulent.texture;
						appState.Scene.turbulentOffsets[i] = stagingBufferSize;
						stagingBufferSize += turbulent.size;
					}
					else
					{
						appState.Scene.turbulentOffsets[i] = -1;
					}
					appState.Scene.turbulentList[i] = texture;
					appState.Scene.turbulentPerKey.insert({ texture->key, texture });
					moveTextureToFront(texture, perImage.turbulent);
				}
				else
				{
					Texture* texture;
					auto mipCount = (int)(std::floor(std::log2(std::max(turbulent.width, turbulent.height)))) + 1;
					createTexture(appState, perImage.commandBuffer, turbulent.width, turbulent.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, texture);
					texture->key = turbulent.texture;
					appState.Scene.turbulentOffsets[i] = stagingBufferSize;
					stagingBufferSize += turbulent.size;
					appState.Scene.turbulentList[i] = texture;
					appState.Scene.turbulentPerKey.insert({ texture->key, texture });
					moveTextureToFront(texture, perImage.turbulent);
				}
			}
			if (d_lists.last_alias >= appState.Scene.aliasOffsets.size())
			{
				appState.Scene.aliasOffsets.resize(d_lists.last_alias + 1);
				appState.Scene.aliasList.resize(d_lists.last_alias + 1);
				appState.Scene.aliasColormapOffsets.resize(d_lists.last_alias + 1);
				appState.Scene.aliasColormapList.resize(d_lists.last_alias + 1);
			}
			for (auto i = 0; i <= d_lists.last_alias; i++)
			{
				auto& alias = d_lists.alias[i];
				auto entry = appState.Scene.aliasPerKey.find(alias.data);
				if (entry == appState.Scene.aliasPerKey.end())
				{
					Texture* texture = nullptr;
					for (Texture** t = &appState.Scene.aliasTextures.oldTextures; *t != nullptr; t = &(*t)->next)
					{
						if ((*t)->width == alias.width && (*t)->height == alias.height)
						{
							texture = *t;
							*t = (*t)->next;
							break;
						}
					}
					if (texture == nullptr)
					{
						auto mipCount = (int)(std::floor(std::log2(std::max(alias.width, alias.height)))) + 1;
						createTexture(appState, perImage.commandBuffer, alias.width, alias.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, texture);
						texture->key = alias.data;
					}
					appState.Scene.aliasOffsets[i] = stagingBufferSize;
					stagingBufferSize += alias.size;
					moveTextureToFront(texture, appState.Scene.aliasTextures);
					appState.Scene.aliasPerKey.insert({ alias.data, texture });
					appState.Scene.aliasList[i] = texture;
				}
				else
				{
					appState.Scene.aliasOffsets[i] = -1;
					appState.Scene.aliasList[i] = entry->second;
				}
			}
			for (auto i = 0; i <= d_lists.last_alias; i++)
			{
				auto& alias = d_lists.alias[i];
				if (alias.is_host_colormap)
				{
					appState.Scene.aliasColormapOffsets[i] = -1;
					appState.Scene.aliasColormapList[i] = perImage.host_colormap;
				}
				else
				{
					appState.Scene.aliasColormapOffsets[i] = stagingBufferSize;
					stagingBufferSize += 16384;
				}
			}
			if (d_lists.last_viewmodel >= appState.Scene.viewmodelOffsets.size())
			{
				appState.Scene.viewmodelOffsets.resize(d_lists.last_viewmodel + 1);
				appState.Scene.viewmodelList.resize(d_lists.last_viewmodel + 1);
				appState.Scene.viewmodelColormapOffsets.resize(d_lists.last_viewmodel + 1);
				appState.Scene.viewmodelColormapList.resize(d_lists.last_viewmodel + 1);
			}
			for (auto i = 0; i <= d_lists.last_viewmodel; i++)
			{
				auto& viewmodel = d_lists.viewmodel[i];
				auto entry = appState.Scene.viewmodelsPerKey.find(viewmodel.data);
				if (entry == appState.Scene.viewmodelsPerKey.end())
				{
					Texture* texture = nullptr;
					for (Texture** t = &appState.Scene.viewmodelTextures.oldTextures; *t != nullptr; t = &(*t)->next)
					{
						if ((*t)->width == viewmodel.width && (*t)->height == viewmodel.height)
						{
							texture = *t;
							*t = (*t)->next;
							break;
						}
					}
					if (texture == nullptr)
					{
						auto mipCount = (int)(std::floor(std::log2(std::max(viewmodel.width, viewmodel.height)))) + 1;
						createTexture(appState, perImage.commandBuffer, viewmodel.width, viewmodel.height, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, texture);
						texture->key = viewmodel.data;
					}
					appState.Scene.viewmodelOffsets[i] = stagingBufferSize;
					stagingBufferSize += viewmodel.size;
					moveTextureToFront(texture, appState.Scene.viewmodelTextures);
					appState.Scene.viewmodelsPerKey.insert({ viewmodel.data, texture });
					appState.Scene.viewmodelList[i] = texture;
				}
				else
				{
					appState.Scene.viewmodelOffsets[i] = -1;
					appState.Scene.viewmodelList[i] = entry->second;
				}
			}
			for (auto i = 0; i <= d_lists.last_viewmodel; i++)
			{
				auto& viewmodel = d_lists.viewmodel[i];
				if (viewmodel.is_host_colormap)
				{
					appState.Scene.viewmodelColormapOffsets[i] = -1;
					appState.Scene.viewmodelColormapList[i] = perImage.host_colormap;
				}
				else
				{
					appState.Scene.viewmodelColormapOffsets[i] = stagingBufferSize;
					stagingBufferSize += 16384;
				}
			}
			perImage.hostClearCount = host_clearcount;
			if (d_lists.last_sky >= 0)
			{
				perImage.skyOffset = stagingBufferSize;
				stagingBufferSize += 16384;
			}
			else
			{
				perImage.skyOffset = -1;
			}
			int floorSize;
			if (perImage.floor == nullptr)
			{
				perImage.floorOffset = stagingBufferSize;
				floorSize = appState.FloorWidth * appState.FloorHeight * sizeof(uint32_t);
				stagingBufferSize += floorSize;
			}
			else
			{
				perImage.floorOffset = -1;
				floorSize = 0;
			}
#if defined(_DEBUG)
			__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "**** %i; %i, %i, %i = %i; %i; %i; %i, %i, %i, %i = %i; %i, %i, %i = %i; %i, %i = %i",
					stagingBufferSize,
					floorVerticesSize, texturedVerticesSize, coloredVerticesSize, verticesSize,
					colormappedVerticesSize,
					colormappedTexCoordsSize,
					floorAttributesSize, texturedAttributesSize, colormappedLightsSize, vertexTransformSize, attributesSize,
					floorIndicesSize, colormappedIndices16Size, coloredIndices16Size, indices16Size,
					colormappedIndices32Size, coloredIndices32Size, indices32Size);
#endif
			stagingBuffer = nullptr;
			if (stagingBufferSize > 0)
			{
				for (Buffer** b = &perImage.stagingBuffers.oldBuffers; *b != nullptr; b = &(*b)->next)
				{
					if ((*b)->size >= stagingBufferSize && (*b)->size < stagingBufferSize * 2)
					{
						stagingBuffer = *b;
						*b = (*b)->next;
						break;
					}
				}
				if (stagingBuffer == nullptr)
				{
					createStagingStorageBuffer(appState, stagingBufferSize, stagingBuffer);
				}
				moveBufferToFront(stagingBuffer, perImage.stagingBuffers);
				VK(appState.Device.vkMapMemory(appState.Device.device, stagingBuffer->memory, 0, stagingBufferSize, 0, &stagingBuffer->mapped));
				auto offset = 0;
				if (perImage.paletteOffset >= 0)
				{
					memcpy(stagingBuffer->mapped, d_8to24table, 1024);
					offset += 1024;
				}
				if (perImage.host_colormapOffset >= 0)
				{
					memcpy(((unsigned char*)stagingBuffer->mapped) + offset, host_colormap.data(), 16384);
					offset += 16384;
				}
				for (auto i = 0; i <= d_lists.last_surface; i++)
				{
					if (appState.Scene.surfaceOffsets[i] >= 0)
					{
						auto& surface = d_lists.surfaces[i];
						memcpy(((unsigned char*)stagingBuffer->mapped) + offset, surface.data.data(), surface.size);
						offset += surface.size;
					}
				}
				for (auto i = 0; i <= d_lists.last_sprite; i++)
				{
					auto& sprite = d_lists.sprites[i];
					memcpy(((unsigned char*)stagingBuffer->mapped) + offset, sprite.data.data(), sprite.size);
					offset += sprite.size;
				}
				for (auto i = 0; i <= d_lists.last_turbulent; i++)
				{
					if (appState.Scene.turbulentOffsets[i] >= 0)
					{
						auto& turbulent = d_lists.turbulent[i];
						memcpy(((unsigned char*)stagingBuffer->mapped) + offset, turbulent.data, turbulent.size);
						offset += turbulent.size;
					}
				}
				for (auto i = 0; i <= d_lists.last_alias; i++)
				{
					auto& alias = d_lists.alias[i];
					if (appState.Scene.aliasOffsets[i] >= 0)
					{
						memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.data, alias.size);
						offset += alias.size;
					}
					if (!alias.is_host_colormap)
					{
						memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap.data(), 16384);
						offset += 16384;
					}
				}
				for (auto i = 0; i <= d_lists.last_viewmodel; i++)
				{
					auto& viewmodel = d_lists.viewmodel[i];
					if (appState.Scene.viewmodelOffsets[i] >= 0)
					{
						memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.data, viewmodel.size);
						offset += viewmodel.size;
					}
					if (!viewmodel.is_host_colormap)
					{
						memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.colormap.data(), 16384);
						offset += 16384;
					}
				}
				if (d_lists.last_sky >= 0)
				{
					auto source = r_skysource;
					auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
					for (auto i = 0; i < 128; i++)
					{
						memcpy(target, source, 128);
						source += 256;
						target += 128;
					}
					offset += 16384;
				}
				if (perImage.floorOffset >= 0)
				{
					memcpy(((unsigned char*)stagingBuffer->mapped) + offset, appState.FloorData.data(), floorSize);
				}
				VC(appState.Device.vkUnmapMemory(appState.Device.device, stagingBuffer->memory));
				VkMappedMemoryRange mappedMemoryRange { };
				mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				mappedMemoryRange.memory = stagingBuffer->memory;
				VC(appState.Device.vkFlushMappedMemoryRanges(appState.Device.device, 1, &mappedMemoryRange));
			}
			if (stagingBufferSize > 0)
			{
				if (perImage.paletteOffset >= 0)
				{
					fillTexture(appState, perImage.palette, stagingBuffer, perImage.paletteOffset, perImage.commandBuffer);
				}
				if (perImage.host_colormapOffset >= 0)
				{
					fillTexture(appState, perImage.host_colormap, stagingBuffer, perImage.host_colormapOffset, perImage.commandBuffer);
				}
				for (auto i = 0; i <= d_lists.last_surface; i++)
				{
					if (appState.Scene.surfaceOffsets[i] >= 0)
					{
						fillMipmappedTexture(appState, appState.Scene.surfaceList[i], stagingBuffer, appState.Scene.surfaceOffsets[i], perImage.commandBuffer);
					}
				}
				for (auto i = 0; i <= d_lists.last_sprite; i++)
				{
					fillMipmappedTexture(appState, appState.Scene.spriteList[i], stagingBuffer, appState.Scene.spriteOffsets[i], perImage.commandBuffer);
				}
				for (auto i = 0; i <= d_lists.last_turbulent; i++)
				{
					if (appState.Scene.turbulentOffsets[i] >= 0)
					{
						auto& turbulent = d_lists.turbulent[i];
						auto texture = appState.Scene.turbulentList[i];
						fillMipmappedTexture(appState, texture, stagingBuffer, appState.Scene.turbulentOffsets[i], perImage.commandBuffer);
					}
				}
				for (auto i = 0; i <= d_lists.last_alias; i++)
				{
					auto& alias = d_lists.alias[i];
					if (appState.Scene.aliasOffsets[i] >= 0)
					{
						auto texture = appState.Scene.aliasList[i];
						fillMipmappedTexture(appState, texture, stagingBuffer, appState.Scene.aliasOffsets[i], perImage.commandBuffer);
					}
					if (!alias.is_host_colormap)
					{
						if (perImage.colormaps.oldTextures != nullptr)
						{
							appState.Scene.aliasColormapList[i] = perImage.colormaps.oldTextures;
							perImage.colormaps.oldTextures = appState.Scene.aliasColormapList[i]->next;
						}
						else
						{
							createTexture(appState, perImage.commandBuffer, 256, 64, VK_FORMAT_R8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, appState.Scene.aliasColormapList[i]);
						}
						fillTexture(appState, appState.Scene.aliasColormapList[i], stagingBuffer, appState.Scene.aliasColormapOffsets[i], perImage.commandBuffer);
						moveTextureToFront(appState.Scene.aliasColormapList[i], perImage.colormaps);
					}
				}
				for (auto i = 0; i <= d_lists.last_viewmodel; i++)
				{
					auto& viewmodel = d_lists.viewmodel[i];
					if (appState.Scene.viewmodelOffsets[i] >= 0)
					{
						auto texture = appState.Scene.viewmodelList[i];
						fillMipmappedTexture(appState, texture, stagingBuffer, appState.Scene.viewmodelOffsets[i], perImage.commandBuffer);
					}
					if (!viewmodel.is_host_colormap)
					{
						if (perImage.colormaps.oldTextures != nullptr)
						{
							appState.Scene.viewmodelColormapList[i] = perImage.colormaps.oldTextures;
							perImage.colormaps.oldTextures = appState.Scene.viewmodelColormapList[i]->next;
						}
						else
						{
							createTexture(appState, perImage.commandBuffer, 256, 64, VK_FORMAT_R8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, appState.Scene.viewmodelColormapList[i]);
						}
						fillTexture(appState, appState.Scene.viewmodelColormapList[i], stagingBuffer, appState.Scene.viewmodelColormapOffsets[i], perImage.commandBuffer);
						moveTextureToFront(appState.Scene.viewmodelColormapList[i], perImage.colormaps);
					}
				}
				if (d_lists.last_sky >= 0)
				{
					if (perImage.sky == nullptr)
					{
						auto mipCount = (int)(std::floor(std::log2(std::max(128, 128)))) + 1;
						createTexture(appState, perImage.commandBuffer, 128, 128, VK_FORMAT_R8_UNORM, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, perImage.sky);
					}
					fillMipmappedTexture(appState, perImage.sky, stagingBuffer, perImage.skyOffset, perImage.commandBuffer);
				}
				if (appState.Mode != AppWorldMode && perImage.floor == nullptr)
				{
					createTexture(appState, perImage.commandBuffer, appState.FloorWidth, appState.FloorHeight, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, perImage.floor);
					fillTexture(appState, perImage.floor, stagingBuffer, perImage.floorOffset, perImage.commandBuffer);
				}
			}
			double clearR;
			double clearG;
			double clearB;
			double clearA;
			if (d_lists.clear_color >= 0)
			{
				auto color = d_8to24table[d_lists.clear_color];
				clearR = (color & 255) / 255.0f;
				clearG = (color >> 8 & 255) / 255.0f;
				clearB = (color >> 16 & 255) / 255.0f;
				clearA = (color >> 24) / 255.0f;
			}
			else
			{
				clearR = 0;
				clearG = 0;
				clearB = 0;
				clearA = 1;
			}
			uint32_t clearValueCount = 0;
			VkClearValue clearValues[3] { };
			clearValues[clearValueCount].color.float32[0] = clearR;
			clearValues[clearValueCount].color.float32[1] = clearG;
			clearValues[clearValueCount].color.float32[2] = clearB;
			clearValues[clearValueCount].color.float32[3] = clearA;
			clearValueCount++;
			clearValues[clearValueCount].color.float32[0] = clearR;
			clearValues[clearValueCount].color.float32[1] = clearG;
			clearValues[clearValueCount].color.float32[2] = clearB;
			clearValues[clearValueCount].color.float32[3] = clearA;
			clearValueCount++;
			clearValues[clearValueCount].depthStencil.depth = 1;
			clearValueCount++;
			VkRenderPassBeginInfo renderPassBeginInfo { };
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = appState.RenderPass;
			renderPassBeginInfo.framebuffer = view.framebuffer.framebuffers[view.index];
			renderPassBeginInfo.renderArea = screenRect;
			renderPassBeginInfo.clearValueCount = clearValueCount;
			renderPassBeginInfo.pClearValues = clearValues;
			VC(appState.Device.vkCmdBeginRenderPass(perImage.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE));
			VkViewport viewport;
			viewport.x = (float) screenRect.offset.x;
			viewport.y = (float) screenRect.offset.y;
			viewport.width = (float) screenRect.extent.width;
			viewport.height = (float) screenRect.extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			VC(appState.Device.vkCmdSetViewport(perImage.commandBuffer, 0, 1, &viewport));
			VC(appState.Device.vkCmdSetScissor(perImage.commandBuffer, 0, 1, &screenRect));
			if (verticesSize > 0)
			{
				auto resources = new PipelineResources();
				memset(resources, 0, sizeof(PipelineResources));
				resources->next = perImage.pipelineResources;
				perImage.pipelineResources = resources;
				VkDescriptorPoolSize poolSizes[2] { };
				VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { };
				descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
				descriptorPoolCreateInfo.maxSets = 1;
				descriptorPoolCreateInfo.pPoolSizes = poolSizes;
				VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { };
				descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorSetAllocateInfo.descriptorSetCount = 1;
				VkDescriptorBufferInfo bufferInfo[3] { };
				VkDescriptorImageInfo textureInfo[2] { };
				textureInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				textureInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				VkWriteDescriptorSet writes[3] { };
				writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[0].descriptorCount = 1;
				writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[1].descriptorCount = 1;
				writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[2].descriptorCount = 1;
				poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSizes[0].descriptorCount = 1;
				descriptorPoolCreateInfo.poolSizeCount = 1;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &resources->palette.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = resources->palette.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.palette;
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &resources->palette.descriptorSet));
				resources->palette.created = true;
				textureInfo[0].sampler = perImage.palette->sampler;
				textureInfo[0].imageView = perImage.palette->view;
				writes[0].dstSet = resources->palette.descriptorSet;
				writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[0].pImageInfo = textureInfo;
				VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
				if (appState.Mode == AppWorldMode)
				{
					VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer, &perImage.texturedVertexBase));
					VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &attributes->buffer, &perImage.texturedAttributeBase));
					VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 2, 1, &attributes->buffer, &perImage.vertexTransformBase));
					VC(appState.Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.textured.pipeline));
					VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.textured.pipelineLayout, 0, 1, &resources->palette.descriptorSet, 0, nullptr));
					poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					poolSizes[0].descriptorCount = 1;
					poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					poolSizes[1].descriptorCount = 1;
					descriptorPoolCreateInfo.poolSizeCount = 2;
					VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &resources->textured.descriptorPool));
					descriptorSetAllocateInfo.descriptorPool = resources->textured.descriptorPool;
					descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.textured.descriptorSetLayout;
					VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &resources->textured.descriptorSet));
					resources->textured.created = true;
					bufferInfo[0].buffer = appState.Scene.matrices.buffer;
					bufferInfo[0].range = appState.Scene.matrices.size;
					bufferInfo[1].offset = 0;
					bufferInfo[2].offset = 0;
					textureInfo[0].sampler = perImage.palette->sampler;
					textureInfo[0].imageView = perImage.palette->view;
					writes[0].dstSet = resources->textured.descriptorSet;
					writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					writes[0].pBufferInfo = bufferInfo;
					writes[1].dstBinding = 2;
					writes[1].dstSet = resources->textured.descriptorSet;
					writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writes[1].pImageInfo = textureInfo;
					VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
					for (auto i = 0; i <= d_lists.last_surface; i++)
					{
						auto& surface = d_lists.surfaces[i];
						float origin[3];
						origin[0] = surface.origin_x;
						origin[1] = surface.origin_z;
						origin[2] = -surface.origin_y;
						VC(appState.Device.vkCmdPushConstants(perImage.commandBuffer, appState.Scene.textured.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 3 * sizeof(float), origin));
						auto texture = appState.Scene.surfaceList[i];
						textureInfo[0].sampler = texture->sampler;
						textureInfo[0].imageView = texture->view;
						writes[0].dstBinding = 1;
						writes[0].dstSet = resources->textured.descriptorSet;
						writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						writes[0].pImageInfo = textureInfo;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.textured.pipelineLayout, 1, 1, &resources->textured.descriptorSet, 0, nullptr));
						VC(appState.Device.vkCmdDraw(perImage.commandBuffer, surface.count, 1, surface.first_vertex, 0));
					}
					VC(appState.Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipeline));
					VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 0, 1, &resources->palette.descriptorSet, 0, nullptr));
					VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &resources->sprites.descriptorPool));
					descriptorSetAllocateInfo.descriptorPool = resources->sprites.descriptorPool;
					descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.sprites.descriptorSetLayout;
					VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &resources->sprites.descriptorSet));
					resources->sprites.created = true;
					for (auto i = 0; i <= d_lists.last_sprite; i++)
					{
						auto& sprite = d_lists.sprites[i];
						auto texture = appState.Scene.spriteList[i];
						textureInfo[0].sampler = texture->sampler;
						textureInfo[0].imageView = texture->view;
						writes[0].dstBinding = 1;
						writes[0].dstSet = resources->textured.descriptorSet;
						writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						writes[0].pImageInfo = textureInfo;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.textured.pipelineLayout, 1, 1, &resources->textured.descriptorSet, 0, nullptr));
						VC(appState.Device.vkCmdDraw(perImage.commandBuffer, sprite.count, 1, sprite.first_vertex, 0));
					}
					VC(appState.Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipeline));
					VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 0, 1, &resources->palette.descriptorSet, 0, nullptr));
					poolSizes[0].descriptorCount = 2;
					VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &resources->turbulent.descriptorPool));
					descriptorSetAllocateInfo.descriptorPool = resources->turbulent.descriptorPool;
					descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.turbulent.descriptorSetLayout;
					VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &resources->turbulent.descriptorSet));
					resources->turbulent.created = true;
					bufferInfo[0].buffer = appState.Scene.matrices.buffer;
					bufferInfo[0].range = appState.Scene.matrices.size;
					textureInfo[0].sampler = perImage.palette->sampler;
					textureInfo[0].imageView = perImage.palette->view;
					writes[0].dstBinding = 0;
					writes[0].dstSet = resources->turbulent.descriptorSet;
					writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					writes[0].pBufferInfo = bufferInfo;
					writes[1].dstBinding = 2;
					writes[1].dstSet = resources->turbulent.descriptorSet;
					writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writes[1].pImageInfo = textureInfo;
					VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
					float originPlusTime[4];
					originPlusTime[3] = (float)cl.time;
					for (auto i = 0; i <= d_lists.last_turbulent; i++)
					{
						auto& turbulent = d_lists.turbulent[i];
						originPlusTime[0] = turbulent.origin_x;
						originPlusTime[1] = turbulent.origin_z;
						originPlusTime[2] = -turbulent.origin_y;
						VC(appState.Device.vkCmdPushConstants(perImage.commandBuffer, appState.Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), originPlusTime));
						auto texture = appState.Scene.turbulentList[i];
						textureInfo[0].sampler = texture->sampler;
						textureInfo[0].imageView = texture->view;
						writes[0].dstBinding = 1;
						writes[0].dstSet = resources->turbulent.descriptorSet;
						writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						writes[0].pImageInfo = textureInfo;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &resources->turbulent.descriptorSet, 0, nullptr));
						VC(appState.Device.vkCmdDraw(perImage.commandBuffer, turbulent.count, 1, turbulent.first_vertex, 0));
					}
					if (d_lists.last_alias >= 0)
					{
						VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 3, 1, &attributes->buffer, &perImage.vertexTransformBase));
						VC(appState.Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipeline));
						VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 0, 1, &resources->palette.descriptorSet, 0, nullptr));
						poolSizes[0].descriptorCount = 1;
						poolSizes[1].descriptorCount = 2;
						VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &resources->alias.descriptorPool));
						descriptorSetAllocateInfo.descriptorPool = resources->alias.descriptorPool;
						descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.alias.descriptorSetLayout;
						VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &resources->alias.descriptorSet));
						resources->alias.created = true;
						bufferInfo[0].buffer = appState.Scene.matrices.buffer;
						bufferInfo[0].range = appState.Scene.matrices.size;
						writes[0].dstBinding = 0;
						writes[0].dstSet = resources->alias.descriptorSet;
						writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writes[0].pBufferInfo = bufferInfo;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						if (indices16 != nullptr)
						{
							VC(appState.Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices16->buffer, perImage.colormappedIndex16Base, VK_INDEX_TYPE_UINT16));
							for (auto i = 0; i <= d_lists.last_alias; i++)
							{
								auto& alias = d_lists.alias[i];
								if (alias.first_index16 < 0)
								{
									continue;
								}
								auto vertices = appState.Scene.aliasVerticesList[i];
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer->buffer, &vertices->offset));
								auto texCoords = appState.Scene.aliasTexCoordsList[i];
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &texCoords->buffer->buffer, &texCoords->offset));
								VkDeviceSize attributeOffset = perImage.colormappedAttributeBase + alias.first_attribute * sizeof(float);
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
								float transforms[16];
								transforms[0] = alias.transform[0][0];
								transforms[1] = alias.transform[2][0];
								transforms[2] = -alias.transform[1][0];
								transforms[3] = 0;
								transforms[4] = alias.transform[0][2];
								transforms[5] = alias.transform[2][2];
								transforms[6] = -alias.transform[1][2];
								transforms[7] = 0;
								transforms[8] = -alias.transform[0][1];
								transforms[9] = -alias.transform[2][1];
								transforms[10] = alias.transform[1][1];
								transforms[11] = 0;
								transforms[12] = alias.transform[0][3];
								transforms[13] = alias.transform[2][3];
								transforms[14] = -alias.transform[1][3];
								transforms[15] = 1;
								VC(appState.Device.vkCmdPushConstants(perImage.commandBuffer, appState.Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), &transforms));
								auto texture = appState.Scene.aliasList[i];
								auto colormap = appState.Scene.aliasColormapList[i];
								textureInfo[0].sampler = texture->sampler;
								textureInfo[0].imageView = texture->view;
								textureInfo[1].sampler = colormap->sampler;
								textureInfo[1].imageView = colormap->view;
								writes[0].dstBinding = 1;
								writes[0].dstSet = resources->alias.descriptorSet;
								writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
								writes[0].pImageInfo = textureInfo;
								writes[1].dstBinding = 2;
								writes[1].dstSet = resources->alias.descriptorSet;
								writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
								writes[1].pImageInfo = textureInfo + 1;
								VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
								VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &resources->alias.descriptorSet, 0, nullptr));
								VC(appState.Device.vkCmdDrawIndexed(perImage.commandBuffer, alias.count, 1, alias.first_index16, 0, 0));
							}
						}
						if (indices32 != nullptr)
						{
							VC(appState.Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
							for (auto i = 0; i <= d_lists.last_alias; i++)
							{
								auto& alias = d_lists.alias[i];
								if (alias.first_index32 < 0)
								{
									continue;
								}
								auto vertices = appState.Scene.aliasVerticesList[i];
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer->buffer, &vertices->offset));
								auto texCoords = appState.Scene.aliasTexCoordsList[i];
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &texCoords->buffer->buffer, &texCoords->offset));
								VkDeviceSize attributeOffset = perImage.colormappedAttributeBase + alias.first_attribute * sizeof(float);
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
								float transforms[16];
								transforms[0] = alias.transform[0][0];
								transforms[1] = alias.transform[2][0];
								transforms[2] = -alias.transform[1][0];
								transforms[3] = 0;
								transforms[4] = alias.transform[0][2];
								transforms[5] = alias.transform[2][2];
								transforms[6] = -alias.transform[1][2];
								transforms[7] = 0;
								transforms[8] = -alias.transform[0][1];
								transforms[9] = -alias.transform[2][1];
								transforms[10] = alias.transform[1][1];
								transforms[11] = 0;
								transforms[12] = alias.transform[0][3];
								transforms[13] = alias.transform[2][3];
								transforms[14] = -alias.transform[1][3];
								transforms[15] = 1;
								VC(appState.Device.vkCmdPushConstants(perImage.commandBuffer, appState.Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), &transforms));
								auto texture = appState.Scene.aliasList[i];
								auto colormap = appState.Scene.aliasColormapList[i];
								textureInfo[0].sampler = texture->sampler;
								textureInfo[0].imageView = texture->view;
								textureInfo[1].sampler = colormap->sampler;
								textureInfo[1].imageView = colormap->view;
								writes[0].dstBinding = 1;
								writes[0].dstSet = resources->alias.descriptorSet;
								writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
								writes[0].pImageInfo = textureInfo;
								writes[1].dstBinding = 2;
								writes[1].dstSet = resources->alias.descriptorSet;
								writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
								writes[1].pImageInfo = textureInfo + 1;
								VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
								VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &resources->alias.descriptorSet, 0, nullptr));
								VC(appState.Device.vkCmdDrawIndexed(perImage.commandBuffer, alias.count, 1, alias.first_index32, 0, 0));
							}
						}
					}
					if (d_lists.last_viewmodel >= 0)
					{
						VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 3, 1, &attributes->buffer, &perImage.vertexTransformBase));
						VC(appState.Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipeline));
						VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 0, 1, &resources->palette.descriptorSet, 0, nullptr));
						poolSizes[0].descriptorCount = 1;
						poolSizes[1].descriptorCount = 2;
						VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &resources->viewmodels.descriptorPool));
						descriptorSetAllocateInfo.descriptorPool = resources->viewmodels.descriptorPool;
						descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.viewmodel.descriptorSetLayout;
						VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &resources->viewmodels.descriptorSet));
						resources->viewmodels.created = true;
						bufferInfo[0].buffer = appState.Scene.matrices.buffer;
						bufferInfo[0].range = appState.Scene.matrices.size;
						writes[0].dstBinding = 0;
						writes[0].dstSet = resources->viewmodels.descriptorSet;
						writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writes[0].pBufferInfo = bufferInfo;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						float transformsPlusTint[24];
						if (appState.NearViewModel)
						{
							transformsPlusTint[16] = vpn[0] / scale;
							transformsPlusTint[17] = vpn[2] / scale;
							transformsPlusTint[18] = -vpn[1] / scale;
							transformsPlusTint[19] = 0;
							transformsPlusTint[20] = 1;
							transformsPlusTint[21] = 1;
							transformsPlusTint[22] = 1;
							transformsPlusTint[23] = 1;
						}
						else
						{
							transformsPlusTint[16] = 1 / scale;
							transformsPlusTint[17] = 0;
							transformsPlusTint[18] = 0;
							transformsPlusTint[19] = 8;
							transformsPlusTint[20] = 1;
							transformsPlusTint[21] = 0;
							transformsPlusTint[22] = 0;
							transformsPlusTint[23] = 0.7 + 0.3 * sin(cl.time * M_PI);
						}
						if (indices16 != nullptr)
						{
							VC(appState.Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices16->buffer, perImage.colormappedIndex16Base, VK_INDEX_TYPE_UINT16));
							for (auto i = 0; i <= d_lists.last_viewmodel; i++)
							{
								auto& viewmodel = d_lists.viewmodel[i];
								if (viewmodel.first_index16 < 0)
								{
									continue;
								}
								auto vertices = appState.Scene.viewmodelVerticesList[i];
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer->buffer, &vertices->offset));
								auto texCoords = appState.Scene.viewmodelTexCoordsList[i];
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &texCoords->buffer->buffer, &texCoords->offset));
								VkDeviceSize attributeOffset = perImage.colormappedAttributeBase + viewmodel.first_attribute * sizeof(float);
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
								transformsPlusTint[0] = viewmodel.transform[0][0];
								transformsPlusTint[1] = viewmodel.transform[2][0];
								transformsPlusTint[2] = -viewmodel.transform[1][0];
								transformsPlusTint[3] = 0;
								transformsPlusTint[4] = viewmodel.transform[0][2];
								transformsPlusTint[5] = viewmodel.transform[2][2];
								transformsPlusTint[6] = -viewmodel.transform[1][2];
								transformsPlusTint[7] = 0;
								transformsPlusTint[8] = -viewmodel.transform[0][1];
								transformsPlusTint[9] = -viewmodel.transform[2][1];
								transformsPlusTint[10] = viewmodel.transform[1][1];
								transformsPlusTint[11] = 0;
								transformsPlusTint[12] = viewmodel.transform[0][3];
								transformsPlusTint[13] = viewmodel.transform[2][3];
								transformsPlusTint[14] = -viewmodel.transform[1][3];
								transformsPlusTint[15] = 1;
								VC(appState.Device.vkCmdPushConstants(perImage.commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), &transformsPlusTint));
								auto texture = appState.Scene.viewmodelList[i];
								auto colormap = appState.Scene.viewmodelColormapList[i];
								textureInfo[0].sampler = texture->sampler;
								textureInfo[0].imageView = texture->view;
								textureInfo[1].sampler = colormap->sampler;
								textureInfo[1].imageView = colormap->view;
								writes[0].dstBinding = 1;
								writes[0].dstSet = resources->viewmodels.descriptorSet;
								writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
								writes[0].pImageInfo = textureInfo;
								writes[1].dstBinding = 2;
								writes[1].dstSet = resources->viewmodels.descriptorSet;
								writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
								writes[1].pImageInfo = textureInfo + 1;
								VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
								VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &resources->viewmodels.descriptorSet, 0, nullptr));
								VC(appState.Device.vkCmdDrawIndexed(perImage.commandBuffer, viewmodel.count, 1, viewmodel.first_index16, 0, 0));
							}
						}
						if (indices32 != nullptr)
						{
							VC(appState.Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32));
							for (auto i = 0; i <= d_lists.last_viewmodel; i++)
							{
								auto& viewmodel = d_lists.viewmodel[i];
								if (viewmodel.first_index32 < 0)
								{
									continue;
								}
								auto vertices = appState.Scene.viewmodelVerticesList[i];
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer->buffer, &vertices->offset));
								auto texCoords = appState.Scene.viewmodelTexCoordsList[i];
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &texCoords->buffer->buffer, &texCoords->offset));
								VkDeviceSize attributeOffset = perImage.colormappedAttributeBase + viewmodel.first_attribute * sizeof(float);
								VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 2, 1, &attributes->buffer, &attributeOffset));
								transformsPlusTint[0] = viewmodel.transform[0][0];
								transformsPlusTint[1] = viewmodel.transform[2][0];
								transformsPlusTint[2] = -viewmodel.transform[1][0];
								transformsPlusTint[3] = 0;
								transformsPlusTint[4] = viewmodel.transform[0][2];
								transformsPlusTint[5] = viewmodel.transform[2][2];
								transformsPlusTint[6] = -viewmodel.transform[1][2];
								transformsPlusTint[7] = 0;
								transformsPlusTint[8] = -viewmodel.transform[0][1];
								transformsPlusTint[9] = -viewmodel.transform[2][1];
								transformsPlusTint[10] = viewmodel.transform[1][1];
								transformsPlusTint[11] = 0;
								transformsPlusTint[12] = viewmodel.transform[0][3];
								transformsPlusTint[13] = viewmodel.transform[2][3];
								transformsPlusTint[14] = -viewmodel.transform[1][3];
								transformsPlusTint[15] = 1;
								VC(appState.Device.vkCmdPushConstants(perImage.commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), &transformsPlusTint));
								auto texture = appState.Scene.viewmodelList[i];
								auto colormap = appState.Scene.viewmodelColormapList[i];
								textureInfo[0].sampler = texture->sampler;
								textureInfo[0].imageView = texture->view;
								textureInfo[1].sampler = colormap->sampler;
								textureInfo[1].imageView = colormap->view;
								writes[0].dstBinding = 1;
								writes[0].dstSet = resources->viewmodels.descriptorSet;
								writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
								writes[0].pImageInfo = textureInfo;
								writes[1].dstBinding = 2;
								writes[1].dstSet = resources->viewmodels.descriptorSet;
								writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
								writes[1].pImageInfo = textureInfo + 1;
								VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
								VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &resources->viewmodels.descriptorSet, 0, nullptr));
								VC(appState.Device.vkCmdDrawIndexed(perImage.commandBuffer, viewmodel.count, 1, viewmodel.first_index32, 0, 0));
							}
						}
					}
					if (d_lists.last_particle >= 0)
					{
						VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer, &perImage.coloredVertexBase));
						VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &attributes->buffer, &perImage.vertexTransformBase));
						VC(appState.Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipeline));
						poolSizes[0].descriptorCount = 1;
						descriptorPoolCreateInfo.poolSizeCount = 1;
						VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &resources->colored.descriptorPool));
						descriptorSetAllocateInfo.descriptorPool = resources->colored.descriptorPool;
						descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.colored.descriptorSetLayout;
						VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &resources->colored.descriptorSet));
						resources->colored.created = true;
						bufferInfo[0].buffer = appState.Scene.matrices.buffer;
						bufferInfo[0].range = appState.Scene.matrices.size;
						writes[0].dstBinding = 0;
						writes[0].dstSet = resources->colored.descriptorSet;
						writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writes[0].pBufferInfo = bufferInfo;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 1, writes, 0, nullptr));
						VkDescriptorSet descriptorSets[2];
						descriptorSets[0] = resources->palette.descriptorSet;
						descriptorSets[1] = resources->colored.descriptorSet;
						VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipelineLayout, 0, 2, descriptorSets, 0, nullptr));
						if (indices16 != nullptr)
						{
							VC(appState.Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices16->buffer, perImage.coloredIndex16Base, VK_INDEX_TYPE_UINT16));
							for (auto i = 0; i <= d_lists.last_particle; i++)
							{
								auto& particle = d_lists.particles[i];
								if (particle.first_index16 < 0)
								{
									continue;
								}
								float color = (float)particle.color;
								VC(appState.Device.vkCmdPushConstants(perImage.commandBuffer, appState.Scene.colored.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &color));
								VC(appState.Device.vkCmdDrawIndexed(perImage.commandBuffer, particle.count, 1, particle.first_index16, 0, 0));
							}
						}
						if (indices32 != nullptr)
						{
							VC(appState.Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices32->buffer, perImage.coloredIndex32Base, VK_INDEX_TYPE_UINT32));
							for (auto i = 0; i <= d_lists.last_particle; i++)
							{
								auto& particle = d_lists.particles[i];
								if (particle.first_index32 < 0)
								{
									continue;
								}
								float color = (float)particle.color;
								VC(appState.Device.vkCmdPushConstants(perImage.commandBuffer, appState.Scene.colored.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &color));
								VC(appState.Device.vkCmdDrawIndexed(perImage.commandBuffer, particle.count, 1, particle.first_index32, 0, 0));
							}
						}
					}
					if (d_lists.last_sky >= 0)
					{
						VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer, &perImage.texturedVertexBase));
						VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &attributes->buffer, &perImage.texturedAttributeBase));
						VC(appState.Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipeline));
						poolSizes[0].descriptorCount = 2;
						poolSizes[1].descriptorCount = 1;
						descriptorPoolCreateInfo.poolSizeCount = 2;
						VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &resources->sky.descriptorPool));
						descriptorSetAllocateInfo.descriptorPool = resources->sky.descriptorPool;
						descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.sky.descriptorSetLayout;
						VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &resources->sky.descriptorSet));
						resources->sky.created = true;
						textureInfo[0].sampler = perImage.sky->sampler;
						textureInfo[0].imageView = perImage.sky->view;
						bufferInfo[0].buffer = appState.Scene.matrices.buffer;
						bufferInfo[0].range = appState.Scene.matrices.size;
						bufferInfo[1].buffer = uniforms->buffer;
						bufferInfo[1].offset = 0;
						bufferInfo[1].range = 13 * sizeof(float);
						writes[0].dstBinding = 0;
						writes[0].dstSet = resources->sky.descriptorSet;
						writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writes[0].pBufferInfo = bufferInfo;
						writes[1].dstBinding = 1;
						writes[1].dstSet = resources->sky.descriptorSet;
						writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
						writes[1].pImageInfo = textureInfo;
						writes[2].dstBinding = 2;
						writes[2].dstSet = resources->sky.descriptorSet;
						writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
						writes[2].pBufferInfo = bufferInfo + 1;
						VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 3, writes, 0, nullptr));
						VkDescriptorSet descriptorSets[2];
						descriptorSets[0] = resources->palette.descriptorSet;
						descriptorSets[1] = resources->sky.descriptorSet;
						VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipelineLayout, 0, 2, descriptorSets, 0, nullptr));
						for (auto i = 0; i <= d_lists.last_sky; i++)
						{
							auto& sky = d_lists.sky[i];
							VC(appState.Device.vkCmdDraw(perImage.commandBuffer, sky.count, 1, sky.first_vertex, 0));
						}
					}
				}
				else
				{
					VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer, &noOffset));
					VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &attributes->buffer, &noOffset));
					VC(appState.Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipeline));
					poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					poolSizes[0].descriptorCount = 1;
					poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					poolSizes[1].descriptorCount = 1;
					descriptorPoolCreateInfo.poolSizeCount = 2;
					VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &resources->floor.descriptorPool));
					descriptorSetAllocateInfo.descriptorPool = resources->floor.descriptorPool;
					descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.floor.descriptorSetLayout;
					VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &resources->floor.descriptorSet));
					resources->floor.created = true;
					bufferInfo[0].buffer = appState.Scene.matrices.buffer;
					bufferInfo[0].range = appState.Scene.matrices.size;
					textureInfo[0].sampler = perImage.floor->sampler;
					textureInfo[0].imageView = perImage.floor->view;
					writes[0].dstBinding = 0;
					writes[0].dstSet = resources->floor.descriptorSet;
					writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					writes[0].pBufferInfo = bufferInfo;
					writes[1].dstBinding = 1;
					writes[1].dstSet = resources->floor.descriptorSet;
					writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writes[1].pImageInfo = textureInfo;
					VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
					VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 1, &resources->floor.descriptorSet, 0, nullptr));
					VC(appState.Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices16->buffer, 0, VK_INDEX_TYPE_UINT16));
					VC(appState.Device.vkCmdDrawIndexed(perImage.commandBuffer, 6, 1, 0, 0, 0));
				}
			}
			VC(appState.Device.vkCmdEndRenderPass(perImage.commandBuffer));
			VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &view.framebuffer.endBarriers[view.index]));
			VK(appState.Device.vkEndCommandBuffer(perImage.commandBuffer));
			VkSubmitInfo submitInfo { };
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &perImage.commandBuffer;
			VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &submitInfo, perImage.fence));
			perImage.submitted = true;
			matrixIndex++;
		}
		if (appState.Mode == AppScreenMode)
		{
			auto consoleIndex = 0;
			auto consoleIndexCache = 0;
			auto screenIndex = 0;
			auto targetIndex = 0;
			auto y = 0;
			while (y < vid_height)
			{
				auto x = 0;
				while (x < vid_width)
				{
					auto entry = con_buffer[consoleIndex];
					if (entry == 255)
					{
						do
						{
							appState.Screen.Data[targetIndex] = d_8to24table[vid_buffer[screenIndex]];
							screenIndex++;
							targetIndex++;
							x++;
						} while ((x % 3) != 0);
					}
					else
					{
						do
						{
							appState.Screen.Data[targetIndex] = d_8to24table[entry];
							screenIndex++;
							targetIndex++;
							x++;
						} while ((x % 3) != 0);
					}
					consoleIndex++;
				}
				y++;
				if ((y % 3) == 0)
				{
					consoleIndexCache = consoleIndex;
				}
				else
				{
					consoleIndex = consoleIndexCache;
				}
			}
			VK(appState.Device.vkMapMemory(appState.Device.device, appState.Screen.Buffer.memory, 0, appState.Screen.Buffer.size, 0, &appState.Screen.Buffer.mapped));
			memcpy(appState.Screen.Buffer.mapped, appState.Screen.Data.data(), appState.Screen.Data.size() * sizeof(uint32_t));
			VC(appState.Device.vkUnmapMemory(appState.Device.device, appState.Screen.Buffer.memory));
			appState.Screen.Buffer.mapped = nullptr;
			VK(appState.Device.vkBeginCommandBuffer(appState.Screen.CommandBuffer, &commandBufferBeginInfo));
			VkBufferImageCopy region { };
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.width = appState.ScreenWidth;
			region.imageExtent.height = appState.ScreenHeight;
			region.imageExtent.depth = 1;
			VC(appState.Device.vkCmdCopyBufferToImage(appState.Screen.CommandBuffer, appState.Screen.Buffer.buffer, appState.Screen.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
			VK(appState.Device.vkEndCommandBuffer(appState.Screen.CommandBuffer));
			VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &appState.Screen.SubmitInfo, VK_NULL_HANDLE));
		}
		else if (appState.Mode == AppWorldMode)
		{
			VkRect2D screenRect { };
			screenRect.extent.width = appState.Console.View.framebuffer.width;
			screenRect.extent.height = appState.Console.View.framebuffer.height;
			appState.Console.View.index = (appState.Console.View.index + 1) % appState.Console.View.framebuffer.swapChainLength;
			auto& perImage = appState.Console.View.perImage[appState.Console.View.index];
			if (perImage.submitted)
			{
				VK(appState.Device.vkWaitForFences(appState.Device.device, 1, &perImage.fence, VK_TRUE, 1ULL * 1000 * 1000 * 1000));
				VK(appState.Device.vkResetFences(appState.Device.device, 1, &perImage.fence));
				perImage.submitted = false;
			}
			resetCachedBuffers(appState, perImage.vertices);
			resetCachedBuffers(appState, perImage.indices);
			resetCachedBuffers(appState, perImage.stagingBuffers);
			for (ConsolePipelineResources** r = &perImage.pipelineResources; *r != nullptr; )
			{
				(*r)->unusedCount++;
				if ((*r)->unusedCount >= MAX_UNUSED_COUNT)
				{
					ConsolePipelineResources *next = (*r)->next;
					deletePipelineDescriptorResources(appState, (*r)->descriptorResources);
					delete *r;
					*r = next;
				}
				else
				{
					r = &(*r)->next;
				}
			}
			VK(appState.Device.vkResetCommandBuffer(perImage.commandBuffer, 0));
			VK(appState.Device.vkBeginCommandBuffer(perImage.commandBuffer, &commandBufferBeginInfo));
			VkMemoryBarrier memoryBarrier { };
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr));
			VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.Console.View.framebuffer.startBarriers[appState.Console.View.index]));
			Buffer* vertices = nullptr;
			if (perImage.vertices.oldBuffers != nullptr)
			{
				vertices = perImage.vertices.oldBuffers;
				perImage.vertices.oldBuffers = perImage.vertices.oldBuffers->next;
			}
			else
			{
				createStagingBuffer(appState, appState.ConsoleVertices.size() * sizeof(float), vertices);
			}
			moveBufferToFront(vertices, perImage.vertices);
			VK(appState.Device.vkMapMemory(appState.Device.device, vertices->memory, 0, vertices->size, 0, &vertices->mapped));
			memcpy(vertices->mapped, appState.ConsoleVertices.data(), vertices->size);
			VC(appState.Device.vkUnmapMemory(appState.Device.device, vertices->memory));
			vertices->mapped = nullptr;
			VkBufferMemoryBarrier bufferMemoryBarrier { };
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			bufferMemoryBarrier.buffer = vertices->buffer;
			bufferMemoryBarrier.size = vertices->size;
			VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			Buffer* indices = nullptr;
			if (perImage.indices.oldBuffers != nullptr)
			{
				indices = perImage.indices.oldBuffers;
				perImage.indices.oldBuffers = perImage.indices.oldBuffers->next;
			}
			else
			{
				createStagingBuffer(appState, appState.ConsoleIndices.size() * sizeof(uint16_t), indices);
			}
			moveBufferToFront(indices, perImage.indices);
			VK(appState.Device.vkMapMemory(appState.Device.device, indices->memory, 0, indices->size, 0, &indices->mapped));
			memcpy(indices->mapped, appState.ConsoleIndices.data(), indices->size);
			VC(appState.Device.vkUnmapMemory(appState.Device.device, indices->memory));
			indices->mapped = nullptr;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
			bufferMemoryBarrier.buffer = indices->buffer;
			bufferMemoryBarrier.size = indices->size;
			VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &bufferMemoryBarrier, 0, nullptr));
			auto stagingBufferSize = 0;
			perImage.paletteOffset = -1;
			if (perImage.palette == nullptr || perImage.paletteChanged != pal_changed)
			{
				if (perImage.palette == nullptr)
				{
					createTexture(appState, perImage.commandBuffer, 256, 1, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, perImage.palette);
					perImage.paletteOffset = stagingBufferSize;
					stagingBufferSize += 1024;
				}
				else if (perImage.paletteChanged != pal_changed)
				{
					perImage.paletteOffset = stagingBufferSize;
					stagingBufferSize += 1024;
				}
				perImage.paletteChanged = pal_changed;
			}
			if (perImage.texture == nullptr)
			{
				createTexture(appState, perImage.commandBuffer, appState.ConsoleWidth, appState.ConsoleHeight, VK_FORMAT_R8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, perImage.texture);
			}
			perImage.textureOffset = stagingBufferSize;
			stagingBufferSize += con_buffer.size();
			Buffer* stagingBuffer = nullptr;
			for (Buffer** b = &perImage.stagingBuffers.oldBuffers; *b != nullptr; b = &(*b)->next)
			{
				if ((*b)->size >= stagingBufferSize && (*b)->size < stagingBufferSize * 2)
				{
					stagingBuffer = *b;
					*b = (*b)->next;
					break;
				}
			}
			if (stagingBuffer == nullptr)
			{
				createStagingBuffer(appState, stagingBufferSize, stagingBuffer);
			}
			moveBufferToFront(stagingBuffer, perImage.stagingBuffers);
			VK(appState.Device.vkMapMemory(appState.Device.device, stagingBuffer->memory, 0, stagingBufferSize, 0, &stagingBuffer->mapped));
			auto offset = 0;
			if (perImage.paletteOffset >= 0)
			{
				memcpy(stagingBuffer->mapped, d_8to24table, 1024);
				offset += 1024;
			}
			if (perImage.textureOffset >= 0)
			{
				memcpy(((unsigned char*)stagingBuffer->mapped) + offset, con_buffer.data(), con_buffer.size());
			}
			VC(appState.Device.vkUnmapMemory(appState.Device.device, stagingBuffer->memory));
			VkMappedMemoryRange mappedMemoryRange { };
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedMemoryRange.memory = stagingBuffer->memory;
			VC(appState.Device.vkFlushMappedMemoryRanges(appState.Device.device, 1, &mappedMemoryRange));
			if (perImage.paletteOffset >= 0)
			{
				fillTexture(appState, perImage.palette, stagingBuffer, perImage.paletteOffset, perImage.commandBuffer);
			}
			if (perImage.textureOffset >= 0)
			{
				fillTexture(appState, perImage.texture, stagingBuffer, perImage.textureOffset, perImage.commandBuffer);
			}
			VkClearValue clearValues[3] { };
			VkRenderPassBeginInfo renderPassBeginInfo { };
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = appState.Console.RenderPass;
			renderPassBeginInfo.framebuffer = appState.Console.View.framebuffer.framebuffers[appState.Console.View.index];
			renderPassBeginInfo.renderArea = screenRect;
			renderPassBeginInfo.clearValueCount = 3;
			renderPassBeginInfo.pClearValues = clearValues;
			VC(appState.Device.vkCmdBeginRenderPass(perImage.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE));
			VkViewport viewport;
			viewport.x = (float) screenRect.offset.x;
			viewport.y = (float) screenRect.offset.y;
			viewport.width = (float) screenRect.extent.width;
			viewport.height = (float) screenRect.extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			VC(appState.Device.vkCmdSetViewport(perImage.commandBuffer, 0, 1, &viewport));
			VC(appState.Device.vkCmdSetScissor(perImage.commandBuffer, 0, 1, &screenRect));
			VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer, &noOffset));
			VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &vertices->buffer, &noOffset));
			VC(appState.Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.console.pipeline));
			auto resources = new ConsolePipelineResources();
			memset(resources, 0, sizeof(ConsolePipelineResources));
			VkDescriptorPoolSize poolSize;
			poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSize.descriptorCount = 2;
			VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { };
			descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			descriptorPoolCreateInfo.maxSets = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			descriptorPoolCreateInfo.pPoolSizes = &poolSize;
			VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &resources->descriptorResources.descriptorPool));
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { };
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = resources->descriptorResources.descriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.console.descriptorSetLayout;
			VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &resources->descriptorResources.descriptorSet));
			resources->descriptorResources.created = true;
			resources->next = perImage.pipelineResources;
			perImage.pipelineResources = resources;
			VkDescriptorImageInfo textureInfo[2] { };
			textureInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureInfo[0].sampler = perImage.texture->sampler;
			textureInfo[0].imageView = perImage.texture->view;
			textureInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureInfo[1].sampler = perImage.palette->sampler;
			textureInfo[1].imageView = perImage.palette->view;
			VkWriteDescriptorSet writes[2] { };
			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = resources->descriptorResources.descriptorSet;
			writes[0].descriptorCount = 1;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].pImageInfo = textureInfo;
			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstBinding = 1;
			writes[1].dstSet = resources->descriptorResources.descriptorSet;
			writes[1].descriptorCount = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[1].pImageInfo = textureInfo + 1;
			VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
			VC(appState.Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices->buffer, noOffset, VK_INDEX_TYPE_UINT16));
			VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.console.pipelineLayout, 0, 1, &resources->descriptorResources.descriptorSet, 0, nullptr));
			VC(appState.Device.vkCmdDrawIndexed(perImage.commandBuffer, appState.ConsoleIndices.size(), 1, 0, 0, 0));
			VC(appState.Device.vkCmdEndRenderPass(perImage.commandBuffer));
			VC(appState.Device.vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.Console.View.framebuffer.endBarriers[appState.Console.View.index]));
			VK(appState.Device.vkEndCommandBuffer(perImage.commandBuffer));
			VkSubmitInfo submitInfo { };
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &perImage.commandBuffer;
			VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &submitInfo, perImage.fence));
			perImage.submitted = true;
		}
		else if (appState.Mode == AppNoGameDataMode)
		{
			VK(appState.Device.vkMapMemory(appState.Device.device, appState.Screen.Buffer.memory, 0, appState.Screen.Buffer.size, 0, &appState.Screen.Buffer.mapped));
			memcpy(appState.Screen.Buffer.mapped, appState.NoGameDataData.data(), appState.NoGameDataData.size() * sizeof(uint32_t));
			VC(appState.Device.vkUnmapMemory(appState.Device.device, appState.Screen.Buffer.memory));
			appState.Screen.Buffer.mapped = nullptr;
			VK(appState.Device.vkBeginCommandBuffer(appState.Screen.CommandBuffer, &commandBufferBeginInfo));
			VkBufferImageCopy region { };
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent.width = appState.ScreenWidth;
			region.imageExtent.height = appState.ScreenHeight;
			region.imageExtent.depth = 1;
			VC(appState.Device.vkCmdCopyBufferToImage(appState.Screen.CommandBuffer, appState.Screen.Buffer.buffer, appState.Screen.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
			VK(appState.Device.vkEndCommandBuffer(appState.Screen.CommandBuffer));
			VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &appState.Screen.SubmitInfo, VK_NULL_HANDLE));
		}
		ovrLayerProjection2 worldLayer = vrapi_DefaultLayerProjection2();
		worldLayer.HeadPose = tracking.HeadPose;
		for (auto i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; i++)
		{
			auto view = (appState.Views.size() == 1 ? 0 : i);
			worldLayer.Textures[i].ColorSwapChain = appState.Views[view].framebuffer.colorTextureSwapChain;
			worldLayer.Textures[i].SwapChainIndex = appState.Views[view].index;
			worldLayer.Textures[i].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&tracking.Eye[i].ProjectionMatrix);
		}
		worldLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
		std::vector<ovrLayerCylinder2> cylinderLayers;
		if (appState.Mode == AppWorldMode)
		{
			auto console = vrapi_DefaultLayerCylinder2();
			console.Header.ColorScale.x = 1;
			console.Header.ColorScale.y = 1;
			console.Header.ColorScale.z = 1;
			console.Header.ColorScale.w = 1;
			console.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA;
			console.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA;
			console.HeadPose = tracking.HeadPose;
			const float density = 4500;
			float rotateYaw = appState.Yaw;
			float rotatePitch = appState.Pitch;
			const float radius = 1;
			const ovrVector3f translation = { tracking.HeadPose.Pose.Position.x, tracking.HeadPose.Pose.Position.y, tracking.HeadPose.Pose.Position.z };
			const ovrMatrix4f scaleMatrix = ovrMatrix4f_CreateScale(radius, radius * (float) appState.ScreenHeight * VRAPI_PI / density, radius);
			const ovrMatrix4f transMatrix = ovrMatrix4f_CreateTranslation(translation.x, translation.y, translation.z);
			const ovrMatrix4f rotXMatrix = ovrMatrix4f_CreateRotation(rotatePitch, 0, 0);
			const ovrMatrix4f rotYMatrix = ovrMatrix4f_CreateRotation(0, rotateYaw, 0);
			const ovrMatrix4f m0 = ovrMatrix4f_Multiply(&rotXMatrix, &scaleMatrix);
			const ovrMatrix4f m1 = ovrMatrix4f_Multiply(&rotYMatrix, &m0);
			ovrMatrix4f cylinderTransform = ovrMatrix4f_Multiply(&transMatrix, &m1);
			float circScale = density * 0.5 / appState.ScreenWidth;
			float circBias = -circScale * (0.5 * (1 - 1 / circScale));
			for (auto i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; i++)
			{
				ovrMatrix4f modelViewMatrix = ovrMatrix4f_Multiply(&tracking.Eye[i].ViewMatrix, &cylinderTransform);
				console.Textures[i].TexCoordsFromTanAngles = ovrMatrix4f_Inverse(&modelViewMatrix);
				console.Textures[i].ColorSwapChain = appState.Console.SwapChain;
				console.Textures[i].SwapChainIndex = appState.Console.View.index;
				const float texScaleX = circScale;
				const float texBiasX = circBias;
				const float texScaleY = 0.5;
				const float texBiasY = -texScaleY * (0.5 * (1 - 1 / texScaleY));
				console.Textures[i].TextureMatrix.M[0][0] = texScaleX;
				console.Textures[i].TextureMatrix.M[0][2] = texBiasX;
				console.Textures[i].TextureMatrix.M[1][1] = texScaleY;
				console.Textures[i].TextureMatrix.M[1][2] = texBiasY;
				console.Textures[i].TextureRect.width = 1;
				console.Textures[i].TextureRect.height = 1;
			}
			cylinderLayers.push_back(console);
		}
		else
		{
			auto screen = vrapi_DefaultLayerCylinder2();
			screen.Header.ColorScale.x = 1;
			screen.Header.ColorScale.y = 1;
			screen.Header.ColorScale.z = 1;
			screen.Header.ColorScale.w = 1;
			screen.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA;
			screen.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA;
			screen.HeadPose = tracking.HeadPose;
			const float density = 4500;
			float rotateYaw = 0;
			float rotatePitch = 0;
			const float radius = 1;
			ovrMatrix4f scaleMatrix = ovrMatrix4f_CreateScale(radius, radius * (float) appState.ScreenHeight * VRAPI_PI / density, radius);
			ovrMatrix4f rotXMatrix = ovrMatrix4f_CreateRotation(rotatePitch, 0.0f, 0.0f);
			ovrMatrix4f rotYMatrix = ovrMatrix4f_CreateRotation(0.0f, rotateYaw, 0.0f);
			ovrMatrix4f m0 = ovrMatrix4f_Multiply(&rotXMatrix, &scaleMatrix);
			ovrMatrix4f cylinderTransform = ovrMatrix4f_Multiply(&rotYMatrix, &m0);
			float circScale = density * 0.5f / appState.ScreenWidth;
			float circBias = -circScale * (0.5f * (1.0f - 1.0f / circScale));
			for (auto i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; i++)
			{
				ovrMatrix4f modelViewMatrix = ovrMatrix4f_Multiply(&tracking.Eye[i].ViewMatrix, &cylinderTransform);
				screen.Textures[i].TexCoordsFromTanAngles = ovrMatrix4f_Inverse(&modelViewMatrix);
				screen.Textures[i].ColorSwapChain = appState.Screen.SwapChain;
				screen.Textures[i].SwapChainIndex = 0;
				const float texScaleX = circScale;
				const float texBiasX = circBias;
				const float texScaleY = 0.5;
				const float texBiasY = -texScaleY * (0.5 * (1 - 1 / texScaleY));
				screen.Textures[i].TextureMatrix.M[0][0] = texScaleX;
				screen.Textures[i].TextureMatrix.M[0][2] = texBiasX;
				screen.Textures[i].TextureMatrix.M[1][1] = texScaleY;
				screen.Textures[i].TextureMatrix.M[1][2] = texBiasY;
				screen.Textures[i].TextureRect.width = 1;
				screen.Textures[i].TextureRect.height = 1;
			}
			cylinderLayers.push_back(screen);
			auto leftArrows = vrapi_DefaultLayerCylinder2();
			leftArrows.Header.ColorScale.x = 1;
			leftArrows.Header.ColorScale.y = 1;
			leftArrows.Header.ColorScale.z = 1;
			leftArrows.Header.ColorScale.w = 1;
			leftArrows.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA;
			leftArrows.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA;
			leftArrows.HeadPose = tracking.HeadPose;
			rotateYaw = 2 * M_PI / 3;
			scaleMatrix = ovrMatrix4f_CreateScale(radius, radius * (float) appState.ScreenHeight * VRAPI_PI / density, radius);
			rotXMatrix = ovrMatrix4f_CreateRotation(rotatePitch, 0.0f, 0.0f);
			rotYMatrix = ovrMatrix4f_CreateRotation(0.0f, rotateYaw, 0.0f);
			m0 = ovrMatrix4f_Multiply(&rotXMatrix, &scaleMatrix);
			cylinderTransform = ovrMatrix4f_Multiply(&rotYMatrix, &m0);
			for (auto i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; i++)
			{
				ovrMatrix4f modelViewMatrix = ovrMatrix4f_Multiply(&tracking.Eye[i].ViewMatrix, &cylinderTransform);
				leftArrows.Textures[i].TexCoordsFromTanAngles = ovrMatrix4f_Inverse(&modelViewMatrix);
				leftArrows.Textures[i].ColorSwapChain = appState.LeftArrows.SwapChain;
				leftArrows.Textures[i].SwapChainIndex = 0;
				const float texScaleX = circScale;
				const float texBiasX = circBias;
				const float texScaleY = 0.5;
				const float texBiasY = -texScaleY * (0.5 * (1 - 1 / texScaleY));
				leftArrows.Textures[i].TextureMatrix.M[0][0] = texScaleX;
				leftArrows.Textures[i].TextureMatrix.M[0][2] = texBiasX;
				leftArrows.Textures[i].TextureMatrix.M[1][1] = texScaleY;
				leftArrows.Textures[i].TextureMatrix.M[1][2] = texBiasY;
				leftArrows.Textures[i].TextureRect.width = 1;
				leftArrows.Textures[i].TextureRect.height = 1;
			}
			cylinderLayers.push_back(leftArrows);
			auto rightArrows = vrapi_DefaultLayerCylinder2();
			rightArrows.Header.ColorScale.x = 1;
			rightArrows.Header.ColorScale.y = 1;
			rightArrows.Header.ColorScale.z = 1;
			rightArrows.Header.ColorScale.w = 1;
			rightArrows.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA;
			rightArrows.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA;
			rightArrows.HeadPose = tracking.HeadPose;
			rotateYaw = -2 * M_PI / 3;
			scaleMatrix = ovrMatrix4f_CreateScale(radius, radius * (float) appState.ScreenHeight * VRAPI_PI / density, radius);
			rotXMatrix = ovrMatrix4f_CreateRotation(rotatePitch, 0.0f, 0.0f);
			rotYMatrix = ovrMatrix4f_CreateRotation(0.0f, rotateYaw, 0.0f);
			m0 = ovrMatrix4f_Multiply(&rotXMatrix, &scaleMatrix);
			cylinderTransform = ovrMatrix4f_Multiply(&rotYMatrix, &m0);
			for (auto i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; i++)
			{
				ovrMatrix4f modelViewMatrix = ovrMatrix4f_Multiply(&tracking.Eye[i].ViewMatrix, &cylinderTransform);
				rightArrows.Textures[i].TexCoordsFromTanAngles = ovrMatrix4f_Inverse(&modelViewMatrix);
				rightArrows.Textures[i].ColorSwapChain = appState.RightArrows.SwapChain;
				rightArrows.Textures[i].SwapChainIndex = 0;
				const float texScaleX = circScale;
				const float texBiasX = circBias;
				const float texScaleY = 0.5;
				const float texBiasY = -texScaleY * (0.5 * (1 - 1 / texScaleY));
				rightArrows.Textures[i].TextureMatrix.M[0][0] = texScaleX;
				rightArrows.Textures[i].TextureMatrix.M[0][2] = texBiasX;
				rightArrows.Textures[i].TextureMatrix.M[1][1] = texScaleY;
				rightArrows.Textures[i].TextureMatrix.M[1][2] = texBiasY;
				rightArrows.Textures[i].TextureRect.width = 1;
				rightArrows.Textures[i].TextureRect.height = 1;
			}
			cylinderLayers.push_back(rightArrows);
		}
		std::vector<ovrLayerHeader2*> layers;
		layers.push_back(&worldLayer.Header);
		for (auto& layer : cylinderLayers)
		{
			layers.push_back((ovrLayerHeader2*)&layer.Header);
		}
		ovrSubmitFrameDescription2 frameDesc = { };
		frameDesc.SwapInterval = appState.SwapInterval;
		frameDesc.FrameIndex = appState.FrameIndex;
		frameDesc.DisplayTime = appState.DisplayTime;
		frameDesc.LayerCount = layers.size();
		frameDesc.Layers = layers.data();
		vrapi_SubmitFrame2(appState.Ovr, &frameDesc);
	}
	for (auto& view : appState.Views)
	{
		for (auto i = 0; i < view.framebuffer.swapChainLength; i++)
		{
			if (view.framebuffer.framebuffers.size() > 0)
			{
				VC(appState.Device.vkDestroyFramebuffer(appState.Device.device, view.framebuffer.framebuffers[i], nullptr));
			}
			if (view.framebuffer.colorTextures.size() > 0)
			{
				deleteTexture(appState, &view.framebuffer.colorTextures[i]);
			}
			if (view.framebuffer.fragmentDensityTextures.size() > 0)
			{
				deleteTexture(appState, &view.framebuffer.fragmentDensityTextures[i]);
			}
		}
		if (view.framebuffer.depthImage != VK_NULL_HANDLE)
		{
			VC(appState.Device.vkDestroyImageView(appState.Device.device, view.framebuffer.depthImageView, nullptr));
			VC(appState.Device.vkDestroyImage(appState.Device.device, view.framebuffer.depthImage, nullptr));
			VC(appState.Device.vkFreeMemory(appState.Device.device, view.framebuffer.depthMemory, nullptr));
		}
		if (view.framebuffer.renderTexture.image != VK_NULL_HANDLE)
		{
			deleteTexture(appState, &view.framebuffer.renderTexture);
		}
		vrapi_DestroyTextureSwapChain(view.framebuffer.colorTextureSwapChain);
		for (auto& perImage : view.perImage)
		{
			VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &perImage.commandBuffer));
			VC(appState.Device.vkDestroyFence(appState.Device.device, perImage.fence, nullptr));
			for (PipelineResources *pipelineResources = perImage.pipelineResources, *next = nullptr; pipelineResources != nullptr; pipelineResources = next)
			{
				next = pipelineResources->next;
				deletePipelineDescriptorResources(appState, pipelineResources->floor);
				deletePipelineDescriptorResources(appState, pipelineResources->sky);
				deletePipelineDescriptorResources(appState, pipelineResources->colored);
				deletePipelineDescriptorResources(appState, pipelineResources->viewmodels);
				deletePipelineDescriptorResources(appState, pipelineResources->alias);
				deletePipelineDescriptorResources(appState, pipelineResources->turbulent);
				deletePipelineDescriptorResources(appState, pipelineResources->sprites);
				deletePipelineDescriptorResources(appState, pipelineResources->textured);
				deletePipelineDescriptorResources(appState, pipelineResources->palette);
				delete pipelineResources;
			}
			if (perImage.floor != nullptr)
			{
				deleteTexture(appState, perImage.floor);
			}
			if (perImage.sky != nullptr)
			{
				deleteTexture(appState, perImage.sky);
			}
			if (perImage.host_colormap != nullptr)
			{
				deleteTexture(appState, perImage.host_colormap);
			}
			if (perImage.palette != nullptr)
			{
				deleteTexture(appState, perImage.palette);
			}
			deleteCachedTextures(appState, perImage.colormaps);
			deleteCachedTextures(appState, perImage.turbulent);
			deleteCachedTextures(appState, perImage.sprites);
			deleteCachedTextures(appState, perImage.surfaces);
			deleteCachedBuffers(appState, perImage.stagingBuffers);
			deleteCachedBuffers(appState, perImage.uniforms);
			deleteCachedBuffers(appState, perImage.indices32);
			deleteCachedBuffers(appState, perImage.indices16);
			deleteCachedBuffers(appState, perImage.attributes);
			deleteCachedBuffers(appState, perImage.vertices);
			deleteCachedBuffers(appState, perImage.sceneMatricesStagingBuffers);
		}
	}
	vrapi_DestroyTextureSwapChain(appState.RightArrows.SwapChain);
	vrapi_DestroyTextureSwapChain(appState.LeftArrows.SwapChain);
	VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &appState.Screen.CommandBuffer));
	vrapi_DestroyTextureSwapChain(appState.Screen.SwapChain);
	for (auto i = 0; i < appState.Console.View.framebuffer.swapChainLength; i++)
	{
		if (appState.Console.View.framebuffer.framebuffers.size() > 0)
		{
			VC(appState.Device.vkDestroyFramebuffer(appState.Device.device, appState.Console.View.framebuffer.framebuffers[i], nullptr));
		}
		if (appState.Console.View.framebuffer.colorTextures.size() > 0)
		{
			deleteTexture(appState, &appState.Console.View.framebuffer.colorTextures[i]);
		}
	}
	if (appState.Console.View.framebuffer.renderTexture.image != VK_NULL_HANDLE)
	{
		deleteTexture(appState, &appState.Console.View.framebuffer.renderTexture);
	}
	vrapi_DestroyTextureSwapChain(appState.Console.View.framebuffer.colorTextureSwapChain);
	for (auto& perImage : appState.Console.View.perImage)
	{
		VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &perImage.commandBuffer));
		VC(appState.Device.vkDestroyFence(appState.Device.device, perImage.fence, nullptr));
		for (ConsolePipelineResources *pipelineResources = perImage.pipelineResources, *next = nullptr; pipelineResources != nullptr; pipelineResources = next)
		{
			next = pipelineResources->next;
			deletePipelineDescriptorResources(appState, pipelineResources->descriptorResources);
			delete pipelineResources;
		}
		if (perImage.texture != nullptr)
		{
			deleteTexture(appState, perImage.texture);
		}
		if (perImage.palette != nullptr)
		{
			deleteTexture(appState, perImage.palette);
		}
		deleteCachedBuffers(appState, perImage.stagingBuffers);
		deleteCachedBuffers(appState, perImage.indices);
		deleteCachedBuffers(appState, perImage.vertices);
	}
	VC(appState.Device.vkDestroyRenderPass(appState.Device.device, appState.Console.RenderPass, nullptr));
	VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &appState.Console.CommandBuffer));
	vrapi_DestroyTextureSwapChain(appState.Console.SwapChain);
	VC(appState.Device.vkDestroyRenderPass(appState.Device.device, appState.RenderPass, nullptr));
	VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
	deleteCachedTextures(appState, appState.Scene.viewmodelTextures);
	deleteCachedTextures(appState, appState.Scene.aliasTextures);
	deleteCachedBuffers(appState, appState.Scene.colormappedTexCoords);
	deleteCachedBuffers(appState, appState.Scene.colormappedVertices);
	deletePipeline(appState, appState.Scene.console);
	deletePipeline(appState, appState.Scene.floor);
	deletePipeline(appState, appState.Scene.sky);
	deletePipeline(appState, appState.Scene.colored);
	deletePipeline(appState, appState.Scene.viewmodel);
	deletePipeline(appState, appState.Scene.alias);
	deletePipeline(appState, appState.Scene.turbulent);
	deletePipeline(appState, appState.Scene.sprites);
	deletePipeline(appState, appState.Scene.textured);
	VC(appState.Device.vkDestroyDescriptorSetLayout(appState.Device.device, appState.Scene.palette, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.consoleFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.consoleVertex, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.floorFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.floorVertex, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.skyFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.skyVertex, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.coloredFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.coloredVertex, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.viewmodelFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.viewmodelVertex, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.aliasFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.aliasVertex, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.turbulentFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.surfaceVertex, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.texturedFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.texturedVertex, nullptr));
	if (appState.Scene.matrices.mapped != nullptr)
	{
		VC(appState.Device.vkUnmapMemory(appState.Device.device, appState.Scene.matrices.memory));
	}
	VC(appState.Device.vkDestroyBuffer(appState.Device.device, appState.Scene.matrices.buffer, nullptr));
	VC(appState.Device.vkFreeMemory(appState.Device.device, appState.Scene.matrices.memory, nullptr));
	vrapi_DestroySystemVulkan();
	if (appState.Context.device != nullptr)
	{
		if (pthread_mutex_trylock(&appState.Device.queueFamilyMutex) == EBUSY)
		{
			pthread_mutex_lock(&appState.Device.queueFamilyMutex);
		}
		if ((appState.Device.queueFamilyUsedQueues[appState.Context.queueFamilyIndex] & (1 << appState.Context.queueIndex)) != 0)
		{
			appState.Device.queueFamilyUsedQueues[appState.Context.queueFamilyIndex] &= ~(1 << appState.Context.queueIndex);
			pthread_mutex_unlock(&appState.Device.queueFamilyMutex);
			VC(appState.Device.vkDestroyCommandPool(appState.Device.device, appState.Context.commandPool, nullptr));
			VC(appState.Device.vkDestroyPipelineCache(appState.Device.device, appState.Context.pipelineCache, nullptr));
		}
	}
	VK(appState.Device.vkDeviceWaitIdle(appState.Device.device));
	pthread_mutex_destroy(&appState.Device.queueFamilyMutex);
	VC(appState.Device.vkDestroyDevice(appState.Device.device, nullptr));
	if (instance.validate && instance.vkDestroyDebugReportCallbackEXT != nullptr)
	{
		VC(instance.vkDestroyDebugReportCallbackEXT(instance.instance, instance.debugReportCallback, nullptr));
	}
	VC(instance.vkDestroyInstance(instance.instance, nullptr));
	if (instance.loader != nullptr)
	{
		dlclose(instance.loader);
	}
	vrapi_Shutdown();
	java.Vm->DetachCurrentThread();
}
