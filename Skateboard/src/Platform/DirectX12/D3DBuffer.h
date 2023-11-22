#pragma once
#include "Skateboard/Renderer/Buffer.h"
#include "Platform/DirectX12/Memory/D3DMemoryAllocator.h"
#include "D3D.h"

namespace Skateboard
{
	class D3DDefaultBuffer : public DefaultBuffer
	{
	public:
		D3DDefaultBuffer(const std::wstring& debugName, const DefaultBufferDesc& desc);
		D3DDefaultBuffer() = delete;
		D3DDefaultBuffer(const D3DDefaultBuffer& rhs) = delete;
		D3DDefaultBuffer(D3DDefaultBuffer&& rhs) = delete;
		virtual ~D3DDefaultBuffer() final override;



		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_Buffer->GetGPUVirtualAddress(); };
		ID3D12Resource* GetResource() const { return m_Buffer.Get(); }
		ID3D12Resource* GetIntermediateResource() const { return m_IntermediateBuffer.Get(); }

		Microsoft::WRL::ComPtr<ID3D12Resource>& GetResourceComPtr() { return m_Buffer; }
		Microsoft::WRL::ComPtr<ID3D12Resource>& GetIntermediateResourceComPtr() { return m_IntermediateBuffer; }

		uint32_t GetStride() const { return m_DataChunkSize; }

		D3DDescriptorHandle GetSRVHandle() const { return m_SRVHandle; }

