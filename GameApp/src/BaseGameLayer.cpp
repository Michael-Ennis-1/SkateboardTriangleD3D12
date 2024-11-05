#include "BaseGameLayer.h"
#include "imgui/imgui.h"
#include "Skateboard/Renderer/FrameResources.h"
#include "Skateboard/Scene/SceneBuilder.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Memory/MemoryManager.h"
#include "Skateboard/Renderer/Model/Model.h"
#include "Skateboard/Scene/Components.h""

BaseGameLayer::BaseGameLayer() :
	m_FPSClock(0.f),
	m_FPS(0.f),
	m_FrameTime(0.f),
	m_Camera(.25f * SKTBD_PI, 1280.f / 720.f, .1f, 1000.f, float3(0.f, 0.f, -10.f), float3(0.f, 0.f, -9.f), float3(0.f, 1.f, 0.f))
{
	// Create shared pointer to scene
	p_Scene = std::make_shared<Skateboard::Scene>("Demo Scene");

	// Create cube instance in the world
	m_Cube = Skateboard::SceneBuilder::AddCubeInstance(p_Scene.get());

	// Create new constant buffer, cache ID for later use
	generalPassCBV = Skateboard::MemoryManager::CreateConstantBuffer(L"Pass Constant Buffer", 1, sizeof(PassBuffer));
	lightCBV = Skateboard::MemoryManager::CreateConstantBuffer(L"Light Constant Buffer", 1, sizeof(LightBuffer));
	materialSBV = Skateboard::MemoryManager::CreateStructuredBuffer(L"Material Structured Buffer", p_Scene->GetTotalInstanceCount(), sizeof(Skateboard::MaterialGPUData));

	// Build all instance buffers for the individual objects
	p_Scene->BuildInstanceStructuredBuffer(sizeof(InstanceBuffer));

	// Setup instance buffers for all entities
	ResetInstanceBufferForAllEntities();

	// Pass buffer information to new constant buffer created in memory manager. Passing specifically view and projection matrices from the Camera.
	PassBuffer pass = {};
	pass.ViewMatrix = m_Camera.GetViewMatrix();
	pass.ProjectionMatrix = m_Camera.GetProjectionMatrix();
	pass.ViewMatrixInverse = Skateboard::MatrixInverse(nullptr, pass.ViewMatrix);
	pass.ProjectionMatrixInverse = Skateboard::MatrixInverse(nullptr, pass.ProjectionMatrix);
	Skateboard::MemoryManager::UploadData(generalPassCBV, 0, &pass);


	// Set up graphics pipeline description
	Skateboard::RasterizationPipelineDesc pipelineDesc = {};
	pipelineDesc.SetType(Skateboard::RasterizationPipelineType_Default);
	pipelineDesc.SetInputLayout(Skateboard::SceneBuilder::GetVertexLayout());
	pipelineDesc.SetWireFrame(false);
	pipelineDesc.SetDepthBias(PIPELINE_SETTINGS_DEFAULT_DEPTH_BIAS);
	pipelineDesc.SetVertexShader(L"VertexShader.cso", L"main");
	pipelineDesc.SetPixelShader(L"PixelShader.cso", L"main");

	// Add Camera buffer information
	pipelineDesc.AddConstantBufferView(Skateboard::MemoryManager::GetUploadBuffer(generalPassCBV), 0, 0);


	p_Pipeline.reset(Skateboard::RasterizationPipeline::Create(L"Pipeline", pipelineDesc));

}

BaseGameLayer::~BaseGameLayer()
{
}

void BaseGameLayer::OnResize(int newClientWidth, int newClientHeight)
{
}

bool BaseGameLayer::OnHandleInput(float dt)
{
	return true;
}

bool BaseGameLayer::OnUpdate(float dt)
{
	m_FPSClock += dt;
	if (m_FPSClock > 1.f)
	{
		m_FPSClock = 0.f;
		m_FPS = 1.f / dt;
		m_FrameTime = dt * 1000.f;
	}

	return true;
}

void BaseGameLayer::OnRender()
{
}

void BaseGameLayer::OnImGuiRender()
{
	ImGui::Begin("Info");
	{
		ImGui::Text("FPS: %f", m_FPS);
		ImGui::Text("FrameTime: %f", m_FrameTime);
	}
	ImGui::End();
}

void BaseGameLayer::ResetInstanceBufferForAllEntities()
{
	std::vector<Skateboard::Entity> renderingEntities;
	p_Scene->GetRegistry().each([&](auto entity_id) {
		Skateboard::Entity entity{ entity_id, m_ActiveScene.get() };
		if (entity.HasComponent<Skateboard::StaticMeshInstanceComponent>())
			renderingEntities.emplace_back(std::move(entity));
		});

	// Check if the instance upload buffer needs to be resized
	Skateboard::UploadBuffer* instanceBuffer = p_Scene->GetInstanceStructuredBuffer();
	const Skateboard::UploadBufferDesc& instanceBufferDesc = instanceBuffer->GetDesc();
	if (instanceBufferDesc.ElementCount < static_cast<uint32_t>(renderingEntities.size()))
	{
		p_Scene->BuildInstanceStructuredBuffer(sizeof(InstanceBuffer));
	}

	// Upload the data to the instance buffer
	for (const Skateboard::Entity& entity : renderingEntities)
	{
		const Skateboard::TransformComponent& transformComponent = entity.GetComponent<Skateboard::TransformComponent>();
		InstanceBuffer buff = {
			Skateboard::MatrixScaling(transformComponent.Scale.x, transformComponent.Scale.y, transformComponent.Scale.z) *
			Skateboard::MatrixRotationPitchYawRoll(Skateboard::DegToRad(transformComponent.Rotation.x), Skateboard::DegToRad(transformComponent.Rotation.y), Skateboard::DegToRad(transformComponent.Rotation.z)) *
			Skateboard::MatrixTranslation(transformComponent.Translation.x, transformComponent.Translation.y, transformComponent.Translation.z),
			static_cast<uint32_t>(entity.GetComponent<Skateboard::StaticMeshInstanceComponent>().MeshId),
			entity.GetComponent<Skateboard::StaticMeshInstanceComponent>().MaterialId
		};
		p_Scene->UploadInstanceData(entity, &buff, &buff.WorldMatrix);
	}
}
