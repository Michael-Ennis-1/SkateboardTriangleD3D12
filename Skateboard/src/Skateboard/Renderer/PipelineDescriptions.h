#pragma once
#include "Skateboard/Renderer/Buffer.h"

namespace Skateboard
{
	enum PipelineType_
	{
		PipelineType_DepthOnly = 0,						// Possible: Vertex
		PipelineType_Rasterization,						// Possible: Vertex, Geo, Pixel
		PipelineType_Rasterization_Tessellation_Tri,	// Possible: Vertex, Hull, Domain, Geo, Pixel
		PipelineType_Rasterization_Tessellation_Quad,	// Possible: Vertex, Hull, Domain, Geo, Pixel
		PipelineType_Compute,
		PipelineType_MeshShading,			// Mesh, Pixel
		PipelineType_Raytracing				// RayGen, Intersection, AnyHit, ClosestHit, Miss
	};

	//struct GraphicsShaderDesc
	//{
	//	const wchar_t* FileName;
	//	const wchar_t* EntryPoint;
	//};

	//struct ComputeShaderDesc
	//{
	//	const wchar_t* FileName;
	//	const wchar_t* EntryPoint;
	//	uint3 DispatchSize;
	//};

	//struct RaytracingShaderLibrary
	//{
	//	const wchar_t* FileName;
	//	const wchar_t* RayGenShaderEntryPoint;
	//	const wchar_t* IntersectionShaderEntryPoint;
	//	const wchar_t* AnyHitShaderEntryPoint;
	//	const wchar_t* ClosestHitShaderEntryPoint;
	//	const wchar_t* MissShaderEntryPoint;
	//	uint3 DispatchSize;
	//};

	//struct ShaderResourceDesc
	//{
	//	uint32_t ShaderRegister;
	//	uint32_t ShaderRegisterSpace;
	//	uint32_t NumOfShaderResources{1u};
	//	ShaderElementType_ ShaderElementType;
	//	ShaderVisibility_ ShaderVisibility;
	//	GPUResource* pBuffer;
	//	uint32_t NumOf32BitConstants{ 0u };
	//	uint32_t Constant32BitOffset{ 0u };
	//	uint8_t* Constant32BitSrc{ nullptr };
	//};

	//struct SamplerDesc
	//{
	//	uint32_t ShaderRegister;
	//	uint32_t ShaderRegisterSpace;
	//	ShaderVisibility_ ShaderVisibility;

	//	SamplerFilter_ Filter;
	//	SamplerMode_ Mode;
	//	float MipMapLevelOffset;
	//	float MipMapMinSampleLevel;
	//	float MipMapMaxSampleLevel;
	//	uint32_t MaxAnisotropy;	// Valid range 1 - 16 -> uint32_t cause padding anyways
	//	SamplerComparisonFunction_ ComparisonFunction;
	//	SamplerBorderColour_ BorderColour;

	//	static SamplerDesc InitAsDefaultTextureSampler(uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
	//	static SamplerDesc InitAsDefaultShadowSampler(uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
	//};

	//struct DispatchSize
	//{
	//	uint32_t ThreadCountX;
	//	uint32_t ThreadCountY;
	//	uint32_t ThreadCountZ;
	//};
}