#pragma once

#ifdef SKTBD_PLATFORM_WINDOWS
	typedef enum D3D12_RAYTRACING_GEOMETRY_TYPE D3D12_RAYTRACING_GEOMETRY_TYPE;
	typedef enum D3D12_HIT_GROUP_TYPE D3D12_HIT_GROUP_TYPE;
	typedef enum DXGI_FORMAT DXGI_FORMAT;
	typedef enum D3D12_RESOURCE_STATES D3D12_RESOURCE_STATES;
	typedef enum D3D12_SHADER_VISIBILITY D3D12_SHADER_VISIBILITY;
	typedef enum D3D12_FILTER D3D12_FILTER;
	typedef enum D3D12_TEXTURE_ADDRESS_MODE D3D12_TEXTURE_ADDRESS_MODE;
	typedef enum D3D12_COMPARISON_FUNC D3D12_COMPARISON_FUNC;
	typedef enum D3D12_STATIC_BORDER_COLOR D3D12_STATIC_BORDER_COLOR;
	typedef enum D3D12_DESCRIPTOR_RANGE_TYPE D3D12_DESCRIPTOR_RANGE_TYPE;
#endif

namespace Skateboard
{
	/////////////////////////////////////////////////////////////////
	// GEOMTRY FORMATS
	/////////////////////////////////////////////////////////////////

	enum GeometryType_ : uint8_t	// Uint32 would overflow in raytracing -> only the 8 bottoms bits are used
	{
		GeometryType_None_NoTrace = 0,
		GeometryType_Triangles = (1 << 0),
		GeometryType_Procedural_Analytics = (1 << 1),
		GeometryType_Procedural_Volumetrics = (1 << 2),
		GeometryType_Procedural_SDFs = (1 << 3),

		GeometryType_Procedural_Mask = GeometryType_Procedural_Analytics & GeometryType_Procedural_Volumetrics & GeometryType_Procedural_SDFs
	};

	/////////////////////////////////////////////////////////////////
	// PIPELINE FORMATS
	/////////////////////////////////////////////////////////////////

	enum RasterizationPipelineType_
	{
		RasterizationPipelineType_DepthOnly = 0,		// Possible: Vertex
		RasterizationPipelineType_Default,				// Possible: Vertex, Geo, Pixel
		RasterizationPipelineType_Tessellation_Tri,		// Possible: Vertex, Hull, Domain, Geo, Pixel
		RasterizationPipelineType_Tessellation_Quad		// Possible: Vertex, Hull, Domain, Geo, Pixel
	};

	enum RaytracingHitGroupType_
	{
		RaytracingHitGroupType_Triangles = 0,
		RaytracingHitGroupType_Procedural,
		RaytracingHitGroupType_Count
	};

	enum MeshletPipelineType_
	{
		MeshletPipelineType_DepthOnly = 0,
		MeshletPipelineType_Default,
		MeshletPipelineType_Cull,
		MeshletPipelineType_Particle,
		MeshletPipelineType_DynamicLOD,
		MeshletPipelineType_Custom
	};

	/////////////////////////////////////////////////////////////////
	// SHADER FORMATS
	/////////////////////////////////////////////////////////////////

	enum class ShaderDataType_
	{
		None = 0, Bool,
		Int, Int2, Int3, Int4,
		Uint, Uint2, Uint3, Uint4,
		Float, Float2, Float3, Float4,
	};

	// TODO: Do's >> Need to bind to specific stages for better performance!
	enum ShaderVisibility_ : uint32_t
	{
		ShaderVisibility_VertexShader = (1 << 0),
		ShaderVisibility_HullShader = (1 << 1),
		ShaderVisibility_DomainShader = (1 << 2),
		ShaderVisibility_GeometryShader = (1 << 3),
		ShaderVisibility_PixelShader = (1 << 4),
		ShaderVisibility_All = 0xFFFFFFFFui32,

		RaytracingShaderVisibility_Global = (1 << 5),
		RaytracingShaderVisibility_Local_RayGen = (1 << 6),
		RaytracingShaderVisibility_Local_Hitgroup = (1 << 7),
		RaytracingShaderVisibility_Local_Miss = (1 << 8),
		RaytracingShaderVisibility_Local_Callable = (1 << 9),
		RaytracingShaderVisibility_Mask = (RaytracingShaderVisibility_Global & RaytracingShaderVisibility_Local_RayGen &
											RaytracingShaderVisibility_Local_Hitgroup & RaytracingShaderVisibility_Local_Miss & RaytracingShaderVisibility_Local_Callable)
	};

	enum ShaderElementType_
	{
		ShaderElementType_RootConstant = 0,
		ShaderElementType_ConstantBufferView,
		ShaderElementType_ShaderResourceView,
		ShaderElementType_UnorderedAccessView,
		ShaderElementType_DescriptorTable
	};

