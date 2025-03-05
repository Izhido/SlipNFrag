#include <jni.h>
#include <string>
#include <locale>
#include <android/log.h>
#include <array>
#include <vector>
#include <cmath>
#include "AppState.h"
#include <list>
#include <map>
#include "sys_oxr.h"
#include "r_local.h"
#include "EngineThread.h"
#include "in_oxr.h"
#include <common/xr_linear.h>
#include "Utils.h"
#include "Input.h"
#include "MemoryAllocateInfo.h"
#include "Constants.h"
#include "CylinderProjection.h"
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <android_native_app_glue.h>
#include "Locks.h"

extern int sound_started;

extern byte cdaudio_playTrack;
extern qboolean cdaudio_playing;

void CDAudio_DisposeBuffers();

qboolean CDAudio_Start(byte track);

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
			auto const eventsLost = reinterpret_cast<const XrEventDataEventsLost*>(baseHeader);
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

static VkBool32 DebugMessengerCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
{
	std::string severityName;
	int priority = ANDROID_LOG_UNKNOWN;

	if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0u)
	{
		severityName += "ERROR:";
		priority = ANDROID_LOG_ERROR;
	}
	if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0u)
	{
        severityName += "WARN:";
		priority = ANDROID_LOG_WARN;
	}
	if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0u)
	{
        severityName += "INFO:";
		priority = ANDROID_LOG_INFO;
	}
    if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0u)
    {
        severityName += "VERB:";
        priority = ANDROID_LOG_VERBOSE;
    }

    std::string typeName;
	if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT ) != 0u)
	{
		typeName += "GENERAL ";
	}
	if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0u)
	{
        typeName += "VALIDATION ";
	}
    if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0u)
    {
        typeName += "PERFORMANCE ";
    }
    if ((messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT) != 0u)
    {
        typeName += "DEVICE ADDRESS BINDING ";
    }

	__android_log_print(priority, "slipnfrag_native", "[%s%s] %s", typeName.c_str(), severityName.c_str(), pCallbackData->pMessage);
	
	return VK_FALSE;
}

static void AppHandleCommand(struct android_app* app, int32_t cmd)
{
	auto appState = (AppState*)app->userData;
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
			delta = GetTime() - appState->PausedTime;
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
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "onPause()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_PAUSE");
			appState->PausedTime = GetTime();
			appState->Resumed = false;
			break;

		case APP_CMD_STOP:
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "onStop()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_STOP");
			break;

		case APP_CMD_DESTROY:
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "onDestroy()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_DESTROY");
			break;

		case APP_CMD_INIT_WINDOW:
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "surfaceCreated()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_INIT_WINDOW");
			break;

		case APP_CMD_TERM_WINDOW:
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "surfaceDestroyed()");
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "    APP_CMD_TERM_WINDOW");
			break;

		default:
			__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Unrecognized app command %i", cmd);
			break;
	}
}

AppState appState { -1, -1, -1 };

void android_main(struct android_app* app)
{
	JNIEnv* Env;
	app->activity->vm->AttachCurrentThread(&Env, nullptr);

	prctl(PR_SET_NAME, (long)"android_main", 0, 0, 0);

	try
	{
		appState.RenderThreadId = gettid();

		app->userData = &appState;
		app->onAppCmd = AppHandleCommand;

		XrGraphicsBindingVulkan2KHR graphicsBinding { XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR };

		VkInstance vulkanInstance = VK_NULL_HANDLE;
		VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;
		uint32_t queueFamilyIndex = 0;

		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
        VkDebugUtilsMessengerEXT vulkanDebugMessenger = VK_NULL_HANDLE;

		PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR = nullptr;
		XrResult res = xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*) (&xrInitializeLoaderKHR));
		CHECK_XRRESULT(res, "xrGetInstanceProcAddr");

		XrLoaderInitInfoAndroidKHR loaderInitInfoAndroid { };
		loaderInitInfoAndroid.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
		loaderInitInfoAndroid.applicationVM = app->activity->vm;
		loaderInitInfoAndroid.applicationContext = app->activity->clazz;
		CHECK_XRCMD(xrInitializeLoaderKHR((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfoAndroid));

		std::vector<std::string> xrInstanceExtensionSources
		{
			XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
			XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME,
			XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME,
			XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME,
			XR_KHR_COMPOSITION_LAYER_CUBE_EXTENSION_NAME,
			XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME,
			XR_FB_COLOR_SPACE_EXTENSION_NAME
		};

		auto performanceSettingsEnabled = false;

		uint32_t instanceExtensionCount;
		CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(nullptr, 0, &instanceExtensionCount, nullptr));

		std::vector<XrExtensionProperties> extensions(instanceExtensionCount);
		for (XrExtensionProperties& extension : extensions)
		{
			extension.type = XR_TYPE_EXTENSION_PROPERTIES;
		}

		CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(nullptr, (uint32_t)extensions.size(), &instanceExtensionCount, extensions.data()));

		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Available OpenXR extensions: (%d)", instanceExtensionCount);

		for (const XrExtensionProperties& extension : extensions)
		{
			if (strncmp(extension.extensionName, XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME, sizeof(extension.extensionName)) == 0)
			{
				performanceSettingsEnabled = true;
				xrInstanceExtensionSources.emplace_back(extension.extensionName);
			}

			__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "  Name=%s SpecVersion=%d", extension.extensionName, extension.extensionVersion);
		}

		for (auto& extensionName : xrInstanceExtensionSources)
		{
			auto found = false;
			for (const XrExtensionProperties& extension : extensions)
			{
				if (extensionName == extension.extensionName)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				THROW(Fmt("Device does not support %s extension", extensionName.c_str()));
			}
		}

		uint32_t xrLayerCount;
		CHECK_XRCMD(xrEnumerateApiLayerProperties(0, &xrLayerCount, nullptr));

		std::vector<XrApiLayerProperties> xrLayers(xrLayerCount);
		for (XrApiLayerProperties& layer : xrLayers)
		{
			layer.type = XR_TYPE_API_LAYER_PROPERTIES;
		}

		CHECK_XRCMD(xrEnumerateApiLayerProperties((uint32_t) xrLayers.size(), &xrLayerCount, xrLayers.data()));

		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Available Layers: (%d)", xrLayerCount);
		for (const XrApiLayerProperties& layer : xrLayers)
		{
			__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "  Name=%s SpecVersion=%s LayerVersion=%d Description=%s", layer.layerName, GetXrVersionString(layer.specVersion).c_str(), layer.layerVersion, layer.description);

			CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layer.layerName, 0, &instanceExtensionCount, nullptr));

			CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layer.layerName, (uint32_t)extensions.size(), &instanceExtensionCount, extensions.data()));

			const std::string indentStr(4, ' ');
			__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "%sAvailable OpenXR extensions for layer %s: (%d)", indentStr.c_str(), layer.layerName, instanceExtensionCount);

			for (const XrExtensionProperties& extension : extensions)
			{
				__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "%s  Name=%s SpecVersion=%d", indentStr.c_str(), extension.extensionName, extension.extensionVersion);
			}
		}

		std::vector<const char*> xrInstanceExtensions;
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

		XrInstance instance = XR_NULL_HANDLE;

		XrInstanceCreateInfoAndroidKHR instanceCreateInfoAndroid { XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR };
		instanceCreateInfoAndroid.applicationVM = app->activity->vm;
		instanceCreateInfoAndroid.applicationActivity = app->activity->clazz;

		XrInstanceCreateInfo createInfo { XR_TYPE_INSTANCE_CREATE_INFO };
		createInfo.next = (XrBaseInStructure*)&instanceCreateInfoAndroid;
		createInfo.enabledExtensionCount = (uint32_t)xrInstanceExtensions.size();
		createInfo.enabledExtensionNames = xrInstanceExtensions.data();
		createInfo.enabledApiLayerCount = (uint32_t)xrInstanceApiLayers.size();
		createInfo.enabledApiLayerNames = xrInstanceApiLayers.data();

		strcpy(createInfo.applicationInfo.applicationName, "SlipNFrag-XR");
		createInfo.applicationInfo.apiVersion = USE_OPENXR_VERSION;

		CHECK_XRCMD(xrCreateInstance(&createInfo, &instance));

		CHECK(instance != XR_NULL_HANDLE);

		XrInstanceProperties instanceProperties { XR_TYPE_INSTANCE_PROPERTIES };
		CHECK_XRCMD(xrGetInstanceProperties(instance, &instanceProperties));

		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Instance RuntimeName=%s RuntimeVersion=%s", instanceProperties.runtimeName, GetXrVersionString(instanceProperties.runtimeVersion).c_str());

		XrSystemId systemId = XR_NULL_SYSTEM_ID;
		XrSystemGetInfo systemInfo { XR_TYPE_SYSTEM_GET_INFO };
		systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		CHECK_XRCMD(xrGetSystem(instance, &systemInfo, &systemId));

		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Using system %lu for form factor %s", systemId, to_string(systemInfo.formFactor));

		CHECK(systemId != XR_NULL_SYSTEM_ID);

		uint32_t viewConfigTypeCount;
		CHECK_XRCMD(xrEnumerateViewConfigurations(instance, systemId, 0, &viewConfigTypeCount, nullptr));
		std::vector<XrViewConfigurationType> viewConfigTypes(viewConfigTypeCount);
		CHECK_XRCMD(xrEnumerateViewConfigurations(instance, systemId, viewConfigTypeCount, &viewConfigTypeCount, viewConfigTypes.data()));
		CHECK((uint32_t) viewConfigTypes.size() == viewConfigTypeCount);

		auto viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Available View Configuration Types: (%d)", viewConfigTypeCount);
		for (XrViewConfigurationType viewConfigType : viewConfigTypes)
		{
			__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "  View Configuration Type: %s %s", to_string(viewConfigType), viewConfigType == viewConfigurationType ? "(Selected)" : "");

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

		PFN_xrGetVulkanGraphicsRequirements2KHR xrGetVulkanGraphicsRequirements2KHR = nullptr;
		CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsRequirements2KHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrGetVulkanGraphicsRequirements2KHR)));

		CHECK_XRCMD(xrGetVulkanGraphicsRequirements2KHR(instance, systemId, &graphicsRequirements));

		std::vector<const char*> instanceLayerNames;
		
