#include "sktbdpch.h"
#include "Model.h"

#include "Skateboard/Renderer/Renderer.h"

#include <Platform/DirectX12/Model/D3DMeshletModel.h>

namespace Skateboard
{
	Model* Model::Create(const wchar_t* filename)
	{
			switch (Renderer::GetAPI())
			{
			case RendererAPI_::RendererAPI_None:
				SKTBD_CORE_ASSERT(false, "API not yet supported.");
				return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
			case RendererAPI_::RendererAPI_DirectX12:
				//TODO: Return a new model compatible with legacy rendering.
				//TODO: or perhaps make it flexible and return one type of api
				//TODO: specific model. i.e D3DMeshletModel is compatible with both!.
				//return new D3DMeshletModel(filename);
			case RendererAPI_::RendererAPI_Vulkan:
				SKTBD_CORE_ASSERT(false, "API not yet supported.");
				return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
			case RendererAPI_::RendererAPI_AGC:
				SKTBD_CORE_ASSERT(false, "API not yet supported.");
				return nullptr;
#endif
			default:
				SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
				return nullptr;
			}
	}


	MeshletModel* MeshletModel::Create(const wchar_t* filename)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DModel(filename);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
			return nullptr;
		}
	}
	
}
