#include "AppState.h"

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
	auto target = (uint32_t*)(perFrame.stagingBuffer.mapped);
	auto count = ConsoleWidth * ConsoleHeight / 2;
	while (count > 0)
	{
		auto entry = *source++;
		*target++ = Scene.paletteData[entry];
		count--;
	}
}
