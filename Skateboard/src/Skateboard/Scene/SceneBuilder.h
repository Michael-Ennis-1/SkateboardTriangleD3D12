#pragma once
#include "InstanceData.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Scene/Entity.h"
#include "AABB.h"

#ifndef SKTBD_SCENEBUILDER_NUM_PRIMITIVES
#define SKTBD_SCENEBUILDER_NUM_PRIMITIVES 6ui64
#endif // !SKTBD_SCENEBUILDER_NUM_PRIMITIVES

namespace Skateboard
{
	class Scene;

	class SceneBuilder
	{
		friend Scene;
	protected:
		struct VertexType
		{
			float3 position;
			float2 texCoord;
			float3 normal;
			float3 tangent;
			float3 bitangent;
		};
		const BufferLayout m_Layout = {
			{ "POSITION", Skateboard::ShaderDataType_::Float3 },
			{ "TEXCOORD", Skateboard::ShaderDataType_::Float2 },
			{ "NORMAL", Skateboard::ShaderDataType_::Float3 },
			{ "TANGENT", Skateboard::ShaderDataType_::Float3 },
			{ "BINORMAL", Skateboard::ShaderDataType_::Float3 }
		};

		struct MeshData
		{
			std::wstring Name;
			std::vector<VertexType> Vertices;
			std::vector<uint32_t> Indices;
		};

	protected:
		SceneBuilder() {}

	public:
		static SceneBuilder& Singleton()
		{
			static SceneBuilder builder;
			return builder;
		}
		~SceneBuilder() {}

		static Entity AddConeInstance(Scene* pScene)		{ return Singleton().AddInstance(pScene, pScene->AddMeshInstance("Cone")); }
		static Entity AddCubeInstance(Scene* pScene)		{ return Singleton().AddInstance(pScene, pScene->AddMeshInstance("Cube")); }
		static Entity AddCubeSphereInstance(Scene* pScene)	{ return Singleton().AddInstance(pScene, pScene->AddMeshInstance("CubeSphere")); }
		static Entity AddCylinderInstance(Scene* pScene)	{ return Singleton().AddInstance(pScene, pScene->AddMeshInstance("Cylinder")); }
		static Entity AddSphereInstance(Scene* pScene)		{ return Singleton().AddInstance(pScene, pScene->AddMeshInstance("Sphere")); }
		static Entity AddTerrainInstance(Scene* pScene)		{ return Singleton().AddInstance(pScene, pScene->AddMeshInstance("Terrain")); }
		static Entity AddRaytracingProceduralGeometryInstance(Scene* pScene, const RaytracingAABBDesc& boundingBox);
		static void GenerateGeometryBuffers(Scene* pScene) { Singleton().GenerateBuffersAndAccelerationStructures(pScene); }
		static const BufferLayout& GetVertexLayout() { return Singleton().m_Layout; }

	protected:
		void GenerateBuffersAndAccelerationStructures(Scene* pScene);
		Entity AddInstance(Scene* pScene, MeshID meshID);
		void BuildCone(MeshData& data, uint32_t resolution = 20u, bool capMesh = false);
		void BuildCube(MeshData& data);
		void BuildCubeSphere(MeshData& data, uint32_t meshResolution = 20u);
		void BuildCylinder(MeshData& data, uint32_t resolution = 20u, uint32_t stackCount = 1, bool capMesh = false);
		void BuildSphere(MeshData& data, uint32_t resolution = 20u);
		void BuildTerrain(MeshData& data, uint32_t resolution = 20u);
	};
}