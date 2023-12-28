#include "Keyboard.h"
#include "AppState.h"
#include "CylinderProjection.h"
#include "Input.h"
#include "Utils.h"

extern byte* draw_chars;

std::vector<std::vector<KeyboardCell>> Keyboard::cells
{
	{
		{ 40, 0, 16, 16, 4, 4, "`"},
		{ 56, 0, 16, 16, 4, 4, "1"},
		{ 72, 0, 16, 16, 4, 4, "2"},
		{ 88, 0, 16, 16, 4, 4, "3"},
		{ 104, 0, 16, 16, 4, 4, "4"},
		{ 120, 0, 16, 16, 4, 4, "5"},
		{ 136, 0, 16, 16, 4, 4, "6"},
		{ 152, 0, 16, 16, 4, 4, "7"},
		{ 168, 0, 16, 16, 4, 4, "8"},
		{ 184, 0, 16, 16, 4, 4, "9"},
		{ 200, 0, 16, 16, 4, 4, "0"},
		{ 216, 0, 16, 16, 4, 4, "-"},
		{ 232, 0, 16, 16, 4, 4, "="},
		{ 248, 0, 32, 16, 0, 4, "Bksp"},
		{ 40, 16, 24, 16, 0, 4, "Tab"},
		{ 64, 16, 16, 16, 4, 4, "q"},
		{ 80, 16, 16, 16, 4, 4, "w"},
		{ 96, 16, 16, 16, 4, 4, "e"},
		{ 112, 16, 16, 16, 4, 4, "r"},
		{ 128, 16, 16, 16, 4, 4, "t"},
		{ 144, 16, 16, 16, 4, 4, "y"},
		{ 160, 16, 16, 16, 4, 4, "u"},
		{ 176, 16, 16, 16, 4, 4, "i"},
		{ 192, 16, 16, 16, 4, 4, "o"},
		{ 208, 16, 16, 16, 4, 4, "p"},
		{ 224, 16, 16, 16, 4, 4, "["},
		{ 240, 16, 16, 16, 4, 4, "]"},
		{ 256, 16, 24, 16, 4, 4, "\\"},
		{ 40, 32, 32, 16, 0, 4, "Caps"},
		{ 72, 32, 16, 16, 4, 4, "a"},
		{ 88, 32, 16, 16, 4, 4, "s"},
		{ 104, 32, 16, 16, 4, 4, "d"},
		{ 120, 32, 16, 16, 4, 4, "f"},
		{ 136, 32, 16, 16, 4, 4, "g"},
		{ 152, 32, 16, 16, 4, 4, "h"},
		{ 168, 32, 16, 16, 4, 4, "j"},
		{ 184, 32, 16, 16, 4, 4, "k"},
		{ 200, 32, 16, 16, 4, 4, "l"},
		{ 216, 32, 16, 16, 4, 4, ";"},
		{ 232, 32, 16, 16, 4, 4, "'"},
		{ 248, 32, 32, 16, 0, 4, "Entr"},
		{ 40, 48, 40, 16, 0, 4, "Shift"},
		{ 80, 48, 16, 16, 4, 4, "z"},
		{ 96, 48, 16, 16, 4, 4, "x"},
		{ 112, 48, 16, 16, 4, 4, "c"},
		{ 128, 48, 16, 16, 4, 4, "v"},
		{ 144, 48, 16, 16, 4, 4, "b"},
		{ 160, 48, 16, 16, 4, 4, "n"},
		{ 176, 48, 16, 16, 4, 4, "m"},
		{ 192, 48, 16, 16, 4, 4, ","},
		{ 208, 48, 16, 16, 4, 4, "."},
		{ 224, 48, 16, 16, 4, 4, "/"},
		{ 240, 48, 40, 16, 0, 4, "Shift"},
		{ 40, 64, 32, 16, 0, 4, "Ctrl"},
		{ 72, 64, 32, 16, 4, 4, "Alt"},
		{ 104, 64, 112, 16, 36, 4, "Space"},
		{ 216, 64, 32, 16, 4, 4, "Alt"},
		{ 248, 64, 32, 16, 0, 4, "Ctrl"}
	},
	{
		{ 40, 0, 16, 16, 4, 4, "`"},
		{ 56, 0, 16, 16, 4, 4, "1"},
		{ 72, 0, 16, 16, 4, 4, "2"},
		{ 88, 0, 16, 16, 4, 4, "3"},
		{ 104, 0, 16, 16, 4, 4, "4"},
		{ 120, 0, 16, 16, 4, 4, "5"},
		{ 136, 0, 16, 16, 4, 4, "6"},
		{ 152, 0, 16, 16, 4, 4, "7"},
		{ 168, 0, 16, 16, 4, 4, "8"},
		{ 184, 0, 16, 16, 4, 4, "9"},
		{ 200, 0, 16, 16, 4, 4, "0"},
		{ 216, 0, 16, 16, 4, 4, "-"},
		{ 232, 0, 16, 16, 4, 4, "="},
		{ 248, 0, 32, 16, 0, 4, "Bksp"},
		{ 40, 16, 24, 16, 0, 4, "Tab"},
		{ 64, 16, 16, 16, 4, 4, "Q"},
		{ 80, 16, 16, 16, 4, 4, "W"},
		{ 96, 16, 16, 16, 4, 4, "E"},
		{ 112, 16, 16, 16, 4, 4, "R"},
		{ 128, 16, 16, 16, 4, 4, "T"},
		{ 144, 16, 16, 16, 4, 4, "Y"},
		{ 160, 16, 16, 16, 4, 4, "U"},
		{ 176, 16, 16, 16, 4, 4, "I"},
		{ 192, 16, 16, 16, 4, 4, "O"},
		{ 208, 16, 16, 16, 4, 4, "P"},
		{ 224, 16, 16, 16, 4, 4, "["},
		{ 240, 16, 16, 16, 4, 4, "]"},
		{ 256, 16, 24, 16, 4, 4, "\\"},
		{ 40, 32, 32, 16, 0, 4, "CAPS"},
		{ 72, 32, 16, 16, 4, 4, "A"},
		{ 88, 32, 16, 16, 4, 4, "S"},
		{ 104, 32, 16, 16, 4, 4, "D"},
		{ 120, 32, 16, 16, 4, 4, "F"},
		{ 136, 32, 16, 16, 4, 4, "G"},
		{ 152, 32, 16, 16, 4, 4, "H"},
		{ 168, 32, 16, 16, 4, 4, "J"},
		{ 184, 32, 16, 16, 4, 4, "K"},
		{ 200, 32, 16, 16, 4, 4, "L"},
		{ 216, 32, 16, 16, 4, 4, ";"},
		{ 232, 32, 16, 16, 4, 4, "'"},
		{ 248, 32, 32, 16, 0, 4, "Entr"},
		{ 40, 48, 40, 16, 0, 4, "Shift"},
		{ 80, 48, 16, 16, 4, 4, "Z"},
		{ 96, 48, 16, 16, 4, 4, "X"},
		{ 112, 48, 16, 16, 4, 4, "C"},
		{ 128, 48, 16, 16, 4, 4, "V"},
		{ 144, 48, 16, 16, 4, 4, "B"},
		{ 160, 48, 16, 16, 4, 4, "N"},
		{ 176, 48, 16, 16, 4, 4, "M"},
		{ 192, 48, 16, 16, 4, 4, ","},
		{ 208, 48, 16, 16, 4, 4, "."},
		{ 224, 48, 16, 16, 4, 4, "/"},
		{ 240, 48, 40, 16, 0, 4, "Shift"},
		{ 40, 64, 32, 16, 0, 4, "Ctrl"},
		{ 72, 64, 32, 16, 4, 4, "Alt"},
		{ 104, 64, 112, 16, 36, 4, "Space"},
		{ 216, 64, 32, 16, 4, 4, "Alt"},
		{ 248, 64, 32, 16, 0, 4, "Ctrl"}
	},
	{
		{ 40, 0, 16, 16, 4, 4, "~"},
		{ 56, 0, 16, 16, 4, 4, "!"},
		{ 72, 0, 16, 16, 4, 4, "@"},
		{ 88, 0, 16, 16, 4, 4, "#"},
		{ 104, 0, 16, 16, 4, 4, "$"},
		{ 120, 0, 16, 16, 4, 4, "%"},
		{ 136, 0, 16, 16, 4, 4, "^"},
		{ 152, 0, 16, 16, 4, 4, "&"},
		{ 168, 0, 16, 16, 4, 4, "*"},
		{ 184, 0, 16, 16, 4, 4, "("},
		{ 200, 0, 16, 16, 4, 4, ")"},
		{ 216, 0, 16, 16, 4, 4, "_"},
		{ 232, 0, 16, 16, 4, 4, "+"},
		{ 248, 0, 32, 16, 0, 4, "Bksp"},
		{ 40, 16, 24, 16, 0, 4, "Tab"},
		{ 64, 16, 16, 16, 4, 4, "Q"},
		{ 80, 16, 16, 16, 4, 4, "W"},
		{ 96, 16, 16, 16, 4, 4, "E"},
		{ 112, 16, 16, 16, 4, 4, "R"},
		{ 128, 16, 16, 16, 4, 4, "T"},
		{ 144, 16, 16, 16, 4, 4, "Y"},
		{ 160, 16, 16, 16, 4, 4, "U"},
		{ 176, 16, 16, 16, 4, 4, "I"},
		{ 192, 16, 16, 16, 4, 4, "O"},
		{ 208, 16, 16, 16, 4, 4, "P"},
		{ 224, 16, 16, 16, 4, 4, "{"},
		{ 240, 16, 16, 16, 4, 4, "}"},
		{ 256, 16, 24, 16, 4, 4, "|"},
		{ 40, 32, 32, 16, 0, 4, "Caps"},
		{ 72, 32, 16, 16, 4, 4, "A"},
		{ 88, 32, 16, 16, 4, 4, "S"},
		{ 104, 32, 16, 16, 4, 4, "D"},
		{ 120, 32, 16, 16, 4, 4, "F"},
		{ 136, 32, 16, 16, 4, 4, "G"},
		{ 152, 32, 16, 16, 4, 4, "H"},
		{ 168, 32, 16, 16, 4, 4, "J"},
		{ 184, 32, 16, 16, 4, 4, "K"},
		{ 200, 32, 16, 16, 4, 4, "L"},
		{ 216, 32, 16, 16, 4, 4, ":"},
		{ 232, 32, 16, 16, 4, 4, "\""},
		{ 248, 32, 32, 16, 0, 4, "Entr"},
		{ 40, 48, 40, 16, 0, 4, "SHIFT"},
		{ 80, 48, 16, 16, 4, 4, "Z"},
		{ 96, 48, 16, 16, 4, 4, "X"},
		{ 112, 48, 16, 16, 4, 4, "C"},
		{ 128, 48, 16, 16, 4, 4, "V"},
		{ 144, 48, 16, 16, 4, 4, "B"},
		{ 160, 48, 16, 16, 4, 4, "N"},
		{ 176, 48, 16, 16, 4, 4, "M"},
		{ 192, 48, 16, 16, 4, 4, "<"},
		{ 208, 48, 16, 16, 4, 4, ">"},
		{ 224, 48, 16, 16, 4, 4, "?"},
		{ 240, 48, 40, 16, 0, 4, "SHIFT"},
		{ 40, 64, 32, 16, 0, 4, "Ctrl"},
		{ 72, 64, 32, 16, 4, 4, "Alt"},
		{ 104, 64, 112, 16, 36, 4, "Space"},
		{ 216, 64, 32, 16, 4, 4, "Alt"},
		{ 248, 64, 32, 16, 0, 4, "Ctrl"}
	},
	{
		{ 40, 0, 16, 16, 4, 4, "~"},
		{ 56, 0, 16, 16, 4, 4, "!"},
		{ 72, 0, 16, 16, 4, 4, "@"},
		{ 88, 0, 16, 16, 4, 4, "#"},
		{ 104, 0, 16, 16, 4, 4, "$"},
		{ 120, 0, 16, 16, 4, 4, "%"},
		{ 136, 0, 16, 16, 4, 4, "^"},
		{ 152, 0, 16, 16, 4, 4, "&"},
		{ 168, 0, 16, 16, 4, 4, "*"},
		{ 184, 0, 16, 16, 4, 4, "("},
		{ 200, 0, 16, 16, 4, 4, ")"},
		{ 216, 0, 16, 16, 4, 4, "_"},
		{ 232, 0, 16, 16, 4, 4, "+"},
		{ 248, 0, 32, 16, 0, 4, "Bksp"},
		{ 40, 16, 24, 16, 0, 4, "Tab"},
		{ 64, 16, 16, 16, 4, 4, "q"},
		{ 80, 16, 16, 16, 4, 4, "w"},
		{ 96, 16, 16, 16, 4, 4, "e"},
		{ 112, 16, 16, 16, 4, 4, "r"},
		{ 128, 16, 16, 16, 4, 4, "t"},
		{ 144, 16, 16, 16, 4, 4, "y"},
		{ 160, 16, 16, 16, 4, 4, "u"},
		{ 176, 16, 16, 16, 4, 4, "i"},
		{ 192, 16, 16, 16, 4, 4, "o"},
		{ 208, 16, 16, 16, 4, 4, "p"},
		{ 224, 16, 16, 16, 4, 4, "{"},
		{ 240, 16, 16, 16, 4, 4, "}"},
		{ 256, 16, 24, 16, 4, 4, "|"},
		{ 40, 32, 32, 16, 0, 4, "CAPS"},
		{ 72, 32, 16, 16, 4, 4, "a"},
		{ 88, 32, 16, 16, 4, 4, "s"},
		{ 104, 32, 16, 16, 4, 4, "d"},
		{ 120, 32, 16, 16, 4, 4, "f"},
		{ 136, 32, 16, 16, 4, 4, "g"},
		{ 152, 32, 16, 16, 4, 4, "h"},
		{ 168, 32, 16, 16, 4, 4, "j"},
		{ 184, 32, 16, 16, 4, 4, "k"},
		{ 200, 32, 16, 16, 4, 4, "l"},
		{ 216, 32, 16, 16, 4, 4, ":"},
		{ 232, 32, 16, 16, 4, 4, "\""},
		{ 248, 32, 32, 16, 0, 4, "Entr"},
		{ 40, 48, 40, 16, 0, 4, "SHIFT"},
		{ 80, 48, 16, 16, 4, 4, "z"},
		{ 96, 48, 16, 16, 4, 4, "x"},
		{ 112, 48, 16, 16, 4, 4, "c"},
		{ 128, 48, 16, 16, 4, 4, "v"},
		{ 144, 48, 16, 16, 4, 4, "b"},
		{ 160, 48, 16, 16, 4, 4, "n"},
		{ 176, 48, 16, 16, 4, 4, "m"},
		{ 192, 48, 16, 16, 4, 4, "<"},
		{ 208, 48, 16, 16, 4, 4, ">"},
		{ 224, 48, 16, 16, 4, 4, "?"},
		{ 240, 48, 40, 16, 0, 4, "SHIFT"},
		{ 40, 64, 32, 16, 0, 4, "Ctrl"},
		{ 72, 64, 32, 16, 4, 4, "Alt"},
		{ 104, 64, 112, 16, 36, 4, "Space"},
		{ 216, 64, 32, 16, 4, 4, "Alt"},
		{ 248, 64, 32, 16, 0, 4, "Ctrl"}
	},
};

