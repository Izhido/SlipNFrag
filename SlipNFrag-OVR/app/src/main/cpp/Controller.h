#pragma once

struct Controller
{
	uint32_t PreviousButtons;
	uint32_t Buttons;
	ovrTracking Tracking;
	ovrResult TrackingResult;
	ovrVector2f PreviousJoystick;
	ovrVector2f Joystick;
};
