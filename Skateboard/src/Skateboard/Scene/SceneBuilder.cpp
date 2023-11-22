#include <sktbdpch.h>
#include "SceneBuilder.h"
#include "Skateboard/Renderer/Renderer.h"
#include "Skateboard/Renderer/GraphicsContext.h"
#include "Skateboard/Scene/Components.h"

namespace Skateboard
{
	Entity SceneBuilder::AddRaytracingProceduralGeometryInstance(Scene* pScene, const RaytracingAABBDesc& boundingBox)
	{
		char buff[255];
		size_t convertedChars;
		wcstombs_s(&convertedChars, buff, boundingBox.Name.c_str(), boundingBox.Name.size());
		SKTBD_CORE_ASSERT(convertedChars == (boundingBox.Name.size() + 1u), "Failed to convert the complete name of the geometry.");
		pScene->v_RaytracingAABBs.push_back(boundingBox);
		return Singleton().AddInstance(pScene, pScene->AddMeshInstance(buff));
	}

	void SceneBuilder::GenerateBuffersAndAccelerationStructures(Scene* pScene)
	{
		// Sanity checks
		SKTBD_CORE_ASSERT(pScene->m_InstanceData.GetTotalInstanceCount() > 0u, "Cannot create empty buffers. Have you created geometry instances before calling this function?");

		// Generate and retrieve data for the primitives
		std::vector<VertexType> vertices;
		std::vector<uint32_t> indices;
		std::vector<MeshData> vMeshData;
		std::vector<AABB> vProceduralAABBs;

		const uint32_t meshCount = pScene->GetTotalMeshCount();
		pScene->v_StartVertexLocations.resize(meshCount);
		pScene->v_StartIndexLocations.resize(meshCount);
		pScene->v_IndexCounts.resize(meshCount);

		uint32_t startVertexLocation = 0u;
		uint32_t startIndexLocation = 0u;
		for (MeshID meshID = 0u; meshID < meshCount; ++meshID)
		{
			MeshData data;

			// Generate one piece of geometry for each type in a single vertex & index buffer
			// Very bad logic but since this is computed on load time it's ok
			const std::string& meshName = pScene->GetMeshTag(meshID);
			if (!meshName.compare("Cone"))
				BuildCone(data, 20u, true);
			else if (!meshName.compare("Cube"))
				BuildCube(data);
			else if (!meshName.compare("CubeSphere"))
				BuildCubeSphere(data);
			else if (!meshName.compare("Cylinder"))
				BuildCylinder(data);
			else if (!meshName.compare("Sphere"))
				BuildSphere(data);
			else if (!meshName.compare("Terrain"))
				BuildTerrain(data);
			else continue;

			vertices.insert(vertices.end(), data.Vertices.begin(), data.Vertices.end());
			indices.insert(indices.end(), data.Indices.begin(), data.Indices.end());

			// Store the vertex, index start location and index counts for each geometry so that they can be retrieved when drawing
			pScene->v_StartVertexLocations[meshID] = startVertexLocation;
			pScene->v_StartIndexLocations[meshID] = startIndexLocation;
			pScene->v_IndexCounts[meshID] = static_cast<uint32_t>(data.Indices.size());

			startVertexLocation += static_cast<uint32_t>(data.Vertices.size());
			startIndexLocation += static_cast<uint32_t>(data.Indices.size());
			vMeshData.emplace_back(std::move(data));
		}

		// Determine the total geometry size
		const uint32_t vertexBufferSize = sizeof(VertexType) * static_cast<uint32_t>(vertices.size());
		const uint32_t indexBufferSize = sizeof(uint32_t) * static_cast<uint32_t>(indices.size());

		// Create the vertex and index buffers
		pScene->m_VertexBuffer.reset(VertexBuffer::Create(L"Main Vertex Buffer", vertices.data(), vertexBufferSize, m_Layout));
		pScene->m_IndexBuffer.reset(IndexBuffer::Create(L"Main Index Buffer", indices.data(), static_cast<uint32_t>(indices.size())));

		// Now create the acceleration structures if supported
		if (GraphicsContext::Context->IsRaytracingSupported())
		{
			// Create default buffers to hold the vertices and indices offsets for each geometry in the buffer
			pScene->m_VertexOffsetsBuffer.reset(DefaultBuffer::Create(
				L"Main Vertex Offset Buffer",
				DefaultBufferDesc::Init(
					pScene->v_StartVertexLocations.data(),
					static_cast<uint32_t>(pScene->v_StartVertexLocations.size()),
					sizeof(uint32_t)
				)
			));
			pScene->m_IndexOffsetsBuffer.reset(DefaultBuffer::Create(
				L"Main Vertex Offset Buffer",
				DefaultBufferDesc::Init(
					pScene->v_StartIndexLocations.data(),
					static_cast<uint32_t>(pScene->v_StartIndexLocations.size()),
					sizeof(uint32_t)
				)
			));
			// Now create a buffer with all the AABBs of procedural primitives
			if (!pScene->v_RaytracingAABBs.empty())
			{
				for (const RaytracingAABBDesc& aabb : pScene->v_RaytracingAABBs)
					vProceduralAABBs.push_back(aabb.BoundingBox);
				pScene->m_ProceduralGeometryAABBBuffer.reset(DefaultBuffer::Create(
					L"Main Vertex Offset Buffer",
					DefaultBufferDesc::Init(
						vProceduralAABBs.data(),
						static_cast<uint32_t>(vProceduralAABBs.size()),
						sizeof(AABB)
					)
				));
			}

			// Create a unique bottom level acceleration structure for each piece of geometry
			for (size_t i = 0u; i < vMeshData.size(); ++i)
			{
				const MeshData& data = vMeshData[i];

				const std::wstring Name = L"Bottom Level Acceleration Structure - TRIANGLES - " + data.Name;

				BottomLevelAccelerationStructureDesc desc = {};
				desc.Type = GeometryType_Triangles;
				desc.Triangles.pVertexBuffer = pScene->m_VertexBuffer.get();
				desc.Triangles.StartVertexLocation = pScene->v_StartVertexLocations[i];
				desc.Triangles.VertexCount = static_cast<uint32_t>(data.Vertices.size());
				desc.Triangles.pIndexBuffer = pScene->m_IndexBuffer.get();
				desc.Triangles.StartIndexLocation = pScene->v_StartIndexLocations[i];
				desc.Triangles.IndexCount = pScene->v_IndexCounts[i];
				pScene->v_BLAS.emplace_back(BottomLevelAccelerationStructure::Create(Name, desc));
			}

			for (size_t i = 0u; i < pScene->v_RaytracingAABBs.size(); ++i)
			{
				const RaytracingAABBDesc& aabb = pScene->v_RaytracingAABBs[i];
				const std::wstring Name = L"Bottom Level Acceleration Structure - PROCEDURAL -" + aabb.Name;

				BottomLevelAccelerationStructureDesc desc = {};
				desc.Type = aabb.Type;
				desc.Procedurals.AABBCount = 1;
				desc.Procedurals.AABB.Offset = i;
				desc.Procedurals.AABB.pProceduralGeometryAABBBuffer = pScene->m_ProceduralGeometryAABBBuffer.get();
				pScene->v_BLAS.emplace_back(BottomLevelAccelerationStructure::Create(Name, desc));
			}

			// Then create a unique top level acceleration structure with the BLAS
			// Make sure to get the data of the rendering entities, such that the top acceleration structure accounts for correct transforms and IDs
			TopLevelAccelerationStructureDesc topDesc = {};
			topDesc.vBLAS = &pScene->v_BLAS;
			pScene->Registry.view<TransformComponent, StaticMeshInstanceComponent>().each([&pScene, &topDesc](entt::entity entity, const TransformComponent& transform, const StaticMeshInstanceComponent& meshInstance) {
				topDesc.vTransforms.emplace_back(transform.GetTransform());
				topDesc.vInstanceIDs.emplace_back(pScene->m_InstanceData.GetInstanceBufferIndex(meshInstance.MeshId, meshInstance.InstanceId));
				topDesc.vMeshIDs.emplace_back(static_cast<uint32_t>(meshInstance.MeshId));
				topDesc.vBufferIndices.emplace_back(pScene->m_InstanceData.GetInstanceBufferIndex(meshInstance.MeshId, meshInstance.InstanceId));
			});
			pScene->m_TLAS.reset(TopLevelAccelerationStructure::Create(L"Top Level Acceleration Structure", topDesc));
		}
	}

