#include <jni.h>
#include <vulkan/vulkan.h>
#include <string>
#include <locale>
#include <android/log.h>
#include <array>
#include <vector>
#include <cmath>
#include "AppState.h"
#include "Time.h"
#include <list>
#include <map>
#include "vid_ovr.h"
#include "sys_ovr.h"
#include "r_local.h"
#include "EngineThread.h"
#include "in_ovr.h"
#include <common/xr_linear.h>
#include "Utils.h"
#include "Input.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"
#include "DirectRect.h"
#include "CylinderProjection.h"

std::string GetXrVersionString(XrVersion ver)
{
	return Fmt("%d.%d.%d", XR_VERSION_MAJOR(ver), XR_VERSION_MINOR(ver), XR_VERSION_PATCH(ver));
}

void SwitchBoundInput(AppState& appState, XrAction action, const char* name)
{
	XrBoundSourcesForActionEnumerateInfo getInfo { XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO };
	getInfo.action = action;
	uint32_t pathCount = 0;
	CHECK_XRCMD(xrEnumerateBoundSourcesForAction(appState.Session, &getInfo, 0, &pathCount, nullptr));
	std::vector<XrPath> paths(pathCount);
	CHECK_XRCMD(xrEnumerateBoundSourcesForAction(appState.Session, &getInfo, uint32_t(paths.size()), &pathCount, paths.data()));

	std::string sourceName;
	for (uint32_t i = 0; i < pathCount; ++i)
	{
		constexpr XrInputSourceLocalizedNameFlags all = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT |
														XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
														XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

		XrInputSourceLocalizedNameGetInfo nameInfo { XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO };
		nameInfo.sourcePath = paths[i];
		nameInfo.whichComponents = all;

		uint32_t size = 0;
		CHECK_XRCMD(xrGetInputSourceLocalizedName(appState.Session, &nameInfo, 0, &size, nullptr));
		if (size < 1)
		{
			continue;
		}
		std::vector<char> grabSource(size);
		CHECK_XRCMD(xrGetInputSourceLocalizedName(appState.Session, &nameInfo, uint32_t(grabSource.size()), &size, grabSource.data()));
		if (!sourceName.empty())
		{
			sourceName += " and ";
		}
		sourceName += "'";
		sourceName += std::string(grabSource.data(), size - 1);
		sourceName += "'";
	}

	__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", (std::string(name) + " action is bound to %s").c_str(), (!sourceName.empty() ? sourceName.c_str() : "nothing"));
}

const XrEventDataBaseHeader* TryReadNextEvent(XrEventDataBuffer& eventDataBuffer, XrInstance& instance)
{
	auto baseHeader = reinterpret_cast<XrEventDataBaseHeader*>(&eventDataBuffer);
	*baseHeader = { XR_TYPE_EVENT_DATA_BUFFER };
	auto xr = xrPollEvent(instance, &eventDataBuffer);
	if (xr == XR_SUCCESS)
	{
		if (baseHeader->type == XR_TYPE_EVENT_DATA_EVENTS_LOST)
		{
			const XrEventDataEventsLost* const eventsLost = reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
			__android_log_print(ANDROID_LOG_WARN, "slipnfrag_native", "%d events lost", eventsLost->lostEventCount);
		}

		return baseHeader;
	}
	if (xr == XR_EVENT_UNAVAILABLE)
	{
		return nullptr;
	}
	THROW_XR(xr, "xrPollEvent");
}

// Note: The output must not outlive the input - this modifies the input and returns a collection of views into that modified
// input!
std::vector<const char*> ParseExtensionString(char* names)
{
	std::vector<const char*> list;
	while (*names != 0)
	{
		list.push_back(names);
		while (*(++names) != 0)
		{
			if (*names == ' ')
			{
				*names++ = '\0';
				break;
			}
		}
	}
	return list;
}

static VkBool32 DebugReport(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t /*location*/,
							int32_t /*messageCode*/, const char* pLayerPrefix, const char* pMessage, void* /*userData*/)
{
	std::string flagNames;
	std::string objName;
	int priority = ANDROID_LOG_ERROR;

	if ((flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) != 0u)
	{
		flagNames += "DEBUG:";
		priority = ANDROID_LOG_DEBUG;
	}
	if ((flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) != 0u)
	{
		flagNames += "INFO:";
		priority = ANDROID_LOG_INFO;
	}
	if ((flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) != 0u)
	{
		flagNames += "PERF:";
		priority = ANDROID_LOG_WARN;
	}
	if ((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0u)
	{
		flagNames += "WARN:";
		priority = ANDROID_LOG_WARN;
	}
	if ((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0u)
	{
		flagNames += "ERROR:";
		priority = ANDROID_LOG_ERROR;
	}

#define LIST_OBJECT_TYPES(_) \
    _(UNKNOWN)               \
    _(INSTANCE)              \
    _(PHYSICAL_DEVICE)       \
    _(DEVICE)                \
    _(QUEUE)                 \
    _(SEMAPHORE)             \
    _(COMMAND_BUFFER)        \
    _(FENCE)                 \
    _(DEVICE_MEMORY)         \
    _(BUFFER)                \
    _(IMAGE)                 \
    _(EVENT)                 \
    _(QUERY_POOL)            \
    _(BUFFER_VIEW)           \
    _(IMAGE_VIEW)            \
    _(SHADER_MODULE)         \
    _(PIPELINE_CACHE)        \
    _(PIPELINE_LAYOUT)       \
    _(RENDER_PASS)           \
    _(PIPELINE)              \
    _(DESCRIPTOR_SET_LAYOUT) \
    _(SAMPLER)               \
    _(DESCRIPTOR_POOL)       \
    _(DESCRIPTOR_SET)        \
    _(FRAMEBUFFER)           \
    _(COMMAND_POOL)          \
    _(SURFACE_KHR)           \
    _(SWAPCHAIN_KHR)         \
    _(DISPLAY_KHR)           \
    _(DISPLAY_MODE_KHR)

	switch (objectType)
	{
		default:
#define MK_OBJECT_TYPE_CASE(name)                  \
    case VK_DEBUG_REPORT_OBJECT_TYPE_##name##_EXT: \
        objName = #name;                           \
        break;
		LIST_OBJECT_TYPES(MK_OBJECT_TYPE_CASE)

#if VK_HEADER_VERSION >= 46
		MK_OBJECT_TYPE_CASE(DESCRIPTOR_UPDATE_TEMPLATE_KHR)
#endif
#if VK_HEADER_VERSION >= 70
		MK_OBJECT_TYPE_CASE(DEBUG_REPORT_CALLBACK_EXT)
#endif
	}

	if ((objectType == VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT) && (strcmp(pLayerPrefix, "Loader Message") == 0) &&
		(strncmp(pMessage, "Device Extension:", 17) == 0))
	{
		return VK_FALSE;
	}

	__android_log_print(priority, "slipnfrag_native", "%s (%s 0x%llx) [%s] %s", flagNames.c_str(), objName.c_str(), object, pLayerPrefix, pMessage);
	if ((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0u)
	{
		return VK_FALSE;
	}
	if ((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0u)
	{
		return VK_FALSE;
	}
	return VK_FALSE;
}

void LogExtensions(const char* layerName, int indent = 0)
{
	uint32_t instanceExtensionCount;
	CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, 0, &instanceExtensionCount, nullptr));

	std::vector<XrExtensionProperties> extensions(instanceExtensionCount);
	for (XrExtensionProperties& extension : extensions)
	{
		extension.type = XR_TYPE_EXTENSION_PROPERTIES;
	}

	CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layerName, (uint32_t) extensions.size(), &instanceExtensionCount, extensions.data()));

	const std::string indentStr(indent, ' ');
	__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "%sAvailable Extensions: (%d)", indentStr.c_str(), instanceExtensionCount);
	for (const XrExtensionProperties& extension : extensions)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "%s  Name=%s SpecVersion=%d", indentStr.c_str(), extension.extensionName, extension.extensionVersion);
	}
}

static void AppHandleCommand(struct android_app* app, int32_t cmd)
{
	AppState* appState = (AppState*)app->userData;
	double delta;

	switch (cmd)
	{
		case APP_CMD_START:
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_START");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "onStart()");
			break;
		case APP_CMD_RESUME:
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "onResume()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_RESUME");
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
		case APP_CMD_PAUSE:
		{
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "onPause()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_PAUSE");
			appState->Resumed = false;
			break;
		}
		case APP_CMD_STOP:
		{
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "onStop()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_STOP");
			break;
		}
		case APP_CMD_DESTROY:
		{
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "onDestroy()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_DESTROY");
			appState->NativeWindow = nullptr;
			break;
		}
		case APP_CMD_INIT_WINDOW:
		{
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "surfaceCreated()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_INIT_WINDOW");
			appState->NativeWindow = app->window;
			break;
		}
		case APP_CMD_TERM_WINDOW:
		{
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "surfaceDestroyed()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_TERM_WINDOW");
			appState->NativeWindow = nullptr;
			break;
		}
	}
}

