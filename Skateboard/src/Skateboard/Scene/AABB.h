#pragma once
#include <string>
#include "Skateboard/Renderer/InternalFormats.h"

namespace Skateboard
{
	struct AABB
	{
		float MinX;
		float MinY;
		float MinZ;
		float MaxX;
		float MaxY;
		float MaxZ;
	};

	struct RaytracingAABBDesc
	{
		std::wstring Name;
		GeometryType_ Type;
		AABB BoundingBox;
	};
}