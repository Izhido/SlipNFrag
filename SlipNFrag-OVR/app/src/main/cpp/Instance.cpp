#include "Instance.h"
#include <android/log.h>
#include "sys_ovr.h"
#include "VrApi.h"

bool Instance::Bind()
{
	vkDestroyInstance = (PFN_vkDestroyInstance) (vkGetInstanceProcAddr(instance, "vkDestroyInstance"));
	if (vkDestroyInstance == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkDestroyInstance.");
		return false;
	}
	vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices) (vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices"));
	if (vkEnumeratePhysicalDevices == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkEnumeratePhysicalDevices.");
		return false;
	}
	vkGetPhysicalDeviceFeatures = (PFN_vkGetPhysicalDeviceFeatures) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures"));
	if (vkGetPhysicalDeviceFeatures == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceFeatures.");
		return false;
	}
	vkGetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR"));
	if (vkGetPhysicalDeviceFeatures2KHR == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceFeatures2KHR.");
		return false;
	}
	vkGetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties"));
	if (vkGetPhysicalDeviceProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceProperties.");
		return false;
	}
	vkGetPhysicalDeviceProperties2KHR = (PFN_vkGetPhysicalDeviceProperties2KHR) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2KHR"));
	if (vkGetPhysicalDeviceProperties2KHR == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceProperties2KHR.");
		return false;
	}
	vkGetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties"));
	if (vkGetPhysicalDeviceMemoryProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceMemoryProperties.");
		return false;
	}
	vkGetPhysicalDeviceQueueFamilyProperties = (PFN_vkGetPhysicalDeviceQueueFamilyProperties) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
	if (vkGetPhysicalDeviceQueueFamilyProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceQueueFamilyProperties.");
		return false;
	}
	vkGetPhysicalDeviceFormatProperties = (PFN_vkGetPhysicalDeviceFormatProperties) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties"));
	if (vkGetPhysicalDeviceFormatProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceFormatProperties.");
		return false;
	}
	vkGetPhysicalDeviceImageFormatProperties = (PFN_vkGetPhysicalDeviceImageFormatProperties) (vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties"));
	if (vkGetPhysicalDeviceImageFormatProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetPhysicalDeviceImageFormatProperties.");
		return false;
	}
	vkEnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties) (vkGetInstanceProcAddr(instance, "vkEnumerateDeviceExtensionProperties"));
	if (vkEnumerateDeviceExtensionProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkEnumerateDeviceExtensionProperties.");
		return false;
	}
	vkEnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties) (vkGetInstanceProcAddr(instance, "vkEnumerateDeviceLayerProperties"));
	if (vkEnumerateDeviceLayerProperties == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkEnumerateDeviceLayerProperties.");
		return false;
	}
	vkCreateDevice = (PFN_vkCreateDevice) (vkGetInstanceProcAddr(instance, "vkCreateDevice"));
	if (vkCreateDevice == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkCreateDevice.");
		return false;
	}
	vkGetDeviceProcAddr = (PFN_vkGetDeviceProcAddr) (vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr"));
	if (vkGetDeviceProcAddr == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Instance::Bind(): vkGetInstanceProcAddr() could not find vkGetDeviceProcAddr.");
		return false;
	}
	return true;
}