	enum ShaderDescriptorTableType_
	{
		ShaderDescriptorType_SRV = 0,
		ShaderDescriptorType_UAV,
		ShaderDescriptorType_CBV,
		ShaderDescriptorType_Sampler
	};

	/////////////////////////////////////////////////////////////////
	// BUFFER FORMATS
	/////////////////////////////////////////////////////////////////

	enum BufferAccessFlag_ : uint32_t
	{
		BufferAccessFlag_GpuOnly = 0,
		BufferAccessFlag_CpuWriteable,
		BufferAccessFlag_CpuWriteable_Aligned
	};

	// Pretty much stolen from the DXGI formats
	enum BufferFormat_ : uint32_t
	{
		BufferFormat_UNKNOWN = 0,
		BufferFormat_R32G32B32A32_TYPELESS = 1,
		BufferFormat_R32G32B32A32_FLOAT = 2,
		BufferFormat_R32G32B32A32_UINT = 3,
		BufferFormat_R32G32B32A32_SINT = 4,
		BufferFormat_R32G32B32_TYPELESS = 5,
		BufferFormat_R32G32B32_FLOAT = 6,
		BufferFormat_R32G32B32_UINT = 7,
		BufferFormat_R32G32B32_SINT = 8,
		BufferFormat_R16G16B16A16_TYPELESS = 9,
		BufferFormat_R16G16B16A16_FLOAT = 10,
		BufferFormat_R16G16B16A16_UNORM = 11,
		BufferFormat_R16G16B16A16_UINT = 12,
		BufferFormat_R16G16B16A16_SNORM = 13,
		BufferFormat_R16G16B16A16_SINT = 14,
		BufferFormat_R32G32_TYPELESS = 15,
		BufferFormat_R32G32_FLOAT = 16,
		BufferFormat_R32G32_UINT = 17,
		BufferFormat_R32G32_SINT = 18,
		BufferFormat_R32G8X24_TYPELESS = 19,
		BufferFormat_D32_FLOAT_S8X24_UINT = 20,
		BufferFormat_R32_FLOAT_X8X24_TYPELESS = 21,
		BufferFormat_X32_TYPELESS_G8X24_UINT = 22,
		BufferFormat_R10G10B10A2_TYPELESS = 23,
		BufferFormat_R10G10B10A2_UNORM = 24,
		BufferFormat_R10G10B10A2_UINT = 25,
		BufferFormat_R11G11B10_FLOAT = 26,
		BufferFormat_R8G8B8A8_TYPELESS = 27,
		BufferFormat_R8G8B8A8_UNORM = 28,
		BufferFormat_R8G8B8A8_UNORM_SRGB = 29,
		BufferFormat_R8G8B8A8_UINT = 30,
		BufferFormat_R8G8B8A8_SNORM = 31,
		BufferFormat_R8G8B8A8_SINT = 32,
		BufferFormat_R16G16_TYPELESS = 33,
		BufferFormat_R16G16_FLOAT = 34,
		BufferFormat_R16G16_UNORM = 35,
		BufferFormat_R16G16_UINT = 36,
		BufferFormat_R16G16_SNORM = 37,
		BufferFormat_R16G16_SINT = 38,
		BufferFormat_R32_TYPELESS = 39,
		BufferFormat_D32_FLOAT = 40,
		BufferFormat_R32_FLOAT = 41,
		BufferFormat_R32_UINT = 42,
		BufferFormat_R32_SINT = 43,
		BufferFormat_R24G8_TYPELESS = 44,
		BufferFormat_D24_UNORM_S8_UINT = 45,
		BufferFormat_R24_UNORM_X8_TYPELESS = 46,
		BufferFormat_X24_TYPELESS_G8_UINT = 47,
		BufferFormat_R8G8_TYPELESS = 48,
		BufferFormat_R8G8_UNORM = 49,
		BufferFormat_R8G8_UINT = 50,
		BufferFormat_R8G8_SNORM = 51,
		BufferFormat_R8G8_SINT = 52,
		BufferFormat_R16_TYPELESS = 53,
		BufferFormat_R16_FLOAT = 54,
		BufferFormat_D16_UNORM = 55,
		BufferFormat_R16_UNORM = 56,
		BufferFormat_R16_UINT = 57,
		BufferFormat_R16_SNORM = 58,
		BufferFormat_R16_SINT = 59,
		BufferFormat_R8_TYPELESS = 60,
		BufferFormat_R8_UNORM = 61,
		BufferFormat_R8_UINT = 62,
		BufferFormat_R8_SNORM = 63,
		BufferFormat_R8_SINT = 64,
		BufferFormat_A8_UNORM = 65,
		BufferFormat_R1_UNORM = 66,
		BufferFormat_R9G9B9E5_SHAREDEXP = 67,
		BufferFormat_R8G8_B8G8_UNORM = 68,
		BufferFormat_G8R8_G8B8_UNORM = 69,
		BufferFormat_BC1_TYPELESS = 70,
		BufferFormat_BC1_UNORM = 71,
		BufferFormat_BC1_UNORM_SRGB = 72,
		BufferFormat_BC2_TYPELESS = 73,
		BufferFormat_BC2_UNORM = 74,
		BufferFormat_BC2_UNORM_SRGB = 75,
		BufferFormat_BC3_TYPELESS = 76,
		BufferFormat_BC3_UNORM = 77,
		BufferFormat_BC3_UNORM_SRGB = 78,
		BufferFormat_BC4_TYPELESS = 79,
		BufferFormat_BC4_UNORM = 80,
		BufferFormat_BC4_SNORM = 81,
		BufferFormat_BC5_TYPELESS = 82,
		BufferFormat_BC5_UNORM = 83,
		BufferFormat_BC5_SNORM = 84,
		BufferFormat_B5G6R5_UNORM = 85,
		BufferFormat_B5G5R5A1_UNORM = 86,
		BufferFormat_B8G8R8A8_UNORM = 87,
		BufferFormat_B8G8R8X8_UNORM = 88,
		BufferFormat_R10G10B10_XR_BIAS_A2_UNORM = 89,
		BufferFormat_B8G8R8A8_TYPELESS = 90,
		BufferFormat_B8G8R8A8_UNORM_SRGB = 91,
		BufferFormat_B8G8R8X8_TYPELESS = 92,
		BufferFormat_B8G8R8X8_UNORM_SRGB = 93,
		BufferFormat_BC6H_TYPELESS = 94,
		BufferFormat_BC6H_UF16 = 95,
		BufferFormat_BC6H_SF16 = 96,
		BufferFormat_BC7_TYPELESS = 97,
		BufferFormat_BC7_UNORM = 98,
		BufferFormat_BC7_UNORM_SRGB = 99,
		BufferFormat_AYUV = 100,
		BufferFormat_Y410 = 101,
		BufferFormat_Y416 = 102,
		BufferFormat_NV12 = 103,
		BufferFormat_P010 = 104,
		BufferFormat_P016 = 105,
		BufferFormat_420_OPAQUE = 106,
		BufferFormat_YUY2 = 107,
		BufferFormat_Y210 = 108,
		BufferFormat_Y216 = 109,
		BufferFormat_NV11 = 110,
		BufferFormat_AI44 = 111,
		BufferFormat_IA44 = 112,
		BufferFormat_P8 = 113,
		BufferFormat_A8P8 = 114,
		BufferFormat_B4G4R4A4_UNORM = 115,

