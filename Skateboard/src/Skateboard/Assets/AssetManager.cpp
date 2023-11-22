#include <sktbdpch.h>
#include "AssetManager.h"
#include "Skateboard/Renderer/Renderer.h"
#include "Platform/DirectX12/Assets/D3DAssetManager.h"

namespace Skateboard
{
	AssetManager* AssetManager::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DAssetManager();
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

	AssetManager& AssetManager::Singleton()
	{
		static std::unique_ptr<AssetManager> singleton(Create());
		return *singleton;
	}
}