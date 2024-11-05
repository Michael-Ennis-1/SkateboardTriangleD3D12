#include "BaseGameLayer.h"
#include "imgui/imgui.h"
#include "Skateboard/Renderer/FrameResources.h"
#include "Skateboard/Scene/SceneBuilder.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Memory/MemoryManager.h"

BaseGameLayer::BaseGameLayer() :
	m_FPSClock(0.f),
	m_FPS(0.f),
	m_FrameTime(0.f),
	m_Camera(.25f * SKTBD_PI, 1280.f / 720.f, .1f, 1000.f, float3(0.f, 0.f, -10.f), float3(0.f, 0.f, -9.f), float3(0.f, 1.f, 0.f))
{
	// Create new constant buffer, cache ID for later use
	generalPassCBV = Skateboard::MemoryManager::CreateConstantBuffer(L"Pass Constant Buffer", 1, sizeof(PassBuffer));

	// Pass buffer information to new constant buffer created in memory manager. Passing specifically view and projection matrices.
	PassBuffer pass = {};
	pass.ViewMatrix = m_Camera.GetViewMatrix();
	pass.ProjectionMatrix = m_Camera.GetProjectionMatrix();
	pass.ViewMatrixInverse = Skateboard::MatrixInverse(nullptr, pass.ViewMatrix);
	pass.ProjectionMatrixInverse = Skateboard::MatrixInverse(nullptr, pass.ProjectionMatrix);
	Skateboard::MemoryManager::UploadData(generalPassCBV, 0, &pass);

	//Skateboard::BufferLayout vertexLayout = {
	//{"POSITION", Skateboard::ShaderDataType_::Float3},
	//{"COLOR", Skateboard::ShaderDataType_::Float3}
	//};

	Skateboard::RasterizationPipelineDesc pipelineDesc = {};
	pipelineDesc.SetType(Skateboard::RasterizationPipelineType_Default);
	pipelineDesc.SetInputLayout(Skateboard::SceneBuilder::GetVertexLayout());
	pipelineDesc.SetWireFrame(false);
	pipelineDesc.SetDepthBias(PIPELINE_SETTINGS_DEFAULT_DEPTH_BIAS);
	pipelineDesc.SetVertexShader(L"VertexShader.cso", L"main");
	pipelineDesc.SetPixelShader(L"PixelShader.cso", L"main");

	// Add Camera buffer information
	pipelineDesc.AddConstantBufferView()

	p_Pipeline.reset(Skateboard::RasterizationPipeline::Create(L"Pipeline", pipelineDesc));

	p_Scene = std::make_shared<Skateboard::Scene>("Demo Scene");

	//m_Cube - 
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
