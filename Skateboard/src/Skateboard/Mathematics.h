#pragma once
#define SKTBD_PI 3.141592654f

#ifdef SKTBD_PLATFORM_WINDOWS
#include <DirectXMath.h>
#include <DirectXCollision.h>
typedef DirectX::XMFLOAT2 float2;
typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMINT2 int2;
typedef DirectX::XMINT3 int3;
typedef DirectX::XMINT4 int4;
typedef DirectX::XMUINT2 uint2;
typedef DirectX::XMUINT3 uint3;
typedef DirectX::XMUINT4 uint4;
typedef DirectX::XMVECTOR vector;
typedef DirectX::FXMVECTOR fvector;
typedef DirectX::XMMATRIX matrix, float4x4;
typedef DirectX::BoundingSphere BoundSphere;
typedef DirectX::BoundingBox BoundBox;

namespace Skateboard
{
	// -----------------------------------------------------------------------------
	// Angles
	// -----------------------------------------------------------------------------
	inline auto RadToDeg(float radians) -> decltype(DirectX::XMConvertToDegrees(radians)) {
		return DirectX::XMConvertToDegrees(radians);
	}
	inline auto DegToRad(float degrees) -> decltype(DirectX::XMConvertToRadians(degrees)) {
		return DirectX::XMConvertToRadians(degrees);
	}

	// -----------------------------------------------------------------------------
	// 4D Vectors
	// -----------------------------------------------------------------------------
	inline auto VectorSet(float xxxx) -> decltype(DirectX::XMVectorSet(xxxx, xxxx, xxxx, xxxx)) {
		return DirectX::XMVectorSet(xxxx, xxxx, xxxx, xxxx);
	}
	inline auto VectorSet(float x, float y, float z, float w) -> decltype(DirectX::XMVectorSet(x, y, z, w)) {
		return DirectX::XMVectorSet(x, y, z, w);
	}
	inline auto VectorAdd(vector v1, vector v2) -> decltype(DirectX::XMVectorAdd(v1, v2)) {
		return DirectX::XMVectorAdd(v1, v2);
	}
	inline auto VectorSub(vector v1, vector v2) -> decltype(DirectX::XMVectorSubtract(v1, v2)) {
		return DirectX::XMVectorSubtract(v1, v2);
	}
	inline auto VectorMul(vector v1, vector v2) -> decltype(DirectX::XMVectorMultiply(v1, v2)) {
		return DirectX::XMVectorMultiply(v1, v2);
	}
	inline auto VectorDiv(vector v1, vector v2) -> decltype(DirectX::XMVectorDivide(v1, v2)) {
		return DirectX::XMVectorDivide(v1, v2);
	}
	inline auto VectorGetX(vector v) -> decltype(DirectX::XMVectorGetX(v)) {
		return DirectX::XMVectorGetX(v);
	}
	inline auto VectorGetY(vector v) -> decltype(DirectX::XMVectorGetY(v)) {
		return DirectX::XMVectorGetY(v);
	}
	inline auto VectorGetZ(vector v) -> decltype(DirectX::XMVectorGetZ(v)) {
		return DirectX::XMVectorGetZ(v);
	}
	inline auto VectorReplicate(float x) ->decltype(DirectX::XMVectorReplicate(x)){
		return DirectX::XMVectorReplicate(x);
	}
	// -----------------------------------------------------------------------------
	// 3D Vectors
	// -----------------------------------------------------------------------------
	inline auto Vector3Load(const float3* pSource) -> decltype(DirectX::XMLoadFloat3(pSource)) {
		return DirectX::XMLoadFloat3(pSource);
	}
	inline auto Vector3Store(float3* pDest, vector source) -> decltype(DirectX::XMStoreFloat3(pDest, source)) {
		return DirectX::XMStoreFloat3(pDest, source);
	}
	inline auto Vector3AngleBetweenNormals(vector n1, vector n2) -> decltype(DirectX::XMVector3AngleBetweenNormals(n1, n2)) {
		return DirectX::XMVector3AngleBetweenNormals(n1, n2);
	}
	inline auto Vector3AngleBetweenVectors(vector v1, vector v2) -> decltype(DirectX::XMVector3AngleBetweenVectors(v1, v2)) {
		return DirectX::XMVector3AngleBetweenVectors(v1, v2);
	}
	inline auto Vector3Dot(vector v1, vector v2) -> decltype(DirectX::XMVector3Dot(v1, v2)) {
		return DirectX::XMVector3Dot(v1, v2);
	}
	inline auto Vector3Cross(vector v1, vector v2) -> decltype(DirectX::XMVector3Cross(v1, v2)) {
		return DirectX::XMVector3Cross(v1, v2);
	}
	inline auto Vector3Length(vector v) -> decltype(DirectX::XMVector3Length(v)) {
		return DirectX::XMVector3Length(v);
	}
	inline auto Vector3Normalise(vector v) -> decltype(DirectX::XMVector3Normalize(v)) {
		return DirectX::XMVector3Normalize(v);
	}
	inline auto Vector3Rotate(vector v, vector rotationQuaternion) -> decltype(DirectX::XMVector3Rotate(v, rotationQuaternion)) {
		return DirectX::XMVector3Rotate(v, rotationQuaternion);
	}
	inline auto Vector3Transform(vector v, float4x4 m) -> decltype(DirectX::XMVector3Transform(v, m)) {
		return DirectX::XMVector3Transform(v, m);
	}
	inline auto Vector3TransformCoord(vector v, float4x4 m) -> decltype(DirectX::XMVector3TransformCoord(v, m)) {
		return DirectX::XMVector3TransformCoord(v, m);
	}
	inline auto Vector3Dot(vector v, float4x4 m) -> decltype(DirectX::XMVector3TransformNormal(v, m)) {
		return DirectX::XMVector3TransformNormal(v, m);
	}