	Entity SceneBuilder::AddInstance(Scene* pScene, MeshID meshID)
	{
		// The scene creates an entity with a transform and a nametag by default
		Entity returnEnt = pScene->CreateEntity(pScene->m_InstanceData.GenerateNameTag(meshID));

		// So, the only thing left to do is to add an instance component on it
		returnEnt.AddComponent<StaticMeshInstanceComponent>(meshID);
		return returnEnt;
	}

	void SceneBuilder::BuildCone(MeshData& data, uint32_t resolution, bool capMesh)
	{
		// Generate the vertices and indices for a cone following a triangle list primitive type
		if (resolution < 3u)
			resolution = 3u;

		// Create writable index and vertex buffers
		const uint32_t vertexCount = resolution + 2u + capMesh; // A base of resolution + 1 (to roll back) + 1 (top vertex) + 1 (middle vertex)
		const uint32_t indexCount = (capMesh ? 2u : 1u) * 3u * resolution;
		data.Vertices.resize(vertexCount, {});
		data.Indices.resize(indexCount, 0u);
		data.Name = L"Cone";

		// Create the vertices
		const float deltaAngle = 2.f * SKTBD_PI / resolution;
		const float deltaTexCoord = 1.f / (resolution + 1u);
		float currentTexCoord = 0.f;
		const vector roationQuaternion = QuaternionRotationAxis(VectorSet(0.f, 1.f, 0.f, 0.f), deltaAngle);
		vector currentVertexPos = VectorSet(0.f, 0.f, -1.f, 0.f);
		vector currentTangent = VectorSet(1.f, 0.f, 0.f, 0.f);
		for (uint32_t i = 0u; i < resolution + 1u; ++i)
		{
			// Round up the last vertex of the circle to the first vertex, to avoid float imprecision errors
			if (i == resolution)
			{
				data.Vertices[i] = data.Vertices[0];
				data.Vertices[i].texCoord = float2(0.f, 1.f);
			}
			else
			{
				Vector3Store(&data.Vertices[i].position, currentVertexPos);
				Vector3Store(&data.Vertices[i].normal, Vector3Normalise(currentVertexPos));
				Vector3Store(&data.Vertices[i].tangent, Vector3Normalise(currentTangent));
				Vector3Store(&data.Vertices[i].bitangent, Vector3Cross(Vector3Normalise(currentVertexPos), Vector3Normalise(currentTangent)));
				data.Vertices[i].texCoord = float2(1.f - currentTexCoord, 1.f);
			}
			currentTexCoord += deltaTexCoord;
			currentVertexPos = std::move(Vector3Rotate(std::move(currentVertexPos), roationQuaternion));
			currentTangent = std::move(Vector3Rotate(std::move(currentTangent), roationQuaternion));
		}
		// The forlast vertex is the top of the cone
		data.Vertices[resolution + 1u] = { float3(0.f, 1.f, 0.f), float2(.5f, 0.f), float3(0.f, 1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f) };
		// The last vertex is the middle of the cone
		if (capMesh) data.Vertices[resolution + 2u] = { float3(0.f, 0.f, 0.f), float2(.5f, 0.f), float3(0.f, -1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, -1.f) };

		// Create the indices
		uint32_t currentIndex = 0u;
		for (uint32_t i = 0u; i < resolution; ++i, currentIndex += 3u)
		{
			data.Indices[currentIndex] = i;
			data.Indices[currentIndex + 1u] = i + 1u;
			data.Indices[currentIndex + 2u] = resolution + 1u;
		}
		for (uint32_t i = 0u; i < resolution && capMesh; ++i, currentIndex += 3u)
		{
			data.Indices[currentIndex] = resolution + 2u;
			data.Indices[currentIndex + 1u] = i + 1u;
			data.Indices[currentIndex + 2u] = i;
		}
	}

