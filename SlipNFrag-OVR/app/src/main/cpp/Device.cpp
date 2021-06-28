#include "Device.h"
#include "AppState.h"
#include <android/log.h>
#include "sys_ovr.h"

bool Device::Bind(AppState& appState, Instance& instance)
{
	appState.Device.vkDestroyDevice = (PFN_vkDestroyDevice)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyDevice"));
	if (appState.Device.vkDestroyDevice == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyDevice.");
		return false;
	}
	appState.Device.vkGetDeviceQueue = (PFN_vkGetDeviceQueue)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkGetDeviceQueue"));
	if (appState.Device.vkGetDeviceQueue == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkGetDeviceQueue.");
		return false;
	}
	appState.Device.vkQueueSubmit = (PFN_vkQueueSubmit)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkQueueSubmit"));
	if (appState.Device.vkQueueSubmit == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkQueueSubmit.");
		return false;
	}
	appState.Device.vkQueueWaitIdle = (PFN_vkQueueWaitIdle)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkQueueWaitIdle"));
	if (appState.Device.vkQueueWaitIdle == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkQueueWaitIdle.");
		return false;
	}
	appState.Device.vkDeviceWaitIdle = (PFN_vkDeviceWaitIdle)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDeviceWaitIdle"));
	if (appState.Device.vkDeviceWaitIdle == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDeviceWaitIdle.");
		return false;
	}
	appState.Device.vkAllocateMemory = (PFN_vkAllocateMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkAllocateMemory"));
	if (appState.Device.vkAllocateMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkAllocateMemory.");
		return false;
	}
	appState.Device.vkFreeMemory = (PFN_vkFreeMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkFreeMemory"));
	if (appState.Device.vkFreeMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkFreeMemory.");
		return false;
	}
	appState.Device.vkMapMemory = (PFN_vkMapMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkMapMemory"));
	if (appState.Device.vkMapMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkMapMemory.");
		return false;
	}
	appState.Device.vkUnmapMemory = (PFN_vkUnmapMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkUnmapMemory"));
	if (appState.Device.vkUnmapMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkUnmapMemory.");
		return false;
	}
	appState.Device.vkBindBufferMemory = (PFN_vkBindBufferMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkBindBufferMemory"));
	if (appState.Device.vkBindBufferMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkBindBufferMemory.");
		return false;
	}
	appState.Device.vkBindImageMemory = (PFN_vkBindImageMemory)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkBindImageMemory"));
	if (appState.Device.vkBindImageMemory == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkBindImageMemory.");
		return false;
	}
	appState.Device.vkGetBufferMemoryRequirements = (PFN_vkGetBufferMemoryRequirements)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkGetBufferMemoryRequirements"));
	if (appState.Device.vkGetBufferMemoryRequirements == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkGetBufferMemoryRequirements.");
		return false;
	}
	appState.Device.vkGetImageMemoryRequirements = (PFN_vkGetImageMemoryRequirements)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkGetImageMemoryRequirements"));
	if (appState.Device.vkGetImageMemoryRequirements == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkGetImageMemoryRequirements.");
		return false;
	}
	appState.Device.vkCreateFence = (PFN_vkCreateFence)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateFence"));
	if (appState.Device.vkCreateFence == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateFence.");
		return false;
	}
	appState.Device.vkDestroyFence = (PFN_vkDestroyFence)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyFence"));
	if (appState.Device.vkDestroyFence == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyFence.");
		return false;
	}
	appState.Device.vkResetFences = (PFN_vkResetFences)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkResetFences"));
	if (appState.Device.vkResetFences == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkResetFences.");
		return false;
	}
	appState.Device.vkGetFenceStatus = (PFN_vkGetFenceStatus)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkGetFenceStatus"));
	if (appState.Device.vkGetFenceStatus == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkGetFenceStatus.");
		return false;
	}
	appState.Device.vkWaitForFences = (PFN_vkWaitForFences)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkWaitForFences"));
	if (appState.Device.vkWaitForFences == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkWaitForFences.");
		return false;
	}
	appState.Device.vkCreateBuffer = (PFN_vkCreateBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateBuffer"));
	if (appState.Device.vkCreateBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateBuffer.");
		return false;
	}
	appState.Device.vkDestroyBuffer = (PFN_vkDestroyBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyBuffer"));
	if (appState.Device.vkDestroyBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyBuffer.");
		return false;
	}
	appState.Device.vkCreateImage = (PFN_vkCreateImage)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateImage"));
	if (appState.Device.vkCreateImage == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateImage.");
		return false;
	}
	appState.Device.vkDestroyImage = (PFN_vkDestroyImage)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyImage"));
	if (appState.Device.vkDestroyImage == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyImage.");
		return false;
	}
	appState.Device.vkCreateImageView = (PFN_vkCreateImageView)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateImageView"));
	if (appState.Device.vkCreateImageView == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateImageView.");
		return false;
	}
	appState.Device.vkDestroyImageView = (PFN_vkDestroyImageView)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyImageView"));
	if (appState.Device.vkDestroyImageView == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyImageView.");
		return false;
	}
	appState.Device.vkCreateShaderModule = (PFN_vkCreateShaderModule)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateShaderModule"));
	if (appState.Device.vkCreateShaderModule == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateShaderModule.");
		return false;
	}
	appState.Device.vkDestroyShaderModule = (PFN_vkDestroyShaderModule)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyShaderModule"));
	if (appState.Device.vkDestroyShaderModule == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyShaderModule.");
		return false;
	}
	appState.Device.vkCreatePipelineCache = (PFN_vkCreatePipelineCache)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreatePipelineCache"));
	if (appState.Device.vkCreatePipelineCache == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreatePipelineCache.");
		return false;
	}
	appState.Device.vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyPipelineCache"));
	if (appState.Device.vkDestroyPipelineCache == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyPipelineCache.");
		return false;
	}
	appState.Device.vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateGraphicsPipelines"));
	if (appState.Device.vkCreateGraphicsPipelines == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateGraphicsPipelines.");
		return false;
	}
	appState.Device.vkDestroyPipeline = (PFN_vkDestroyPipeline)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyPipeline"));
	if (appState.Device.vkDestroyPipeline == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyPipeline.");
		return false;
	}
	appState.Device.vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreatePipelineLayout"));
	if (appState.Device.vkCreatePipelineLayout == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreatePipelineLayout.");
		return false;
	}
	appState.Device.vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyPipelineLayout"));
	if (appState.Device.vkDestroyPipelineLayout == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyPipelineLayout.");
		return false;
	}
	appState.Device.vkCreateSampler = (PFN_vkCreateSampler)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateSampler"));
	if (appState.Device.vkCreateSampler == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateSampler.");
		return false;
	}
	appState.Device.vkDestroySampler = (PFN_vkDestroySampler)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroySampler"));
	if (appState.Device.vkDestroySampler == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroySampler.");
		return false;
	}
	appState.Device.vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateDescriptorSetLayout"));
	if (appState.Device.vkCreateDescriptorSetLayout == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateDescriptorSetLayout.");
		return false;
	}
	appState.Device.vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyDescriptorSetLayout"));
	if (appState.Device.vkDestroyDescriptorSetLayout == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyDescriptorSetLayout.");
		return false;
	}
	appState.Device.vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateDescriptorPool"));
	if (appState.Device.vkCreateDescriptorPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateDescriptorPool.");
		return false;
	}
	appState.Device.vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyDescriptorPool"));
	if (appState.Device.vkDestroyDescriptorPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyDescriptorPool.");
		return false;
	}
	appState.Device.vkResetDescriptorPool = (PFN_vkResetDescriptorPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkResetDescriptorPool"));
	if (appState.Device.vkResetDescriptorPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkResetDescriptorPool.");
		return false;
	}
	appState.Device.vkAllocateDescriptorSets = (PFN_vkAllocateDescriptorSets)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkAllocateDescriptorSets"));
	if (appState.Device.vkAllocateDescriptorSets == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkAllocateDescriptorSets.");
		return false;
	}
	appState.Device.vkUpdateDescriptorSets = (PFN_vkUpdateDescriptorSets)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkUpdateDescriptorSets"));
	if (appState.Device.vkUpdateDescriptorSets == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkUpdateDescriptorSets.");
		return false;
	}
	appState.Device.vkCreateFramebuffer = (PFN_vkCreateFramebuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateFramebuffer"));
	if (appState.Device.vkCreateFramebuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateFramebuffer.");
		return false;
	}
	appState.Device.vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyFramebuffer"));
	if (appState.Device.vkDestroyFramebuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyFramebuffer.");
		return false;
	}
	appState.Device.vkCreateRenderPass = (PFN_vkCreateRenderPass)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateRenderPass"));
	if (appState.Device.vkCreateRenderPass == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateRenderPass.");
		return false;
	}
	appState.Device.vkDestroyRenderPass = (PFN_vkDestroyRenderPass)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyRenderPass"));
	if (appState.Device.vkDestroyRenderPass == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyRenderPass.");
		return false;
	}
	appState.Device.vkCreateCommandPool = (PFN_vkCreateCommandPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCreateCommandPool"));
	if (appState.Device.vkCreateCommandPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCreateCommandPool.");
		return false;
	}
	appState.Device.vkDestroyCommandPool = (PFN_vkDestroyCommandPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkDestroyCommandPool"));
	if (appState.Device.vkDestroyCommandPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkDestroyCommandPool.");
		return false;
	}
	appState.Device.vkResetCommandPool = (PFN_vkResetCommandPool)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkResetCommandPool"));
	if (appState.Device.vkResetCommandPool == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkResetCommandPool.");
		return false;
	}
	appState.Device.vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkAllocateCommandBuffers"));
	if (appState.Device.vkAllocateCommandBuffers == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkAllocateCommandBuffers.");
		return false;
	}
	appState.Device.vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkFreeCommandBuffers"));
	if (appState.Device.vkFreeCommandBuffers == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkFreeCommandBuffers.");
		return false;
	}
	appState.Device.vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkBeginCommandBuffer"));
	if (appState.Device.vkBeginCommandBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkBeginCommandBuffer.");
		return false;
	}
	appState.Device.vkEndCommandBuffer = (PFN_vkEndCommandBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkEndCommandBuffer"));
	if (appState.Device.vkEndCommandBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkEndCommandBuffer.");
		return false;
	}
	appState.Device.vkResetCommandBuffer = (PFN_vkResetCommandBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkResetCommandBuffer"));
	if (appState.Device.vkResetCommandBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkResetCommandBuffer.");
		return false;
	}
	appState.Device.vkCmdBindPipeline = (PFN_vkCmdBindPipeline)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBindPipeline"));
	if (appState.Device.vkCmdBindPipeline == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdBindPipeline.");
		return false;
	}
	appState.Device.vkCmdSetViewport = (PFN_vkCmdSetViewport)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdSetViewport"));
	if (appState.Device.vkCmdSetViewport == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdSetViewport.");
		return false;
	}
	appState.Device.vkCmdSetScissor = (PFN_vkCmdSetScissor)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdSetScissor"));
	if (appState.Device.vkCmdSetScissor == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdSetScissor.");
		return false;
	}
	appState.Device.vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBindDescriptorSets"));
	if (appState.Device.vkCmdBindDescriptorSets == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdBindDescriptorSets.");
		return false;
	}
	appState.Device.vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBindIndexBuffer"));
	if (appState.Device.vkCmdBindIndexBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdBindIndexBuffer.");
		return false;
	}
	appState.Device.vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBindVertexBuffers"));
	if (appState.Device.vkCmdBindVertexBuffers == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdBindVertexBuffers.");
		return false;
	}
	appState.Device.vkCmdBlitImage = (PFN_vkCmdBlitImage)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBlitImage"));
	if (appState.Device.vkCmdBlitImage == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdBlitImage.");
		return false;
	}
	appState.Device.vkCmdDraw = (PFN_vkCmdDraw)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdDraw"));
	if (appState.Device.vkCmdDraw == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdDraw.");
		return false;
	}
	appState.Device.vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdDrawIndexed"));
	if (appState.Device.vkCmdDrawIndexed == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdDrawIndexed.");
		return false;
	}
	appState.Device.vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdCopyBuffer"));
	if (appState.Device.vkCmdCopyBuffer == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdCopyBuffer.");
		return false;
	}
	appState.Device.vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdCopyBufferToImage"));
	if (appState.Device.vkCmdCopyBufferToImage == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdCopyBufferToImage.");
		return false;
	}
	appState.Device.vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdPipelineBarrier"));
	if (appState.Device.vkCmdPipelineBarrier == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdPipelineBarrier.");
		return false;
	}
	appState.Device.vkCmdPushConstants = (PFN_vkCmdPushConstants)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdPushConstants"));
	if (appState.Device.vkCmdPushConstants == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdPushConstants.");
		return false;
	}
	appState.Device.vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdBeginRenderPass"));
	if (appState.Device.vkCmdBeginRenderPass == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdBeginRenderPass.");
		return false;
	}
	appState.Device.vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)(instance.vkGetDeviceProcAddr(appState.Device.device, "vkCmdEndRenderPass"));
	if (appState.Device.vkCmdEndRenderPass == nullptr)
	{
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Device::Bind(): vkGetDeviceProcAddr() could not find vkCmdEndRenderPass.");
		return false;
	}
	return true;
}
