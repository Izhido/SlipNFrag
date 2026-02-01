#include "AppState.h"
#include "FileLoader.h"
#include "Utils.h"

void AppState::AnglesFromQuaternion(XrQuaternionf& quat, float& yaw, float& pitch, float& roll)
{
	auto x = quat.x;
	auto y = quat.y;
	auto z = quat.z;
	auto w = quat.w;

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
		yaw = 0;
		pitch = -M_PI / 2;
		roll = atan2(2 * (psign * Q[1] * Q[0] + w * Q[2]), ww + Q22 - Q11 - Q33);
	}
	else if (s2 > 1 - singularityRadius)
	{
		yaw = 0;
		pitch = M_PI / 2;
		roll = atan2(2 * (psign * Q[1] * Q[0] + w * Q[2]), ww + Q22 - Q11 - Q33);
	}
	else
	{
		yaw = -(atan2(-2 * (w * Q[1] - psign * Q[0] * Q[2]), ww + Q33 - Q11 - Q22));
		pitch = asin(s2);
		roll = atan2(2 * (w * Q[2] - psign * Q[1] * Q[0]), ww + Q11 - Q22 - Q33);
	}
}

void AppState::RenderKeyboard(ScreenPerFrame& perFrame)
{
	auto source = Keyboard.buffer.data();
	perFrame.stagingBuffer.Map(*this);
	auto target = (uint32_t*)perFrame.stagingBuffer.mapped;
	auto count = ConsoleWidth * ConsoleHeight / 2;
	while (count > 0)
	{
		auto entry = *source++;
		*target++ = Scene.paletteData[entry];
		count--;
	}
	perFrame.stagingBuffer.UnmapAndFlush(*this);
}

void AppState::DestroyImageSources()
{
	delete SharewareGameDataImageSource;
	SharewareGameDataImageSource = nullptr;

	delete InvalidGameDataUncompressImageSource;
	InvalidGameDataUncompressImageSource = nullptr;

	delete NoGameDataUncompressImageSource;
	NoGameDataUncompressImageSource = nullptr;

	delete NoGameDataImageSource;
	NoGameDataImageSource = nullptr;
}

void AppState::Destroy()
{
	if (Device != VK_NULL_HANDLE)
	{
		for (auto& perFrame : PerFrame)
		{
			perFrame.second.DestroyFramebuffer(*this);
		}

		Scene.Reset(*this);

		for (auto& texture : KeyboardTextures)
		{
			texture.second.Delete(*this);
		}
		KeyboardTextures.clear();

		for (auto& perFrame : Keyboard.Screen.perFrame)
		{
			perFrame.second.stagingBuffer.Delete(*this);
		}
		Keyboard.Screen.perFrame.clear();

		for (auto& texture : StatusBarTextures)
		{
			texture.Delete(*this);
		}
		StatusBarTextures.clear();

		for (auto& texture : ConsoleTextures)
		{
			texture.Delete(*this);
		}
		ConsoleTextures.clear();

		for (auto& perFrame : Screen.perFrame)
		{
			perFrame.second.stagingBuffer.Delete(*this);
		}
		Screen.perFrame.clear();

		for (auto& perFrame : PerFrame)
		{
			perFrame.second.Destroy(*this);
		}

		PerFrame.clear();

		if (RenderPass != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(Device, RenderPass, nullptr);
			RenderPass = VK_NULL_HANDLE;
		}

		Scene.Destroy(*this);

		if (SetupCommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(Device, SetupCommandPool, nullptr);
			SetupCommandPool = VK_NULL_HANDLE;
		}
	}

	Scene.created = false;

	if (ActionSet != XR_NULL_HANDLE)
	{
		xrDestroySpace(HandSpaces[0]);
		xrDestroySpace(HandSpaces[1]);
		HandSpaces.clear();

		xrDestroyActionSet(ActionSet);
		ActionSet = XR_NULL_HANDLE;
	}

	Skybox::DeleteAll(*this);

	if (RightArrowsSwapchain != XR_NULL_HANDLE)
	{
		xrDestroySwapchain(RightArrowsSwapchain);
		RightArrowsSwapchain = XR_NULL_HANDLE;
	}

	if (LeftArrowsSwapchain != XR_NULL_HANDLE)
	{
		xrDestroySwapchain(LeftArrowsSwapchain);
		LeftArrowsSwapchain = XR_NULL_HANDLE;
	}

	if (Keyboard.Screen.swapchain != XR_NULL_HANDLE)
	{
		xrDestroySwapchain(Keyboard.Screen.swapchain);
		Keyboard.Screen.swapchain = XR_NULL_HANDLE;
	}

	if (Screen.swapchain != XR_NULL_HANDLE)
	{
		xrDestroySwapchain(Screen.swapchain);
		Screen.swapchain = XR_NULL_HANDLE;
	}

	DestroyImageSources();

	if (Allocator != VK_NULL_HANDLE)
	{
		vmaDestroyAllocator(Allocator);
	}
}