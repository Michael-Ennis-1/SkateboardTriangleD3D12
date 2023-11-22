#include "sktbdpch.h"
#include "D3DBuffer.h"
#include "D3DGraphicsContext.h"

namespace Skateboard
{
	D3DDefaultBuffer::D3DDefaultBuffer(const std::wstring& debugName, const DefaultBufferDesc& desc) : DefaultBuffer(debugName, desc)
	{
		ID3D12Device* pDevice = gD3DContext->Device();
		ID3D12GraphicsCommandList* pCommandList = gD3DContext->GraphicsCommandList();

		m_DataChunkSize = m_Desc.ElementSize;
		m_Desc.Width = m_Desc.ElementCount * m_Desc.ElementSize;

		// Describe the gpu buffer
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;			// This buffer is having only 1 dimension (this is a buffer)
		bufferDesc.Alignment = 0;										// Alignment is a power-of-two number which the offset of the beginning of the resource
		bufferDesc.Width = m_Desc.Width;								// The width defines the total size in bytes of the data
		bufferDesc.Height = 1u;											// Only one dimension
		bufferDesc.DepthOrArraySize = 1u;								// Again, one dimension so the size is 1
		bufferDesc.MipLevels = 1;										// This is a buffer, it does not have multiple mips
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;						// Declare the format as not know for flexibility
		bufferDesc.SampleDesc.Count = 1;								// MSAA is not supported on pipeline state buffers anymore, so sample count is 1 and quality is 0
		bufferDesc.SampleDesc.Quality = 0;								// Use MSAA on render targets instead
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;				// Not relevant, default value
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// Describe the heap for the gpu buffer
		D3D12_HEAP_PROPERTIES heapGPUProperties = {};
		heapGPUProperties.Type = D3D12_HEAP_TYPE_DEFAULT;						// No CPU access, only the GPU will access this data
		heapGPUProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapGPUProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapGPUProperties.CreationNodeMask = 0;
		heapGPUProperties.VisibleNodeMask = 0;

		// Describe the heap for the cpu buffer
		D3D12_HEAP_PROPERTIES heapCPUProperties = {};
		heapCPUProperties.Type = D3D12_HEAP_TYPE_UPLOAD;						// The intermediate buffer will be used to transfer data from RAM to VRAM (CPU -> GPU)
		heapCPUProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapCPUProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapCPUProperties.CreationNodeMask = 0;
		heapCPUProperties.VisibleNodeMask = 0;

		// Create the buffers
		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(
			&heapGPUProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(m_Buffer.ReleaseAndGetAddressOf())
		));
		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(
			&heapCPUProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_IntermediateBuffer.ReleaseAndGetAddressOf())
		));

		// Write the data to the cpu buffer
		void* pDest;
		m_IntermediateBuffer->Map(0, nullptr, &pDest);
		memcpy(pDest, m_Desc.pDataToTransfer, m_Desc.Width);
		m_IntermediateBuffer->Unmap(0, nullptr);
		m_Desc.pDataToTransfer = nullptr;

		// Transition the gpu buffer to be writable on the gpu, write the data from the one-shot upload buffer on the gpu, and transition back
		D3D12_RESOURCE_BARRIER toWritable = D3D::TransitionBarrier(m_Buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		D3D12_RESOURCE_BARRIER toReadable = D3D::TransitionBarrier(m_Buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		pCommandList->ResourceBarrier(1, &toWritable);
		pCommandList->CopyResource(m_Buffer.Get(), m_IntermediateBuffer.Get());
		pCommandList->ResourceBarrier(1, &toReadable);

#ifndef SKTBD_SHIP
		m_Buffer->SetName(debugName.c_str());
		m_IntermediateBuffer->SetName((debugName + L" - Intermediate").c_str());
#endif // !SKTBD_SHIP

		// Create the a view so it can be used as a shader resource
		auto* pRWSrvHeap = gD3DContext->GetRWSrvHeap();

		m_SRVHandle = pRWSrvHeap->Allocate();
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewDesc.Format = BufferFormatToD3D(m_Desc.Format);
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		viewDesc.Buffer.FirstElement = 0;
		viewDesc.Buffer.NumElements = m_Desc.ElementCount;
		viewDesc.Buffer.StructureByteStride = m_DataChunkSize;
		viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		gD3DContext->Device()->CreateShaderResourceView(m_Buffer.Get(), &viewDesc, m_SRVHandle.GetCPUHandle());
	}

	D3DDefaultBuffer::~D3DDefaultBuffer()
	{
	}

	void D3DDefaultBuffer::Release()
	{
		gD3DContext->DeferredRelease(m_Buffer.Get());
		gD3DContext->SetDeferredReleasesFlag();
	}
	
	D3DUploadBuffer::D3DUploadBuffer(const std::wstring& debugName, const UploadBufferDesc& desc) : UploadBuffer(debugName, desc)
	{
		ID3D12Device* pDevice = gD3DContext->Device();
		const bool isConstantBuffer = m_Desc.AccessFlag == BufferAccessFlag_CpuWriteable_Aligned;

		// Create an upload buffer so data can be transferred to the gpu
		if (isConstantBuffer)
			m_DataChunkSize = ROUND_UP(m_Desc.ElementSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		else
			m_DataChunkSize = ROUND_UP(m_Desc.ElementSize, D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT);

		m_Desc.Width = static_cast<uint64_t>(m_DataChunkSize) * m_Desc.ElementCount * SKTBD_SETTINGS_NUMFRAMERESOURCES;

		// Describe the buffer properties
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0;
		bufferDesc.Width = m_Desc.Width;	// UploadBuffers are subject to frame resources
		bufferDesc.Height = 1u;
		bufferDesc.DepthOrArraySize = 1u;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// Describe the heap properties
		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;						// CPU access, on-shot buffer that will be uploaded to the gpu
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 0;
		heapProperties.VisibleNodeMask = 0;

		// Create the buffer
		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_Buffer.ReleaseAndGetAddressOf())
		));

		// The MSDN documentation says it is bad practice to continuously map and unmap a buffer as it adds unnecessary overhead
		// DX12 allows for a single call of Map() on the buffer, and simply unmap on destruction
		D3D_CHECK_FAILURE(m_Buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));

