#include <sktbdpch.h>
#include "InternalFormats.h"

#ifdef SKTBD_PLATFORM_WINDOWS
	#include "Platform/DirectX12/D3D.h"
#endif

// Note: These switches may seem rather unefficient, but modern compiler will try to optimise them into nice lookup tables for O(1) conversion
// Read more here:
//		https://parses.wordpress.com/2017/02/21/c-switch-as-a-lookup-table/
//		https://stackoverflow.com/questions/931890/what-is-more-efficient-a-switch-case-or-an-stdmap

namespace Skateboard
{
	uint32_t ShaderDataTypeSizeInBytes(ShaderDataType_ type)

	{
		switch (type)
		{
		case ShaderDataType_::None:			return NULL;
		case ShaderDataType_::Bool:			return 1 * sizeof(bool);
		case ShaderDataType_::Int:			return 1 * sizeof(int32_t);
		case ShaderDataType_::Int2:			return 2 * sizeof(int32_t);
		case ShaderDataType_::Int3:			return 3 * sizeof(int32_t);
		case ShaderDataType_::Int4:			return 4 * sizeof(int32_t);
		case ShaderDataType_::Uint:			return 1 * sizeof(uint32_t);
		case ShaderDataType_::Uint2:		return 2 * sizeof(uint32_t);
		case ShaderDataType_::Uint3:		return 3 * sizeof(uint32_t);
		case ShaderDataType_::Uint4:		return 4 * sizeof(uint32_t);
		case ShaderDataType_::Float:		return 1 * sizeof(float);
		case ShaderDataType_::Float2:		return 2 * sizeof(float);
		case ShaderDataType_::Float3:		return 3 * sizeof(float);
		case ShaderDataType_::Float4:		return 4 * sizeof(float);
		default:
			SKTBD_CORE_ASSERT(false, "Impossible shader data type!");
			return NULL;
		}
	}

	D3D12_COMPARISON_FUNC SamplerComparisonFunctionToD3D(SamplerComparisonFunction_ function)
	{
		switch (function)
		{
		case SamplerComparisonFunction_Never:			return D3D12_COMPARISON_FUNC_NEVER;
		case SamplerComparisonFunction_Less:			return D3D12_COMPARISON_FUNC_LESS;
		case SamplerComparisonFunction_Equal:			return D3D12_COMPARISON_FUNC_EQUAL;
		case SamplerComparisonFunction_Less_Equal:		return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case SamplerComparisonFunction_Greater:			return D3D12_COMPARISON_FUNC_GREATER;
		case SamplerComparisonFunction_Not_Equal:		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case SamplerComparisonFunction_Greater_Equal:	return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case SamplerComparisonFunction_Always:			return D3D12_COMPARISON_FUNC_ALWAYS;
		default:										return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		}
	}

#ifdef SKTBD_PLATFORM_WINDOWS

	D3D12_HIT_GROUP_TYPE RaytracingHitGroupTypeToD3D(RaytracingHitGroupType_ type)
	{
		switch (type)
		{
		case RaytracingHitGroupType_Triangles:		return D3D12_HIT_GROUP_TYPE_TRIANGLES;
		case RaytracingHitGroupType_Procedural:		return D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
		default:	SKTBD_CORE_ASSERT(false, "Invalid type supplied on this hit group!"); return D3D12_HIT_GROUP_TYPE_TRIANGLES;
		}
	}

	D3D12_RAYTRACING_GEOMETRY_TYPE GeometryTypeToD3D(GeometryType_ type)
	{
		switch (type)
		{
		case GeometryType_Triangles:	return D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		case GeometryType_Procedural_Analytics:
		case GeometryType_Procedural_Volumetrics:
		case GeometryType_Procedural_SDFs:
			return D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
		default: SKTBD_CORE_ASSERT(false, "Unknown geometry type. Triangles assumed."); return D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		}
	}