void android_main(struct android_app* app)
{
	JNIEnv* Env;
	app->activity->vm->AttachCurrentThread(&Env, nullptr);

	try
	{
		AppState appState { };

		appState.PausedTime = -1;
		appState.PreviousTime = -1;
		appState.CurrentTime = -1;

		app->userData = &appState;
		app->onAppCmd = AppHandleCommand;

		XrGraphicsBindingVulkan2KHR graphicsBinding { XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR };

		VkInstance vulkanInstance = VK_NULL_HANDLE;
		VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;
		uint32_t queueFamilyIndex = 0;

		PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = nullptr;
		PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = nullptr;
		VkDebugReportCallbackEXT vulkanDebugReporter = VK_NULL_HANDLE;

		std::vector<XrView> m_views;
		
		PFN_xrInitializeLoaderKHR initializeLoader = nullptr;
		XrResult res = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*) (&initializeLoader));
		CHECK_XRRESULT(res, "xrGetInstanceProcAddr");
		XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid;
		memset(&loaderInitInfoAndroid, 0, sizeof(loaderInitInfoAndroid));
		loaderInitInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
		loaderInitInfoAndroid.next = nullptr;
		loaderInitInfoAndroid.applicationVM = app->activity->vm;
		loaderInitInfoAndroid.applicationContext = app->activity->clazz;
		initializeLoader((const XrLoaderInitInfoBaseHeaderKHR*) &loaderInitInfoAndroid);

		LogExtensions(nullptr);

		{
			uint32_t layerCount;
			CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));

			std::vector<XrApiLayerProperties> layers(layerCount);
			for (XrApiLayerProperties& layer : layers)
			{
				layer.type = XR_TYPE_API_LAYER_PROPERTIES;
			}

			CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t) layers.size(), &layerCount, layers.data()));

			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Available Layers: (%d)", layerCount);
			for (const XrApiLayerProperties& layer : layers)
			{
				__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "  Name=%s SpecVersion=%s LayerVersion=%d Description=%s", layer.layerName, GetXrVersionString(layer.specVersion).c_str(), layer.layerVersion, layer.description);
				LogExtensions(layer.layerName, 4);
			}
		}

		std::vector<const char*> xrInstanceExtensions;
		std::vector<std::string> xrInstanceExtensionSources
		{
			XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
			XR_KHR_VULKAN_ENABLE_EXTENSION_NAME,
			XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME
		};
		std::transform(xrInstanceExtensionSources.begin(), xrInstanceExtensionSources.end(), std::back_inserter(xrInstanceExtensions), [](const std::string& extension)
		{ 
			return extension.c_str(); 
		});

		std::vector<const char*> xrInstanceApiLayers;
		std::vector<std::string> xrInstanceApiLayerSources
		{
			//"XR_APILAYER_LUNARG_core_validation"
		};
		std::transform(xrInstanceApiLayerSources.begin(), xrInstanceApiLayerSources.end(), std::back_inserter(xrInstanceApiLayers), [](const std::string& layer)
		{
			return layer.c_str();
		});

		XrInstanceCreateInfoAndroidKHR instanceCreateInfoAndroid { XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR };
		instanceCreateInfoAndroid.applicationVM = app->activity->vm;
		instanceCreateInfoAndroid.applicationActivity = app->activity->clazz;

		XrInstance instance = XR_NULL_HANDLE;

		XrInstanceCreateInfo createInfo { XR_TYPE_INSTANCE_CREATE_INFO };
		createInfo.next = (XrBaseInStructure*)&instanceCreateInfoAndroid;
		createInfo.enabledExtensionCount = (uint32_t)xrInstanceExtensions.size();
		createInfo.enabledExtensionNames = xrInstanceExtensions.data();
		createInfo.enabledApiLayerCount = (uint32_t)xrInstanceApiLayers.size();
		createInfo.enabledApiLayerNames = xrInstanceApiLayers.data();

		strcpy(createInfo.applicationInfo.applicationName, "SlipNFrag-XR");
		createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

		CHECK_XRCMD(xrCreateInstance(&createInfo, &instance));

		CHECK(instance != XR_NULL_HANDLE);

		XrInstanceProperties instanceProperties { XR_TYPE_INSTANCE_PROPERTIES };
		CHECK_XRCMD(xrGetInstanceProperties(instance, &instanceProperties));

		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Instance RuntimeName=%s RuntimeVersion=%s", instanceProperties.runtimeName, GetXrVersionString(instanceProperties.runtimeVersion).c_str());

		auto m_viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

		XrSystemId systemId = XR_NULL_SYSTEM_ID;
		XrSystemGetInfo systemInfo { XR_TYPE_SYSTEM_GET_INFO };
		systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		CHECK_XRCMD(xrGetSystem(instance, &systemInfo, &systemId));

		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Using system %d for form factor %s", systemId, to_string(systemInfo.formFactor));

		CHECK(systemId != XR_NULL_SYSTEM_ID);

		uint32_t viewConfigTypeCount;
		CHECK_XRCMD(xrEnumerateViewConfigurations(instance, systemId, 0, &viewConfigTypeCount, nullptr));
		std::vector<XrViewConfigurationType> viewConfigTypes(viewConfigTypeCount);
		CHECK_XRCMD(xrEnumerateViewConfigurations(instance, systemId, viewConfigTypeCount, &viewConfigTypeCount, viewConfigTypes.data()));
		CHECK((uint32_t) viewConfigTypes.size() == viewConfigTypeCount);

		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Available View Configuration Types: (%d)", viewConfigTypeCount);
		for (XrViewConfigurationType viewConfigType : viewConfigTypes)
		{
			__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "  View Configuration Type: %s %s", to_string(viewConfigType), viewConfigType == m_viewConfigType ? "(Selected)" : "");

			XrViewConfigurationProperties viewConfigProperties { XR_TYPE_VIEW_CONFIGURATION_PROPERTIES };
			CHECK_XRCMD(xrGetViewConfigurationProperties(instance, systemId, viewConfigType, &viewConfigProperties));

			__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "  View configuration FovMutable=%s", (viewConfigProperties.fovMutable == XR_TRUE ? "True" : "False"));

			uint32_t viewCount;
			CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId, viewConfigType, 0, &viewCount, nullptr));
			if (viewCount > 0)
			{
				std::vector<XrViewConfigurationView> views(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
				CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId, viewConfigType, viewCount, &viewCount, views.data()));

				for (uint32_t i = 0; i < views.size(); i++)
				{
					const XrViewConfigurationView& view = views[i];

					__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "    View [%d]: Recommended Width=%d Height=%d SampleCount=%d", i, view.recommendedImageRectWidth, view.recommendedImageRectHeight, view.recommendedSwapchainSampleCount);
					__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "    View [%d]:     Maximum Width=%d Height=%d SampleCount=%d", i, view.maxImageRectWidth, view.maxImageRectHeight, view.maxSwapchainSampleCount);
				}
			}
			else
			{
				__android_log_print(ANDROID_LOG_ERROR, "slipnfrag_native", "Empty view configuration type");
			}

			uint32_t count;
			CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigType, 0, &count, nullptr));
			CHECK(count > 0);

			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Available Environment Blend Mode count : (%d)", count);

			std::vector<XrEnvironmentBlendMode> blendModes(count);
			CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigType, count, &count, blendModes.data()));

			bool blendModeFound = false;
			for (XrEnvironmentBlendMode mode : blendModes)
			{
				const bool blendModeMatch = (mode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE);
				__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Environment Blend Mode (%s) : %s", to_string(mode), blendModeMatch ? "(Selected)" : "");
				blendModeFound |= blendModeMatch;
			}
			CHECK(blendModeFound);
		}

		XrGraphicsRequirementsVulkan2KHR graphicsRequirements { XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN2_KHR };

		PFN_xrGetVulkanGraphicsRequirementsKHR xrGetVulkanGraphicsRequirementsKHR = nullptr;
		CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrGetVulkanGraphicsRequirementsKHR)));

		XrGraphicsRequirementsVulkanKHR legacyRequirements { XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR };
		CHECK_XRCMD(xrGetVulkanGraphicsRequirementsKHR(instance, systemId, &legacyRequirements));

		graphicsRequirements.maxApiVersionSupported = legacyRequirements.maxApiVersionSupported;
		graphicsRequirements.minApiVersionSupported = legacyRequirements.minApiVersionSupported;

		std::vector<const char*> layers;
		
#if !defined(NDEBUG)
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		std::vector<const char*> validationLayerNames 
		{ 
			"VK_LAYER_KHRONOS_validation",
			"VK_LAYER_LUNARG_standard_validation"
		};

		auto validationLayerFound = false;
		for (auto& validationLayerName : validationLayerNames)
		{
			for (const auto& layerProperties : availableLayers)
			{
				if (0 == strcmp(validationLayerName, layerProperties.layerName))
				{
					layers.push_back(validationLayerName);
					validationLayerFound = true;
				}
			}
		}
		if (!validationLayerFound)
		{
			__android_log_print(ANDROID_LOG_WARN, "slipnfrag_native", "No validation layers found in the system, skipping");
		}