#ifndef SKTBD_SHIP
		m_Buffer->SetName(m_DebugName.c_str());
#endif // !SKTBD_SHIP

		// Structured buffer are sent as shader resource views
		// The according view must be created in that event so it can be used in a pipeline
		if (!isConstantBuffer)
		{
			auto* pRWSrvHeap = gD3DContext->GetRWSrvHeap();
			m_SRVHandle = pRWSrvHeap->Allocate();
			D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
			viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			viewDesc.Format = BufferFormatToD3D(m_Desc.Format);
			viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			viewDesc.Buffer.FirstElement = 0;
			viewDesc.Buffer.NumElements = m_Desc.ElementCount;
			viewDesc.Buffer.StructureByteStride = m_DataChunkSize;
			viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			gD3DContext->Device()->CreateShaderResourceView(m_Buffer.Get(), &viewDesc, m_SRVHandle.GetCPUHandle());
		}
	}

	D3D12_GPU_VIRTUAL_ADDRESS D3DUploadBuffer::GetGPUVirtualAddress(uint32_t elementIndex) const
	{
		SKTBD_CORE_ASSERT(elementIndex < m_Desc.ElementCount, "Out of bounds access. The index provided is not valid in this constant buffer.");
		return m_Buffer->GetGPUVirtualAddress() + (gD3DContext->GetCurrentFrameResourceIndex() * m_Desc.ElementCount + elementIndex) * m_DataChunkSize;
	}

	void D3DUploadBuffer::Release()
	{
		gD3DContext->DeferredRelease(m_Buffer.Get());
		gD3DContext->SetDeferredReleasesFlag();
	}

	D3DUploadBuffer::~D3DUploadBuffer()
	{
	}

	void D3DUploadBuffer::CopyData(uint32_t elementIndex, const void* pData)
	{
		SKTBD_CORE_ASSERT(m_Desc.AccessFlag != BufferAccessFlag_GpuOnly, "Illegal upload of data to a buffer that is only visible to the GPU");
		SKTBD_CORE_ASSERT(elementIndex < m_Desc.ElementCount, "Out-of-bound element access on this buffer!");

		// Important: We cannot copy data while it is being used by the GPU!
		// This is solved with synchronisation solutions, in this framework through FrameResources
		memcpy(&m_MappedData[(gD3DContext->GetCurrentFrameResourceIndex() * m_Desc.ElementCount + elementIndex) * m_DataChunkSize], pData, m_Desc.ElementSize);
	}

	void D3DUploadBuffer::CopyData(uint32_t elementIndex, const void* pData, uint32_t size)
	{
	}

	D3DUnorderedAccessBuffer::D3DUnorderedAccessBuffer(const std::wstring& debugName, const UnorderedAccessBufferDesc& desc) :
		UnorderedAccessBuffer(debugName, desc),
		m_SRVHandleSize(gD3DContext->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
	{
		BuildResource(true);
	}

	D3DUnorderedAccessBuffer::~D3DUnorderedAccessBuffer()
	{
	}

	void D3DUnorderedAccessBuffer::Resize(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth)
	{
		m_Desc.Width = newWidth;
		m_Desc.Height = newHeight;
		m_Desc.Depth = newDepth;
		BuildResource(false);
	}

	void D3DUnorderedAccessBuffer::TransitionToType(GPUResourceType_ newType)
	{
		// Sanity check
		if (m_Type == newType)
			return;

		D3D12_RESOURCE_BARRIER barrier = D3D::TransitionBarrier(m_Buffer.Get(), ResourceTypeToStateD3D(m_Type), ResourceTypeToStateD3D(newType));
		gD3DContext->GraphicsCommandList()->ResourceBarrier(1, &barrier);
		m_Type = newType;
	}

	void D3DUnorderedAccessBuffer::Release()
	{
		gD3DContext->DeferredRelease(m_Buffer.Get());
		gD3DContext->SetDeferredReleasesFlag();
	}

	void D3DUnorderedAccessBuffer::BuildResource(bool allocateDescriptorHeap)
	{
		ID3D12Device* pDevice = gD3DContext->Device();

		// Create the UAV render target
		// -> 2D with dimension client width * client height to fill up the screen
		D3D12_RESOURCE_DESC uavResourceDesc = {};
		uavResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		uavResourceDesc.Alignment = 0;
		uavResourceDesc.Width = m_Desc.Width;
		uavResourceDesc.Height = m_Desc.Height;
		uavResourceDesc.DepthOrArraySize = m_Desc.Depth;
		uavResourceDesc.MipLevels = 1;
		uavResourceDesc.Format = BufferFormatToD3D(m_Desc.Format);
		uavResourceDesc.SampleDesc.Count = 1;
		uavResourceDesc.SampleDesc.Quality = 0;
		uavResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		uavResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // TODO: Read back??
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 0;
		heapProperties.VisibleNodeMask = 0;

		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&uavResourceDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(m_Buffer.ReleaseAndGetAddressOf())
		));

#ifndef SKTBD_SHIP
		m_Buffer->SetName(m_DebugName.c_str());
