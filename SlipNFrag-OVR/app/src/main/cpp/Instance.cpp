#include "Instance.h"
#include <android/log.h>
#include "sys_ovr.h"
#include "VrApi.h"

void Instance::Bind()
{
	vkDestroyInstance = (PFN_vkDestroyInstance) (vkGetInstanceProcAddr(instance, "vkDestroyInstance"));
	if (vkDestroyInstance == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkDestroyInstance.");
		vrapi_Shutdown();
		exit(0);
	}
	vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) (vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices"));
	if (vkEnumeratePhysicalDevices == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkEnumeratePhysicalDevices.");
		vrapi_Shutdown();
		exit(0);
	}
	vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures"));
	if (vkGetPhysicalDeviceFeatures == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceFeatures.");
		vrapi_Shutdown();
		exit(0);
	}
	vkGetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR"));
	if (vkGetPhysicalDeviceFeatures2KHR == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceFeatures2KHR.");
		vrapi_Shutdown();
		exit(0);
	}
	vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties"));
	if (vkGetPhysicalDeviceProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR"));
	if (vkGetPhysicalDeviceProperties2KHR == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceProperties2KHR.");
		vrapi_Shutdown();
		exit(0);
	}
	vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties"));
	if (vkGetPhysicalDeviceMemoryProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceMemoryProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
	if (vkGetPhysicalDeviceQueueFamilyProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceQueueFamilyProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties"));
	if (vkGetPhysicalDeviceFormatProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceFormatProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties"));
	if (vkGetPhysicalDeviceImageFormatProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceImageFormatProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties) (vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties"));
	if (vkEnumerateDeviceExtensionProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkEnumerateDeviceExtensionProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties) (vkGetInstanceProcAddr(instance, "vkEnumerateDeviceLayerProperties"));
	if (vkEnumerateDeviceLayerProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkEnumerateDeviceLayerProperties.");
		vrapi_Shutdown();
		exit(0);
	}
	vkCreateDevice = (PFN_vkCreateDevice) (vkGetInstanceProcAddr(instance, "vkCreateDevice"));
	if (vkCreateDevice == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkCreateDevice.");
		vrapi_Shutdown();
		exit(0);
	}
	vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) (vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr"));
	if (vkGetDeviceProcAddr == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetDeviceProcAddr.");
		vrapi_Shutdown();
		exit(0);
	}
}