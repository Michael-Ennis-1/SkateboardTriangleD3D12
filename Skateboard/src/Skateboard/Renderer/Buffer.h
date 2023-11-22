#pragma once
#include "InternalFormats.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Log.h"

#include <string>
#include <vector>

namespace Skateboard
{
	struct BufferElement
	{
		std::string SemanticName;		// Name of the semantic used in the shader (careful with different APIs!)
		ShaderDataType_ Type;			// Abstraction of the shader data type of this element
		uint32_t InputSlot;				// The input slot of this element for this semantic
		uint32_t Size;					// The size of the element (obtained automatically)
		uint32_t Offset;				// The offset of the element in the overall layout (obtained automatically)

		BufferElement() = default;
		BufferElement(const std::string& name, ShaderDataType_ type, uint32_t inputSlot = 0) :
			SemanticName(name), Type(type), InputSlot(inputSlot), Size(ShaderDataTypeSizeInBytes(type)), Offset(0)
		{
		}

		/// <summary>
		/// An shader element contains one or multiple components. For instance:
		///		- A float3 contains 3 components (3 floats)
		///		- A int2 contains 2 components (2 ints)
		///		- A float4x4 contains 16 components (4x4 floats)
		/// This function retrieves this information outside of an API context.
		/// </summary>
		/// <returns>Returns the number of components present in this element</returns>
		uint32_t GetComponentCount() const;
	};

	class BufferLayout
	{
	public:
		BufferLayout() : m_Stride(0u) {};
		BufferLayout(const std::initializer_list<BufferElement>& elements) :
			v_Elements(elements),
			m_Stride(0u)
		{
			CalculateOffsetsAndStride();
		}

		/// <summary>
		/// Function to get the default layout for graphics applicaitons.
		/// Custom layouts can be created using the same pattern used in this function for simpler/more complicated layouts
		/// (for instance when using skinning). For static geometry this layout is perfect as it gives all the necessary
		/// elements to render high quality graphics!
		/// </summary>
		/// <returns>The default BufferLayout for unskinned geometry</returns>
		static BufferLayout GetDefaultLayout()
		{
			return {
				{ "POSITION", ShaderDataType_::Float3 },
				{ "TEXCOORD", ShaderDataType_::Float2 },
				{ "NORMAL", ShaderDataType_::Float3 },
				{ "TANGENT", ShaderDataType_::Float3 },
				{ "BITANGENT", ShaderDataType_::Float3 }
			};
		}

		/// <summary>
		/// Retrieve the different elements in this layout as a vector.
		/// See BufferElement for an overview of what elements contain.
		/// </summary>
		/// <returns></returns>
		const std::vector<BufferElement>& GetElements() const { return v_Elements; }
		/// <summary>
		/// Retrieves the overall stride of this layout, that is how many bytes in total
		/// each vertex occupies in the vertex buffer.
		/// </summary>
		/// <returns>The overall stride, in bytes</returns>
		uint32_t GetStride() const { return m_Stride; }

		constexpr BufferFormat_ GetVertexPositionFormat() const { return BufferFormat_R32G32B32_FLOAT; }

		// Iterators for nice C++ functionalities
		std::vector<BufferElement>::iterator begin() { return v_Elements.begin(); }
		std::vector<BufferElement>::iterator end() { return v_Elements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return v_Elements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return v_Elements.end(); }

	private:
		// This function is called privately to calculate the internal offsets of the different elements
		// as well as the overall stride of the layout. These calculation can only be performed once the
		// layout contains all the elements that define it.
		void CalculateOffsetsAndStride()
		{
			uint32_t offset = 0u;
			for (BufferElement& element : v_Elements)
			{
				element.Offset = offset;
				offset += element.Size;
				m_Stride += element.Size;

				SKTBD_CORE_ASSERT(!element.SemanticName.compare("POSITION") && element.Type == ShaderDataType_::Float3 || element.SemanticName.compare("POSITION"), "The poisition semantic can only accept a vertex type of 3 regular floats. Please change your layout.");
			}
		}