#endif // !SKTBD_SHIP

		// Create the shader resource views so that in can be used in the pipelines
		D3DDescriptorHandle tempHandle;
		if (allocateDescriptorHeap)
		{
			auto* pRWSrvHeap = gD3DContext->GetRWSrvHeap();
			tempHandle = m_SRVHandle = pRWSrvHeap->Allocate(2);
		}
		else
		{
			tempHandle = m_SRVHandle;
		}
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavViewDesc = {};
		uavViewDesc.Format = BufferFormatToD3D(m_Desc.Format);
		if (m_Desc.Height > 1 && m_Desc.Depth > 1)
		{
			uavViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavViewDesc.Texture3D.MipSlice = 0;
			uavViewDesc.Texture3D.FirstWSlice = 0;
			uavViewDesc.Texture3D.WSize = 0;
		}
		else if (m_Desc.Height > 1)
		{
			uavViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavViewDesc.Texture2D.MipSlice = 0;
			uavViewDesc.Texture2D.PlaneSlice = 0;
		}
		else
		{
			uavViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavViewDesc.Texture1D.MipSlice = 0;
		}
		gD3DContext->Device()->CreateUnorderedAccessView(m_Buffer.Get(), nullptr, &uavViewDesc, tempHandle.GetCPUHandle());

		tempHandle += m_SRVHandleSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC textureViewDesc = {};
		textureViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		textureViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		textureViewDesc.Format = BufferFormatToD3D(m_Desc.Format);
		textureViewDesc.Texture2D.MostDetailedMip = 0;
		textureViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		textureViewDesc.Texture2D.MipLevels = 1;
		gD3DContext->Device()->CreateShaderResourceView(m_Buffer.Get(), &textureViewDesc, tempHandle.GetCPUHandle());
	}


	D3DByteAddressBuffer::D3DByteAddressBuffer(const std::wstring& debugName, const ByteAddressBufferDesc& desc)
		:
		ByteAddressBuffer(debugName, desc)
		, m_SRVHandle()
		, m_SRVHandleSize()
	{
		ID3D12Device* pDevice = gD3DContext->Device();


		// Create an upload buffer so data can be transferred to the gpu
		m_DataChunkSize = m_Desc.ElementSize;
		m_DataChunkSize = ROUND_UP(m_DataChunkSize, D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT);

		m_Desc.Width = static_cast<uint64_t>(m_DataChunkSize) * m_Desc.ElementCount;

		// Describe the buffer properties
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Alignment = 0;
		bufferDesc.Width = m_Desc.Width;	// UploadBuffers are subject to frame resources
		bufferDesc.Height = 1u;
		bufferDesc.DepthOrArraySize = 1u;
		bufferDesc.MipLevels = 1;
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		// Describe the heap properties
		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;						// CPU access, on-shot buffer that will be uploaded to the gpu
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 0;
		heapProperties.VisibleNodeMask = 0;

		// Create the buffer
		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_Buffer.ReleaseAndGetAddressOf())
		));

		// The MSDN documentation says it is bad practice to continuously map and unmap a buffer as it adds unnecessary overhead
		// DX12 allows for a single call of Map() on the buffer, and simply unmap on destruction
		D3D_CHECK_FAILURE(m_Buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));

#ifndef SKTBD_SHIP
		m_Buffer->SetName(m_DebugName.c_str());
#endif // !SKTBD_SHIP

		// Structured buffer are sent as shader resource views
		// The according view must be created in that event so it can be used in a pipeline
		auto* pRWSrvHeap = const_cast<D3DDescriptorHeap*>(gD3DContext->GetRWSrvHeap());
		// Create the a view so it can be used as a shader resource
		m_SRVHandle = pRWSrvHeap->Allocate();

		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		// SetBufferTypeAsByteAddress address buffers *must* be a typless format 
		viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		viewDesc.Buffer.FirstElement = 0;
		viewDesc.Buffer.NumElements = m_Desc.ElementCount;
		// Note: SetBufferTypeAsByteAddress address buffers require the stride to be set to 0.
		viewDesc.Buffer.StructureByteStride = 0u;
		viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		gD3DContext->Device()->CreateShaderResourceView(m_Buffer.Get(), &viewDesc, m_SRVHandle.GetCPUHandle());

	}

	D3DByteAddressBuffer::~D3DByteAddressBuffer()
	{
	}

	void D3DByteAddressBuffer::CopyData(const void* pData, uint32_t elementIndex, uint32_t elementCount)
	{
		SKTBD_CORE_ASSERT(m_Desc.AccessFlag != BufferAccessFlag_GpuOnly, "Illegal upload of data to a buffer that is only visible to the GPU");
		SKTBD_CORE_ASSERT(elementIndex < m_Desc.ElementCount, "Out-of-bound element access on this buffer!");

		// Important: We cannot copy data while it is being used by the GPU!
		// This is solved with synchronisation solutions, in this framework through FrameResources
		memcpy(&m_MappedData[elementIndex], pData, elementCount);
	}

	void D3DByteAddressBuffer::Release()
	{
		gD3DContext->DeferredRelease(m_Buffer.Get());
		gD3DContext->SetDeferredReleasesFlag();
	}

	void D3DByteAddressBuffer::BuildResource(bool allocateDescriptorHeap)
	{
		ID3D12Device* pDevice = gD3DContext->Device();

		// Create the UAV render target
		// -> 2D with dimension client width * client height to fill up the screen
		D3D12_RESOURCE_DESC uavResourceDesc = {};
		uavResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		uavResourceDesc.Alignment = 0;
		uavResourceDesc.Width = m_Desc.Width;
		uavResourceDesc.Height = 1;
		uavResourceDesc.DepthOrArraySize = 1;
		uavResourceDesc.MipLevels = 1;
		uavResourceDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		uavResourceDesc.SampleDesc.Count = 1;
		uavResourceDesc.SampleDesc.Quality = 0;
		uavResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		uavResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // TODO: Read back??
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 0;
		heapProperties.VisibleNodeMask = 0;

		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&uavResourceDesc,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			nullptr,
			IID_PPV_ARGS(m_Buffer.ReleaseAndGetAddressOf())
		));

#ifndef SKTBD_SHIP
		m_Buffer->SetName(m_DebugName.c_str());
