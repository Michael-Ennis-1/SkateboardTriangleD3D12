#include <sktbdpch.h>
#include "MemoryManager.h"
#include "Skateboard/Renderer/FrameResources.h"

namespace Skateboard
{
	MemoryManagerData MemoryManager::s_Data;

	MemoryManagerData::~MemoryManagerData()
	{
		for (SizedPtr& ptr : v_UploadBufferPendingDatas)
		{
			free(ptr.Ptr);
			ptr = { nullptr, 0u };
		}
	}

	uint32_t MemoryManager::CreateUploadBuffer(const std::wstring& debugName, const UploadBufferDesc& desc)
	{
		const uint32_t returnVal = static_cast<uint32_t>(s_Data.v_UploadBufferPendingDatas.size());
		SKTBD_CORE_ASSERT(returnVal != UINT32_MAX, "Possible overflow! Did you really mean to create that many upload buffer?");

		s_Data.v_UploadBuffers.emplace_back(UploadBuffer::Create(debugName, desc));

		s_Data.v_UploadBufferPendingDatas.emplace_back(SizedPtr(calloc(desc.ElementCount, desc.ElementSize), desc.ElementSize));
		return returnVal;
	}

	void MemoryManager::ResetUploadBuffer(uint32_t uploadBufferID, uint32_t elementCount, uint32_t dataSize)
	{
		SKTBD_CORE_ASSERT(uploadBufferID < static_cast<uint32_t>(s_Data.v_UploadBuffers.size()), "Out of bounds access. Verify that you created a buffer with this input uploadBufferID.");

		const std::wstring& debugName = s_Data.v_UploadBuffers[uploadBufferID]->GetDebugName();
		UploadBufferDesc desc = s_Data.v_UploadBuffers[uploadBufferID]->GetDesc().AccessFlag == BufferAccessFlag_CpuWriteable_Aligned ? UploadBufferDesc::Init(true, elementCount, dataSize) : UploadBufferDesc::Init(false, elementCount, dataSize);
		s_Data.v_UploadBuffers[uploadBufferID].reset(UploadBuffer::Create(debugName, desc));
	}

	void MemoryManager::Update()
	{
		// Manage upload buffers so that they are updated for all following frame resources after a change
		for (auto it = s_Data.v_BuffersDirty.begin(); it != s_Data.v_BuffersDirty.end();)
		{
			const uint32_t& bufferID = it->first;
			uint32_t& numFramesDirty = it->second;

			std::unique_ptr<UploadBuffer>& currentBuffer = s_Data.v_UploadBuffers[bufferID];
			const uint32_t elementCount = currentBuffer->GetDesc().ElementCount;

			//currentBuffer->CopyData(0, s_Data.v_UploadBufferPendingDatas[bufferID].Ptr);
			for (uint32_t i = 0u; i < elementCount; ++i)
			{
				uint8_t* ptr = static_cast<uint8_t*>(s_Data.v_UploadBufferPendingDatas[bufferID].Ptr) + s_Data.v_UploadBufferPendingDatas[bufferID].Size * i;
				currentBuffer->CopyData(i, ptr);
			}

			--numFramesDirty;
			if (numFramesDirty == 0u)
				it = s_Data.v_BuffersDirty.erase(it);
			else ++it;
		}
	}

	void MemoryManager::Clean()
	{
		for(auto& buffer:s_Data.v_UploadBuffers)
		{
			buffer->Release();
		}
	}

	void MemoryManager::UploadData(uint32_t uploadBufferID, uint32_t elementIndex, void* pData)
	{
		SKTBD_CORE_ASSERT(uploadBufferID < static_cast<uint32_t>(s_Data.v_UploadBufferPendingDatas.size()), "Out of bounds access. Verify that you created a buffer with this input uploadBufferID.");
		SKTBD_CORE_ASSERT(uploadBufferID < static_cast<uint32_t>(s_Data.v_UploadBuffers.size()), "Out of bounds access. Verify that you created a buffer with this input uploadBufferID.");
		SKTBD_CORE_ASSERT(elementIndex < s_Data.v_UploadBuffers[uploadBufferID]->GetDesc().ElementCount, "Out of bounds access. Invalid element index.");

		uint8_t* bufferStart = static_cast<uint8_t*>(s_Data.v_UploadBufferPendingDatas[uploadBufferID].Ptr) + elementIndex * s_Data.v_UploadBuffers[uploadBufferID]->GetDesc().ElementSize;
		memcpy(bufferStart, pData, s_Data.v_UploadBufferPendingDatas[uploadBufferID].Size);
		if (s_Data.v_BuffersDirty.contains(uploadBufferID))
			s_Data.v_BuffersDirty[uploadBufferID] = SKTBD_SETTINGS_NUMFRAMERESOURCES;
		else
			s_Data.v_BuffersDirty.insert({ uploadBufferID, SKTBD_SETTINGS_NUMFRAMERESOURCES });
	}

	UploadBuffer* MemoryManager::GetUploadBuffer(uint32_t uploadBufferID)
	{
		SKTBD_CORE_ASSERT(uploadBufferID < static_cast<uint32_t>(s_Data.v_UploadBuffers.size()), "Out of bound access. Verify that you created a buffer with this input uploadBufferID.");
		return s_Data.v_UploadBuffers[uploadBufferID].get();
	}
}