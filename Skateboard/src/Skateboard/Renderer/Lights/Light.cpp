#include "sktbdpch.h"
#include "Light.h"


namespace Skateboard
{
	Light::Light()
		:
		m_Diffuse(1.0f, 1.0f, 1.0f),
		m_FalloffStart(1.0f),
		m_Direction(1.0f, -1.0f, 0.0f),
		m_FalloffEnd(10.0f),
		m_Position(0.0f, 10.0f, 0.0f),
		m_SpotPower(64.0f),
		m_Radiance(0.01f, 0.01f, 0.01f),
		m_ProjectionMatrix(MatrixIdentity()),
		m_ViewMatrix(MatrixIdentity()),
		m_ViewProjTex(MatrixIdentity())
	{
	}

	Light::Light(const Light& rhs)
	{
		m_Diffuse			= rhs.m_Diffuse;
		m_FalloffStart		= rhs.m_FalloffStart;
		m_Direction			= rhs.m_Direction;
		m_FalloffEnd		= rhs.m_FalloffEnd;
		m_Position			= rhs.m_Position;
		m_SpotPower			= rhs.m_SpotPower;
		m_Radiance			= rhs.m_Radiance;
		m_ProjectionMatrix	= rhs.m_ProjectionMatrix;
		m_ViewMatrix		= rhs.m_ViewMatrix;
		m_ViewProjTex		= rhs.m_ViewProjTex;
	}

	auto Light::operator=(const Light& rhs) -> Light&
	{
		m_Diffuse			= rhs.m_Diffuse;
		m_FalloffStart		= rhs.m_FalloffStart;
		m_Direction			= rhs.m_Direction;
		m_FalloffEnd		= rhs.m_FalloffEnd;
		m_Position			= rhs.m_Position;
		m_SpotPower			= rhs.m_SpotPower;
		m_Radiance			= rhs.m_Radiance;
		m_ProjectionMatrix	= rhs.m_ProjectionMatrix;
		m_ViewMatrix		= rhs.m_ViewMatrix;
		m_ViewProjTex		= rhs.m_ViewProjTex;
		return *this;
	}

	void Light::SetDiffuse(float3 diffuse)
	{
		m_Diffuse = diffuse;
	}

	void Light::SetFalloffStart(float falloffStart)
	{
		m_FalloffStart = falloffStart;
	}

	void Light::SetDirection(float3 direction)
	{
		m_Direction = direction;
	}

	void Light::SetFalloffEnd(float falloffEnd)
	{
		m_FalloffEnd = falloffEnd;
	}

	void Light::SetPosition(float3 position)
	{
		m_Position = position;
	}

	void Light::SetSpotPower(float spotPower)
	{
		m_SpotPower = spotPower;
	}

	void Light::SetRadiance(float3 radiance)
	{
		m_Radiance = radiance;
	}

	void Light::GenerateOrthographicProjectionMatrix(float viewWidth, float viewHeight, float nearPlane, float farPlane)
	{
		m_ProjectionMatrix = MatrixOrthographic(viewWidth, viewHeight, nearPlane, farPlane);
	}

	void Light::GeneratePerspectiveProjectionMatrix(float nearPlane, float farPlane)
	{
		m_ProjectionMatrix = MatrixPerspectiveFov(SKTBD_PI / 2.f, 1.f, nearPlane, farPlane);
	}

	void Light::GenerateViewMatrix()
	{
		vector pos = Vector3Load(&m_Position);
		vector dir = Vector3Normalise(Vector3Load(&m_Direction));

		// Direction = Target - position
		vector target = VectorSet(0.f, 0.f, 0.f, 0.f);
		pos = VectorMul(VectorSub(target, dir), VectorSet(10.f));
		Vector3Store(&m_Position, pos);

		// TODO: Projection (point lights, spotlights) would probably want this instead
		//vector target = VectorAdd(dir, pos);

		// As the direction is variable, need to rotate up accordingly
		const float yAngle = VectorGetX(Vector3AngleBetweenNormals(VectorSet(0.f, 1.f, 0.f, 0.f), dir));
		const float xAngle = VectorGetX(Vector3AngleBetweenNormals(VectorSet(1.f, 0.f, 0.f, 0.f), dir));
		const float zAngle = VectorGetX(Vector3AngleBetweenNormals(VectorSet(0.f, 0.f, 1.f, 0.f), dir));
		
		const float4x4 viewRotation = MatrixRotationPitchYawRoll(xAngle, yAngle, zAngle);
		vector up = VectorSet(0.f, 1.f, 0.f, 0.f);
		up = Vector3TransformCoord(up, viewRotation);

		//dir = VectorAdd(dir, pos);
		m_ViewMatrix = MatrixLookAt(pos, target, up);

		GenerateTextureSpaceMatrix();
	}

	void Light::GenerateTextureSpaceMatrix()
	{
		// This matrix allows to transform a position from normalised device coordinates (NDC) to texture space (u,v)
		float4x4 T = {
			.5f,	0.f,	0.f,	0.f,
			.0f,	-.5f,	0.f,	0.f,
			.0f,	0.f,	1.f,	0.f,
			.5f,	.5f,	0.f,	1.f
		};
		m_ViewProjTex = m_ViewMatrix * m_ProjectionMatrix * T;
	}
}