	void SceneBuilder::BuildCube(MeshData& data)
	{
		// Generate the vertices and indices for a cube
		//const uint32_t vertexCount = 24u;
		//const uint32_t indexCount = 36u;

		// Declare the vertex data and the size of the vertex buffer to be created
		data.Vertices.assign({
			// Top
			{ float3(-1.f, 1.f, -1.f), float2(0.f, 1.f), float3(0.f, 1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f) },
			{ float3(1.f, 1.f, -1.f), float2(1.f, 1.f), float3(0.f, 1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, 1.f, 1.f), float2(1.f, 0.f), float3(0.f, 1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, 1.f, 1.f), float2(0.f, 0.f), float3(0.f, 1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },

			// Bottom
			{ float3(1.f, -1.f, 1.f), float2(0.f, 1.f), float3(0.f, -1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, -1.f, 1.f), float2(1.f, 1.f), float3(0.f, -1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, -1.f, -1.f), float2(1.f, 0.f), float3(0.f, -1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, -1.f, -1.f), float2(0.f, 0.f), float3(0.f, -1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },

			// Front
			{ float3(-1.f, -1.f, -1.f), float2(0.f, 1.f), float3(0.f, 0.f, -1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, -1.f, -1.f), float2(1.f, 1.f), float3(0.f, 0.f, -1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, 1.f, -1.f), float2(1.f, 0.f), float3(0.f, 0.f, -1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, 1.f, -1.f), float2(0.f, 0.f), float3(0.f, 0.f, -1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },

			// Back
			{ float3(1.f, -1.f, 1.f), float2(0.f, 1.f), float3(0.f, 0.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, -1.f, 1.f), float2(1.f, 1.f), float3(0.f, 0.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, 1.f, 1.f), float2(1.f, 0.f), float3(0.f, 0.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, 1.f, 1.f), float2(0.f, 0.f), float3(0.f, 0.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },

			// Left
			{ float3(-1.f, -1.f, 1.f), float2(0.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, -1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, -1.f, -1.f), float2(1.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, -1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, 1.f, -1.f), float2(1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, -1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, 1.f, 1.f), float2(0.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, -1.f), float3(0.f, 0.f, 1.f)  },

			// Right
			{ float3(1.f, -1.f, -1.f), float2(0.f, 1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, -1.f, 1.f), float2(1.f, 1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, 1.f, 1.f), float2(1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, 1.f, -1.f), float2(0.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f), float3(0.f, 0.f, 1.f)  }
			}
		);

		// Declare the indices
		data.Indices.assign({
			// Top face
			2, 1, 0,
			3, 2, 0,

			// Bottom face
			4, 5, 6,
			4, 6, 7,

			// Front face
			10, 9, 8,
			11, 10, 8,

			// Back face
			14, 13, 12,
			15, 14, 12,

			// Left face
			18, 17, 16,
			19, 18, 16,

			// Right face
			22, 21, 20,
			23, 22, 20
			}
		);

		data.Name = L"Cube";
	}

	void SceneBuilder::BuildCubeSphere(MeshData& data, uint32_t meshResolution)
	{
		// Generate the vertices and indices for a cube sphere
		if (meshResolution == 0u)
			meshResolution = 1u;

		// Create a writable vertex buffer for all the vertices
		const uint32_t vertexCount = 6u * (meshResolution + 1u) * (meshResolution + 1u); // a face is of resolution * resolution vertices
		const uint32_t indexCount = 6u * 6u * meshResolution * meshResolution;
		data.Vertices.resize(vertexCount);
		data.Indices.resize(indexCount);
		data.Name = L"CubeSphere";

		// Define the radius of the sphere
		const float radius = 1.f;	// 1.f since the Normalise() operation on vectors gives a length of 1 (don't change this)

		// Create the vertices and indices
		uint32_t currentVertex = 0u;
		float fRes = static_cast<float>(meshResolution);	//fRes -> float Resolution
		uint32_t currentIndex = 0u;
		uint32_t verticesPerFace = vertexCount / 6u;

		// Top, bottom, front, back, left, right
		for (uint8_t faceNum = 0u; faceNum < 6u; ++faceNum)
		{
			// Generate the vertices for a face
			// Each plane is made of dimension (m_MeshResolution + 1) * (m_MeshResolution + 1) vertices
			// x and y identify the coordinates of a vertex in this plane
			for (uint32_t y = 0u; y <= meshResolution; ++y)
			{
				for (uint32_t x = 0u; x <= meshResolution; ++x, ++currentVertex)
				{
					// Get the position of the vertex in the [-1, 1] plane
					// x and y divided by the resolution go from [0, 1], so double them [0, 2] and substract 1 to get [-1, 1]
					// Store the results as floats
					float fX = static_cast<float>(x) * 2.f / fRes - 1.f;
					float fY = static_cast<float>(y) * 2.f / fRes - 1.f;

					// This is how you can rll 6 faces into a loop, each face needs a different input for each argument
					// For instance, faceNum 0u is the top face
					//	- The X position should be fX
					//	- The Y position should be positive radius
					//	- The Z position should be fY
					// This result with the plane being on the XZ plane at a distance 'radius' from the origin
					// The bottom face would be similar, but the Y position should be below the origin (-radius)
					// Repeat the logic for front, back, left and right faces
					float arg1 = (faceNum < 4u) * fX + (faceNum == 4u) * radius + (faceNum == 5u) * -radius;
					float arg2 = (faceNum == 0u) * radius + (faceNum == 1u) * -radius + (faceNum > 1u) * fY;
					float arg3 = (faceNum < 2u) * fY + (faceNum == 2u) * radius + (faceNum == 3u) * -radius + (faceNum > 3u) * fX;

					// The VECTOR object is the target position of the vertex if this was a flat plane
					// The position of the vertex must be in the direction of the targeted position, but of length (radius) 1, thus normalised
					// The normal is therefore equals to the position as it is a unit vector from the center
					vector target = Vector3Normalise(VectorSet(arg1, arg2, arg3, 0.f));
					Vector3Store(&data.Vertices[currentVertex].position, target);
					Vector3Store(&data.Vertices[currentVertex].normal, target);

					// Calculate the tangent vector
					// Since this is a unit sphere, the cartesian equation of the sphere is x^2 + y^2 + z^2 = 1
					// Thus, the parametric equation becomes r(t, p) = cos(t)*sin(p)*i + sin(t)*sin(p)*j + cos(p)*k, 0 <= t <= 2*PI && 0 <= p <= PI
					// The tangent is then stated with dr/dt. The bitagent would be dr/dp
					// dr/dt = -sin(t)*sin(p)*i + cos(t)*sin(p)*j + 0
					// t (theta) is obtained using atan2 (https://en.wikipedia.org/wiki/Atan2) which in essence calculates the angle between a point P(x,y) and the x-axis
					// p (phi) is obtained using trigonometry: cos(p) = adjacent / hypothenuse. The hypothenuse is the radius of the unit sphere, thus p = acos(adjacent)
					float theta = atan2f(data.Vertices[currentVertex].position.z, data.Vertices[currentVertex].position.x); // P(Z,X) makes an point on the horizontal plane on the sphere, since Y is up
					if (theta < 0) theta += 2.f * SKTBD_PI;
					float phi = acosf(data.Vertices[currentVertex].position.y);
					/*data.Vertices[currentVertex].tangent.x = -sinf(theta) * sinf(phi);	// In essence, these are the calculations. But it needs to be normalised to ensure it is a unit vector
					data.Vertices[currentVertex].tangent.y = 0.f;
					data.Vertices[currentVertex].tangent.z = cosf(theta) * sinf(phi);*/
					vector tangent = Vector3Normalise(VectorSet(-sinf(theta) * sinf(phi), 0.f, cosf(theta) * sinf(phi), 0.f));
					Vector3Store(&data.Vertices[currentVertex].tangent, tangent);
					Vector3Store(&data.Vertices[currentVertex].bitangent, Vector3Cross(target, tangent));

					// The texture coordinates should match the coordinates on the plane in the range [0, 1]
					bool inverseCoord = faceNum == 1u || faceNum == 2u || faceNum == 5u;
					arg1 = static_cast<float>(x) / fRes;
					arg2 = 1.f - static_cast<float>(y) / fRes;	// V goes from top (0) to bottom (1), but the generation from top to bottom
					data.Vertices[currentVertex].texCoord = float2(inverseCoord ? 1.f - arg1 : arg1, arg2);
				}
			}

			// Generate the indices for a face
			// Determine whether the face bust be rendered in a clockwise order
			// For instance, the top face is clock wise, but the bottom face should be anti-clockwise (otherwise it would render inside the sphere)
			// This simply depends on how the vertices have been generated
			bool clockWiseIndexing = faceNum == 0u || faceNum == 3u || faceNum == 4u;
			for (uint32_t y = 0u; y < meshResolution; ++y)
			{
				for (uint32_t x = 0u; x < meshResolution; ++x, currentIndex += 6u)
				{
					data.Indices[currentIndex + 0u] = (x + 0u) + (y + 0u) * (meshResolution + 1u) + faceNum * verticesPerFace;
					data.Indices[currentIndex + 1u] = (x + 1u) + (y + clockWiseIndexing * 1u) * (meshResolution + 1u) + faceNum * verticesPerFace;
					data.Indices[currentIndex + 2u] = (x + 1u) + (y + !clockWiseIndexing * 1u) * (meshResolution + 1u) + faceNum * verticesPerFace;
					data.Indices[currentIndex + 3u] = (x + 0u) + (y + 0u) * (meshResolution + 1u) + faceNum * verticesPerFace;
					data.Indices[currentIndex + 4u] = (x + !clockWiseIndexing * 1u) + (y + 1u) * (meshResolution + 1u) + faceNum * verticesPerFace;
					data.Indices[currentIndex + 5u] = (x + clockWiseIndexing * 1u) + (y + 1u) * (meshResolution + 1u) + faceNum * verticesPerFace;
				}
			}
		}
	}

	void SceneBuilder::BuildCylinder(MeshData& data, uint32_t resolution, uint32_t stackCount, bool capMesh)
	{
		// Capping yet unsupported.
		SKTBD_CORE_ASSERT(!capMesh, "Cylinder cap mesh not yet supported! (FIXME)");
		// FIXME: Reflections on cylinders are broken! Normals seem correct however, not sure what is going wrong..

		// Generate the vertices and indices for a cylinder
		if (resolution < 3u)
			resolution = 3u;

		// Create a writable vertex buffer for all the vertices
		const uint32_t vertexCount = (stackCount + 1u) * (resolution + 1u); // A base of resolution + 1 (to roll back) times n stacks
		const uint32_t indexCount = 2u * 3u * resolution * stackCount;
		data.Vertices.resize(vertexCount);
		data.Indices.resize(indexCount);
		data.Name = L"Cylinder";

		// Create the vertices
		const float deltaTexCoord = 2.f / resolution;
		float currentTexCoord = 0.f;
		const vector roationQuaternion = QuaternionRotationAxis(VectorSet(0.f, 1.f, 0.f, 0.f), 2.f * SKTBD_PI / resolution);
		vector currentVertexPos = VectorSet(0.f, -1.f, -1.f, 0.f);
		uint32_t currentVertex = 0u;
		vector currentTangent = VectorSet(1.f, 0.f, 0.f, 0.f);
		for (float height = -1.f; height < 1.01f; height += 2.f / static_cast<float>(stackCount))
		{
			currentVertexPos = VectorSet(0.f, height, -1.f, 0.f);
			for (uint32_t i = 0u; i < resolution + 1u; ++i, ++currentVertex)
			{
				// Round up the last vertex of the circle to the first vertex, to avoid float imprecision errors
				if (i == resolution)
				{
					data.Vertices[currentVertex] = data.Vertices[currentVertex - resolution];
					data.Vertices[currentVertex].texCoord = float2(-1.f, 1.f - (height + 1.f) / 2.f);
				}
				else
				{
					Vector3Store(&data.Vertices[currentVertex].position, currentVertexPos);
					vector normal = Vector3Normalise(VectorSet(data.Vertices[currentVertex].position.x, 0, data.Vertices[currentVertex].position.z, 0.f));
					vector tangent = Vector3Normalise(currentTangent);
					Vector3Store(&data.Vertices[currentVertex].normal, normal);
					Vector3Store(&data.Vertices[currentVertex].tangent, tangent);
					Vector3Store(&data.Vertices[currentVertex].bitangent, Vector3Cross(normal, tangent));
					data.Vertices[currentVertex].texCoord = float2(1.f - currentTexCoord, 1.f - (height + 1.f) / 2.f);
				}
				currentTexCoord += deltaTexCoord;
				currentVertexPos = std::move(Vector3Rotate(std::move(currentVertexPos), roationQuaternion));
				currentTangent = std::move(Vector3Rotate(std::move(currentTangent), roationQuaternion));
			}
			currentTexCoord = 0.f;
			currentTangent = VectorSet(1.f, 0.f, 0.f, 0.f);
		}

		// Create the indices
		uint32_t currentIndex = 0u;
		for (uint32_t h = 0u; h < stackCount; ++h)
		{
			for (uint32_t i = 0; i < resolution; ++i, currentIndex += 6u)
			{
				data.Indices[currentIndex + 0u] = h * (resolution + 1u) + i;
				data.Indices[currentIndex + 1u] = h * (resolution + 1u) + i + 1;
				data.Indices[currentIndex + 2u] = h * (resolution + 1u) + (i + 1) + (resolution + 1u);
				data.Indices[currentIndex + 3u] = h * (resolution + 1u) + i;
				data.Indices[currentIndex + 4u] = h * (resolution + 1u) + (i + 1) + (resolution + 1u);
				data.Indices[currentIndex + 5u] = h * (resolution + 1u) + i + (resolution + 1u);
			}
		}
	}

	void SceneBuilder::BuildSphere(MeshData& data, uint32_t resolution)
	{
		// Generate the vertices and indices for a traditional sphere
		// The least resolution contains 3 layers of both lats and longs to make a sphere
		if (resolution < 3u)
			resolution = 3u;

		// Create a writable vertex buffer for all the vertices
		const uint32_t vertexCount = 2u + (resolution - 2u) * (resolution + 1u); // Top and bottom vertex + (m_MeshResolution - 2u) discs of (m_MeshResolution + 1u) vertices
		const uint32_t indexCount = 6u * resolution + 6u * (resolution - 3u) * resolution;
		data.Vertices.resize(vertexCount);
		data.Indices.resize(indexCount);
		data.Name = L"Sphere";

		// Build the vertices
		// Using longitude and latitude to create the vertices of the sphere
		const float deltaTheta = 2.f * SKTBD_PI / resolution;
		const float deltaGamma = SKTBD_PI / (resolution - 1u);	// Resolution includes the bottom vertex, remove it
		const float deltaTexCoordX = 1.f / resolution;	// There will be an extra vertex to go around the circle properly
		const float deltaTexCoordY = 1.f / (resolution - 1u);
		const vector latitudeRoationQuaternion = QuaternionRotationAxis(VectorSet(0.f, 1.f, 0.f, 0.f), deltaTheta);
		// First vertex is the top of the sphere
		data.Vertices[0].position = data.Vertices[0].normal = float3(0.f, 1.f, 0.f);
		data.Vertices[0].texCoord = float2(.5f, 0.f);
		// Last vertex is the bottom of the sphere
		data.Vertices[vertexCount - 1u].position = data.Vertices[vertexCount - 1u].normal = float3(0.f, -1.f, 0.f);
		data.Vertices[vertexCount - 1u].texCoord = float2(.5f, 1.f);
		// Other vertices are a series of discs
		float2 currentVertexTex = float2(1.f, deltaTexCoordY);
		for (uint32_t longitude = 0u; longitude < resolution - 2u; ++longitude)	// Exclude top and bottom vertices
		{
			const vector longitudeRoationQuaternion = QuaternionRotationAxis(VectorSet(1.f, 0.f, 0.f, 0.f), static_cast<float>(longitude + 1u) * deltaGamma);
			vector currentVertexPos = std::move(Vector3Rotate(std::move(VectorSet(0.f, 1.f, 0.f, 0.f)), longitudeRoationQuaternion));
			for (uint32_t latitude = 0u; latitude < resolution + 1u; ++latitude)
			{
				const uint32_t vertexIndex = latitude + longitude * (resolution + 1u) + 1u;	// Add 1 to consider the top vertex

				// Round up the last vertex of the circle to the first vertex, to avoid float imprecision errors
				if (latitude == resolution)
				{
					data.Vertices[vertexIndex] = data.Vertices[vertexIndex - resolution];
					data.Vertices[vertexIndex].texCoord = float2(0.f, (longitude + 1u) * deltaTexCoordY);
				}
				else
				{
					Vector3Store(&data.Vertices[vertexIndex].position, currentVertexPos);
					data.Vertices[vertexIndex].normal = data.Vertices[vertexIndex].position;
					float theta = atan2f(data.Vertices[vertexIndex].position.z, data.Vertices[vertexIndex].position.x); // See cube sphere to understand how this works
					if (theta < 0) theta += 2.f * SKTBD_PI;
					float phi = acosf(data.Vertices[vertexIndex].position.y);
					vector tangent = Vector3Normalise(VectorSet(-sinf(theta) * sinf(phi), 0.f, cosf(theta) * sinf(phi), 0.f));
					Vector3Store(&data.Vertices[vertexIndex].tangent, tangent);
					Vector3Store(&data.Vertices[vertexIndex].bitangent, Vector3Cross(currentVertexPos, tangent));
					data.Vertices[vertexIndex].texCoord = currentVertexTex;
				}
				currentVertexTex.x -= deltaTexCoordX;
				currentVertexPos = std::move(Vector3Rotate(std::move(currentVertexPos), latitudeRoationQuaternion));
			}
			currentVertexTex.x = 1.f;
			currentVertexTex.y += deltaTexCoordY;
		}

		// Create the indices
		// Index the top cone
		uint32_t currentIndex = 0u;
		for (uint32_t i = 0u; i < resolution; ++i, currentIndex += 3u)
		{
			data.Indices[currentIndex + 0u] = i + 1u;
			data.Indices[currentIndex + 1u] = i + 2u;
			data.Indices[currentIndex + 2u] = 0u;
		}
		// Index the disc quads
		for (uint32_t j = 1u; j < resolution - 2u; ++j)
		{
			for (uint32_t i = 0u; i < resolution; ++i, currentIndex += 6u)
			{
				data.Indices[currentIndex + 0u] = i + 2u + (j - 1u) * (resolution + 1u);
				data.Indices[currentIndex + 1u] = i + 1u + (j - 1u) * (resolution + 1u);
				data.Indices[currentIndex + 2u] = i + 1u + (j - 0u) * (resolution + 1u);
				data.Indices[currentIndex + 3u] = i + 2u + (j - 1u) * (resolution + 1u);
				data.Indices[currentIndex + 4u] = i + 1u + (j - 0u) * (resolution + 1u);
				data.Indices[currentIndex + 5u] = i + 2u + (j - 0u) * (resolution + 1u);
			}
		}
		// Index the bottom cone
		for (uint32_t i = 0u; i < resolution; ++i, currentIndex += 3u)
		{
			data.Indices[currentIndex + 0u] = i + 1u + (resolution - 3u) * (resolution + 1u);
			data.Indices[currentIndex + 1u] = vertexCount - 1u;
			data.Indices[currentIndex + 2u] = i + 2u + (resolution - 3u) * (resolution + 1u);
		}
	}

	void SceneBuilder::BuildTerrain(MeshData& data, uint32_t resolution)
	{
		// Generate the vertices and indices for a traditional terrain
		if (resolution < 1u) resolution = 1u;

		// Create a writable vertex buffer for all the vertices
		const uint32_t vertexCount = (resolution + 1u) * (resolution + 1u);
		const uint32_t indexCount = 6u * (resolution) * (resolution);
		data.Vertices.resize(vertexCount);
		data.Indices.resize(indexCount);
		data.Name = L"Terrain";

		//Create vertex buffer and the index buffer
		const float fWidht = static_cast<float>(resolution);
		const float fHeight = static_cast<float>(resolution);
		for (uint32_t z = 0u; z < resolution + 1u; ++z)
		{
			for (uint32_t x = 0u; x < resolution + 1u; ++x)
			{
				const float fX = static_cast<float>(x) / fWidht;
				const float fZ = static_cast<float>(z) / fHeight;
				const uint32_t idx = x + z * (resolution + 1u);
				data.Vertices[idx].position = float3(fX * 2.f - 1.f, 0.f, fZ * 2.f - 1.f);	// x2 -1 to convert from [0,1] to [-1,1]
				data.Vertices[idx].texCoord = float2(fX, 1.f - fZ);
				data.Vertices[idx].normal = float3(0.f, 1.f, 0.f);
				data.Vertices[idx].tangent = float3(1.f, 0.f, 0.f);
				data.Vertices[idx].bitangent = float3(0.f, 0.f, 1.f);
			}
		}

		uint32_t currentIndex = 0u;
		for (uint32_t z = 0u; z < resolution; ++z)
		{
			for (uint32_t x = 0u; x < resolution; ++x, currentIndex += 6u)
			{
				const uint32_t idx = x + z * (resolution + 1u);
				data.Indices[currentIndex + 0u] = idx;
				data.Indices[currentIndex + 1u] = idx + 1u + (resolution + 1u);
				data.Indices[currentIndex + 2u] = idx + 1u;
				data.Indices[currentIndex + 3u] = idx;
				data.Indices[currentIndex + 4u] = idx + 0u + (resolution + 1u);
				data.Indices[currentIndex + 5u] = idx + 1u + (resolution + 1u);
			}
		}
	}
}