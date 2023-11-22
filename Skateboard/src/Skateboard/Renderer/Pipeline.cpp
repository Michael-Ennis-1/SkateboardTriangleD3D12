#include "sktbdpch.h"
#include "Pipeline.h"
#include "Renderer.h"

#include "Platform/DirectX12/D3DPipeline.h"
#include "Platform/DirectX12/MeshletPipeline/D3DMeshletPipeline.h"

namespace Skateboard
{
	SamplerDesc SamplerDesc::InitAsDefaultTextureSampler(uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		SamplerDesc desc = {};
		desc.ShaderRegister = shaderRegister;
		desc.ShaderRegisterSpace = shaderRegisterSpace;
		desc.ShaderVisibility = shaderVisibility;
		desc.Filter = SamplerFilter_Min_Mag_Mip_Linear;
		desc.Mode = SamplerMode_Warp;
		desc.MipMapLevelOffset = 0.f;
		desc.MipMapMinSampleLevel = 0.f;
		desc.MipMapMaxSampleLevel = D3D12_FLOAT32_MAX;
		desc.MaxAnisotropy = 16u;
		desc.ComparisonFunction = SamplerComparisonFunction_Less_Equal;
		desc.BorderColour = SamplerBorderColour_White;
		return desc;
	}

	SamplerDesc SamplerDesc::InitAsDefaultShadowSampler(uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		SamplerDesc desc = {};
		desc.ShaderRegister = shaderRegister;
		desc.ShaderRegisterSpace = shaderRegisterSpace;
		desc.ShaderVisibility = shaderVisibility;
		desc.Filter = SamplerFilter_Min_Mag_Linear_Mip_Point;
		desc.Mode = SamplerMode_Border;
		desc.MipMapLevelOffset = 0.f;
		desc.MipMapMinSampleLevel = 0.f;
		desc.MipMapMaxSampleLevel = D3D12_FLOAT32_MAX;
		desc.MaxAnisotropy = 16u;
		desc.ComparisonFunction = SamplerComparisonFunction_Less_Equal;
		desc.BorderColour = SamplerBorderColour_Black;
		return desc;
	}

	void PipelineDesc::AddConstantBufferView(GPUResource* resource, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		vCBV.emplace_back(CreateDesc(resource, ShaderElementType_ConstantBufferView, shaderRegister, shaderRegisterSpace, shaderVisibility));
	}

	void PipelineDesc::AddShaderResourceView(GPUResource* resource, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		vSRV.emplace_back(CreateDesc(resource, ShaderElementType_ShaderResourceView, shaderRegister, shaderRegisterSpace, shaderVisibility));
	}

	void PipelineDesc::AddUnorderedAccessView(GPUResource* resource, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		vUAV.emplace_back(CreateDesc(resource, ShaderElementType_UnorderedAccessView, shaderRegister, shaderRegisterSpace, shaderVisibility));
	}

	void PipelineDesc::AddDescriptorTable(DescriptorTable* table, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		ShaderResourceDesc desc = {};
		desc.pDescriptorTable = table;
		desc.ShaderElementType = ShaderElementType_DescriptorTable;
		desc.ShaderRegister = shaderRegister;
		desc.ShaderRegisterSpace = shaderRegisterSpace;
		desc.ShaderVisibility = shaderVisibility;
		vDescriptorTables.emplace_back(std::move(desc));
	}

	void PipelineDesc::AddSampler(const SamplerDesc& desc)
	{
		vStaticSamplers.emplace_back(desc);
	}

	void  MeshletPipelineDesc::SetWireFrame(bool isWire)
	{
		IsWireFrame = isWire;
	}

	void MeshletPipelineDesc::AddConstantBuffer(GPUResource* buffer, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		auto desc = CreateDesc(buffer, ShaderElementType_ConstantBufferView, shaderRegister, shaderRegisterSpace, shaderVisibility);
		desc.RootIndex = static_cast<uint32_t>(vCBV.size() + v32bitConstants.size() + vSRV.size() + vUAV.size());
		vCBV.emplace_back(desc);
	}

	void MeshletPipelineDesc::AddShaderResource(GPUResource* buffer, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		auto desc = CreateDesc(buffer, ShaderElementType_ShaderResourceView, shaderRegister, shaderRegisterSpace, shaderVisibility);
		desc.RootIndex = static_cast<uint32_t>(vCBV.size() + v32bitConstants.size() + vSRV.size() + vUAV.size());
		vSRV.emplace_back(desc);
	}

	void MeshletPipelineDesc::AddUnorderedAccess(GPUResource* buffer, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		auto desc = CreateDesc(buffer, ShaderElementType_UnorderedAccessView, shaderRegister, shaderRegisterSpace, shaderVisibility);
		desc.RootIndex = static_cast<uint32_t>(vCBV.size() + v32bitConstants.size() + vSRV.size() + vUAV.size());
		vUAV.emplace_back(desc);
	}

	void MeshletPipelineDesc::Add32BitConstant(uint32_t numOf32BitConstants, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		auto desc = CreateDesc(nullptr, ShaderElementType_RootConstant, shaderRegister, shaderRegisterSpace, shaderVisibility, numOf32BitConstants);
		desc.RootIndex = static_cast<uint32_t>(vCBV.size() + v32bitConstants.size() + vSRV.size() + vUAV.size());
		v32bitConstants.push_back(desc);
	}

