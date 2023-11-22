#pragma once
#include "Skateboard/SizedPtr.h"
#include "Skateboard/Renderer/Buffer.h"

namespace Skateboard
{
	struct MemoryManagerData
	{
		~MemoryManagerData();

		std::vector<std::unique_ptr<Skateboard::UploadBuffer>>	v_UploadBuffers;			// A container of all the upload buffers
		std::vector<SizedPtr>									v_UploadBufferPendingDatas;	// A container of cpu buffers with the data to be transferred to the GPU
		std::unordered_map<uint32_t, uint32_t>					v_BuffersDirty;				// pair< bufferID , numFramesDirty > (Keeps track of the number of frames a buffer is dirty)
	};

	class MemoryManager
	{
	public:
		static uint32_t CreateConstantBuffer(const std::wstring& debugName, uint32_t elementCount, uint32_t dataSize) { return CreateUploadBuffer(debugName, UploadBufferDesc::Init(true, elementCount, dataSize)); }
		static uint32_t CreateStructuredBuffer(const std::wstring& debugName, uint32_t elementCount, uint32_t dataSize) { return CreateUploadBuffer(debugName, UploadBufferDesc::Init(false, elementCount, dataSize)); }
		static uint32_t CreateUploadBuffer(const std::wstring& debugName, const UploadBufferDesc& desc);

		static void ResetUploadBuffer(uint32_t uploadBufferID, uint32_t elementCount, uint32_t dataSize);

		static void Update();

		static void Clean();

		static void UploadData(uint32_t uploadBufferID, uint32_t elementIndex, void* pData);
		static UploadBuffer* GetUploadBuffer(uint32_t uploadBufferID);

	private:
		static MemoryManagerData s_Data;
	};
}