#if !defined(NDEBUG)
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		const char* vulkanValidationLayerName = "VK_LAYER_KHRONOS_validation";

		std::vector<const char*> validationLayerNames
		{
			vulkanValidationLayerName,
			"VK_LAYER_ADRENO_debug"
		};

		auto validationLayersFound = false;
		auto vulkanValidationLayerFound = false;
		for (auto& validationLayerName : validationLayerNames)
		{
			for (const auto& layerProperties : availableLayers)
			{
				if (0 == strcmp(validationLayerName, layerProperties.layerName))
				{
					instanceLayerNames.push_back(validationLayerName);
					validationLayersFound = true;
					if (0 == strcmp(validationLayerName, vulkanValidationLayerName))
					{
						vulkanValidationLayerFound = true;
					}
				}
			}
		}
		if (!validationLayersFound)
		{
			__android_log_print(ANDROID_LOG_WARN, "slipnfrag_native", "No validation layers found in the system, skipping");
		}
#endif

		uint32_t vulkanSwapchainSampleCount;
		{
			std::vector<const char*> vulkanExtensions;
			vulkanExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			VkApplicationInfo appInfo { VK_STRUCTURE_TYPE_APPLICATION_INFO };
			appInfo.pApplicationName = "slipnfrag_xr";
			appInfo.applicationVersion = 1;
			appInfo.pEngineName = "slipnfrag_xr";
			appInfo.engineVersion = 1;
			appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 1, 0);

			VkInstanceCreateInfo instInfo { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
			instInfo.pApplicationInfo = &appInfo;
			instInfo.enabledLayerCount = (uint32_t)instanceLayerNames.size();
			instInfo.ppEnabledLayerNames = (instanceLayerNames.empty() ? nullptr : instanceLayerNames.data());
			instInfo.enabledExtensionCount = (uint32_t)vulkanExtensions.size();
			instInfo.ppEnabledExtensionNames = (vulkanExtensions.empty() ? nullptr : vulkanExtensions.data());

#if !defined(NDEBUG)
			VkBool32 vvlValidateSync = VK_TRUE;
			VkBool32 vvlThreadSafety = VK_TRUE;
			VkBool32 vvlBestPractices = VK_TRUE;
			VkBool32 vvlBestPracticesARM = VK_TRUE;

			VkLayerSettingEXT settings[] =
			{
				{ vulkanValidationLayerName, "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &vvlValidateSync },
				{ vulkanValidationLayerName, "thread_safety", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &vvlThreadSafety },
				{ vulkanValidationLayerName, "validate_best_practices", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &vvlBestPractices },
				{ vulkanValidationLayerName, "validate_best_practices_arm", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &vvlBestPracticesARM }
			};

			VkLayerSettingsCreateInfoEXT layerSettingsCreate { VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT };
			layerSettingsCreate.settingCount = std::size(settings);
			layerSettingsCreate.pSettings = settings;

			if (vulkanValidationLayerFound)
			{
				instInfo.pNext = &layerSettingsCreate;
			}
#endif

			XrVulkanInstanceCreateInfoKHR vulkanCreateInfo { XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR };
			vulkanCreateInfo.systemId = systemId;
			vulkanCreateInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
			vulkanCreateInfo.vulkanCreateInfo = &instInfo;
			vulkanCreateInfo.vulkanAllocator = nullptr;

			PFN_xrCreateVulkanInstanceKHR xrCreateVulkanInstanceKHR = nullptr;
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrCreateVulkanInstanceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrCreateVulkanInstanceKHR)));

			VkResult errCreateVulkanInstance;
			CHECK_XRCMD(xrCreateVulkanInstanceKHR(instance, &vulkanCreateInfo, &vulkanInstance, &errCreateVulkanInstance));
			CHECK_VKCMD(errCreateVulkanInstance);

            vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkCreateDebugUtilsMessengerEXT");
            vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
            VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
            messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
            messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
#if !defined(NDEBUG)
            messengerCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
            messengerCreateInfo.messageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT/* | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT*/;