void Keyboard::Create(AppState& appState)
{
	buffer.resize(appState.ConsoleWidth * appState.ConsoleHeight / 2, 255);
}

void Keyboard::AddKeyInput(int key, bool down)
{
	auto& source = cells[(int)layout];
	auto& cell = source[key];
	auto text = cell.text;
	if (text[1] == 0)
	{
		Input::AddKeyInput(text[0], down);
	}
	else if (strcmp(text, "Bksp") == 0)
	{
		Input::AddKeyInput(K_BACKSPACE, down);
	}
	else if (strcmp(text, "Tab") == 0)
	{
		Input::AddKeyInput(K_TAB, down);
	}
	else if (strcmp(text, "Entr") == 0)
	{
		Input::AddKeyInput(K_ENTER, down);
	}
	else if (strcmp(text, "Ctrl") == 0)
	{
		Input::AddKeyInput(K_CTRL, down);
	}
	else if (strcmp(text, "Alt") == 0)
	{
		Input::AddKeyInput(K_ALT, down);
	}
	else if (strcmp(text, "Space") == 0)
	{
		Input::AddKeyInput(K_SPACE, down);
	}
	else if (strcmp(text, "Caps") == 0 || strcmp(text, "CAPS") == 0)
	{
		if (down)
		{
			if (layout == RegularKeys)
			{
				layout = CapsKeys;
			}
			else if (layout == CapsKeys)
			{
				layout = RegularKeys;
			}
			else if (layout == ShiftKeys)
			{
				layout = CapsShiftKeys;
			}
			else
			{
				layout = ShiftKeys;
			}
		}
	}
	else if (strcmp(text, "Shift") == 0 || strcmp(text, "SHIFT") == 0)
	{
		if (down)
		{
			if (layout == RegularKeys)
			{
				layout = ShiftKeys;
			}
			else if (layout == ShiftKeys)
			{
				layout = RegularKeys;
			}
			else if (layout == CapsKeys)
			{
				layout = CapsShiftKeys;
			}
			else
			{
				layout = CapsKeys;
			}
		}
	}
}

