#include "AppState.h"
#include "Constants.h"
#include "DirectRect.h"
#include "Locks.h"

void AppState::RenderScreen(ScreenPerFrame& perFrame)
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
				auto target = (uint32_t*)(perFrame.stagingBuffer.mapped);
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
			}
			else
			{
				auto console = Scene.consoleData.data();
				auto previousConsole = console;
				auto screen = Scene.screenData.data();
				auto target = (uint32_t*)(perFrame.stagingBuffer.mapped);
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
			}
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto width = directRect.width * Constants::screenToConsoleMultiplier;
					auto height = directRect.height * Constants::screenToConsoleMultiplier;
					auto source = directRect.data;
					auto target = (uint32_t*)(perFrame.stagingBuffer.mapped) + (directRect.y * ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
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
			auto source = Scene.consoleData.data();
			auto target = (uint32_t*)(perFrame.stagingBuffer.mapped);
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
					memset(target, 0, leading * sizeof(uint32_t));
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
				memset(target, 0, ConsoleWidth * ConsoleHeight * sizeof(uint32_t));
			}
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto source = directRect.data;
					auto target = (uint32_t*)(perFrame.stagingBuffer.mapped) + directRect.y * ConsoleWidth + directRect.x;
					for (auto y = 0; y < directRect.height; y++)
					{
						for (auto x = 0; x < directRect.width; x++)
						{
							*target++ = Scene.paletteData[*source++];
						}
						target += ConsoleWidth - directRect.width;
					}
				}
			}
		}
		else
		{
			auto source = Scene.consoleData.data();
			auto previousSource = source;
			auto target = (uint32_t*)(perFrame.stagingBuffer.mapped);
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
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					auto width = directRect.width * Constants::screenToConsoleMultiplier;
					auto height = directRect.height * Constants::screenToConsoleMultiplier;
					auto source = directRect.data;
					auto target = (uint32_t*)(perFrame.stagingBuffer.mapped) + (directRect.y * ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
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
		memcpy(perFrame.stagingBuffer.mapped, ScreenData.data(), perFrame.stagingBuffer.size);
	}
	else
	{
		memcpy(perFrame.stagingBuffer.mapped, ScreenData.data(), perFrame.stagingBuffer.size);
	}
}

void AppState::RenderKeyboard(ScreenPerFrame& perFrame)
{
	auto source = Keyboard.buffer.data();
	auto target = (uint32_t*)(perFrame.stagingBuffer.mapped);
	auto count = ConsoleWidth * ConsoleHeight / 2;
	while (count > 0)
	{
		auto entry = *source++;
		*target++ = Scene.paletteData[entry];
		count--;
	}
}
