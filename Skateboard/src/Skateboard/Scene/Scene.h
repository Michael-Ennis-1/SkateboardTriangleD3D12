#pragma once
#include "entt.hpp"
#include "SceneRenderer.h"

#include "Skateboard/Scene/InstanceData.h"
#include "Skateboard/Renderer/Buffer.h"
#include "Skateboard/SizedPtr.h"
#include "Skateboard/Scene/AABB.h"

struct PassBuffer
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 ViewMatrixInverse;
	float4x4 ProjectionMatrixInverse;
	float DeltaTime;
	float ElapsedTime;
};

struct InstanceBuffer
{
	float4x4 WorldMatrix;
	uint32_t MeshID;
	uint32_t MaterialIndex;
	uint32_t Pad0;
	uint32_t Pad1;
};


struct LightBuffer
{
	float3	Diffuse{ 0.9f, 0.9f, 0.9f };
	float	FalloffStart{ 1.0f };
	float3	Direction{ 0.5f, -1.0f, 0.0f };
	float	FalloffEnd{ 10.0f };
	float3	Position{ 0.f, 5.0f, 0.f };
	float	SpotPower{ 64.0f };
	float3	Radiance{ 0.01f, 0.01f, 0.01f };
	float	Pad0{ 0.f };
	float4x4 LightViewProjTex;
	float3  CameraPosition{ 0.f,0.f,0.f };
	float	Pad1{ 0.f };
};

struct MaterialData
{
	float4 m_Albedo;
	float3 m_FresnelR0;
	float m_Metallic;
	float3 m_Specular;
	float m_Roughness;
};

struct ProceduralPrimitiveBuffer
{
	float Radius;
};

namespace Skateboard
{
	class Model;
	class MeshletModel;

	class Entity;
	struct StaticMeshInstanceComponent;

	class Scene
	{
		friend class Entity;
		friend class SceneBuilder;
	public:
		explicit Scene(const std::string& name);

		Scene() = delete;
		DISABLE_COPY_AND_MOVE(Scene);
		~Scene();


		Entity CreateEntity(const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnUpdate(float time);
		void OnRender(float time);

		void BuildInstanceStructuredBuffer(uint32_t instanceBufferSize);

		void UploadInstanceData(Entity entity, void* pData, float4x4* pNewTransform);


		void AddMeshletPipeline(const std::wstring_view& name, std::unique_ptr<MeshletPipeline>& pipeline);
		void RemoveMeshletPipeline(const std::wstring_view& name);
		MeshletPipeline* GetMeshletPipeline(const std::wstring_view& name) const;

		void AddPipeline(const std::wstring_view& name, std::unique_ptr<RasterizationPipeline>& pipeline);
		void RemovePipeline(const std::wstring_view& name);
		RasterizationPipeline* GetPipeline(const std::wstring_view& name);

		MeshletModel* AddMeshletModel(const std::wstring_view& name, std::unique_ptr<MeshletModel>& model);
		void RemoveMeshletModel(const std::wstring_view& name);
		MeshletModel* GetMeshletModel(const std::wstring_view& name) const;

		Model* AddMesh(const std::wstring_view& name, std::unique_ptr<MeshletModel>& model);
		void RemoveMesh(const std::wstring_view& name);
		Model* GetMesh(const std::wstring_view& name) const;

		UploadBuffer* GetInstanceStructuredBuffer();
		VertexBuffer* GetVertexBuffer() const { return m_VertexBuffer.get(); }
		IndexBuffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }
		DefaultBuffer* GetVertexOffsetsBuffer() const { return m_VertexOffsetsBuffer.get(); }
		DefaultBuffer* GetIndexOffsetsBuffer() const { return m_IndexOffsetsBuffer.get(); }
		TopLevelAccelerationStructure* GetTopLevelAccelerationStructure() const { return m_TLAS.get(); }

		entt::registry& GetRegistry() { return Registry; }

		MeshID GetMeshID(const char* nametag) { return m_InstanceData.GetMeshID(nametag); }
		MeshID AddMeshInstance(const char* nametag) { return m_InstanceData.AddMesh(nametag); }
		bool IsMeshValid(MeshID meshID) const { return m_InstanceData.IsMeshValid(meshID); }
		const std::string& GetMeshTag(MeshID meshID) const { return m_InstanceData.GetMeshTag(meshID); }
		uint32_t GetTotalMeshCount() const { return m_InstanceData.GetMeshCount(); }
		void ChangeInstanceMeshID(StaticMeshInstanceComponent& instanceComponent, MeshID newMeshID);
		uint32_t GetMeshStartVertexLocation(MeshID id) const;
		uint32_t GetMeshStartIndexLocation(MeshID id) const;
		uint32_t GetMeshIndexCount(MeshID id) const;
		uint32_t GetTotalInstanceCount() const;
		uint32_t GetMeshInstanceCount(MeshID id) const;
		uint32_t GetMeshStartInstanceLocation(MeshID id) const;

		void BindGeometryBuffers() const;

	private:
		template<typename T> void OnComponentAdded(Entity entity, T& component);
		template<typename T> void OnComponentRemoved(Entity entity, T& removedComponent);

	private:
		entt::registry Registry;
		int32_t Width = 0;
		int32_t Height = 0;

		std::unique_ptr<SceneRenderer> m_SceneRenderer;

		uint32_t m_InstanceBufferID;

		SceneInstanceData				m_InstanceData;
		std::unique_ptr<VertexBuffer>	m_VertexBuffer;
		std::unique_ptr<IndexBuffer>	m_IndexBuffer;
		std::unique_ptr<DefaultBuffer>	m_VertexOffsetsBuffer;
		std::unique_ptr<DefaultBuffer>	m_IndexOffsetsBuffer;
		std::unique_ptr<DefaultBuffer>	m_ProceduralGeometryAABBBuffer;
		std::vector<std::unique_ptr<BottomLevelAccelerationStructure>> v_BLAS;
		std::unique_ptr<TopLevelAccelerationStructure> m_TLAS;
		std::vector<uint32_t> v_StartVertexLocations;	// Index by the MeshID. These have to be vectors as we want these locations in continious memory
		std::vector<uint32_t> v_StartIndexLocations;
		std::vector<uint32_t> v_IndexCounts;
		std::vector<RaytracingAABBDesc> v_RaytracingAABBs;
	};
}