	// -----------------------------------------------------------------------------
	// Quaternions
	// -----------------------------------------------------------------------------
	inline auto QuaternionRotationAxis(vector axis, float angle) -> decltype(DirectX::XMQuaternionRotationAxis(axis, angle)) {
		return DirectX::XMQuaternionRotationAxis(axis, angle);
	}
	inline auto QuaternionRotationNormal(vector normalAxis, float angle) -> decltype(DirectX::XMQuaternionRotationNormal(normalAxis, angle)) {
		return DirectX::XMQuaternionRotationNormal(normalAxis, angle);
	}
	inline auto QuaternionRotationMatrix(float4x4 m) -> decltype(DirectX::XMQuaternionRotationMatrix(m)) {
		return DirectX::XMQuaternionRotationMatrix(m);
	}
	inline auto QuaternionPitchYawRoll(float pitch, float yaw, float roll) -> decltype(DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll)) {
		return DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);
	}
	inline auto QuaternionPitchYawRollFromVector(vector vectorPitchYawRoll) -> decltype(DirectX::XMQuaternionRotationRollPitchYawFromVector(vectorPitchYawRoll)) {
		return DirectX::XMQuaternionRotationRollPitchYawFromVector(vectorPitchYawRoll);
	}
	inline auto QuaternionToAxisAngle(vector* pAxis, float* pAngle, vector quaternion) -> decltype(DirectX::XMQuaternionToAxisAngle(pAxis, pAngle, quaternion)) {
		return DirectX::XMQuaternionToAxisAngle(pAxis, pAngle, quaternion);
	}

	// -----------------------------------------------------------------------------
	// Matrices
	// -----------------------------------------------------------------------------
	inline auto MatrixIdentity() -> decltype(DirectX::XMMatrixIdentity()) {
		return DirectX::XMMatrixIdentity();
	}
	inline auto MatrixTranspose(float4x4 m) -> decltype(DirectX::XMMatrixTranspose(m)) {
		return DirectX::XMMatrixTranspose(m);
	}
	inline auto MatrixInverse(vector* pDeterminant, float4x4 m) -> decltype(DirectX::XMMatrixInverse(pDeterminant, m)) {
		return DirectX::XMMatrixInverse(pDeterminant, m);
	}
	inline auto MatrixTranslation(float offsetX, float offsetY, float offsetZ) -> decltype(DirectX::XMMatrixTranslation(offsetX, offsetY, offsetZ)) {
		return DirectX::XMMatrixTranslation(offsetX, offsetY, offsetZ);
	}
	inline auto MatrixTranslationFromVector(vector translationVector) -> decltype(DirectX::XMMatrixTranslationFromVector(translationVector)) {
		return DirectX::XMMatrixTranslationFromVector(translationVector);
	}
	inline auto MatrixRotationAxis(vector axis, float angle) -> decltype(DirectX::XMMatrixRotationAxis(axis, angle)) {
		return DirectX::XMMatrixRotationAxis(axis, angle);
	}
	inline auto MatrixRotationNormal(vector normalAxis, float angle) -> decltype(DirectX::XMMatrixRotationNormal(normalAxis, angle)) {
		return DirectX::XMMatrixRotationNormal(normalAxis, angle);
	}
	inline auto MatrixRotationQuaternion(vector rotationQuaternion) -> decltype(DirectX::XMMatrixRotationQuaternion(rotationQuaternion)) {
		return DirectX::XMMatrixRotationQuaternion(rotationQuaternion);
	}
	inline auto MatrixRotationPitchYawRoll(float pitch, float yaw, float roll) -> decltype(DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll)) {
		return DirectX::XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
	}
	inline auto MatrixRotationPitchYawRollFromVector(vector angles) -> decltype(DirectX::XMMatrixRotationRollPitchYawFromVector(angles)) {
		return DirectX::XMMatrixRotationRollPitchYawFromVector(angles);
	}
	inline auto MatrixRotationX(float angle) -> decltype(DirectX::XMMatrixRotationX(angle)) {
		return DirectX::XMMatrixRotationX(angle);
	}
	inline auto MatrixRotationY(float angle) -> decltype(DirectX::XMMatrixRotationY(angle)) {
		return DirectX::XMMatrixRotationY(angle);
	}
	inline auto MatrixRotationZ(float angle) -> decltype(DirectX::XMMatrixRotationZ(angle)) {
		return DirectX::XMMatrixRotationZ(angle);
	}
	inline auto MatrixScaling(float scaleX, float scaleY, float scaleZ) -> decltype(DirectX::XMMatrixScaling(scaleX, scaleY, scaleZ)) {
		return DirectX::XMMatrixScaling(scaleX, scaleY, scaleZ);
	}
	inline auto MatrixScalingFromVector(vector scaleVector) -> decltype(DirectX::XMMatrixScalingFromVector(scaleVector)) {
		return DirectX::XMMatrixScalingFromVector(scaleVector);
	}
	inline auto MatrixLookAt(vector eyePosition, vector focusPoint, vector upDirection) -> decltype(DirectX::XMMatrixLookAtLH(eyePosition, focusPoint, upDirection)) {
		return DirectX::XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
	}
	inline auto MatrixPerspectiveFov(float fov, float aspectRatio, float nearPlane, float farPlane) -> decltype(DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane)) {
		return DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
	}
	inline auto MatrixOrthographic(float viewWidth, float viewHeight, float nearPlane, float farPlane) -> decltype(DirectX::XMMatrixOrthographicLH(viewWidth, viewHeight, nearPlane, farPlane)) {
		return DirectX::XMMatrixOrthographicLH(viewWidth, viewHeight, nearPlane, farPlane);
	}
	
	inline auto MatrixDecompose(vector* out_Scale, vector* out_Rotation, vector* out_Translation, float4x4 in_Matrix) -> decltype(DirectX::XMMatrixDecompose(out_Scale, out_Rotation, out_Translation, in_Matrix)) {
		bool returnval = DirectX::XMMatrixDecompose(out_Scale, out_Rotation, out_Translation, in_Matrix);

		// Quaternion to Euler
		// TODO: Broken
		float3 rotationEuler;
		float3 mRow1, mRow2, mRow3;
		Vector3Store(&mRow1, in_Matrix.r[0]);
		Vector3Store(&mRow2, in_Matrix.r[1]);
		Vector3Store(&mRow3, in_Matrix.r[2]);
		rotationEuler.y = asinf(-mRow1.z);
		if (cosf(rotationEuler.y))
		{
			rotationEuler.x = atan2f(mRow2.z, mRow3.z);
			rotationEuler.z = atan2f(mRow1.y, mRow1.x);
		}
		else
		{
			rotationEuler.x = atan2f(-mRow2.x, mRow1.y);
			rotationEuler.z = 0.f;
		}
		rotationEuler = { RadToDeg(rotationEuler.x), RadToDeg(rotationEuler.y), RadToDeg(rotationEuler.z) };
		*out_Rotation = Vector3Load(&rotationEuler);

		return returnval;
	}
}
#endif

#ifdef SKTBD_PLATFORM_PS5
#error How did you get here now?


#endif