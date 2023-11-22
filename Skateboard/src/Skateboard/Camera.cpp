#include "sktbdpch.h"
#include "Camera.h"

namespace Skateboard
{
	Camera::Camera() :
		m_NearPlane(0.1f),
		m_FarPlane(1000.f),
		m_Position(0.f, 0.f, 0.f),
		m_Rotation(0.f, 0.f, 0.f),
		m_ViewMatrix(MatrixIdentity()),
		m_ProjectionMatrix(MatrixIdentity())
	{
	}

	Camera::Camera(float nearPlane, float farPlane, float3 position, float3 target, float3 up) :
		m_NearPlane(nearPlane),
		m_FarPlane(farPlane),
		m_Position(position),
		m_Rotation(0.f, 0.f, 0.f),
		m_ViewMatrix(MatrixLookAt(Vector3Load(&m_Position), Vector3Load(&target), Vector3Load(&up))),
		m_ProjectionMatrix(MatrixIdentity())
	{
	}

	void Camera::UpdateViewMatrix()
	{
		// Init data
		vector up = VectorSet(0.f, 1.f, 0.f, 1.f);  // Set w to 1 for the following matrix multiplication
		vector position = Vector3Load(&m_Position);
		vector lookAt = VectorSet(0.f, 0.f, 1.f, 1.f);

		// Init rotation matrix
		const float yaw = DegToRad(m_Rotation.y);
		const float pitch = DegToRad(m_Rotation.x);
		const float roll = DegToRad(m_Rotation.z);
		const float4x4 viewRotation = MatrixRotationPitchYawRoll(pitch, yaw, roll);

		// Transform the lookAt and up vectors based on the current view rotation
		up = Vector3TransformCoord(up, viewRotation);
		lookAt = Vector3TransformCoord(lookAt, viewRotation);

		// Translate the target position to the position of the camera
		lookAt = VectorAdd(lookAt, position);

		// Finally, create the view matrix
		m_ViewMatrix = MatrixLookAt(position, lookAt, up);
	}

	PerspectiveCamera::PerspectiveCamera()
	:
			m_Fov(45.0f)
		,	m_AspectRatio(1920.0f/1080.0f)
		,	m_MovementSpeed(CAMERA_DEFAULT_MOVESPEED)
		,	m_Sensitivity(CAMERA_DEFAULT_SENSITIVITY)
		,	m_HasMoved(false)
		
	{
		m_ProjectionMatrix = std::move(MatrixPerspectiveFov(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane));
	}

	PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearPlane, float farPlane, float3 position, float3 target, float3 up) :
		Camera(nearPlane, farPlane, position, target, up),
		m_Fov(fov),
		m_AspectRatio(aspectRatio),
		m_MovementSpeed(CAMERA_DEFAULT_MOVESPEED),
		m_Sensitivity(CAMERA_DEFAULT_SENSITIVITY),
		m_HasMoved(false)
	{
		m_ProjectionMatrix = std::move(MatrixPerspectiveFov(m_Fov, aspectRatio, m_NearPlane, m_FarPlane));
	}

	void PerspectiveCamera::Build(float fov, float aspectRatio, float nearPlane, float farPlane, float3 position, float3 target, float3 up)
	{
		// Build projection
		m_ProjectionMatrix = std::move(MatrixPerspectiveFov(fov, aspectRatio, nearPlane, farPlane));

		// Build the view matrix
		m_ViewMatrix = MatrixLookAt(Vector3Load(&position), Vector3Load(&target), Vector3Load(&up));

		// Update relevant members
		m_Fov = fov;
		m_AspectRatio = aspectRatio;
		m_NearPlane = nearPlane;
		m_FarPlane = farPlane;
		m_Position = position;
	}

	void PerspectiveCamera::SetFov(float fov)
	{
		m_Fov = fov;
		m_ProjectionMatrix = std::move(MatrixPerspectiveFov(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane));
	}

	void PerspectiveCamera::SetFrustum(float nearPlane, float farPlane)
	{
		m_NearPlane = nearPlane;
		m_FarPlane = farPlane;
		m_ProjectionMatrix = std::move(MatrixPerspectiveFov(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane));
	}

	void PerspectiveCamera::OnResize(int newClientWidth, int newClientHeight)
	{
		m_AspectRatio = static_cast<float>(newClientWidth) / newClientHeight;
		m_ProjectionMatrix = std::move(MatrixPerspectiveFov(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane));
	}

	OrthographicCamera::OrthographicCamera(float viewWidth, float viewHeight, float nearPlane, float farPlane, float3 position, float3 target, float3 up) :
		Camera(nearPlane, farPlane, position, target, up)
	{
		m_ProjectionMatrix = std::move(MatrixOrthographic(viewWidth, viewHeight, nearPlane, farPlane));
	}

	void OrthographicCamera::OnResize(int newClientWidth, int newClientHeight)
	{
		m_ProjectionMatrix = std::move(MatrixOrthographic(static_cast<float>(newClientWidth), static_cast<float>(newClientHeight), m_NearPlane, m_FarPlane));
	}
}