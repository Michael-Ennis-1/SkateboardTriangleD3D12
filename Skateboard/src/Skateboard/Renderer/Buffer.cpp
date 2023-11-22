#include "sktbdpch.h"
#include "Buffer.h"
#include "Renderer.h"
#include "Platform/Windows/WindowsPlatform.h"
#include "Platform/DirectX12/D3DGraphicsContext.h"
#include "Platform/DirectX12/D3DBuffer.h"

namespace Skateboard
{
	uint32_t BufferElement::GetComponentCount() const
	{
		switch (Type)
		{
		case ShaderDataType_::None:
			return NULL;
		case ShaderDataType_::Bool:
		case ShaderDataType_::Int:
		case ShaderDataType_::Uint:
		case ShaderDataType_::Float:
			return 1;
		case ShaderDataType_::Int2:
		case ShaderDataType_::Uint2:
		case ShaderDataType_::Float2:
			return 2;
		case ShaderDataType_::Int3:
		case ShaderDataType_::Uint3:
		case ShaderDataType_::Float3:
			return 3;
		case ShaderDataType_::Int4:
		case ShaderDataType_::Uint4:
		case ShaderDataType_::Float4:
			return 4;
		default:
			SKTBD_CORE_ASSERT(false, "Could not get component count on this buffer element, impossible shader data type!");
			return NULL;
		}
	}

	DefaultBufferDesc DefaultBufferDesc::Init(void* pDataToTransfer, uint32_t elementCount, uint32_t elementSize)
	{
		DefaultBufferDesc desc = {};
		desc.Format = BufferFormat_UNKNOWN;
		desc.ElementCount = elementCount;
		desc.ElementSize = elementSize;
		desc.pDataToTransfer = pDataToTransfer;
		return desc;
	}

	UploadBufferDesc UploadBufferDesc::Init(bool isConstantBuffer, uint32_t elementCount, uint32_t elementSize)
	{
		UploadBufferDesc desc = {};
		desc.AccessFlag = isConstantBuffer ? BufferAccessFlag_CpuWriteable_Aligned : BufferAccessFlag_CpuWriteable;
		desc.Format = BufferFormat_UNKNOWN;
		desc.ElementCount = elementCount;
		desc.ElementSize = elementSize;
		return desc;
	}

	UnorderedAccessBufferDesc UnorderedAccessBufferDesc::Init(uint64_t width, uint32_t height, uint32_t depth, bool cpuWritable, BufferFormat_ format)
	{
		UnorderedAccessBufferDesc desc = {};
		desc.AccessFlag = cpuWritable ? BufferAccessFlag_CpuWriteable : BufferAccessFlag_GpuOnly;
		desc.Format = format;
		desc.Width = width;
		desc.Height = height;
		desc.Depth = depth;
		return desc;
	}

	ByteAddressBufferDesc ByteAddressBufferDesc::Init(void* pDataToTransfer, uint32_t elementCount)
	{
		ByteAddressBufferDesc desc = {};
		desc.ElementCount = elementCount;
		desc.AccessFlag = BufferAccessFlag_CpuWriteable;
		desc.ElementSize = 4u;
		desc.pDataToTransfer = pDataToTransfer;
		return desc;
	}

	FrameBufferDesc FrameBufferDesc::InitAsFullRenderTarget(uint32_t width, uint32_t height, BufferFormat_ rtvFormat, BufferFormat_ dsvFormat)
	{
		RenderTargetDesc rtdesc = {};
		rtdesc.Format = rtvFormat;
		rtdesc.ClearColour = GRAPHICS_FRAMEBUFFER_DEFAULT_CLEAR_COLOUR;

		DepthStencilTargetDesc dsDesc = {};
		dsDesc.Format = dsvFormat;
		dsDesc.DepthStencil.Depth = 1.f;
		dsDesc.DepthStencil.Stencil = 0u;

		FrameBufferDesc desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.RenderTarget = rtdesc;
		desc.DepthStencilTarget = dsDesc;
		return desc;
	}

	FrameBufferDesc FrameBufferDesc::InitAsDepthStencilTargetOnly(uint32_t width, uint32_t height, BufferFormat_ dsvFormat)
	{
		DepthStencilTargetDesc dsDesc = {};
		dsDesc.Format = dsvFormat;
		dsDesc.DepthStencil.Depth = 1.f;
		dsDesc.DepthStencil.Stencil = 0u;

		FrameBufferDesc desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.DepthStencilTarget = dsDesc;
		desc.NumRenderTargets = 0u;
		return desc;
	}

	FrameBufferDesc FrameBufferDesc::InitAsCubeMap(uint32_t width, uint32_t height, BufferFormat_ rtvFormat, BufferFormat_ dsvFormat)
	{
		RenderTargetDesc rtdesc = {};
		rtdesc.Format = rtvFormat;
		rtdesc.ClearColour = GRAPHICS_FRAMEBUFFER_DEFAULT_CLEAR_COLOUR;

		DepthStencilTargetDesc dsDesc = {};
		dsDesc.Format = dsvFormat;
		dsDesc.DepthStencil.Depth = 1.f;
		dsDesc.DepthStencil.Stencil = 0u;

		FrameBufferDesc desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.RenderTarget = rtdesc;
		desc.DepthStencilTarget = dsDesc;
		desc.NumRenderTargets = 6u;	// A cubemap has 6 render targets
		return desc;
	}

	DefaultBuffer* DefaultBuffer::Create(const std::wstring& debugName, const DefaultBufferDesc& bufferDesc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DDefaultBuffer(debugName, bufferDesc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
			return nullptr;
		}
	}

	UploadBuffer* UploadBuffer::Create(const std::wstring& debugName, const UploadBufferDesc& bufferDesc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DUploadBuffer(debugName, bufferDesc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
			return nullptr;
		}
	}

	UnorderedAccessBuffer* UnorderedAccessBuffer::Create(const std::wstring& debugName, const UnorderedAccessBufferDesc& bufferDesc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DUnorderedAccessBuffer(debugName, bufferDesc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
			return nullptr;
		}
	}

	ByteAddressBuffer* ByteAddressBuffer::Create(const std::wstring& debugName, const ByteAddressBufferDesc& bufferDesc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DByteAddressBuffer(debugName, bufferDesc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
			return nullptr;
		}
	}


	IndexBuffer* IndexBuffer::Create(const std::wstring& debugName, uint32_t* indices, uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DIndexBuffer(debugName, indices, count);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
			return nullptr;
		}
	}

	VertexBuffer* VertexBuffer::Create(const std::wstring& debugName, void* vertices, uint32_t size, const BufferLayout& layout)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DVertexBuffer(debugName, vertices, size, layout);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
			return nullptr;
		}
	}

	FrameBuffer* FrameBuffer::Create(const std::wstring& debugName, const FrameBufferDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DFrameBuffer(debugName, desc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
			return nullptr;
		}
	}

	BottomLevelAccelerationStructure* BottomLevelAccelerationStructure::Create(const std::wstring& debugName, const BottomLevelAccelerationStructureDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DBottomLevelAccelerationStructure(debugName, desc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
			return nullptr;
		}
	}

	TopLevelAccelerationStructure* TopLevelAccelerationStructure::Create(const std::wstring& debugName, const TopLevelAccelerationStructureDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DTopLevelAccelerationStructure(debugName, desc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create an upload buffer, the input API does not exist!");
			return nullptr;
		}
	}
}