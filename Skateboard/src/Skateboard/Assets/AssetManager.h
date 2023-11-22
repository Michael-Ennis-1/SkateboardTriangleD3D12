#pragma once
#include "Skateboard/Core.h"
#include "Skateboard/Renderer/Buffer.h"

namespace Skateboard
{
	class DescriptorTable;

	class AssetManager
	{
	protected:
		AssetManager() : m_TextureMaxIndex(-1) {}
	public:
		static AssetManager* Create();
		static AssetManager& Singleton();
		virtual ~AssetManager() {}

		static Texture* LoadTexture(const wchar_t* filename, GPUResourceType_ resType = GPUResourceType_Texture2D) { return Singleton().LoadTextureImpl(filename, resType); }
		static DescriptorTable* GetTextureDescriptorTable() { return Singleton().GetTextureDescriptorTableImpl(); }
		static int32_t GetTexureMaxIndex() { return Singleton().m_TextureMaxIndex; }

	protected:
		virtual Texture* LoadTextureImpl(const wchar_t* filename, GPUResourceType_ resType) = 0;
		virtual DescriptorTable* GetTextureDescriptorTableImpl() = 0;

	protected:
		int32_t m_TextureMaxIndex;
	};
}