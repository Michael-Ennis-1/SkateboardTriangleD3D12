#pragma once
#include "Skateboard/Log.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Renderer/Model/Model.h"

namespace Skateboard
{

 

	//	<summary>
	//	The meshlet engine serves mainly to generate meshlet buffers from geometry which has been configured for the
	//  traditional pipeline. Additionally, the class contains features to flatten meshlet buffers into traditional vertex
	//	and index buffers.
	//	</summary>
	class MeshEngine
	{
	public:
		MeshEngine() = default;
		DISABLE_COPY_AND_MOVE(MeshEngine);
		virtual ~MeshEngine(){}

		virtual void GenerateMeshlets
		(
			uint32_t maxVertices, uint32_t maxPrimitives,
			uint32_t* indices, uint32_t iCount,
			float3* positions, uint32_t vCount,
			std::vector<Subset>& subsets,
            std::vector<Meshlet>& meshlets,
            std::vector<uint8_t>& uniqueVertexIndices,
            std::vector<PackedTriangle>& primitiveIndices
		) {}

        virtual void GenerateMeshlets
        (
            uint32_t maxVertices, uint32_t maxPrimitives,
            uint16_t* indices, uint32_t iCount,
            float3* positions, uint32_t vCount,
            std::vector<Subset>& subsets,
            std::vector<Meshlet>& meshlets,
            std::vector<uint8_t>& uniqueVertexIndices,
            std::vector<PackedTriangle>& primitiveIndices
        ) {}

		virtual void GenerateIBuffers
		(
            uint32_t maxVertices, uint32_t maxPrimitives,
            float3* positions, uint32_t vCount,
            std::vector<Subset>& subsets,
            std::vector<Meshlet>& meshlets,
            std::vector<uint8_t>& uniqueVertexIndices,
            std::vector<PackedTriangle>& primitiveIndices,
            std::vector<uint32_t> outIBuffer
        ) {}

        virtual void GenerateIBuffers
        (
            uint32_t maxVertices, uint32_t maxPrimitives,
            float3* positions, uint32_t vCount,
            std::vector<Subset>& subsets,
            std::vector<Meshlet>& meshlets,
            std::vector<uint8_t>& uniqueVertexIndices,
            std::vector<PackedTriangle>& primitiveIndices,
            std::vector<uint16_t> outIBuffer
        ) {}

	protected:

	};


    class MeshletBank
    {
    public:
        MeshletBank() = default;
        ~MeshletBank() = default;

        MeshletModel* AddMeshletModel(const std::wstring_view& name, MeshletModel* model);
        MeshletModel* AddMeshletModel(const std::wstring_view& name, std::unique_ptr<MeshletModel>& model);

        MeshletModel* Get(const std::wstring_view& name);

        bool Contains(const std::wstring_view& name);
        bool ReleaseModel(const std::wstring_view& name);

    private:
        std::unordered_map<std::wstring_view, std::unique_ptr<MeshletModel>> m_MeshletModels;
    };


    class MeshBank
    {
    public:
        Model* AddModel(const std::wstring_view& name, Model* model);
        Model* AddModel(const std::wstring_view& name, std::unique_ptr<Model>& model);

        Model* Get(const std::wstring_view& name);

        bool Contains(const std::wstring_view& name);
        bool ReleaseModel(const std::wstring_view& name);

    private:
        std::unordered_map<std::wstring_view, std::unique_ptr<Model>> m_Models;

    };

}
