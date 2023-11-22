#include "sktbdpch.h"
#include "D3DTableAllocator.h"
#include "Platform/DirectX12/D3DGraphicsContext.h"

#include "Platform/DirectX12/D3DGraphicsContext.h"

namespace Skateboard
{
	D3DTableAllocator::D3DTableAllocator(D3DDescriptorHeap* heap, uint32_t capacity)
		:
			p_Heap(heap)
		,	m_SubRangeBegin()
		,	m_SubRangeEnd()
		,	m_SubRangeNextAvailableHandle()
		,	m_ParentHeapType(heap->GetHeapType())
		,	m_SubRange()
		,	m_SubRangeSize(heap->GetHeapSize())
		,	m_SubRangeCapacity(capacity)
		,	m_SubRangeCount(0)
		,	m_HasAllocatedSubRange(FALSE)
	{
		SKTBD_CORE_ASSERT(capacity, "Must supply a non-zero sub-range!")

		m_SubRangeBegin = p_Heap->Allocate(capacity);
		m_SubRangeEnd = m_SubRangeBegin + m_SubRangeSize * m_SubRangeCapacity;
		m_SubRangeNextAvailableHandle = m_SubRangeBegin;
	}

	D3DTableAllocator::~D3DTableAllocator()
	{
	}

	D3DDescriptorHandle D3DTableAllocator::Allocate()
	{
		// For sub ranges, at this time, it is not possible to skip slots in allocation.
		// Primarily, this class should be reserved for dynamic indexing on the GPU
		// for simple material systems or texture arrays. 

		std::lock_guard lock(m_TableMutex);

		// Check if there is enough space, otherwise throw as this would be an invalid descriptor allocation (your GPU would crash either way after using it)
		SKTBD_CORE_ASSERT(m_SubRangeCount > m_SubRangeCapacity, "Out of Memory!");

		// Allocation simply means changing the memory pointer to a further region of the allocated heap
		D3DDescriptorHandle ret = m_SubRangeNextAvailableHandle;
		m_SubRangeNextAvailableHandle += 1u * m_SubRangeSize;
		ret.SetIndex(m_OffsetFromHeapStart + m_SubRangeCount);

		m_SubRangeCount++;

		return ret;
	}

	void D3DTableAllocator::Free(D3DDescriptorHandle& handle)
	{
		p_Heap->Free(handle);
	}

	D3DDescriptorTable::D3DDescriptorTable() :
		p_Heap(nullptr),
		m_ParentHeapType(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
		m_DescriptorIncrementSize(0u),
		m_DescriptorCapacity(0u),
		m_DescriptorCount(0u),
		m_HasAllocatedSubRange(false)
	{
	}

	D3DDescriptorTable::~D3DDescriptorTable()
	{
		v_DescriptorsDirty.clear();
		v_PendingDescriptors.clear();
	}

	void D3DDescriptorTable::Init(D3DDescriptorHeap* heap, uint32_t capacity, ShaderDescriptorTableType_ type)
	{
		SKTBD_CORE_ASSERT(heap != nullptr, "Invalid heap. Heap cannot be null.");
		SKTBD_CORE_ASSERT(capacity != 0u, "Invalid capacity. Capacity must be greater than zero.");
		SKTBD_CORE_ASSERT(p_Heap == nullptr, "Invalid initialisation, this descriptor table has already been initialised.");

		std::lock_guard lock(m_TableMutex);
		m_Type = type;
		p_Heap = heap;
		m_DescriptorCapacity = capacity;
		v_PendingDescriptors.resize(capacity);

		SKTBD_CORE_ASSERT(capacity, "Must supply a non-zero sub-range!");
		m_ParentHeapType = p_Heap->GetHeapType();
		m_DescriptorIncrementSize = p_Heap->GetDescriptorIncrementSize();

		for (uint32_t i = 0u; i < SKTBD_SETTINGS_NUMFRAMERESOURCES; ++i)
			m_DescriptorTableStart[i] = p_Heap->Allocate(capacity);

		m_HasAllocatedSubRange = true;
	}

	void D3DDescriptorTable::Update(uint64_t frameResourceIndex)
	{
		for (auto it = v_DescriptorsDirty.begin(); it != v_DescriptorsDirty.end();)
		{
			const uint32_t& descriptorID = it->first;
			uint32_t& numFramesDirty = it->second;

			D3DDescriptorHandle dest = m_DescriptorTableStart[frameResourceIndex] + m_DescriptorIncrementSize * descriptorID;
			gD3DContext->Device()->CopyDescriptorsSimple(1, dest.GetCPUHandle(), v_PendingDescriptors[descriptorID].GetCPUHandle(), m_ParentHeapType);

			--numFramesDirty;
			if (numFramesDirty == 0u)
				it = v_DescriptorsDirty.erase(it);
			else ++it;
		}
	}

	void D3DDescriptorTable::UpdateAll()
	{
		for (uint32_t i = 0u; i < 3u; ++i)
			Update(i);
	}

	uint32_t D3DDescriptorTable::Allocate(uint32_t descriptorCount)
	{
		// Check if there is enough space, otherwise throw as this would be an invalid descriptor allocation (your GPU would crash either way after using it)
		SKTBD_CORE_ASSERT(m_DescriptorCount + descriptorCount < m_DescriptorCapacity, "This descriptor table ran out of Memory! Consider increasing the capacity supplied in Init().");

		// Allocation simply means returning a descriptor ID.
		// The actual resource descriptor will be uploaded to the table with CopyDescriptor()
		uint32_t descriptorID = m_DescriptorCount;
		m_DescriptorCount += descriptorCount;
		return descriptorID;
	}

	void D3DDescriptorTable::CopyDescriptor(uint32_t descriptorID, const D3DDescriptorHandle& srcHandle)
	{
		if (v_DescriptorsDirty.contains(descriptorID))
			v_DescriptorsDirty[descriptorID] = SKTBD_SETTINGS_NUMFRAMERESOURCES;
		else
			v_DescriptorsDirty.insert({ descriptorID, SKTBD_SETTINGS_NUMFRAMERESOURCES });
		v_PendingDescriptors[descriptorID] = srcHandle;
	}

	void D3DDescriptorTable::Free(uint32_t descriptorID)
	{
		SKTBD_CORE_ERROR("We didn't test removing descriptors from a table, it is likely broken from what I read. Should probably clear the descriptor from the table and resize the table. Right now all IDs become obsolete.");
		D3DDescriptorHandle handleToRemove = m_DescriptorTableStart[gD3DContext->GetCurrentFrameResourceIndex()] + m_DescriptorIncrementSize * descriptorID;
		p_Heap->Free(handleToRemove);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE D3DDescriptorTable::GetGPUHandle() const
	{
		return m_DescriptorTableStart[gD3DContext->GetCurrentFrameResourceIndex()].GetGPUHandle();
	}
}
