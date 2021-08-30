#include "AppState.h"
#include "Constants.h"
#include "DirectRect.h"

void AppState::RenderScreen()
{
	if (Mode == AppScreenMode || Mode == AppWorldMode)
	{
		if (Mode == AppScreenMode)
		{
			if (key_dest == key_game)
			{
				std::lock_guard<std::mutex> lock(RenderMutex);
				auto console = con_buffer.data();
				auto previousConsole = console;
				auto screen = vid_buffer.data();
				auto target = Screen.Data.data();
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
				std::lock_guard<std::mutex> lock(RenderMutex);
				auto console = con_buffer.data();
				auto previousConsole = console;
				auto screen = vid_buffer.data();
				auto target = Screen.Data.data();
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
		}
		else if (key_dest == key_game)
		{
			std::lock_guard<std::mutex> lock(RenderMutex);
			auto source = con_buffer.data();
			auto target = Screen.Data.data();
			auto y = 0;
			auto limit = ScreenHeight - (SBAR_HEIGHT + 24) * Constants::screenToConsoleMultiplier;
			while (y < limit)
			{
				auto x = 0;
				while (x < ScreenWidth)
				{
					auto entry = *source++;
					if (entry == 255)
					{
						do
						{
							*target++ = 0;
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
				while ((y % Constants::screenToConsoleMultiplier) != 0)
				{
					memcpy(target, target - ScreenWidth, ScreenWidth * sizeof(uint32_t));
					target += ScreenWidth;
					y++;
				}
			}
			limit = ScreenHeight - SBAR_HEIGHT - 24;
			memset(target, 0, (limit - y) * ScreenWidth * sizeof(uint32_t));
			target += (limit - y) * ScreenWidth;
			y = limit;
			limit = ScreenWidth / Constants::screenToConsoleMultiplier;
			while (y < ScreenHeight)
			{
				auto x = 0;
				while (x < limit)
				{
					auto entry = *source++;
					if (entry == 255)
					{
						*target++ = 0;
					}
					else
					{
						*target++ = d_8to24table[entry];
					}
					x++;
				}
				while (x < ScreenWidth)
				{
					*target++ = 0;
					x++;
				}
				y++;
			}
		}
		else
		{
			std::lock_guard<std::mutex> lock(RenderMutex);
			auto source = con_buffer.data();
			auto target = Screen.Data.data();
			auto black = d_8to24table[0];
			auto y = 0;
			auto limit = ScreenHeight - (SBAR_HEIGHT + 24) * Constants::screenToConsoleMultiplier;
			auto oneLine = ScreenWidth * Constants::screenToConsoleMultiplier;
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
				while ((y % Constants::screenToConsoleMultiplier) != 0)
				{
					memcpy(target, target - ScreenWidth, ScreenWidth * sizeof(uint32_t));
					target += ScreenWidth;
					y++;
				}
			}
			limit = ScreenHeight - SBAR_HEIGHT - 24;
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
		}
		{
			std::lock_guard<std::mutex> lock(DirectRect::DirectRectMutex);
			for (auto& directRect : DirectRect::directRects)
			{
				auto width = directRect.width * Constants::screenToConsoleMultiplier;
				auto height = directRect.height * Constants::screenToConsoleMultiplier;
				auto directRectIndex = 0;
				auto directRectIndexCache = 0;
				auto target = Screen.Data.data() + (directRect.y * ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
				auto y = 0;
				while (y < height)
				{
					auto x = 0;
					while (x < width)
					{
						auto entry = d_8to24table[directRect.data[directRectIndex]];
						do
						{
							*target++ = entry;
							x++;
						} while ((x % Constants::screenToConsoleMultiplier) != 0);
						directRectIndex++;
					}
					y++;
					if ((y % Constants::screenToConsoleMultiplier) == 0)
					{
						directRectIndexCache = directRectIndex;
					}
					else
					{
						directRectIndex = directRectIndexCache;
					}
					target += ScreenWidth - width;
				}
			}
		}
	}
	else if (Mode == AppNoGameDataMode)
	{
		static auto noGameDataLoaded = false;
		if (!noGameDataLoaded)
		{
			memcpy(Screen.Data.data(), NoGameDataData.data(), NoGameDataData.size() * sizeof(uint32_t));
			noGameDataLoaded = true;
		}
	}
	memcpy(Screen.StagingBuffer.mapped, Screen.Data.data(), Screen.StagingBuffer.size);
}