		void Release() override;
	protected:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer, m_IntermediateBuffer;
		uint32_t m_DataChunkSize;
		D3DDescriptorHandle m_SRVHandle;
	};

	class D3DUploadBuffer : public UploadBuffer
	{
	public:
		D3DUploadBuffer(const std::wstring& debugName, const UploadBufferDesc& desc);
		D3DUploadBuffer() = delete;
		D3DUploadBuffer(const D3DUploadBuffer& rhs) = delete;
		D3DUploadBuffer(D3DUploadBuffer&& rhs) = delete;
		virtual ~D3DUploadBuffer() final override;

		void CopyData(uint32_t elementIndex, const void* pData) final override;

		void CopyData(uint32_t elementIndex, const void* pData, uint32_t size) override;

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t elementIndex = 0u) const;
		ID3D12Resource* GetResource() const { return m_Buffer.Get(); }
		Microsoft::WRL::ComPtr<ID3D12Resource>& GetResourceComPtr() { return m_Buffer; }
		D3DDescriptorHandle GetSRVHandle() const { return m_SRVHandle; }
		uint32_t GetStride() const { return m_DataChunkSize; }

		void Release() override;
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer;
		uint32_t m_DataChunkSize;
		uint8_t* m_MappedData;
		D3DDescriptorHandle m_SRVHandle;
	};

	class D3DUnorderedAccessBuffer : public UnorderedAccessBuffer
	{
	public:
		D3DUnorderedAccessBuffer(const std::wstring& debugName, const UnorderedAccessBufferDesc& desc);
		D3DUnorderedAccessBuffer() = delete;
		D3DUnorderedAccessBuffer(const D3DUnorderedAccessBuffer& rhs) = delete;
		D3DUnorderedAccessBuffer(D3DUnorderedAccessBuffer&& rhs) = delete;
		virtual ~D3DUnorderedAccessBuffer() final override;



		D3D12_GPU_VIRTUAL_ADDRESS GetResourceGPUVirtualAddress() const { return m_Buffer->GetGPUVirtualAddress(); };
		ID3D12Resource* GetResource() const { return m_Buffer.Get(); }

		Microsoft::WRL::ComPtr<ID3D12Resource>& GetResourceComPtr() { return m_Buffer; }

		virtual void Resize(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth = 1) final override;
		virtual void TransitionToType(GPUResourceType_ newType) final override;

		D3DDescriptorHandle GetSRVHandle() const { return m_SRVHandle + m_SRVHandleSize; }
		D3DDescriptorHandle GetUAVHandle() const { return m_SRVHandle; }


		void Release() override;
	private:
		void BuildResource(bool allocateDescriptorHeap);

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer;
		D3DDescriptorHandle m_SRVHandle;
		const uint32_t m_SRVHandleSize;
	};

	class D3DByteAddressBuffer : public ByteAddressBuffer
	{
	public:
		D3DByteAddressBuffer(const std::wstring& debugName, const ByteAddressBufferDesc& desc);
		DISABLE_COPY_AND_MOVE(D3DByteAddressBuffer);
		virtual ~D3DByteAddressBuffer() final override;

		D3D12_GPU_VIRTUAL_ADDRESS GetResourceGPUVirtualAddress() const { return m_Buffer->GetGPUVirtualAddress(); };
		ID3D12Resource* GetResource() const { return m_Buffer.Get(); }

		Microsoft::WRL::ComPtr<ID3D12Resource>& GetResourceComPtr() { return m_Buffer; }

		D3DDescriptorHandle GetSRVHandle() const { return m_SRVHandle + m_SRVHandleSize; }
		D3DDescriptorHandle GetUAVHandle() const { return m_SRVHandle; }

		void CopyData(const void* pData, uint32_t elementIndex, uint32_t elementCount) override;

		void Release() override;
	private:
		void BuildResource(bool allocateDescriptorHeap);

		uint32_t m_DataChunkSize;
		uint8_t* m_MappedData;
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer;
		D3DDescriptorHandle m_SRVHandle;
		const uint32_t m_SRVHandleSize;
	};

	class D3DTexture : public Texture
	{
	public:
		D3DTexture(const std::wstring& debugName, const TextureDesc& desc);
		D3DTexture() = delete;
		D3DTexture(const D3DTexture& rhs) = delete;
		D3DTexture(D3DTexture&& rhs) = delete;
		virtual ~D3DTexture() final override;

		void CreateSRV();



		D3D12_GPU_VIRTUAL_ADDRESS GetResourceGPUVirtualAddress() const { return m_Buffer->GetGPUVirtualAddress(); };
		ID3D12Resource* GetResource() const { return m_Buffer.Get(); }
		ID3D12Resource* GetIntermediateResource() const { return m_IntermediateBuffer.Get(); }

		Microsoft::WRL::ComPtr<ID3D12Resource>& GetResourceComPtr() { return m_Buffer; }
		Microsoft::WRL::ComPtr<ID3D12Resource>& GetIntermediateResourceComPtr() { return m_IntermediateBuffer; }

		D3DDescriptorHandle GetSRVHandle() const { return m_SRVHandle; }

		void Release() override;
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer, m_IntermediateBuffer;
		D3DDescriptorHandle m_SRVHandle;
	};

	class D3DIndexBuffer : public IndexBuffer
	{
	public:
		D3DIndexBuffer(const std::wstring& debugName, uint32_t* indices, uint32_t count);
		D3DIndexBuffer() = delete;
		D3DIndexBuffer(const D3DIndexBuffer& rhs) = delete;
		D3DIndexBuffer(D3DIndexBuffer&& rhs) = delete;
		virtual ~D3DIndexBuffer() final override;


		virtual void Bind() const final override;
		virtual void Unbind() const final override {};

		virtual uint32_t GetIndexCount() const final override { return m_IndexCount; }

		const D3D12_INDEX_BUFFER_VIEW& GetView() const { return m_IndexBufferView; }

		virtual DefaultBuffer* GetBuffer() const final override { return m_IndexBuffer.get(); }

		void Release() override;
	private:
		uint32_t m_IndexCount;
		std::unique_ptr<D3DDefaultBuffer> m_IndexBuffer;
		D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	};

	class D3DVertexBuffer : public VertexBuffer
	{
	public:
		D3DVertexBuffer(const std::wstring& debugName, void* vertices, uint32_t size, const BufferLayout& layout);
		D3DVertexBuffer() = delete;
		D3DVertexBuffer(const D3DVertexBuffer& rhs) = delete;
		D3DVertexBuffer(D3DVertexBuffer&& rhs) = delete;
		virtual ~D3DVertexBuffer() final override;

		virtual void Bind() const final override;
		virtual void Unbind() const final override {};

		virtual void SetLayout(const BufferLayout& layout) final override { m_Layout = layout; }
		virtual const BufferLayout& GetLayout() const final override { return m_Layout; }

		virtual D3DDefaultBuffer* GetBuffer() const final override { return m_VertexBuffer.get(); }

		const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return m_VertexBufferView; }

		void Release() override;
	private:
		BufferLayout m_Layout;
		std::unique_ptr<D3DDefaultBuffer> m_VertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	};

	class D3DFrameBuffer : public FrameBuffer
	{
	public:
		D3DFrameBuffer(const std::wstring& debugName, const FrameBufferDesc& desc);
		virtual ~D3DFrameBuffer() final override;

		virtual void Bind(uint32_t renderTargetIndex) const final override;
		virtual void Unbind() const final override;

		virtual const FrameBufferDesc& GetDesc() const final override { return m_Desc; }

		virtual void Resize(uint32_t newWidth, uint32_t newHeight);
		virtual float GetAspectRatio() const { return static_cast<float>(m_Desc.Width) / m_Desc.Height; }

		ID3D12Resource* GetResource() const { return m_RTResources.Get(); }
		virtual void* GetRenderTargetAsImGuiTextureID(uint32_t renderTargetIndex) const final override;
		virtual void* GetDepthStencilTargetAsImGuiTextureID(uint32_t renderTargetIndex) const final override;

		const D3DDescriptorHandle GetSRVHandle(uint32_t colourBufferIndex) const { return m_CPUSRVHandle + colourBufferIndex * m_SRVDescriptorSize; }
		const D3DDescriptorHandle GetRTVHandle(uint32_t colourBufferIndex) const { return m_RTVHandle + colourBufferIndex * m_RTVDescriptorSize; }
		const D3DDescriptorHandle GetDSVHandle() const { return m_DSVHandle; }

		void Release() override;
	private:
		inline void BuildColourRenderTargets();
		inline void BuildDepthStencilTargets();
		inline void BuildDescriptors(bool allocate);

	private:
		FrameBufferDesc m_Desc;
		D3D12_VIEWPORT m_ViewPort;
		D3D12_RECT m_ScissorRect;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_RTResources;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_DSResource;
		D3DDescriptorHandle m_CPUSRVHandle;
		D3DDescriptorHandle m_GPUSRVHandle;
		D3DDescriptorHandle m_RTVHandle;
		D3DDescriptorHandle m_DSVHandle;
		uint32_t m_SRVDescriptorSize;
		uint32_t m_RTVDescriptorSize;
	};

	class D3DBottomLevelAccelerationStructure : public BottomLevelAccelerationStructure
	{
	public:
		D3DBottomLevelAccelerationStructure(const std::wstring& debugName, const BottomLevelAccelerationStructureDesc& desc);
		virtual ~D3DBottomLevelAccelerationStructure() final override;

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_Result->GetGPUVirtualAddress(); }
		void Release() override;
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Scratch;			// Scratch memory used to build the acceleration structure
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Result;			// The result of the building of the acceleration structure
	};

	class D3DTopLevelAccelerationStructure : public TopLevelAccelerationStructure
	{
	public:
		D3DTopLevelAccelerationStructure(const std::wstring& debugName, const TopLevelAccelerationStructureDesc& desc);
		virtual ~D3DTopLevelAccelerationStructure() final override;

		virtual void PerformUpdate(uint32_t bufferIndex, const float4x4& transform) final override;

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return m_Result->GetGPUVirtualAddress(); }
		D3DDescriptorHandle GetSRVHandle() const { return m_SRVHandle; }
		void Release() override;
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Scratch;			// Scratch memory used to build the acceleration structure
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Result;			// The result of the building of the acceleration structure
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Instance;			// The constant buffer of the instance
		D3DDescriptorHandle m_SRVHandle;
	};

}