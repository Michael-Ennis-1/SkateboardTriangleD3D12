#include "sktbdpch.h"
#include "Scene.h"
#include "Components.h"
#include "Entity.h"
#include "ScriptableEntity.h"

#include "Skateboard/Camera.h"
#include "Skateboard/Scene/SceneBuilder.h"
#include "Skateboard/Renderer/FrameResources.h"
#include "Skateboard/Memory/MemoryManager.h"
#include "Skateboard/Renderer/RenderCommand.h"
#include "Skateboard/Renderer/MeshletEngine/MeshEngine.h"

namespace Skateboard
{
	Scene::Scene(const std::string& name) : m_InstanceBufferID(UINT32_MAX)
	{

		m_SceneRenderer = std::make_unique<SceneRenderer>();
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { Registry.create(), this };
		entity.AddComponent<TransformComponent>();
		TagComponent& tag = entity.AddComponent<TagComponent>(name);
		tag.tag = name.empty() ? "Entity" : name;
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		Registry.destroy(entity);
	}

	template<> void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<> void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		//TODO: Set the camera's aspect ration on component added.
		//

	}

	template<> void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<> void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}

	template<> void Scene::OnComponentAdded<MaterialComponent>(Entity entity, MaterialComponent& component)
	{
	}

	template<> void Scene::OnComponentAdded<StaticMeshInstanceComponent>(Entity entity, StaticMeshInstanceComponent& component)
	{
		// When the static mesh component is added, make sure to add an instance in the system
		component.InstanceId = m_InstanceData.AddInstance(component.MeshId);
	}

	template<> void Scene::OnComponentAdded<StaticMeshletComponent>(Entity entity, StaticMeshletComponent& component)
	{
	}

	template<> void Scene::OnComponentAdded<MeshletRendererComponent>(Entity entity, MeshletRendererComponent& component)
	{
	}

	template<> void Scene::OnComponentRemoved<TransformComponent>(Entity entity, TransformComponent& removedComponent)
	{
	}

	template<> void Scene::OnComponentRemoved<CameraComponent>(Entity entity, CameraComponent& removedComponent)
	{
	}

	template<> void Scene::OnComponentRemoved<TagComponent>(Entity entity, TagComponent& removedComponent)
	{
	}

	template<> void Scene::OnComponentRemoved<NativeScriptComponent>(Entity entity, NativeScriptComponent& removedComponent)
	{
	}

	template<> void Scene::OnComponentRemoved<MaterialComponent>(Entity entity, MaterialComponent& removedComponent)
	{
	}

	template<> void Scene::OnComponentRemoved<StaticMeshInstanceComponent>(Entity entity, StaticMeshInstanceComponent& removedComponent)
	{
		// When the static mesh component is removed, we need to also remove it from the rendering list
		m_InstanceData.RemoveInstance(removedComponent.MeshId, removedComponent.InstanceId);
	}

	template<> void Scene::OnComponentRemoved<StaticMeshletComponent>(Entity entity, StaticMeshletComponent& removedComponent)
	{
	}

	template<> void Scene::OnComponentRemoved<MeshletRendererComponent>(Entity entity, MeshletRendererComponent& removedComponent)
	{
	}


	void Scene::OnUpdate(float time)
	{
		/** process scripts and entities */


		
		{// Cull mesh instance transforms
			auto group = Registry.view<TransformComponent, StaticMeshInstanceComponent>();
			for (auto entity : group)
			{
				auto [transform, meshInstance] = group.get<TransformComponent, StaticMeshInstanceComponent>(entity);

				// When the transform changed we need to upload the need data to this entity instance
				InstanceBuffer buff = {
					MatrixScaling(transform.Scale.x, transform.Scale.y, transform.Scale.z) *
						MatrixRotationPitchYawRoll(Skateboard::DegToRad(transform.Rotation.x), Skateboard::DegToRad(transform.Rotation.y), Skateboard::DegToRad(transform.Rotation.z)) *
						MatrixTranslation(transform.Translation.x, transform.Translation.y, transform.Translation.z),
					static_cast<uint32_t>(meshInstance.MeshId),
					meshInstance.MaterialId

				};
				/*UploadInstanceData(entity, &buff, &buff.WorldMatrix);*/

				const uint32_t bufferIndex = m_InstanceData.GetInstanceBufferIndex(meshInstance.MeshId, meshInstance.InstanceId);
				MemoryManager::UploadData(m_InstanceBufferID, bufferIndex, &buff);

				// Cull the top level acceleration structure (if present) to account for the new data
				/*if (m_TLAS.get())
					m_TLAS->PerformUpdate(bufferIndex, transform.GetTransform());*/

			}
		}

		

		{// Cull meshlet instance transforms
			auto group = Registry.view<StaticMeshletComponent, TransformComponent>();

			for (auto entity : group)
			{
				auto [staticMeshletComponent, transformComponent] = group.get<StaticMeshletComponent, TransformComponent>(entity);

				const auto pModel = m_SceneRenderer->m_MeshletBank->Get(staticMeshletComponent.MeshTag);
				if (pModel != nullptr)
				{
					pModel->SetTransform(transformComponent.GetTransform());
				}

			}

		}

	}

	void Scene::OnRender(float time)
	{

		{// for each static meshlet render 
			auto group = Registry.view<StaticMeshletComponent, MeshletRendererComponent>();

			for (auto entity : group)
			{
				auto [staticMeshletComponent, rendererComponent] = group.get<StaticMeshletComponent, MeshletRendererComponent>(entity);

				const auto pModel = m_SceneRenderer->m_MeshletBank->Get(staticMeshletComponent.MeshTag);
				if(pModel)
				{
					RenderCommand::DispatchMeshletModel(m_SceneRenderer->m_MeshletPipelines[staticMeshletComponent.MeshTag].get(), pModel);
				}

			}
		}
	}

	void Scene::BuildInstanceStructuredBuffer(uint32_t instanceBufferSize)
	{
		const uint32_t totalInstanceCount = m_InstanceData.GetTotalInstanceCount();
		if (m_InstanceBufferID != UINT32_MAX)
		{
			SKTBD_CORE_INFO("Resetting instance buffer for {0} entities", totalInstanceCount);
			SKTBD_CORE_CRITICAL("You will get a GPU crash now as the pipeline does not know of the new address of the upload buffer. We need to fix the pipeline system.");
			SKTBD_CORE_ASSERT(false, "");
			MemoryManager::ResetUploadBuffer(m_InstanceBufferID, totalInstanceCount, instanceBufferSize);
			return;
		}

		m_InstanceBufferID = MemoryManager::CreateStructuredBuffer(L"Instance Structured Buffer", totalInstanceCount, instanceBufferSize);
	}

	void Scene::UploadInstanceData(Entity entity, void* pData, float4x4* pNewTransform)
	{
		// Find the location of this entity in the buffer
		const StaticMeshInstanceComponent meshInstance = Registry.get<Skateboard::StaticMeshInstanceComponent>(entity);
		const uint32_t bufferIndex = m_InstanceData.GetInstanceBufferIndex(meshInstance.MeshId, meshInstance.InstanceId);
		MemoryManager::UploadData(m_InstanceBufferID, bufferIndex, pData);

		// Cull the top level acceleration structure (if present) to account for the new data
		if(m_TLAS.get())
			m_TLAS->PerformUpdate(bufferIndex, *pNewTransform);
	}

	void Scene::AddMeshletPipeline(const std::wstring_view& name, std::unique_ptr<MeshletPipeline>& pipeline)
	{
		if(m_SceneRenderer->m_MeshletPipelines.contains(name))
		{
			SKTBD_CORE_WARN("The pipeline already exists in the map!");
			return;
		}

		m_SceneRenderer->m_MeshletPipelines.emplace(name, std::move(pipeline));

	}

	void Scene::RemoveMeshletPipeline(const std::wstring_view& name)
	{
		if (!m_SceneRenderer->m_MeshletPipelines.contains(name))
		{
			SKTBD_CORE_WARN("The pipeline does not exist in the map!");
			return;
		}

		m_SceneRenderer->m_MeshletPipelines.erase(name);
	}

	MeshletPipeline* Scene::GetMeshletPipeline(const std::wstring_view& name) const 
	{
		if (!m_SceneRenderer->m_MeshletPipelines.contains(name))
		{
			SKTBD_CORE_WARN("The pipeline does not exist in the map!");
			return nullptr;
		}

		return m_SceneRenderer->m_MeshletPipelines.at(name).get();
	}

	void Scene::AddPipeline(const std::wstring_view& name, std::unique_ptr<RasterizationPipeline>& pipeline)
	{
		if (m_SceneRenderer->m_RasterizationPipelines.contains(name))
		{
			SKTBD_CORE_WARN("The pipeline already exists in the map!");
			return;
		}

		m_SceneRenderer->m_RasterizationPipelines.emplace(name, std::move(pipeline));
	}

	void Scene::RemovePipeline(const std::wstring_view& name)
	{
		if (!m_SceneRenderer->m_RasterizationPipelines.contains(name))
		{
			SKTBD_CORE_WARN("The pipeline does not exist in the map!");
			return;
		}

		m_SceneRenderer->m_RasterizationPipelines.erase(name);
	}

	RasterizationPipeline* Scene::GetPipeline(const std::wstring_view& name)
	{
		if (!m_SceneRenderer->m_RasterizationPipelines.contains(name))
		{
			SKTBD_CORE_WARN("The pipeline does not exist in the map!");
			return nullptr;
		}

		return m_SceneRenderer->m_RasterizationPipelines.at(name).get();
	}

	MeshletModel* Scene::AddMeshletModel(const std::wstring_view& name, std::unique_ptr<MeshletModel>& model)
	{
		if(!m_SceneRenderer->m_MeshletBank->Contains(name))
		{
			return m_SceneRenderer->m_MeshletBank->AddMeshletModel(name, model);
		}

		SKTBD_CORE_WARN("Meshlet model already exists!");
	}

	void Scene::RemoveMeshletModel(const std::wstring_view& name)
	{
		if (!m_SceneRenderer->m_MeshletBank->Contains(name))
		{
			SKTBD_CORE_WARN("Meshlet model does not exist!");
			return;
		}

		m_SceneRenderer->m_MeshletBank->ReleaseModel(name);
	}

	MeshletModel* Scene::GetMeshletModel(const std::wstring_view& name) const
	{
		if (!m_SceneRenderer->m_MeshletBank->Contains(name))
		{
			SKTBD_CORE_WARN("Meshlet model does not exist!");
			return nullptr;
		}

		return m_SceneRenderer->m_MeshletBank->Get(name);
	}

	Model* Scene::AddMesh(const std::wstring_view& name, std::unique_ptr<MeshletModel>& model)
	{
		return nullptr;
	}

	void Scene::RemoveMesh(const std::wstring_view& name)
	{
	}

	Model* Scene::GetMesh(const std::wstring_view& name) const
	{
		return m_SceneRenderer->m_MeshBank->Get(name);
	}

	UploadBuffer* Scene::GetInstanceStructuredBuffer()
	{
		return MemoryManager::GetUploadBuffer(m_InstanceBufferID);
	}

	void Scene::ChangeInstanceMeshID(StaticMeshInstanceComponent& instanceComponent, MeshID newMeshID)
	{
		// Returns the new instance index
		m_InstanceData.RemoveInstance(instanceComponent.MeshId, instanceComponent.InstanceId);
		instanceComponent.InstanceId = m_InstanceData.AddInstance(newMeshID);
		instanceComponent.MeshId = newMeshID;
	}

	void Scene::BindGeometryBuffers() const
	{
		m_VertexBuffer->Bind();
		m_IndexBuffer->Bind();
	}

	uint32_t Scene::GetMeshStartVertexLocation(MeshID meshID) const
	{
		SKTBD_CORE_ASSERT(meshID < static_cast<uint32_t>(v_StartVertexLocations.size()), "Invalid mesh ID. Out of bound access.");
		return v_StartVertexLocations[meshID];
	}

	uint32_t Scene::GetMeshStartIndexLocation(MeshID meshID) const
	{
		SKTBD_CORE_ASSERT(meshID < static_cast<uint32_t>(v_StartIndexLocations.size()), "Invalid mesh ID. Out of bound access.");
		return v_StartIndexLocations[meshID];
	}

	uint32_t Scene::GetMeshIndexCount(MeshID meshID) const
	{
		SKTBD_CORE_ASSERT(meshID < static_cast<uint32_t>(v_IndexCounts.size()), "Invalid mesh ID. Out of bound access.");
		return v_IndexCounts[meshID];
	}

	uint32_t Scene::GetTotalInstanceCount() const
	{
		return m_InstanceData.GetTotalInstanceCount();
	}

	uint32_t Scene::GetMeshInstanceCount(MeshID id) const
	{
		return m_InstanceData.GetInstanceCount(id);
	}

	uint32_t Scene::GetMeshStartInstanceLocation(MeshID id) const
	{
		return m_InstanceData.GetInstanceBufferIndex(id, 0);
	}
}
