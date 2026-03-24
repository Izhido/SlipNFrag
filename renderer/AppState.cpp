#include "AppState.h"
#include "Constants.h"
#include "Locks.h"
#include "DirectRect.h"
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

void AppState::RenderScreen(ScreenPerFrame& perFrame, int width, int height)
{
	if (Mode == AppScreenMode || Mode == AppWorldMode)
	{
		if (Mode == AppScreenMode)
		{
			if (key_dest == key_game)
			{
				auto console = Scene.consoleData.data();
				auto previousConsole = console;
				auto screen = Scene.screenData.data();
				perFrame.stagingBuffer.Map(*this);
				auto target = (uint32_t*)perFrame.stagingBuffer.mapped;
				auto y = 0;
				while (y < height)
				{
					auto x = 0;
					while (x < width)
					{
						auto entry = *console++;
						if (entry == 255)
						{
							do
							{
								*target++ = Scene.paletteData[*screen++];
								x++;
							} while ((x % Constants::screenToConsoleMultiplier) != 0);
						}
						else
						{
							auto converted = Scene.paletteData[entry];
							do
							{
								*target++ = converted;
								screen++;
								x++;
							} while ((x % Constants::screenToConsoleMultiplier) != 0);
						}
					}
					y++;
					if ((y % Constants::screenToConsoleMultiplier) == 0)
					{
						previousConsole = console;
					}
					else
					{
						console = previousConsole;
					}
				}
				perFrame.stagingBuffer.UnmapAndFlush(*this);
			}
			else
			{
				auto console = Scene.consoleData.data();
				auto previousConsole = console;
				auto screen = Scene.screenData.data();
				perFrame.stagingBuffer.Map(*this);
				auto target = (uint32_t*)perFrame.stagingBuffer.mapped;
				auto black = Scene.paletteData[0];
				auto y = 0;
				while (y < height)
				{
					auto t = (y & 1) << 1;
					auto x = 0;
					while (x < width)
					{
						auto entry = *console++;
						if (entry == 255)
						{
							do
							{
								if ((x & 3) == t)
								{
									*target++ = Scene.paletteData[*screen++];
								}
								else
								{
									*target++ = black;
									screen++;
								}
								x++;
							} while ((x % Constants::screenToConsoleMultiplier) != 0);
						}
						else
						{
							auto converted = Scene.paletteData[entry];
							do
							{
								*target++ = converted;
								screen++;
								x++;
							} while ((x % Constants::screenToConsoleMultiplier) != 0);
						}
					}
					y++;
					if ((y % Constants::screenToConsoleMultiplier) == 0)
					{
						previousConsole = console;
					}
					else
					{
						console = previousConsole;
					}
				}
				perFrame.stagingBuffer.UnmapAndFlush(*this);
			}
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto w = directRect.width * Constants::screenToConsoleMultiplier;
					auto h = directRect.height * Constants::screenToConsoleMultiplier;
					auto source = directRect.data;
					perFrame.stagingBuffer.Map(*this);
					auto target = (uint32_t*)perFrame.stagingBuffer.mapped + (directRect.y * ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
					auto y = 0;
					while (y < h)
					{
						auto x = 0;
						while (x < w)
						{
							auto entry = Scene.paletteData[*source++];
							do
							{
								*target++ = entry;
								x++;
							} while ((x % Constants::screenToConsoleMultiplier) != 0);
						}
						y++;
						while ((y % Constants::screenToConsoleMultiplier) != 0)
						{
							target -= w;
							std::copy(target, target + w, target + ScreenWidth);
							target += ScreenWidth + w;
							y++;
						}
						target += ScreenWidth - w;
					}
					perFrame.stagingBuffer.UnmapAndFlush(*this);
				}
			}
		}
		else if (key_dest == key_game)
		{
			auto source = Scene.consoleData.data();
			perFrame.stagingBuffer.Map(*this);
			auto target = (uint32_t*)perFrame.stagingBuffer.mapped;
			if (Scene.statusBarVerticesSize > 0)
			{
				auto count = ConsoleWidth * (ConsoleHeight - (SBAR_HEIGHT + 24));
				auto found = false;
				while (count > 0)
				{
					auto entry = *source++;
					count--;
					if (entry != 255)
					{
						found = true;
						auto leading = ConsoleWidth * (ConsoleHeight - (SBAR_HEIGHT + 24)) - count + 1;
						std::fill(target, target + leading, 0);
						target += leading;
						*target++ = Scene.paletteData[entry];
						while (count > 0)
						{
							entry = *source++;
							*target++ = Scene.paletteData[entry];
							count--;
						}
					}
				}
				if (!found)
				{
					std::fill(target, target + ConsoleWidth * (ConsoleHeight - (SBAR_HEIGHT + 24)), 0);
					target += ConsoleWidth * (ConsoleHeight - (SBAR_HEIGHT + 24));
				}
				std::fill(target, target + ConsoleWidth * (SBAR_HEIGHT + 24), 0);
			}
			else
			{
				auto count = ConsoleWidth * ConsoleHeight;
				auto found = false;
				while (count > 0)
				{
					auto entry = *source++;
					count--;
					if (entry != 255)
					{
						found = true;
						auto leading = ConsoleWidth * ConsoleHeight - count + 1;
						std::fill(target, target + leading, 0);
						target += leading;
						*target++ = Scene.paletteData[entry];
						while (count > 0)
						{
							entry = *source++;
							*target++ = Scene.paletteData[entry];
							count--;
						}
					}
				}
				if (!found)
				{
					std::fill(target, target + ConsoleWidth * ConsoleHeight, 0);
				}
			}
			perFrame.stagingBuffer.UnmapAndFlush(*this);
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					source = directRect.data;
					perFrame.stagingBuffer.Map(*this);
					target = (uint32_t*)perFrame.stagingBuffer.mapped + directRect.y * ConsoleWidth + directRect.x;
					for (auto y = 0; y < directRect.height; y++)
					{
						for (auto x = 0; x < directRect.width; x++)
						{
							*target++ = Scene.paletteData[*source++];
						}
						target += ConsoleWidth - directRect.width;
					}
					perFrame.stagingBuffer.UnmapAndFlush(*this);
				}
			}
		}
		else
		{
			auto source = Scene.consoleData.data();
			auto previousSource = source;
			perFrame.stagingBuffer.Map(*this);
			auto target = (uint32_t*)perFrame.stagingBuffer.mapped;
			auto black = Scene.paletteData[0];
			auto y = 0;
			auto limit = ScreenHeight - (SBAR_HEIGHT + 24) * Constants::screenToConsoleMultiplier;
			while (y < limit)
			{
				auto t = (y & 1) << 1;
				auto x = 0;
				while (x < ScreenWidth)
				{
					auto entry = *source++;
					if (entry == 255)
					{
						do
						{
							if ((x & 3) == t)
							{
								*target++ = 0;
							}
							else
							{
								*target++ = black;
							}
							x++;
						} while ((x % Constants::screenToConsoleMultiplier) != 0);
					}
					else
					{
						auto converted = Scene.paletteData[entry];
						do
						{
							*target++ = converted;
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
			limit = ScreenHeight - (SBAR_HEIGHT + 24);
			while (y < limit)
			{
				auto t = (y & 1) << 1;
				auto x = 0;
				while (x < ScreenWidth)
				{
					if ((x & 3) == t)
					{
						*target++ = 0;
					}
					else
					{
						*target++ = black;
					}
					x++;
				}
				y++;
			}
			limit = ScreenWidth / Constants::screenToConsoleMultiplier;
			while (y < ScreenHeight)
			{
				auto t = (y & 1) << 1;
				auto x = 0;
				while (x < limit)
				{
					auto entry = *source++;
					if (entry == 255)
					{
						if ((x & 3) == t)
						{
							*target++ = 0;
						}
						else
						{
							*target++ = black;
						}
					}
					else
					{
						*target++ = Scene.paletteData[entry];
					}
					x++;
				}
				while (x < ScreenWidth)
				{
					if ((x & 3) == t)
					{
						*target++ = 0;
					}
					else
					{
						*target++ = black;
					}
					x++;
				}
				y++;
			}
			perFrame.stagingBuffer.UnmapAndFlush(*this);
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto w = directRect.width * Constants::screenToConsoleMultiplier;
					auto h = directRect.height * Constants::screenToConsoleMultiplier;
					source = directRect.data;
					perFrame.stagingBuffer.Map(*this);
					target = (uint32_t*)perFrame.stagingBuffer.mapped + (directRect.y * ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
					y = 0;
					while (y < h)
					{
						auto x = 0;
						while (x < w)
						{
							auto entry = Scene.paletteData[*source++];
							do
							{
								*target++ = entry;
								x++;
							} while ((x % Constants::screenToConsoleMultiplier) != 0);
						}
						y++;
						while ((y % Constants::screenToConsoleMultiplier) != 0)
						{
							target -= w;
							std::copy(target, target + w, target + ScreenWidth);
							target += ScreenWidth + w;
							y++;
						}
						target += ScreenWidth - w;
					}
					perFrame.stagingBuffer.UnmapAndFlush(*this);
				}
			}
		}
	}
	else
	{
		if (Mode == AppNoGameDataMode)
		{
			if (NoGameDataImageSource != nullptr)
			{
				std::copy(NoGameDataImageSource->data(), NoGameDataImageSource->data() + NoGameDataImageSource->size(), ScreenData.data());
				delete NoGameDataImageSource;
				NoGameDataImageSource = nullptr;
			}
		}
		else if (Mode == AppNoGameDataUncompressMode)
		{
			if (NoGameDataUncompressImageSource != nullptr)
			{
				std::copy(NoGameDataUncompressImageSource->data(), NoGameDataUncompressImageSource->data() + NoGameDataUncompressImageSource->size(), ScreenData.data());
				delete NoGameDataUncompressImageSource;
				NoGameDataUncompressImageSource = nullptr;
			}
		}
		else if (Mode == AppInvalidGameDataUncompressMode)
		{
			if (InvalidGameDataUncompressImageSource != nullptr)
			{
				std::copy(InvalidGameDataUncompressImageSource->data(), InvalidGameDataUncompressImageSource->data() + InvalidGameDataUncompressImageSource->size(), ScreenData.data());
				delete InvalidGameDataUncompressImageSource;
				InvalidGameDataUncompressImageSource = nullptr;
			}
		}
		else if (Mode == AppSharewareGameDataMode)
		{
			if (SharewareGameDataImageSource != nullptr)
			{
				std::copy(SharewareGameDataImageSource->data(), SharewareGameDataImageSource->data() + SharewareGameDataImageSource->size(), ScreenData.data());
				delete SharewareGameDataImageSource;
				SharewareGameDataImageSource = nullptr;
			}
		}
		CHECK_VKCMD(vmaCopyMemoryToAllocation(Allocator, ScreenData.data(), perFrame.stagingBuffer.allocation, 0, perFrame.stagingBuffer.size));
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

	if (Actions.ActionSet != XR_NULL_HANDLE)
	{
		xrDestroySpace(HandSpaces[0]);
		xrDestroySpace(HandSpaces[1]);
		HandSpaces.clear();

		xrDestroyActionSet(Actions.ActionSet);
		Actions.ActionSet = XR_NULL_HANDLE;
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