#endif // !SKTBD_SHIP

		auto* pRWSrvHeap = const_cast<D3DDescriptorHeap*>(gD3DContext->GetRWSrvHeap());

		// Create the shader resource views so that in can be used in the pipelines
		D3DDescriptorHandle tempHandle;
		if (allocateDescriptorHeap)
		{

			tempHandle = m_SRVHandle = pRWSrvHeap->Allocate();
		}
		else
		{
			tempHandle = m_SRVHandle;
		}

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavViewDesc = {};
		uavViewDesc.Format = BufferFormatToD3D(m_Desc.Format);
		uavViewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavViewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		uavViewDesc.Buffer.CounterOffsetInBytes = 0;
		uavViewDesc.Buffer.FirstElement = 0;
		uavViewDesc.Buffer.StructureByteStride = 0;
		uavViewDesc.Buffer.NumElements = m_Desc.ElementCount;

		gD3DContext->Device()->CreateUnorderedAccessView(m_Buffer.Get(), nullptr, &uavViewDesc, tempHandle.GetCPUHandle());

		tempHandle += m_SRVHandleSize;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

		gD3DContext->Device()->CreateShaderResourceView(m_Buffer.Get(), &srvDesc, tempHandle.GetCPUHandle());
	}


	D3DTexture::D3DTexture(const std::wstring& debugName, const TextureDesc& desc) : Texture(L"", desc)
	{
		// The asset manager is responsible to load data from the texture file and upload it to the gpu
		// Resources are therefore managed in the asset manager
		// The debug name will be set to the texture filename
	}

	D3DTexture::~D3DTexture()
	{
	}

	void D3DTexture::CreateSRV()
	{
		// TODO: Support cube maps
		if (m_Type == GPUResourceType_Texture3D)
		{
			SKTBD_CORE_ASSERT(false, "Cube maps yet unsupported");
			return;
		}

		auto* pRWSrvHeap = gD3DContext->GetRWSrvHeap();
		m_SRVHandle = pRWSrvHeap->Allocate();
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		viewDesc.Format = m_Buffer->GetDesc().Format;							// The viewing format (the same as the texture)
		viewDesc.Texture2D.MostDetailedMip = 0;									// This indexes the most detailed level in the mipmap
		viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;							// Clamps to a minimum mip level
		viewDesc.Texture2D.MipLevels = m_Buffer->GetDesc().MipLevels;			// The maximum level of mipmaps
		gD3DContext->Device()->CreateShaderResourceView(m_Buffer.Get(), &viewDesc, m_SRVHandle.GetCPUHandle());
	}

	void D3DTexture::Release()
	{
		gD3DContext->DeferredRelease(m_Buffer.Get());
		gD3DContext->DeferredRelease(m_IntermediateBuffer.Get());
		gD3DContext->SetDeferredReleasesFlag();
	}

	D3DIndexBuffer::D3DIndexBuffer(const std::wstring& debugName, uint32_t* indices, uint32_t count) :
		m_IndexCount(count),
		m_IndexBuffer(static_cast<D3DDefaultBuffer*>(DefaultBuffer::Create(debugName, DefaultBufferDesc::Init(indices, count, this->GetStride())))),
		m_IndexBufferView{
			m_IndexBuffer->GetResource()->GetGPUVirtualAddress(),
			count * this->GetStride(),
			BufferFormatToD3D(this->GetFormat())
		}
	{
	}

	D3DIndexBuffer::~D3DIndexBuffer()
	{
	}

	void D3DIndexBuffer::Bind() const
	{
		// Send the index buffer
		gD3DContext->GraphicsCommandList()->IASetIndexBuffer(&m_IndexBufferView);
	}

	void D3DIndexBuffer::Release()
	{
	}

	D3DVertexBuffer::D3DVertexBuffer(const std::wstring& debugName, void* vertices, uint32_t size, const BufferLayout& layout) :
		m_Layout(layout),
		m_VertexBuffer(static_cast<D3DDefaultBuffer*>(DefaultBuffer::Create(debugName, DefaultBufferDesc::Init(vertices, size / layout.GetStride(), layout.GetStride())))),
		m_VertexBufferView{
			m_VertexBuffer->GetResource()->GetGPUVirtualAddress(),			// Identify the address of the buffer
			size,															// Specify the size of the entire buffer
			m_Layout.GetStride()											// Specify the size of each vertex entry
		}
	{
	}

	D3DVertexBuffer::~D3DVertexBuffer()
	{
	}

	void D3DVertexBuffer::Bind() const
	{
		// Send the vertex buffer
		gD3DContext->GraphicsCommandList()->IASetVertexBuffers(0, 1, &m_VertexBufferView);
	}

	void D3DVertexBuffer::Release()
	{
		m_VertexBuffer->Release();
	}

	D3DFrameBuffer::D3DFrameBuffer(const std::wstring& debugName, const FrameBufferDesc& desc) :
		FrameBuffer(debugName),
		m_Desc(desc),
		m_ViewPort{ 0.f, 0.f, static_cast<float>(desc.Width), static_cast<float>(desc.Height), 0.f, 1.f },
		m_ScissorRect{ 0, 0, static_cast<long>(desc.Width), static_cast<long>(desc.Height) },
		m_SRVDescriptorSize(gD3DContext->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)),
		m_RTVDescriptorSize(gD3DContext->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV))
	{
		BuildColourRenderTargets();
		BuildDepthStencilTargets();
		BuildDescriptors(true);
	}

	D3DFrameBuffer::~D3DFrameBuffer()
	{
	}

	void D3DFrameBuffer::Bind(uint32_t renderTargetIndex) const
	{
		if (m_RTResources.Get())
		{
			// Colour pass
			D3D12_RESOURCE_BARRIER barriers[2] = {
				D3D::TransitionBarrier(m_RTResources.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET),
				D3D::TransitionBarrier(m_DSResource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE)
			};
			gD3DContext->GraphicsCommandList()->ResourceBarrier(2, barriers);

			gD3DContext->GraphicsCommandList()->RSSetViewports(1, &m_ViewPort);
			gD3DContext->GraphicsCommandList()->RSSetScissorRects(1, &m_ScissorRect);
			float4 clear = m_Desc.RenderTarget.ClearColour;
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetRTVHandle(renderTargetIndex).GetCPUHandle();
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_DSVHandle.GetCPUHandle();

			gD3DContext->GraphicsCommandList()->ClearRenderTargetView(rtvHandle, &clear.x, 0u, nullptr);
			gD3DContext->GraphicsCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0u, 0u, nullptr);
			gD3DContext->GraphicsCommandList()->OMSetRenderTargets(1u, &rtvHandle, true, &dsvHandle);
		}
		else
		{
			// Transition for depth write
			D3D12_RESOURCE_BARRIER barrier = D3D::TransitionBarrier(m_DSResource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			gD3DContext->GraphicsCommandList()->ResourceBarrier(1, &barrier);

			// Depth only pass
			gD3DContext->GraphicsCommandList()->RSSetViewports(1, &m_ViewPort);
			gD3DContext->GraphicsCommandList()->RSSetScissorRects(1, &m_ScissorRect);
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_DSVHandle.GetCPUHandle();

			gD3DContext->GraphicsCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0u, 0u, nullptr);
			gD3DContext->GraphicsCommandList()->OMSetRenderTargets(0u, nullptr, false, &dsvHandle);
		}
	}

	void D3DFrameBuffer::Unbind() const
	{
		if (m_RTResources.Get())
		{
			//D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE
			D3D12_RESOURCE_BARRIER barriers[2] = {
				D3D::TransitionBarrier(m_RTResources.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ),
				D3D::TransitionBarrier(m_DSResource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ)
			};
			gD3DContext->GraphicsCommandList()->ResourceBarrier(2, barriers);
		}
		else
		{
			D3D12_RESOURCE_BARRIER barrier = D3D::TransitionBarrier(m_DSResource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
			gD3DContext->GraphicsCommandList()->ResourceBarrier(1, &barrier);
		}
	}

	void D3DFrameBuffer::Resize(uint32_t newWidth, uint32_t newHeight)
	{
		// Sanity check
		if (newWidth == 0u || newHeight == 0u)
			return;

		m_Desc.Width = newWidth;
		m_Desc.Height = newHeight;
		m_ViewPort = { 0.f, 0.f, static_cast<float>(newWidth), static_cast<float>(newHeight), 0.f, 1.f };
		m_ScissorRect = { 0, 0, static_cast<long>(newWidth), static_cast<long>(newHeight) };

		// Resources need to be rebuilt with new dimensions
		BuildColourRenderTargets();
		BuildDepthStencilTargets();
		BuildDescriptors(false);
	}

	void* D3DFrameBuffer::GetRenderTargetAsImGuiTextureID(uint32_t renderTargetIndex) const
	{
		SKTBD_CORE_ASSERT(m_RTResources.Get(), "Cannot use a NULL render target as a texture. Did you mean to use the depth/stencil target?");

		D3DDescriptorHandle temp = m_GPUSRVHandle + renderTargetIndex * m_SRVDescriptorSize;
		return (void*)(temp.GetGPUHandle().ptr);
	}

	void* D3DFrameBuffer::GetDepthStencilTargetAsImGuiTextureID(uint32_t renderTargetIndex) const
	{
		D3DDescriptorHandle temp = m_GPUSRVHandle + m_Desc.NumRenderTargets * m_SRVDescriptorSize;
		return (void*)(temp.GetGPUHandle().ptr);
	}

	void D3DFrameBuffer::Release()
	{
		gD3DContext->DeferredRelease(m_RTResources.Get());
		gD3DContext->DeferredRelease(m_DSResource.Get());
		gD3DContext->SetDeferredReleasesFlag();
	}

	inline void D3DFrameBuffer::BuildColourRenderTargets()
	{
		// If there are no render targets, then this frame buffer is a depth only pass!
		if (m_Desc.NumRenderTargets == 0u)
			return;

		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// The target itself remains a texture 2D
		resDesc.Alignment = 0;
		resDesc.Width = m_Desc.Width;
		resDesc.Height = m_Desc.Height;
		resDesc.DepthOrArraySize = m_Desc.NumRenderTargets;
		resDesc.MipLevels = 1;
		resDesc.SampleDesc.Count = 1;
		resDesc.SampleDesc.Quality = 0;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_HEAP_PROPERTIES heap = {};
		heap.Type = D3D12_HEAP_TYPE_DEFAULT;					// This cubemap will only be needed on the GPU
		heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap.CreationNodeMask = 0;
		heap.VisibleNodeMask = 0;

		D3D12_CLEAR_VALUE clear = {};
		memcpy(clear.Color, &m_Desc.RenderTarget.ClearColour.x, sizeof(float4));

		DXGI_FORMAT format = BufferFormatToD3D(m_Desc.RenderTarget.Format);
		resDesc.Format = format;
		clear.Format = format;

		D3D_CHECK_FAILURE(gD3DContext->Device()->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&clear,
			IID_PPV_ARGS(m_RTResources.ReleaseAndGetAddressOf())
		));
#ifndef SKTBD_SHIP
		m_RTResources->SetName((m_DebugName + L" - Render Targets").c_str());
#endif
	}

	inline void D3DFrameBuffer::BuildDepthStencilTargets()
	{
		D3D12_RESOURCE_DESC resDesc = {};
		resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;		// The shadowmap is effectively a 2D texture
		resDesc.Alignment = 0u;
		resDesc.Width = m_Desc.Width;
		resDesc.Height = m_Desc.Height;
		resDesc.DepthOrArraySize = 1u;
		resDesc.MipLevels = 1u;
		resDesc.SampleDesc.Count = 1u;
		resDesc.SampleDesc.Quality = 0u;
		resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;		// Allow this resource to be written to as the depth/stencil buffer

		D3D12_HEAP_PROPERTIES heap = {};
		heap.Type = D3D12_HEAP_TYPE_DEFAULT;						// This texture will only be needed on the GPU
		heap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heap.CreationNodeMask = 0;
		heap.VisibleNodeMask = 0;


		D3D12_CLEAR_VALUE clear = {};
		clear.DepthStencil.Depth = m_Desc.DepthStencilTarget.DepthStencil.Depth;
		clear.DepthStencil.Stencil = m_Desc.DepthStencilTarget.DepthStencil.Stencil;

		DXGI_FORMAT format = BufferFormatToD3D(m_Desc.DepthStencilTarget.Format);
		resDesc.Format = format;
		clear.Format = format;

		D3D_CHECK_FAILURE(gD3DContext->Device()->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			&clear,
			IID_PPV_ARGS(m_DSResource.ReleaseAndGetAddressOf())
		));

