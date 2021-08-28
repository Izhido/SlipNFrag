#include "Controller.h"
#include <common/xr_linear.h>

float* Controller::WriteVertices(float* vertices)
{
	auto position = SpaceLocation.pose.position;
	XrMatrix4x4f transform;
	XrMatrix4x4f_CreateFromQuaternion(&transform, &SpaceLocation.pose.orientation);
	
	XrVector3f untransformedForward { 0, 0, -1 };
	XrVector3f untransformedRight { -1, 0, 0 };
	XrVector3f untransformedUp { 0, 1, 0 };
	
	XrVector3f forward;
	XrMatrix4x4f_TransformVector3f(&forward, &transform, &untransformedForward);
	XrVector3f right;
	XrMatrix4x4f_TransformVector3f(&right, &transform, &untransformedRight);
	XrVector3f up;
	XrMatrix4x4f_TransformVector3f(&up, &transform, &untransformedUp);
	
	*vertices++ = position.x - right.x * 0.015 - up.x * 0.015;
	*vertices++ = position.y - right.y * 0.015 - up.y * 0.015;
	*vertices++ = position.z - right.z * 0.015 - up.z * 0.015;
	*vertices++ = position.x - right.x * 0.025 + up.x * 0.015;
	*vertices++ = position.y - right.y * 0.025 + up.y * 0.015;
	*vertices++ = position.z - right.z * 0.025 + up.z * 0.015;
	*vertices++ = position.x + right.x * 0.025 + up.x * 0.015;
	*vertices++ = position.y + right.y * 0.025 + up.y * 0.015;
	*vertices++ = position.z + right.z * 0.025 + up.z * 0.015;
	*vertices++ = position.x + right.x * 0.015 - up.x * 0.015;
	*vertices++ = position.y + right.y * 0.015 - up.y * 0.015;
	*vertices++ = position.z + right.z * 0.015 - up.z * 0.015;
	*vertices++ = position.x - right.x * 0.015 - up.x * 0.015 - forward.x * 0.15;
	*vertices++ = position.y - right.y * 0.015 - up.y * 0.015 - forward.y * 0.15;
	*vertices++ = position.z - right.z * 0.015 - up.z * 0.015 - forward.z * 0.15;
	*vertices++ = position.x - right.x * 0.015 + up.x * 0.015 - forward.x * 0.15;
	*vertices++ = position.y - right.y * 0.015 + up.y * 0.015 - forward.y * 0.15;
	*vertices++ = position.z - right.z * 0.015 + up.z * 0.015 - forward.z * 0.15;
	*vertices++ = position.x + right.x * 0.015 + up.x * 0.015 - forward.x * 0.15;
	*vertices++ = position.y + right.y * 0.015 + up.y * 0.015 - forward.y * 0.15;
	*vertices++ = position.z + right.z * 0.015 + up.z * 0.015 - forward.z * 0.15;
	*vertices++ = position.x + right.x * 0.015 - up.x * 0.015 - forward.x * 0.15;
	*vertices++ = position.y + right.y * 0.015 - up.y * 0.015 - forward.y * 0.15;
	*vertices++ = position.z + right.z * 0.015 - up.z * 0.015 - forward.z * 0.15;
	
	*vertices++ = position.x - right.x * 0.0025 - up.x * 0.0025;
	*vertices++ = position.y - right.y * 0.0025 - up.y * 0.0025;
	*vertices++ = position.z - right.z * 0.0025 - up.z * 0.0025;
	*vertices++ = position.x - right.x * 0.0025 + up.x * 0.0025;
	*vertices++ = position.y - right.y * 0.0025 + up.y * 0.0025;
	*vertices++ = position.z - right.z * 0.0025 + up.z * 0.0025;
	*vertices++ = position.x + right.x * 0.0025 + up.x * 0.0025;
	*vertices++ = position.y + right.y * 0.0025 + up.y * 0.0025;
	*vertices++ = position.z + right.z * 0.0025 + up.z * 0.0025;
	*vertices++ = position.x + right.x * 0.0025 - up.x * 0.0025;
	*vertices++ = position.y + right.y * 0.0025 - up.y * 0.0025;
	*vertices++ = position.z + right.z * 0.0025 - up.z * 0.0025;
	*vertices++ = position.x - right.x * 0.0025 - up.x * 0.0025 + forward.x;
	*vertices++ = position.y - right.y * 0.0025 - up.y * 0.0025 + forward.y;
	*vertices++ = position.z - right.z * 0.0025 - up.z * 0.0025 + forward.z;
	*vertices++ = position.x - right.x * 0.0025 + up.x * 0.0025 + forward.x;
	*vertices++ = position.y - right.y * 0.0025 + up.y * 0.0025 + forward.y;
	*vertices++ = position.z - right.z * 0.0025 + up.z * 0.0025 + forward.z;
	*vertices++ = position.x + right.x * 0.0025 + up.x * 0.0025 + forward.x;
	*vertices++ = position.y + right.y * 0.0025 + up.y * 0.0025 + forward.y;
	*vertices++ = position.z + right.z * 0.0025 + up.z * 0.0025 + forward.z;
	*vertices++ = position.x + right.x * 0.0025 - up.x * 0.0025 + forward.x;
	*vertices++ = position.y + right.y * 0.0025 - up.y * 0.0025 + forward.y;
	*vertices++ = position.z + right.z * 0.0025 - up.z * 0.0025 + forward.z;
	return vertices;
}

