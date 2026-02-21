#pragma once

#include <openxr/openxr.h>

struct Actions
{
	XrActionSet ActionSet;
	XrAction Play1;
	XrAction Play2;
	XrAction JumpLeftHanded;
	XrAction JumpRightHanded;
	XrAction SwimDownLeftHanded;
	XrAction SwimDownRightHanded;
	XrAction Run;
	XrAction Fire;
	XrAction MoveX;
	XrAction MoveY;
	XrAction SwitchWeapon;
	XrAction Menu;
	XrAction MenuLeftHanded;
	XrAction MenuRightHanded;
	XrAction EnterTrigger;
	XrAction EnterNonTriggerLeftHanded;
	XrAction EnterNonTriggerRightHanded;
	XrAction EscapeY;
	XrAction EscapeNonY;
	XrAction Quit;
	XrAction Pose;
	XrAction LeftKeyPress;
	XrAction RightKeyPress;

	void Create(struct AppState& appState, XrInstance instance);
	static void LogAction(struct AppState& appState, XrAction action, const char* name);
	void Log(AppState& appState) const;
};