#endif

		{
			std::vector<const char*> vulkanExtensions;
			vulkanExtensions.push_back("VK_EXT_debug_report");

			VkApplicationInfo appInfo { VK_STRUCTURE_TYPE_APPLICATION_INFO };
			appInfo.pApplicationName = "slipnfrag_xr";
			appInfo.applicationVersion = 1;
			appInfo.pEngineName = "slipnfrag_xr";
			appInfo.engineVersion = 1;
			appInfo.apiVersion = VK_API_VERSION_1_0;

			VkInstanceCreateInfo instInfo { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
			instInfo.pApplicationInfo = &appInfo;
			instInfo.enabledLayerCount = (uint32_t)layers.size();
			instInfo.ppEnabledLayerNames = (layers.empty() ? nullptr : layers.data());
			instInfo.enabledExtensionCount = (uint32_t)vulkanExtensions.size();
			instInfo.ppEnabledExtensionNames = (vulkanExtensions.empty() ? nullptr : vulkanExtensions.data());

			XrVulkanInstanceCreateInfoKHR vulkanCreateInfo { XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR };
			vulkanCreateInfo.systemId = systemId;
			vulkanCreateInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
			vulkanCreateInfo.vulkanCreateInfo = &instInfo;
			vulkanCreateInfo.vulkanAllocator = nullptr;

			PFN_xrGetVulkanInstanceExtensionsKHR xrGetVulkanInstanceExtensionsKHR = nullptr;
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetVulkanInstanceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrGetVulkanInstanceExtensionsKHR)));

			uint32_t extensionNamesSize = 0;
			CHECK_XRCMD(xrGetVulkanInstanceExtensionsKHR(instance, vulkanCreateInfo.systemId, 0, &extensionNamesSize, nullptr));

			std::vector<char> extensionNames(extensionNamesSize);
			CHECK_XRCMD(xrGetVulkanInstanceExtensionsKHR(instance, vulkanCreateInfo.systemId, extensionNamesSize, &extensionNamesSize, &extensionNames[0]));
			{
				std::vector<const char*> extensions = ParseExtensionString(&extensionNames[0]);

				for (uint32_t i = 0; i < vulkanCreateInfo.vulkanCreateInfo->enabledExtensionCount; ++i)
				{
					extensions.push_back(vulkanCreateInfo.vulkanCreateInfo->ppEnabledExtensionNames[i]);
				}

				VkInstanceCreateInfo instInfo { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
				memcpy(&instInfo, vulkanCreateInfo.vulkanCreateInfo, sizeof(instInfo));
				instInfo.enabledExtensionCount = (uint32_t) extensions.size();
				instInfo.ppEnabledExtensionNames = extensions.empty() ? nullptr : extensions.data();

				auto vkCreateInstance = (PFN_vkCreateInstance)vulkanCreateInfo.pfnGetInstanceProcAddr(nullptr, "vkCreateInstance");
				CHECK_VKCMD(vkCreateInstance(&instInfo, vulkanCreateInfo.vulkanAllocator, &vulkanInstance));
			}

			vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(vulkanInstance, "vkCreateDebugReportCallbackEXT");
			vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(vulkanInstance, "vkDestroyDebugReportCallbackEXT");
			VkDebugReportCallbackCreateInfoEXT debugInfo { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
			debugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
#if !defined(NDEBUG)
			debugInfo.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
#endif
			debugInfo.pfnCallback = &DebugReport;
			CHECK_VKCMD(vkCreateDebugReportCallbackEXT(vulkanInstance, &debugInfo, nullptr, &vulkanDebugReporter));

			XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo { XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR };
			deviceGetInfo.systemId = systemId;
			deviceGetInfo.vulkanInstance = vulkanInstance;

			PFN_xrGetVulkanGraphicsDeviceKHR xrGetVulkanGraphicsDeviceKHR = nullptr;
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDeviceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrGetVulkanGraphicsDeviceKHR)));

			if (deviceGetInfo.next != nullptr)
			{
				CHECK_XRCMD(XR_ERROR_FEATURE_UNSUPPORTED);
			}

			CHECK_XRCMD(xrGetVulkanGraphicsDeviceKHR(instance, deviceGetInfo.systemId, deviceGetInfo.vulkanInstance, &vulkanPhysicalDevice));

			VkDeviceQueueCreateInfo queueInfo { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			float queuePriorities = 0;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &queuePriorities;

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(vulkanPhysicalDevice, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(vulkanPhysicalDevice, &queueFamilyCount, &queueFamilyProps[0]);

			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				if ((queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u)
				{
					queueFamilyIndex = queueInfo.queueFamilyIndex = i;
					break;
				}
			}

			std::vector<const char*> deviceExtensions;

			VkPhysicalDeviceFeatures features { };
			features.samplerAnisotropy = VK_TRUE;

			VkDeviceCreateInfo deviceInfo { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
			deviceInfo.queueCreateInfoCount = 1;
			deviceInfo.pQueueCreateInfos = &queueInfo;
			deviceInfo.enabledLayerCount = 0;
			deviceInfo.ppEnabledLayerNames = nullptr;
			deviceInfo.enabledExtensionCount = (uint32_t) deviceExtensions.size();
			deviceInfo.ppEnabledExtensionNames = deviceExtensions.empty() ? nullptr : deviceExtensions.data();
			deviceInfo.pEnabledFeatures = &features;

			XrVulkanDeviceCreateInfoKHR deviceCreateInfo { XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR };
			deviceCreateInfo.systemId = systemId;
			deviceCreateInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
			deviceCreateInfo.vulkanCreateInfo = &deviceInfo;
			deviceCreateInfo.vulkanPhysicalDevice = vulkanPhysicalDevice;
			deviceCreateInfo.vulkanAllocator = nullptr;

			PFN_xrGetVulkanDeviceExtensionsKHR xrGetVulkanDeviceExtensionsKHR = nullptr;
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetVulkanDeviceExtensionsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrGetVulkanDeviceExtensionsKHR)));

			uint32_t deviceExtensionNamesSize = 0;
			CHECK_XRCMD(xrGetVulkanDeviceExtensionsKHR(instance, vulkanCreateInfo.systemId, 0, &deviceExtensionNamesSize, nullptr));
			std::vector<char> deviceExtensionNames(deviceExtensionNamesSize);
			CHECK_XRCMD(xrGetVulkanDeviceExtensionsKHR(instance, vulkanCreateInfo.systemId, deviceExtensionNamesSize, &deviceExtensionNamesSize, &deviceExtensionNames[0]));

			std::vector<const char*> enabledExtensions = ParseExtensionString(&deviceExtensionNames[0]);

			for (uint32_t i = 0; i < deviceCreateInfo.vulkanCreateInfo->enabledExtensionCount; ++i)
			{
				enabledExtensions.push_back(deviceCreateInfo.vulkanCreateInfo->ppEnabledExtensionNames[i]);
			}

			bool supportsMultiview = false;
			auto vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(vulkanInstance, "vkEnumerateDeviceExtensionProperties");
			uint32_t availableExtensionCount = 0;
			vkEnumerateDeviceExtensionProperties(vulkanPhysicalDevice, nullptr, &availableExtensionCount, nullptr);
			std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
			vkEnumerateDeviceExtensionProperties(vulkanPhysicalDevice, nullptr, &availableExtensionCount, availableExtensions.data());
			for (int extensionIdx = 0; extensionIdx < availableExtensionCount; extensionIdx++)
			{
				if (strcmp(availableExtensions[extensionIdx].extensionName, VK_KHR_MULTIVIEW_EXTENSION_NAME) == 0)
				{
					supportsMultiview = true;
					VkPhysicalDeviceFeatures2 physicalDeviceFeatures { };
					physicalDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
					physicalDeviceFeatures.pNext = nullptr;
					VkPhysicalDeviceProperties2 physicalDeviceProperties { };
					physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
					physicalDeviceProperties.pNext = nullptr;
					VkPhysicalDeviceMultiviewFeatures deviceMultiviewFeatures { };
					VkPhysicalDeviceMultiviewProperties deviceMultiviewProperties { };
					deviceMultiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
					physicalDeviceFeatures.pNext = &deviceMultiviewFeatures;
					deviceMultiviewProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES;
					physicalDeviceProperties.pNext = &deviceMultiviewProperties;
					auto vkGetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR) vkGetInstanceProcAddr(vulkanInstance, "vkGetPhysicalDeviceFeatures2KHR");
					vkGetPhysicalDeviceFeatures2KHR(vulkanPhysicalDevice, &physicalDeviceFeatures);
					auto vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR) vkGetInstanceProcAddr(vulkanInstance, "vkGetPhysicalDeviceProperties2KHR");
					vkGetPhysicalDeviceProperties2KHR(vulkanPhysicalDevice, &physicalDeviceProperties);
					supportsMultiview = (bool)deviceMultiviewFeatures.multiview;
					if (supportsMultiview)
					{
						__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Device supports multiview rendering, with %d views and %u max instances", deviceMultiviewProperties.maxMultiviewViewCount, deviceMultiviewProperties.maxMultiviewInstanceIndex);
						char *extensionNameToAdd = new char[strlen(VK_KHR_MULTIVIEW_EXTENSION_NAME) + 1];
						strcpy(extensionNameToAdd, VK_KHR_MULTIVIEW_EXTENSION_NAME);
						enabledExtensions.push_back(extensionNameToAdd);
					}
					break;
				}
			}

			if (!supportsMultiview)
			{
				THROW("Device does not support multiview rendering.");
			}

			memcpy(&features, deviceCreateInfo.vulkanCreateInfo->pEnabledFeatures, sizeof(features));
			memcpy(&deviceInfo, deviceCreateInfo.vulkanCreateInfo, sizeof(deviceInfo));
			deviceInfo.pEnabledFeatures = &features;
			deviceInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
			deviceInfo.ppEnabledExtensionNames = (enabledExtensions.empty() ? nullptr : enabledExtensions.data());

			auto vkCreateDevice = (PFN_vkCreateDevice)deviceCreateInfo.pfnGetInstanceProcAddr(vulkanInstance, "vkCreateDevice");
			CHECK_VKCMD(vkCreateDevice(vulkanPhysicalDevice, &deviceInfo, deviceCreateInfo.vulkanAllocator, &appState.Device));

			vkGetDeviceQueue(appState.Device, queueInfo.queueFamilyIndex, 0, &appState.Queue);

			vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice, &appState.MemoryProperties);

			graphicsBinding.instance = vulkanInstance;
			graphicsBinding.physicalDevice = vulkanPhysicalDevice;
			graphicsBinding.device = appState.Device;
			graphicsBinding.queueFamilyIndex = queueInfo.queueFamilyIndex;
			graphicsBinding.queueIndex = 0;
		}

		CHECK(instance != XR_NULL_HANDLE);

		VkCommandPoolCreateInfo commandPoolCreateInfo { };
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
		CHECK_VKCMD(vkCreateCommandPool(appState.Device, &commandPoolCreateInfo, nullptr, &appState.CommandPool));

		VkFenceCreateInfo fenceCreateInfo { };
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		VkCommandBufferAllocateInfo commandBufferAllocateInfo { };
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.commandPool = appState.CommandPool;
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
		CHECK_VKCMD(vkCreatePipelineCache(appState.Device, &pipelineCacheCreateInfo, nullptr, &appState.PipelineCache));

		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Creating session...");

		XrSessionCreateInfo sessionCreateInfo { XR_TYPE_SESSION_CREATE_INFO };
		sessionCreateInfo.next = reinterpret_cast<const XrBaseInStructure*>(&graphicsBinding);
		sessionCreateInfo.systemId = systemId;
		CHECK_XRCMD(xrCreateSession(instance, &sessionCreateInfo, &appState.Session));
		CHECK(appState.Session != XR_NULL_HANDLE);

		uint32_t spaceCount;
		CHECK_XRCMD(xrEnumerateReferenceSpaces(appState.Session, 0, &spaceCount, nullptr));
		std::vector<XrReferenceSpaceType> spaces(spaceCount);
		CHECK_XRCMD(xrEnumerateReferenceSpaces(appState.Session, spaceCount, &spaceCount, spaces.data()));

		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Available reference spaces: %d", spaceCount);
		for (XrReferenceSpaceType space : spaces)
		{
			__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "  Name: %s", to_string(space));
		}

		appState.SubactionPaths.resize(2);
		appState.HandSpaces.resize(2);
		appState.HandScales.resize(2);
		appState.ActiveHands.resize(2);
		
		XrActionSetCreateInfo actionSetInfo { XR_TYPE_ACTION_SET_CREATE_INFO };
		strcpy(actionSetInfo.actionSetName, "gameplay");
		strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
		actionSetInfo.priority = 0;
		CHECK_XRCMD(xrCreateActionSet(instance, &actionSetInfo, &appState.ActionSet));

		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left", &appState.SubactionPaths[0]));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right", &appState.SubactionPaths[1]));

		XrActionCreateInfo actionInfo { XR_TYPE_ACTION_CREATE_INFO };

		actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(actionInfo.actionName, "play_1");
		strcpy(actionInfo.localizedActionName, "Play 1");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.Play1Action));

		strcpy(actionInfo.actionName, "play_2");
		strcpy(actionInfo.localizedActionName, "Play 2");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.Play2Action));

		strcpy(actionInfo.actionName, "jump");
		strcpy(actionInfo.localizedActionName, "Jump");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.JumpAction));

		strcpy(actionInfo.actionName, "swim_down");
		strcpy(actionInfo.localizedActionName, "Swim down");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.SwimDownAction));

		strcpy(actionInfo.actionName, "run");
		strcpy(actionInfo.localizedActionName, "Run");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.RunAction));

		strcpy(actionInfo.actionName, "fire");
		strcpy(actionInfo.localizedActionName, "Fire");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.FireAction));

		actionInfo.actionType = XR_ACTION_TYPE_FLOAT_INPUT;
		strcpy(actionInfo.actionName, "move_x");
		strcpy(actionInfo.localizedActionName, "Move X");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.MoveXAction));

		strcpy(actionInfo.actionName, "move_y");
		strcpy(actionInfo.localizedActionName, "Move Y");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.MoveYAction));

		actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(actionInfo.actionName, "switch_weapon");
		strcpy(actionInfo.localizedActionName, "Switch weapon");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.SwitchWeaponAction));

		strcpy(actionInfo.actionName, "menu");
		strcpy(actionInfo.localizedActionName, "Menu");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.MenuAction));

		strcpy(actionInfo.actionName, "enter_trigger");
		strcpy(actionInfo.localizedActionName, "Enter (with triggers)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.EnterTriggerAction));

		strcpy(actionInfo.actionName, "enter_non_trigger");
		strcpy(actionInfo.localizedActionName, "Enter (without triggers)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.EnterNonTriggerAction));

		strcpy(actionInfo.actionName, "escape_y");
		strcpy(actionInfo.localizedActionName, "Escape (plus Y)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.EscapeYAction));

		strcpy(actionInfo.actionName, "escape_non_y");
		strcpy(actionInfo.localizedActionName, "Escape (minus Y)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.EscapeNonYAction));

		actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(actionInfo.actionName, "quit");
		strcpy(actionInfo.localizedActionName, "Quit");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.QuitAction));

		actionInfo.actionType = XR_ACTION_TYPE_POSE_INPUT;
		strcpy(actionInfo.actionName, "hand_pose");
		strcpy(actionInfo.localizedActionName, "Hand pose");
		actionInfo.countSubactionPaths = (uint32_t)appState.SubactionPaths.size();
		actionInfo.subactionPaths = appState.SubactionPaths.data();
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.PoseAction));

		actionInfo.actionType = XR_ACTION_TYPE_BOOLEAN_INPUT;
		strcpy(actionInfo.actionName, "left_key_press");
		strcpy(actionInfo.localizedActionName, "Left key press");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.LeftKeyPressAction));

		strcpy(actionInfo.actionName, "right_key_press");
		strcpy(actionInfo.localizedActionName, "Right key press");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.RightKeyPressAction));

		XrPath aClick;
		XrPath bClick;
		XrPath xClick;
		XrPath yClick;
		XrPath leftTrigger;
		XrPath rightTrigger;
		XrPath leftSqueeze;
		XrPath rightSqueeze;
		XrPath leftThumbstickX;
		XrPath leftThumbstickY;
		XrPath rightThumbstickX;
		XrPath rightThumbstickY;
		XrPath leftThumbstickClick;
		XrPath rightThumbstickClick;
		XrPath menuClick;
		XrPath leftPose;
		XrPath rightPose;
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/a/click", &aClick));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/b/click", &bClick));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/x/click", &xClick));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/y/click", &yClick));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/trigger/value", &leftTrigger));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/trigger/value", &rightTrigger));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/squeeze/value", &leftSqueeze));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/squeeze/value", &rightSqueeze));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/thumbstick/x", &leftThumbstickX));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/thumbstick/y", &leftThumbstickY));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/thumbstick/x", &rightThumbstickX));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/thumbstick/y", &rightThumbstickY));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/thumbstick/click", &leftThumbstickClick));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/thumbstick/click", &rightThumbstickClick));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/menu/click", &menuClick));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/left/input/aim/pose", &leftPose));
		CHECK_XRCMD(xrStringToPath(instance, "/user/hand/right/input/aim/pose", &rightPose));

		XrPath interaction;
		CHECK_XRCMD(xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller", &interaction));
		std::vector<XrActionSuggestedBinding> bindings
		{
			{
				{ appState.Play1Action, aClick },
				{ appState.Play2Action, xClick },
				{ appState.JumpAction, bClick },
				{ appState.JumpAction, yClick },
				{ appState.SwimDownAction, aClick },
				{ appState.SwimDownAction, xClick },
				{ appState.RunAction, leftSqueeze },
				{ appState.RunAction, rightSqueeze },
				{ appState.FireAction, leftTrigger },
				{ appState.FireAction, rightTrigger },
				{ appState.MoveXAction, leftThumbstickX },
				{ appState.MoveXAction, rightThumbstickX },
				{ appState.MoveYAction, leftThumbstickY },
				{ appState.MoveYAction, rightThumbstickY },
				{ appState.SwitchWeaponAction, leftThumbstickClick },
				{ appState.SwitchWeaponAction, rightThumbstickClick },
				{ appState.MenuAction, menuClick },
				{ appState.EnterTriggerAction, leftTrigger },
				{ appState.EnterTriggerAction, rightTrigger },
				{ appState.EnterNonTriggerAction, aClick },
				{ appState.EnterNonTriggerAction, xClick },
				{ appState.EscapeYAction, leftSqueeze },
				{ appState.EscapeYAction, rightSqueeze },
				{ appState.EscapeYAction, bClick },
				{ appState.EscapeYAction, yClick },
				{ appState.EscapeNonYAction, leftSqueeze },
				{ appState.EscapeNonYAction, rightSqueeze },
				{ appState.EscapeNonYAction, bClick },
				{ appState.QuitAction, yClick },
				{ appState.PoseAction, leftPose },
				{ appState.PoseAction, rightPose },
				{ appState.LeftKeyPressAction, leftTrigger },
				{ appState.RightKeyPressAction, rightTrigger }
			}
		};
		
		XrInteractionProfileSuggestedBinding suggestedBindings { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
		suggestedBindings.interactionProfile = interaction;
		suggestedBindings.suggestedBindings = bindings.data();
		suggestedBindings.countSuggestedBindings = (uint32_t)bindings.size();
		CHECK_XRCMD(xrSuggestInteractionProfileBindings(instance, &suggestedBindings));

		XrActionSpaceCreateInfo actionSpaceInfo { XR_TYPE_ACTION_SPACE_CREATE_INFO };
		actionSpaceInfo.action = appState.PoseAction;
		actionSpaceInfo.poseInActionSpace.orientation.w = 1.f;
		actionSpaceInfo.subactionPath = appState.SubactionPaths[0];
		CHECK_XRCMD(xrCreateActionSpace(appState.Session, &actionSpaceInfo, &appState.HandSpaces[0]));
		actionSpaceInfo.subactionPath = appState.SubactionPaths[1];
		CHECK_XRCMD(xrCreateActionSpace(appState.Session, &actionSpaceInfo, &appState.HandSpaces[1]));

		XrSessionActionSetsAttachInfo attachInfo { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
		attachInfo.countActionSets = 1;
		attachInfo.actionSets = &appState.ActionSet;
		CHECK_XRCMD(xrAttachSessionActionSets(appState.Session, &attachInfo));

		XrReferenceSpaceCreateInfo referenceSpaceCreateInfo { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
		referenceSpaceCreateInfo.poseInReferenceSpace.orientation.w = 1;

		XrSpace worldSpace = XR_NULL_HANDLE;
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
		CHECK_XRCMD(xrCreateReferenceSpace(appState.Session, &referenceSpaceCreateInfo, &worldSpace));

		XrSpace appSpace = XR_NULL_HANDLE;
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
		CHECK_XRCMD(xrCreateReferenceSpace(appState.Session, &referenceSpaceCreateInfo, &appSpace));

		XrSpace screenSpace = XR_NULL_HANDLE;
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		CHECK_XRCMD(xrCreateReferenceSpace(appState.Session, &referenceSpaceCreateInfo, &screenSpace));

		XrSpace keyboardSpace = XR_NULL_HANDLE;
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		referenceSpaceCreateInfo.poseInReferenceSpace.position.y = -CylinderProjection::screenLowerLimit - CylinderProjection::keyboardLowerLimit;
		CHECK_XRCMD(xrCreateReferenceSpace(appState.Session, &referenceSpaceCreateInfo, &keyboardSpace));

		XrSpace consoleKeyboardSpace = XR_NULL_HANDLE;
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		referenceSpaceCreateInfo.poseInReferenceSpace.position.y = -CylinderProjection::keyboardLowerLimit;
		CHECK_XRCMD(xrCreateReferenceSpace(appState.Session, &referenceSpaceCreateInfo, &consoleKeyboardSpace));

		XrSystemProperties systemProperties { XR_TYPE_SYSTEM_PROPERTIES };
		CHECK_XRCMD(xrGetSystemProperties(instance, systemId, &systemProperties));

		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "System Properties: Name=%s VendorId=%d", systemProperties.systemName, systemProperties.vendorId);
		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "System Graphics Properties: MaxWidth=%d MaxHeight=%d MaxLayers=%d", systemProperties.graphicsProperties.maxSwapchainImageWidth, systemProperties.graphicsProperties.maxSwapchainImageHeight, systemProperties.graphicsProperties.maxLayerCount);
		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "System Tracking Properties: OrientationTracking=%s PositionTracking=%s", (systemProperties.trackingProperties.orientationTracking == XR_TRUE ? "True" : "False"), (systemProperties.trackingProperties.positionTracking == XR_TRUE ? "True" : "False"));

		CHECK_MSG(m_viewConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, "Unsupported view configuration type");

		uint32_t viewCount;
		CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId, m_viewConfigType, 0, &viewCount, nullptr));
		std::vector<XrViewConfigurationView> configViews;
		configViews.resize(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
		CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId, m_viewConfigType, viewCount, &viewCount, configViews.data()));

		for (auto configView : configViews)
		{
			appState.EyeTextureWidth = configView.recommendedImageRectWidth;
			appState.EyeTextureHeight = configView.recommendedImageRectHeight;
			appState.EyeTextureMaxDimension = std::max(appState.EyeTextureWidth, appState.EyeTextureHeight);
			break;
		}

		m_views.resize(viewCount, { XR_TYPE_VIEW });

		uint32_t swapchainFormatCount;
		CHECK_XRCMD(xrEnumerateSwapchainFormats(appState.Session, 0, &swapchainFormatCount, nullptr));
		std::vector<int64_t> swapchainFormats(swapchainFormatCount);
		CHECK_XRCMD(xrEnumerateSwapchainFormats(appState.Session, (uint32_t) swapchainFormats.size(), &swapchainFormatCount, swapchainFormats.data()));
		CHECK(swapchainFormatCount == swapchainFormats.size());

		auto found = false;
		for (auto format : swapchainFormats)
		{
			if (format == (int64_t)Constants::colorFormat)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			THROW(Fmt("No runtime swapchain format supported for color swapchain %i", Constants::colorFormat));
		}

		std::string swapchainFormatsString;
		for (int64_t format : swapchainFormats)
		{
			const bool selected = (format == Constants::colorFormat);
			swapchainFormatsString += " ";
			if (selected)
			{
				swapchainFormatsString += "[";
			}
			swapchainFormatsString += std::to_string(format);
			if (selected)
			{
				swapchainFormatsString += "]";
			}
		}
		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Swapchain Formats: %s", swapchainFormatsString.c_str());

		appState.SwapchainWidth = configViews[0].recommendedImageRectWidth;
		appState.SwapchainHeight = configViews[0].recommendedImageRectHeight;
		appState.SwapchainSampleCount = configViews[0].recommendedSwapchainSampleCount;
		
		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Creating swapchain with dimensions Width=%d Height=%d SampleCount=%d", appState.SwapchainWidth, appState.SwapchainHeight, appState.SwapchainSampleCount);

		XrSwapchainCreateInfo swapchainCreateInfo { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		swapchainCreateInfo.arraySize = viewCount;
		swapchainCreateInfo.format = Constants::colorFormat;
		swapchainCreateInfo.width = appState.SwapchainWidth;
		swapchainCreateInfo.height = appState.SwapchainHeight;
		swapchainCreateInfo.mipCount = 1;
		swapchainCreateInfo.faceCount = 1;
		swapchainCreateInfo.sampleCount = appState.SwapchainSampleCount;
		swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		XrSwapchain swapchain = VK_NULL_HANDLE;
		CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &swapchain));

		uint32_t imageCount;
		CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr));

		VkSubpassDescription subpass { };
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		VkAttachmentReference colorRef { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthRef { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		VkAttachmentReference resolveRef { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkAttachmentDescription attachments[3];

		VkRenderPassCreateInfo rpInfo { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		rpInfo.attachmentCount = 3;
		rpInfo.pAttachments = attachments;
		rpInfo.subpassCount = 1;
		rpInfo.pSubpasses = &subpass;

		VkRenderPassMultiviewCreateInfo multiviewCreateInfo { };
		const uint32_t viewMask = 3;
		multiviewCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
		multiviewCreateInfo.subpassCount = 1;
		multiviewCreateInfo.pViewMasks = &viewMask;
		multiviewCreateInfo.correlationMaskCount = 1;
		multiviewCreateInfo.pCorrelationMasks = &viewMask;
		rpInfo.pNext = &multiviewCreateInfo;

		attachments[0].format = Constants::colorFormat;
		attachments[0].samples = VK_SAMPLE_COUNT_4_BIT;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorRef;

		attachments[1].format = Constants::depthFormat;
		attachments[1].samples = VK_SAMPLE_COUNT_4_BIT;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		subpass.pDepthStencilAttachment = &depthRef;

		attachments[2].format = Constants::colorFormat;
		attachments[2].samples = (VkSampleCountFlagBits)appState.SwapchainSampleCount;
		attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		subpass.pResolveAttachments = &resolveRef;

		CHECK_VKCMD(vkCreateRenderPass(appState.Device, &rpInfo, nullptr, &appState.RenderPass));

		std::vector<XrSwapchainImageVulkan2KHR> swapchainVulkanImages(imageCount);

		std::vector<XrSwapchainImageBaseHeader*> swapchainImages(imageCount);
		for (uint32_t i = 0; i < imageCount; ++i)
		{
			swapchainVulkanImages[i] = { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR };
			swapchainImages[i] = reinterpret_cast<XrSwapchainImageBaseHeader*>(&swapchainVulkanImages[i]);
		}

		CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount, swapchainImages[0]));

		for (auto j = 0; j < imageCount; j++)
		{
			appState.PerImage.emplace_back();
			auto& perImage = appState.PerImage[appState.PerImage.size() - 1];
			CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &perImage.commandBuffer));
			CHECK_VKCMD(vkCreateFence(appState.Device, &fenceCreateInfo, nullptr, &perImage.fence));
		}

		XrEventDataBuffer eventDataBuffer { };

		auto sessionState = XR_SESSION_STATE_UNKNOWN;
		auto sessionRunning = false;

		while (app->destroyRequested == 0)
		{
			for (;;)
			{
				int events;
				struct android_poll_source* source;
				const int timeoutMilliseconds = ((!appState.Resumed && !sessionRunning && app->destroyRequested == 0) ? -1 : 0);

				if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void**) &source) < 0)
				{
					break;
				}

				if (source != nullptr)
				{
					source->process(app, source);
				}
			}

			auto exitRenderLoop = false;
			auto requestRestart = false;

			while (const XrEventDataBaseHeader* event = TryReadNextEvent(eventDataBuffer, instance))
			{
				switch (event->type)
				{
					case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
					{
						const auto& instanceLossPending = *reinterpret_cast<const XrEventDataInstanceLossPending*>(event);
						__android_log_print(ANDROID_LOG_WARN, "slipnfrag_native", "XrEventDataInstanceLossPending by %lld", instanceLossPending.lossTime);
						exitRenderLoop = true;
						requestRestart = true;
						return;
					}
					case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
					{
						auto sessionStateChangedEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(event);
						const XrSessionState oldState = sessionState;
						sessionState = sessionStateChangedEvent.state;

						__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "XrEventDataSessionStateChanged: state %s->%s session=%lld time=%lld", to_string(oldState), to_string(sessionState), sessionStateChangedEvent.session, sessionStateChangedEvent.time);

						if ((sessionStateChangedEvent.session != XR_NULL_HANDLE) && (sessionStateChangedEvent.session != appState.Session))
						{
							__android_log_print(ANDROID_LOG_ERROR, "slipnfrag_native", "XrEventDataSessionStateChanged for unknown session");
							return;
						}

						switch (sessionState)
						{
							case XR_SESSION_STATE_READY:
							{
								CHECK(appState.Session != XR_NULL_HANDLE);
								XrSessionBeginInfo sessionBeginInfo { XR_TYPE_SESSION_BEGIN_INFO };
								sessionBeginInfo.primaryViewConfigurationType = m_viewConfigType;
								CHECK_XRCMD(xrBeginSession(appState.Session, &sessionBeginInfo));
								sessionRunning = true;
								break;
							}
							case XR_SESSION_STATE_STOPPING:
							{
								CHECK(appState.Session != XR_NULL_HANDLE);
								sessionRunning = false;
								ANativeActivity_finish(app->activity);
								CHECK_XRCMD(xrEndSession(appState.Session))
								break;
							}
							case XR_SESSION_STATE_EXITING:
							{
								exitRenderLoop = true;
								requestRestart = false;
								break;
							}
							case XR_SESSION_STATE_LOSS_PENDING:
							{
								exitRenderLoop = true;
								requestRestart = true;
								break;
							}
							default:
								break;
						}
						break;
					}
					case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
						SwitchBoundInput(appState, appState.Play1Action, "Play 1");
						SwitchBoundInput(appState, appState.Play2Action, "Play 2");
						SwitchBoundInput(appState, appState.JumpAction, "Jump");
						SwitchBoundInput(appState, appState.SwimDownAction, "Swim down");
						SwitchBoundInput(appState, appState.RunAction, "Run");
						SwitchBoundInput(appState, appState.FireAction, "Fire");
						SwitchBoundInput(appState, appState.MoveXAction, "Move X");
						SwitchBoundInput(appState, appState.MoveYAction, "Move Y");
						SwitchBoundInput(appState, appState.SwitchWeaponAction, "Switch weapon");
						SwitchBoundInput(appState, appState.MenuAction, "Menu");
						SwitchBoundInput(appState, appState.EnterTriggerAction, "Enter (with triggers)");
						SwitchBoundInput(appState, appState.EnterNonTriggerAction, "Enter (without triggers)");
						SwitchBoundInput(appState, appState.EscapeYAction, "Escape (plus Y)");
						SwitchBoundInput(appState, appState.EscapeNonYAction, "Escape (minus Y)");
						SwitchBoundInput(appState, appState.QuitAction, "Quit");
						SwitchBoundInput(appState, appState.PoseAction, "Hand pose");
						SwitchBoundInput(appState, appState.LeftKeyPressAction, "Left key press");
						SwitchBoundInput(appState, appState.RightKeyPressAction, "Right key press");
						break;
					case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
					default:
						__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Ignoring event type %d", event->type);
						break;
				}
			}

			if (!sessionRunning)
			{
				continue;
			}

			auto keyPressHandled = appState.Keyboard.Handle(appState);
			Input::Handle(appState, keyPressHandled);

			XrActionStatePose poseState { XR_TYPE_ACTION_STATE_POSE };

			XrActionStateGetInfo getInfo { XR_TYPE_ACTION_STATE_GET_INFO };
			getInfo.action = appState.PoseAction;

			getInfo.subactionPath = appState.SubactionPaths[0];
			CHECK_XRCMD(xrGetActionStatePose(appState.Session, &getInfo, &poseState));
			appState.ActiveHands[0] = poseState.isActive;

			getInfo.subactionPath = appState.SubactionPaths[1];
			CHECK_XRCMD(xrGetActionStatePose(appState.Session, &getInfo, &poseState));
			appState.ActiveHands[1] = poseState.isActive;

			if (!appState.Scene.created)
			{
				appState.Scene.Create(appState, commandBufferAllocateInfo, setupCommandBuffer, commandBufferBeginInfo, setupSubmitInfo, app);
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
						sys_version = "OVR 1.0.9";
						const char* basedir = "/sdcard/android/data/com.heribertodelgado.slipnfrag_xr/files";
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
								Throw(Fmt("Sys_Error: %s", sys_errormessage.c_str()), nullptr, FILE_AND_LINE);
							}
						}
						//appState.LeftController.PreviousButtons = 0;
						//appState.RightController.PreviousButtons = 0;
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

			appState.Scene.textures.DeleteOld(appState);
			appState.Scene.lightmaps.DeleteOld(appState);
			appState.Scene.buffers.DeleteOld(appState);

			XrFrameWaitInfo frameWaitInfo { XR_TYPE_FRAME_WAIT_INFO };
			XrFrameState frameState { XR_TYPE_FRAME_STATE };
			CHECK_XRCMD(xrWaitFrame(appState.Session, &frameWaitInfo, &frameState));

			XrFrameBeginInfo frameBeginInfo { XR_TYPE_FRAME_BEGIN_INFO };
			CHECK_XRCMD(xrBeginFrame(appState.Session, &frameBeginInfo));

			std::vector<XrCompositionLayerBaseHeader*> layers;

			XrCompositionLayerProjection worldLayer { XR_TYPE_COMPOSITION_LAYER_PROJECTION };

			std::vector<XrCompositionLayerProjectionView> projectionLayerViews;

			XrCompositionLayerCylinderKHR screenLayer { XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR };

			XrCompositionLayerCylinderKHR keyboardLayer { XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR };

			if (frameState.shouldRender)
			{
				XrViewState viewState { XR_TYPE_VIEW_STATE };
				uint32_t viewCapacityInput = (uint32_t) m_views.size();
				uint32_t viewCountOutput;

				XrViewLocateInfo viewLocateInfo { XR_TYPE_VIEW_LOCATE_INFO };
				viewLocateInfo.viewConfigurationType = m_viewConfigType;
				viewLocateInfo.displayTime = frameState.predictedDisplayTime;
				viewLocateInfo.space = appSpace;

				res = xrLocateViews(appState.Session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, m_views.data());
				CHECK_XRRESULT(res, "xrLocateViews");
				if ((viewState.viewStateFlags & (XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT)) == (XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT))
				{
					CHECK(viewCountOutput == viewCapacityInput);
					CHECK(viewCountOutput == configViews.size());

					{
						std::lock_guard<std::mutex> lock(appState.RenderInputMutex);

						appState.LeftController.SpaceLocation.type = XR_TYPE_SPACE_LOCATION;
						appState.LeftController.PoseIsValid = false;
						res = xrLocateSpace(appState.HandSpaces[0], appSpace, frameState.predictedDisplayTime, &appState.LeftController.SpaceLocation);
						CHECK_XRRESULT(res, "xrLocateSpace(appState.HandSpaces[0], appSpace)");
						if (XR_UNQUALIFIED_SUCCESS(res)) 
						{
							if ((appState.LeftController.SpaceLocation.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) == (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
							{
								appState.LeftController.PoseIsValid = true;
							}
						} 
						else if (appState.ActiveHands[0]) 
						{
							__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Unable to locate left hand action space in app space: %d", res);
						}

						appState.RightController.SpaceLocation.type = XR_TYPE_SPACE_LOCATION;
						appState.RightController.PoseIsValid = false;
						res = xrLocateSpace(appState.HandSpaces[1], appSpace, frameState.predictedDisplayTime, &appState.RightController.SpaceLocation);
						CHECK_XRRESULT(res, "xrLocateSpace(appState.HandSpaces[1], appSpace)");
						if (XR_UNQUALIFIED_SUCCESS(res))
						{
							if ((appState.RightController.SpaceLocation.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) == (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
							{
								appState.RightController.PoseIsValid = true;
							}
						}
						else if (appState.ActiveHands[1])
						{
							__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Unable to locate right hand action space in app space: %d", res);
						}

						for (size_t i = 0; i < viewCountOutput; i++)
						{
							XrMatrix4x4f_CreateProjectionFov(&appState.Scene.projectionMatrices[i], GRAPHICS_VULKAN, m_views[i].fov, 0.05f, 100.0f);
							XrMatrix4x4f rotation;
							XrMatrix4x4f_CreateFromQuaternion(&rotation, &m_views[i].pose.orientation);
							XrMatrix4x4f transposedRotation;
							XrMatrix4x4f_Transpose(&transposedRotation, &rotation);
							XrMatrix4x4f translation;
							XrMatrix4x4f_CreateTranslation(&translation, -m_views[i].pose.position.x, -m_views[i].pose.position.y, -m_views[i].pose.position.z);
							XrMatrix4x4f_Multiply(&appState.Scene.viewMatrices[i], &transposedRotation, &translation);
						}
						
						XrQuaternionf& first = m_views[0].pose.orientation;
						XrQuaternionf cumulative = first;
						for (size_t i = 1; i < viewCountOutput; i++)
						{
							XrQuaternionf current = m_views[i].pose.orientation;
							if (first.x * current.x + first.y * current.y + first.z * current.z + first.w * current.w < 0)
							{
								current.x = -current.x;
								current.y = -current.y;
								current.z = -current.z;
								current.w = -current.w;
							}
							cumulative.x += current.x;
							cumulative.y += current.y;
							cumulative.z += current.z;
							cumulative.w += current.w;
						}
						auto x = cumulative.x / viewCountOutput;
						auto y = cumulative.y / viewCountOutput;
						auto z = cumulative.z / viewCountOutput;
						auto w = cumulative.w / viewCountOutput;
						auto lengthSquared = x * x + y * y + z * z + w * w;
						if (lengthSquared > 0)
						{
							auto length = sqrt(lengthSquared);
							x /= length;
							y /= length;
							z /= length;
							w /= length;
						}
						appState.Scene.orientation = { x, y, z, w };
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
						auto positionX = 0.0f;
						auto positionY = 0.0f;
						auto positionZ = 0.0f;
						for (auto& view : m_views)
						{
							positionX += view.pose.position.x;
							positionY += view.pose.position.y;
							positionZ += view.pose.position.z;
						}
						appState.PositionX = positionX / viewCountOutput;
						appState.PositionY = positionY / viewCountOutput;
						appState.PositionZ = positionZ / viewCountOutput;
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

						XrSpaceLocation spaceLocation { XR_TYPE_SPACE_LOCATION };
						res = xrLocateSpace(worldSpace, appSpace, frameState.predictedDisplayTime, &spaceLocation);
						
						appState.DistanceToFloor = spaceLocation.pose.position.y;
						appState.Scale = -spaceLocation.pose.position.y / playerHeight;
						appState.OriginX = -r_refdef.vieworg[0] * appState.Scale;
						appState.OriginY = -r_refdef.vieworg[2] * appState.Scale;
						appState.OriginZ = r_refdef.vieworg[1] * appState.Scale;
						
						if (appState.FOV == 0)
						{
							for (size_t i = 0; i < viewCountOutput; i++)
							{
								auto fov = (m_views[i].fov.angleRight - m_views[i].fov.angleLeft) * 180 / M_PI;
								appState.FOV += fov;
							}
							appState.FOV /= viewCountOutput;
						}
					}
					
					projectionLayerViews.resize(viewCountOutput);
					
					XrSwapchainImageAcquireInfo acquireInfo { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };

					uint32_t swapchainImageIndex;
					CHECK_XRCMD(xrAcquireSwapchainImage(swapchain, &acquireInfo, &swapchainImageIndex));

					XrSwapchainImageWaitInfo waitInfo { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
					waitInfo.timeout = XR_INFINITE_DURATION;
					CHECK_XRCMD(xrWaitSwapchainImage(swapchain, &waitInfo));

					for (auto i = 0; i < viewCountOutput; i++)
					{
						projectionLayerViews[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
						projectionLayerViews[i].pose = m_views[i].pose;
						projectionLayerViews[i].fov = m_views[i].fov;
						projectionLayerViews[i].subImage.swapchain = swapchain;
						projectionLayerViews[i].subImage.imageRect.extent.width = appState.SwapchainWidth;
						projectionLayerViews[i].subImage.imageRect.extent.height = appState.SwapchainHeight;
						projectionLayerViews[i].subImage.imageArrayIndex = i;
					}

					auto& perImage = appState.PerImage[swapchainImageIndex];
					if (perImage.submitted)
					{
						CHECK_VKCMD(vkWaitForFences(appState.Device, 1, &perImage.fence, VK_TRUE, 1ULL * 1000 * 1000 * 1000));
						CHECK_VKCMD(vkResetFences(appState.Device, 1, &perImage.fence));
						perImage.submitted = false;
					}

					CHECK_VKCMD(vkResetCommandBuffer(perImage.commandBuffer, 0));
					CHECK_VKCMD(vkBeginCommandBuffer(perImage.commandBuffer, &commandBufferBeginInfo));

					double clearR = 0;
					double clearG = 0;
					double clearB = 0;
					double clearA = 1;
					
					auto readClearColor = false;
					
					if (appState.Mode != AppWorldMode/* || appState.Scene.skybox != VK_NULL_HANDLE*/)
					{
						clearA = 0;
					}
					else
					{
						readClearColor = true;
					}
					
					//double elapsed = 0;
					{
						std::lock_guard<std::mutex> lock(appState.RenderMutex);
						//auto startTime = Sys_FloatTime();
						if (readClearColor && d_lists.clear_color >= 0)
						{
							auto color = d_8to24table[d_lists.clear_color];
							clearR = (color & 255) / 255.0f;
							clearG = (color >> 8 & 255) / 255.0f;
							clearB = (color >> 16 & 255) / 255.0f;
							clearA = (color >> 24) / 255.0f;
						}
						perImage.Reset(appState);
						appState.Scene.Initialize();
						auto stagingBufferSize = perImage.GetStagingBufferSize(appState);
						auto stagingBuffer = perImage.stagingBuffers.GetStorageBuffer(appState, stagingBufferSize);
						perImage.LoadStagingBuffer(appState, stagingBuffer);
						perImage.FillFromStagingBuffer(appState, stagingBuffer);
						perImage.LoadRemainingBuffers(appState);
						perImage.hostClearCount = host_clearcount;
						//auto endTime = Sys_FloatTime();
						//elapsed += (endTime - startTime);
					}
					
					VkClearValue clearValues[3] { };
					clearValues[0].color.float32[0] = clearR;
					clearValues[0].color.float32[1] = clearG;
					clearValues[0].color.float32[2] = clearB;
					clearValues[0].color.float32[3] = clearA;
					clearValues[1].depthStencil.depth = 1.0f;
					clearValues[1].depthStencil.stencil = 0;

					if (perImage.framebuffer == VK_NULL_HANDLE)
					{
						VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
						imageInfo.imageType = VK_IMAGE_TYPE_2D;
						imageInfo.extent.width = appState.SwapchainWidth;
						imageInfo.extent.height = appState.SwapchainHeight;
						imageInfo.extent.depth = 1;
						imageInfo.mipLevels = 1;
						imageInfo.arrayLayers = 2;
						imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
						imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						imageInfo.samples = VK_SAMPLE_COUNT_4_BIT;
						imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

						imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
						imageInfo.format = Constants::colorFormat;
						CHECK_VKCMD(vkCreateImage(appState.Device, &imageInfo, nullptr, &perImage.colorImage));

						imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
						imageInfo.format = Constants::depthFormat;
						CHECK_VKCMD(vkCreateImage(appState.Device, &imageInfo, nullptr, &perImage.depthImage));

						perImage.resolveImage = swapchainVulkanImages[swapchainImageIndex].image;

						VkMemoryRequirements memRequirements { };
						VkMemoryAllocateInfo memoryAllocateInfo { };

						vkGetImageMemoryRequirements(appState.Device, perImage.colorImage, &memRequirements);
						createMemoryAllocateInfo(appState, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo);
						CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &perImage.colorMemory));
						CHECK_VKCMD(vkBindImageMemory(appState.Device, perImage.colorImage, perImage.colorMemory, 0));
						
						vkGetImageMemoryRequirements(appState.Device, perImage.depthImage, &memRequirements);
						createMemoryAllocateInfo(appState, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo);
						CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &perImage.depthMemory));
						CHECK_VKCMD(vkBindImageMemory(appState.Device, perImage.depthImage, perImage.depthMemory, 0));

						VkImageView attachments[3];

						VkImageViewCreateInfo viewInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
						viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
						viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
						viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
						viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
						viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
						viewInfo.subresourceRange.levelCount = 1;
						viewInfo.subresourceRange.layerCount = 2;

						viewInfo.image = perImage.colorImage;
						viewInfo.format = Constants::colorFormat;
						viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						CHECK_VKCMD(vkCreateImageView(appState.Device, &viewInfo, nullptr, &perImage.colorView));
						attachments[0] = perImage.colorView;

						viewInfo.image = perImage.depthImage;
						viewInfo.format = Constants::depthFormat;
						viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
						CHECK_VKCMD(vkCreateImageView(appState.Device, &viewInfo, nullptr, &perImage.depthView));
						attachments[1] = perImage.depthView;

						viewInfo.image = perImage.resolveImage;
						viewInfo.format = Constants::colorFormat;
						viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						CHECK_VKCMD(vkCreateImageView(appState.Device, &viewInfo, nullptr, &perImage.resolveView));
						attachments[2] = perImage.resolveView;

						VkFramebufferCreateInfo framebufferInfo { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
						framebufferInfo.renderPass = appState.RenderPass;
						framebufferInfo.attachmentCount = 3;
						framebufferInfo.pAttachments = attachments;
						framebufferInfo.width = appState.SwapchainWidth;
						framebufferInfo.height = appState.SwapchainHeight;
						framebufferInfo.layers = 1;
						CHECK_VKCMD(vkCreateFramebuffer(appState.Device, &framebufferInfo, nullptr, &perImage.framebuffer));

						VkImageMemoryBarrier colorBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
						colorBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
						colorBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						colorBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						colorBarrier.image = perImage.colorImage;
						colorBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 2 };
						vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &colorBarrier);

						colorBarrier.image = perImage.resolveImage;
						vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &colorBarrier);

						VkImageMemoryBarrier depthBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
						depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
						depthBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
						depthBarrier.image = perImage.depthImage;
						depthBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 2 };
						vkCmdPipelineBarrier(perImage.commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &depthBarrier);
					}

					VkRenderPassBeginInfo renderPassBeginInfo { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
					renderPassBeginInfo.clearValueCount = 3;
					renderPassBeginInfo.pClearValues = clearValues;
					
					renderPassBeginInfo.renderPass = appState.RenderPass;
					renderPassBeginInfo.framebuffer = perImage.framebuffer;
					renderPassBeginInfo.renderArea.offset = {0, 0};
					renderPassBeginInfo.renderArea.extent.width = appState.SwapchainWidth;
					renderPassBeginInfo.renderArea.extent.height = appState.SwapchainHeight;
					vkCmdBeginRenderPass(perImage.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

					VkRect2D screenRect { };
					screenRect.extent.width = appState.SwapchainWidth;
					screenRect.extent.height = appState.SwapchainHeight;
					
					VkViewport viewport;
					viewport.x = (float)screenRect.offset.x;
					viewport.y = (float)screenRect.offset.y;
					viewport.width = (float)screenRect.extent.width;
					viewport.height = (float)screenRect.extent.height;
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;
					vkCmdSetViewport(perImage.commandBuffer, 0, 1, &viewport);
					vkCmdSetScissor(perImage.commandBuffer, 0, 1, &screenRect);
					perImage.Render(appState);

					vkCmdEndRenderPass(perImage.commandBuffer);

					CHECK_VKCMD(vkEndCommandBuffer(perImage.commandBuffer));
					VkSubmitInfo submitInfo { };
					submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = &perImage.commandBuffer;
					CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &submitInfo, perImage.fence));

					perImage.submitted = true;
					//auto endTime = Sys_FloatTime();
					//elapsed += (endTime - startTime);
					//__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "AppState::RenderScene(): %.6f s.", elapsed);

					XrSwapchainImageReleaseInfo releaseInfo { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
					CHECK_XRCMD(xrReleaseSwapchainImage(swapchain, &releaseInfo));

					worldLayer.space = appSpace;
					worldLayer.viewCount = (uint32_t) projectionLayerViews.size();
					worldLayer.views = projectionLayerViews.data();

					if (appState.Mode != AppWorldMode)
					{
						worldLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
					}
					
					layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&worldLayer));

					if (appState.Mode == AppScreenMode || appState.Mode == AppWorldMode)
					{
						if (appState.Mode == AppScreenMode)
						{
							std::lock_guard<std::mutex> lock(appState.RenderMutex);
							auto consoleIndex = 0;
							auto consoleIndexCache = 0;
							auto screenIndex = 0;
							auto target = appState.Screen.Data.data();
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
											*target++ = d_8to24table[vid_buffer[screenIndex]];
											screenIndex++;
											x++;
										} while ((x % Constants::screenToConsoleMultiplier) != 0);
									}
									else
									{
										auto converted = d_8to24table[entry];
										do
										{
											*target++ = converted;
											screenIndex++;
											x++;
										} while ((x % Constants::screenToConsoleMultiplier) != 0);
									}
									consoleIndex++;
								}
								y++;
								if ((y % Constants::screenToConsoleMultiplier) == 0)
								{
									consoleIndexCache = consoleIndex;
								}
								else
								{
									consoleIndex = consoleIndexCache;
								}
							}
						}
						else
						{
							std::lock_guard<std::mutex> lock(appState.RenderMutex);
							auto source = con_buffer.data();
							auto previousSource = source;
							auto targetIndex = 0;
							auto y = 0;
							auto limit = appState.ScreenHeight - (SBAR_HEIGHT + 24) * Constants::screenToConsoleMultiplier;
							while (y < limit)
							{
								auto x = 0;
								while (x < appState.ScreenWidth)
								{
									auto entry = *source++;
									if (entry == 255)
									{
										do
										{
											appState.Screen.Data[targetIndex] = 0;
											targetIndex++;
											x++;
										} while ((x % Constants::screenToConsoleMultiplier) != 0);
									}
									else
									{
										auto converted = d_8to24table[entry];
										do
										{
											appState.Screen.Data[targetIndex] = converted;
											targetIndex++;
											x++;
										} while ((x % Constants::screenToConsoleMultiplier) != 0);
									}
								}
								y++;
								if ((y % Constants::screenToConsoleMultiplier) == 0)
								{
									previousSource = source;
								}
								else
								{
									source = previousSource;
								}
							}
							limit = appState.ScreenHeight - SBAR_HEIGHT - 24;
							std::fill(appState.Screen.Data.begin() + y * appState.ScreenWidth, appState.Screen.Data.begin() + limit * appState.ScreenWidth, 0);
							targetIndex += (limit - y) * appState.ScreenWidth;
							y = limit;
							limit = appState.ScreenWidth / Constants::screenToConsoleMultiplier;
							while (y < appState.ScreenHeight)
							{
								auto x = 0;
								while (x < limit)
								{
									appState.Screen.Data[targetIndex] =  d_8to24table[*source++];
									targetIndex++;
									x++;
								}
								while (x < appState.ScreenWidth)
								{
									appState.Screen.Data[targetIndex] = 0;
									targetIndex++;
									x++;
								}
								y++;
							}
						}
						{
							std::lock_guard<std::mutex> lock(DirectRect::DirectRectMutex);
							for (auto& directRect : DirectRect::directRects)
							{
								auto width = directRect.width * Constants::screenToConsoleMultiplier;
								auto height = directRect.height * Constants::screenToConsoleMultiplier;
								auto directRectIndex = 0;
								auto directRectIndexCache = 0;
								auto targetIndex = (directRect.y * appState.ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
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
										} while ((x % Constants::screenToConsoleMultiplier) != 0);
										directRectIndex++;
									}
									y++;
									if ((y % Constants::screenToConsoleMultiplier) == 0)
									{
										directRectIndexCache = directRectIndex;
									}
									else
									{
										directRectIndex = directRectIndexCache;
									}
									targetIndex += appState.ScreenWidth - width;
								}
							}
						}
					}
					else if (appState.Mode == AppNoGameDataMode)
					{
						static auto noGameDataLoaded = false;
						if (!noGameDataLoaded)
						{
							memcpy(appState.Screen.Data.data(), appState.NoGameDataData.data(), appState.NoGameDataData.size() * sizeof(uint32_t));
							noGameDataLoaded = true;
						}
					}
					
					memcpy(appState.Screen.StagingBuffer.mapped, appState.Screen.Data.data(), appState.Screen.StagingBuffer.size);

					CHECK_XRCMD(xrAcquireSwapchainImage(appState.Screen.Swapchain, &acquireInfo, &swapchainImageIndex));

					CHECK_XRCMD(xrWaitSwapchainImage(appState.Screen.Swapchain, &waitInfo));

					CHECK_VKCMD(vkResetCommandBuffer(appState.Screen.CommandBuffers[swapchainImageIndex], 0));
					CHECK_VKCMD(vkBeginCommandBuffer(appState.Screen.CommandBuffers[swapchainImageIndex], &commandBufferBeginInfo));

					VkImageMemoryBarrier imageMemoryBarrier { };
					imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
					imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
					imageMemoryBarrier.image = appState.Screen.VulkanImages[swapchainImageIndex].image;
					imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageMemoryBarrier.subresourceRange.levelCount = 1;
					imageMemoryBarrier.subresourceRange.layerCount = 1;
					vkCmdPipelineBarrier(appState.Screen.CommandBuffers[swapchainImageIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

					VkBufferImageCopy region { };
					region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.imageSubresource.layerCount = 1;
					region.imageExtent.width = appState.ScreenWidth;
					region.imageExtent.height = appState.ScreenHeight;
					region.imageExtent.depth = 1;
					vkCmdCopyBufferToImage(appState.Screen.CommandBuffers[swapchainImageIndex], appState.Screen.StagingBuffer.buffer, appState.Screen.VulkanImages[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

					CHECK_VKCMD(vkEndCommandBuffer(appState.Screen.CommandBuffers[swapchainImageIndex]));

					CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &appState.Screen.SubmitInfo[swapchainImageIndex], VK_NULL_HANDLE));

					CHECK_XRCMD(xrReleaseSwapchainImage(appState.Screen.Swapchain, &releaseInfo));

					screenLayer.radius = CylinderProjection::radius;
					screenLayer.aspectRatio = (float)appState.ScreenWidth / appState.ScreenHeight;
					screenLayer.centralAngle = CylinderProjection::horizontalAngle;
					screenLayer.subImage.swapchain = appState.Screen.Swapchain;
					screenLayer.subImage.imageRect.extent.width = appState.ScreenWidth;
					screenLayer.subImage.imageRect.extent.height = appState.ScreenHeight;
					screenLayer.space = appSpace;

					if (appState.Mode == AppWorldMode)
					{
						XrSpaceLocation screenLocation { XR_TYPE_SPACE_LOCATION };
						res = xrLocateSpace(screenSpace, appSpace, frameState.predictedDisplayTime, &screenLocation);
						CHECK_XRRESULT(res, "xrLocateSpace(screenSpace, appSpace)");
						if (XR_UNQUALIFIED_SUCCESS(res) && (screenLocation.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) == (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
						{
							screenLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
							screenLayer.pose = screenLocation.pose;

							layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&screenLayer));
						}
					}
					else
					{
						screenLayer.layerFlags = 0;
						screenLayer.pose = { };
						screenLayer.pose.orientation.w = 1;

						layers.insert(layers.begin(), reinterpret_cast<XrCompositionLayerBaseHeader*>(&screenLayer));
					}

					if (appState.Keyboard.Draw(appState))
					{
						unsigned char* source = appState.Keyboard.buffer.data();
						unsigned char* previousSource = source;
						uint32_t* target = appState.Keyboard.Screen.Data.data();
						auto limit = appState.ScreenHeight / 2;
						auto y = 0;
						while (y < limit)
						{
							auto x = 0;
							while (x < appState.ScreenWidth)
							{
								auto entry = *source++;
								unsigned int converted;
								if (entry == 255)
								{
									converted = 0;
								}
								else
								{
									converted = d_8to24table[entry];
								}
								do
								{
									*target++ = converted;
									x++;
								} while ((x % Constants::screenToConsoleMultiplier) != 0);
							}
							y++;
							if ((y % Constants::screenToConsoleMultiplier) == 0)
							{
								previousSource = source;
							}
							else
							{
								source = previousSource;
							}
						}

						memcpy(appState.Keyboard.Screen.StagingBuffer.mapped, appState.Keyboard.Screen.Data.data(), appState.Keyboard.Screen.Data.size() * sizeof(uint32_t));

						CHECK_XRCMD(xrAcquireSwapchainImage(appState.Keyboard.Screen.Swapchain, &acquireInfo, &swapchainImageIndex));

						CHECK_XRCMD(xrWaitSwapchainImage(appState.Keyboard.Screen.Swapchain, &waitInfo));

						CHECK_VKCMD(vkResetCommandBuffer(appState.Keyboard.Screen.CommandBuffers[swapchainImageIndex], 0));
						CHECK_VKCMD(vkBeginCommandBuffer(appState.Keyboard.Screen.CommandBuffers[swapchainImageIndex], &commandBufferBeginInfo));

						VkImageMemoryBarrier imageMemoryBarrier { };
						imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						imageMemoryBarrier.image = appState.Keyboard.Screen.VulkanImages[swapchainImageIndex].image;
						imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						imageMemoryBarrier.subresourceRange.levelCount = 1;
						imageMemoryBarrier.subresourceRange.layerCount = 1;
						vkCmdPipelineBarrier(appState.Keyboard.Screen.CommandBuffers[swapchainImageIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

						VkBufferImageCopy region { };
						region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						region.imageSubresource.layerCount = 1;
						region.imageExtent.width = appState.ScreenWidth;
						region.imageExtent.height = limit;
						region.imageExtent.depth = 1;
						vkCmdCopyBufferToImage(appState.Keyboard.Screen.CommandBuffers[swapchainImageIndex], appState.Keyboard.Screen.StagingBuffer.buffer, appState.Keyboard.Screen.VulkanImages[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

						CHECK_VKCMD(vkEndCommandBuffer(appState.Keyboard.Screen.CommandBuffers[swapchainImageIndex]));

						CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &appState.Keyboard.Screen.SubmitInfo[swapchainImageIndex], VK_NULL_HANDLE));

						CHECK_XRCMD(xrReleaseSwapchainImage(appState.Keyboard.Screen.Swapchain, &releaseInfo));

						keyboardLayer.radius = CylinderProjection::radius;
						keyboardLayer.aspectRatio = (float)appState.ScreenWidth / limit;
						keyboardLayer.centralAngle = CylinderProjection::horizontalAngle;
						keyboardLayer.subImage.swapchain = appState.Keyboard.Screen.Swapchain;
						keyboardLayer.subImage.imageRect.extent.width = appState.ScreenWidth;
						keyboardLayer.subImage.imageRect.extent.height = limit;
						keyboardLayer.space = appSpace;

						if (appState.Mode == AppWorldMode)
						{
							XrSpaceLocation keyboardLocation { XR_TYPE_SPACE_LOCATION };
							if (key_dest == key_console)
							{
								res = xrLocateSpace(consoleKeyboardSpace, appSpace, frameState.predictedDisplayTime, &keyboardLocation);
								CHECK_XRRESULT(res, "xrLocateSpace(consoleKeyboardSpace, appSpace)");
							}
							else
							{
								res = xrLocateSpace(keyboardSpace, appSpace, frameState.predictedDisplayTime, &keyboardLocation);
								CHECK_XRRESULT(res, "xrLocateSpace(keyboardSpace, appSpace)");
							}
							if (XR_UNQUALIFIED_SUCCESS(res) && (keyboardLocation.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) == (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
							{
								keyboardLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
								keyboardLayer.pose = keyboardLocation.pose;

								layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&keyboardLayer));
							}
						}
						else
						{
							keyboardLayer.layerFlags = 0;
							keyboardLayer.pose = { };
							keyboardLayer.pose.position.y = -CylinderProjection::screenLowerLimit - CylinderProjection::keyboardLowerLimit;
							keyboardLayer.pose.orientation.w = 1;

							layers.insert(layers.begin() + 1, reinterpret_cast<XrCompositionLayerBaseHeader*>(&keyboardLayer));
						}
					}
				}
			}

			XrFrameEndInfo frameEndInfo { XR_TYPE_FRAME_END_INFO };
			frameEndInfo.displayTime = frameState.predictedDisplayTime;
			frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
			frameEndInfo.layerCount = (uint32_t)layers.size();
			frameEndInfo.layers = layers.data();
			
			CHECK_XRCMD(xrEndFrame(appState.Session, &frameEndInfo));
		}

		if (appState.EngineThread.joinable())
		{
			appState.EngineThreadStopped = true;
			appState.EngineThread.join();
		}

		for (auto perImage : appState.PerImage)
		{
			if (perImage.framebuffer != VK_NULL_HANDLE)
			{
				vkDestroyFramebuffer(appState.Device, perImage.framebuffer, nullptr);
			}
			if (perImage.resolveView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(appState.Device, perImage.resolveView, nullptr);
			}
			if (perImage.depthView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(appState.Device, perImage.depthView, nullptr);
			}
			if (perImage.colorView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(appState.Device, perImage.colorView, nullptr);
			}
			if (perImage.depthImage != VK_NULL_HANDLE)
			{
				vkDestroyImage(appState.Device, perImage.depthImage, nullptr);
			}
			if (perImage.depthMemory != VK_NULL_HANDLE)
			{
				vkFreeMemory(appState.Device, perImage.depthMemory, nullptr);
			}
			if (perImage.colorImage != VK_NULL_HANDLE)
			{
				vkDestroyImage(appState.Device, perImage.colorImage, nullptr);
			}
			if (perImage.colorMemory != VK_NULL_HANDLE)
			{
				vkFreeMemory(appState.Device, perImage.colorMemory, nullptr);
			}
		}

		if (appState.Device != VK_NULL_HANDLE)
		{
			for (auto& perImage : appState.PerImage)
			{
				vkFreeCommandBuffers(appState.Device, appState.CommandPool, 1, &perImage.commandBuffer);
				vkDestroyFence(appState.Device, perImage.fence, nullptr);
				perImage.controllerResources.Delete(appState);
				perImage.floorResources.Delete(appState);
				perImage.skyResources.Delete(appState);
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
				perImage.stagingBuffers.Delete(appState);
				perImage.cachedColors.Delete(appState);
				perImage.cachedIndices32.Delete(appState);
				perImage.cachedIndices16.Delete(appState);
				perImage.cachedAttributes.Delete(appState);
				perImage.cachedVertices.Delete(appState);
			}
	
			if (appState.RenderPass != VK_NULL_HANDLE)
			{
				vkDestroyRenderPass(appState.Device, appState.RenderPass, nullptr);
			}
	
			CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));
			
			vkDestroySampler(appState.Device, appState.Scene.lightmapSampler, nullptr);
			for (auto i = 0; i < appState.Scene.samplers.size(); i++)
			{
				vkDestroySampler(appState.Device, appState.Scene.samplers[i], nullptr);
			}
			appState.Scene.controllerTexture.Delete(appState);
			appState.Scene.floorTexture.Delete(appState);
			appState.Scene.textures.Delete(appState);
			for (auto& entry : appState.Scene.allocations)
			{
				for (auto& list : entry.second.allocations)
				{
					vkFreeMemory(appState.Device, list.memory, nullptr);
				}
			}
			appState.Scene.buffers.Delete(appState);
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
			vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.doubleImageLayout, nullptr);
			vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.singleImageLayout, nullptr);
			vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.bufferAndTwoImagesLayout, nullptr);
			vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.bufferAndImageLayout, nullptr);
			vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.singleBufferLayout, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.consoleFragment, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.consoleVertex, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.floorFragment, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.floorVertex, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.skyFragment, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.skyVertex, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.coloredFragment, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.coloredVertex, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.viewmodelFragment, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.viewmodelVertex, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.aliasFragment, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.aliasVertex, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.turbulentFragment, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.turbulentVertex, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.spriteFragment, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.spriteVertex, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.fenceFragment, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.surfaceFragment, nullptr);
			vkDestroyShaderModule(appState.Device, appState.Scene.surfaceVertex, nullptr);
			appState.Scene.matrices.Delete(appState);

			appState.Keyboard.Screen.StagingBuffer.Delete(appState);

			for (size_t i = 0; i < appState.Keyboard.Screen.Images.size(); i++)
			{
				if (appState.Keyboard.Screen.CommandBuffers[i] != VK_NULL_HANDLE)
				{
					vkFreeCommandBuffers(appState.Device, appState.CommandPool, 1, &appState.Keyboard.Screen.CommandBuffers[i]);
				}
			}

			appState.Screen.StagingBuffer.Delete(appState);

			for (size_t i = 0; i < appState.Screen.Images.size(); i++)
			{
				if (appState.Screen.CommandBuffers[i] != VK_NULL_HANDLE)
				{
					vkFreeCommandBuffers(appState.Device, appState.CommandPool, 1, &appState.Screen.CommandBuffers[i]);
				}
			}
		}

		if (appState.ActionSet != XR_NULL_HANDLE)
		{
			xrDestroySpace(appState.HandSpaces[0]);
			xrDestroySpace(appState.HandSpaces[1]);
			xrDestroyActionSet(appState.ActionSet);
		}

		if (appState.Keyboard.Screen.Swapchain != XR_NULL_HANDLE)
		{
			xrDestroySwapchain(appState.Keyboard.Screen.Swapchain);
		}

		if (appState.Screen.Swapchain != XR_NULL_HANDLE)
		{
			xrDestroySwapchain(appState.Screen.Swapchain);
		}

		if (swapchain != XR_NULL_HANDLE)
		{
			xrDestroySwapchain(swapchain);
		}

		if (worldSpace != XR_NULL_HANDLE)
		{
			xrDestroySpace(worldSpace);
		}

		if (appSpace != XR_NULL_HANDLE)
		{
			xrDestroySpace(appSpace);
		}

		if (appState.Session != XR_NULL_HANDLE)
		{
			xrDestroySession(appState.Session);
		}

		if (instance != XR_NULL_HANDLE)
		{
			xrDestroyInstance(instance);
		}
	}
	catch (const std::exception& ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, "slipnfrag_native", "%s", ex.what());
	}
	catch (...)
	{
		__android_log_print(ANDROID_LOG_ERROR, "slipnfrag_native", "Unknown Error");
	}

	app->activity->vm->DetachCurrentThread();
}
