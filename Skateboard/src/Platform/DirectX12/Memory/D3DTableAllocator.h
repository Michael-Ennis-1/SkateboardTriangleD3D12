#pragma once
#include "Platform/DirectX12/D3D.h"
#include "Platform/DirectX12/Memory/D3DMemoryAllocator.h"
#include <unordered_map>
#include <vector>
#include "Skateboard/Memory/DescriptorTable.h"
#include "Platform/DirectX12/D3DGraphicsContext.h"

namespace Skateboard
{
	struct ShaderResourceDesc;
	// <summary>
	//	A convenience class to assist in sorting resources into pools on the GPU.
	// </summary>

	class D3DTableAllocator
	{
	public:

		explicit D3DTableAllocator(D3DDescriptorHeap* heap, uint32_t capacity);
		DISABLE_COPY_AND_MOVE(D3DTableAllocator);

		~D3DTableAllocator();

		D3DDescriptorHandle Allocate();

		void Free(D3DDescriptorHandle& handle);

		void Release();

		_NODISCARD D3DDescriptorHandle GetResourceTable() const;

	private:
		D3DDescriptorHeap* p_Heap;

		D3DDescriptorHandle m_SubRangeBegin;
		D3DDescriptorHandle m_SubRangeEnd;
		D3DDescriptorHandle m_SubRangeNextAvailableHandle;

		D3D12_DESCRIPTOR_HEAP_TYPE m_ParentHeapType;
		D3D12_DESCRIPTOR_RANGE1 m_SubRange;

		uint32_t m_SubRangeSize;
		uint32_t m_SubRangeCapacity;
		uint32_t m_SubRangeCount;
		uint32_t m_OffsetFromHeapStart;
		BOOL m_HasAllocatedSubRange;

		std::mutex m_TableMutex;

	};

	class D3DDescriptorTable : public DescriptorTable
	{
	public:
		D3DDescriptorTable();
		D3DDescriptorTable(const D3DDescriptorTable&) = delete;
		D3DDescriptorTable& operator=(const D3DDescriptorTable&) = delete;
		D3DDescriptorTable(D3DDescriptorTable&&) noexcept = delete;
		D3DDescriptorTable& operator=(D3DDescriptorTable&&) noexcept = delete;
		virtual ~D3DDescriptorTable() final override;

		void Init(D3DDescriptorHeap* heap, uint32_t capacity, ShaderDescriptorTableType_ type);
		void Update(uint64_t frameResourceIndex = gD3DContext->GetCurrentFrameResourceIndex());
		void UpdateAll();
		uint32_t Allocate(uint32_t descriptorCount);
		void CopyDescriptor(uint32_t descriptorID, const D3DDescriptorHandle& srcHandle);
		void Free(uint32_t descriptorID);
		bool IsValid() const { return m_HasAllocatedSubRange; }

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const;
		virtual uint32_t GetDescriptorCount() const final override { return m_DescriptorCount; }

	private:
		std::mutex m_TableMutex;
		D3DDescriptorHeap* p_Heap;
		D3DDescriptorHandle m_DescriptorTableStart[SKTBD_SETTINGS_NUMFRAMERESOURCES];
		D3D12_DESCRIPTOR_HEAP_TYPE m_ParentHeapType;


		uint32_t m_DescriptorIncrementSize;
		uint32_t m_DescriptorCapacity;
		uint32_t m_DescriptorCount;

		std::unordered_map<uint32_t, uint32_t> v_DescriptorsDirty;
		std::vector<D3DDescriptorHandle> v_PendingDescriptors;

		bool m_HasAllocatedSubRange;
	};
}