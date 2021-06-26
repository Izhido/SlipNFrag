#define VK_USE_PLATFORM_ANDROID_KHR
#include "VrApi_Helpers.h"
#include "VulkanCallWrappers.h"
#include <android/log.h>
#include "sys_ovr.h"
#include "AppState.h"
#include "Time.h"
#include <android/window.h>
#include <sys/prctl.h>
#include "VrApi_Vulkan.h"
#include <dlfcn.h>
#include <unistd.h>
#include "MemoryAllocateInfo.h"
#include "VrApi_Input.h"
#include "Input.h"
#include "in_ovr.h"
#include "EngineThread.h"
#include "Constants.h"
#include "DirectRect.h"
#include "r_local.h"
#include "CylinderProjection.h"

static const int queueCount = 1;
static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;

extern m_state_t m_state;

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

void appHandleCommands(struct android_app* app, int32_t cmd)
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
			vrapi_Shutdown();
			exit(0);
	}
}

void android_main(struct android_app* app)
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
	instance.validate = true;
#else
	instance.validate = false;
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
		"VK_LAYER_KHRONOS_validation"
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
	instanceCreateInfo.pNext = (instance.validate ? &debugReportCallbackCreateInfo : nullptr);
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledLayerCount = enabledLayerNames.size();
	instanceCreateInfo.ppEnabledLayerNames = (const char *const *) (enabledLayerNames.size() != 0 ? enabledLayerNames.data() : nullptr);
	instanceCreateInfo.enabledExtensionCount = enabledExtensionNames.size();
	instanceCreateInfo.ppEnabledExtensionNames = (const char *const *) (enabledExtensionNames.size() != 0 ? enabledExtensionNames.data() : nullptr);
	VK(instance.vkCreateInstance(&instanceCreateInfo, nullptr, &instance.instance));
	if (!instance.Bind())
	{
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
		auto lazyAllocateFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		for (auto i = 0; i < appState.Device.physicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			const VkFlags propertyFlags = appState.Device.physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags;
			if ((propertyFlags & lazyAllocateFlags) == lazyAllocateFlags)
			{
				appState.Device.supportsLazyAllocate = true;
				__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "Device supports lazy allocated memory pools");
				break;
			}
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
	if (!appState.Device.Bind(appState, instance))
	{
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
	appState.EyeTextureWidth = vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
	appState.EyeTextureHeight = vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);
	if (appState.EyeTextureWidth > appState.Device.physicalDeviceProperties.properties.limits.maxFramebufferWidth)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): Eye texture width exceeds the physical device's limits.");
		vrapi_Shutdown();
		exit(0);
	}
	if (appState.EyeTextureHeight > appState.Device.physicalDeviceProperties.properties.limits.maxFramebufferHeight)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): Eye texture height exceeds the physical device's limits.");
		vrapi_Shutdown();
		exit(0);
	}
	appState.EyeTextureMaxDimension = std::max(appState.EyeTextureWidth, appState.EyeTextureHeight);
	auto horizontalFOV = vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X);
	auto verticalFOV = vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y);
	appState.FOV = std::max(horizontalFOV, verticalFOV);
	for (auto& view : appState.Views)
	{
		view.colorSwapChain.SwapChain = vrapi_CreateTextureSwapChain3(isMultiview ? VRAPI_TEXTURE_TYPE_2D_ARRAY : VRAPI_TEXTURE_TYPE_2D, VK_FORMAT_R8G8B8A8_UNORM, appState.EyeTextureWidth, appState.EyeTextureHeight, 1, 3);
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
		view.framebuffer.width = appState.EyeTextureWidth;
		view.framebuffer.height = appState.EyeTextureHeight;
		for (auto i = 0; i < view.framebuffer.swapChainLength; i++)
		{
			auto& texture = view.framebuffer.colorTextures[i];
			texture.width = appState.EyeTextureWidth;
			texture.height = appState.EyeTextureHeight;
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
		imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK(appState.Device.vkCreateImage(appState.Device.device, &imageCreateInfo, nullptr, &texture.image));
		VkMemoryRequirements memoryRequirements;
		VC(appState.Device.vkGetImageMemoryRequirements(appState.Device.device, texture.image, &memoryRequirements));
		VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		if (appState.Device.supportsLazyAllocate)
		{
			memFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		}
		VkMemoryAllocateInfo memoryAllocateInfo { };
		createMemoryAllocateInfo(appState, memoryRequirements, memFlags, memoryAllocateInfo);
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
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		VK(appState.Device.vkCreateImage(appState.Device.device, &imageCreateInfo, nullptr, &view.framebuffer.depthImage));
		VC(appState.Device.vkGetImageMemoryRequirements(appState.Device.device, view.framebuffer.depthImage, &memoryRequirements));
		createMemoryAllocateInfo(appState, memoryRequirements, memFlags, memoryAllocateInfo);
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
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		imageMemoryBarrier.image = view.framebuffer.depthImage;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageMemoryBarrier.subresourceRange.layerCount = numLayers;
		VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
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
	appState.NoOffset = 0;
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
					parms.ModeParms.Flags |= VRAPI_MODE_FLAG_PHASE_SYNC;
					parms.ModeParms.WindowSurface = (size_t)appState.NativeWindow;
					parms.ModeParms.Display = 0;
					parms.ModeParms.ShareContext = 0;
					appState.Ovr = vrapi_EnterVrMode((ovrModeParms*)&parms);
					if (appState.Ovr == nullptr)
					{
						__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "android_main(): vrapi_EnterVRMode() failed: invalid ANativeWindow");
						appState.NativeWindow = nullptr;
					}
					if (appState.Ovr != nullptr)
					{
						vrapi_SetClockLevels(appState.Ovr, appState.CpuLevel, appState.GpuLevel);
						vrapi_SetPerfThread(appState.Ovr, VRAPI_PERF_THREAD_TYPE_MAIN, appState.MainThreadTid);
						vrapi_SetPerfThread(appState.Ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, appState.RenderThreadTid);
						auto refreshRateCount = vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_NUM_SUPPORTED_DISPLAY_REFRESH_RATES);
						std::vector<float> refreshRates(refreshRateCount);
						vrapi_GetSystemPropertyFloatArray(&appState.Java, VRAPI_SYS_PROP_SUPPORTED_DISPLAY_REFRESH_RATES, refreshRates.data(), refreshRateCount);
						float highestRefreshRate = 0;
						for (auto& entry : refreshRates)
						{
							highestRefreshRate = std::max(highestRefreshRate, entry);
						}
						vrapi_SetDisplayRefreshRate(appState.Ovr, highestRefreshRate);
					}
				}
			}
			else if (appState.Ovr != nullptr)
			{
				vrapi_LeaveVrMode(appState.Ovr);
				appState.Ovr = nullptr;
			}
		}
		ovrEventDataBuffer eventDataBuffer { };
		// Poll for VrApi events
		for (;;)
		{
			ovrEventHeader* eventHeader = (ovrEventHeader*)(&eventDataBuffer);
			ovrResult res = vrapi_PollEvent(eventHeader);
			if (res != ovrSuccess)
			{
				break;
			}
			std::string eventName;
			switch (eventHeader->EventType) {
				case VRAPI_EVENT_DATA_LOST:
					eventName = "VRAPI_EVENT_DATA_LOST";
					break;
				case VRAPI_EVENT_VISIBILITY_GAINED:
					eventName = "VRAPI_EVENT_VISIBILITY_GAINED";
					break;
				case VRAPI_EVENT_VISIBILITY_LOST:
					eventName = "VRAPI_EVENT_VISIBILITY_LOST";
					break;
				case VRAPI_EVENT_FOCUS_GAINED:
					eventName = "VRAPI_EVENT_FOCUS_GAINED";
					break;
				case VRAPI_EVENT_FOCUS_LOST:
					eventName = "VRAPI_EVENT_FOCUS_LOST";
					break;
				case VRAPI_EVENT_DISPLAY_REFRESH_RATE_CHANGE:
					eventName = "VRAPI_EVENT_DISPLAY_REFRESH_RATE_CHANGE";
					break;
			}
			if (eventName.length() == 0)
			{
				__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "android_main(): Unknown vrapi_PollEvent() received");
			}
			else
			{
				__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "android_main(): vrapi_PollEvent() returned %s", eventName.c_str());
			}
		}
		appState.FrameIndex++;
		vrapi_WaitFrame(appState.Ovr, appState.FrameIndex);
		const double predictedDisplayTime = vrapi_GetPredictedDisplayTime(appState.Ovr, appState.FrameIndex);
		appState.Tracking = vrapi_GetPredictedTracking2(appState.Ovr, predictedDisplayTime);
		appState.DisplayTime = predictedDisplayTime;
		auto leftRemoteFound = false;
		auto rightRemoteFound = false;
		appState.LeftController.Buttons = 0;
		appState.RightController.Buttons = 0;
		appState.LeftController.Joystick.x = 0;
		appState.LeftController.Joystick.y = 0;
		appState.RightController.Joystick.x = 0;
		appState.RightController.Joystick.y = 0;
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
							appState.LeftController.Buttons = trackedRemoteState.Buttons;
							appState.LeftController.Joystick = trackedRemoteState.Joystick;
							appState.LeftController.TrackingResult = vrapi_GetInputTrackingState(appState.Ovr, cap.DeviceID, predictedDisplayTime, &appState.LeftController.Tracking);
						}
						else if ((trCap->ControllerCapabilities & ovrControllerCaps_RightHand) != 0)
						{
							rightRemoteFound = true;
							appState.RightController.Buttons = trackedRemoteState.Buttons;
							appState.RightController.Joystick = trackedRemoteState.Joystick;
							appState.RightController.TrackingResult = vrapi_GetInputTrackingState(appState.Ovr, cap.DeviceID, predictedDisplayTime, &appState.RightController.Tracking);
						}
					}
				}
			}
		}
		if (leftRemoteFound && rightRemoteFound)
		{
			auto triggerHandled = appState.Keyboard.Handle(appState);
			Input::Handle(appState, triggerHandled);
		}
		else
		{
			joy_avail = false;
		}
		if (leftRemoteFound)
		{
			appState.LeftController.PreviousButtons = appState.LeftController.Buttons;
			appState.LeftController.PreviousJoystick = appState.LeftController.Joystick;
		}
		else
		{
			appState.LeftController.PreviousButtons = 0;
			appState.LeftController.PreviousJoystick.x = 0;
			appState.LeftController.PreviousJoystick.y = 0;
		}
		if (rightRemoteFound)
		{
			appState.RightController.PreviousButtons = appState.RightController.Buttons;
			appState.RightController.PreviousJoystick = appState.RightController.Joystick;
		}
		else
		{
			appState.RightController.PreviousButtons = 0;
			appState.RightController.PreviousJoystick.x = 0;
			appState.RightController.PreviousJoystick.y = 0;
		}
		if (appState.Ovr == nullptr)
		{
			continue;
		}
		if (!appState.Scene.createdScene)
		{
			appState.Scene.Create(appState, commandBufferAllocateInfo, setupCommandBuffer, commandBufferBeginInfo, setupSubmitInfo, instance, fenceCreateInfo, app);
			appState.Scene.createdScene = true;
		}
		{
			std::lock_guard<std::mutex> lock(appState.ModeChangeMutex);
			if (appState.Mode != AppStartupMode && appState.Mode != AppNoGameDataMode)
			{
				if (cls.demoplayback || cl.intermission || con_forcedup || scr_disabled_for_loading)
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
					sys_version = "OVR 1.0.7";
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
							vrapi_Shutdown();
							exit(0);
						}
					}
					appState.LeftController.PreviousButtons = 0;
					appState.RightController.PreviousButtons = 0;
					appState.DefaultFOV = (int)Cvar_VariableValue("fov");
					r_skybox_as_rgba = true;
					appState.EngineThread = std::thread(runEngine, &appState, app);
				}
				if (appState.Mode == AppScreenMode)
				{
					std::lock_guard<std::mutex> lock(appState.RenderMutex);
					D_ResetLists();
					d_uselists = false;
					r_skip_fov_check = false;
					sb_onconsole = false;
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
					r_modelorg_delta[0] = 0;
					r_modelorg_delta[1] = 0;
					r_modelorg_delta[2] = 0;
				}
				else if (appState.Mode == AppWorldMode)
				{
					std::lock_guard<std::mutex> lock(appState.RenderMutex);
					D_ResetLists();
					d_uselists = true;
					r_skip_fov_check = true;
					sb_onconsole = true;
					Cvar_SetValue("joyadvanced", 1);
					Cvar_SetValue("joyadvaxisx", AxisSide);
					Cvar_SetValue("joyadvaxisy", AxisForward);
					Cvar_SetValue("joyadvaxisz", AxisNada);
					Cvar_SetValue("joyadvaxisr", AxisNada);
					Joy_AdvancedUpdate_f();
					vid_width = appState.EyeTextureMaxDimension;
					vid_height = appState.EyeTextureMaxDimension;
					con_width = appState.ConsoleWidth;
					con_height = appState.ConsoleHeight;
					Cvar_SetValue("fov", appState.FOV);
					VID_Resize(1);
				}
				else if (appState.Mode == AppNoGameDataMode)
				{
					std::lock_guard<std::mutex> lock(appState.RenderMutex);
					D_ResetLists();
					d_uselists = false;
					r_skip_fov_check = false;
					sb_onconsole = false;
				}
				appState.PreviousMode = appState.Mode;
			}
		}
		for (auto& entry : appState.Scene.lightmaps)
		{
			auto total = 0;
			auto erased = 0;
			for (auto l = &entry.second; *l != nullptr; )
			{
				(*l)->unusedCount++;
				if ((*l)->unusedCount >= MAX_UNUSED_COUNT)
				{
					auto next = (*l)->next;
					(*l)->next = appState.Scene.oldSurfaces;
					appState.Scene.oldSurfaces = *l;
					*l = next;
					erased++;
				}
				else
				{
					l = &(*l)->next;
				}
				total++;
			}
			if (total == erased)
			{
				appState.Scene.lightmapsToDelete.push_back(entry.first);
			}
		}
		for (auto& entry : appState.Scene.lightmapsToDelete)
		{
			appState.Scene.lightmaps.erase(entry);
		}
		appState.Scene.lightmapsToDelete.clear();
		SharedMemoryTexture::DeleteOld(appState, &appState.Scene.viewmodelTextures.oldTextures);
		SharedMemoryTexture::DeleteOld(appState, &appState.Scene.aliasTextures.oldTextures);
		SharedMemoryTexture::DeleteOld(appState, &appState.Scene.spriteTextures.oldTextures);
		SharedMemoryTexture::DeleteOld(appState, &appState.Scene.fenceTextures.oldTextures);
		SharedMemoryTexture::DeleteOld(appState, &appState.Scene.surfaceTextures.oldTextures);
		Lightmap::DeleteOld(appState, &appState.Scene.oldSurfaces);
		for (Buffer** b = &appState.Scene.oldSurfaceVerticesPerModel; *b != nullptr; )
		{
			(*b)->unusedCount++;
			if ((*b)->unusedCount >= MAX_UNUSED_COUNT)
			{
				Buffer* next = (*b)->next;
				(*b)->Destroy(appState);
				delete *b;
				*b = next;
			}
			else
			{
				b = &(*b)->next;
			}
		}
		appState.Scene.colormappedBuffers.DeleteOld(appState);
		{
			std::lock_guard<std::mutex> lock(appState.RenderInputMutex);
			for (auto i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; i++)
			{
				appState.ViewMatrices[i] = ovrMatrix4f_Transpose(&appState.Tracking.Eye[i].ViewMatrix);
				appState.ProjectionMatrices[i] = ovrMatrix4f_Transpose(&appState.Tracking.Eye[i].ProjectionMatrix);
			}
			appState.Scene.orientation = appState.Tracking.HeadPose.Pose.Orientation;
			auto x = appState.Scene.orientation.x;
			auto y = appState.Scene.orientation.y;
			auto z = appState.Scene.orientation.z;
			auto w = appState.Scene.orientation.w;
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
			appState.Scene.pose = vrapi_LocateTrackingSpace(appState.Ovr, VRAPI_TRACKING_SPACE_LOCAL_FLOOR);
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
			appState.Scale = -appState.Scene.pose.Position.y / playerHeight;
		}
		vrapi_BeginFrame(appState.Ovr, appState.FrameIndex);
		appState.RenderScene(commandBufferBeginInfo);
		if (appState.Mode == AppScreenMode)
		{
			{
				std::lock_guard<std::mutex> lock(appState.RenderMutex);
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
			}
			{
				std::lock_guard<std::mutex> lock(DirectRect::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto width = directRect.width * 3;
					auto height = directRect.height * 3;
					auto directRectIndex = 0;
					auto directRectIndexCache = 0;
					auto targetIndex = directRect.y * 3 * vid_width + directRect.x * 3;
					auto y = 0;
					while (y < height)
					{
						auto x = 0;
						while (x < width)
						{
							auto entry = d_8to24table[directRect.data[directRectIndex]];
							do
							{
								appState.Screen.Data[targetIndex] = entry;
								targetIndex++;
								x++;
							} while ((x % 3) != 0);
							directRectIndex++;
						}
						y++;
						if ((y % 3) == 0)
						{
							directRectIndexCache = directRectIndex;
						}
						else
						{
							directRectIndex = directRectIndexCache;
						}
						targetIndex += vid_width - width;
					}
				}
			}
			memcpy(appState.Screen.Buffer.mapped, appState.Screen.Data.data(), appState.Screen.Data.size() * sizeof(uint32_t));
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
			perImage.Reset(appState);
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
				vertices = new Buffer();
				vertices->CreateVertexBuffer(appState, appState.ConsoleVertices.size() * sizeof(float));
				VK(appState.Device.vkMapMemory(appState.Device.device, vertices->memory, 0, VK_WHOLE_SIZE, 0, &vertices->mapped));
			}
			perImage.vertices.MoveToFront(vertices);
			memcpy(vertices->mapped, appState.ConsoleVertices.data(), vertices->size);
			VkBufferMemoryBarrier bufferMemoryBarrier { };
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			vertices->SubmitVertexBuffer(appState, perImage.commandBuffer, bufferMemoryBarrier);
			Buffer* indices = nullptr;
			if (perImage.indices.oldBuffers != nullptr)
			{
				indices = perImage.indices.oldBuffers;
				perImage.indices.oldBuffers = perImage.indices.oldBuffers->next;
			}
			else
			{
				indices = new Buffer();
				indices->CreateIndexBuffer(appState, appState.ConsoleIndices.size() * sizeof(uint16_t));
				VK(appState.Device.vkMapMemory(appState.Device.device, indices->memory, 0, VK_WHOLE_SIZE, 0, &indices->mapped));
			}
			perImage.indices.MoveToFront(indices);
			memcpy(indices->mapped, appState.ConsoleIndices.data(), indices->size);
			indices->SubmitIndexBuffer(appState, perImage.commandBuffer, bufferMemoryBarrier);
			auto keyboardDrawn = appState.Keyboard.Draw(appState);
			auto stagingBufferSize = 0;
			perImage.paletteOffset = -1;
			if (perImage.palette == nullptr || perImage.paletteChanged != pal_changed)
			{
				if (perImage.palette == nullptr)
				{
					perImage.palette = new Texture();
					perImage.palette->Create(appState, 256, 1, VK_FORMAT_R8G8B8A8_UNORM, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
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
				perImage.texture = new Texture();
				perImage.texture->Create(appState, appState.ConsoleWidth, appState.ConsoleHeight, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			}
			perImage.textureOffset = stagingBufferSize;
			stagingBufferSize += con_buffer.size();
			if (appState.Keyboard.texture == nullptr && keyboardDrawn)
			{
				appState.Keyboard.texture = new Texture();
				appState.Keyboard.texture->Create(appState, appState.ConsoleWidth, appState.ConsoleHeight / 2, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			}
			appState.Keyboard.textureOffset = -1;
			if (keyboardDrawn)
			{
				appState.Keyboard.textureOffset = stagingBufferSize;
				stagingBufferSize += appState.Keyboard.buffer.size();
			}
			auto stagingBuffer = perImage.stagingBuffers.GetStagingBuffer(appState, stagingBufferSize);
			auto offset = 0;
			if (perImage.paletteOffset >= 0)
			{
				// Copy all but the last entry of the palette:
				memcpy(stagingBuffer->mapped, d_8to24table, 1020);
				// Force the last entry to be transparent black - thus adding back the transparent pixel removed in VID_SetPalette():
				*((unsigned char*)stagingBuffer->mapped + 1020) = 0;
				*((unsigned char*)stagingBuffer->mapped + 1021) = 0;
				*((unsigned char*)stagingBuffer->mapped + 1022) = 0;
				*((unsigned char*)stagingBuffer->mapped + 1023) = 0;
				offset += 1024;
			}
			if (perImage.textureOffset >= 0)
			{
				{
					std::lock_guard<std::mutex> lock(appState.RenderMutex);
					memcpy(((unsigned char*)stagingBuffer->mapped) + offset, con_buffer.data(), con_buffer.size());
					offset += con_buffer.size();
				}
				{
					std::lock_guard<std::mutex> lock(DirectRect::DirectRectMutex);
					for (auto& directRect : DirectRect::directRects)
					{
						auto directRectIndex = 0;
						auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
						auto targetIndex = directRect.y * con_width + directRect.x;
						auto y = 0;
						while (y < directRect.height)
						{
							auto x = 0;
							while (x < directRect.width)
							{
								auto entry = directRect.data[directRectIndex];
								target[targetIndex] = entry;
								targetIndex++;
								x++;
								directRectIndex++;
							}
							y++;
							targetIndex += con_width - directRect.width;
						}
					}
				}
			}
			if (appState.Keyboard.textureOffset >= 0)
			{
				memcpy(((unsigned char*)stagingBuffer->mapped) + offset, appState.Keyboard.buffer.data(), appState.Keyboard.buffer.size());
			}
			VkMappedMemoryRange mappedMemoryRange { };
			mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			mappedMemoryRange.memory = stagingBuffer->memory;
			VC(appState.Device.vkFlushMappedMemoryRanges(appState.Device.device, 1, &mappedMemoryRange));
			if (perImage.paletteOffset >= 0)
			{
				perImage.palette->Fill(appState, stagingBuffer, perImage.paletteOffset, perImage.commandBuffer);
			}
			if (perImage.textureOffset >= 0)
			{
				perImage.texture->Fill(appState, stagingBuffer, perImage.textureOffset, perImage.commandBuffer);
			}
			if (appState.Keyboard.textureOffset >= 0)
			{
				appState.Keyboard.texture->Fill(appState, stagingBuffer, appState.Keyboard.textureOffset, perImage.commandBuffer);
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
			VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset));
			VC(appState.Device.vkCmdBindVertexBuffers(perImage.commandBuffer, 1, 1, &vertices->buffer, &appState.NoOffset));
			VC(appState.Device.vkCmdBindPipeline(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.console.pipeline));
			if (!perImage.descriptorResources.created)
			{
				VkDescriptorPoolSize poolSize;
				poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSize.descriptorCount = 2;
				VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { };
				descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				descriptorPoolCreateInfo.maxSets = 1;
				descriptorPoolCreateInfo.poolSizeCount = 1;
				descriptorPoolCreateInfo.pPoolSizes = &poolSize;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &perImage.descriptorResources.descriptorPool));
				VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { };
				descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorSetAllocateInfo.descriptorPool = perImage.descriptorResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = 1;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.doubleImageLayout;
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &perImage.descriptorResources.descriptorSet));
				VkDescriptorImageInfo textureInfo[2] { };
				textureInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				textureInfo[0].sampler = appState.Scene.textureSamplers[perImage.texture->mipCount];
				textureInfo[0].imageView = perImage.texture->view;
				textureInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				textureInfo[1].sampler = appState.Scene.textureSamplers[perImage.palette->mipCount];
				textureInfo[1].imageView = perImage.palette->view;
				VkWriteDescriptorSet writes[2] { };
				writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[0].dstSet = perImage.descriptorResources.descriptorSet;
				writes[0].descriptorCount = 1;
				writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[0].pImageInfo = textureInfo;
				writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[1].dstBinding = 1;
				writes[1].dstSet = perImage.descriptorResources.descriptorSet;
				writes[1].descriptorCount = 1;
				writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[1].pImageInfo = textureInfo + 1;
				VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
				perImage.descriptorResources.created = true;
			}
			if (!appState.Keyboard.descriptorResources.created && keyboardDrawn)
			{
				VkDescriptorPoolSize poolSize;
				poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSize.descriptorCount = 2;
				VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { };
				descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				descriptorPoolCreateInfo.maxSets = 1;
				descriptorPoolCreateInfo.poolSizeCount = 1;
				descriptorPoolCreateInfo.pPoolSizes = &poolSize;
				VK(appState.Device.vkCreateDescriptorPool(appState.Device.device, &descriptorPoolCreateInfo, nullptr, &appState.Keyboard.descriptorResources.descriptorPool));
				VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { };
				descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorSetAllocateInfo.descriptorPool = appState.Keyboard.descriptorResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = 1;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.doubleImageLayout;
				VK(appState.Device.vkAllocateDescriptorSets(appState.Device.device, &descriptorSetAllocateInfo, &appState.Keyboard.descriptorResources.descriptorSet));
				VkDescriptorImageInfo textureInfo[2] { };
				textureInfo[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				textureInfo[0].sampler = appState.Scene.textureSamplers[appState.Keyboard.texture->mipCount];
				textureInfo[0].imageView = appState.Keyboard.texture->view;
				textureInfo[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				textureInfo[1].sampler = appState.Scene.textureSamplers[perImage.palette->mipCount];
				textureInfo[1].imageView = perImage.palette->view;
				VkWriteDescriptorSet writes[2] { };
				writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[0].dstSet = appState.Keyboard.descriptorResources.descriptorSet;
				writes[0].descriptorCount = 1;
				writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[0].pImageInfo = textureInfo;
				writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[1].dstBinding = 1;
				writes[1].dstSet = appState.Keyboard.descriptorResources.descriptorSet;
				writes[1].descriptorCount = 1;
				writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[1].pImageInfo = textureInfo + 1;
				VC(appState.Device.vkUpdateDescriptorSets(appState.Device.device, 2, writes, 0, nullptr));
				appState.Keyboard.descriptorResources.created = true;
			}
			VC(appState.Device.vkCmdBindIndexBuffer(perImage.commandBuffer, indices->buffer, appState.NoOffset, VK_INDEX_TYPE_UINT16));
			VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.console.pipelineLayout, 0, 1, &perImage.descriptorResources.descriptorSet, 0, nullptr));
			VC(appState.Device.vkCmdDrawIndexed(perImage.commandBuffer, appState.ConsoleIndices.size() - 6, 1, 0, 0, 0));
			if (keyboardDrawn)
			{
				VC(appState.Device.vkCmdBindDescriptorSets(perImage.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.console.pipelineLayout, 0, 1, &appState.Keyboard.descriptorResources.descriptorSet, 0, nullptr));
				VC(appState.Device.vkCmdDrawIndexed(perImage.commandBuffer, 6, 1, appState.ConsoleIndices.size() - 6, 0, 0));
			}
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
			memcpy(appState.Screen.Buffer.mapped, appState.NoGameDataData.data(), appState.NoGameDataData.size() * sizeof(uint32_t));
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
		std::vector<ovrLayerCube2> skyboxLayers;
		ovrLayerProjection2 worldLayer = vrapi_DefaultLayerProjection2();
		worldLayer.HeadPose = appState.Tracking.HeadPose;
		for (auto i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; i++)
		{
			auto view = (appState.Views.size() == 1 ? 0 : i);
			worldLayer.Textures[i].ColorSwapChain = appState.Views[view].framebuffer.colorTextureSwapChain;
			worldLayer.Textures[i].SwapChainIndex = appState.Views[view].index;
			worldLayer.Textures[i].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&appState.Tracking.Eye[i].ProjectionMatrix);
		}
		if (appState.Mode != AppWorldMode || appState.Scene.skybox != VK_NULL_HANDLE)
		{
			worldLayer.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA;
			worldLayer.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA;
		}
		worldLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
		std::vector<ovrLayerCylinder2> cylinderLayers;
		if (appState.Mode == AppWorldMode)
		{
			if (d_lists.last_skybox >= 0 && appState.Scene.skybox == VK_NULL_HANDLE)
			{
				int width = -1;
				int height = -1;
				auto& skybox = d_lists.skyboxes[0];
				for (auto i = 0; i < 6; i++)
				{
					auto texture = skybox.textures[i].texture;
					if (texture == nullptr)
					{
						width = -1;
						height = -1;
						break;
					}
					if (width < 0 && height < 0)
					{
						width = texture->width;
						height = texture->height;
					}
					else if (width != texture->width || height != texture->height)
					{
						width = -1;
						height = -1;
						break;
					}
				}
				if (width > 0 && height > 0)
				{
					appState.Scene.skybox = vrapi_CreateTextureSwapChain3(VRAPI_TEXTURE_TYPE_CUBE, VK_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1);
					VkBufferCreateInfo bufferCreateInfo { };
					bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
					bufferCreateInfo.size = width * height * 4;
					bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
					bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
					std::vector<VkBuffer> buffers(6);
					VkMemoryRequirements memoryRequirements;
					VkMemoryAllocateInfo memoryAllocateInfo { };
					std::vector<VkDeviceMemory> memoryBlocks(6);
					VkMappedMemoryRange mappedMemoryRange { };
					mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
					VkImageMemoryBarrier imageMemoryBarrier { };
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageMemoryBarrier.subresourceRange.levelCount = 1;
					imageMemoryBarrier.subresourceRange.layerCount = 1;
					VkBufferImageCopy region { };
					region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.imageSubresource.layerCount = 1;
					region.imageExtent.width = width;
					region.imageExtent.height = height;
					region.imageExtent.depth = 1;
					VK(appState.Device.vkAllocateCommandBuffers(appState.Device.device, &commandBufferAllocateInfo, &setupCommandBuffer));
					VK(appState.Device.vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
					auto image = vrapi_GetTextureSwapChainBufferVulkan(appState.Scene.skybox, 0);
					imageMemoryBarrier.image = image;
					for (auto i = 0; i < 6; i++)
					{
						VK(appState.Device.vkCreateBuffer(appState.Device.device, &bufferCreateInfo, nullptr, &buffers[i]));
						VC(appState.Device.vkGetBufferMemoryRequirements(appState.Device.device, buffers[i], &memoryRequirements));
						createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memoryAllocateInfo);
						VK(appState.Device.vkAllocateMemory(appState.Device.device, &memoryAllocateInfo, nullptr, &memoryBlocks[i]));
						VK(appState.Device.vkBindBufferMemory(appState.Device.device, buffers[i], memoryBlocks[i], 0));
						void* mapped;
						VK(appState.Device.vkMapMemory(appState.Device.device, memoryBlocks[i], 0, memoryRequirements.size, 0, &mapped));
						std::string name;
						switch (i)
						{
							case 0:
								name = "ft";
								break;
							case 1:
								name = "bk";
								break;
							case 2:
								name = "up";
								break;
							case 3:
								name = "dn";
								break;
							case 4:
								name = "rt";
								break;
							default:
								name = "lf";
								break;
						}
						auto index = 0;
						while (index < 5)
						{
							if (name == std::string(skybox.textures[index].texture->name))
							{
								break;
							}
							index++;
						}
						memcpy(mapped, (byte*)skybox.textures[index].texture + sizeof(texture_t) + width * height, width * height * 4);
						mappedMemoryRange.memory = memoryBlocks[i];
						VC(appState.Device.vkFlushMappedMemoryRanges(appState.Device.device, 1, &mappedMemoryRange));
						VC(appState.Device.vkUnmapMemory(appState.Device.device, memoryBlocks[i]));
						imageMemoryBarrier.subresourceRange.baseArrayLayer = i;
						VC(appState.Device.vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier));
						region.imageSubresource.baseArrayLayer = i;
						VC(appState.Device.vkCmdCopyBufferToImage(setupCommandBuffer, buffers[i], image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));
					}
					VK(appState.Device.vkEndCommandBuffer(setupCommandBuffer));
					VK(appState.Device.vkQueueSubmit(appState.Context.queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));
					VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
					VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &setupCommandBuffer));
					for (auto i = 0; i < 6; i++)
					{
						VC(appState.Device.vkDestroyBuffer(appState.Device.device, buffers[i], nullptr));
						VC(appState.Device.vkFreeMemory(appState.Device.device, memoryBlocks[i], nullptr));
					}
				}
			}
			if (appState.Scene.skybox != VK_NULL_HANDLE)
			{
				const auto view = vrapi_GetViewMatrixFromPose(&appState.Tracking.HeadPose.Pose);
				const auto rotation = ovrMatrix4f_CreateRotation(0, M_PI / 2, 0);
				const auto scale = ovrMatrix4f_CreateScale(-1, 1, 1);
				const auto matrix1 = ovrMatrix4f_Multiply(&view, &rotation);
				const auto matrix2 = ovrMatrix4f_Multiply(&matrix1, &scale);
				const auto cubeMatrix = ovrMatrix4f_TanAngleMatrixForCubeMap(&matrix2);
				auto skyboxLayer = vrapi_DefaultLayerCube2();
				skyboxLayer.HeadPose = appState.Tracking.HeadPose;
				skyboxLayer.TexCoordsFromTanAngles = cubeMatrix;
				for (auto i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; i++)
				{
					skyboxLayer.Textures[i].ColorSwapChain = appState.Scene.skybox;
					skyboxLayer.Textures[i].SwapChainIndex = 0;
				}
				skyboxLayers.push_back(skyboxLayer);
			}
			auto console = vrapi_DefaultLayerCylinder2();
			CylinderProjection::Setup(appState, console); // First setup - rendering variables
			const ovrMatrix4f scaleMatrix = ovrMatrix4f_CreateScale(CylinderProjection::radius, CylinderProjection::radius * (float) appState.ScreenHeight * VRAPI_PI / CylinderProjection::density, CylinderProjection::radius);
			const ovrVector3f& position = appState.Tracking.HeadPose.Pose.Position;
			const ovrMatrix4f transMatrix = ovrMatrix4f_CreateTranslation(position.x, position.y, position.z);
			const ovrMatrix4f rotXMatrix = ovrMatrix4f_CreateRotation(appState.Pitch, 0, 0);
			const ovrMatrix4f rotYMatrix = ovrMatrix4f_CreateRotation(0, appState.Yaw, 0);
			const ovrMatrix4f m0 = ovrMatrix4f_Multiply(&rotXMatrix, &scaleMatrix);
			const ovrMatrix4f m1 = ovrMatrix4f_Multiply(&rotYMatrix, &m0);
			const ovrMatrix4f transform = ovrMatrix4f_Multiply(&transMatrix, &m1);
			CylinderProjection::Setup(appState, console, &transform, appState.Console.SwapChain, appState.Console.View.index); // Second setup - transforms for each eye
			cylinderLayers.push_back(console);
		}
		else
		{
			auto screen = vrapi_DefaultLayerCylinder2();
			CylinderProjection::Setup(appState, screen, 0, appState.Screen.SwapChain);
			cylinderLayers.push_back(screen);
			auto leftArrows = vrapi_DefaultLayerCylinder2();
			CylinderProjection::Setup(appState, leftArrows, 2 * M_PI / 3, appState.LeftArrows.SwapChain);
			cylinderLayers.push_back(leftArrows);
			auto rightArrows = vrapi_DefaultLayerCylinder2();
			CylinderProjection::Setup(appState, rightArrows, -2 * M_PI / 3, appState.RightArrows.SwapChain);
			cylinderLayers.push_back(rightArrows);
		}
		std::vector<ovrLayerHeader2*> layers;
		for (auto& layer : skyboxLayers)
		{
			layers.push_back((ovrLayerHeader2*)&layer.Header);
		}
		if (appState.Mode == AppWorldMode)
		{
			layers.push_back(&worldLayer.Header);
		}
		for (auto& layer : cylinderLayers)
		{
			layers.push_back((ovrLayerHeader2*)&layer.Header);
		}
		if (appState.Mode != AppWorldMode)
		{
			layers.push_back(&worldLayer.Header);
		}
		ovrSubmitFrameDescription2 frameDesc = { };
		frameDesc.SwapInterval = appState.SwapInterval;
		frameDesc.FrameIndex = appState.FrameIndex;
		frameDesc.DisplayTime = appState.DisplayTime;
		frameDesc.LayerCount = layers.size();
		frameDesc.Layers = layers.data();
		vrapi_SubmitFrame2(appState.Ovr, &frameDesc);
	}
	if (appState.EngineThread.joinable())
	{
		appState.EngineThreadStopped = true;
		appState.EngineThread.join();
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
				view.framebuffer.colorTextures[i].Delete(appState);
			}
			if (view.framebuffer.fragmentDensityTextures.size() > 0)
			{
				view.framebuffer.fragmentDensityTextures[i].Delete(appState);
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
			view.framebuffer.renderTexture.Delete(appState);
		}
		vrapi_DestroyTextureSwapChain(view.framebuffer.colorTextureSwapChain);
		for (auto& perImage : view.perImage)
		{
			VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &perImage.commandBuffer));
			VC(appState.Device.vkDestroyFence(appState.Device.device, perImage.fence, nullptr));
			perImage.controllerResources.Delete(appState);
			perImage.floorResources.Delete(appState);
			perImage.skyResources.Delete(appState);
			perImage.viewmodelResources.Delete(appState);
			perImage.aliasResources.Delete(appState);
			perImage.turbulentResources.Delete(appState);
			perImage.spriteResources.Delete(appState);
			perImage.fenceTextureResources.Delete(appState);
			perImage.surfaceTextureResources.Delete(appState);
			perImage.sceneMatricesAndColormapResources.Delete(appState);
			perImage.sceneMatricesAndPaletteResources.Delete(appState);
			perImage.sceneMatricesResources.Delete(appState);
			perImage.host_colormapResources.Delete(appState);
			if (perImage.sky != nullptr)
			{
				perImage.sky->Delete(appState);
			}
			if (perImage.host_colormap != nullptr)
			{
				perImage.host_colormap->Delete(appState);
			}
			if (perImage.palette != nullptr)
			{
				perImage.palette->Delete(appState);
			}
			perImage.colormaps.Delete(appState);
			perImage.turbulent.Delete(appState);
			perImage.stagingBuffers.Delete(appState);
			perImage.cachedColors.Delete(appState);
			perImage.cachedIndices32.Delete(appState);
			perImage.cachedIndices16.Delete(appState);
			perImage.cachedAttributes.Delete(appState);
			perImage.cachedVertices.Delete(appState);
			perImage.sceneMatricesStagingBuffers.Delete(appState);
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
			appState.Console.View.framebuffer.colorTextures[i].Delete(appState);
		}
	}
	if (appState.Console.View.framebuffer.renderTexture.image != VK_NULL_HANDLE)
	{
		appState.Console.View.framebuffer.renderTexture.Delete(appState);
	}
	vrapi_DestroyTextureSwapChain(appState.Console.View.framebuffer.colorTextureSwapChain);
	for (auto& perImage : appState.Console.View.perImage)
	{
		VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &perImage.commandBuffer));
		VC(appState.Device.vkDestroyFence(appState.Device.device, perImage.fence, nullptr));
		if (perImage.descriptorResources.created)
		{
			VC(appState.Device.vkDestroyDescriptorPool(appState.Device.device, perImage.descriptorResources.descriptorPool, nullptr));
		}
		if (perImage.texture != nullptr)
		{
			perImage.texture->Delete(appState);
		}
		if (perImage.palette != nullptr)
		{
			perImage.palette->Delete(appState);
		}
		perImage.stagingBuffers.Delete(appState);
		perImage.indices.Delete(appState);
		perImage.vertices.Delete(appState);
	}
	VC(appState.Device.vkDestroyRenderPass(appState.Device.device, appState.Console.RenderPass, nullptr));
	VC(appState.Device.vkFreeCommandBuffers(appState.Device.device, appState.Context.commandPool, 1, &appState.Console.CommandBuffer));
	vrapi_DestroyTextureSwapChain(appState.Console.SwapChain);
	VC(appState.Device.vkDestroyRenderPass(appState.Device.device, appState.RenderPass, nullptr));
	VK(appState.Device.vkQueueWaitIdle(appState.Context.queue));
	VC(appState.Device.vkDestroySampler(appState.Device.device, appState.Scene.lightmapSampler, nullptr));
	for (auto i = 0; i < appState.Scene.textureSamplers.size(); i++)
	{
		VC(appState.Device.vkDestroySampler(appState.Device.device, appState.Scene.textureSamplers[i], nullptr));
	}
	appState.Scene.controllerTexture.Delete(appState);
	appState.Scene.floorTexture.Delete(appState);
	appState.Scene.viewmodelTextures.Delete(appState);
	appState.Scene.aliasTextures.Delete(appState);
	appState.Scene.spriteTextures.Delete(appState);
	appState.Scene.fenceTextures.Delete(appState);
	appState.Scene.surfaceTextures.Delete(appState);
	for (auto& entry : appState.Scene.allocations)
	{
		for (auto& list : entry.second.allocations)
		{
			VC(appState.Device.vkFreeMemory(appState.Device.device, list.memory, nullptr));
		}
	}
	appState.Scene.colormappedBuffers.Delete(appState);
	appState.Scene.console.Delete(appState);
	appState.Scene.floor.Delete(appState);
	appState.Scene.sky.Delete(appState);
	appState.Scene.colored.Delete(appState);
	appState.Scene.viewmodel.Delete(appState);
	appState.Scene.alias.Delete(appState);
	appState.Scene.turbulent.Delete(appState);
	appState.Scene.sprites.Delete(appState);
	appState.Scene.fences.Delete(appState);
	appState.Scene.surfaces.Delete(appState);
	VC(appState.Device.vkDestroyDescriptorSetLayout(appState.Device.device, appState.Scene.doubleImageLayout, nullptr));
	VC(appState.Device.vkDestroyDescriptorSetLayout(appState.Device.device, appState.Scene.singleImageLayout, nullptr));
	VC(appState.Device.vkDestroyDescriptorSetLayout(appState.Device.device, appState.Scene.bufferAndTwoImagesLayout, nullptr));
	VC(appState.Device.vkDestroyDescriptorSetLayout(appState.Device.device, appState.Scene.bufferAndImageLayout, nullptr));
	VC(appState.Device.vkDestroyDescriptorSetLayout(appState.Device.device, appState.Scene.singleBufferLayout, nullptr));
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
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.turbulentVertex, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.spriteFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.spriteVertex, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.fenceFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.surfaceFragment, nullptr));
	VC(appState.Device.vkDestroyShaderModule(appState.Device.device, appState.Scene.surfaceVertex, nullptr));
	appState.Scene.matrices.Destroy(appState);
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
