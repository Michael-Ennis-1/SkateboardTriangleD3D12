#pragma once
#include "Platform/DirectX12/D3D.h"
#include "Skateboard/Assets/AssetManager.h"
#include "Platform/DirectX12/Memory/D3DTableAllocator.h"

namespace Skateboard
{
	class D3DAssetManager : public AssetManager
	{
	public:
		D3DAssetManager();
		virtual ~D3DAssetManager() final override {}

		virtual DescriptorTable* GetTextureDescriptorTableImpl() final override { return &m_TextureDescriptorTable; }

	private:
		virtual Texture* LoadTextureImpl(const wchar_t* filename, GPUResourceType_ resType) final override;

	private:
		// There will be one big descriptor table referencing all the textures
		D3DDescriptorTable m_TextureDescriptorTable;
	};
}