	void MeshletPipelineDesc::AddConstantBuffer(uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		auto desc = CreateDesc(nullptr, ShaderElementType_ConstantBufferView, shaderRegister, shaderRegisterSpace, shaderVisibility);
		desc.RootIndex = static_cast<uint32_t>(vCBV.size() + v32bitConstants.size() + vSRV.size() + vUAV.size());
		vCBV.emplace_back(desc);
	}

	void MeshletPipelineDesc::AddShaderResource(uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		auto desc = CreateDesc(nullptr, ShaderElementType_ShaderResourceView, shaderRegister, shaderRegisterSpace, shaderVisibility);
		desc.RootIndex = static_cast<uint32_t>(vCBV.size() + v32bitConstants.size() + vSRV.size() + vUAV.size());
		vSRV.emplace_back(desc);
	}

	void MeshletPipelineDesc::AddUnorderedAccess(uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		auto desc = CreateDesc(nullptr, ShaderElementType_UnorderedAccessView, shaderRegister, shaderRegisterSpace, shaderVisibility);
		desc.RootIndex = static_cast<uint32_t>(vCBV.size() + v32bitConstants.size() + vSRV.size() + vUAV.size());
		vUAV.emplace_back(desc);
	}

	void MeshletPipelineDesc::AddSampler(const SamplerDesc& desc)
	{
		vStaticSamplers.emplace_back(desc);
	}

	const ShaderResourceDesc MeshletPipelineDesc::CreateDesc(GPUResource* buffer, ShaderElementType_ type, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility,
		uint32_t numOf32BitConstants)
	{
		ShaderResourceDesc desc = {};
		desc.pResource = buffer;
		desc.ShaderElementType = type;
		desc.ShaderRegister = shaderRegister;
		desc.ShaderRegisterSpace = shaderRegisterSpace;
		desc.ShaderVisibility = shaderVisibility;
		desc.NumOf32BitConstants = numOf32BitConstants;
		return desc;
	}

	const ShaderResourceDesc PipelineDesc::CreateDesc(GPUResource* resource, ShaderElementType_ type, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		ShaderResourceDesc desc = {};
		desc.pResource = resource;
		desc.ShaderElementType = type;
		desc.ShaderRegister = shaderRegister;
		desc.ShaderRegisterSpace = shaderRegisterSpace;
		desc.ShaderVisibility = shaderVisibility;
		desc.NumOfShaderResources = 1u;
		desc.NumOf32BitConstants = 0u;
		return desc;
	}

	void RaytracingPipelineDesc::SetRaytracingLibrary(const wchar_t* libraryFilename, const wchar_t* raygenEntryPoint)
	{
		RaytracingShaders.FileName = libraryFilename;
		RaytracingShaders.RayGenShaderEntryPoint = raygenEntryPoint;
	}

	void RaytracingPipelineDesc::AddHitGroup(const wchar_t* hitGroupName, const wchar_t* anyHitEntryPoint, const wchar_t* closestHitEntryPoint, const wchar_t* intersectionEntryPoint, RaytracingHitGroupType_ type)
	{
		RaytracingHitGroup group = {};
		group.HitGroupName = hitGroupName;
		group.AnyHitShaderEntryPoint = anyHitEntryPoint;
		group.ClosestHitShaderEntryPoint = closestHitEntryPoint;
		group.IntersectionShaderEntryPoint = intersectionEntryPoint;
		group.Type = type;
		RaytracingShaders.HitGroups.emplace_back(std::move(group));
	}

	void RaytracingPipelineDesc::AddMissShader(const wchar_t* missEntryPoint)
	{
		RaytracingShaders.MissShaderEntryPoints.push_back(missEntryPoint);
	}
	void RaytracingPipelineDesc::AddCallableShader(const wchar_t* shaderEntryPoint)
	{
		RaytracingShaders.CallableShaders.push_back(shaderEntryPoint);
	}
	void RaytracingPipelineDesc::SetConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize, uint32_t maxRecursionDepth, uint32_t maxCallableShaderRecursionDepth)
	{
		MaxPayloadSize = maxPayloadSize;
		MaxAttributeSize = maxAttributeSize;
		MaxTraceRecursionDepth = maxRecursionDepth;
		MaxCallableShaderRecursionDepth = maxCallableShaderRecursionDepth;
	}

	RasterizationPipeline* RasterizationPipeline::Create(const std::wstring& debugName, const RasterizationPipelineDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DRasterizationPipeline(debugName, desc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create a pipeline, the input API does not exist!");
			return nullptr;
		}
	}

	MeshletPipeline* MeshletPipeline::Create(const std::wstring& debugName, const MeshletPipelineDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DMeshletPipeline(debugName, desc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create a pipeline, the input API does not exist!");
			return nullptr;
		}
	}

	ComputePipeline* ComputePipeline::Create(const std::wstring& debugName, const ComputePipelineDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DComputePipeline(debugName, desc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create a pipeline, the input API does not exist!");
			return nullptr;
		}
	}

	RaytracingPipeline* RaytracingPipeline::Create(const std::wstring& debugName, const RaytracingPipelineDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DRaytracingPipeline(debugName, desc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create a pipeline, the input API does not exist!");
			return nullptr;
		}
	}
}
