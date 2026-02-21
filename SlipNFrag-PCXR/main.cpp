#include "AppState_pcxr.h"
#include "Utils.h"
#include "FileLoader_pcxr.h"
#include "Logger_pcxr.h"
#include <locale>
#include "CylinderProjection.h"
#include "PlanarProjection.h"
#include "Constants.h"
#include "AppInput.h"
#include "EngineThread.h"
#include "Locks.h"
#include "sys_pcxr.h"
#include "r_local.h"
#include "in_pcxr.h"
#include "vid_pcxr.h"

wchar_t snd_audio_output_device_id[XR_MAX_AUDIO_DEVICE_STR_SIZE_OCULUS];
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

	appState.Logger->Info((std::string(name) + " action is bound to %s").c_str(), (!sourceName.empty() ? sourceName.c_str() : "nothing"));
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
			printf("[LOG_WARN] %s: %d events lost\n", Logger_pcxr::tag, eventsLost->lostEventCount);
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
	const char* priority = "LOG_UNKNOWN";

	if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0u)
	{
		severityName += "ERROR:";
		priority = "LOG_ERROR";
	}
	if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0u)
	{
		severityName += "WARN:";
		priority = "LOG_WARN";
	}
	if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) != 0u)
	{
		severityName += "INFO:";
		priority = "LOG_INFO";
	}
	if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0u)
	{
		severityName += "VERB:";
		priority = "LOG_VERBOSE";
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

	printf("[%s] %s: [%s%s] %s\n", priority, Logger_pcxr::tag, typeName.c_str(), severityName.c_str(), pCallbackData->pMessage);
	
	return VK_FALSE;
}

void PrintErrorMessage(const std::string& message)
{
	constexpr size_t columns = 80;
	constexpr size_t rows = 25;
	std::vector<size_t> lines;
	size_t column = 0;
	auto last_space = -1;
	size_t position = 0;
	while (position < message.length())
	{
		auto c = message[position];
		if (c == '\n')
		{
			lines.push_back(position - column);
			last_space = -1;
			column = 0;
			position++;
		}
		else if (c == ' ')
		{
			last_space = column;
			column++;
			position++;
		}
		else if (column >= columns - 4)
		{
			if (last_space >= 0)
			{
				position -= (column - last_space);
				lines.push_back(position - column + 1);
			}
			else
			{
				lines.push_back(position - column);
			}
			last_space = -1;
			column = 0;
			position++;
		}
		else
		{
			column++;
			position++;
		}
	}
	if (lines.size() == 0)
	{
		lines.push_back(0);
	}
	lines.push_back(message.length());
	for (auto i = 0; i < columns - 1; i++)
	{
		printf("*");
	}
	printf("\n");
	auto blank = rows - 2 - (int)lines.size();
	int top;
	int bottom;
	if (blank > 0)
	{
		top = (int)floor((float)blank / 2);
		bottom = blank - top;
	}
	else
	{
		top = 0;
		bottom = 0;
	}
	while (top > 0)
	{
		printf("*");
		for (auto i = 0; i < columns - 3; i++)
		{
			printf(" ");
		}
		printf("*\n");
		top--;
	}
	for (size_t l = 0; l < lines.size() - 1; l++)
	{
		auto start = lines[l];
		auto end = lines[l + 1];
		printf("* ");
		for (size_t i = start; i < end; i++)
		{
			if (message[i] < ' ')
			{
				continue;
			}
			printf("%c", message[i]);
		}
		auto remaining = columns - 3 - end + start;
		while (remaining > 0)
		{
			printf(" ");
			remaining--;
		}
		printf("*\n");
	}
	while (bottom > 0)
	{
		printf("*");
		for (auto i = 0; i < columns - 3; i++)
		{
			printf(" ");
		}
		printf("*\n");
		bottom--;
	}
	for (auto i = 0; i < columns - 1; i++)
	{
		printf("*");
	}
	printf("\n");
}

AppState_pcxr appState { };