#ifndef SKTBD_SHIP
		m_DSResource->SetName((m_DebugName + L" - Depth Stencil Target").c_str());
#endif
	}

	void D3DFrameBuffer::BuildDescriptors(bool allocate)
	{
		// Allocate the render targets and create the views
		if (allocate)
		{
			auto* pSrvHeap		= gD3DContext->GetSrvHeap();
			auto* pRWSrvHeap	= gD3DContext->GetRWSrvHeap();
			auto* pRtvHeap		= gD3DContext->GetRtvHeap();
			auto* pDsvHeap		= gD3DContext->GetDsvHeap();

			m_GPUSRVHandle	= pSrvHeap->Allocate(static_cast<uint32_t>(m_Desc.NumRenderTargets + 1u));	// + the depth-stencil target for rendering it as a texture
			m_CPUSRVHandle	= pRWSrvHeap->Allocate(static_cast<uint32_t>(m_Desc.NumRenderTargets + 1u));	// + the depth-stencil target for rendering it as a texture
			m_RTVHandle		= pRtvHeap->Allocate(static_cast<uint32_t>(m_Desc.NumRenderTargets));
			m_DSVHandle		= pDsvHeap->Allocate(static_cast<uint32_t>(1u));
		}

		// Create a shader resource view for each render target so that each individual target can be addressed
		// Create the appropriate render target views for appropriate binding on the pipeline
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = BufferFormatToD3D(m_Desc.RenderTarget.Format);
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ArraySize = 1;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.f;

		// Create the desired number of render target
		// Even if only one render target is present, it stays a texture array -> just the size was set to 1 on resource creation
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = BufferFormatToD3D(m_Desc.RenderTarget.Format);
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.ArraySize = 1; // Only one Tex2D

		D3DDescriptorHandle tempSRVHandle = m_CPUSRVHandle;
		D3DDescriptorHandle tempRTVHandle = m_RTVHandle;
		for (uint32_t i = 0u; i < m_Desc.NumRenderTargets; ++i)
		{
			// Determine where to start in the rtv array
			srvDesc.Texture2DArray.FirstArraySlice = i;
			rtvDesc.Texture2DArray.FirstArraySlice = i;

			// Create view and store handles
			gD3DContext->Device()->CreateShaderResourceView(m_RTResources.Get(), &srvDesc, tempSRVHandle.GetCPUHandle());
			tempSRVHandle += m_SRVDescriptorSize;
			gD3DContext->Device()->CreateRenderTargetView(m_RTResources.Get(), &rtvDesc, tempRTVHandle.GetCPUHandle());
			tempRTVHandle += m_RTVDescriptorSize;
		}

		// Create a default depth/stencil view for the depth stencil target so it can be used in the pipeline
		gD3DContext->Device()->CreateDepthStencilView(m_DSResource.Get(), nullptr, m_DSVHandle.GetCPUHandle());
		srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DepthStencilToSRVD3D(m_Desc.DepthStencilTarget.Format);
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
		gD3DContext->Device()->CreateShaderResourceView(m_DSResource.Get(), &srvDesc, tempSRVHandle.GetCPUHandle());
		gD3DContext->Device()->CopyDescriptorsSimple(m_Desc.NumRenderTargets + 1u, m_GPUSRVHandle.GetCPUHandle(), m_CPUSRVHandle.GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	D3DBottomLevelAccelerationStructure::D3DBottomLevelAccelerationStructure(const std::wstring& debugName, const BottomLevelAccelerationStructureDesc& desc) :
		BottomLevelAccelerationStructure(debugName, desc)
	{
		// DXR interfaces
		ID3D12Device5* pDevice = gD3DContext->RaytracingDevice();
		ID3D12GraphicsCommandList4* pCommandList = gD3DContext->RaytracingCommandList();

		D3D12_RAYTRACING_GEOMETRY_DESC geometry = {};
		geometry.Type = GeometryTypeToD3D(desc.Type);
		geometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		if (geometry.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES)
		{
			// Sanity checks
			SKTBD_CORE_ASSERT(m_Desc.Triangles.pVertexBuffer != nullptr, "A vertex buffer is required to create this object's acceleration structure.");
			SKTBD_CORE_ASSERT(m_Desc.Triangles.VertexCount != 0u, "A vertex buffer is required to create this object's acceleration structure.");
			SKTBD_CORE_ASSERT(m_Desc.Triangles.pIndexBuffer != nullptr, "An index buffer is required to create this object's acceleration structure.");
			SKTBD_CORE_ASSERT(m_Desc.Triangles.IndexCount != 0u, "An index buffer is required to create this object's acceleration structure.");

			// Get the vertex & index start locations
			const uint32_t vertexStride = m_Desc.Triangles.pVertexBuffer->GetLayout().GetStride();
			const uint32_t indexStride = m_Desc.Triangles.pIndexBuffer->GetStride();
			const D3D12_GPU_VIRTUAL_ADDRESS vertexStart = static_cast<D3DDefaultBuffer*>(m_Desc.Triangles.pVertexBuffer->GetBuffer())->GetResource()->GetGPUVirtualAddress() + m_Desc.Triangles.StartVertexLocation * vertexStride;
			const D3D12_GPU_VIRTUAL_ADDRESS indexStart = static_cast<D3DDefaultBuffer*>(m_Desc.Triangles.pIndexBuffer->GetBuffer())->GetResource()->GetGPUVirtualAddress() + m_Desc.Triangles.StartIndexLocation * indexStride;

			geometry.Triangles.VertexBuffer.StartAddress = vertexStart;
			geometry.Triangles.VertexBuffer.StrideInBytes = vertexStride;
			geometry.Triangles.VertexCount = m_Desc.Triangles.VertexCount;
			geometry.Triangles.VertexFormat = BufferFormatToD3D(m_Desc.Triangles.pVertexBuffer->GetLayout().GetVertexPositionFormat());
			geometry.Triangles.IndexBuffer = indexStart;
			geometry.Triangles.IndexFormat = BufferFormatToD3D(m_Desc.Triangles.pIndexBuffer->GetFormat());
			geometry.Triangles.IndexCount = m_Desc.Triangles.IndexCount;
			geometry.Triangles.Transform3x4 = NULL;
		}
		else
		{
			// Sanity checks
			SKTBD_CORE_ASSERT(m_Desc.Procedurals.AABBCount > 0u, "Cannot instanciate a zero procedural primitive BLAS.");
			SKTBD_CORE_ASSERT(m_Desc.Procedurals.AABB.pProceduralGeometryAABBBuffer != nullptr, "A procedural primitive buffer that contains the AABBs is required to create this object's acceleration structure");

			geometry.AABBs.AABBCount = desc.Procedurals.AABBCount;
			geometry.AABBs.AABBs.StrideInBytes = sizeof(D3D12_RAYTRACING_AABB);
			geometry.AABBs.AABBs.StartAddress = static_cast<D3DDefaultBuffer*>(desc.Procedurals.AABB.pProceduralGeometryAABBBuffer)->GetGPUVirtualAddress() + desc.Procedurals.AABB.Offset * geometry.AABBs.AABBs.StrideInBytes;
		}

		// Query the the memory needed to build the BLAS and to store the fully built structure
		// These are reffered to as the scratch and result buffers respectively
		// In order to perform this query, a description of the above initialised geometry must be defined
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS preBuildDesc = {};
		preBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		preBuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		preBuildDesc.pGeometryDescs = &geometry;
		preBuildDesc.NumDescs = 1;
		preBuildDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
		pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&preBuildDesc, &info);	// "info" now holds the buffers sizes

		// Allocate GPU buffers for the BLAS
		// These must be UAVs
		// Create a generic buffer description and heap before assigning the particular sizes
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Alignment = 0;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;		// UAV
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.Height = 1;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.MipLevels = 1;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;

		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 0;
		heapProperties.VisibleNodeMask = 0;

		bufferDesc.Width = ROUND_UP(info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(m_Scratch.ReleaseAndGetAddressOf())));

		bufferDesc.Width = ROUND_UP(info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(m_Result.ReleaseAndGetAddressOf())));

