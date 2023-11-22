#pragma once
#include "Span.h"

#include "Skateboard/Renderer/Buffer.h"

namespace Skateboard
{
	class PerspectiveCamera;

#define CULL_FLAG 0x1
    #define MESHLET_FLAG 0x2

    enum class GIOptions_
    {
        GIOptions_Baked,
        GIOptions_Runtime,
    };

    struct Attribute
    {
        enum EType : uint32_t
        {
            Position,
            Normal,
            TexCoord,
            Tangent,
            Bitangent,
            Count
        };

        EType    Type;
        uint32_t Offset;
    };

    struct Subset
    {
        uint32_t Offset;
        uint32_t Count;
    };

    __declspec(align(256u))
    struct MeshInfo
    {
        uint32_t IndexSize;
        uint32_t MeshletCount;

        uint32_t LastMeshletVertCount;
        uint32_t LastMeshletPrimCount;
    };

    struct Meshlet
    {
        uint32_t VertCount;
        uint32_t VertOffset;
        uint32_t PrimCount;
        uint32_t PrimOffset;
    };

    struct PackedTriangle
    {
        uint32_t i0 : 10;
        uint32_t i1 : 10;
        uint32_t i2 : 10;
    };

    struct CullData
    {
        float4  BoundingSphere;   // xyz = center, w = radius
        uint8_t NormalCone[4];    // xyz = axis, w = -cos(a + 90)
        float   ApexOffset;       // apex = center - axis * offset
    };

    __declspec(align(256u))
	struct MeshletCullPass
    {
        float4x4 View;
        float4x4 ViewProj;
        float4 Planes[6];

        float3 ViewPosition;
        uint32_t HighlightedIndex;

        float3 CullViewPosition;
        uint32_t SelectedIndex;

        uint32_t DrawMeshlets;
    };

    struct MeshInstance
    {
        float4x4 WorldTransform;
        float4x4 WorldInvTransform;
        float Scale;
        uint32_t Flags;
    };

	class Model
	{
	public:
        virtual ~Model() = default;
        static Model* Create(const wchar_t* filename);

		virtual void SetInputLayout(const BufferLayout& inputLayout) = 0;
		virtual bool LoadFromFile(const wchar_t* filename) = 0;

		virtual VertexBuffer* GetVertexBuffer(const std::wstring_view& meshTag) = 0;
		virtual IndexBuffer* GetIndexBuffer(const std::wstring_view& meshTag) = 0;

        virtual GPUResource* GetModelCullData(const std::wstring_view& meshTag) = 0;
        virtual GPUResource* GetSceneCullData() = 0;

        virtual void Cull(PerspectiveCamera* camera, PerspectiveCamera* debugCamera) = 0;

		_NODISCARD virtual const DirectX::BoundingSphere& GetBoundingSphere() const = 0;

        virtual void SetTransform(float4x4 transform) = 0;
		virtual GPUResource* GetTransformBuffer() = 0;

		virtual void Release() = 0;
	};

	class MeshletModel : public Model
	{
	public:
		static MeshletModel* Create(const wchar_t* filename);

		virtual GPUResource* GetMeshletBuffer(const std::wstring_view& meshTag)         = 0;
		virtual GPUResource* GetUniqueVertexIndices(const std::wstring_view& meshTag)   = 0;
		virtual GPUResource* GetPrimitiveIndices(const std::wstring_view& meshTag)      = 0;
        virtual GPUResource* GetMeshInfo(const std::wstring_view& meshTag)              = 0;

		_NODISCARD virtual const std::vector<Span<uint8_t>>& GetRawVertexData(uint32_t meshIndex = 0)			= 0;
		_NODISCARD virtual const Span<Meshlet>&				GetRawMeshletData(uint32_t meshIndex = 0)			= 0;
		_NODISCARD virtual const Span<PackedTriangle>&		GetRawPrimitiveIndices(uint32_t meshIndex = 0)		= 0;
		_NODISCARD virtual const Span<uint8_t>&				GetRawUniqueVertexIndices(uint32_t meshIndex = 0)	= 0;

	};

}