float* Controller::WriteAttributes(float* attributes)
{
	*attributes++ = 0;
	*attributes++ = 0.1;
	*attributes++ = 1;
	*attributes++ = 0.1;
	*attributes++ = 0;
	*attributes++ = 0.1;
	*attributes++ = 1;
	*attributes++ = 0.1;
	*attributes++ = 0;
	*attributes++ = 0.95;
	*attributes++ = 1;
	*attributes++ = 0.95;
	*attributes++ = 0;
	*attributes++ = 0.95;
	*attributes++ = 1;
	*attributes++ = 0.95;
	
	*attributes++ = 0;
	*attributes++ = 0.1;
	*attributes++ = 1;
	*attributes++ = 0.1;
	*attributes++ = 0;
	*attributes++ = 0.1;
	*attributes++ = 1;
	*attributes++ = 0.1;
	*attributes++ = 0;
	*attributes++ = 0;
	*attributes++ = 1;
	*attributes++ = 0;
	*attributes++ = 0;
	*attributes++ = 0;
	*attributes++ = 1;
	*attributes++ = 0;
	
	return attributes;
}

uint16_t* Controller::WriteIndices(uint16_t* indices, uint16_t offset)
{
	*indices++ = 0 + offset;
	*indices++ = 1 + offset;
	*indices++ = 2 + offset;
	*indices++ = 2 + offset;
	*indices++ = 3 + offset;
	*indices++ = 0 + offset;

	*indices++ = 0 + offset;
	*indices++ = 4 + offset;
	*indices++ = 5 + offset;
	*indices++ = 5 + offset;
	*indices++ = 1 + offset;
	*indices++ = 0 + offset;

	*indices++ = 1 + offset;
	*indices++ = 5 + offset;
	*indices++ = 6 + offset;
	*indices++ = 6 + offset;
	*indices++ = 2 + offset;
	*indices++ = 1 + offset;

	*indices++ = 2 + offset;
	*indices++ = 6 + offset;
	*indices++ = 7 + offset;
	*indices++ = 7 + offset;
	*indices++ = 3 + offset;
	*indices++ = 2 + offset;

	*indices++ = 3 + offset;
	*indices++ = 7 + offset;
	*indices++ = 4 + offset;
	*indices++ = 4 + offset;
	*indices++ = 0 + offset;
	*indices++ = 3 + offset;

	*indices++ = 5 + offset;
	*indices++ = 4 + offset;
	*indices++ = 7 + offset;
	*indices++ = 7 + offset;
	*indices++ = 6 + offset;
	*indices++ = 5 + offset;

	return indices;
}
