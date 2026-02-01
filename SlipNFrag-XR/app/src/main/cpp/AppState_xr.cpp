#include "AppState_xr.h"
#include "vid_xr.h"
#include "Constants.h"
#include "DirectRect.h"
#include "Locks.h"
#include "Utils.h"

void AppState_xr::RenderScreen(ScreenPerFrame& perFrame)
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
				void* mapped;
				CHECK_VKCMD(vmaMapMemory(Allocator, perFrame.allocation, &mapped));
				auto target = (uint32_t*)mapped;
				auto y = 0;
				while (y < vid_height)
				{
					auto x = 0;
					while (x < vid_width)
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
				vmaUnmapMemory(Allocator, perFrame.allocation);
				CHECK_VKCMD(vmaFlushAllocation(Allocator, perFrame.allocation, 0, VK_WHOLE_SIZE));
			}
			else
			{
				auto console = Scene.consoleData.data();
				auto previousConsole = console;
				auto screen = Scene.screenData.data();
				void* mapped;
				CHECK_VKCMD(vmaMapMemory(Allocator, perFrame.allocation, &mapped));
				auto target = (uint32_t*)mapped;
				auto black = Scene.paletteData[0];
				auto y = 0;
				while (y < vid_height)
				{
					auto t = (y & 1) << 1;
					auto x = 0;
					while (x < vid_width)
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
				vmaUnmapMemory(Allocator, perFrame.allocation);
				CHECK_VKCMD(vmaFlushAllocation(Allocator, perFrame.allocation, 0, VK_WHOLE_SIZE));
			}
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto width = directRect.width * Constants::screenToConsoleMultiplier;
					auto height = directRect.height * Constants::screenToConsoleMultiplier;
					auto source = directRect.data;
					void* mapped;
					CHECK_VKCMD(vmaMapMemory(Allocator, perFrame.allocation, &mapped));
					auto target = (uint32_t*)mapped + (directRect.y * ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
					auto y = 0;
					while (y < height)
					{
						auto x = 0;
						while (x < width)
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
							target -= width;
							std::copy(target, target + width, target + ScreenWidth);
							target += ScreenWidth + width;
							y++;
						}
						target += ScreenWidth - width;
					}
					vmaUnmapMemory(Allocator, perFrame.allocation);
					CHECK_VKCMD(vmaFlushAllocation(Allocator, perFrame.allocation, 0, VK_WHOLE_SIZE));
				}
			}
		}
		else if (key_dest == key_game)
		{
			auto source = Scene.consoleData.data();
			void* mapped;
			CHECK_VKCMD(vmaMapMemory(Allocator, perFrame.allocation, &mapped));
			auto target = (uint32_t*)mapped;
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
			vmaUnmapMemory(Allocator, perFrame.allocation);
			CHECK_VKCMD(vmaFlushAllocation(Allocator, perFrame.allocation, 0, VK_WHOLE_SIZE));
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					source = directRect.data;
					CHECK_VKCMD(vmaMapMemory(Allocator, perFrame.allocation, &mapped));
					target = (uint32_t*)mapped + directRect.y * ConsoleWidth + directRect.x;
					for (auto y = 0; y < directRect.height; y++)
					{
						for (auto x = 0; x < directRect.width; x++)
						{
							*target++ = Scene.paletteData[*source++];
						}
						target += ConsoleWidth - directRect.width;
					}
					vmaUnmapMemory(Allocator, perFrame.allocation);
					CHECK_VKCMD(vmaFlushAllocation(Allocator, perFrame.allocation, 0, VK_WHOLE_SIZE));
				}
			}
		}
		else
		{
			auto source = Scene.consoleData.data();
			auto previousSource = source;
			void* mapped;
			CHECK_VKCMD(vmaMapMemory(Allocator, perFrame.allocation, &mapped));
			auto target = (uint32_t*)mapped;
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
			vmaUnmapMemory(Allocator, perFrame.allocation);
			CHECK_VKCMD(vmaFlushAllocation(Allocator, perFrame.allocation, 0, VK_WHOLE_SIZE));
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto width = directRect.width * Constants::screenToConsoleMultiplier;
					auto height = directRect.height * Constants::screenToConsoleMultiplier;
					source = directRect.data;
					CHECK_VKCMD(vmaMapMemory(Allocator, perFrame.allocation, &mapped));
					target = (uint32_t*)mapped + (directRect.y * ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
					auto y = 0;
					while (y < height)
					{
						auto x = 0;
						while (x < width)
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
							target -= width;
							std::copy(target, target + width, target + ScreenWidth);
							target += ScreenWidth + width;
							y++;
						}
						target += ScreenWidth - width;
					}
					vmaUnmapMemory(Allocator, perFrame.allocation);
					CHECK_VKCMD(vmaFlushAllocation(Allocator, perFrame.allocation, 0, VK_WHOLE_SIZE));
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
		CHECK_VKCMD(vmaCopyMemoryToAllocation(Allocator, ScreenData.data(), perFrame.allocation, 0, perFrame.size));
	}
}

