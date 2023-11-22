#pragma once
#include "Skateboard/Mathematics.h"
#include "Buffer.h"
#include "InternalFormats.h"
#include "Skateboard/Memory/DescriptorTable.h"

#define PIPELINE_SETTINGS_DEFAULT_DEPTH_BIAS 0

namespace Skateboard
{
	class MeshletModel;
	//Forward declarations
	class Model;

	/*enum class MeshletFlags_ : uint8_t
	{
		MeshletFlags_UseShaderDefinedRootSignature = (0),
		MeshletFlags_UseDefaultMeshletPipeline = (1)
	};

	inline auto operator|(MeshletFlags_ lhs, MeshletFlags_ rhs) noexcept -> uint8_t
	{
		return lhs | rhs;
	}

	inline auto operator|=(uint8_t& lhs, MeshletFlags_ rhs) noexcept -> uint8_t&
	{
		return lhs = (1u << static_cast<uint8_t>(rhs));
	}*/

	struct GraphicsShaderDesc
	{
		GraphicsShaderDesc()
			:
			FileName(nullptr)
		,	EntryPoint(nullptr)
		{}

		const wchar_t* FileName;
		const wchar_t* EntryPoint;
	};

	struct ComputeShaderDesc
	{
		ComputeShaderDesc()
			:
			FileName(nullptr)
		,	EntryPoint(nullptr)
		{}

		const wchar_t* FileName;
		const wchar_t* EntryPoint;
	};

	struct RaytracingHitGroup
	{
		RaytracingHitGroup()
			:
			HitGroupName(nullptr)
		,	AnyHitShaderEntryPoint(nullptr)
		,	ClosestHitShaderEntryPoint(nullptr)
		,	IntersectionShaderEntryPoint(nullptr)
		,	Type()
		{}

		const wchar_t* HitGroupName;
		const wchar_t* AnyHitShaderEntryPoint;
		const wchar_t* ClosestHitShaderEntryPoint;
		const wchar_t* IntersectionShaderEntryPoint;
		RaytracingHitGroupType_ Type;
	};

	struct RaytracingShaderLibrary
	{
		const wchar_t* FileName;
		const wchar_t* RayGenShaderEntryPoint;
		std::vector<RaytracingHitGroup> HitGroups;
		std::vector<const wchar_t*> MissShaderEntryPoints;
		std::vector<const wchar_t*> CallableShaders;
	};

	struct ShaderResourceDesc
	{
		uint32_t ShaderRegister;
		uint32_t ShaderRegisterSpace;
		uint32_t NumOf32BitConstants;
		uint32_t Constant32BitSrc;
		uint32_t Constant32BitOffset;
		uint32_t NumOfShaderResources{1};
		uint32_t RootIndex;
		ShaderElementType_ ShaderElementType;
		ShaderVisibility_ ShaderVisibility;
		union
		{
			GPUResource* pResource;
			DescriptorTable* pDescriptorTable;
		};
	};

	struct SamplerDesc
	{
		uint32_t ShaderRegister;
		uint32_t ShaderRegisterSpace;
		ShaderVisibility_ ShaderVisibility;

		SamplerFilter_ Filter;
		SamplerMode_ Mode;
		float MipMapLevelOffset;
		float MipMapMinSampleLevel;
		float MipMapMaxSampleLevel;
		uint32_t MaxAnisotropy;	// Valid range 1 - 16 -> uint32_t cause padding anyways
		SamplerComparisonFunction_ ComparisonFunction;
		SamplerBorderColour_ BorderColour;

