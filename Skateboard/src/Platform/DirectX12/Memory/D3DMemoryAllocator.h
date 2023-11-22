#pragma once
#include <memory>
#include <vector>
#include <Skateboard/Core.h>

#include "Platform/DirectX12/D3D.h"
#include "Skateboard/Renderer/FrameResources.h"
#include "Skateboard/SizedPtr.h"

namespace Skateboard
{
	class D3DDescriptorHandle
	{
	public:
		D3DDescriptorHandle() : m_CPUHandle((D3D12_CPU_DESCRIPTOR_HANDLE)-1), m_GPUHandle((D3D12_GPU_DESCRIPTOR_HANDLE)-1), m_IsValid(false), m_Index(UINT32_MAX) {}
		D3DDescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cH, D3D12_GPU_DESCRIPTOR_HANDLE gH) : m_CPUHandle(cH), m_GPUHandle(gH), m_IsValid(true), m_Index(UINT32_MAX) {}
		~D3DDescriptorHandle() = default;

		D3DDescriptorHandle operator+(uint32_t offset) const;
		void operator+=(uint32_t offset);

		uint64_t GetCPUPointer() const { return m_CPUHandle.ptr; }
		uint64_t GetGPUPointer() const { return m_GPUHandle.ptr; }

		const D3D12_CPU_DESCRIPTOR_HANDLE& GetCPUHandle() const { return m_CPUHandle; }
		const D3D12_GPU_DESCRIPTOR_HANDLE& GetGPUHandle() const { return m_GPUHandle; }
		D3D12_CPU_DESCRIPTOR_HANDLE* GetCPUHandlePtr() { return &m_CPUHandle; }
		D3D12_GPU_DESCRIPTOR_HANDLE* GetGPUHandlePtr() { return &m_GPUHandle; }

		void SetValid(bool isValid);
		[[nodiscard("")]] bool IsValid() const { return m_IsValid; }

		void SetIndex(uint32_t index);
		[[nodiscard("")]] uint32_t GetIndex() const { return m_Index; }

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
		bool m_IsValid;
		uint32_t m_Index;
	};

	class D3DDescriptorHeap
	{
	public:
		D3DDescriptorHeap() : m_DescriptorIncrementSize(0u), m_FreeDescriptorCount(0u) {}
		DISABLE_COPY_AND_MOVE(D3DDescriptorHeap);

		~D3DDescriptorHeap() = default;

		void Create(const std::wstring& debugName, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity, bool shaderVisible);
		D3DDescriptorHandle Allocate(uint32_t count=1u);

		void Free(D3DDescriptorHandle& handle);
		void ProcessDeferredFree(uint32_t frameResourceIndex);

		void Release();

		[[nodiscard("")]] ID3D12DescriptorHeap* const* GetHeap() const { return m_Heap.GetAddressOf(); }
		[[nodiscard("")]] D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_Type; }
		[[nodiscard("")]] uint32_t GetHeapSize() const { return m_HeapSize; }
		[[nodiscard("")]] uint32_t GetDescriptorIncrementSize() const { return m_DescriptorIncrementSize; }
	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
		uint32_t m_DescriptorIncrementSize;	// Avoid continuous query
		uint32_t m_FreeDescriptorCount;

		uint32_t m_HeapSize{ 0u };
		uint32_t m_HeapCapacity{ 0u };
		uint32_t m_HeapCount{ 0u };

		D3DDescriptorHandle m_FirstHandle;
		D3DDescriptorHandle m_NextFreeHandle;

		std::unique_ptr<uint32_t[]>		m_AvailableHandles{ nullptr };
		std::vector<uint32_t>			m_DeferredAvailableIndices[SKTBD_SETTINGS_NUMFRAMERESOURCES]{};
		std::mutex m_HeapMutex;

		bool m_IsShaderVisible{ true };
		const D3D12_DESCRIPTOR_HEAP_TYPE m_Type{};
	
	};
}