	private:
		std::vector<BufferElement> v_Elements;
		uint32_t m_Stride;
	};

	struct DefaultBufferDesc
	{
		BufferFormat_ Format;
		uint32_t ElementCount;
		uint32_t ElementSize;
		void* pDataToTransfer;
		uint64_t Width;

		uint32_t BufferStride;
		bool OverrideStride{ false };

		static DefaultBufferDesc Init(void* pDataToTransfer, uint32_t elementCount, uint32_t elementSize);
	};

	struct UploadBufferDesc
	{
		BufferAccessFlag_ AccessFlag;
		BufferFormat_ Format;
		uint32_t ElementCount;
		uint32_t ElementSize;
		uint64_t Width;
		bool ForceStaticData{false};

		static UploadBufferDesc Init(bool isConstantBuffer, uint32_t elementCount, uint32_t elementSize);
	};

	struct UnorderedAccessBufferDesc
	{
		BufferAccessFlag_ AccessFlag;
		BufferFormat_ Format;

		uint64_t Width;
		uint32_t Height;
		uint32_t Depth;

		static UnorderedAccessBufferDesc Init(uint64_t width, uint32_t height, uint32_t depth, bool cpuWritable = false, BufferFormat_ format = BufferFormat_DEFAULT_BACKBUFFER);
	};

	struct ByteAddressBufferDesc
	{
		BufferAccessFlag_ AccessFlag;
		BufferFormat_ Format;
		uint32_t ElementCount;
		uint32_t ElementSize;
		void* pDataToTransfer;
		uint64_t Width;
		static ByteAddressBufferDesc Init(void* pDataToTransfer, uint32_t elementCount);
	};

	struct TextureDesc
	{
		GPUResourceType_ Type;
		BufferFormat_ Format;
		uint64_t Width;
		uint32_t Height;
		uint32_t Depth;
		uint32_t ID;
	};

	typedef struct {
		BufferFormat_ Format;
		bool CubeMapTarget;
		union
		{
			float4 ClearColour;
			struct {
				float Depth;
				uint8_t Stencil;
			} DepthStencil;
		};
	} RenderTargetDesc, DepthStencilTargetDesc;

	struct FrameBufferDesc
	{
		uint32_t Width, Height;
		RenderTargetDesc RenderTarget;
		DepthStencilTargetDesc DepthStencilTarget;
		uint32_t NumRenderTargets = 1u;

		static FrameBufferDesc InitAsFullRenderTarget(uint32_t width, uint32_t height, BufferFormat_ rtvFormat = BufferFormat_DEFAULT_BACKBUFFER, BufferFormat_ dsvFormat = BufferFormat_DEFAULT_DEPTHSTENCIL);
		static FrameBufferDesc InitAsDepthStencilTargetOnly(uint32_t width, uint32_t height, BufferFormat_ format = BufferFormat_DEFAULT_DEPTHSTENCIL);
		static FrameBufferDesc InitAsCubeMap(uint32_t width, uint32_t height, BufferFormat_ rtvFormat = BufferFormat_DEFAULT_BACKBUFFER, BufferFormat_ dsvFormat = BufferFormat_DEFAULT_DEPTHSTENCIL);
	};

	struct GeometryTriangleDesc
	{
		class VertexBuffer* pVertexBuffer;
		uint32_t StartVertexLocation;
		uint32_t VertexCount;
		class IndexBuffer* pIndexBuffer;
		uint32_t StartIndexLocation;
		uint32_t IndexCount;
	};

	struct GeometryAABBDesc
	{
		uint64_t AABBCount;
		struct
		{
			uint64_t Offset;
			class DefaultBuffer* pProceduralGeometryAABBBuffer;
		}AABB;
	};

	struct BottomLevelAccelerationStructureDesc
	{
		GeometryType_ Type;
		union
		{
			GeometryTriangleDesc Triangles;
			GeometryAABBDesc Procedurals;
		};
	};

	struct TopLevelAccelerationStructureDesc
	{
		std::vector<float4x4> vTransforms;
		std::vector<uint32_t> vInstanceIDs;
		std::vector<uint32_t> vMeshIDs;
		std::vector<uint32_t> vBufferIndices;
		std::vector<std::unique_ptr<class BottomLevelAccelerationStructure>>* vBLAS;
	};