bool Keyboard::Handle(AppState& appState)
{
	if (draw_chars == nullptr || (key_dest != key_console && key_dest != key_menu && appState.Mode != AppScreenMode))
	{
		if (leftPressed >= 0)
		{
			AddKeyInput(leftPressed, false);
			leftPressed = -1;
		}
		if (rightPressed >= 0)
		{
			AddKeyInput(rightPressed, false);
			rightPressed = -1;
		}
		return false;
	}
	if (appState.Mode == AppWorldMode && key_dest == key_console)
	{
		appState.KeyboardHitOffsetY = -CylinderProjection::keyboardLowerLimit / 6;
	}
	else
	{
		appState.KeyboardHitOffsetY = -CylinderProjection::screenLowerLimit;
	}
	leftHighlighted = -1;
	if (appState.LeftController.PoseIsValid)
	{
		float x;
		float y;
		bool hit;
		if (appState.Mode == AppScreenMode)
		{
			hit = CylinderProjection::HitPointForScreenMode(appState, appState.LeftController, x, y);
		}
		else
		{
			hit = CylinderProjection::HitPoint(appState, appState.LeftController, x, y);
		}
		if (hit)
		{
			auto horizontal = (int)(x * appState.ConsoleWidth);
			auto vertical = (int)(y * appState.ConsoleHeight) - appState.ConsoleHeight / 2;
			auto& source = cells[(int)layout];
			for (auto i = 0; i < source.size(); i++)
			{
				auto& cell = source[i];
				if (horizontal >= cell.left && horizontal < cell.left + cell.width && vertical >= cell.top && vertical < cell.top + cell.height)
				{
					leftHighlighted = i;
					break;
				}
			}
		}
	}
	rightHighlighted = -1;
	if (appState.RightController.PoseIsValid)
	{
		float x;
		float y;
		bool hit;
		if (appState.Mode == AppScreenMode)
		{
			hit = CylinderProjection::HitPointForScreenMode(appState, appState.RightController, x, y);
		}
		else
		{
			hit = CylinderProjection::HitPoint(appState, appState.RightController, x, y);
		}
		if (hit)
		{
			auto horizontal = (int)(x * appState.ConsoleWidth);
			auto vertical = (int)(y * appState.ConsoleHeight) - appState.ConsoleHeight / 2;
			auto& source = cells[(int)layout];
			for (auto i = 0; i < source.size(); i++)
			{
				auto& cell = source[i];
				if (horizontal >= cell.left && horizontal < cell.left + cell.width && vertical >= cell.top && vertical < cell.top + cell.height)
				{
					rightHighlighted = i;
					break;
				}
			}
		}
	}
	
	auto keyPressHandled = false;
	
	XrActionStateGetInfo actionGetInfo { XR_TYPE_ACTION_STATE_GET_INFO };
	XrActionStateBoolean booleanActionState { XR_TYPE_ACTION_STATE_BOOLEAN };
	
	actionGetInfo.action = appState.LeftKeyPressAction;
	CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
	if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
	{
		if (booleanActionState.currentState)
		{
			if (leftHighlighted >= 0 && leftPressed < 0)
			{
				leftPressed = leftHighlighted;
				AddKeyInput(leftPressed, true);
				keyPressHandled = true;
			}
		}
		else if (leftPressed >= 0)
		{
			AddKeyInput(leftPressed, false);
			leftPressed = -1;
			keyPressHandled = true;
		}
	}

	actionGetInfo.action = appState.RightKeyPressAction;
	CHECK_XRCMD(xrGetActionStateBoolean(appState.Session, &actionGetInfo, &booleanActionState));
	if (booleanActionState.isActive && booleanActionState.changedSinceLastSync)
	{
		if (booleanActionState.currentState)
		{
			if (rightHighlighted >= 0 && rightPressed < 0)
			{
				rightPressed = rightHighlighted;
				AddKeyInput(rightPressed, true);
				keyPressHandled = true;
			}
		}
		else if (rightPressed >= 0)
		{
			AddKeyInput(rightPressed, false);
			rightPressed = -1;
			keyPressHandled = true;
		}
	}

	return keyPressHandled;
}