	DXGI_FORMAT ShaderDataTypeToD3D(ShaderDataType_ type)
	{
		// Can this be done in a much more elegant way?
		switch (type)
		{
		case ShaderDataType_::None:			return DXGI_FORMAT_UNKNOWN;
		case ShaderDataType_::Bool:			return DXGI_FORMAT_R8_UNORM;
		case ShaderDataType_::Int:			return DXGI_FORMAT_R32_SINT;
		case ShaderDataType_::Int2:			return DXGI_FORMAT_R32G32_SINT;
		case ShaderDataType_::Int3:			return DXGI_FORMAT_R32G32B32_SINT;
		case ShaderDataType_::Int4:			return DXGI_FORMAT_R32G32B32A32_SINT;
		case ShaderDataType_::Uint:			return DXGI_FORMAT_R32_UINT;
		case ShaderDataType_::Uint2:		return DXGI_FORMAT_R32G32_UINT;
		case ShaderDataType_::Uint3:		return DXGI_FORMAT_R32G32B32_UINT;
		case ShaderDataType_::Uint4:		return DXGI_FORMAT_R32G32B32A32_UINT;
		case ShaderDataType_::Float:		return DXGI_FORMAT_R32_FLOAT;
		case ShaderDataType_::Float2:		return DXGI_FORMAT_R32G32_FLOAT;
		case ShaderDataType_::Float3:		return DXGI_FORMAT_R32G32B32_FLOAT;
		case ShaderDataType_::Float4:		return DXGI_FORMAT_R32G32B32A32_FLOAT;
		default:							SKTBD_CORE_ASSERT(false, "Impossible shader data type supplied in pipeline!"); return DXGI_FORMAT_UNKNOWN;
		}
	}

	D3D12_SHADER_VISIBILITY ShaderVisibilityToD3D(ShaderVisibility_ v)
	{
		switch (v)
		{
		case ShaderVisibility_All:				return D3D12_SHADER_VISIBILITY_ALL;
		case ShaderVisibility_VertexShader:		return D3D12_SHADER_VISIBILITY_VERTEX;
		case ShaderVisibility_HullShader:		return D3D12_SHADER_VISIBILITY_HULL;
		case ShaderVisibility_DomainShader:		return D3D12_SHADER_VISIBILITY_DOMAIN;
		case ShaderVisibility_GeometryShader:	return D3D12_SHADER_VISIBILITY_GEOMETRY;
		case ShaderVisibility_PixelShader:		return D3D12_SHADER_VISIBILITY_PIXEL;
		default:								return D3D12_SHADER_VISIBILITY_ALL;
		}
	}

	D3D12_DESCRIPTOR_RANGE_TYPE ShaderDescriptorTableTypeToD3D(ShaderDescriptorTableType_ type)
	{
		switch (type)
		{
		case ShaderDescriptorType_SRV:		return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		case ShaderDescriptorType_UAV:		return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		case ShaderDescriptorType_CBV:		return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		case ShaderDescriptorType_Sampler:	return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		default:
			SKTBD_CORE_WARN("Invalid Descriptor Range type supplied. Assuming SRV.");
			return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		}
	}

	D3D12_FILTER SamplerFilterToD3D(SamplerFilter_ filter)
	{
		switch (filter)
		{
		case SamplerFilter_Min_Mag_Mip_Point:								return D3D12_FILTER_MIN_MAG_MIP_POINT;
		case SamplerFilter_Min_Mag_Point_Mip_Linear:						return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		case SamplerFilter_Min_Point_Mag_Linear_Mip_Point:					return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case SamplerFilter_Min_Point_Mag_Mip_Linear:						return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		case SamplerFilter_Min_Linear_Mag_Mip_Point:						return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		case SamplerFilter_Min_Linear_Mag_Point_Mip_Linear:					return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case SamplerFilter_Min_Mag_Linear_Mip_Point:						return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		case SamplerFilter_Min_Mag_Mip_Linear:								return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		case SamplerFilter_Anisotropic:										return D3D12_FILTER_ANISOTROPIC;
		case SamplerFilter_Comaprison_Min_Mag_Mip_Point:					return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		case SamplerFilter_Comparions_Min_Mag_Point_Mip_Linear:				return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
		case SamplerFilter_Comparison_Min_Point_Mag_Linear_Mip_Point:		return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case SamplerFilter_Comparison_Min_Point_Mag_Mip_Linear:				return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
		case SamplerFilter_Comaprison_Min_Linear_Mag_Mip_Point:				return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
		case SamplerFilter_Comparison_Min_Linear_Mag_Point_Mip_Linear:		return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case SamplerFilter_Comparison_Min_Mag_Linear_Mip_Point:				return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		case SamplerFilter_Comparison_Min_Mag_Mip_Linear:					return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		case SamplerFilter_Comparison_Anisotropic:							return D3D12_FILTER_COMPARISON_ANISOTROPIC;
		case SamplerFilter_Minimum_Min_Mag_Mip_Point:						return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
		case SamplerFilter_Minimum_Min_Mag_Point_Mip_Linear:				return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
		case SamplerFilter_Minimum_Min_Point_Mag_Linear_Mip_Point:			return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case SamplerFilter_Minimum_Min_Point_Mag_Mip_Linear:				return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
		case SamplerFilter_Minimum_Min_Point_Mag_Mip_Point:					return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
		case SamplerFilter_Minimum_Min_Linear_Mag_Point_Mip_Linear:			return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case SamplerFilter_Minimum_Min_Mag_Linear_Mip_Point:				return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
		case SamplerFilter_Minimum_Min_Mag_Mip_Linear:						return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
		case SamplerFilter_Minimum_Anisotropic:								return D3D12_FILTER_MINIMUM_ANISOTROPIC;
		case SamplerFilter_Maximum_Min_Mag_Mip_Point:						return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
		case SamplerFilter_Maximum_Min_Mag_Point_Mip_Linear:				return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
		case SamplerFilter_Maximum_Min_Point_Mag_Linear_Mip_Point:			return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case SamplerFilter_Maximum_Min_Point_Mag_Mip_Linear:				return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
		case SamplerFilter_Maximum_Min_Point_Mag_Mip_Point:					return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
		case SamplerFilter_Maximum_Min_Linear_Mag_Point_Mip_Linear:			return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case SamplerFilter_Maximum_Min_Mag_Linear_Mip_Point:				return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
		case SamplerFilter_Maximum_Min_Mag_Mip_Linear:						return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
		case SamplerFilter_Maximum_Anisotropic:								return D3D12_FILTER_MAXIMUM_ANISOTROPIC;
		default:															return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		}
	}

