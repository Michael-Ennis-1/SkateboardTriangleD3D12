#include <sktbdpch.h>
#include "D3DAssetManager.h"
#include "Skateboard/Assets/AssetManager.h"
#include "Platform/DirectX12/D3DBuffer.h"
#include "Platform/DirectX12/D3DGraphicsContext.h"
#include "DDSTextureLoader.h"
#include <filesystem>

namespace Skateboard
{
	D3DAssetManager::D3DAssetManager()
	{
		// We'll initialise a pool of textures to an unnecessary large number (unlikely that students will load that many textures)
		m_TextureDescriptorTable.Init(gD3DContext->GetSrvHeap(), 1024, ShaderDescriptorType_SRV);
	}

	Texture* Skateboard::D3DAssetManager::LoadTextureImpl(const wchar_t* filename, GPUResourceType_ resType)
	{
		SKTBD_CORE_ASSERT(resType == GPUResourceType_Texture2D || resType == GPUResourceType_Texture3D, "You supplied an incorrect resource type for your texture! (Either 2D/3D)");

		TextureDesc desc = {};
		desc.Type = resType;

		std::filesystem::path path(filename);
		D3DTexture* result = new D3DTexture(path.stem().wstring().c_str(), desc);

		// Check the file and load the texture using Microsoft's DDS library
		std::ifstream inputFile(path.native().c_str());
		if (inputFile.good())
		{
			if (!path.extension().wstring().compare(L".dds"))
			{
				D3D_CHECK_FAILURE(DirectX::CreateDDSTextureFromFile12(
					gD3DContext->Device(),
					gD3DContext->GraphicsCommandList(),
					path.native().c_str(),
					result->GetResourceComPtr(),
					result->GetIntermediateResourceComPtr())
				);
#ifndef SKTBD_SHIP
				result->GetResource()->SetName(path.native().c_str());
#endif
				// Assign remaining description
				D3D12_RESOURCE_DESC d = result->GetResource()->GetDesc();
				static uint32_t textureID = 0u;
				desc.Width = d.Width;
				desc.Height = d.Height;
				desc.Depth = d.DepthOrArraySize;
				desc.Format = D3DBufferFormatToSkateboard(d.Format);
				desc.ID = textureID++;
				result->SetDesc(desc);

				//TODO: REMOVE when descriptor tables are made properly with correct shader visibilities
				auto barrier = D3D::TransitionBarrier(result->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				gD3DContext->GraphicsCommandList()->ResourceBarrier(1, &barrier);

				// Create a view for this resource
				result->CreateSRV();
				m_TextureDescriptorTable.CopyDescriptor(m_TextureDescriptorTable.Allocate(1), result->GetSRVHandle());
				m_TextureDescriptorTable.UpdateAll();
			}
			else
			{
				SKTBD_CORE_ASSERT(false, "Unsopported texture type. For now only DDS files are supported.");
				delete result, result = nullptr;
				return nullptr;
			}
		}
		else
		{
			SKTBD_CORE_ASSERT(false, "Could not load the specified texture, texture not found.");
			delete result, result = nullptr;
			return nullptr;
		}

		// Success
		++m_TextureMaxIndex;
		return result;
	}

}