#ifndef SKTBD_SHIP
		m_Result->SetName(m_DebugName.c_str());
		std::wstring scratchName = m_DebugName + L" - Scratch Buffer";
		m_Scratch->SetName(scratchName.c_str());
#endif

		// Now that the buffers are created, build the acceleration structure
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
		asDesc.Inputs.Type = preBuildDesc.Type;
		asDesc.Inputs.DescsLayout = preBuildDesc.DescsLayout;
		asDesc.Inputs.NumDescs = preBuildDesc.NumDescs;
		asDesc.Inputs.pGeometryDescs = preBuildDesc.pGeometryDescs;
		asDesc.Inputs.Flags = preBuildDesc.Flags;
		asDesc.DestAccelerationStructureData = m_Result->GetGPUVirtualAddress();
		asDesc.ScratchAccelerationStructureData = m_Scratch->GetGPUVirtualAddress();

		// Build the AS
		pCommandList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

		// Ideally, we would want to wait for completion of the building of the last bottom level acceleration structure
		// But for abstraction simplicity, we'll introduce a wait after each to ensure the last one is being built before proceeding (FIXME?)
		// This is particularly important as the construction of the top-level hierarchy may be called right afterwards, before executing the command list (Nvidia)
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = m_Result.Get();
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		pCommandList->ResourceBarrier(1, &uavBarrier);
	}

	D3DBottomLevelAccelerationStructure::~D3DBottomLevelAccelerationStructure()
	{
	}

	void D3DBottomLevelAccelerationStructure::Release()
	{
		gD3DContext->DeferredRelease(m_Scratch.Get());
		gD3DContext->DeferredRelease(m_Result.Get());
		gD3DContext->SetDeferredReleasesFlag();
	}

	D3DTopLevelAccelerationStructure::D3DTopLevelAccelerationStructure(const std::wstring& debugName, const TopLevelAccelerationStructureDesc& desc) :
		TopLevelAccelerationStructure(debugName, desc)
	{
		// Sanity checks
		const uint32_t instanceCount = static_cast<uint32_t>(desc.vInstanceIDs.size());
		SKTBD_CORE_ASSERT(instanceCount != 0u, "Cannot create an empty acceleration structure! Have you created geometry instances before calling this function?");

		// DXR interfaces
		ID3D12Device5* pDevice = gD3DContext->RaytracingDevice();
		ID3D12GraphicsCommandList4* pCommandList = gD3DContext->RaytracingCommandList();

		// Query the the memory needed to build the TLAS and to store the fully built structure
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS preBuildDesc = {};
		preBuildDesc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		preBuildDesc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		preBuildDesc.NumDescs = instanceCount;
		preBuildDesc.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
		pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&preBuildDesc, &info);	// "info" now holds the buffers sizes

		// Allocate the UAV buffers for the TLAS
		D3D12_RESOURCE_DESC bufferDesc = {};
		bufferDesc.Alignment = 0;
		bufferDesc.DepthOrArraySize = 1;
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;		// UAV
		bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		bufferDesc.Height = 1;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		bufferDesc.MipLevels = 1;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.SampleDesc.Quality = 0;

		D3D12_HEAP_PROPERTIES heapProperties = {};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProperties.CreationNodeMask = 0;
		heapProperties.VisibleNodeMask = 0;

		bufferDesc.Width = ROUND_UP(info.ScratchDataSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(m_Scratch.ReleaseAndGetAddressOf())));

		bufferDesc.Width = ROUND_UP(info.ResultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(m_Result.ReleaseAndGetAddressOf())));

		bufferDesc.Width = ROUND_UP(sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * instanceCount, D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);
		bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D_CHECK_FAILURE(pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_Instance.ReleaseAndGetAddressOf())));