		static SamplerDesc InitAsDefaultTextureSampler(uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		static SamplerDesc InitAsDefaultShadowSampler(uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
	};

	struct DispatchSize
	{
		uint32_t ThreadCountX;
		uint32_t ThreadCountY;
		uint32_t ThreadCountZ;
	};

	struct PipelineDesc
	{
		std::vector<ShaderResourceDesc> vCBV;
		std::vector<ShaderResourceDesc> vSRV;
		std::vector<ShaderResourceDesc> vUAV;
		std::vector<ShaderResourceDesc> vDescriptorTables;

		std::vector<SamplerDesc> vStaticSamplers;	// We'll only use static samplers for now, TODO: support non-static samplers if necessary

		void AddConstantBufferView(GPUResource* resource, uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddShaderResourceView(GPUResource* resource, uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddUnorderedAccessView(GPUResource* resource, uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddDescriptorTable(DescriptorTable* table, uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddSampler(const SamplerDesc& desc);
		const ShaderResourceDesc CreateDesc(GPUResource* resource, ShaderElementType_ type, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility);
	};

	struct RasterizationPipelineDesc : public PipelineDesc
	{
		RasterizationPipelineType_ Type;
		BufferLayout InputVertexLayout;
		bool Wireframe;
		int DepthBias;

		GraphicsShaderDesc VertexShader;
		GraphicsShaderDesc HullShader;
		GraphicsShaderDesc DomainShader;
		GraphicsShaderDesc GeometryShader;
		GraphicsShaderDesc PixelShader;

		void SetType(RasterizationPipelineType_ type) { Type = type; }
		void SetInputLayout(const BufferLayout& layout) { InputVertexLayout = layout; }
		void SetWireFrame(bool wf) { Wireframe = wf; }
		void SetDepthBias(int bias) { DepthBias = bias; }

		void SetVertexShader(const wchar_t* filename, const wchar_t* entrypoint) { VertexShader.FileName = filename, VertexShader.EntryPoint = entrypoint; }
		void SetHullShader(const wchar_t* filename, const wchar_t* entrypoint) { HullShader.FileName = filename, HullShader.EntryPoint = entrypoint; }
		void SetDomainShader(const wchar_t* filename, const wchar_t* entrypoint) { DomainShader.FileName = filename, DomainShader.EntryPoint = entrypoint; }
		void SetGeometryShader(const wchar_t* filename, const wchar_t* entrypoint) { GeometryShader.FileName = filename, GeometryShader.EntryPoint = entrypoint; }
		void SetPixelShader(const wchar_t* filename, const wchar_t* entrypoint) { PixelShader.FileName = filename, PixelShader.EntryPoint = entrypoint; }
	};

	struct MeshletPipelineDesc
	{
		MeshletPipelineDesc()
			:
				DepthBias(PIPELINE_SETTINGS_DEFAULT_DEPTH_BIAS)
			,	AmplificationShaderDesc()
			,	MeshShaderDesc()
			,	PixelShaderDesc()
			,	DispatchSize()
			,	Layout()
			,	ShaderResourceIndex(0)
			,	InstanceCount(0)
			,	Flags(0)
			,	UseShaderRootSignatureDefinition(false)
			,	UseModelAttribsAsShaderDispatchDesc(false)
			,	UseDefaultPipelinePresets(false)
			,	IsWireFrame(false)
			,	Type()
		{
			
		}
		~MeshletPipelineDesc() {}

		void SetInputLayout(const BufferLayout& layout) { Layout = layout; }
		void SetDepthBias(int bias) { DepthBias = bias; }
		int32_t DepthBias;

		void SetMeshShader(const wchar_t* filename, const wchar_t* entrypoint) { MeshShaderDesc.FileName = filename, MeshShaderDesc.EntryPoint = entrypoint; }
		void SetPixelShader(const wchar_t* filename, const wchar_t* entrypoint) { PixelShaderDesc.FileName = filename, PixelShaderDesc.EntryPoint = entrypoint; }
		void SetAmplificationShader(const wchar_t* filename, const wchar_t* entrypoint) { AmplificationShaderDesc.FileName = filename, AmplificationShaderDesc.EntryPoint = entrypoint; }
		
		void SetType(MeshletPipelineType_ type) { Type = type; }	
		void SetDispatchCount(uint32_t x, uint32_t y, uint32_t z) { DispatchSize.ThreadCountX = x, DispatchSize.ThreadCountY = y, DispatchSize.ThreadCountZ = z; }


		void SetWireFrame(bool isWire);

		void AddConstantBuffer(GPUResource* buffer, uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddConstantBuffer(uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddShaderResource(GPUResource* buffer, uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddShaderResource(uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddUnorderedAccess(GPUResource* buffer, uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddUnorderedAccess(uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void Add32BitConstant(uint32_t numOf32BitConstants, uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddSampler(const SamplerDesc& desc);

		const ShaderResourceDesc CreateDesc(GPUResource* buffer, ShaderElementType_ type, uint32_t shaderRegister, uint32_t shaderRegisterSpace,
			ShaderVisibility_ shaderVisibility, uint32_t numOf32BitConstants = 0u);

		GraphicsShaderDesc AmplificationShaderDesc;
		GraphicsShaderDesc MeshShaderDesc;
		GraphicsShaderDesc PixelShaderDesc;
		DispatchSize	   DispatchSize;
		BufferLayout Layout;
		uint32_t ShaderResourceIndex;
		uint32_t InstanceCount;
		uint8_t Flags;
		
		MeshletPipelineType_ Type;

		std::vector<ShaderResourceDesc> vCBV;
		std::vector<ShaderResourceDesc> vSRV;
		std::vector<ShaderResourceDesc> vUAV;
		std::vector<ShaderResourceDesc> v32bitConstants;

		std::vector<SamplerDesc> vStaticSamplers;	// TODO: support non-static samplers if necessary
		bool UseDefaultPipelinePresets;
		bool IsWireFrame;
		bool UseShaderRootSignatureDefinition;
		bool UseModelAttribsAsShaderDispatchDesc;

	};

	struct ComputePipelineDesc : public PipelineDesc
	{
		DispatchSize DispatchSize;
		ComputeShaderDesc ComputeShader;

		void SetDispatchSize(uint32_t width, uint32_t height, uint32_t depth) { DispatchSize = { width, height, depth }; }
		void SetComputeShader(const wchar_t* filename, const wchar_t* entrypoint) { ComputeShader.FileName = filename, ComputeShader.EntryPoint = entrypoint; }
	};

	struct RaytracingPipelineDesc : public PipelineDesc
	{
		DispatchSize DispatchSize;
		RaytracingShaderLibrary RaytracingShaders;
		uint32_t MaxPayloadSize;
		uint32_t MaxAttributeSize;
		uint32_t MaxTraceRecursionDepth;
		uint32_t MaxCallableShaderRecursionDepth;

		void SetDispatchSize(uint32_t width, uint32_t height, uint32_t depth) { DispatchSize = { width, height, depth }; }
		void SetRaytracingLibrary(const wchar_t* libraryFilename, const wchar_t* raygenEntryPoint);
		void AddHitGroup(const wchar_t* hitGroupName, const wchar_t* anyHitEntryPoint, const wchar_t* closestHitEntryPoint, const wchar_t* intersectionEntryPoint, RaytracingHitGroupType_ type);
		void AddMissShader(const wchar_t* missEntryPoint);
		void AddCallableShader(const wchar_t* shaderEntryPoint);
		void SetConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize, uint32_t maxRecursionDepth, uint32_t maxCallableShaderRecursionDepth = 1u);
	};

	class Pipeline
	{
	protected:
#ifndef SKTBD_SHIP
		Pipeline(const std::wstring& debugName) : m_DebugName(debugName) {}
		const std::wstring& GetDebugName() const { return m_DebugName; }
#else
		Pipeline(const std::wstring& debugName) {}
		const std::wstring& GetDebugName() const { return L""; }
#endif
	public:
		virtual ~Pipeline() {}
		virtual void Release() = 0;
	protected:
#ifndef SKTBD_SHIP
		std::wstring m_DebugName;
#endif // !SKTBD_SHIP
	};

	class RasterizationPipeline : public Pipeline
	{
	protected:
		RasterizationPipeline(const std::wstring& debugName, const RasterizationPipelineDesc& desc) : Pipeline(debugName), m_Desc(desc) {}

	public:
		static RasterizationPipeline* Create(const std::wstring& debugName, const RasterizationPipelineDesc& desc);

		virtual void Bind(uint32_t geoID = 0u) = 0;
		virtual void Unbind() {}

		virtual const RasterizationPipelineDesc& GetDesc() const { return m_Desc; }

	protected:
		RasterizationPipelineDesc m_Desc;
	};

	class ComputePipeline : public Pipeline
	{
	protected:
		ComputePipeline(const std::wstring& debugName, const ComputePipelineDesc& desc) : Pipeline(debugName), m_Desc(desc) {}

	public:
		static ComputePipeline* Create(const std::wstring& debugName, const ComputePipelineDesc& desc);

		virtual void Bind() = 0;
		virtual void Unbind() {}

		virtual const ComputePipelineDesc& GetDesc() const { return m_Desc; }

	protected:
		ComputePipelineDesc m_Desc;
	};

	class MeshletPipeline : public Pipeline
	{
	protected:
		MeshletPipeline(const std::wstring& debugName, const MeshletPipelineDesc& desc) : Pipeline(debugName), m_Desc(desc) {}
	public:
		static MeshletPipeline* Create(const std::wstring& debugName, const MeshletPipelineDesc& desc);

		virtual ~MeshletPipeline() {}

		virtual void SetInputLayout(const BufferLayout& layout) = 0;
		virtual void SetDefaultInputLayout() = 0;
		virtual void SetResource(GPUResource* buffer, uint32_t shaderRegister, uint32_t registerSpace = 0) = 0;

		virtual void BindMeshPipeline(MeshletModel* model=nullptr) = 0;
		virtual void Unbind() {};

		virtual const MeshletPipelineDesc& GetDesc() const = 0;

	protected:
		virtual void LoadMeshShader(const wchar_t* filename, void** blob) = 0;

		MeshletPipelineDesc m_Desc;
	};

	class RaytracingPipeline : public Pipeline
	{
	protected:
		RaytracingPipeline(const std::wstring& debugName, const RaytracingPipelineDesc& desc) : Pipeline(debugName), m_Desc(desc) {}

	public:
		static RaytracingPipeline* Create(const std::wstring& debugName, const RaytracingPipelineDesc& desc);

		virtual void Bind() = 0;
		virtual void Unbind() {}

		virtual void ResizeDispatchAndOutputUAV(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth = 1u) = 0;

		const RaytracingPipelineDesc& GetDesc() const { return m_Desc; }
		UnorderedAccessBuffer* GetOutputUAV() const { return m_RaytracingOutput.get(); }

	protected:
		RaytracingPipelineDesc m_Desc;
		std::unique_ptr<Skateboard::UnorderedAccessBuffer> m_RaytracingOutput;
	};
}

