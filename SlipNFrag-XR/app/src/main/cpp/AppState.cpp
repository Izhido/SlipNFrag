#include "AppState.h"
#include "Constants.h"
#include "DirectRect.h"
#include "Locks.h"

void AppState::RenderScreen(uint32_t swapchainImageIndex)
{
	if (Mode == AppScreenMode || Mode == AppWorldMode)
	{
		if (Mode == AppScreenMode)
		{
			if (key_dest == key_game)
			{
				std::lock_guard<std::mutex> lock(Locks::RenderMutex);
				auto console = con_buffer.data();
				auto previousConsole = console;
				auto screen = vid_buffer.data();
				auto target = (uint32_t*)(Screen.PerImage[swapchainImageIndex].stagingBuffer.mapped);
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
								*target++ = d_8to24table[*screen++];
								x++;
							} while ((x % Constants::screenToConsoleMultiplier) != 0);
						}
						else
						{
							auto converted = d_8to24table[entry];
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
			}
			else
			{
				std::lock_guard<std::mutex> lock(Locks::RenderMutex);
				auto console = con_buffer.data();
				auto previousConsole = console;
				auto screen = vid_buffer.data();
				auto target = (uint32_t*)(Screen.PerImage[swapchainImageIndex].stagingBuffer.mapped);
				auto black = d_8to24table[0];
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
									*target++ = d_8to24table[*screen++];
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
							auto converted = d_8to24table[entry];
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
			}
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto width = directRect.width * Constants::screenToConsoleMultiplier;
					auto height = directRect.height * Constants::screenToConsoleMultiplier;
					auto source = directRect.data;
					auto target = (uint32_t*)(Screen.PerImage[swapchainImageIndex].stagingBuffer.mapped) + (directRect.y * ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
					auto y = 0;
					while (y < height)
					{
						auto x = 0;
						while (x < width)
						{
							auto entry = d_8to24table[*source++];
							do
							{
								*target++ = entry;
								x++;
							} while ((x % Constants::screenToConsoleMultiplier) != 0);
						}
						y++;
						while ((y % Constants::screenToConsoleMultiplier) != 0)
						{
							target += ScreenWidth - width;
							memcpy(target, target - ScreenWidth, width * sizeof(uint32_t));
							target += width;
							y++;
						}
						target += ScreenWidth - width;
					}
				}
			}
		}
		else if (key_dest == key_game)
		{
			std::lock_guard<std::mutex> lock(Locks::RenderMutex);
			auto source = con_buffer.data();
			auto target = (uint32_t*)(Screen.PerImage[swapchainImageIndex].stagingBuffer.mapped);
			auto count = ConsoleWidth * ConsoleHeight;
			while (count > 0)
			{
				auto entry = *source++;
				*target++ = d_8to24table[entry];
				count--;
			}
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto source = directRect.data;
					auto target = (uint32_t*)(Screen.PerImage[swapchainImageIndex].stagingBuffer.mapped) + directRect.y * ConsoleWidth + directRect.x;
					for (auto y = 0; y < directRect.height; y++)
					{
						for (auto x = 0; x < directRect.width; x++)
						{
							*target++ = d_8to24table[*source++];
						}
						target += ConsoleWidth - directRect.width;
					}
				}
			}
		}
		else
		{
			std::lock_guard<std::mutex> lock(Locks::RenderMutex);
			auto source = con_buffer.data();
			auto previousSource = source;
			auto target = (uint32_t*)(Screen.PerImage[swapchainImageIndex].stagingBuffer.mapped);
			auto black = d_8to24table[0];
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
						auto converted = d_8to24table[entry];
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
						*target++ = d_8to24table[entry];
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
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto width = directRect.width * Constants::screenToConsoleMultiplier;
					auto height = directRect.height * Constants::screenToConsoleMultiplier;
					auto source = directRect.data;
					auto target = (uint32_t*)(Screen.PerImage[swapchainImageIndex].stagingBuffer.mapped) + (directRect.y * ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
					auto y = 0;
					while (y < height)
					{
						auto x = 0;
						while (x < width)
						{
							auto entry = d_8to24table[*source++];
							do
							{
								*target++ = entry;
								x++;
							} while ((x % Constants::screenToConsoleMultiplier) != 0);
						}
						y++;
						while ((y % Constants::screenToConsoleMultiplier) != 0)
						{
							target += ScreenWidth - width;
							memcpy(target, target - ScreenWidth, width * sizeof(uint32_t));
							target += width;
							y++;
						}
						target += ScreenWidth - width;
					}
				}
			}
		}
	}
	else if (Mode == AppNoGameDataMode)
	{
		if (!NoGameDataLoaded)
		{
			memcpy(ScreenData.data(), NoGameDataData.data(), NoGameDataData.size() * sizeof(uint32_t));
			NoGameDataLoaded = true;
		}
		memcpy(Screen.PerImage[swapchainImageIndex].stagingBuffer.mapped, ScreenData.data(), Screen.PerImage[swapchainImageIndex].stagingBuffer.size);
	}
	else
	{
		memcpy(Screen.PerImage[swapchainImageIndex].stagingBuffer.mapped, ScreenData.data(), Screen.PerImage[swapchainImageIndex].stagingBuffer.size);
	}
}

void AppState::RenderKeyboard(uint32_t swapchainImageIndex)
{
	auto source = Keyboard.buffer.data();
	auto target = (uint32_t*)(Keyboard.Screen.PerImage[swapchainImageIndex].stagingBuffer.mapped);
	auto count = ConsoleWidth * ConsoleHeight / 2;
	while (count > 0)
	{
		auto entry = *source++;
		*target++ = d_8to24table[entry];
		count--;
	}
}