	// A 'GpuResource' is a resource that will be transferred to the platform GPU in some way
	class GPUResource
	{
	public:
#ifndef SKTBD_SHIP
		GPUResource(const std::wstring& debugName, GPUResourceType_ type) : m_DebugName(debugName), m_Type(type) {}
		const std::wstring& GetDebugName() const { return m_DebugName; }
#else
		GpuResource(const std::wstring& debugName) {}
		const std::wstring& GetDebugName() const { return L""; }
#endif
		GPUResourceType_ GetType() const { return m_Type; }

		virtual void Release() = 0;

	protected:
#ifndef SKTBD_SHIP
		std::wstring m_DebugName;
#endif // !SKTBD_SHIP
		GPUResourceType_ m_Type;
	};

	class DefaultBuffer : public GPUResource
	{
	protected:
		DefaultBuffer(const std::wstring& debugName, const DefaultBufferDesc& desc) : GPUResource(debugName, GPUResourceType_DefaultBuffer), m_Desc(desc) {}

	public:
		virtual ~DefaultBuffer() {}
		static DefaultBuffer* Create(const std::wstring& debugName, const DefaultBufferDesc& bufferDesc);

		const DefaultBufferDesc& GetDesc() const { return m_Desc; }
		void SetDesc(const DefaultBufferDesc& desc) { m_Desc = desc; }

	protected:
		DefaultBufferDesc m_Desc;
	};

	class UploadBuffer : public GPUResource
	{
	protected:
		UploadBuffer(const std::wstring& debugName, const UploadBufferDesc& desc) : GPUResource(debugName, GPUResourceType_UploadBuffer), m_Desc(desc) {}
	public:
		virtual ~UploadBuffer() {}
		static UploadBuffer* Create(const std::wstring& debugName, const UploadBufferDesc& bufferDesc);

		virtual void CopyData(uint32_t elementIndex, const void* pData) = 0;
		virtual void CopyData(uint32_t elementIndex, const void* pData, uint32_t sizeInBytes) = 0;


		const UploadBufferDesc& GetDesc() const { return m_Desc; }
		void SetDesc(const UploadBufferDesc& desc) { m_Desc = desc; }

	protected:
		UploadBufferDesc m_Desc;
	};

	class ByteAddressBuffer : public GPUResource
	{
	protected:
		ByteAddressBuffer(const std::wstring& debugName, const ByteAddressBufferDesc& desc)
		:
			GPUResource(debugName, GPUResourceType_ByteAddress)
		,	m_Desc(desc)
		{}

	public:
		virtual ~ByteAddressBuffer(){}
		static ByteAddressBuffer* Create(const std::wstring& debugName, const ByteAddressBufferDesc& bufferDesc);

		virtual void CopyData(const void* pData, uint32_t elementIndex, uint32_t elementCount) = 0;

		const ByteAddressBufferDesc& GetDesc() const { return m_Desc; }
		void SetDesc(const ByteAddressBufferDesc& desc) { m_Desc = desc; }

	protected:
		ByteAddressBufferDesc m_Desc;
	};

	class UnorderedAccessBuffer : public GPUResource
	{
	protected:
		UnorderedAccessBuffer(const std::wstring& debugName, const UnorderedAccessBufferDesc& desc) : GPUResource(debugName, GPUResourceType_UnorderedAccessBuffer), m_Desc(desc) {}
	public:
		virtual ~UnorderedAccessBuffer() {}
		static UnorderedAccessBuffer* Create(const std::wstring& debugName, const UnorderedAccessBufferDesc& bufferDesc);

		virtual void Resize(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth = 1) = 0;
		virtual void TransitionToType(GPUResourceType_ newType) = 0;

		const UnorderedAccessBufferDesc& GetDesc() const { return m_Desc; }
		void SetDesc(const UnorderedAccessBufferDesc& desc) { m_Desc = desc; }