		BufferFormat_P208 = 130,
		BufferFormat_V208 = 131,
		BufferFormat_V408 = 132,

		BufferFormat_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE = 189,
		BufferFormat_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,

		BufferFormat_DEFAULT_BACKBUFFER = BufferFormat_R8G8B8A8_UNORM,
		BufferFormat_DEFAULT_DEPTHSTENCIL = BufferFormat_D24_UNORM_S8_UINT,
		BufferFormat_FORCE_UINT = 0xffffffff
	};

	enum GPUResourceType_ : uint32_t
	{
		GPUResourceType_DefaultBuffer = 0,
		GPUResourceType_UploadBuffer,
		GPUResourceType_UnorderedAccessBuffer,
		GPUResourceType_FrameBuffer,
		GPUResourceType_Texture2D,
		GPUResourceType_Texture3D,
		GPUResourceType_BottomLevelAccelerationStructure,
		GPUResourceType_TopLevelAccelerationStructure,
		GPUResourceType_ByteAddress
	};

	enum TextureType_ : uint32_t
	{
		TextureType_Texture1D = 0,
		TextureType_Texture2D,
		TextureType_Texture3D,
	};

	/////////////////////////////////////////////////////////////////
	// SAMPLER FORMATS
	/////////////////////////////////////////////////////////////////