#ifndef SKTBD_SHIP
		m_Result->SetName(m_DebugName.c_str());
		std::wstring tmpName = m_DebugName + L" - Scratch Buffer";
		m_Scratch->SetName(tmpName.c_str());
		tmpName = m_DebugName + L" - Instance Buffer";
		m_Instance->SetName(tmpName.c_str());
#endif

		// Copy the instance data into the instance buffer
		std::vector<std::unique_ptr<class BottomLevelAccelerationStructure>>& blas = *m_Desc.vBLAS;
		D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
		m_Instance->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));
		ZeroMemory(instanceDescs, bufferDesc.Width);		// bufferDesc.Width still holds the size of the instance buffer

		// Create the description for each instance
		for (uint32_t i = 0u; i < instanceCount; ++i)
		{
			// Instance ID visible in the shader in InstanceID()
			instanceDescs[m_Desc.vBufferIndices[i]].InstanceID = m_Desc.vInstanceIDs[i];
			// Instance Mask that can be used to include/reject groups of instance within a TraceRay
			instanceDescs[m_Desc.vBufferIndices[i]].InstanceMask = static_cast<uint32_t>(blas[m_Desc.vMeshIDs[i]]->GetDesc().Type);
			// Index of the hit group invoked upon intersection
			instanceDescs[m_Desc.vBufferIndices[i]].InstanceContributionToHitGroupIndex = blas[m_Desc.vMeshIDs[i]]->GetDesc().Type & GeometryType_Triangles ? 0u : RaytracingHitGroupType_Count;
			// Instance flags, including backface culling, winding, etc 
			instanceDescs[m_Desc.vBufferIndices[i]].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			// Instance transform matrix
			DirectX::XMMATRIX m = MatrixTranspose(m_Desc.vTransforms[i]); // GLM is column major, the INSTANCE_DESC is row major
			memcpy(instanceDescs[m_Desc.vBufferIndices[i]].Transform, &m, sizeof(instanceDescs[m_Desc.vBufferIndices[i]].Transform));
			// Get access to the bottom level
			instanceDescs[m_Desc.vBufferIndices[i]].AccelerationStructure = static_cast<D3DBottomLevelAccelerationStructure*>(blas[m_Desc.vMeshIDs[i]].get())->GetGPUVirtualAddress();
		}

		m_Instance->Unmap(0, nullptr);

		// Create a descriptor of the requested builder work, to generate a top-level AS from the input parameters
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		buildDesc.Inputs.Type = preBuildDesc.Type;
		buildDesc.Inputs.DescsLayout = preBuildDesc.DescsLayout;
		buildDesc.Inputs.InstanceDescs = m_Instance->GetGPUVirtualAddress();
		buildDesc.Inputs.NumDescs = preBuildDesc.NumDescs;
		buildDesc.Inputs.Flags = preBuildDesc.Flags;
		buildDesc.DestAccelerationStructureData = m_Result->GetGPUVirtualAddress();
		buildDesc.ScratchAccelerationStructureData = m_Scratch->GetGPUVirtualAddress();

		// Build the top-level AS
		pCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		// Wait for the builder to complete by setting a barrier on the resulting
		// buffer. This can be important in case the rendering is triggered
		// immediately afterwards, without executing the command list
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = m_Result.Get();
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		pCommandList->ResourceBarrier(1, &uavBarrier);

		// Create a shader resource view for this top acceleration structure
		auto* pRWSrvHeap	= gD3DContext->GetRWSrvHeap();
		m_SRVHandle			= pRWSrvHeap->Allocate();
		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		viewDesc.Format = DXGI_FORMAT_UNKNOWN;
		viewDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		viewDesc.RaytracingAccelerationStructure.Location = m_Result->GetGPUVirtualAddress();
		gD3DContext->Device()->CreateShaderResourceView(nullptr, &viewDesc, m_SRVHandle.GetCPUHandle());
	}

	D3DTopLevelAccelerationStructure::~D3DTopLevelAccelerationStructure()
	{
	}

	void D3DTopLevelAccelerationStructure::PerformUpdate(uint32_t bufferIndex, const float4x4& transform)
	{
		// Sanity checks
		const uint32_t instanceCount = static_cast<uint32_t>(m_Desc.vInstanceIDs.size());
		SKTBD_CORE_ASSERT(instanceCount != 0u, "Cannot create an empty acceleration structure! Have you created geometry instances before calling this function?");

		// DXR interfaces
		ID3D12GraphicsCommandList4* pCommandList = gD3DContext->RaytracingCommandList();

		// Copy the instance data into the instance buffer
		D3D12_RAYTRACING_INSTANCE_DESC* instanceDescs;
		m_Instance->Map(0, nullptr, reinterpret_cast<void**>(&instanceDescs));

		// Instance transform matrix
		m_Desc.vTransforms[bufferIndex] = transform;
		DirectX::XMMATRIX m = XMMatrixTranspose(transform);
		memcpy(instanceDescs[bufferIndex].Transform, &m, sizeof(instanceDescs[bufferIndex].Transform));

		m_Instance->Unmap(0, nullptr);

		// Create a descriptor of the requested builder work, to generate a top-level AS from the input parameters
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
		buildDesc.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		buildDesc.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		buildDesc.Inputs.InstanceDescs = m_Instance->GetGPUVirtualAddress();
		buildDesc.Inputs.NumDescs = instanceCount;
		buildDesc.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		buildDesc.DestAccelerationStructureData = m_Result->GetGPUVirtualAddress();
		buildDesc.ScratchAccelerationStructureData = m_Scratch->GetGPUVirtualAddress();
		buildDesc.SourceAccelerationStructureData = m_Result->GetGPUVirtualAddress();

		// Build the top-level AS
		pCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

		// Wait for the builder to complete by setting a barrier on the resulting
		// buffer. This can be important in case the rendering is triggered
		// immediately afterwards, without executing the command list
		D3D12_RESOURCE_BARRIER uavBarrier = {};
		uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uavBarrier.UAV.pResource = m_Result.Get();
		uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		pCommandList->ResourceBarrier(1, &uavBarrier);
	}

	void D3DTopLevelAccelerationStructure::Release()
	{
		gD3DContext->DeferredRelease(m_Scratch.Get());
		gD3DContext->DeferredRelease(m_Result.Get());
		gD3DContext->DeferredRelease(m_Instance.Get());
		gD3DContext->SetDeferredReleasesFlag();
	}
	
}