	D3D12_TEXTURE_ADDRESS_MODE SamplerModeToD3D(SamplerMode_ mode)
	{
		switch (mode)
		{
		case SamplerMode_Warp:			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case SamplerMode_Mirror:		return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case SamplerMode_Clamp:			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case SamplerMode_Border:		return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		case SamplerMode_Mirror_Once:	return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		default:						return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		}
	}

	D3D12_STATIC_BORDER_COLOR SamplerBorderColourToD3D(SamplerBorderColour_ colour)
	{
		switch (colour)
		{
		case SamplerBorderColour_TransparentBlack:	return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		case SamplerBorderColour_White:				return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		case SamplerBorderColour_Black:				return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		default:									return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		}
	}

	DXGI_FORMAT DepthStencilToSRVD3D(BufferFormat_ format)
	{
		switch (format)
		{
		case BufferFormat_D32_FLOAT_S8X24_UINT:	return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		case BufferFormat_D32_FLOAT:			return DXGI_FORMAT_R32_FLOAT;
		case BufferFormat_D24_UNORM_S8_UINT:	return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case BufferFormat_D16_UNORM:			return DXGI_FORMAT_R16_UNORM;
		default:
			SKTBD_CORE_ASSERT(false, "Invalid depth/stencil buffer format.");
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	DXGI_FORMAT BufferFormatToD3D(BufferFormat_ format)
	{
		switch (format)
		{
		case BufferFormat_UNKNOWN:									return DXGI_FORMAT_UNKNOWN;
		case BufferFormat_R32G32B32A32_TYPELESS:					return DXGI_FORMAT_R32G32B32A32_TYPELESS;
		case BufferFormat_R32G32B32A32_FLOAT:						return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case BufferFormat_R32G32B32A32_UINT:						return DXGI_FORMAT_R32G32B32A32_UINT;
		case BufferFormat_R32G32B32A32_SINT:						return DXGI_FORMAT_R32G32B32A32_SINT;
		case BufferFormat_R32G32B32_TYPELESS:						return DXGI_FORMAT_R32G32B32_TYPELESS;
		case BufferFormat_R32G32B32_FLOAT:							return DXGI_FORMAT_R32G32B32_FLOAT;
		case BufferFormat_R32G32B32_UINT:							return DXGI_FORMAT_R32G32B32_UINT;
		case BufferFormat_R32G32B32_SINT:							return DXGI_FORMAT_R32G32B32_SINT;
		case BufferFormat_R16G16B16A16_TYPELESS:					return DXGI_FORMAT_R16G16B16A16_TYPELESS;
		case BufferFormat_R16G16B16A16_FLOAT:						return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case BufferFormat_R16G16B16A16_UNORM:						return DXGI_FORMAT_R16G16B16A16_UNORM;
		case BufferFormat_R16G16B16A16_UINT:						return DXGI_FORMAT_R16G16B16A16_UINT;
		case BufferFormat_R16G16B16A16_SNORM:						return DXGI_FORMAT_R16G16B16A16_SNORM;
		case BufferFormat_R16G16B16A16_SINT:						return DXGI_FORMAT_R16G16B16A16_SINT;
		case BufferFormat_R32G32_TYPELESS:							return DXGI_FORMAT_R32G32_TYPELESS;
		case BufferFormat_R32G32_FLOAT:								return DXGI_FORMAT_R32G32_FLOAT;
		case BufferFormat_R32G32_UINT:								return DXGI_FORMAT_R32G32_UINT;
		case BufferFormat_R32G32_SINT:								return DXGI_FORMAT_R32G32_SINT;
		case BufferFormat_R32G8X24_TYPELESS:						return DXGI_FORMAT_R32G8X24_TYPELESS;
		case BufferFormat_D32_FLOAT_S8X24_UINT:						return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case BufferFormat_R32_FLOAT_X8X24_TYPELESS:					return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
		case BufferFormat_X32_TYPELESS_G8X24_UINT:					return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
		case BufferFormat_R10G10B10A2_TYPELESS:						return DXGI_FORMAT_R10G10B10A2_TYPELESS;
		case BufferFormat_R10G10B10A2_UNORM:						return DXGI_FORMAT_R10G10B10A2_UNORM;
		case BufferFormat_R10G10B10A2_UINT:							return DXGI_FORMAT_R10G10B10A2_UINT;
		case BufferFormat_R11G11B10_FLOAT:							return DXGI_FORMAT_R11G11B10_FLOAT;
		case BufferFormat_R8G8B8A8_TYPELESS:						return DXGI_FORMAT_R8G8B8A8_TYPELESS;
		case BufferFormat_R8G8B8A8_UNORM:							return DXGI_FORMAT_R8G8B8A8_UNORM;
		case BufferFormat_R8G8B8A8_UNORM_SRGB:						return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case BufferFormat_R8G8B8A8_UINT:							return DXGI_FORMAT_R8G8B8A8_UINT;
		case BufferFormat_R8G8B8A8_SNORM:							return DXGI_FORMAT_R8G8B8A8_SNORM;
		case BufferFormat_R8G8B8A8_SINT:							return DXGI_FORMAT_R8G8B8A8_SINT;
		case BufferFormat_R16G16_TYPELESS:							return DXGI_FORMAT_R16G16_TYPELESS;
		case BufferFormat_R16G16_FLOAT:								return DXGI_FORMAT_R16G16_FLOAT;
		case BufferFormat_R16G16_UNORM:								return DXGI_FORMAT_R16G16_UNORM;
		case BufferFormat_R16G16_UINT:								return DXGI_FORMAT_R16G16_UINT;
		case BufferFormat_R16G16_SNORM:								return DXGI_FORMAT_R16G16_SNORM;
		case BufferFormat_R16G16_SINT:								return DXGI_FORMAT_R16G16_SINT;
		case BufferFormat_R32_TYPELESS:								return DXGI_FORMAT_R32_TYPELESS;
		case BufferFormat_D32_FLOAT:								return DXGI_FORMAT_D32_FLOAT;
		case BufferFormat_R32_FLOAT:								return DXGI_FORMAT_R32_FLOAT;
		case BufferFormat_R32_UINT:									return DXGI_FORMAT_R32_UINT;
		case BufferFormat_R32_SINT:									return DXGI_FORMAT_R32_SINT;
		case BufferFormat_R24G8_TYPELESS:							return DXGI_FORMAT_R24G8_TYPELESS;
		case BufferFormat_D24_UNORM_S8_UINT:						return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case BufferFormat_R24_UNORM_X8_TYPELESS:					return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case BufferFormat_X24_TYPELESS_G8_UINT:						return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		case BufferFormat_R8G8_TYPELESS:							return DXGI_FORMAT_R8G8_TYPELESS;
		case BufferFormat_R8G8_UNORM:								return DXGI_FORMAT_R8G8_UNORM;
		case BufferFormat_R8G8_UINT:								return DXGI_FORMAT_R8G8_UINT;
		case BufferFormat_R8G8_SNORM:								return DXGI_FORMAT_R8G8_SNORM;
		case BufferFormat_R8G8_SINT:								return DXGI_FORMAT_R8G8_SINT;
		case BufferFormat_R16_TYPELESS:								return DXGI_FORMAT_R16_TYPELESS;
		case BufferFormat_R16_FLOAT:								return DXGI_FORMAT_R16_FLOAT;
		case BufferFormat_D16_UNORM:								return DXGI_FORMAT_D16_UNORM;
		case BufferFormat_R16_UNORM:								return DXGI_FORMAT_R16_UNORM;
		case BufferFormat_R16_UINT:									return DXGI_FORMAT_R16_UINT;
		case BufferFormat_R16_SNORM:								return DXGI_FORMAT_R16_SNORM;
		case BufferFormat_R16_SINT:									return DXGI_FORMAT_R16_SINT;
		case BufferFormat_R8_TYPELESS:								return DXGI_FORMAT_R8_TYPELESS;
		case BufferFormat_R8_UNORM:									return DXGI_FORMAT_R8_UNORM;
		case BufferFormat_R8_UINT:									return DXGI_FORMAT_R8_UINT;
		case BufferFormat_R8_SNORM:									return DXGI_FORMAT_R8_SNORM;
		case BufferFormat_R8_SINT:									return DXGI_FORMAT_R8_SINT;
		case BufferFormat_A8_UNORM:									return DXGI_FORMAT_A8_UNORM;
		case BufferFormat_R1_UNORM:									return DXGI_FORMAT_R1_UNORM;
		case BufferFormat_R9G9B9E5_SHAREDEXP:						return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
		case BufferFormat_R8G8_B8G8_UNORM:							return DXGI_FORMAT_R8G8_B8G8_UNORM;
		case BufferFormat_G8R8_G8B8_UNORM:							return DXGI_FORMAT_G8R8_G8B8_UNORM;
		case BufferFormat_BC1_TYPELESS:								return DXGI_FORMAT_BC1_TYPELESS;
		case BufferFormat_BC1_UNORM:								return DXGI_FORMAT_BC1_UNORM;
		case BufferFormat_BC1_UNORM_SRGB:							return DXGI_FORMAT_BC1_UNORM_SRGB;
		case BufferFormat_BC2_TYPELESS:								return DXGI_FORMAT_BC2_TYPELESS;
		case BufferFormat_BC2_UNORM:								return DXGI_FORMAT_BC2_UNORM;
		case BufferFormat_BC2_UNORM_SRGB:							return DXGI_FORMAT_BC2_UNORM_SRGB;
		case BufferFormat_BC3_TYPELESS:								return DXGI_FORMAT_BC3_TYPELESS;
		case BufferFormat_BC3_UNORM:								return DXGI_FORMAT_BC3_UNORM;
		case BufferFormat_BC3_UNORM_SRGB:							return DXGI_FORMAT_BC3_UNORM_SRGB;
		case BufferFormat_BC4_TYPELESS:								return DXGI_FORMAT_BC4_TYPELESS;
		case BufferFormat_BC4_UNORM:								return DXGI_FORMAT_BC4_UNORM;
		case BufferFormat_BC4_SNORM:								return DXGI_FORMAT_BC4_SNORM;
		case BufferFormat_BC5_TYPELESS:								return DXGI_FORMAT_BC5_TYPELESS;
		case BufferFormat_BC5_UNORM:								return DXGI_FORMAT_BC5_UNORM;
		case BufferFormat_BC5_SNORM:								return DXGI_FORMAT_BC5_SNORM;
		case BufferFormat_B5G6R5_UNORM:								return DXGI_FORMAT_B5G6R5_UNORM;
		case BufferFormat_B5G5R5A1_UNORM:							return DXGI_FORMAT_B5G5R5A1_UNORM;
		case BufferFormat_B8G8R8A8_UNORM:							return DXGI_FORMAT_B8G8R8A8_UNORM;
		case BufferFormat_B8G8R8X8_UNORM:							return DXGI_FORMAT_B8G8R8X8_UNORM;
		case BufferFormat_R10G10B10_XR_BIAS_A2_UNORM:				return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
		case BufferFormat_B8G8R8A8_TYPELESS:						return DXGI_FORMAT_B8G8R8A8_TYPELESS;
		case BufferFormat_B8G8R8A8_UNORM_SRGB:						return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case BufferFormat_B8G8R8X8_TYPELESS:						return DXGI_FORMAT_B8G8R8X8_TYPELESS;
		case BufferFormat_B8G8R8X8_UNORM_SRGB:						return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
		case BufferFormat_BC6H_TYPELESS:							return DXGI_FORMAT_BC6H_TYPELESS;
		case BufferFormat_BC6H_UF16:								return DXGI_FORMAT_BC6H_UF16;
		case BufferFormat_BC6H_SF16:								return DXGI_FORMAT_BC6H_SF16;
		case BufferFormat_BC7_TYPELESS:								return DXGI_FORMAT_BC7_TYPELESS;
		case BufferFormat_BC7_UNORM:								return DXGI_FORMAT_BC7_UNORM;
		case BufferFormat_BC7_UNORM_SRGB:							return DXGI_FORMAT_BC7_UNORM_SRGB;
		case BufferFormat_AYUV:										return DXGI_FORMAT_AYUV;
		case BufferFormat_Y410:										return DXGI_FORMAT_Y410;
		case BufferFormat_Y416:										return DXGI_FORMAT_Y416;
		case BufferFormat_NV12:										return DXGI_FORMAT_NV12;
		case BufferFormat_P010:										return DXGI_FORMAT_P010;
		case BufferFormat_P016:										return DXGI_FORMAT_P016;
		case BufferFormat_420_OPAQUE:								return DXGI_FORMAT_420_OPAQUE;
		case BufferFormat_YUY2:										return DXGI_FORMAT_YUY2;
		case BufferFormat_Y210:										return DXGI_FORMAT_Y210;
		case BufferFormat_Y216:										return DXGI_FORMAT_Y216;
		case BufferFormat_NV11:										return DXGI_FORMAT_NV11;
		case BufferFormat_AI44:										return DXGI_FORMAT_AI44;
		case BufferFormat_IA44:										return DXGI_FORMAT_IA44;
		case BufferFormat_P8:										return DXGI_FORMAT_P8;
		case BufferFormat_A8P8:										return DXGI_FORMAT_A8P8;
		case BufferFormat_B4G4R4A4_UNORM:							return DXGI_FORMAT_B4G4R4A4_UNORM;
		case BufferFormat_P208:										return DXGI_FORMAT_P208;
		case BufferFormat_V208:										return DXGI_FORMAT_V208;
		case BufferFormat_V408:										return DXGI_FORMAT_V408;
		case BufferFormat_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE:			return DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE;
		case BufferFormat_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE:	return DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE;
		default:													return DXGI_FORMAT_FORCE_UINT;
		}
	}

	BufferFormat_ D3DBufferFormatToSkateboard(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_UNKNOWN:									return BufferFormat_UNKNOWN;
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:						return BufferFormat_R32G32B32A32_TYPELESS;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:						return BufferFormat_R32G32B32A32_FLOAT;
		case DXGI_FORMAT_R32G32B32A32_UINT:							return BufferFormat_R32G32B32A32_UINT;
		case DXGI_FORMAT_R32G32B32A32_SINT:							return BufferFormat_R32G32B32A32_SINT;
		case DXGI_FORMAT_R32G32B32_TYPELESS:						return BufferFormat_R32G32B32_TYPELESS;
		case DXGI_FORMAT_R32G32B32_FLOAT:							return BufferFormat_R32G32B32_FLOAT;
		case DXGI_FORMAT_R32G32B32_UINT:							return BufferFormat_R32G32B32_UINT;
		case DXGI_FORMAT_R32G32B32_SINT:							return BufferFormat_R32G32B32_SINT;
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:						return BufferFormat_R16G16B16A16_TYPELESS;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:						return BufferFormat_R16G16B16A16_FLOAT;
		case DXGI_FORMAT_R16G16B16A16_UNORM:						return BufferFormat_R16G16B16A16_UNORM;
		case DXGI_FORMAT_R16G16B16A16_UINT:							return BufferFormat_R16G16B16A16_UINT;
		case DXGI_FORMAT_R16G16B16A16_SNORM:						return BufferFormat_R16G16B16A16_SNORM;
		case DXGI_FORMAT_R16G16B16A16_SINT:							return BufferFormat_R16G16B16A16_SINT;
		case DXGI_FORMAT_R32G32_TYPELESS:							return BufferFormat_R32G32_TYPELESS;
		case DXGI_FORMAT_R32G32_FLOAT:								return BufferFormat_R32G32_FLOAT;
		case DXGI_FORMAT_R32G32_UINT:								return BufferFormat_R32G32_UINT;
		case DXGI_FORMAT_R32G32_SINT:								return BufferFormat_R32G32_SINT;
		case DXGI_FORMAT_R32G8X24_TYPELESS:							return BufferFormat_R32G8X24_TYPELESS;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:						return BufferFormat_D32_FLOAT_S8X24_UINT;
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:					return BufferFormat_R32_FLOAT_X8X24_TYPELESS;
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:					return BufferFormat_X32_TYPELESS_G8X24_UINT;
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:						return BufferFormat_R10G10B10A2_TYPELESS;
		case DXGI_FORMAT_R10G10B10A2_UNORM:							return BufferFormat_R10G10B10A2_UNORM;
		case DXGI_FORMAT_R10G10B10A2_UINT:							return BufferFormat_R10G10B10A2_UINT;
		case DXGI_FORMAT_R11G11B10_FLOAT:							return BufferFormat_R11G11B10_FLOAT;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:							return BufferFormat_R8G8B8A8_TYPELESS;
		case DXGI_FORMAT_R8G8B8A8_UNORM:							return BufferFormat_R8G8B8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:						return BufferFormat_R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT_R8G8B8A8_UINT:								return BufferFormat_R8G8B8A8_UINT;
		case DXGI_FORMAT_R8G8B8A8_SNORM:							return BufferFormat_R8G8B8A8_SNORM;
		case DXGI_FORMAT_R8G8B8A8_SINT:								return BufferFormat_R8G8B8A8_SINT;
		case DXGI_FORMAT_R16G16_TYPELESS:							return BufferFormat_R16G16_TYPELESS;
		case DXGI_FORMAT_R16G16_FLOAT:								return BufferFormat_R16G16_FLOAT;
		case DXGI_FORMAT_R16G16_UNORM:								return BufferFormat_R16G16_UNORM;
		case DXGI_FORMAT_R16G16_UINT:								return BufferFormat_R16G16_UINT;
		case DXGI_FORMAT_R16G16_SNORM:								return BufferFormat_R16G16_SNORM;
		case DXGI_FORMAT_R16G16_SINT:								return BufferFormat_R16G16_SINT;
		case DXGI_FORMAT_R32_TYPELESS:								return BufferFormat_R32_TYPELESS;
		case DXGI_FORMAT_D32_FLOAT:									return BufferFormat_D32_FLOAT;
		case DXGI_FORMAT_R32_FLOAT:									return BufferFormat_R32_FLOAT;
		case DXGI_FORMAT_R32_UINT:									return BufferFormat_R32_UINT;
		case DXGI_FORMAT_R32_SINT:									return BufferFormat_R32_SINT;
		case DXGI_FORMAT_R24G8_TYPELESS:							return BufferFormat_R24G8_TYPELESS;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:							return BufferFormat_D24_UNORM_S8_UINT;
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:						return BufferFormat_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:						return BufferFormat_X24_TYPELESS_G8_UINT;
		case DXGI_FORMAT_R8G8_TYPELESS:								return BufferFormat_R8G8_TYPELESS;
		case DXGI_FORMAT_R8G8_UNORM:								return BufferFormat_R8G8_UNORM;
		case DXGI_FORMAT_R8G8_UINT:									return BufferFormat_R8G8_UINT;
		case DXGI_FORMAT_R8G8_SNORM:								return BufferFormat_R8G8_SNORM;
		case DXGI_FORMAT_R8G8_SINT:									return BufferFormat_R8G8_SINT;
		case DXGI_FORMAT_R16_TYPELESS:								return BufferFormat_R16_TYPELESS;
		case DXGI_FORMAT_R16_FLOAT:									return BufferFormat_R16_FLOAT;
		case DXGI_FORMAT_D16_UNORM:									return BufferFormat_D16_UNORM;
		case DXGI_FORMAT_R16_UNORM:									return BufferFormat_R16_UNORM;
		case DXGI_FORMAT_R16_UINT:									return BufferFormat_R16_UINT;
		case DXGI_FORMAT_R16_SNORM:									return BufferFormat_R16_SNORM;
		case DXGI_FORMAT_R16_SINT:									return BufferFormat_R16_SINT;
		case DXGI_FORMAT_R8_TYPELESS:								return BufferFormat_R8_TYPELESS;
		case DXGI_FORMAT_R8_UNORM:									return BufferFormat_R8_UNORM;
		case DXGI_FORMAT_R8_UINT:									return BufferFormat_R8_UINT;
		case DXGI_FORMAT_R8_SNORM:									return BufferFormat_R8_SNORM;
		case DXGI_FORMAT_R8_SINT:									return BufferFormat_R8_SINT;
		case DXGI_FORMAT_A8_UNORM:									return BufferFormat_A8_UNORM;
		case DXGI_FORMAT_R1_UNORM:									return BufferFormat_R1_UNORM;
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:						return BufferFormat_R9G9B9E5_SHAREDEXP;
		case DXGI_FORMAT_R8G8_B8G8_UNORM:							return BufferFormat_R8G8_B8G8_UNORM;
		case DXGI_FORMAT_G8R8_G8B8_UNORM:							return BufferFormat_G8R8_G8B8_UNORM;
		case DXGI_FORMAT_BC1_TYPELESS:								return BufferFormat_BC1_TYPELESS;
		case DXGI_FORMAT_BC1_UNORM:									return BufferFormat_BC1_UNORM;
		case DXGI_FORMAT_BC1_UNORM_SRGB:							return BufferFormat_BC1_UNORM_SRGB;
		case DXGI_FORMAT_BC2_TYPELESS:								return BufferFormat_BC2_TYPELESS;
		case DXGI_FORMAT_BC2_UNORM:									return BufferFormat_BC2_UNORM;
		case DXGI_FORMAT_BC2_UNORM_SRGB:							return BufferFormat_BC2_UNORM_SRGB;
		case DXGI_FORMAT_BC3_TYPELESS:								return BufferFormat_BC3_TYPELESS;
		case DXGI_FORMAT_BC3_UNORM:									return BufferFormat_BC3_UNORM;
		case DXGI_FORMAT_BC3_UNORM_SRGB:							return BufferFormat_BC3_UNORM_SRGB;
		case DXGI_FORMAT_BC4_TYPELESS:								return BufferFormat_BC4_TYPELESS;
		case DXGI_FORMAT_BC4_UNORM:									return BufferFormat_BC4_UNORM;
		case DXGI_FORMAT_BC4_SNORM:									return BufferFormat_BC4_SNORM;
		case DXGI_FORMAT_BC5_TYPELESS:								return BufferFormat_BC5_TYPELESS;
		case DXGI_FORMAT_BC5_UNORM:									return BufferFormat_BC5_UNORM;
		case DXGI_FORMAT_BC5_SNORM:									return BufferFormat_BC5_SNORM;
		case DXGI_FORMAT_B5G6R5_UNORM:								return BufferFormat_B5G6R5_UNORM;
		case DXGI_FORMAT_B5G5R5A1_UNORM:							return BufferFormat_B5G5R5A1_UNORM;
		case DXGI_FORMAT_B8G8R8A8_UNORM:							return BufferFormat_B8G8R8A8_UNORM;
		case DXGI_FORMAT_B8G8R8X8_UNORM:							return BufferFormat_B8G8R8X8_UNORM;
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:				return BufferFormat_R10G10B10_XR_BIAS_A2_UNORM;
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:							return BufferFormat_B8G8R8A8_TYPELESS;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:						return BufferFormat_B8G8R8A8_UNORM_SRGB;
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:							return BufferFormat_B8G8R8X8_TYPELESS;
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:						return BufferFormat_B8G8R8X8_UNORM_SRGB;
		case DXGI_FORMAT_BC6H_TYPELESS:								return BufferFormat_BC6H_TYPELESS;
		case DXGI_FORMAT_BC6H_UF16:									return BufferFormat_BC6H_UF16;
		case DXGI_FORMAT_BC6H_SF16:									return BufferFormat_BC6H_SF16;
		case DXGI_FORMAT_BC7_TYPELESS:								return BufferFormat_BC7_TYPELESS;
		case DXGI_FORMAT_BC7_UNORM:									return BufferFormat_BC7_UNORM;
		case DXGI_FORMAT_BC7_UNORM_SRGB:							return BufferFormat_BC7_UNORM_SRGB;
		case DXGI_FORMAT_AYUV:										return BufferFormat_AYUV;
		case DXGI_FORMAT_Y410:										return BufferFormat_Y410;
		case DXGI_FORMAT_Y416:										return BufferFormat_Y416;
		case DXGI_FORMAT_NV12:										return BufferFormat_NV12;
		case DXGI_FORMAT_P010:										return BufferFormat_P010;
		case DXGI_FORMAT_P016:										return BufferFormat_P016;
		case DXGI_FORMAT_420_OPAQUE:								return BufferFormat_420_OPAQUE;
		case DXGI_FORMAT_YUY2:										return BufferFormat_YUY2;
		case DXGI_FORMAT_Y210:										return BufferFormat_Y210;
		case DXGI_FORMAT_Y216:										return BufferFormat_Y216;
		case DXGI_FORMAT_NV11:										return BufferFormat_NV11;
		case DXGI_FORMAT_AI44:										return BufferFormat_AI44;
		case DXGI_FORMAT_IA44:										return BufferFormat_IA44;
		case DXGI_FORMAT_P8:										return BufferFormat_P8;
		case DXGI_FORMAT_A8P8:										return BufferFormat_A8P8;
		case DXGI_FORMAT_B4G4R4A4_UNORM:							return BufferFormat_B4G4R4A4_UNORM;
		case DXGI_FORMAT_P208:										return BufferFormat_P208;
		case DXGI_FORMAT_V208:										return BufferFormat_V208;
		case DXGI_FORMAT_V408:										return BufferFormat_V408;
		case DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE:			return BufferFormat_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE;
		case DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE:	return BufferFormat_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE;
		default:													return BufferFormat_FORCE_UINT;
		}
	}

	D3D12_RESOURCE_STATES ResourceTypeToStateD3D(GPUResourceType_ type)
	{
		switch (type)
		{
		case GPUResourceType_DefaultBuffer:
			return D3D12_RESOURCE_STATE_GENERIC_READ;
		case GPUResourceType_Texture2D:
		case GPUResourceType_Texture3D:
			return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
		case GPUResourceType_UnorderedAccessBuffer:
			return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		default:
			SKTBD_CORE_ASSERT(false, "This resource type cannot or should not be switched to a state. Illegal state.");
			return D3D12_RESOURCE_STATE_COMMON;
		}
	}
#endif
}