int main(int argc, char* argv[])
{
	CHECK_HRCMD(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

	appState.FileLoader = new FileLoader_pcxr();
	appState.Logger = new Logger_pcxr();

	bool exitWithError = false;

	try
	{
		XrGraphicsBindingVulkan2KHR graphicsBinding { XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR };

		VkInstance vulkanInstance = VK_NULL_HANDLE;
		VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;
		uint32_t queueFamilyIndex = 0;

#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
		VkDebugUtilsMessengerEXT vulkanDebugMessenger = VK_NULL_HANDLE;
#endif

		std::vector<std::string> xrInstanceExtensionSources
		{
			XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME,
			XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME
		};

		auto performanceSettingsEnabled = false;
		auto colorSpacesEnabled = false;
		auto cubeCompositionLayerEnabled = false;
		auto depthCompositionLayerEnabled = false;
		auto audioDeviceGuidEnabled = false;
		auto handTrackingEnabled = false;
		auto simultaneousHandsAndControllersEnabled = false;

		uint32_t instanceExtensionCount;
		CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(nullptr, 0, &instanceExtensionCount, nullptr));

		std::vector<XrExtensionProperties> extensions(instanceExtensionCount);
		for (XrExtensionProperties& extension : extensions)
		{
			extension.type = XR_TYPE_EXTENSION_PROPERTIES;
		}

		CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(nullptr, (uint32_t)extensions.size(), &instanceExtensionCount, extensions.data()));

		appState.Logger->Verbose("Available OpenXR extensions: (%d)", instanceExtensionCount);

		for (const XrExtensionProperties& extension : extensions)
		{
			if (strncmp(extension.extensionName, XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME, sizeof(extension.extensionName)) == 0)
			{
				performanceSettingsEnabled = true;
				xrInstanceExtensionSources.emplace_back(extension.extensionName);
			}
			else if (strncmp(extension.extensionName, XR_FB_COLOR_SPACE_EXTENSION_NAME, sizeof(extension.extensionName)) == 0)
			{
				colorSpacesEnabled = true;
				xrInstanceExtensionSources.emplace_back(extension.extensionName);
			}
			else if (strncmp(extension.extensionName, XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME, sizeof(extension.extensionName)) == 0)
			{
				appState.CylinderCompositionLayerEnabled = true;
				xrInstanceExtensionSources.emplace_back(extension.extensionName);
			}
			else if (strncmp(extension.extensionName, XR_KHR_COMPOSITION_LAYER_CUBE_EXTENSION_NAME, sizeof(extension.extensionName)) == 0)
			{
				cubeCompositionLayerEnabled = true;
				xrInstanceExtensionSources.emplace_back(extension.extensionName);
			}
			else if (strncmp(extension.extensionName, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME, sizeof(extension.extensionName)) == 0)
			{
				// Temporarily disabled - the current implementation induces wobbling when moving around the scene:
				//depthCompositionLayerEnabled = true;
				xrInstanceExtensionSources.emplace_back(extension.extensionName);
			}
			else if (strncmp(extension.extensionName, XR_OCULUS_AUDIO_DEVICE_GUID_EXTENSION_NAME, sizeof(extension.extensionName)) == 0)
			{
				audioDeviceGuidEnabled = true;
				xrInstanceExtensionSources.emplace_back(extension.extensionName);
			}
			else if (strncmp(extension.extensionName, XR_EXT_HAND_TRACKING_EXTENSION_NAME, sizeof(extension.extensionName)) == 0)
			{
				handTrackingEnabled = true;
				xrInstanceExtensionSources.emplace_back(extension.extensionName);
			}
			else if (strncmp(extension.extensionName, XR_META_SIMULTANEOUS_HANDS_AND_CONTROLLERS_EXTENSION_NAME, sizeof(extension.extensionName)) == 0)
			{
				simultaneousHandsAndControllersEnabled = true;
				xrInstanceExtensionSources.emplace_back(extension.extensionName);
			}

			appState.Logger->Verbose("  Name=%s SpecVersion=%d", extension.extensionName, extension.extensionVersion);
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

		appState.Logger->Info("Available Layers: (%d)", xrLayerCount);
		for (const XrApiLayerProperties& layer : xrLayers)
		{
			appState.Logger->Verbose("  Name=%s SpecVersion=%s LayerVersion=%d Description=%s", layer.layerName, GetXrVersionString(layer.specVersion).c_str(), layer.layerVersion, layer.description);

			CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layer.layerName, 0, &instanceExtensionCount, nullptr));

			CHECK_XRCMD(xrEnumerateInstanceExtensionProperties(layer.layerName, (uint32_t)extensions.size(), &instanceExtensionCount, extensions.data()));

			const std::string indentStr(4, ' ');
			appState.Logger->Verbose("%sAvailable OpenXR extensions for layer %s: (%d)", indentStr.c_str(), layer.layerName, instanceExtensionCount);

			for (const XrExtensionProperties& extension : extensions)
			{
				appState.Logger->Verbose("%s  Name=%s SpecVersion=%d", indentStr.c_str(), extension.extensionName, extension.extensionVersion);
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

		XrInstanceCreateInfo createInfo { XR_TYPE_INSTANCE_CREATE_INFO };
		createInfo.enabledExtensionCount = (uint32_t)xrInstanceExtensions.size();
		createInfo.enabledExtensionNames = xrInstanceExtensions.data();
		createInfo.enabledApiLayerCount = (uint32_t)xrInstanceApiLayers.size();
		createInfo.enabledApiLayerNames = xrInstanceApiLayers.data();

		strcpy(createInfo.applicationInfo.applicationName, "SlipNFrag-PCXR");

		createInfo.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 34);

		CHECK_XRCMD(xrCreateInstance(&createInfo, &instance));

		CHECK(instance != XR_NULL_HANDLE);

		XrInstanceProperties instanceProperties { XR_TYPE_INSTANCE_PROPERTIES };
		CHECK_XRCMD(xrGetInstanceProperties(instance, &instanceProperties));

		appState.Logger->Info("Instance RuntimeName=%s RuntimeVersion=%s", instanceProperties.runtimeName, GetXrVersionString(instanceProperties.runtimeVersion).c_str());

		XrSystemId systemId = XR_NULL_SYSTEM_ID;
		XrSystemGetInfo systemInfo { XR_TYPE_SYSTEM_GET_INFO };
		systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		CHECK_XRCMD(xrGetSystem(instance, &systemInfo, &systemId));

		appState.Logger->Verbose("Using system %lu for form factor %s", systemId, to_string(systemInfo.formFactor));

		CHECK(systemId != XR_NULL_SYSTEM_ID);

		uint32_t viewConfigTypeCount;
		CHECK_XRCMD(xrEnumerateViewConfigurations(instance, systemId, 0, &viewConfigTypeCount, nullptr));
		std::vector<XrViewConfigurationType> viewConfigTypes(viewConfigTypeCount);
		CHECK_XRCMD(xrEnumerateViewConfigurations(instance, systemId, viewConfigTypeCount, &viewConfigTypeCount, viewConfigTypes.data()));
		CHECK((uint32_t) viewConfigTypes.size() == viewConfigTypeCount);

		auto viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;

		appState.Logger->Info("Available View Configuration Types: (%d)", viewConfigTypeCount);
		for (XrViewConfigurationType viewConfigType : viewConfigTypes)
		{
			appState.Logger->Verbose("  View Configuration Type: %s %s", to_string(viewConfigType), viewConfigType == viewConfigurationType ? "(Selected)" : "");

			XrViewConfigurationProperties viewConfigProperties { XR_TYPE_VIEW_CONFIGURATION_PROPERTIES };
			CHECK_XRCMD(xrGetViewConfigurationProperties(instance, systemId, viewConfigType, &viewConfigProperties));

			appState.Logger->Verbose("  View configuration FovMutable=%s", (viewConfigProperties.fovMutable == XR_TRUE ? "True" : "False"));

			uint32_t viewCount;
			CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId, viewConfigType, 0, &viewCount, nullptr));
			if (viewCount > 0)
			{
				std::vector<XrViewConfigurationView> views(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
				CHECK_XRCMD(xrEnumerateViewConfigurationViews(instance, systemId, viewConfigType, viewCount, &viewCount, views.data()));

				for (uint32_t i = 0; i < views.size(); i++)
				{
					const XrViewConfigurationView& view = views[i];

					appState.Logger->Verbose("    View [%d]: Recommended Width=%d Height=%d SampleCount=%d", i, view.recommendedImageRectWidth, view.recommendedImageRectHeight, view.recommendedSwapchainSampleCount);
					appState.Logger->Verbose("    View [%d]:     Maximum Width=%d Height=%d SampleCount=%d", i, view.maxImageRectWidth, view.maxImageRectHeight, view.maxSwapchainSampleCount);
				}
			}
			else
			{
				appState.Logger->Error("Empty view configuration type");
			}

			uint32_t count;
			CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigType, 0, &count, nullptr));
			CHECK(count > 0);

			appState.Logger->Info("Available Environment Blend Mode count : (%d)", count);

			std::vector<XrEnvironmentBlendMode> blendModes(count);
			CHECK_XRCMD(xrEnumerateEnvironmentBlendModes(instance, systemId, viewConfigType, count, &count, blendModes.data()));

			bool blendModeFound = false;
			for (XrEnvironmentBlendMode mode : blendModes)
			{
				const bool blendModeMatch = (mode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE);
				appState.Logger->Info("Environment Blend Mode (%s) : %s", to_string(mode), blendModeMatch ? "(Selected)" : "");
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
		//const char* vulkanAPIDumpLayerName = "VK_LAYER_LUNARG_api_dump";

		std::vector<const char*> validationLayerNames
		{
			vulkanValidationLayerName
		//	,vulkanAPIDumpLayerName
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
			appState.Logger->Warn("No validation layers found in the system, skipping");
		}
#endif

		auto maintenance4 = false;
		auto bufferDeviceAddress = false;
		auto createRenderPass2 = false;
		auto depthStencilResolve = false;
		uint32_t vulkanSwapchainSampleCount;
		{
			std::vector<const char*> vulkanExtensions;

#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			vulkanExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

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
			VkBool32 vvlBestPracticesNVIDIA = VK_FALSE; // VK_TRUE;
			//VkBool32 adlOutputToFile = VK_TRUE;
			//const char* adlFilename[] = { "apiDump.txt" };

			VkLayerSettingEXT settings[] =
			{
				{ vulkanValidationLayerName, "validate_sync", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &vvlValidateSync },
				{ vulkanValidationLayerName, "thread_safety", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &vvlThreadSafety },
				{ vulkanValidationLayerName, "validate_best_practices", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &vvlBestPractices },
				{ vulkanValidationLayerName, "validate_best_practices_nvidia", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &vvlBestPracticesNVIDIA }
			//	,{ vulkanAPIDumpLayerName, "file", VK_LAYER_SETTING_TYPE_BOOL32_EXT, 1, &adlOutputToFile },
			//	,{ vulkanAPIDumpLayerName, "log_filename", VK_LAYER_SETTING_TYPE_STRING_EXT, (uint32_t)std::size(adlFilename), &adlFilename }
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

#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkCreateDebugUtilsMessengerEXT");
			vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
			VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
			messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
			messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT/* | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT*/;
			messengerCreateInfo.pfnUserCallback = &DebugMessengerCallback;
			CHECK_VKCMD(vkCreateDebugUtilsMessengerEXT(vulkanInstance, &messengerCreateInfo, nullptr, &vulkanDebugMessenger));
#endif

			XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo { XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR };
			deviceGetInfo.systemId = systemId;
			deviceGetInfo.vulkanInstance = vulkanInstance;

			PFN_xrGetVulkanGraphicsDevice2KHR xrGetVulkanGraphicsDevice2KHR = nullptr;
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetVulkanGraphicsDevice2KHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrGetVulkanGraphicsDevice2KHR)));

			CHECK_XRCMD(xrGetVulkanGraphicsDevice2KHR(instance, &deviceGetInfo, &vulkanPhysicalDevice));

			VkPhysicalDeviceFeatures2 physicalDeviceFeatures { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
			vkGetPhysicalDeviceFeatures2(vulkanPhysicalDevice, &physicalDeviceFeatures);

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

			auto shaderDemoteToHelperInvocation = false;
			auto shaderTerminateInvocation = false;
			std::vector<const char*> enabledExtensions;

			const std::string indentStr(4, ' ');
			appState.Logger->Verbose("%sAvailable Vulkan Extensions: (%d)", indentStr.c_str(), availableExtensionCount);
			for (uint32_t i = 0; i < availableExtensionCount; ++i)
			{
				appState.Logger->Verbose("%s  Name=%s SpecVersion=%d", indentStr.c_str(), availableExtensions[i].extensionName, availableExtensions[i].specVersion);

				if (strncmp(availableExtensions[i].extensionName, VK_KHR_MAINTENANCE_4_EXTENSION_NAME, sizeof(availableExtensions[i].extensionName)) == 0)
				{
					maintenance4 = true;
					enabledExtensions.push_back(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
				}
				else if (strncmp(availableExtensions[i].extensionName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, sizeof(availableExtensions[i].extensionName)) == 0)
				{
					bufferDeviceAddress = true;
					enabledExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
				}
				else if (strncmp(availableExtensions[i].extensionName, VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME, sizeof(availableExtensions[i].extensionName)) == 0)
				{
					appState.IndexTypeUInt8Enabled = true;
					enabledExtensions.push_back(VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME);
				}
				else if (strncmp(availableExtensions[i].extensionName, VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME, sizeof(availableExtensions[i].extensionName)) == 0)
				{
					shaderDemoteToHelperInvocation = true;
					enabledExtensions.push_back(VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME);
				}
				else if (strncmp(availableExtensions[i].extensionName, VK_KHR_SHADER_TERMINATE_INVOCATION_EXTENSION_NAME, sizeof(availableExtensions[i].extensionName)) == 0)
				{
					shaderTerminateInvocation = true;
					enabledExtensions.push_back(VK_KHR_SHADER_TERMINATE_INVOCATION_EXTENSION_NAME);
				}
				else if (strncmp(availableExtensions[i].extensionName, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME, sizeof(availableExtensions[i].extensionName)) == 0)
				{
					createRenderPass2 = true;
					enabledExtensions.push_back(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
				}
				else if (strncmp(availableExtensions[i].extensionName, VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, sizeof(availableExtensions[i].extensionName)) == 0)
				{
					depthStencilResolve = true;
					enabledExtensions.push_back(VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
				}
			}

			VkDeviceCreateInfo deviceInfo { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
			deviceInfo.queueCreateInfoCount = 1;
			deviceInfo.pQueueCreateInfos = &queueInfo;
			deviceInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
			deviceInfo.ppEnabledExtensionNames = enabledExtensions.data();

			void* chain = &deviceInfo;

			VkPhysicalDeviceFeatures2 features { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
			((VkBaseInStructure*)chain)->pNext = (VkBaseInStructure*)&features;

			features.features.shaderStorageImageMultisample = physicalDeviceFeatures.features.shaderStorageImageMultisample;
			features.features.fragmentStoresAndAtomics = physicalDeviceFeatures.features.fragmentStoresAndAtomics;
			features.features.robustBufferAccess = physicalDeviceFeatures.features.robustBufferAccess;
			features.features.shaderInt64 = physicalDeviceFeatures.features.shaderInt64;
			features.features.independentBlend = physicalDeviceFeatures.features.independentBlend;

			chain = (void*)((VkBaseInStructure*)chain)->pNext;

			VkPhysicalDeviceMultiviewFeatures multiviewFeatures { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES };
			multiviewFeatures.multiview = VK_TRUE;
			((VkBaseInStructure*)chain)->pNext = (VkBaseInStructure*)&multiviewFeatures;
			chain = (void*)((VkBaseInStructure*)chain)->pNext;

			VkPhysicalDeviceIndexTypeUint8FeaturesEXT indexTypeUint8Features { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_INDEX_TYPE_UINT8_FEATURES_EXT };
			if (appState.IndexTypeUInt8Enabled)
			{
				indexTypeUint8Features.indexTypeUint8 = VK_TRUE;
				((VkBaseInStructure*)chain)->pNext = (VkBaseInStructure*)&indexTypeUint8Features;
				chain = (void*)((VkBaseInStructure*)chain)->pNext;
			}

			VkPhysicalDeviceShaderDemoteToHelperInvocationFeaturesEXT shaderDemoteToHelperInvocationFeatures { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES_EXT };
			if (shaderDemoteToHelperInvocation)
			{
				shaderDemoteToHelperInvocationFeatures.shaderDemoteToHelperInvocation = VK_TRUE;
				((VkBaseInStructure*)chain)->pNext = (VkBaseInStructure*)&shaderDemoteToHelperInvocationFeatures;
				chain = (void*)((VkBaseInStructure*)chain)->pNext;
			}

			VkPhysicalDeviceShaderTerminateInvocationFeaturesKHR shaderTerminateInvocationFeatures { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_TERMINATE_INVOCATION_FEATURES_KHR };
			if (shaderTerminateInvocation)
			{
				shaderTerminateInvocationFeatures.shaderTerminateInvocation = VK_TRUE;
				((VkBaseInStructure*)chain)->pNext = (VkBaseInStructure*)&shaderTerminateInvocationFeatures;
				chain = (void*)((VkBaseInStructure*)chain)->pNext;
			}

			VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
			if (bufferDeviceAddress)
			{
				bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
				((VkBaseInStructure*)chain)->pNext = (VkBaseInStructure*)&bufferDeviceAddressFeatures;
				chain = (void*)((VkBaseInStructure*)chain)->pNext;
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

		appState.Logger->Verbose("Creating session...");

		XrSessionCreateInfo sessionCreateInfo { XR_TYPE_SESSION_CREATE_INFO };
		sessionCreateInfo.next = reinterpret_cast<const XrBaseInStructure*>(&graphicsBinding);
		sessionCreateInfo.systemId = systemId;
		CHECK_XRCMD(xrCreateSession(instance, &sessionCreateInfo, &appState.Session));
		CHECK(appState.Session != XR_NULL_HANDLE);

		if (colorSpacesEnabled)
		{
			// Enumerate the supported color space options for the system.
			PFN_xrEnumerateColorSpacesFB xrEnumerateColorSpacesFB = nullptr;
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrEnumerateColorSpacesFB", (PFN_xrVoidFunction*)(&xrEnumerateColorSpacesFB)));

			uint32_t colorSpaceCount = 0;
			CHECK_XRCMD(xrEnumerateColorSpacesFB(appState.Session, 0, &colorSpaceCount, nullptr));

			std::vector<XrColorSpaceFB> colorSpaces(colorSpaceCount);

			CHECK_XRCMD(xrEnumerateColorSpacesFB(appState.Session, colorSpaceCount, &colorSpaceCount, colorSpaces.data()));
			appState.Logger->Verbose("Supported color spaces:");

			auto rec2020Supported = false;

			for (uint32_t i = 0; i < colorSpaceCount; i++)
			{
				const char* name;
				switch (colorSpaces[i])
				{
					case 1:
						name = "XR_COLOR_SPACE_REC2020_FB";
						rec2020Supported = true;
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
				appState.Logger->Verbose("%d:%d (%s)", i, colorSpaces[i], name);
			}

			if (rec2020Supported)
			{
				PFN_xrSetColorSpaceFB xrSetColorSpaceFB = nullptr;
				CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrSetColorSpaceFB", (PFN_xrVoidFunction*)(&xrSetColorSpaceFB)));

				appState.Logger->Verbose("Setting color space %i (%s)...", XR_COLOR_SPACE_REC2020_FB, "XR_COLOR_SPACE_REC2020_FB");
				CHECK_XRCMD(xrSetColorSpaceFB(appState.Session, XR_COLOR_SPACE_REC2020_FB));
			}
		}

		// Get the supported display refresh rates for the system.
		PFN_xrEnumerateDisplayRefreshRatesFB xrEnumerateDisplayRefreshRatesFB = nullptr;
		CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrEnumerateDisplayRefreshRatesFB", (PFN_xrVoidFunction*)(&xrEnumerateDisplayRefreshRatesFB)));

		uint32_t numSupportedDisplayRefreshRates;
		CHECK_XRCMD(xrEnumerateDisplayRefreshRatesFB(appState.Session, 0, &numSupportedDisplayRefreshRates, nullptr));

		std::vector<float> supportedDisplayRefreshRates(numSupportedDisplayRefreshRates);
		CHECK_XRCMD(xrEnumerateDisplayRefreshRatesFB(appState.Session, numSupportedDisplayRefreshRates, &numSupportedDisplayRefreshRates, supportedDisplayRefreshRates.data()));

		appState.Logger->Verbose("Supported Refresh Rates:");
		auto highestDisplayRefreshRate = 0.0f;
		for (uint32_t i = 0; i < numSupportedDisplayRefreshRates; i++)
		{
			appState.Logger->Verbose("%d:%.1f", i, supportedDisplayRefreshRates[i]);
			highestDisplayRefreshRate = std::max(highestDisplayRefreshRate, supportedDisplayRefreshRates[i]);
		}

		PFN_xrRequestDisplayRefreshRateFB xrRequestDisplayRefreshRateFB;
		CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrRequestDisplayRefreshRateFB", (PFN_xrVoidFunction*)(&xrRequestDisplayRefreshRateFB)));

		appState.Logger->Verbose("Requesting display refresh rate of %.1fHz...", highestDisplayRefreshRate);
		CHECK_XRCMD(xrRequestDisplayRefreshRateFB(appState.Session, highestDisplayRefreshRate));

		PFN_xrGetDisplayRefreshRateFB xrGetDisplayRefreshRateFB;
		CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetDisplayRefreshRateFB", (PFN_xrVoidFunction*)(&xrGetDisplayRefreshRateFB)));

		auto currentDisplayRefreshRate = 0.0f;
		CHECK_XRCMD(xrGetDisplayRefreshRateFB(appState.Session, &currentDisplayRefreshRate));
		appState.Logger->Verbose("Current System Display Refresh Rate: %.1fHz.", currentDisplayRefreshRate);

		uint32_t spaceCount;
		CHECK_XRCMD(xrEnumerateReferenceSpaces(appState.Session, 0, &spaceCount, nullptr));
		std::vector<XrReferenceSpaceType> spaces(spaceCount);
		CHECK_XRCMD(xrEnumerateReferenceSpaces(appState.Session, spaceCount, &spaceCount, spaces.data()));

		appState.Logger->Info("Available reference spaces: %d", spaceCount);
		for (XrReferenceSpaceType space : spaces)
		{
			appState.Logger->Verbose("  Name: %s", to_string(space));
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

		strcpy(actionInfo.actionName, "jump_left_handed");
		strcpy(actionInfo.localizedActionName, "Jump (left handed)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.JumpLeftHandedAction));

		strcpy(actionInfo.actionName, "jump_right_handed");
		strcpy(actionInfo.localizedActionName, "Jump (right handed)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.JumpRightHandedAction));

		strcpy(actionInfo.actionName, "swim_down_left_handed");
		strcpy(actionInfo.localizedActionName, "Swim down (left handed)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.SwimDownLeftHandedAction));

		strcpy(actionInfo.actionName, "swim_down_right_handed");
		strcpy(actionInfo.localizedActionName, "Swim down (right handed)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.SwimDownRightHandedAction));

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

		strcpy(actionInfo.actionName, "menu_left_handed");
		strcpy(actionInfo.localizedActionName, "Menu (left handed)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.MenuLeftHandedAction));

		strcpy(actionInfo.actionName, "menu_right_handed");
		strcpy(actionInfo.localizedActionName, "Menu (right handed)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.MenuRightHandedAction));

		strcpy(actionInfo.actionName, "enter_trigger");
		strcpy(actionInfo.localizedActionName, "Enter (with triggers)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.EnterTriggerAction));

		strcpy(actionInfo.actionName, "enter_non_trigger_left_handed");
		strcpy(actionInfo.localizedActionName, "Enter (without triggers, left handed)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.EnterNonTriggerLeftHandedAction));

		strcpy(actionInfo.actionName, "enter_non_trigger_right_handed");
		strcpy(actionInfo.localizedActionName, "Enter (without triggers, right handed)");
		actionInfo.countSubactionPaths = 0;
		actionInfo.subactionPaths = nullptr;
		CHECK_XRCMD(xrCreateAction(appState.ActionSet, &actionInfo, &appState.EnterNonTriggerRightHandedAction));

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
				{ appState.Play1Action, xClick },
				{ appState.Play2Action, aClick },
				{ appState.JumpLeftHandedAction, yClick },
				{ appState.JumpRightHandedAction, bClick },
				{ appState.SwimDownLeftHandedAction, xClick },
				{ appState.SwimDownRightHandedAction, aClick },
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
				{ appState.MenuLeftHandedAction, aClick },
				{ appState.MenuRightHandedAction, xClick },
				{ appState.EnterTriggerAction, leftTrigger },
				{ appState.EnterTriggerAction, rightTrigger },
				{ appState.EnterNonTriggerLeftHandedAction, xClick },
				{ appState.EnterNonTriggerRightHandedAction, aClick },
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
		if (appState.CylinderCompositionLayerEnabled)
		{
			referenceSpaceCreateInfo.poseInReferenceSpace.position.y = -CylinderProjection::screenLowerLimit - CylinderProjection::keyboardLowerLimit;
		}
		else
		{
			referenceSpaceCreateInfo.poseInReferenceSpace.position.y = -PlanarProjection::screenLowerLimit - PlanarProjection::keyboardLowerLimit;
		}
		CHECK_XRCMD(xrCreateReferenceSpace(appState.Session, &referenceSpaceCreateInfo, &keyboardSpace));

		XrSpace consoleKeyboardSpace = XR_NULL_HANDLE;
		referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
		if (appState.CylinderCompositionLayerEnabled)
		{
			referenceSpaceCreateInfo.poseInReferenceSpace.position.y = -CylinderProjection::keyboardLowerLimit;
		}
		else
		{
			referenceSpaceCreateInfo.poseInReferenceSpace.position.y = -PlanarProjection::keyboardLowerLimit;
		}
		CHECK_XRCMD(xrCreateReferenceSpace(appState.Session, &referenceSpaceCreateInfo, &consoleKeyboardSpace));

		XrSystemProperties systemProperties { XR_TYPE_SYSTEM_PROPERTIES };

		void* chain = &systemProperties;

		XrSystemHandTrackingPropertiesEXT handTrackingProperties { XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT };
		if (handTrackingEnabled)
		{
			((XrBaseInStructure*)chain)->next = (XrBaseInStructure*)&handTrackingProperties;
			chain = (void*)((XrBaseInStructure*)chain)->next;
		}

		XrSystemSimultaneousHandsAndControllersPropertiesMETA simultaneousHandsAndControllersProperties { XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT };
		if (simultaneousHandsAndControllersEnabled)
		{
			((XrBaseInStructure*)chain)->next = (XrBaseInStructure*)&simultaneousHandsAndControllersProperties;
			chain = (void*)((XrBaseInStructure*)chain)->next;
		}

		CHECK_XRCMD(xrGetSystemProperties(instance, systemId, &systemProperties));

		appState.Logger->Info("System Properties: Name=%s VendorId=%d", systemProperties.systemName, systemProperties.vendorId);
		appState.Logger->Info("System Graphics Properties: MaxWidth=%d MaxHeight=%d MaxLayers=%d", systemProperties.graphicsProperties.maxSwapchainImageWidth, systemProperties.graphicsProperties.maxSwapchainImageHeight, systemProperties.graphicsProperties.maxLayerCount);
		appState.Logger->Info("System Tracking Properties: OrientationTracking=%s PositionTracking=%s", (systemProperties.trackingProperties.orientationTracking == XR_TRUE ? "True" : "False"), (systemProperties.trackingProperties.positionTracking == XR_TRUE ? "True" : "False"));
		appState.Logger->Info("Hand Tracking Properties: SupportsHandTracking=%s", (handTrackingProperties.supportsHandTracking == XR_TRUE ? "True" : "False"));
		appState.Logger->Info("Simultaneous Hand and Controllers Properties: SupportsSimultaneousHandsAndControllers=%s", (simultaneousHandsAndControllersProperties.supportsSimultaneousHandsAndControllers == XR_TRUE ? "True" : "False"));

		appState.HandTrackingEnabled = (handTrackingProperties.supportsHandTracking == XR_TRUE);
		appState.SimultaneousHandsAndControllersEnabled = (simultaneousHandsAndControllersProperties.supportsSimultaneousHandsAndControllers == XR_TRUE);

		PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT = nullptr;
		PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT = nullptr;
		if (appState.HandTrackingEnabled)
		{
			PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT = nullptr;
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrCreateHandTrackerEXT", reinterpret_cast<PFN_xrVoidFunction*>(&xrCreateHandTrackerEXT)));
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrLocateHandJointsEXT", reinterpret_cast<PFN_xrVoidFunction*>(&xrLocateHandJointsEXT)));
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrDestroyHandTrackerEXT", reinterpret_cast<PFN_xrVoidFunction*>(&xrDestroyHandTrackerEXT)));

			appState.HandTrackers.resize(2);

			XrHandTrackerCreateInfoEXT handTrackerCreateInfo { XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
			handTrackerCreateInfo.hand = XR_HAND_LEFT_EXT;
			CHECK_XRCMD(xrCreateHandTrackerEXT(appState.Session, &handTrackerCreateInfo, &appState.HandTrackers[LEFT_TRACKED_HAND].tracker));

			handTrackerCreateInfo.hand = XR_HAND_RIGHT_EXT;
			CHECK_XRCMD(xrCreateHandTrackerEXT(appState.Session, &handTrackerCreateInfo, &appState.HandTrackers[RIGHT_TRACKED_HAND].tracker));

			appState.HandTrackers[LEFT_TRACKED_HAND].locations.type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
			appState.HandTrackers[LEFT_TRACKED_HAND].locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
			appState.HandTrackers[LEFT_TRACKED_HAND].locations.jointLocations = appState.HandTrackers[LEFT_TRACKED_HAND].jointLocations;

			appState.HandTrackers[RIGHT_TRACKED_HAND].locations.type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
			appState.HandTrackers[RIGHT_TRACKED_HAND].locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
			appState.HandTrackers[RIGHT_TRACKED_HAND].locations.jointLocations = appState.HandTrackers[RIGHT_TRACKED_HAND].jointLocations;
		}

		PFN_xrResumeSimultaneousHandsAndControllersTrackingMETA xrResumeSimultaneousHandsAndControllersTrackingMETA = nullptr;
		PFN_xrPauseSimultaneousHandsAndControllersTrackingMETA xrPauseSimultaneousHandsAndControllersTrackingMETA = nullptr;
		if (appState.HandTrackingEnabled && appState.SimultaneousHandsAndControllersEnabled)
		{
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrResumeSimultaneousHandsAndControllersTrackingMETA", reinterpret_cast<PFN_xrVoidFunction*>(&xrResumeSimultaneousHandsAndControllersTrackingMETA)));
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrPauseSimultaneousHandsAndControllersTrackingMETA", reinterpret_cast<PFN_xrVoidFunction*>(&xrPauseSimultaneousHandsAndControllersTrackingMETA)));

			XrSimultaneousHandsAndControllersTrackingResumeInfoMETA resumeInfo { XR_TYPE_SIMULTANEOUS_HANDS_AND_CONTROLLERS_TRACKING_RESUME_INFO_META };
			CHECK_XRCMD(xrResumeSimultaneousHandsAndControllersTrackingMETA(appState.Session, &resumeInfo));
		}

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
		appState.Logger->Verbose("Swapchain Formats: %s", swapchainFormatsString.c_str());
		if (!found)
		{
			THROW(Fmt("No runtime swapchain format supported for color swapchain %i", Constants::colorFormat));
		}

		appState.SwapchainRect.extent.width = configViews[0].recommendedImageRectWidth;
		appState.SwapchainRect.extent.height = configViews[0].recommendedImageRectHeight;
		appState.SwapchainSampleCount = vulkanSwapchainSampleCount;

		appState.Logger->Info("Creating swapchain with dimensions Width=%d Height=%d", appState.SwapchainRect.extent.width, appState.SwapchainRect.extent.height);

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

		XrSwapchain depthSwapchain = VK_NULL_HANDLE;
		if (createRenderPass2)
		{
			if (depthStencilResolve && depthCompositionLayerEnabled)
			{
				appState.Logger->Info("Creating depth swapchain with dimensions Width=%d Height=%d", appState.SwapchainRect.extent.width, appState.SwapchainRect.extent.height);

				swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				swapchainCreateInfo.format = Constants::depthFormat;
				CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &depthSwapchain));

				uint32_t depthImageCount;
				CHECK_XRCMD(xrEnumerateSwapchainImages(depthSwapchain, 0, &depthImageCount, nullptr));

				appState.DepthSwapchainImages.resize(imageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
				CHECK_XRCMD(xrEnumerateSwapchainImages(depthSwapchain, depthImageCount, &depthImageCount, (XrSwapchainImageBaseHeader*)appState.DepthSwapchainImages.data()));

				appState.DepthSwapchainImageViews.resize(imageCount);
			}

			VkAttachmentReference2 colorReference { VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2 };
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference2 depthReference { VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2 };
			depthReference.attachment = 1;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference2 resolveReference { VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2 };
			resolveReference.attachment = 2;
			resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentReference2 resolveDepthReference { VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2 };
			resolveDepthReference.attachment = 3;
			resolveDepthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkSubpassDescriptionDepthStencilResolve depthStencilResolveForSubpass { VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE };
			depthStencilResolveForSubpass.depthResolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
			depthStencilResolveForSubpass.stencilResolveMode = VK_RESOLVE_MODE_NONE;
			depthStencilResolveForSubpass.pDepthStencilResolveAttachment = &resolveDepthReference;

			const uint32_t viewMask = 3;

			VkSubpassDescription2 subpass { VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2 };
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorReference;
			subpass.pDepthStencilAttachment = &depthReference;
			subpass.pResolveAttachments = &resolveReference;
			subpass.viewMask = viewMask;
			if (depthStencilResolve && depthCompositionLayerEnabled)
			{
				subpass.pNext = &depthStencilResolveForSubpass;
			}

			VkAttachmentDescription2 attachments[4] { };

			attachments[0].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			attachments[0].format = Constants::colorFormat;
			attachments[0].samples = (VkSampleCountFlagBits)appState.SwapchainSampleCount;
			attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			attachments[1].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			attachments[1].format = Constants::depthFormat;
			attachments[1].samples = (VkSampleCountFlagBits)appState.SwapchainSampleCount;
			attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			attachments[2].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			attachments[2].format = Constants::colorFormat;
			attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			attachments[3].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
			attachments[3].format = Constants::depthFormat;
			attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
			attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkRenderPassCreateInfo2 renderPassInfo { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2 };
			renderPassInfo.attachmentCount = (depthStencilResolve && depthCompositionLayerEnabled ? 4 : 3);
			renderPassInfo.pAttachments = attachments;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.pCorrelatedViewMasks = &viewMask;
			renderPassInfo.correlatedViewMaskCount = 1;

			auto vkCreateRenderPass2KHR = (PFN_vkCreateRenderPass2KHR)vkGetDeviceProcAddr(appState.Device, "vkCreateRenderPass2KHR");

			CHECK_VKCMD(vkCreateRenderPass2KHR(appState.Device, &renderPassInfo, nullptr, &appState.RenderPass));
		}
		else
		{
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
		}

		VkClearValue clearValues[2] { };
		clearValues[1].depthStencil.depth = 1.0f;

		VkRenderPassBeginInfo renderPassBeginInfo { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearValues;
		renderPassBeginInfo.renderPass = appState.RenderPass;
		renderPassBeginInfo.renderArea.extent = appState.SwapchainRect.extent;

		std::vector<XrCompositionLayerBaseHeader*> layers;

		XrCompositionLayerProjection worldLayer { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
		XrCompositionLayerCylinderKHR screenCylinderLayer { XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR };
		XrCompositionLayerQuad screenPlanarLayer { XR_TYPE_COMPOSITION_LAYER_QUAD };
		XrCompositionLayerCylinderKHR leftArrowsCylinderLayer { XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR };
		XrCompositionLayerQuad leftArrowsPlanarLayer { XR_TYPE_COMPOSITION_LAYER_QUAD };
		XrCompositionLayerCylinderKHR rightArrowsCylinderLayer { XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR };
		XrCompositionLayerQuad rightArrowsPlanarLayer { XR_TYPE_COMPOSITION_LAYER_QUAD };
		XrCompositionLayerCylinderKHR keyboardCylinderLayer { XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR };
		XrCompositionLayerQuad keyboardPlanarLayer { XR_TYPE_COMPOSITION_LAYER_QUAD };

		std::vector<XrCompositionLayerProjectionView> projectionLayerViews;
		std::vector<XrCompositionLayerDepthInfoKHR> depthInfoForProjectionLayerViews;

		XrCompositionLayerCubeKHR skyboxLayer { XR_TYPE_COMPOSITION_LAYER_CUBE_KHR };
		skyboxLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;

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

		appState.VertexTransform.m[15] = 1;

#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
		appState.vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(vulkanInstance, "vkSetDebugUtilsObjectNameEXT");
		appState.vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(vulkanInstance, "vkCmdBeginDebugUtilsLabelEXT");
		appState.vkCmdInsertDebugUtilsLabelEXT = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr(vulkanInstance, "vkCmdInsertDebugUtilsLabelEXT");
		appState.vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(vulkanInstance, "vkCmdEndDebugUtilsLabelEXT");
#endif

		VmaVulkanFunctions vulkanFunctions { };
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorCreateInfo { };
		if (maintenance4) allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
		if (bufferDeviceAddress) allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_1;
		allocatorCreateInfo.physicalDevice = vulkanPhysicalDevice;
		allocatorCreateInfo.device = appState.Device;
		allocatorCreateInfo.instance = vulkanInstance;
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		CHECK_VKCMD(vmaCreateAllocator(&allocatorCreateInfo, &appState.Allocator));

		if (audioDeviceGuidEnabled)
		{
			PFN_xrGetAudioOutputDeviceGuidOculus xrGetAudioOutputDeviceGuidOculus = nullptr;
			CHECK_XRCMD(xrGetInstanceProcAddr(instance, "xrGetAudioOutputDeviceGuidOculus", reinterpret_cast<PFN_xrVoidFunction*>(&xrGetAudioOutputDeviceGuidOculus)));

			CHECK_XRCMD(xrGetAudioOutputDeviceGuidOculus(instance, snd_audio_output_device_id));
		}

		auto sessionState = XR_SESSION_STATE_UNKNOWN;
		auto sessionRunning = false;

		auto exitRenderLoop = false;
		auto requestRestart = false;

		do
		{
			while (const XrEventDataBaseHeader* event = TryReadNextEvent(eventDataBuffer, instance))
			{
				switch (event->type)
				{
					case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
					{
						const auto& instanceLossPending = *reinterpret_cast<const XrEventDataInstanceLossPending*>(event);
						appState.Logger->Warn("XrEventDataInstanceLossPending by %ld", instanceLossPending.lossTime);
						exitRenderLoop = true;
						requestRestart = true;
						break;
					}
					case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
					{
						auto sessionStateChangedEvent = *reinterpret_cast<const XrEventDataSessionStateChanged*>(event);
						const XrSessionState oldState = sessionState;
						sessionState = sessionStateChangedEvent.state;

						appState.Logger->Info("XrEventDataSessionStateChanged: state %s->%s session=%p time=%ld", to_string(oldState), to_string(sessionState), sessionStateChangedEvent.session, sessionStateChangedEvent.time);

						if ((sessionStateChangedEvent.session != XR_NULL_HANDLE) && (sessionStateChangedEvent.session != appState.Session))
						{
							appState.Logger->Error("XrEventDataSessionStateChanged for unknown session");
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
								break;
							}
							case XR_SESSION_STATE_STOPPING:
							{
								CHECK(appState.Session != XR_NULL_HANDLE);
								sessionRunning = false;
								exitRenderLoop = true;
								CHECK_XRCMD(xrEndSession(appState.Session));
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
							{
								appState.Logger->Error("Ignoring session state %d", sessionState);
							}
						}
						break;
					}
					case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
					{
						const auto& perfSettingsEvent = *reinterpret_cast<const XrEventDataPerfSettingsEXT*>(event);
						appState.Logger->Verbose("XrEventDataPerfSettingsEXT: type %d subdomain %d : level %d -> level %d", perfSettingsEvent.type, perfSettingsEvent.subDomain, perfSettingsEvent.fromLevel, perfSettingsEvent.toLevel);
						break;
					}
					case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB:
					{
						const auto& refreshRateChangedEvent = *reinterpret_cast<const XrEventDataDisplayRefreshRateChangedFB*>(event);
						appState.Logger->Verbose("XrEventDataDisplayRefreshRateChangedFB: fromRate %f -> toRate %f", refreshRateChangedEvent.fromDisplayRefreshRate, refreshRateChangedEvent.toDisplayRefreshRate);
						break;
					}
					case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
					{
						SwitchBoundInput(appState, appState.Play1Action, "Play 1");
						SwitchBoundInput(appState, appState.Play2Action, "Play 2");
						SwitchBoundInput(appState, appState.JumpLeftHandedAction, "Jump (left handed)");
						SwitchBoundInput(appState, appState.JumpRightHandedAction, "Jump (right handed)");
						SwitchBoundInput(appState, appState.SwimDownLeftHandedAction, "Swim down (left handed)");
						SwitchBoundInput(appState, appState.SwimDownRightHandedAction, "Swim down (right handed)");
						SwitchBoundInput(appState, appState.RunAction, "Run");
						SwitchBoundInput(appState, appState.FireAction, "Fire");
						SwitchBoundInput(appState, appState.MoveXAction, "Move X");
						SwitchBoundInput(appState, appState.MoveYAction, "Move Y");
						SwitchBoundInput(appState, appState.SwitchWeaponAction, "Switch weapon");
						SwitchBoundInput(appState, appState.MenuAction, "Menu");
						SwitchBoundInput(appState, appState.MenuLeftHandedAction, "Menu (left handed)");
						SwitchBoundInput(appState, appState.MenuRightHandedAction, "Menu (right handed)");
						SwitchBoundInput(appState, appState.EnterTriggerAction, "Enter (with triggers)");
						SwitchBoundInput(appState, appState.EnterNonTriggerLeftHandedAction, "Enter (without triggers, left handed)");
						SwitchBoundInput(appState, appState.EnterNonTriggerRightHandedAction, "Enter (without triggers, right handed)");
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
						appState.Logger->Verbose("XrEventDataReferenceSpaceChangePending: changed space: %d for session %p at time %ld", referenceSpaceChangeEvent.referenceSpaceType, (void*)referenceSpaceChangeEvent.session, referenceSpaceChangeEvent.changeTime);
						break;
					}
					default:
					{
						appState.Logger->Verbose("Ignoring event type %d", event->type);
					}
				}

				if (exitRenderLoop || requestRestart)
				{
					break;
				}
			}

			if ((sys_errorcalled || sys_quitcalled) && sessionRunning)
			{
				xrRequestExitSession(appState.Session);
				sessionRunning = false;
			}

			if (!sessionRunning)
			{
				std::this_thread::yield();
				continue;
			}

			const XrActiveActionSet activeActionSet { appState.ActionSet, XR_NULL_PATH };
			XrActionsSyncInfo syncInfo { XR_TYPE_ACTIONS_SYNC_INFO };
			syncInfo.countActiveActionSets = 1;
			syncInfo.activeActionSets = &activeActionSet;
			CHECK_XRCMD(xrSyncActions(appState.Session, &syncInfo));

			auto keyPressHandled = appState.Keyboard.Handle(appState);
			AppInput::Handle(appState, keyPressHandled);

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

			if (appState.EngineThreadCreated && !appState.EngineThreadStarted)
			{
				if (snd_initialized)
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

				appState.EngineThread = std::thread(runEngine, &appState);
				appState.EngineThreadStarted = true;
			}

			{
				std::lock_guard<std::mutex> lock(Locks::ModeChangeMutex);
				if (appState.Mode == AppScreenMode || appState.Mode == AppWorldMode)
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
						sys_version = "PCXR 1.1.32";
						sys_argc = argc;
						sys_argv = argv;
						cl_bobdisabled.default_value = "1";
						Sys_Init(sys_argc, sys_argv);
						if (sys_errorcalled)
						{
							xrRequestExitSession(appState.Session);
							sessionRunning = false;
							std::this_thread::yield();
							continue;
						}
						appState.DefaultFOV = (int)Cvar_VariableValue("fov");
						r_load_as_rgba = true;
						d_skipfade = true;
						appState.EngineThread = std::thread(runEngine, &appState);
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
						AppInput::AddKeyInput(K_DOWNARROW, false);
						AppInput::AddKeyInput(K_UPARROW, false);
						AppInput::AddKeyInput(K_LEFTARROW, false);
						AppInput::AddKeyInput(K_RIGHTARROW, false);

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
					}
					appState.PreviousMode = appState.Mode;
				}
			}

			{
				std::lock_guard<std::mutex> lock(Locks::RenderMutex);

				appState.Scene.textures.DeleteOld(appState);
				for (auto& entry : appState.Scene.surfaceRGBATextures)
				{
					entry.DeleteOld(appState);
				}
				for (auto& entry : appState.Scene.surfaceTextures)
				{
					entry.DeleteOld(appState);
				}
				appState.Scene.lightmapsRGBToDelete.DeleteOld(appState);
				appState.Scene.lightmapsToDelete.DeleteOld(appState);
				appState.Scene.indexBuffers.DeleteOld(appState);
				appState.Scene.aliasBuffers.DeleteOld(appState);

				Skybox::DeleteOld(appState);

				for (auto& entry : appState.Scene.perSurfaceCache)
				{
					if (entry.second.frameCount != appState.Scene.frameCount)
					{
						if (entry.second.lightmap != nullptr)
						{
							appState.Scene.lightmapsToDelete.Dispose(entry.second.lightmap);
							entry.second.lightmap = nullptr;
						}
						if (entry.second.lightmapRGB != nullptr)
						{
							appState.Scene.lightmapsRGBToDelete.Dispose(entry.second.lightmapRGB);
							entry.second.lightmapRGB = nullptr;
						}
					}
				}
			}

			XrFrameWaitInfo frameWaitInfo { XR_TYPE_FRAME_WAIT_INFO };
			XrFrameState frameState { XR_TYPE_FRAME_STATE };
			CHECK_XRCMD(xrWaitFrame(appState.Session, &frameWaitInfo, &frameState));

			uint32_t swapchainImageIndex;
			CHECK_XRCMD(xrAcquireSwapchainImage(swapchain, nullptr, &swapchainImageIndex));

			uint32_t depthSwapchainImageIndex;
			if (depthSwapchain != VK_NULL_HANDLE)
			{
				CHECK_XRCMD(xrAcquireSwapchainImage(depthSwapchain, nullptr, &depthSwapchainImageIndex));
				if (depthSwapchainImageIndex != swapchainImageIndex)
				{
					appState.Logger->Warn("Image indices for swapchain and depth swapchain do not match: %i vs %i", swapchainImageIndex, depthSwapchainImageIndex);
				}
			}

			auto& perFrame = appState.PerFrame[swapchainImageIndex];

			if (perFrame.fence == VK_NULL_HANDLE)
			{
				VkFenceCreateInfo fenceCreate{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
				CHECK_VKCMD(vkCreateFence(appState.Device, &fenceCreate, nullptr, &perFrame.fence));
			}
			else
			{
				uint64_t timeout = UINT64_MAX;
				CHECK_VKCMD(vkWaitForFences(appState.Device, 1, &perFrame.fence, VK_TRUE, timeout));
			}

			XrFrameBeginInfo frameBeginInfo { XR_TYPE_FRAME_BEGIN_INFO };
			CHECK_XRCMD(xrBeginFrame(appState.Session, &frameBeginInfo));

			CHECK_VKCMD(vkResetFences(appState.Device, 1, &perFrame.fence));

			CHECK_XRCMD(xrWaitSwapchainImage(swapchain, &waitInfo));
			if (depthSwapchain != VK_NULL_HANDLE)
			{
				CHECK_XRCMD(xrWaitSwapchainImage(depthSwapchain, &waitInfo));
			}

			layers.clear();

			auto screenRendered = false;
			auto keyboardRendered = false;

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

			if (frameState.shouldRender)
			{
				XrViewState viewState { XR_TYPE_VIEW_STATE };
				auto viewCapacityInput = (uint32_t)views.size();
				uint32_t viewCountOutput;

				XrViewLocateInfo viewLocateInfo { XR_TYPE_VIEW_LOCATE_INFO };
				viewLocateInfo.viewConfigurationType = viewConfigurationType;
				viewLocateInfo.displayTime = frameState.predictedDisplayTime;
				viewLocateInfo.space = appSpace;

				auto res = xrLocateViews(appState.Session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, views.data());
				CHECK_XRRESULT(res, "xrLocateViews");
				if ((viewState.viewStateFlags & (XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT)) == (XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT))
				{
					CHECK(viewCountOutput == viewCapacityInput);
					CHECK(viewCountOutput == configViews.size());

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
						XrMatrix4x4f_CreateProjectionFov(&appState.ProjectionMatrices[i], GRAPHICS_VULKAN, views[i].fov, Constants::nearPlaneForProjection, Constants::farPlaneForProjection);
					}

					{
						std::lock_guard<std::mutex> lock(Locks::RenderInputMutex);

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
							appState.Logger->Verbose("Unable to locate left hand action space in app space: %d", res);
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
							appState.Logger->Verbose("Unable to locate right hand action space in app space: %d", res);
						}

						appState.CameraLocation.type = XR_TYPE_SPACE_LOCATION;
						res = xrLocateSpace(screenSpace, appSpace, frameState.predictedDisplayTime, &appState.CameraLocation);
						CHECK_XRRESULT(res, "xrLocateSpace(screenSpace, appSpace)");
						appState.CameraLocationIsValid = (XR_UNQUALIFIED_SUCCESS(res) && (appState.CameraLocation.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) == (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT));

						if (appState.CameraLocationIsValid)
						{
							AppState::AnglesFromQuaternion(appState.CameraLocation.pose.orientation, appState.Yaw, appState.Pitch, appState.Roll);
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

					if (appState.HandTrackingEnabled)
					{
						XrHandJointsLocateInfoEXT locateInfo { XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT };
						locateInfo.baseSpace = appSpace;
						locateInfo.time = frameState.predictedDisplayTime;

						CHECK_XRCMD(xrLocateHandJointsEXT(appState.HandTrackers[LEFT_TRACKED_HAND].tracker, &locateInfo, &appState.HandTrackers[LEFT_TRACKED_HAND].locations));
						CHECK_XRCMD(xrLocateHandJointsEXT(appState.HandTrackers[RIGHT_TRACKED_HAND].tracker, &locateInfo, &appState.HandTrackers[RIGHT_TRACKED_HAND].locations));
					}

					projectionLayerViews.resize(viewCountOutput);
					depthInfoForProjectionLayerViews.resize(viewCountOutput);

					if (perFrame.matrices == nullptr)
					{
						perFrame.matrices = new Buffer();
						perFrame.matrices->CreateMappableUniformBuffer(appState, (2 * 2 + 1) * sizeof(XrMatrix4x4f));
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

						if (depthSwapchain != VK_NULL_HANDLE)
						{
							projectionLayerViews[i].next = &depthInfoForProjectionLayerViews[i];

							depthInfoForProjectionLayerViews[i].type = XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR;
							depthInfoForProjectionLayerViews[i].subImage.swapchain = depthSwapchain;
							depthInfoForProjectionLayerViews[i].subImage.imageRect.extent.width = appState.SwapchainRect.extent.width;
							depthInfoForProjectionLayerViews[i].subImage.imageRect.extent.height = appState.SwapchainRect.extent.height;
							depthInfoForProjectionLayerViews[i].subImage.imageArrayIndex = i;
							depthInfoForProjectionLayerViews[i].maxDepth = 1;
							depthInfoForProjectionLayerViews[i].nearZ = Constants::nearPlaneForProjection;
							depthInfoForProjectionLayerViews[i].farZ = Constants::farPlaneForDepthCompositionLayer;
						}
					}

					float clearR = 0;
					float clearG = 0;
					float clearB = 0;
					float clearA = 1;

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
							clearR = (float)(color & 255) / 255.0f;
							clearG = (float)(color >> 8 & 255) / 255.0f;
							clearB = (float)(color >> 16 & 255) / 255.0f;
							clearA = (float)(color >> 24) / 255.0f;
						}

						if (appState.Mode == AppScreenMode || appState.Mode == AppWorldMode)
						{
							std::copy(d_8to24table, d_8to24table + 256, appState.Scene.paletteData);
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

						stagingBufferSize = appState.Scene.GetStagingBufferSize(appState, perFrame);

						stagingBufferSize = ((stagingBufferSize >> 19) + 1) << 19;
						stagingBuffer = perFrame.stagingBuffers.GetStagingBuffer(appState, stagingBufferSize);
						stagingBuffer->Map(appState);
						perFrame.LoadStagingBuffer(appState, stagingBuffer);
						stagingBuffer->UnmapAndFlush(appState);

						perFrame.LoadNonStagedResources(appState);
					}

					if (perFrame.commandBuffer == VK_NULL_HANDLE)
					{
						CHECK_VKCMD(vkCreateCommandPool(appState.Device, &commandPoolCreateInfo, nullptr, &perFrame.commandPool));

						commandBufferAllocateInfo.commandPool = perFrame.commandPool;
						CHECK_VKCMD(vkAllocateCommandBuffers(appState.Device, &commandBufferAllocateInfo, &perFrame.commandBuffer));
					}

					commandBuffer = perFrame.commandBuffer;

					CHECK_VKCMD(vkResetCommandPool(appState.Device, perFrame.commandPool, 0));

					CHECK_VKCMD(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

					perFrame.FillFromStagingBuffer(appState, stagingBuffer, swapchainImageIndex);

					clearValues[0].color.float32[0] = clearR;
					clearValues[0].color.float32[1] = clearG;
					clearValues[0].color.float32[2] = clearB;
					clearValues[0].color.float32[3] = clearA;

					if (perFrame.framebuffer == VK_NULL_HANDLE)
					{
						VkImageCreateInfo imageCreateInfo { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
						imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
						imageCreateInfo.extent.width = appState.SwapchainRect.extent.width;
						imageCreateInfo.extent.height = appState.SwapchainRect.extent.height;
						imageCreateInfo.extent.depth = 1;
						imageCreateInfo.mipLevels = 1;
						imageCreateInfo.arrayLayers = viewCount;
						imageCreateInfo.samples = (VkSampleCountFlagBits)appState.SwapchainSampleCount;

						imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
						imageCreateInfo.format = Constants::colorFormat;

						VmaAllocationCreateInfo allocInfo { };
						allocInfo.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;

						auto createRes = vmaCreateImage(appState.Allocator, &imageCreateInfo, &allocInfo, &perFrame.colorImage, &perFrame.colorAllocation, nullptr);
						if (createRes != VK_SUCCESS)
						{
							allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
							allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
							CHECK_VKCMD(vmaCreateImage(appState.Allocator, &imageCreateInfo, &allocInfo, &perFrame.colorImage, &perFrame.colorAllocation, nullptr));
							allocInfo.flags = 0;
							allocInfo.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
						}

						imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
						imageCreateInfo.format = Constants::depthFormat;

						createRes = vmaCreateImage(appState.Allocator, &imageCreateInfo, &allocInfo, &perFrame.depthImage, &perFrame.depthAllocation, nullptr);
						if (createRes != VK_SUCCESS)
						{
							allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
							allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
							CHECK_VKCMD(vmaCreateImage(appState.Allocator, &imageCreateInfo, &allocInfo, &perFrame.depthImage, &perFrame.depthAllocation, nullptr));
						}

						perFrame.resolveImage = appState.SwapchainImages[swapchainImageIndex].image;

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

						if (depthSwapchain != VK_NULL_HANDLE)
						{
							viewInfo.image = appState.DepthSwapchainImages[depthSwapchainImageIndex].image;
							viewInfo.format = Constants::depthFormat;
							viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
							CHECK_VKCMD(vkCreateImageView(appState.Device, &viewInfo, nullptr, &appState.DepthSwapchainImageViews[depthSwapchainImageIndex]));
						}

						VkImageView attachments[4];
						attachments[0] = perFrame.colorView;
						attachments[1] = perFrame.depthView;
						attachments[2] = perFrame.resolveView;
						if (depthSwapchain != VK_NULL_HANDLE)
						{
							attachments[3] = appState.DepthSwapchainImageViews[depthSwapchainImageIndex];
						}

						VkFramebufferCreateInfo framebufferInfo { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
						framebufferInfo.renderPass = appState.RenderPass;
						framebufferInfo.attachmentCount = (depthSwapchain != VK_NULL_HANDLE ? 4 : 3);
						framebufferInfo.pAttachments = attachments;
						framebufferInfo.width = appState.SwapchainRect.extent.width;
						framebufferInfo.height = appState.SwapchainRect.extent.height;
						framebufferInfo.layers = 1;
						CHECK_VKCMD(vkCreateFramebuffer(appState.Device, &framebufferInfo, nullptr, &perFrame.framebuffer));
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
						worldLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
					}
					else
					{
						worldLayer.layerFlags = 0;
					}

					layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&worldLayer));

					CHECK_XRCMD(xrAcquireSwapchainImage(appState.Screen.swapchain, nullptr, &swapchainImageIndex));
					CHECK_XRCMD(xrWaitSwapchainImage(appState.Screen.swapchain, &waitInfo));

					auto& screenPerFrame = appState.Screen.perFrame[swapchainImageIndex];

					if (screenPerFrame.image == VK_NULL_HANDLE)
					{
						screenPerFrame.image = appState.Screen.swapchainImages[swapchainImageIndex].image;
					}

					if (screenPerFrame.stagingBuffer.buffer == VK_NULL_HANDLE)
					{
						screenPerFrame.stagingBuffer.CreateStagingBuffer(appState, appState.ScreenData.size() * sizeof(uint32_t));
					}

					appState.RenderScreen(screenPerFrame);

					appState.copyBarrier.image = screenPerFrame.image;
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.copyBarrier);

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
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.submitBarrier);

					if (appState.CylinderCompositionLayerEnabled)
					{
						screenCylinderLayer.radius = CylinderProjection::radius;
						screenCylinderLayer.aspectRatio = (float)appState.ScreenWidth / (float)appState.ScreenHeight;
						screenCylinderLayer.centralAngle = CylinderProjection::horizontalAngle;
						screenCylinderLayer.subImage.swapchain = appState.Screen.swapchain;
						screenCylinderLayer.subImage.imageRect.extent.width = appState.ScreenWidth;
						screenCylinderLayer.subImage.imageRect.extent.height = appState.ScreenHeight;
						screenCylinderLayer.space = appSpace;

						if (appState.Mode == AppWorldMode)
						{
							if (appState.CameraLocationIsValid)
							{
								screenCylinderLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
								screenCylinderLayer.pose = appState.CameraLocation.pose;

								layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&screenCylinderLayer));
							}
						}
						else
						{
							screenCylinderLayer.layerFlags = 0;
							screenCylinderLayer.pose = { };
							screenCylinderLayer.pose.orientation.w = 1;

							layers.insert(layers.begin(), reinterpret_cast<XrCompositionLayerBaseHeader*>(&screenCylinderLayer));
						}
					}
					else
					{
						screenPlanarLayer.size.width = PlanarProjection::distance;
						screenPlanarLayer.size.height = PlanarProjection::distance * (float)appState.ScreenHeight / (float)appState.ScreenWidth;
						screenPlanarLayer.subImage.swapchain = appState.Screen.swapchain;
						screenPlanarLayer.subImage.imageRect.extent.width = appState.ScreenWidth;
						screenPlanarLayer.subImage.imageRect.extent.height = appState.ScreenHeight;
						screenPlanarLayer.space = appSpace;

						if (appState.Mode == AppWorldMode)
						{
							if (appState.CameraLocationIsValid)
							{
								screenPlanarLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
								screenPlanarLayer.pose = appState.CameraLocation.pose;

								XrMatrix4x4f rotation;
								XrMatrix4x4f_CreateFromQuaternion(&rotation, &screenPlanarLayer.pose.orientation);

								XrVector3f translation { 0, 0, -PlanarProjection::distance };
								XrVector3f translated;
								XrMatrix4x4f_TransformVector3f(&translated, &rotation, &translation);

								XrVector3f position;
								XrVector3f_Add(&position, &screenPlanarLayer.pose.position, &translated);

								screenPlanarLayer.pose.position = position;

								layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&screenPlanarLayer));
							}
						}
						else
						{
							screenPlanarLayer.layerFlags = 0;
							screenPlanarLayer.pose = { };
							screenPlanarLayer.pose.position.z = -PlanarProjection::distance;
							screenPlanarLayer.pose.orientation.w = 1;

							layers.insert(layers.begin(), reinterpret_cast<XrCompositionLayerBaseHeader*>(&screenPlanarLayer));
						}
					}

					screenRendered = true;

					if (appState.Keyboard.Draw(appState))
					{
						CHECK_XRCMD(xrAcquireSwapchainImage(appState.Keyboard.Screen.swapchain, nullptr, &swapchainImageIndex));
						CHECK_XRCMD(xrWaitSwapchainImage(appState.Keyboard.Screen.swapchain, &waitInfo));

						auto& keyboardPerFrame = appState.Keyboard.Screen.perFrame[swapchainImageIndex];

						if (keyboardPerFrame.image == VK_NULL_HANDLE)
						{
							keyboardPerFrame.image = appState.Keyboard.Screen.swapchainImages[swapchainImageIndex].image;
						}

						if (keyboardPerFrame.stagingBuffer.buffer == VK_NULL_HANDLE)
						{
							keyboardPerFrame.stagingBuffer.CreateStagingBuffer(appState, appState.ConsoleWidth * appState.ConsoleHeight / 2 * sizeof(uint32_t));
						}

						auto& keyboardTexture = appState.KeyboardTextures[swapchainImageIndex];
						if (keyboardTexture.image == VK_NULL_HANDLE)
						{
							keyboardTexture.Create(appState, appState.ConsoleWidth, appState.ConsoleHeight / 2, Constants::colorFormat, 1, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, false);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
							while (appState.KeyboardTextureNames.size() <= swapchainImageIndex)
							{
								appState.KeyboardTextureNames.push_back("");
							}
							appState.KeyboardTextureNames[swapchainImageIndex] = "Keyboard texture " + std::to_string(swapchainImageIndex);
							VkDebugUtilsObjectNameInfoEXT textureName { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
							textureName.objectType = VK_OBJECT_TYPE_IMAGE;
							textureName.objectHandle = (uint64_t)keyboardTexture.image;
							textureName.pObjectName = appState.KeyboardTextureNames[swapchainImageIndex].c_str();
							CHECK_VKCMD(appState.vkSetDebugUtilsObjectNameEXT(appState.Device, &textureName));
#endif
						}

						appState.RenderKeyboard(keyboardPerFrame);

						appState.copyBarrier.image = keyboardPerFrame.image;
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.copyBarrier);

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
						vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.submitBarrier);

						if (appState.CylinderCompositionLayerEnabled)
						{
							keyboardCylinderLayer.radius = CylinderProjection::radius;
							keyboardCylinderLayer.aspectRatio = (float)appState.ScreenWidth / ((float)appState.ScreenHeight / 2);
							keyboardCylinderLayer.centralAngle = CylinderProjection::horizontalAngle;
							keyboardCylinderLayer.subImage.swapchain = appState.Keyboard.Screen.swapchain;
							keyboardCylinderLayer.subImage.imageRect.extent.width = appState.ScreenWidth;
							keyboardCylinderLayer.subImage.imageRect.extent.height = appState.ScreenHeight / 2;
							keyboardCylinderLayer.space = appSpace;

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
									keyboardCylinderLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
									keyboardCylinderLayer.pose = keyboardLocation.pose;

									layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&keyboardCylinderLayer));
								}
							}
							else
							{
								keyboardCylinderLayer.layerFlags = 0;
								keyboardCylinderLayer.pose = { };
								keyboardCylinderLayer.pose.position.y = -CylinderProjection::screenLowerLimit - CylinderProjection::keyboardLowerLimit;
								keyboardCylinderLayer.pose.orientation.w = 1;

								layers.insert(layers.begin() + 1, reinterpret_cast<XrCompositionLayerBaseHeader*>(&keyboardCylinderLayer));
							}
						}
						else
						{
							keyboardPlanarLayer.size.width = PlanarProjection::distance;
							keyboardPlanarLayer.size.height = PlanarProjection::distance * (float)appState.ScreenHeight / (float)appState.ScreenWidth / 2;
							keyboardPlanarLayer.subImage.swapchain = appState.Keyboard.Screen.swapchain;
							keyboardPlanarLayer.subImage.imageRect.extent.width = appState.ScreenWidth;
							keyboardPlanarLayer.subImage.imageRect.extent.height = appState.ScreenHeight / 2;
							keyboardPlanarLayer.space = appSpace;

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
									keyboardPlanarLayer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_UNPREMULTIPLIED_ALPHA_BIT;
									keyboardPlanarLayer.pose = keyboardLocation.pose;

									XrMatrix4x4f rotation;
									XrMatrix4x4f_CreateFromQuaternion(&rotation, &keyboardPlanarLayer.pose.orientation);

									XrVector3f translation { 0, 0, -PlanarProjection::distance };
									XrVector3f translated;
									XrMatrix4x4f_TransformVector3f(&translated, &rotation, &translation);

									XrVector3f position;
									XrVector3f_Add(&position, &keyboardPlanarLayer.pose.position, &translated);

									keyboardPlanarLayer.pose.position = position;

									layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader*>(&keyboardPlanarLayer));
								}
							}
							else
							{
								keyboardPlanarLayer.layerFlags = 0;
								keyboardPlanarLayer.pose = { };
								keyboardPlanarLayer.pose.position.y = -PlanarProjection::screenLowerLimit - PlanarProjection::keyboardLowerLimit;
								keyboardPlanarLayer.pose.position.z = -PlanarProjection::distance;
								keyboardPlanarLayer.pose.orientation.w = 1;

								layers.insert(layers.begin() + 1, reinterpret_cast<XrCompositionLayerBaseHeader*>(&keyboardPlanarLayer));
							}
						}

						keyboardRendered = true;
					}

					CHECK_VKCMD(vkEndCommandBuffer(commandBuffer));

					if (appState.Mode != AppWorldMode)
					{
						if (appState.CylinderCompositionLayerEnabled)
						{
							leftArrowsCylinderLayer.radius = CylinderProjection::radius;
							leftArrowsCylinderLayer.aspectRatio = 450.0f / 150.0f;
							leftArrowsCylinderLayer.centralAngle = CylinderProjection::horizontalAngle * 450.0f / 960.0f;
							leftArrowsCylinderLayer.subImage.swapchain = appState.LeftArrowsSwapchain;
							leftArrowsCylinderLayer.subImage.imageRect.extent.width = 450;
							leftArrowsCylinderLayer.subImage.imageRect.extent.height = 150;
							leftArrowsCylinderLayer.space = appSpace;
							leftArrowsCylinderLayer.layerFlags = 0;
							leftArrowsCylinderLayer.pose = { };

							XrMatrix4x4f rotation;
							XrMatrix4x4f_CreateRotation(&rotation, 0, 120, 0);
							XrMatrix4x4f_GetRotation(&leftArrowsCylinderLayer.pose.orientation, &rotation);

							layers.insert(layers.begin() + 1, reinterpret_cast<XrCompositionLayerBaseHeader*>(&leftArrowsCylinderLayer));
						}
						else
						{
							leftArrowsPlanarLayer.size.width = 450.0f / 960.0f;
							leftArrowsPlanarLayer.size.height = (450.0f / 960.0f) * 150.0f / 450.0f;
							leftArrowsPlanarLayer.subImage.swapchain = appState.LeftArrowsSwapchain;
							leftArrowsPlanarLayer.subImage.imageRect.extent.width = 450;
							leftArrowsPlanarLayer.subImage.imageRect.extent.height = 150;
							leftArrowsPlanarLayer.space = appSpace;
							leftArrowsPlanarLayer.layerFlags = 0;
							leftArrowsPlanarLayer.pose = { };

							XrMatrix4x4f rotation;
							XrMatrix4x4f_CreateRotation(&rotation, 0, 120, 0);
							XrMatrix4x4f_GetRotation(&leftArrowsPlanarLayer.pose.orientation, &rotation);

							XrVector3f position { 0, 0, -PlanarProjection::distance };
							XrMatrix4x4f_TransformVector3f(&leftArrowsPlanarLayer.pose.position, &rotation, &position);

							layers.insert(layers.begin() + 1, reinterpret_cast<XrCompositionLayerBaseHeader*>(&leftArrowsPlanarLayer));
						}

						if (appState.CylinderCompositionLayerEnabled)
						{
							rightArrowsCylinderLayer.radius = CylinderProjection::radius;
							rightArrowsCylinderLayer.aspectRatio = 450.0f / 150.0f;
							rightArrowsCylinderLayer.centralAngle = CylinderProjection::horizontalAngle * 450.0f / 960.0f;
							rightArrowsCylinderLayer.subImage.swapchain = appState.RightArrowsSwapchain;
							rightArrowsCylinderLayer.subImage.imageRect.extent.width = 450;
							rightArrowsCylinderLayer.subImage.imageRect.extent.height = 150;
							rightArrowsCylinderLayer.space = appSpace;
							rightArrowsCylinderLayer.layerFlags = 0;
							rightArrowsCylinderLayer.pose = { };

							XrMatrix4x4f rotation;
							XrMatrix4x4f_CreateRotation(&rotation, 0, -120, 0);
							XrMatrix4x4f_GetRotation(&rightArrowsCylinderLayer.pose.orientation, &rotation);

							layers.insert(layers.begin() + 2, reinterpret_cast<XrCompositionLayerBaseHeader*>(&rightArrowsCylinderLayer));
						}
						else
						{
							rightArrowsPlanarLayer.size.width =  450.0f / 960.0f;
							rightArrowsPlanarLayer.size.height = (450.0f / 960.0f) * 150.0f / 450.0f;
							rightArrowsPlanarLayer.subImage.swapchain = appState.RightArrowsSwapchain;
							rightArrowsPlanarLayer.subImage.imageRect.extent.width = 450;
							rightArrowsPlanarLayer.subImage.imageRect.extent.height = 150;
							rightArrowsPlanarLayer.space = appSpace;
							rightArrowsPlanarLayer.layerFlags = 0;
							rightArrowsPlanarLayer.pose = { };

							XrMatrix4x4f rotation;
							XrMatrix4x4f_CreateRotation(&rotation, 0, -120, 0);
							XrMatrix4x4f_GetRotation(&rightArrowsPlanarLayer.pose.orientation, &rotation);

							XrVector3f position { 0, 0, -PlanarProjection::distance };
							XrMatrix4x4f_TransformVector3f(&rightArrowsPlanarLayer.pose.position, &rotation, &position);

							layers.insert(layers.begin() + 2, reinterpret_cast<XrCompositionLayerBaseHeader*>(&rightArrowsPlanarLayer));
						}
					}

					if (cubeCompositionLayerEnabled && appState.Mode == AppWorldMode)
					{
						if (d_lists.last_skybox >= 0)
						{
							int width = -1;
							int height = -1;
							auto& skybox = d_lists.skyboxes[d_lists.last_skybox];
							if (appState.Scene.skybox == nullptr)
							{
								for (size_t i = 0; i < 6; i++)
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
							}
							else
							{
								auto same = true;
								for (size_t i = 0; i < 6; i++)
								{
									auto texture = skybox.textures[i].texture;
									if (texture != appState.Scene.skybox->sources[i])
									{
										same = false;
										break;
									}
								}
								if (!same)
								{
									Skybox::MoveToPrevious(appState.Scene);
									for (size_t i = 0; i < 6; i++)
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
								}
							}
							if (width > 0 && height > 0)
							{
								appState.Scene.skybox = new Skybox { };

								appState.Scene.skybox->sources.resize(6);

								for (size_t i = 0; i < 6; i++)
								{
									appState.Scene.skybox->sources[i] = skybox.textures[i].texture;
								}

								swapchainCreateInfo.createFlags = XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
								swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
								swapchainCreateInfo.format = Constants::colorFormat;
								swapchainCreateInfo.width = width;
								swapchainCreateInfo.height = height;
								swapchainCreateInfo.faceCount = 6;
								swapchainCreateInfo.arraySize = 1;
								CHECK_XRCMD(xrCreateSwapchain(appState.Session, &swapchainCreateInfo, &appState.Scene.skybox->swapchain));

								uint32_t skyboxImageCount;
								CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Scene.skybox->swapchain, 0, &skyboxImageCount, nullptr));

								appState.Scene.skybox->images.resize(skyboxImageCount, { XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR });
								CHECK_XRCMD(xrEnumerateSwapchainImages(appState.Scene.skybox->swapchain, skyboxImageCount, &skyboxImageCount, (XrSwapchainImageBaseHeader*)appState.Scene.skybox->images.data()));

								Buffer stagingBuffer;
								stagingBuffer.CreateStagingBuffer(appState, 6 * width * height * sizeof(uint32_t));

								stagingBuffer.Map(appState);

								auto target = (uint32_t*)stagingBuffer.mapped;

								for (size_t i = 0; i < 6; i++)
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

								stagingBuffer.UnmapAndFlush(appState);

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
									vkCmdCopyBufferToImage(setupCommandBuffer, stagingBuffer.buffer, appState.Scene.skybox->images[swapchainImageIndex].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
									region.bufferOffset += width * height * 4;
									appState.submitBarrier.subresourceRange.baseArrayLayer = i;
									vkCmdPipelineBarrier(setupCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &appState.submitBarrier);
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

								stagingBuffer.Delete(appState);
							}
						}
						if (appState.Scene.skybox != VK_NULL_HANDLE)
						{
							XrMatrix4x4f rotation;
							XrMatrix4x4f_CreateRotation(&rotation, 0, 90, 0);
							XrMatrix4x4f_GetRotation(&skyboxLayer.orientation, &rotation);

							skyboxLayer.space = appSpace;
							skyboxLayer.swapchain = appState.Scene.skybox->swapchain;

							layers.insert(layers.begin(), reinterpret_cast<XrCompositionLayerBaseHeader*>(&skyboxLayer));
						}
					}
				}
			}

			if (commandBuffer != VK_NULL_HANDLE)
			{
				submitInfo.pCommandBuffers = &commandBuffer;
				CHECK_VKCMD(vkQueueSubmit(appState.Queue, 1, &submitInfo, perFrame.fence));
			}
			else
			{
				submitInfo.pCommandBuffers = VK_NULL_HANDLE;
				CHECK_VKCMD(vkQueueSubmit(appState.Queue, 0, &submitInfo, perFrame.fence));
			}

			CHECK_XRCMD(xrReleaseSwapchainImage(swapchain, &releaseInfo));
			if (depthSwapchain != VK_NULL_HANDLE)
			{
				CHECK_XRCMD(xrReleaseSwapchainImage(depthSwapchain, &releaseInfo));
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

		} while (!exitRenderLoop);

		if (appState.EngineThread.joinable())
		{
			appState.EngineThreadStopped = true;
			appState.EngineThread.join();
			appState.EngineThreadStopped = false;
			appState.EngineThreadStarted = false;
		}

		if (appState.Queue != VK_NULL_HANDLE)
		{
			CHECK_VKCMD(vkQueueWaitIdle(appState.Queue));
		}

		appState.Destroy();

		for (auto imageView : appState.DepthSwapchainImageViews)
		{
			if (imageView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(appState.Device, imageView, nullptr);
			}
		}

		if (depthSwapchain != XR_NULL_HANDLE)
		{
			xrDestroySwapchain(depthSwapchain);
		}

		if (swapchain != XR_NULL_HANDLE)
		{
			xrDestroySwapchain(swapchain);
		}

		if (appState.HandTrackingEnabled)
		{
			for (auto& handTracker : appState.HandTrackers)
			{
				xrDestroyHandTrackerEXT(handTracker.tracker);
			}
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

#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
		if (vulkanDebugMessenger != VK_NULL_HANDLE)
		{
			vkDestroyDebugUtilsMessengerEXT(vulkanInstance, vulkanDebugMessenger, nullptr);
		}
#endif

		if (vulkanInstance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(vulkanInstance, nullptr);
		}

		if (sys_nogamedata)
		{
			Throw("No game data found in the current directory.", nullptr, FILE_AND_LINE);
		}
		else if (sys_errorcalled)
		{
			Throw(Fmt("Sys_Error: %s", sys_errormessage.c_str()), nullptr, FILE_AND_LINE);
		}
	}
	catch (const std::exception& ex)
	{
		std::string message = "The following error ocurred:\n\n";
		message += ex.what();
		if (message.find("XR_ERROR_FORM_FACTOR_UNAVAILABLE") != std::string::npos)
		{
			message += "\n\nThis usually indicates that the VR headset is not connected.";
			message += "\nVerify that the headset is connected to begin playing.";
		}
		else
		{
			message += "\n\nCheck the logs above for hints on what could have happened.";
		}
		message += "\n\n\nPress Enter to exit the application.";
		PrintErrorMessage(message);
		exitWithError = true;
	}
	catch (...)
	{
		std::string message = "An error occurred, for which no error message could be provided.";
		message += "\n\nCheck the logs above for hints on what could have happened.";
		message += "\n\n\nPress Enter to exit the application.";
		PrintErrorMessage(message);
		exitWithError = true;
	}

	if (!exitWithError)
	{
		PrintErrorMessage("Application closed. Press Enter to close this window.");
	}

	if (sound_started)
	{
		SNDDMA_Shutdown();
	}

	delete appState.Logger;
	appState.Logger = nullptr;

	delete appState.FileLoader;
	appState.FileLoader = nullptr;

	CoUninitialize();

	auto c = getchar();
	while (c != '\n')
	{
		c = getchar();
	}

	return (exitWithError ? EXIT_FAILURE : EXIT_SUCCESS);
}
