#include "CylinderProjection.h"
#include "AppState.h"
#include <android/log.h>

const float CylinderProjection::density = 4500.0f;
const float CylinderProjection::radius = 1.0f;
const float CylinderProjection::horizontalAngle = 76.75 * M_PI / 180;
const float CylinderProjection::verticalAngle = horizontalAngle / 1.6;
const float CylinderProjection::epsilon = 1e-5;

void CylinderProjection::Setup(AppState& appState, ovrLayerCylinder2& layer)
{
	layer.Header.ColorScale.x = 1;
	layer.Header.ColorScale.y = 1;
	layer.Header.ColorScale.z = 1;
	layer.Header.ColorScale.w = 1;
	layer.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA;
	layer.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA;
	layer.HeadPose = appState.Tracking.HeadPose;
}

void CylinderProjection::Setup(AppState& appState, ovrLayerCylinder2& layer, float yaw, ovrTextureSwapChain* swapChain)
{
	Setup(appState, layer);
	const auto pitch = 0.0f;
	const auto scaleMatrix = ovrMatrix4f_CreateScale(radius, radius * (float) appState.ScreenHeight * VRAPI_PI / density, radius);
	const auto rotXMatrix = ovrMatrix4f_CreateRotation(pitch, 0.0f, 0.0f);
	const auto rotYMatrix = ovrMatrix4f_CreateRotation(0.0f, yaw, 0.0f);
	const auto m0 = ovrMatrix4f_Multiply(&rotXMatrix, &scaleMatrix);
	const auto transform = ovrMatrix4f_Multiply(&rotYMatrix, &m0);
	Setup(appState, layer, &transform, swapChain, 0);
}

void CylinderProjection::Setup(AppState& appState, ovrLayerCylinder2& layer, const ovrMatrix4f* transform, ovrTextureSwapChain* swapChain, int index)
{
	const auto circScale = density * 0.5f / appState.ScreenWidth;
	const auto circBias = -circScale * (0.5f * (1.0f - 1.0f / circScale));
	for (auto i = 0; i < VRAPI_FRAME_LAYER_EYE_MAX; i++)
	{
		auto& texture = layer.Textures[i];
		const auto modelViewMatrix = ovrMatrix4f_Multiply(&appState.Tracking.Eye[i].ViewMatrix, transform);
		texture.TexCoordsFromTanAngles = ovrMatrix4f_Inverse(&modelViewMatrix);
		texture.ColorSwapChain = swapChain;
		texture.SwapChainIndex = index;
		const float texScaleX = circScale;
		const float texBiasX = circBias;
		const float texScaleY = 0.5;
		const float texBiasY = -texScaleY * (0.5 * (1 - 1 / texScaleY));
		texture.TextureMatrix.M[0][0] = texScaleX;
		texture.TextureMatrix.M[0][2] = texBiasX;
		texture.TextureMatrix.M[1][1] = texScaleY;
		texture.TextureMatrix.M[1][2] = texBiasY;
		texture.TextureRect.width = 1;
		texture.TextureRect.height = 1;
	}
}

bool CylinderProjection::HitPoint(AppState& appState, Controller& controller, float& x, float& y)
{
	auto controllerTransform = ovrMatrix4f_CreateFromQuaternion(&controller.Tracking.HeadPose.Pose.Orientation);
	ovrVector4f controllerDirection { 0, 0, -1, 0 };
	controllerDirection = ovrVector4f_MultiplyMatrix4f(&controllerTransform, &controllerDirection);
	auto transform = vrapi_GetViewMatrixFromPose(&appState.Tracking.HeadPose.Pose);
	auto& origin3d = appState.Tracking.HeadPose.Pose.Position;
	auto& controllerOrigin3d = controller.Tracking.HeadPose.Pose.Position;
	ovrVector4f controllerOrigin { controllerOrigin3d.x - origin3d.x, controllerOrigin3d.y - origin3d.y, controllerOrigin3d.z - origin3d.z, 0 };
	controllerOrigin = ovrVector4f_MultiplyMatrix4f(&transform, &controllerOrigin);
	ovrVector4f controllerTip { controllerOrigin3d.x - origin3d.x + controllerDirection.x, controllerOrigin3d.y - origin3d.y + controllerDirection.y, controllerOrigin3d.z - origin3d.z + controllerDirection.z, 0 };
	controllerTip = ovrVector4f_MultiplyMatrix4f(&transform, &controllerTip);
	controllerDirection.x = controllerTip.x - controllerOrigin.x;
	controllerDirection.y = controllerTip.y - controllerOrigin.y;
	controllerDirection.z = controllerTip.z - controllerOrigin.z;
	auto lengthSquared2d = controllerDirection.x * controllerDirection.x + controllerDirection.z * controllerDirection.z;
	if (lengthSquared2d < epsilon)
	{
		return false;
	}
	auto length2d = sqrt(lengthSquared2d);
	ovrVector2f controllerDirection2d { controllerDirection.x / length2d, controllerDirection.z / length2d };
	auto projection2d = -controllerOrigin.x * controllerDirection2d.x - controllerOrigin.z * controllerDirection2d.y;
	ovrVector2f projected2d { controllerOrigin.x + controllerDirection2d.x * projection2d, controllerOrigin.z + controllerDirection2d.y * projection2d };
	auto rejection2d = sqrt(projected2d.x * projected2d.x + projected2d.y * projected2d.y);
	if (rejection2d >= radius)
	{
		return false;
	}
	auto distanceToHitPoint = sqrt(radius * radius - rejection2d * rejection2d);
	ovrVector2f hitPoint2d { projected2d.x + controllerDirection2d.x * distanceToHitPoint, projected2d.y + controllerDirection2d.y * distanceToHitPoint };
	auto angle = atan2(hitPoint2d.y, hitPoint2d.x);
	if (angle < -M_PI / 2 - horizontalAngle / 2 || angle >= -M_PI / 2 + horizontalAngle / 2)
	{
		return false;
	}
	auto vertical = controllerOrigin.y + controllerDirection.y * (projection2d + distanceToHitPoint) / length2d;
	x = (angle + M_PI / 2 + horizontalAngle / 2) / horizontalAngle;
	y = (radius * verticalAngle / 2 - vertical) / (radius * verticalAngle);
	return true;
}