#endif
            messengerCreateInfo.pfnUserCallback = &DebugMessengerCallback;
			CHECK_VKCMD(vkCreateDebugUtilsMessengerEXT(vulkanInstance, &messengerCreateInfo, nullptr, &vulkanDebugMessenger));

			XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo { XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR };
			deviceGetInfo.systemId = systemId;
			deviceGetInfo.vulkanInstance = vulkanInstance;

			PFN_xrGetVulkanGraphicsDevice2KHR xrGetVulkanGraphicsDevice2KHR = nullptr;
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDevice2KHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrGetVulkanGraphicsDevice2KHR)));

			CHECK_XRCMD(xrGetVulkanGraphicsDevice2KHR(instance, &deviceGetInfo, &vulkanPhysicalDevice));

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

			auto vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)vkGetInstanceProcAddr(vulkanInstance, "vkEnumerateDeviceExtensionProperties");

			uint32_t availableExtensionCount = 0;
			vkEnumerateDeviceExtensionProperties(vulkanPhysicalDevice, nullptr, &availableExtensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
			vkEnumerateDeviceExtensionProperties(vulkanPhysicalDevice, nullptr, &availableExtensionCount, availableExtensions.data());

			auto indexTypeUInt8Enabled = false;
			auto externalMemoryFdEnabled = false;
			
			const std::string indentStr(4, ' ');
			__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "%sAvailable Vulkan Extensions: (%d)", indentStr.c_str(), availableExtensionCount);
			for (uint32_t i = 0; i < availableExtensionCount; ++i)
			{
				__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "%s  Name=%s SpecVersion=%d", indentStr.c_str(), availableExtensions[i].extensionName, availableExtensions[i].specVersion);

				if (strcmp(availableExtensions[i].extensionName, VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME) == 0)
				{
					indexTypeUInt8Enabled = true;
				}

				if (strcmp(availableExtensions[i].extensionName, VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) == 0)
				{
					externalMemoryFdEnabled = true;
				}
			}

			std::vector<const char*> deviceExtensions;

			if (indexTypeUInt8Enabled)
			{
				deviceExtensions.push_back(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME);
				appState.IndexTypeUInt8Enabled = true;
			}

			if (externalMemoryFdEnabled)
			{
				// UGLY HACK. Meta Quest devices require this extension, but the validation layers report that the OpenXR runtime does not enable it for you.
				deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
			}

			VkPhysicalDeviceFeatures features { };
			features.samplerAnisotropy = VK_TRUE;

			VkPhysicalDeviceMultiviewFeatures multiviewFeatures { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES };
			multiviewFeatures.multiview = VK_TRUE;

			VkDeviceCreateInfo deviceInfo { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
			deviceInfo.queueCreateInfoCount = 1;
			deviceInfo.pQueueCreateInfos = &queueInfo;
			deviceInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
			deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
			deviceInfo.pEnabledFeatures = &features;
			deviceInfo.pNext = &multiviewFeatures;

			VkPhysicalDeviceIndexTypeUint8FeaturesEXT indexTypeUint8Feature { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT };
			if (indexTypeUInt8Enabled)
			{
				indexTypeUint8Feature.indexTypeUint8 = VK_TRUE;
				multiviewFeatures.pNext = &indexTypeUint8Feature;
			}

			XrVulkanDeviceCreateInfoKHR deviceCreateInfo { XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR };
			deviceCreateInfo.systemId = systemId;
			deviceCreateInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
			deviceCreateInfo.vulkanCreateInfo = &deviceInfo;
			deviceCreateInfo.vulkanPhysicalDevice = vulkanPhysicalDevice;

			PFN_xrCreateVulkanDeviceKHR xrCreateVulkanDeviceKHR = nullptr;
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrCreateVulkanDeviceKHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrCreateVulkanDeviceKHR)));

			VkResult errCreateVulkanDevice;
			CHECK_XRCMD(xrCreateVulkanDeviceKHR(instance, &deviceCreateInfo, &appState.Device, &errCreateVulkanDevice));
			CHECK_VKCMD(errCreateVulkanDevice);

			vkGetDeviceQueue(appState.Device, queueInfo.queueFamilyIndex, 0, &appState.Queue);

			vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice, &appState.MemoryProperties);

			graphicsBinding.instance = vulkanInstance;
			graphicsBinding.physicalDevice = vulkanPhysicalDevice;
			graphicsBinding.device = appState.Device;
			graphicsBinding.queueFamilyIndex = queueInfo.queueFamilyIndex;
			graphicsBinding.queueIndex = 0;

			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(vulkanPhysicalDevice, &properties);

			for (auto i = 0; i < 32; i++)
			{
				auto flag = 1 << i;
				if ((properties.limits.framebufferColorSampleCounts & flag) == flag && (properties.limits.framebufferDepthSampleCounts & flag) == flag)
				{
					vulkanSwapchainSampleCount = flag;
				}
			}
		}

		CHECK(instance != XR_NULL_HANDLE);

		VkCommandPoolCreateInfo commandPoolCreateInfo { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
		CHECK_VKCMD(vkCreateCommandPool(appState.Device, &commandPoolCreateInfo, nullptr, &appState.SetupCommandPool));

		VkPipelineCacheCreateInfo pipelineCacheCreateInfo { VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		CHECK_VKCMD(vkCreatePipelineCache(appState.Device, &pipelineCacheCreateInfo, nullptr, &appState.PipelineCache));

		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Creating session...");

		XrSessionCreateInfo sessionCreateInfo { XR_TYPE_SESSION_CREATE_INFO };
		sessionCreateInfo.next = reinterpret_cast<const XrBaseInStructure*>(&graphicsBinding);
		sessionCreateInfo.systemId = systemId;
		CHECK_XRCMD(xrCreateSession(instance, &sessionCreateInfo, &appState.Session));
		CHECK(appState.Session != XR_NULL_HANDLE);

		// Enumerate the supported color space options for the system.
		PFN_xrEnumerateColorSpacesFB xrEnumerateColorSpacesFB = nullptr;
		CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrEnumerateColorSpacesFB", (PFN_xrVoidFunction*)(&xrEnumerateColorSpacesFB)));

		uint32_t colorSpaceCount = 0;
		CHECK_XRCMD(xrEnumerateColorSpacesFB(appState.Session, 0, &colorSpaceCount, nullptr));

		std::vector<XrColorSpaceFB> colorSpaces(colorSpaceCount);

		CHECK_XRCMD(xrEnumerateColorSpacesFB(appState.Session, colorSpaceCount, &colorSpaceCount, colorSpaces.data()));
		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Supported color spaces:");

		for (uint32_t i = 0; i < colorSpaceCount; i++) 
		{
			const char* name;
			switch (colorSpaces[i])
			{
				case 1:
					name = "XR_COLOR_SPACE_REC2020_FB";
					break;
				case 2:
					name = "XR_COLOR_SPACE_REC709_FB";
					break;
				case 3:
					name = "XR_COLOR_SPACE_RIFT_CV1_FB";
					break;
				case 4:
					name = "XR_COLOR_SPACE_RIFT_S_FB";
					break;
				case 5:
					name = "XR_COLOR_SPACE_QUEST_FB";
					break;
				case 6:
					name = "XR_COLOR_SPACE_P3_FB";
					break;
				case 7:
					name = "XR_COLOR_SPACE_ADOBE_RGB_FB";
					break;
				default:
					name = "XR_COLOR_SPACE_UNMANAGED_FB";
					break;
			}
			__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "%d:%d (%s)", i, colorSpaces[i], name);
		}

		PFN_xrSetColorSpaceFB xrSetColorSpaceFB = nullptr;
		CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrSetColorSpaceFB", (PFN_xrVoidFunction*)(&xrSetColorSpaceFB)));

		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Setting color space %i (%s)...", XR_COLOR_SPACE_REC2020_FB, "XR_COLOR_SPACE_REC2020_FB");
		CHECK_XRCMD(xrSetColorSpaceFB(appState.Session, XR_COLOR_SPACE_REC2020_FB));

		// Get the supported display refresh rates for the system.
		PFN_xrEnumerateDisplayRefreshRatesFB xrEnumerateDisplayRefreshRatesFB = nullptr;
		CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrEnumerateDisplayRefreshRatesFB", (PFN_xrVoidFunction*)(&xrEnumerateDisplayRefreshRatesFB)));

		uint32_t numSupportedDisplayRefreshRates;
		CHECK_XRCMD(xrEnumerateDisplayRefreshRatesFB(appState.Session, 0, &numSupportedDisplayRefreshRates, nullptr));

		std::vector<float> supportedDisplayRefreshRates(numSupportedDisplayRefreshRates);
		CHECK_XRCMD(xrEnumerateDisplayRefreshRatesFB(appState.Session, numSupportedDisplayRefreshRates, &numSupportedDisplayRefreshRates, supportedDisplayRefreshRates.data()));

		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Supported Refresh Rates:");
		auto highestDisplayRefreshRate = 0.0f;
		for (uint32_t i = 0; i < numSupportedDisplayRefreshRates; i++) 
		{
			__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "%d:%.1f", i, supportedDisplayRefreshRates[i]);
			highestDisplayRefreshRate = std::max(highestDisplayRefreshRate, supportedDisplayRefreshRates[i]);
		}

		PFN_xrRequestDisplayRefreshRateFB xrRequestDisplayRefreshRateFB;
		CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrRequestDisplayRefreshRateFB", (PFN_xrVoidFunction*)(&xrRequestDisplayRefreshRateFB)));

		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Requesting display refresh rate of %.1fHz...", highestDisplayRefreshRate);
		CHECK_XRCMD(xrRequestDisplayRefreshRateFB(appState.Session, highestDisplayRefreshRate));
		
		PFN_xrGetDisplayRefreshRateFB xrGetDisplayRefreshRateFB;
		CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction*)(&xrGetDisplayRefreshRateFB)));

		auto currentDisplayRefreshRate = 0.0f;
		CHECK_XRCMD(xrGetDisplayRefreshRateFB(appState.Session, &currentDisplayRefreshRate));
		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Current System Display Refresh Rate: %.1fHz.", currentDisplayRefreshRate);
		
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

		uint32_t viewCount;
		CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, 0, &viewCount, nullptr));
		std::vector<XrViewConfigurationView> configViews;
		configViews.resize(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
		CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, viewCount, &viewCount, configViews.data()));

		for (auto configView : configViews)
		{
			appState.EyeTextureWidth = configView.recommendedImageRectWidth;
			appState.EyeTextureHeight = configView.recommendedImageRectHeight;
			appState.EyeTextureMaxDimension = std::max(appState.EyeTextureWidth, appState.EyeTextureHeight);
			break;
		}

		std::vector<XrView> views(viewCount, { XR_TYPE_VIEW });

		uint32_t swapchainFormatCount;
		CHECK_XRCMD(xrEnumerateSwapchainFormats(appState.Session, 0, &swapchainFormatCount, nullptr));
		std::vector<int64_t> swapchainFormats(swapchainFormatCount);
		CHECK_XRCMD(xrEnumerateSwapchainFormats(appState.Session, (uint32_t) swapchainFormats.size(), &swapchainFormatCount, swapchainFormats.data()));
		CHECK(swapchainFormatCount == swapchainFormats.size());

		auto found = false;
		std::string swapchainFormatsString;
		for (int64_t format : swapchainFormats)
		{
			const bool selected = (format == (int64_t)Constants::colorFormat);
			swapchainFormatsString += " ";
			if (selected)
			{
				found = true;
				swapchainFormatsString += "[";
			}
			swapchainFormatsString += std::to_string(format);
			if (selected)
			{
				swapchainFormatsString += "]";
			}
		}
		__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Swapchain Formats: %s", swapchainFormatsString.c_str());
		if (!found)
		{
			THROW(Fmt("No runtime swapchain format supported for color swapchain %i", Constants::colorFormat));
		}

		appState.SwapchainRect.extent.width = configViews[0].recommendedImageRectWidth;
		appState.SwapchainRect.extent.height = configViews[0].recommendedImageRectHeight;
		appState.SwapchainSampleCount = vulkanSwapchainSampleCount;
		
		__android_log_print(ANDROID_LOG_INFO, "slipnfrag_native", "Creating swapchain with dimensions Width=%d Height=%d", appState.SwapchainRect.extent.width, appState.SwapchainRect.extent.height);

		XrSwapchainCreateInfo swapchainCreateInfo { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfo.format = Constants::colorFormat;
		swapchainCreateInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
		swapchainCreateInfo.width = appState.SwapchainRect.extent.width;
		swapchainCreateInfo.height = appState.SwapchainRect.extent.height;
		swapchainCreateInfo.faceCount = 1;
		swapchainCreateInfo.arraySize = viewCount;
		swapchainCreateInfo.mipCount = 1;
		XrSwapchain swapchain = VK_NULL_HANDLE;
		CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &swapchain));

		uint32_t imageCount;
		CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, 0, &imageCount, nullptr));

		appState.SwapchainImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
		CHECK_XRCMD(xrEnumerateSwapchainImages(swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)appState.SwapchainImages.data()));

		VkAttachmentReference colorReference { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		VkAttachmentReference resolveReference { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

		VkAttachmentDescription attachments[3] { };

		attachments[0].format = Constants::colorFormat;
		attachments[0].samples = (VkSampleCountFlagBits)appState.SwapchainSampleCount;
		attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		attachments[1].format = Constants::depthFormat;
		attachments[1].samples = (VkSampleCountFlagBits)appState.SwapchainSampleCount;
		attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		attachments[2].format = Constants::colorFormat;
		attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
		attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass { };
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;
		subpass.pDepthStencilAttachment = &depthReference;
		subpass.pResolveAttachments = &resolveReference;

		VkRenderPassCreateInfo renderPassInfo { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassInfo.attachmentCount = 3;
		renderPassInfo.pAttachments = attachments;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		const uint32_t viewMask = 3;

		VkRenderPassMultiviewCreateInfo multiviewCreateInfo { VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO };
		multiviewCreateInfo.subpassCount = 1;
		multiviewCreateInfo.pViewMasks = &viewMask;
		multiviewCreateInfo.correlationMaskCount = 1;
		multiviewCreateInfo.pCorrelationMasks = &viewMask;
		renderPassInfo.pNext = &multiviewCreateInfo;

		CHECK_VKCMD(vkCreateRenderPass(appState.Device, &renderPassInfo, nullptr, &appState.RenderPass));

		VkClearValue clearValues[2] { };
		clearValues[1].depthStencil.depth = 1.0f;

		VkRenderPassBeginInfo renderPassBeginInfo { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.renderPass = appState.RenderPass;
		renderPassBeginInfo.renderArea.extent = appState.SwapchainRect.extent;

		std::vector<XrCompositionLayerBaseHeader*> layers;

		XrCompositionLayerProjection worldLayer { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
		XrCompositionLayerCylinderKHR screenLayer { XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR };
		XrCompositionLayerCylinderKHR leftArrowsLayer { XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR };
		XrCompositionLayerCylinderKHR rightArrowsLayer { XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR };
		XrCompositionLayerCylinderKHR keyboardLayer { XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR };

		std::vector<XrCompositionLayerProjectionView> projectionLayerViews;

		XrCompositionLayerCubeKHR skyboxLayer { XR_TYPE_COMPOSITION_LAYER_CUBE_KHR };

		XrSwapchainImageWaitInfo waitInfo { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
		waitInfo.timeout = XR_INFINITE_DURATION;

		XrSwapchainImageReleaseInfo releaseInfo { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };

		VkSubmitInfo submitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;

		VkCommandBufferAllocateInfo commandBufferAllocateInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		commandBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBufferBeginInfo commandBufferBeginInfo { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		XrEventDataBuffer eventDataBuffer { };

        appState.FileLoader = new FileLoader(app);

        appState.VertexTransform.m[15] = 1;

		auto sessionState = XR_SESSION_STATE_UNKNOWN;
		auto sessionRunning = false;

		while (app->destroyRequested == 0)
		{
            struct android_poll_source* poll_source;
            const int timeoutMilliseconds = ((!appState.Resumed && !sessionRunning && app->destroyRequested == 0) ? -1 : 0);
            auto result = ALooper_pollOnce(timeoutMilliseconds, nullptr, nullptr, (void**) &poll_source);

            if (result == ALOOPER_POLL_ERROR)
            {
                THROW("ALooper_pollOnce returned ALOOPER_POLL_ERROR");
            }

            if (poll_source != nullptr)
            {
                poll_source->process(app, poll_source);
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
						__android_log_print(ANDROID_LOG_WARN, "slipnfrag_native", "XrEventDataInstanceLossPending by %ld", instanceLossPending.lossTime);
						exitRenderLoop = true;
						requestRestart = true;
						break;
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
							continue;
						}

						switch (sessionState)
						{
							case XR_SESSION_STATE_FOCUSED:
							{
								appState.Focused = true;
								break;
							}
							case XR_SESSION_STATE_VISIBLE:
							{
								appState.Focused = false;
								break;
							}
							case XR_SESSION_STATE_READY:
							{
								CHECK(appState.Session != XR_NULL_HANDLE);
								XrSessionBeginInfo sessionBeginInfo { XR_TYPE_SESSION_BEGIN_INFO };
								sessionBeginInfo.primaryViewConfigurationType = viewConfigurationType;
								CHECK_XRCMD(xrBeginSession(appState.Session, &sessionBeginInfo));
								sessionRunning = true;

								if (performanceSettingsEnabled)
								{
									// Set session state once we have entered VR mode and have a valid session object.
									PFN_xrPerfSettingsSetPerformanceLevelEXT xrPerfSettingsSetPerformanceLevelEXT = nullptr;
									CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrPerfSettingsSetPerformanceLevelEXT", (PFN_xrVoidFunction*)(&xrPerfSettingsSetPerformanceLevelEXT)));

									CHECK_XRCMD(xrPerfSettingsSetPerformanceLevelEXT(appState.Session, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT));
									CHECK_XRCMD(xrPerfSettingsSetPerformanceLevelEXT(appState.Session, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, XR_PERF_SETTINGS_LEVEL_BOOST_EXT));
								}

								if (appState.xrSetAndroidApplicationThreadKHR == nullptr)
								{
									CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrSetAndroidApplicationThreadKHR", (PFN_xrVoidFunction*) (&appState.xrSetAndroidApplicationThreadKHR)));
								}

								CHECK_XRCMD(appState.xrSetAndroidApplicationThreadKHR(appState.Session, XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR, appState.RenderThreadId));
								
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
						}
						break;
					}
					case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
					{
						const auto& perfSettingsEvent = *reinterpret_cast<const XrEventDataPerfSettingsEXT*>(event);
						__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "XrEventDataPerfSettingsEXT: type %d subdomain %d : level %d -> level %d", perfSettingsEvent.type, perfSettingsEvent.subDomain, perfSettingsEvent.fromLevel, perfSettingsEvent.toLevel);
						break;
					}
					case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB:
					{
						const auto& refreshRateChangedEvent = *reinterpret_cast<const XrEventDataDisplayRefreshRateChangedFB*>(event);
						__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "XrEventDataDisplayRefreshRateChangedFB: fromRate %f -> toRate %f", refreshRateChangedEvent.fromDisplayRefreshRate, refreshRateChangedEvent.toDisplayRefreshRate);
						break;
					}
					case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
					{
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
					}
					case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
					{
						const auto& referenceSpaceChangeEvent = *reinterpret_cast<const XrEventDataReferenceSpaceChangePending*>(event);
						__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "XrEventDataReferenceSpaceChangePending: changed space: %d for session %p at time %ld", referenceSpaceChangeEvent.referenceSpaceType, (void*)referenceSpaceChangeEvent.session, referenceSpaceChangeEvent.changeTime);
						break;
					}
					default:
						__android_log_print(ANDROID_LOG_VERBOSE, "slipnfrag_native", "Ignoring event type %d", event->type);
						break;
				}

				if (exitRenderLoop || requestRestart)
				{
					break;
				}
			}

			if (!sessionRunning)
			{
				continue;
			}

			const XrActiveActionSet activeActionSet { appState.ActionSet, XR_NULL_PATH };
			XrActionsSyncInfo syncInfo { XR_TYPE_ACTIONS_SYNC_INFO };
			syncInfo.countActiveActionSets = 1;
			syncInfo.activeActionSets = &activeActionSet;
			CHECK_XRCMD(xrSyncActions(appState.Session, &syncInfo));

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
				appState.Scene.Create(appState);
			}

			if (appState.EngineThreadCreated && !appState.EngineThreadStarted) // That is, if a previous call to android_main() already created an instance of EngineThread, but had to be closed:
			{
				if (snd_initialized) // If the sound system was already initialized in a previous call to android_main():
				{
					S_Startup ();
				}

				if (cdaudio_playing)
				{
					if (!CDAudio_Start (cdaudio_playTrack))
					{
						cdaudio_playing = false;
					}
				}

				appState.EngineThread = std::thread(runEngine, &appState, app);
				appState.EngineThreadStarted = true;
			}

			{
				std::lock_guard<std::mutex> lock(Locks::ModeChangeMutex);
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
						sys_version = "OXR 1.0.28";
						const char* basedir = "/sdcard/android/data/com.heribertodelgado.slipnfrag_xr/files";
						std::vector<std::string> arguments;
						arguments.emplace_back("SlipNFrag");
						arguments.emplace_back("-basedir");
						arguments.emplace_back(basedir);
						std::vector<unsigned char> commandline;
						auto file = fopen((std::string(basedir) + "/commandline.txt").c_str(), "rb");
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
						auto word_count = 0;
						for (auto c : commandline)
						{
							if (c <= ' ')
							{
								if (word_count == 0 && Q_strcasecmp(arguments[arguments.size() - 1].c_str(), "quake") == 0)
								{
									arguments[arguments.size() - 1].clear();
								}
								else if (!arguments[arguments.size() - 1].empty())
								{
									arguments.emplace_back();
									word_count++;
								}
							}
							else
							{
								arguments[arguments.size() - 1] += c;
							}
						}
						if (arguments[arguments.size() - 1].empty())
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
						appState.DefaultFOV = (int)Cvar_VariableValue("fov");
						r_load_as_rgba = true;
						d_skipfade = true;
						appState.EngineThread = std::thread(runEngine, &appState, app);
						appState.EngineThreadStarted = true;
						appState.EngineThreadCreated = true;
					}
					if (appState.Mode == AppScreenMode)
					{
						std::lock_guard<std::mutex> lock(Locks::RenderMutex);
						D_ResetLists();
						d_uselists = false;
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
					}
					else if (appState.Mode == AppWorldMode)
					{
						std::lock_guard<std::mutex> lock(Locks::RenderMutex);
						D_ResetLists();
						d_uselists = true;
						sb_onconsole = true;
						Cvar_SetValue("joyadvanced", 1);
						Cvar_SetValue("joyadvaxisx", AxisSide);
						Cvar_SetValue("joyadvaxisy", AxisForward);
						Cvar_SetValue("joyadvaxisz", AxisNada);
						Cvar_SetValue("joyadvaxisr", AxisNada);
						Joy_AdvancedUpdate_f();

						// The following is to prevent having stuck arrow keys at transition time:
						Input::AddKeyInput(K_DOWNARROW, false);
						Input::AddKeyInput(K_UPARROW, false);
						Input::AddKeyInput(K_LEFTARROW, false);
						Input::AddKeyInput(K_RIGHTARROW, false);

						vid_width = (int)appState.EyeTextureMaxDimension;
						vid_height = (int)appState.EyeTextureMaxDimension;
						con_width = appState.ConsoleWidth;
						con_height = appState.ConsoleHeight;
						Cvar_SetValue("fov", appState.FOV);
						VID_Resize(1);
					}
					else if (appState.Mode == AppNoGameDataMode)
					{
						std::lock_guard<std::mutex> lock(Locks::RenderMutex);
						D_ResetLists();
						d_uselists = false;
						sb_onconsole = false;
						appState.CallExitFunction = true;
					}
					appState.PreviousMode = appState.Mode;
				}
			}

			appState.Scene.textures.DeleteOld(appState);
			for (auto& entry : appState.Scene.surfaceRGBATextures)
			{
				entry.DeleteOld(appState);
			}
			for (auto& entry : appState.Scene.surfaceTextures)
			{
				entry.DeleteOld(appState);
			}
			appState.Scene.lightmapsRGBA.DeleteOld(appState);
			appState.Scene.lightmaps.DeleteOld(appState);
			appState.Scene.indexBuffers.DeleteOld(appState);
			appState.Scene.aliasBuffers.DeleteOld(appState);

			Skybox::DeleteOld(appState);

			XrFrameWaitInfo frameWaitInfo { XR_TYPE_FRAME_WAIT_INFO };
			XrFrameState frameState { XR_TYPE_FRAME_STATE };
			CHECK_XRCMD(xrWaitFrame(appState.Session, &frameWaitInfo, &frameState));

			XrFrameBeginInfo frameBeginInfo { XR_TYPE_FRAME_BEGIN_INFO };
			CHECK_XRCMD(xrBeginFrame(appState.Session, &frameBeginInfo));

			layers.clear();

			auto worldRendered = false;
			auto screenRendered = false;
			auto keyboardRendered = false;

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			VkFence fence = VK_NULL_HANDLE;

			if (frameState.shouldRender)
			{
				XrViewState viewState { XR_TYPE_VIEW_STATE };
				auto viewCapacityInput = (uint32_t)views.size();
				uint32_t viewCountOutput;

				XrViewLocateInfo viewLocateInfo { XR_TYPE_VIEW_LOCATE_INFO };
				viewLocateInfo.viewConfigurationType = viewConfigurationType;
				viewLocateInfo.displayTime = frameState.predictedDisplayTime;
				viewLocateInfo.space = appSpace;

				res = xrLocateViews(appState.Session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, views.data());
				CHECK_XRRESULT(res, "xrLocateViews");
				if ((viewState.viewStateFlags & (XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT)) == (XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT))
				{
					CHECK(viewCountOutput == viewCapacityInput);
					CHECK(viewCountOutput == configViews.size());

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

					if (appState.ViewMatrices.size() != viewCountOutput)
					{
						appState.ViewMatrices.resize(viewCountOutput);
					}
					if (appState.ProjectionMatrices.size() != viewCountOutput)
					{
						appState.ProjectionMatrices.resize(viewCountOutput);
					}

					for (size_t i = 0; i < viewCountOutput; i++)
					{
						XrMatrix4x4f rotation;
						XrMatrix4x4f_CreateFromQuaternion(&rotation, &views[i].pose.orientation);
						XrMatrix4x4f transposedRotation;
						XrMatrix4x4f_Transpose(&transposedRotation, &rotation);
						XrMatrix4x4f translation;
						XrMatrix4x4f_CreateTranslation(&translation, -views[i].pose.position.x, -views[i].pose.position.y, -views[i].pose.position.z);
						XrMatrix4x4f_Multiply(&appState.ViewMatrices[i], &transposedRotation, &translation);
						XrMatrix4x4f_CreateProjectionFov(&appState.ProjectionMatrices[i], GRAPHICS_VULKAN, views[i].fov, 0.05f, 0);
					}

					{
						std::lock_guard<std::mutex> lock(Locks::RenderInputMutex);
						
						appState.CameraLocation.type = XR_TYPE_SPACE_LOCATION;
						res = xrLocateSpace(screenSpace, appSpace, frameState.predictedDisplayTime, &appState.CameraLocation);
						CHECK_XRRESULT(res, "xrLocateSpace(screenSpace, appSpace)");
						appState.CameraLocationIsValid = (XR_UNQUALIFIED_SUCCESS(res) && (appState.CameraLocation.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) == (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT));

						if (appState.CameraLocationIsValid)
						{
							auto x = appState.CameraLocation.pose.orientation.x;
							auto y = appState.CameraLocation.pose.orientation.y;
							auto z = appState.CameraLocation.pose.orientation.z;
							auto w = appState.CameraLocation.pose.orientation.w;

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
						}

						float playerHeight = 32;
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

                        float angleHorizontal = FLT_MIN;
                        float angleUp = 0;
                        float angleDown = 0;
						for (size_t i = 0; i < viewCountOutput; i++)
						{
                            angleHorizontal = std::max(angleHorizontal, fabs(views[i].fov.angleLeft));
                            angleHorizontal = std::max(angleHorizontal, fabs(views[i].fov.angleRight));
                            angleUp += fabs(views[i].fov.angleUp);
                            angleDown += fabs(views[i].fov.angleDown);
						}
						appState.FOV = 2 * (int)floor(angleHorizontal * 180 / M_PI);
                        appState.SkyLeft = tan(angleHorizontal);
                        appState.SkyHorizontal = 2 * appState.SkyLeft;
                        appState.SkyTop = tan(angleUp / (float)viewCountOutput);
                        appState.SkyVertical = appState.SkyTop + tan(angleDown / (float)viewCountOutput);
					}
					
					projectionLayerViews.resize(viewCountOutput);
					
					uint32_t swapchainImageIndex;
					CHECK_XRCMD(xrAcquireSwapchainImage(swapchain, nullptr, &swapchainImageIndex));

					auto& perFrame = appState.PerFrame[swapchainImageIndex];

					if (perFrame.matrices == nullptr)
					{
						perFrame.matrices = new Buffer();
						perFrame.matrices->CreateHostVisibleUniformBuffer(appState, (2 * 2 + 1) * sizeof(XrMatrix4x4f));
						CHECK_VKCMD(vkMapMemory(appState.Device, perFrame.matrices->memory, 0, VK_WHOLE_SIZE, 0, &perFrame.matrices->mapped));
					}

					for (auto i = 0; i < viewCountOutput; i++)
					{
						projectionLayerViews[i] = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
						projectionLayerViews[i].pose = views[i].pose;
						projectionLayerViews[i].fov = views[i].fov;
						projectionLayerViews[i].subImage.swapchain = swapchain;
						projectionLayerViews[i].subImage.imageRect.extent.width = appState.SwapchainRect.extent.width;
						projectionLayerViews[i].subImage.imageRect.extent.height = appState.SwapchainRect.extent.height;
						projectionLayerViews[i].subImage.imageArrayIndex = i;
					}

					double clearR = 0;
					double clearG = 0;
					double clearB = 0;
					double clearA = 1;
					
					auto readClearColor = false;
					
					if (appState.Mode != AppWorldMode || appState.Scene.skybox != nullptr)
					{
						clearA = 0;
					}
					else
					{
						readClearColor = true;
					}

					perFrame.Reset(appState);

					appState.Scene.Initialize();

					VkDeviceSize stagingBufferSize;
					Buffer* stagingBuffer;
					{
						std::lock_guard<std::mutex> lock(Locks::RenderMutex);

						if (readClearColor && d_lists.clear_color >= 0)
						{
							auto color = d_8to24table[d_lists.clear_color];
							clearR = (color & 255) / 255.0f;
							clearG = (color >> 8 & 255) / 255.0f;
							clearB = (color >> 16 & 255) / 255.0f;
							clearA = (color >> 24) / 255.0f;
						}

						stagingBufferSize = appState.Scene.GetStagingBufferSize(appState, perFrame);

                        stagingBufferSize = ((stagingBufferSize >> 19) + 1) << 19;
                        stagingBuffer = perFrame.stagingBuffers.GetHostVisibleStorageBuffer(appState, stagingBufferSize);
                        if (stagingBuffer->mapped == nullptr)
                        {
                            CHECK_VKCMD(vkMapMemory(appState.Device, stagingBuffer->memory, 0, VK_WHOLE_SIZE, 0, &stagingBuffer->mapped));
                        }
                        perFrame.LoadStagingBuffer(appState, stagingBuffer);

						perFrame.LoadNonStagedResources(appState);

						if (appState.Mode == AppScreenMode || appState.Mode == AppWorldMode)
						{
							memcpy(appState.Scene.paletteData, d_8to24table, 256 * sizeof(unsigned int));
							if (appState.Mode == AppScreenMode)
							{
								size_t screenSize = vid_width * vid_height;
								if (appState.Scene.screenData.size() < screenSize)
								{
									appState.Scene.screenData.resize(screenSize);
								}
								memcpy(appState.Scene.screenData.data(), vid_buffer.data(), screenSize);
							}
							size_t consoleSize = con_width * con_height;
							if (appState.Scene.consoleData.size() < consoleSize)
							{
								appState.Scene.consoleData.resize(consoleSize);
							}
							memcpy(appState.Scene.consoleData.data(), con_buffer.data(), consoleSize);
						}
					}

					if (perFrame.commandBuffer == VK_NULL_HANDLE)
					{
						CHECK_VKCMD(vkCreateCommandPool(appState.Device, &commandPoolCreateInfo, nullptr, &perFrame.commandPool));

						commandBufferAllocateInfo.commandPool = perFrame.commandPool;
						CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &perFrame.commandBuffer));
					}

					commandBuffer = perFrame.commandBuffer;

					if (perFrame.fence == VK_NULL_HANDLE)
					{
						VkFenceCreateInfo fenceCreate {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
						CHECK_VKCMD(vkCreateFence(appState.Device, &fenceCreate, nullptr, &perFrame.fence));
					}
					else
					{
						uint64_t timeout = UINT64_MAX;
						CHECK_VKCMD(vkWaitForFences(appState.Device, 1, &perFrame.fence, VK_TRUE, timeout));
					}

					fence = perFrame.fence;

					CHECK_VKCMD(vkResetFences(appState.Device, 1, &fence));

					CHECK_VKCMD(vkResetCommandPool(appState.Device, perFrame.commandPool, 0));

					CHECK_VKCMD(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

                    perFrame.FillFromStagingBuffer(appState, stagingBuffer, swapchainImageIndex);

					clearValues[0].color.float32[0] = clearR;
					clearValues[0].color.float32[1] = clearG;
					clearValues[0].color.float32[2] = clearB;
					clearValues[0].color.float32[3] = clearA;

					if (perFrame.framebuffer == VK_NULL_HANDLE)
					{
						VkImageCreateInfo imageInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
						imageInfo.imageType = VK_IMAGE_TYPE_2D;
						imageInfo.extent.width = appState.SwapchainRect.extent.width;
						imageInfo.extent.height = appState.SwapchainRect.extent.height;
						imageInfo.extent.depth = 1;
						imageInfo.mipLevels = 1;
						imageInfo.arrayLayers = 2;
						imageInfo.samples = (VkSampleCountFlagBits)appState.SwapchainSampleCount;

						imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
						imageInfo.format = Constants::colorFormat;
						CHECK_VKCMD(vkCreateImage(appState.Device, &imageInfo, nullptr, &perFrame.colorImage));

						imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
						imageInfo.format = Constants::depthFormat;
						CHECK_VKCMD(vkCreateImage(appState.Device, &imageInfo, nullptr, &perFrame.depthImage));

						perFrame.resolveImage = appState.SwapchainImages[swapchainImageIndex].image;

						VkMemoryRequirements memRequirements { };
						VkMemoryAllocateInfo memoryAllocateInfo { };

						vkGetImageMemoryRequirements(appState.Device, perFrame.colorImage, &memRequirements);
						if (!createMemoryAllocateInfo(appState, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, memoryAllocateInfo, false))
						{
							createMemoryAllocateInfo(appState, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo, true);
						}
						CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &perFrame.colorMemory));
						CHECK_VKCMD(vkBindImageMemory(appState.Device, perFrame.colorImage, perFrame.colorMemory, 0));
						
						vkGetImageMemoryRequirements(appState.Device, perFrame.depthImage, &memRequirements);
						if (!createMemoryAllocateInfo(appState, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, memoryAllocateInfo, false))
						{
							createMemoryAllocateInfo(appState, memRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo, true);
						}
						CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &perFrame.depthMemory));
						CHECK_VKCMD(vkBindImageMemory(appState.Device, perFrame.depthImage, perFrame.depthMemory, 0));

						VkImageViewCreateInfo viewInfo { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
						viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
						viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
						viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
						viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
						viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
						viewInfo.subresourceRange.levelCount = 1;
						viewInfo.subresourceRange.layerCount = 2;

						viewInfo.image = perFrame.colorImage;
						viewInfo.format = Constants::colorFormat;
						viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						CHECK_VKCMD(vkCreateImageView(appState.Device, &viewInfo, nullptr, &perFrame.colorView));

						viewInfo.image = perFrame.depthImage;
						viewInfo.format = Constants::depthFormat;
						viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
						CHECK_VKCMD(vkCreateImageView(appState.Device, &viewInfo, nullptr, &perFrame.depthView));

						viewInfo.image = perFrame.resolveImage;
						viewInfo.format = Constants::colorFormat;
						viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						CHECK_VKCMD(vkCreateImageView(appState.Device, &viewInfo, nullptr, &perFrame.resolveView));

						VkImageView attachments[3];
						attachments[0] = perFrame.colorView;
						attachments[1] = perFrame.depthView;
						attachments[2] = perFrame.resolveView;

						VkFramebufferCreateInfo framebufferInfo { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
						framebufferInfo.renderPass = appState.RenderPass;
						framebufferInfo.attachmentCount = 3;
						framebufferInfo.pAttachments = attachments;
						framebufferInfo.width = appState.SwapchainRect.extent.width;
						framebufferInfo.height = appState.SwapchainRect.extent.height;
						framebufferInfo.layers = 1;
						CHECK_VKCMD(vkCreateFramebuffer(appState.Device, &framebufferInfo, nullptr, &perFrame.framebuffer));

						VkImageMemoryBarrier colorBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
						colorBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
						colorBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						colorBarrier.image = perFrame.colorImage;
						colorBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 2 };
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &colorBarrier);

						colorBarrier.image = perFrame.resolveImage;
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &colorBarrier);

						VkImageMemoryBarrier depthBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
						depthBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
						depthBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
						depthBarrier.image = perFrame.depthImage;
						depthBarrier.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 2 };
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr, 1, &depthBarrier);
					}

					renderPassBeginInfo.framebuffer = perFrame.framebuffer;
					vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

					perFrame.Render(appState, swapchainImageIndex);

					vkCmdEndRenderPass(commandBuffer);

					worldLayer.space = appSpace;
					worldLayer.viewCount = (uint32_t) projectionLayerViews.size();
					worldLayer.views = projectionLayerViews.data();

					if (appState.Mode != AppWorldMode || appState.Scene.skybox != nullptr)
					{
						worldLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
					}
					else
					{
						worldLayer.layerFlags = 0;
					}
					worldLayer.layerFlags |= XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
					
					layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&worldLayer));

					worldRendered = true;

					CHECK_XRCMD(xrAcquireSwapchainImage(appState.Screen.swapchain, nullptr, &swapchainImageIndex));

					auto& screenPerFrame = appState.Screen.perFrame[swapchainImageIndex];

					if (screenPerFrame.image == VK_NULL_HANDLE)
					{
						screenPerFrame.image = appState.Screen.swapchainImages[swapchainImageIndex].image;
					}

					if (screenPerFrame.stagingBuffer.buffer == VK_NULL_HANDLE)
					{
						screenPerFrame.stagingBuffer.CreateHostVisibleStorageBuffer(appState, appState.ScreenData.size() * sizeof(uint32_t));
						CHECK_VKCMD(vkMapMemory(appState.Device, screenPerFrame.stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &screenPerFrame.stagingBuffer.mapped));
					}

					appState.RenderScreen(screenPerFrame);

					appState.copyBarrier.image = screenPerFrame.image;
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.copyBarrier);

					VkBufferImageCopy region { };
					region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.imageSubresource.layerCount = 1;

					if (appState.Mode == AppWorldMode && key_dest == key_game)
					{
						auto& consoleTexture = appState.ConsoleTextures[swapchainImageIndex];

						VkImageMemoryBarrier imageMemoryBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						imageMemoryBarrier.subresourceRange.levelCount = 1;
						imageMemoryBarrier.subresourceRange.layerCount = 1;

						if (consoleTexture.filled)
						{
							imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
							imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						}
						imageMemoryBarrier.image = consoleTexture.image;
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

						region.imageExtent.width = appState.ConsoleWidth;
						region.imageExtent.height = appState.ConsoleHeight - (SBAR_HEIGHT + 24);
						region.imageExtent.depth = 1;
						vkCmdCopyBufferToImage(commandBuffer, screenPerFrame.stagingBuffer.buffer, consoleTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
						consoleTexture.filled = true;

						imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

						auto& statusBarTexture = appState.StatusBarTextures[swapchainImageIndex];
						if (statusBarTexture.filled)
						{
							imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
							imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						}
						else
						{
							imageMemoryBarrier.srcAccessMask = 0;
							imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
						}
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						imageMemoryBarrier.image = statusBarTexture.image;
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

						region.bufferOffset = region.imageExtent.width * sizeof(uint32_t) * region.imageExtent.height;
						region.imageOffset.y = (SBAR_HEIGHT + 24) * (Constants::screenToConsoleMultiplier - 1);
						region.imageExtent.height = SBAR_HEIGHT + 24;
						vkCmdCopyBufferToImage(commandBuffer, screenPerFrame.stagingBuffer.buffer, statusBarTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
						statusBarTexture.filled = true;
						region.imageOffset.y = 0;

						imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

						VkImageBlit blit { };
						blit.srcOffsets[1].x = appState.ConsoleWidth;
						blit.srcOffsets[1].y = appState.ConsoleHeight - (SBAR_HEIGHT + 24);
						blit.srcOffsets[1].z = 1;
						blit.dstOffsets[1].x = appState.ScreenWidth;
						blit.dstOffsets[1].y = appState.ScreenHeight - (SBAR_HEIGHT + 24) * Constants::screenToConsoleMultiplier;
						blit.dstOffsets[1].z = 1;
						blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						blit.srcSubresource.layerCount = 1;
						blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						blit.dstSubresource.layerCount = 1;
						vkCmdBlitImage(commandBuffer, consoleTexture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, screenPerFrame.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);

						VkImageCopy copy { };
						copy.dstOffset.y = appState.ScreenHeight - (SBAR_HEIGHT + 24) * Constants::screenToConsoleMultiplier;
						copy.extent.width = appState.ScreenWidth;
						copy.extent.height = (SBAR_HEIGHT + 24) * Constants::screenToConsoleMultiplier;
						copy.extent.depth = 1;
						copy.srcSubresource = blit.srcSubresource;
						copy.dstSubresource = blit.dstSubresource;
						vkCmdCopyImage(commandBuffer, statusBarTexture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, screenPerFrame.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

						imageMemoryBarrier.image = screenPerFrame.image;
					}
					else
					{
						region.imageExtent.width = appState.ScreenWidth;
						region.imageExtent.height = appState.ScreenHeight;
						region.imageExtent.depth = 1;
						vkCmdCopyBufferToImage(commandBuffer, screenPerFrame.stagingBuffer.buffer, screenPerFrame.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
					}

					appState.submitBarrier.image = screenPerFrame.image;
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.submitBarrier);

					screenLayer.radius = CylinderProjection::radius;
					screenLayer.aspectRatio = (float)appState.ScreenWidth / appState.ScreenHeight;
					screenLayer.centralAngle = CylinderProjection::horizontalAngle;
					screenLayer.subImage.swapchain = appState.Screen.swapchain;
					screenLayer.subImage.imageRect.extent.width = appState.ScreenWidth;
					screenLayer.subImage.imageRect.extent.height = appState.ScreenHeight;
					screenLayer.space = appSpace;

					if (appState.Mode == AppWorldMode)
					{
						if (appState.CameraLocationIsValid)
						{
							screenLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
							screenLayer.pose = appState.CameraLocation.pose;

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

					screenRendered = true;

					if (appState.Keyboard.Draw(appState))
					{
						CHECK_XRCMD(xrAcquireSwapchainImage(appState.Keyboard.Screen.swapchain, nullptr, &swapchainImageIndex));

						auto& keyboardPerFrame = appState.Keyboard.Screen.perFrame[swapchainImageIndex];

						if (keyboardPerFrame.image == VK_NULL_HANDLE)
						{
							keyboardPerFrame.image = appState.Keyboard.Screen.swapchainImages[swapchainImageIndex].image;
						}

						if (keyboardPerFrame.stagingBuffer.buffer == VK_NULL_HANDLE)
						{
							keyboardPerFrame.stagingBuffer.CreateHostVisibleStorageBuffer(appState, appState.ConsoleWidth * appState.ConsoleHeight / 2 * sizeof(uint32_t));
							CHECK_VKCMD(vkMapMemory(appState.Device, keyboardPerFrame.stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &keyboardPerFrame.stagingBuffer.mapped));
						}

						auto& keyboardTexture = appState.KeyboardTextures[swapchainImageIndex];
						if (keyboardTexture.image == VK_NULL_HANDLE)
						{
							keyboardTexture.width = appState.ConsoleWidth;
							keyboardTexture.height = appState.ConsoleHeight / 2;
							keyboardTexture.mipCount = 1;
							keyboardTexture.layerCount = 1;

							VkImageCreateInfo imageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
							imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
							imageCreateInfo.format = Constants::colorFormat;
							imageCreateInfo.extent.depth = 1;
							imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
							imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
							imageCreateInfo.extent.width = keyboardTexture.width;
							imageCreateInfo.extent.height = keyboardTexture.height;
							imageCreateInfo.mipLevels = keyboardTexture.mipCount;
							imageCreateInfo.arrayLayers = keyboardTexture.layerCount;
							CHECK_VKCMD(vkCreateImage(appState.Device, &imageCreateInfo, nullptr, &keyboardTexture.image));

							VkMemoryRequirements memoryRequirements;
							vkGetImageMemoryRequirements(appState.Device, keyboardTexture.image, &memoryRequirements);

							VkMemoryAllocateInfo memoryAllocateInfo { };
							createMemoryAllocateInfo(appState, memoryRequirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memoryAllocateInfo, true);

							CHECK_VKCMD(vkAllocateMemory(appState.Device, &memoryAllocateInfo, nullptr, &keyboardTexture.memory));
							CHECK_VKCMD(vkBindImageMemory(appState.Device, keyboardTexture.image, keyboardTexture.memory, 0));
						}

						appState.RenderKeyboard(keyboardPerFrame);

						appState.copyBarrier.image = keyboardPerFrame.image;
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.copyBarrier);

						VkBufferImageCopy region { };
						region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						region.imageSubresource.layerCount = 1;

						VkImageMemoryBarrier imageMemoryBarrier { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						imageMemoryBarrier.subresourceRange.levelCount = 1;
						imageMemoryBarrier.subresourceRange.layerCount = 1;

						if (keyboardTexture.filled)
						{
							imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
							imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						}
						imageMemoryBarrier.image = keyboardTexture.image;
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

						region.imageExtent.width = appState.ConsoleWidth;
						region.imageExtent.height = appState.ConsoleHeight / 2;
						region.imageExtent.depth = 1;
						vkCmdCopyBufferToImage(commandBuffer, keyboardPerFrame.stagingBuffer.buffer, keyboardTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
						keyboardTexture.filled = true;

						imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
						imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

						VkImageBlit blit { };
						blit.srcOffsets[1].x = appState.ConsoleWidth;
						blit.srcOffsets[1].y = appState.ConsoleHeight / 2;
						blit.srcOffsets[1].z = 1;
						blit.dstOffsets[1].x = appState.ScreenWidth;
						blit.dstOffsets[1].y = appState.ScreenHeight / 2;
						blit.dstOffsets[1].z = 1;
						blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						blit.srcSubresource.layerCount = 1;
						blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						blit.dstSubresource.layerCount = 1;
						vkCmdBlitImage(commandBuffer, keyboardTexture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, keyboardPerFrame.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);

						appState.submitBarrier.image = keyboardPerFrame.image;
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.submitBarrier);

						keyboardLayer.radius = CylinderProjection::radius;
						keyboardLayer.aspectRatio = (float)appState.ScreenWidth / (float)(appState.ScreenHeight / 2);
						keyboardLayer.centralAngle = CylinderProjection::horizontalAngle;
						keyboardLayer.subImage.swapchain = appState.Keyboard.Screen.swapchain;
						keyboardLayer.subImage.imageRect.extent.width = appState.ScreenWidth;
						keyboardLayer.subImage.imageRect.extent.height = appState.ScreenHeight / 2;
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

						keyboardRendered = true;
					}

					CHECK_VKCMD(vkEndCommandBuffer(commandBuffer));

					if (appState.Mode != AppWorldMode)
					{
						leftArrowsLayer.radius = CylinderProjection::radius;
						leftArrowsLayer.aspectRatio = 450 / 150;
						leftArrowsLayer.centralAngle = CylinderProjection::horizontalAngle * 450 / 960;
						leftArrowsLayer.subImage.swapchain = appState.LeftArrowsSwapchain;
						leftArrowsLayer.subImage.imageRect.extent.width = 450;
						leftArrowsLayer.subImage.imageRect.extent.height = 150;
						leftArrowsLayer.space = appSpace;
						leftArrowsLayer.layerFlags = 0;
						leftArrowsLayer.pose = { };
						
						XrMatrix4x4f rotation;
						
						XrMatrix4x4f_CreateRotation(&rotation, 0, 120, 0);
						XrMatrix4x4f_GetRotation(&leftArrowsLayer.pose.orientation, &rotation);

						layers.insert(layers.begin() + 1, reinterpret_cast<XrCompositionLayerBaseHeader*>(&leftArrowsLayer));

						rightArrowsLayer.radius = CylinderProjection::radius;
						rightArrowsLayer.aspectRatio = 450 / 150;
						rightArrowsLayer.centralAngle = CylinderProjection::horizontalAngle * 450 / 960;
						rightArrowsLayer.subImage.swapchain = appState.RightArrowsSwapchain;
						rightArrowsLayer.subImage.imageRect.extent.width = 450;
						rightArrowsLayer.subImage.imageRect.extent.height = 150;
						rightArrowsLayer.space = appSpace;
						rightArrowsLayer.layerFlags = 0;
						rightArrowsLayer.pose = { };

						XrMatrix4x4f_CreateRotation(&rotation, 0, -120, 0);
						XrMatrix4x4f_GetRotation(&rightArrowsLayer.pose.orientation, &rotation);

						layers.insert(layers.begin() + 2, reinterpret_cast<XrCompositionLayerBaseHeader*>(&rightArrowsLayer));
					}
					
					if (appState.Mode == AppWorldMode)
					{
						if (d_lists.last_skybox >= 0 && appState.Scene.skybox == nullptr)
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
								appState.Scene.skybox = new Skybox { };
								
								XrSwapchainCreateInfo swapchainCreateInfo { XR_TYPE_SWAPCHAIN_CREATE_INFO };
								swapchainCreateInfo.createFlags = XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
								swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
								swapchainCreateInfo.format = Constants::colorFormat;
								swapchainCreateInfo.sampleCount = VK_SAMPLE_COUNT_1_BIT;
								swapchainCreateInfo.width = width;
								swapchainCreateInfo.height = height;
								swapchainCreateInfo.faceCount = 6;
								swapchainCreateInfo.arraySize = 1;
								swapchainCreateInfo.mipCount = 1;
								CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.Scene.skybox->swapchain));

								uint32_t imageCount;
								CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Scene.skybox->swapchain, 0, &imageCount, nullptr));

								appState.Scene.skybox->images.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
								CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Scene.skybox->swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)appState.Scene.skybox->images.data()));

								Buffer buffer;
								buffer.CreateSourceBuffer(appState, 6 * width * height * 4);

								CHECK_VKCMD(vkMapMemory(appState.Device, buffer.memory, 0, VK_WHOLE_SIZE, 0, &buffer.mapped));

								auto target = (uint32_t*)buffer.mapped;

								for (auto i = 0; i < 6; i++)
								{
									std::string name;
									switch (i)
									{
										case 0:
											name = "bk";
											break;
										case 1:
											name = "ft";
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
									auto source = (uint32_t*)(((byte*)skybox.textures[index].texture) + sizeof(texture_t) + width * height) + width - 1;
									auto y = 0;
									while (y < height)
									{
										auto x = 0;
										while (x < width)
										{
											*target++ = *source--;
											x++;
										}
										y++;
										source += width;
										source += width;
									}
								}

								VkBufferImageCopy region { };
								region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
								region.imageSubresource.layerCount = 1;
								region.imageExtent.width = width;
								region.imageExtent.height = height;
								region.imageExtent.depth = 1;

								CHECK_XRCMD(xrAcquireSwapchainImage(appState.Scene.skybox->swapchain, nullptr, &swapchainImageIndex));

								CHECK_XRCMD(xrWaitSwapchainImage(appState.Scene.skybox->swapchain, &waitInfo));

								VkCommandBuffer setupCommandBuffer;

								commandBufferAllocateInfo.commandPool = appState.SetupCommandPool;
								CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &setupCommandBuffer));
								CHECK_VKCMD(vkBeginCommandBuffer(setupCommandBuffer, &commandBufferBeginInfo));
								
								appState.copyBarrier.image = appState.Scene.skybox->images[swapchainImageIndex].image;
								appState.submitBarrier.image = appState.copyBarrier.image;

								for (auto i = 0; i < 6; i++)
								{
									appState.copyBarrier.subresourceRange.baseArrayLayer = i;
									vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.copyBarrier);
									region.imageSubresource.baseArrayLayer = i;
									vkCmdCopyBufferToImage(setupCommandBuffer, buffer.buffer, appState.Scene.skybox->images[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
									region.bufferOffset += width * height * 4;
									appState.submitBarrier.subresourceRange.baseArrayLayer = i;
									vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.submitBarrier);
								}

								appState.copyBarrier.subresourceRange.baseArrayLayer = 0;
								appState.submitBarrier.subresourceRange.baseArrayLayer = 0;

								CHECK_VKCMD(vkEndCommandBuffer(setupCommandBuffer));

								VkSubmitInfo setupSubmitInfo { VK_STRUCTURE_TYPE_SUBMIT_INFO };
								setupSubmitInfo.commandBufferCount = 1;
								setupSubmitInfo.pCommandBuffers = &setupCommandBuffer;
								CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &setupSubmitInfo, VK_NULL_HANDLE));

								CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));

								CHECK_XRCMD(xrReleaseSwapchainImage(appState.Scene.skybox->swapchain, &releaseInfo));

								vkFreeCommandBuffers(appState.Device, appState.SetupCommandPool, 1, &setupCommandBuffer);

								buffer.Delete(appState);
							}
						}
						if (appState.Scene.skybox != VK_NULL_HANDLE)
						{
							XrMatrix4x4f rotation;
							XrMatrix4x4f_CreateRotation(&rotation, 0, 90, 0);
							XrMatrix4x4f scale;
							XrMatrix4x4f_CreateScale(&scale, 1, 1, 1);
							XrMatrix4x4f transform;
							XrMatrix4x4f_Multiply(&transform, &rotation, &scale);
							XrMatrix4x4f_GetRotation(&skyboxLayer.orientation, &transform);

							skyboxLayer.space = appSpace;
							skyboxLayer.swapchain = appState.Scene.skybox->swapchain;

							layers.insert(layers.begin(), reinterpret_cast<XrCompositionLayerBaseHeader*>(&skyboxLayer));
						}
					}
				}
			}

			if (worldRendered)
			{
				CHECK_XRCMD(xrWaitSwapchainImage(swapchain, &waitInfo));
			}
			if (screenRendered)
			{
				CHECK_XRCMD(xrWaitSwapchainImage(appState.Screen.swapchain, &waitInfo));
			}
			if (keyboardRendered)
			{
				CHECK_XRCMD(xrWaitSwapchainImage(appState.Keyboard.Screen.swapchain, &waitInfo));
			}

			if (commandBuffer != VK_NULL_HANDLE && fence != VK_NULL_HANDLE)
			{
				submitInfo.pCommandBuffers = &commandBuffer;
				CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &submitInfo, fence));
			}

			if (worldRendered)
			{
				CHECK_XRCMD(xrReleaseSwapchainImage(swapchain, &releaseInfo));
			}
			if (screenRendered)
			{
				CHECK_XRCMD(xrReleaseSwapchainImage(appState.Screen.swapchain, &releaseInfo));
			}
			if (keyboardRendered)
			{
				CHECK_XRCMD(xrReleaseSwapchainImage(appState.Keyboard.Screen.swapchain, &releaseInfo));
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
			appState.EngineThreadStopped = false; // By doing this, EngineThread is allowed to be recreated and resumed the next time android_main() is called.
			appState.EngineThreadStarted = false;
		}

		if (appState.Queue != VK_NULL_HANDLE)
		{
			CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));
		}

		if (appState.Device != VK_NULL_HANDLE)
		{
			for (auto& perFrame : appState.PerFrame)
			{
				if (perFrame.second.framebuffer != VK_NULL_HANDLE)
				{
					vkDestroyFramebuffer(appState.Device, perFrame.second.framebuffer, nullptr);
				}
				if (perFrame.second.resolveView != VK_NULL_HANDLE)
				{
					vkDestroyImageView(appState.Device, perFrame.second.resolveView, nullptr);
				}
				if (perFrame.second.depthView != VK_NULL_HANDLE)
				{
					vkDestroyImageView(appState.Device, perFrame.second.depthView, nullptr);
				}
				if (perFrame.second.colorView != VK_NULL_HANDLE)
				{
					vkDestroyImageView(appState.Device, perFrame.second.colorView, nullptr);
				}
				if (perFrame.second.depthImage != VK_NULL_HANDLE)
				{
					vkDestroyImage(appState.Device, perFrame.second.depthImage, nullptr);
				}
				if (perFrame.second.depthMemory != VK_NULL_HANDLE)
				{
					vkFreeMemory(appState.Device, perFrame.second.depthMemory, nullptr);
				}
				if (perFrame.second.colorImage != VK_NULL_HANDLE)
				{
					vkDestroyImage(appState.Device, perFrame.second.colorImage, nullptr);
				}
				if (perFrame.second.colorMemory != VK_NULL_HANDLE)
				{
					vkFreeMemory(appState.Device, perFrame.second.colorMemory, nullptr);
				}
			}

			appState.Scene.Reset(appState);

			for (auto& texture : appState.KeyboardTextures)
			{
				texture.second.Delete(appState);
			}
			appState.KeyboardTextures.clear();

			for (auto& perFrame : appState.Keyboard.Screen.perFrame)
			{
				perFrame.second.stagingBuffer.Delete(appState);
			}
			appState.Keyboard.Screen.perFrame.clear();

			for (auto& texture : appState.StatusBarTextures)
			{
				texture.Delete(appState);
			}
			appState.StatusBarTextures.clear();

			for (auto& texture : appState.ConsoleTextures)
			{
				texture.Delete(appState);
			}
			appState.ConsoleTextures.clear();

			for (auto& perFrame : appState.Screen.perFrame)
			{
				perFrame.second.stagingBuffer.Delete(appState);
			}
			appState.Screen.perFrame.clear();

			for (auto& perFrame : appState.PerFrame)
			{
				perFrame.second.controllerResources.Delete(appState);
				perFrame.second.floorResources.Delete(appState);
				perFrame.second.colormapResources.Delete(appState);
                perFrame.second.storageAttributesResources.Delete(appState);
				perFrame.second.sceneMatricesAndColormapResources.Delete(appState);
				perFrame.second.sceneMatricesAndNeutralPaletteResources.Delete(appState);
				perFrame.second.sceneMatricesAndPaletteResources.Delete(appState);
				perFrame.second.sceneMatricesResources.Delete(appState);
				perFrame.second.host_colormapResources.Delete(appState);
                perFrame.second.skyRGBAResources.Delete(appState);
                perFrame.second.skyResources.Delete(appState);
				if (perFrame.second.skyRGBA != nullptr)
				{
					perFrame.second.skyRGBA->Delete(appState);
				}
				if (perFrame.second.sky != nullptr)
				{
					perFrame.second.sky->Delete(appState);
				}
				perFrame.second.colormaps.Delete(appState);
				perFrame.second.stagingBuffers.Delete(appState);
				perFrame.second.cachedColors.Delete(appState);
				perFrame.second.cachedIndices32.Delete(appState);
				perFrame.second.cachedIndices16.Delete(appState);
				perFrame.second.cachedIndices8.Delete(appState);
                perFrame.second.cachedStorageAttributes.Delete(appState);
				perFrame.second.cachedAttributes.Delete(appState);
				perFrame.second.cachedHostVisibleVertices.Delete(appState);
				perFrame.second.cachedVertices.Delete(appState);

				if (perFrame.second.fence != VK_NULL_HANDLE)
				{
					vkDestroyFence(appState.Device, perFrame.second.fence, nullptr);
				}

				if (perFrame.second.commandPool != VK_NULL_HANDLE)
				{
					vkDestroyCommandPool(appState.Device, perFrame.second.commandPool, nullptr);
				}
				if (perFrame.second.matrices != nullptr)
				{
					perFrame.second.matrices->Delete(appState);
				}
			}

			appState.PerFrame.clear();

			if (appState.RenderPass != VK_NULL_HANDLE)
			{
				vkDestroyRenderPass(appState.Device, appState.RenderPass, nullptr);
				appState.RenderPass = VK_NULL_HANDLE;
			}

			vkDestroySampler(appState.Device, appState.Scene.lightmapSampler, nullptr);
			if (appState.Scene.sampler != VK_NULL_HANDLE)
			{
				vkDestroySampler(appState.Device, appState.Scene.sampler, nullptr);
				appState.Scene.sampler = VK_NULL_HANDLE;
			}

			appState.Scene.controllerTexture.Delete(appState);
			appState.Scene.floorTexture.Delete(appState);

			appState.Scene.textures.Delete(appState);

			for (auto& entry : appState.Scene.surfaceRGBATextures)
			{
				entry.Delete(appState);
			}
			appState.Scene.surfaceRGBATextures.clear();

			for (auto& entry : appState.Scene.surfaceTextures)
			{
				entry.Delete(appState);
			}
			appState.Scene.surfaceTextures.clear();

			appState.Scene.lightmapsRGBA.Delete(appState);
			appState.Scene.lightmapRGBATextures.clear();

			appState.Scene.lightmaps.Delete(appState);
			appState.Scene.lightmapTextures.clear();

			appState.Scene.indexBuffers.Delete(appState);
			appState.Scene.aliasBuffers.Delete(appState);

            if (appState.Scene.colormap.image != VK_NULL_HANDLE)
            {
                appState.Scene.colormap.Delete(appState);
            }

            for (auto& buffer : appState.Scene.neutralPaletteBuffers)
            {
                if (buffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(appState.Device, buffer, nullptr);
                }
            }
            for (auto& buffer : appState.Scene.paletteBuffers)
            {
                if (buffer != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(appState.Device, buffer, nullptr);
                }
            }
            if (appState.Scene.paletteMemory != VK_NULL_HANDLE)
            {
                vkFreeMemory(appState.Device, appState.Scene.paletteMemory, nullptr);
            }

			appState.Scene.floorStrip.Delete(appState);
			appState.Scene.floor.Delete(appState);
			appState.Scene.skyRGBA.Delete(appState);
			appState.Scene.sky.Delete(appState);
			appState.Scene.colored.Delete(appState);
			appState.Scene.particle.Delete(appState);
			appState.Scene.viewmodels.Delete(appState);
			appState.Scene.alias.Delete(appState);
			appState.Scene.sprites.Delete(appState);
			appState.Scene.turbulentRotatedRGBAColoredLights.Delete(appState);
			appState.Scene.turbulentRotatedRGBALit.Delete(appState);
			appState.Scene.turbulentRotatedColoredLights.Delete(appState);
			appState.Scene.turbulentRotatedLit.Delete(appState);
			appState.Scene.turbulentRotatedRGBA.Delete(appState);
			appState.Scene.turbulentRotated.Delete(appState);
			appState.Scene.turbulentRGBAColoredLights.Delete(appState);
			appState.Scene.turbulentRGBALit.Delete(appState);
			appState.Scene.turbulentColoredLights.Delete(appState);
			appState.Scene.turbulentLit.Delete(appState);
			appState.Scene.turbulentRGBA.Delete(appState);
			appState.Scene.turbulent.Delete(appState);
			appState.Scene.fencesRotatedRGBANoGlowColoredLights.Delete(appState);
			appState.Scene.fencesRotatedRGBANoGlow.Delete(appState);
			appState.Scene.fencesRotatedRGBAColoredLights.Delete(appState);
			appState.Scene.fencesRotatedRGBA.Delete(appState);
			appState.Scene.fencesRotatedColoredLights.Delete(appState);
			appState.Scene.fencesRotated.Delete(appState);
			appState.Scene.fencesRGBANoGlowColoredLights.Delete(appState);
			appState.Scene.fencesRGBANoGlow.Delete(appState);
			appState.Scene.fencesRGBAColoredLights.Delete(appState);
			appState.Scene.fencesRGBA.Delete(appState);
			appState.Scene.fencesColoredLights.Delete(appState);
			appState.Scene.fences.Delete(appState);
			appState.Scene.surfacesRotatedRGBANoGlowColoredLights.Delete(appState);
			appState.Scene.surfacesRotatedRGBANoGlow.Delete(appState);
			appState.Scene.surfacesRotatedRGBAColoredLights.Delete(appState);
			appState.Scene.surfacesRotatedRGBA.Delete(appState);
			appState.Scene.surfacesRotatedColoredLights.Delete(appState);
			appState.Scene.surfacesRotated.Delete(appState);
			appState.Scene.surfacesRGBANoGlowColoredLights.Delete(appState);
			appState.Scene.surfacesRGBANoGlow.Delete(appState);
			appState.Scene.surfacesRGBAColoredLights.Delete(appState);
			appState.Scene.surfacesRGBA.Delete(appState);
			appState.Scene.surfacesColoredLights.Delete(appState);
			appState.Scene.surfaces.Delete(appState);

			vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.singleImageLayout, nullptr);
			appState.Scene.singleImageLayout = VK_NULL_HANDLE;
			vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.twoBuffersAndImageLayout, nullptr);
			appState.Scene.twoBuffersAndImageLayout = VK_NULL_HANDLE;
			vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.doubleBufferLayout, nullptr);
			appState.Scene.doubleBufferLayout = VK_NULL_HANDLE;
			vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.singleBufferLayout, nullptr);
			appState.Scene.singleBufferLayout = VK_NULL_HANDLE;
			vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.singleFragmentStorageBufferLayout, nullptr);
			appState.Scene.singleFragmentStorageBufferLayout = VK_NULL_HANDLE;
            vkDestroyDescriptorSetLayout(appState.Device, appState.Scene.singleStorageBufferLayout, nullptr);
            appState.Scene.singleStorageBufferLayout = VK_NULL_HANDLE;

			if (appState.SetupCommandPool != VK_NULL_HANDLE)
			{
				vkDestroyCommandPool(appState.Device, appState.SetupCommandPool, nullptr);
				appState.SetupCommandPool = VK_NULL_HANDLE;
			}
		}

		appState.Scene.created = false;

        delete appState.FileLoader;

		if (appState.ActionSet != XR_NULL_HANDLE)
		{
			xrDestroySpace(appState.HandSpaces[0]);
			xrDestroySpace(appState.HandSpaces[1]);
			appState.HandSpaces.clear();

			xrDestroyActionSet(appState.ActionSet);
			appState.ActionSet = XR_NULL_HANDLE;
		}

		Skybox::DeleteAll(appState);

		if (appState.RightArrowsSwapchain != XR_NULL_HANDLE)
		{
			xrDestroySwapchain(appState.RightArrowsSwapchain);
			appState.RightArrowsSwapchain = XR_NULL_HANDLE;
		}

		if (appState.LeftArrowsSwapchain != XR_NULL_HANDLE)
		{
			xrDestroySwapchain(appState.LeftArrowsSwapchain);
			appState.LeftArrowsSwapchain = XR_NULL_HANDLE;
		}

		if (appState.Keyboard.Screen.swapchain != XR_NULL_HANDLE)
		{
			xrDestroySwapchain(appState.Keyboard.Screen.swapchain);
			appState.Keyboard.Screen.swapchain = XR_NULL_HANDLE;
		}

		if (appState.Screen.swapchain != XR_NULL_HANDLE)
		{
			xrDestroySwapchain(appState.Screen.swapchain);
			appState.Screen.swapchain = XR_NULL_HANDLE;
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
			appState.Session = XR_NULL_HANDLE;
		}

		if (instance != XR_NULL_HANDLE)
		{
			xrDestroyInstance(instance);
		}

		if (appState.PipelineCache != VK_NULL_HANDLE)
		{
			vkDestroyPipelineCache(appState.Device, appState.PipelineCache, nullptr);
			appState.PipelineCache = VK_NULL_HANDLE;
		}

		if (appState.Device != VK_NULL_HANDLE)
		{
			vkDestroyDevice(appState.Device, nullptr);
			appState.Device = VK_NULL_HANDLE;
		}

		if (vulkanDebugMessenger != VK_NULL_HANDLE)
		{
			vkDestroyDebugUtilsMessengerEXT(vulkanInstance, vulkanDebugMessenger, nullptr);
		}

		if (vulkanInstance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(vulkanInstance, nullptr);
		}
	}
	catch (const std::exception& ex)
	{
		__android_log_print(ANDROID_LOG_ERROR, "slipnfrag_native", "Caught exception: %s", ex.what());
		appState.CallExitFunction = true;
	}
	catch (...)
	{
		__android_log_print(ANDROID_LOG_ERROR, "slipnfrag_native", "Caught unknown exception");
		appState.CallExitFunction = true;
	}

	app->activity->vm->DetachCurrentThread();

	if (appState.CallExitFunction)
	{
		__android_log_print(ANDROID_LOG_ERROR, "slipnfrag_native", "exit(-1) called");
		exit(-1); // Not redundant - app won't truly exit Android unless forcefully terminated
	}
	else
	{
		// Shut down temporarily the sound system from the core engine:
        {
            std::lock_guard<std::mutex> lock(Locks::SoundMutex);
            CDAudio_DisposeBuffers ();
        }

		if (sound_started)
		{
			SNDDMA_Shutdown();
		}
	}
}
