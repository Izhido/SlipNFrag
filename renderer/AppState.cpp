#include "AppState.h"
#include "vid_oxr.h"
#include "Constants.h"
#include "DirectRect.h"
#include "Locks.h"

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
							target -= width;
							std::copy(target, target + width, target + ScreenWidth);
							target += ScreenWidth + width;
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
			{
				std::lock_guard<std::mutex> lock(Locks::DirectRectMutex);
				for (auto& directRect : DirectRect::directRects)
				{
					source = directRect.data;
					target = (uint32_t*)(perFrame.stagingBuffer.mapped) + directRect.y * ConsoleWidth + directRect.x;
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
					source = directRect.data;
					target = (uint32_t*)(perFrame.stagingBuffer.mapped) + (directRect.y * ScreenWidth + directRect.x) * Constants::screenToConsoleMultiplier;
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
				}
			}
		}
	}
	else
	{
		if (Mode == AppNoGameDataMode && !NoGameDataLoaded)
		{
			std::copy(NoGameDataData.data(), NoGameDataData.data() + NoGameDataData.size(), ScreenData.data());
			NoGameDataLoaded = true;
		}
		auto count = (size_t)perFrame.stagingBuffer.size / sizeof(uint32_t);
		std::copy(ScreenData.data(), ScreenData.data() + count, (uint32_t*)perFrame.stagingBuffer.mapped);
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