	enum SamplerFilter_ : uint32_t
	{
		SamplerFilter_Min_Mag_Mip_Point = 0,
		SamplerFilter_Min_Mag_Point_Mip_Linear,
		SamplerFilter_Min_Point_Mag_Linear_Mip_Point,
		SamplerFilter_Min_Point_Mag_Mip_Linear,
		SamplerFilter_Min_Linear_Mag_Mip_Point,
		SamplerFilter_Min_Linear_Mag_Point_Mip_Linear,
		SamplerFilter_Min_Mag_Linear_Mip_Point,
		SamplerFilter_Min_Mag_Mip_Linear,
		SamplerFilter_Anisotropic,
		SamplerFilter_Comaprison_Min_Mag_Mip_Point,
		SamplerFilter_Comparions_Min_Mag_Point_Mip_Linear,
		SamplerFilter_Comparison_Min_Point_Mag_Linear_Mip_Point,
		SamplerFilter_Comparison_Min_Point_Mag_Mip_Linear,
		SamplerFilter_Comaprison_Min_Linear_Mag_Mip_Point,
		SamplerFilter_Comparison_Min_Linear_Mag_Point_Mip_Linear,
		SamplerFilter_Comparison_Min_Mag_Linear_Mip_Point,
		SamplerFilter_Comparison_Min_Mag_Mip_Linear,
		SamplerFilter_Comparison_Anisotropic,
		SamplerFilter_Minimum_Min_Mag_Mip_Point,
		SamplerFilter_Minimum_Min_Mag_Point_Mip_Linear,
		SamplerFilter_Minimum_Min_Point_Mag_Linear_Mip_Point,
		SamplerFilter_Minimum_Min_Point_Mag_Mip_Linear,
		SamplerFilter_Minimum_Min_Point_Mag_Mip_Point,
		SamplerFilter_Minimum_Min_Linear_Mag_Point_Mip_Linear,
		SamplerFilter_Minimum_Min_Mag_Linear_Mip_Point,
		SamplerFilter_Minimum_Min_Mag_Mip_Linear,
		SamplerFilter_Minimum_Anisotropic,
		SamplerFilter_Maximum_Min_Mag_Mip_Point,
		SamplerFilter_Maximum_Min_Mag_Point_Mip_Linear,
		SamplerFilter_Maximum_Min_Point_Mag_Linear_Mip_Point,
		SamplerFilter_Maximum_Min_Point_Mag_Mip_Linear,
		SamplerFilter_Maximum_Min_Point_Mag_Mip_Point,
		SamplerFilter_Maximum_Min_Linear_Mag_Point_Mip_Linear,
		SamplerFilter_Maximum_Min_Mag_Linear_Mip_Point,
		SamplerFilter_Maximum_Min_Mag_Mip_Linear,
		SamplerFilter_Maximum_Anisotropic
	};

	enum SamplerMode_ : uint32_t
	{
		SamplerMode_Warp = 0,
		SamplerMode_Mirror,
		SamplerMode_Clamp,
		SamplerMode_Border,
		SamplerMode_Mirror_Once
	};

	enum SamplerComparisonFunction_ : uint32_t
	{
		SamplerComparisonFunction_Never = 0,
		SamplerComparisonFunction_Less,
		SamplerComparisonFunction_Equal,
		SamplerComparisonFunction_Less_Equal,
		SamplerComparisonFunction_Greater,
		SamplerComparisonFunction_Not_Equal,
		SamplerComparisonFunction_Greater_Equal,
		SamplerComparisonFunction_Always
	};

	enum SamplerBorderColour_ : uint32_t
	{
		SamplerBorderColour_TransparentBlack = 0,
		SamplerBorderColour_White,
		SamplerBorderColour_Black,
	};

	/////////////////////////////////////////////////////////////////
	// HELPER FUNCTIONS
	/////////////////////////////////////////////////////////////////

	uint32_t ShaderDataTypeSizeInBytes(ShaderDataType_ type);

#ifdef SKTBD_PLATFORM_WINDOWS
	D3D12_RAYTRACING_GEOMETRY_TYPE GeometryTypeToD3D(GeometryType_ type);
	D3D12_HIT_GROUP_TYPE RaytracingHitGroupTypeToD3D(RaytracingHitGroupType_ type);
	DXGI_FORMAT ShaderDataTypeToD3D(ShaderDataType_ type);
	D3D12_SHADER_VISIBILITY ShaderVisibilityToD3D(ShaderVisibility_ v);
	D3D12_DESCRIPTOR_RANGE_TYPE ShaderDescriptorTableTypeToD3D(ShaderDescriptorTableType_ type);
	D3D12_FILTER SamplerFilterToD3D(SamplerFilter_ filter);
	D3D12_TEXTURE_ADDRESS_MODE SamplerModeToD3D(SamplerMode_ mode);
	D3D12_COMPARISON_FUNC SamplerComparisonFunctionToD3D(SamplerComparisonFunction_ function);
	D3D12_STATIC_BORDER_COLOR SamplerBorderColourToD3D(SamplerBorderColour_ colour);
	DXGI_FORMAT DepthStencilToSRVD3D(BufferFormat_ format);
	DXGI_FORMAT BufferFormatToD3D(BufferFormat_ format);
	BufferFormat_ D3DBufferFormatToSkateboard(DXGI_FORMAT format);
	D3D12_RESOURCE_STATES ResourceTypeToStateD3D(GPUResourceType_ type);
#endif
}