void Keyboard::Fill(AppState& appState, KeyboardCell& cell, unsigned char color)
{
	auto target = buffer.data() + cell.top * appState.ConsoleWidth + cell.left;
	for (auto j = cell.height; j > 0; j--)
	{
		for (auto i = cell.width; i > 0; i--)
		{
			*target++ = color;
		}
		target += appState.ConsoleWidth - cell.width;
	}
}

void Keyboard::Print(AppState& appState, KeyboardCell& cell, bool upper)
{
	auto start = buffer.data() + (cell.top + cell.offsetY) * appState.ConsoleWidth + cell.left + cell.offsetX;
	for (auto k = 0; cell.text[k] != 0; k++)
	{
		auto num = cell.text[k];
		if (upper)
		{
			num += 128;
		}
		auto row = num>>4;
		auto col = num&15;
		auto source = draw_chars + (row<<10) + (col<<3);
		auto target = start;
		for (auto j = 8; j > 0; j--)
		{
			for (auto i = 8; i > 0; i--)
			{
				auto c = *source++;
				if (c > 0)
				{
					*target = c;
				}
				target++;
			}
			target += appState.ConsoleWidth - 8;
			source += 128 - 8;
		}
		start += 8;
	}
}

bool Keyboard::Draw(AppState& appState)
{
	if (draw_chars == nullptr || (key_dest != key_console && key_dest != key_menu && appState.Mode != AppScreenMode))
	{
		return false;
	}
	auto& source = cells[(int)layout];
	for (auto i = 0; i < source.size(); i++)
	{
		auto& cell = source[i];
		if (i == leftPressed)
		{
			Fill(appState, cell, 15);
			Print(appState, cell, true);
			continue;
		}
		if (i == rightPressed)
		{
			Fill(appState, cell, 15);
			Print(appState, cell, true);
			continue;
		}
		if (i == leftHighlighted && leftPressed == -1)
		{
			Fill(appState, cell, 3);
			Print(appState, cell, false);
			continue;
		}
		if (i == rightHighlighted && rightPressed == -1)
		{
			Fill(appState, cell, 3);
			Print(appState, cell, false);
			continue;
		}
		if (layout != RegularKeys)
		{
			auto text = cell.text;
			if (((layout == CapsKeys || layout == CapsShiftKeys) && (strcmp(text, "Caps") == 0 || strcmp(text, "CAPS") == 0)) || ((layout == ShiftKeys || layout == CapsShiftKeys) && (strcmp(text, "Shift") == 0 || strcmp(text, "SHIFT") == 0)))
			{
				Fill(appState, cell, 7);
				Print(appState, cell, false);
				continue;
			}
		}
		Fill(appState, cell, 255);
		Print(appState, cell, false);
	}
	return true;
}