	protected:
		UnorderedAccessBufferDesc m_Desc;
	};

	class Texture : public GPUResource
	{
	protected:
		Texture(const std::wstring& debugName, const TextureDesc& desc) : GPUResource(debugName, desc.Type), m_Desc(desc) {}
	public:
		virtual ~Texture() {}

		const TextureDesc& GetDesc() const { return m_Desc; }
		void SetDesc(const TextureDesc& desc) { m_Desc = desc; }

	protected:
		TextureDesc m_Desc;
	};

	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() {}

		static IndexBuffer* Create(const std::wstring& debugName, uint32_t* indices, uint32_t count);

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual uint32_t GetIndexCount() const = 0;
		constexpr uint32_t GetStride() const { return sizeof(uint32_t); }	// This engine will always use 32 bits indices.
		constexpr BufferFormat_ GetFormat() const { return BufferFormat_R32_UINT; }

		virtual DefaultBuffer* GetBuffer() const = 0;
		virtual void Release() = 0;
	};

	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() {}

		static VertexBuffer* Create(const std::wstring& debugName, void* vertices, uint32_t size, const BufferLayout& layout = BufferLayout::GetDefaultLayout());

		virtual void Bind() const = 0;
		virtual void Unbind() const = 0;

		virtual void SetLayout(const BufferLayout& layout) = 0;
		virtual const BufferLayout& GetLayout() const = 0;

		virtual DefaultBuffer* GetBuffer() const = 0;
		virtual void Release() = 0;
	};

	// A render target could be a colour buffer or a depth/stencil buffer

	class FrameBuffer : public GPUResource
	{
	protected:
		FrameBuffer(const std::wstring& debugName) : GPUResource(debugName, GPUResourceType_FrameBuffer) {}

	public:
		virtual ~FrameBuffer() {}

		static FrameBuffer* Create(const std::wstring& debugName, const FrameBufferDesc& desc);

		virtual void Bind(uint32_t renderTargetIndex = 0u) const = 0;
		virtual void Unbind() const = 0;

		virtual const FrameBufferDesc& GetDesc() const = 0;

		virtual void Resize(uint32_t newWidth, uint32_t newHeight) = 0;
		virtual float GetAspectRatio() const = 0;
		//virtual void ClearRenderTarget(uint32_t renderTargetIndex, uint32_t value) = 0;

		virtual void* GetRenderTargetAsImGuiTextureID(uint32_t renderTargetIndex = 0u) const = 0;
		virtual void* GetDepthStencilTargetAsImGuiTextureID(uint32_t renderTargetIndex = 0u) const = 0;
	};

	class BottomLevelAccelerationStructure : public GPUResource
	{
	protected:
		BottomLevelAccelerationStructure(const std::wstring& debugName, const BottomLevelAccelerationStructureDesc& desc) :
			GPUResource(debugName, GPUResourceType_BottomLevelAccelerationStructure), m_Desc(desc) {}
	public:
		static BottomLevelAccelerationStructure* Create(const std::wstring& debugName, const BottomLevelAccelerationStructureDesc& desc);
		virtual ~BottomLevelAccelerationStructure() {}

		const BottomLevelAccelerationStructureDesc& GetDesc() const { return m_Desc; }

	protected:
		BottomLevelAccelerationStructureDesc m_Desc;
	};

	class TopLevelAccelerationStructure : public GPUResource
	{
	protected:
		TopLevelAccelerationStructure(const std::wstring& debugName, const TopLevelAccelerationStructureDesc& desc) :
			GPUResource(debugName, GPUResourceType_TopLevelAccelerationStructure), m_Desc(desc) {}
	public:
		static TopLevelAccelerationStructure* Create(const std::wstring& debugName, const TopLevelAccelerationStructureDesc& desc);
		virtual ~TopLevelAccelerationStructure() {}

		virtual void PerformUpdate(uint32_t bufferIndex, const float4x4& transform) = 0;

		const TopLevelAccelerationStructureDesc& GetDesc() const { return m_Desc; }

	protected:
		TopLevelAccelerationStructureDesc m_Desc;
	};
}
