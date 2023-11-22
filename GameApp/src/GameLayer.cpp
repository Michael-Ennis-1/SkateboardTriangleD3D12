#include "GameLayer.h"
#include "imgui/imgui.h"
#include "Skateboard/Renderer/FrameResources.h"
#include "Skateboard/Scene/SceneBuilder.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Memory/MemoryManager.h"




GameLayer::GameLayer() :
	m_FPSClock(0.f),
	m_FPS(0.f),
	m_FrameTime(0.f)
{

	Skateboard::BufferLayout vertexLayout = {
	{"POSITION", Skateboard::ShaderDataType_::Float3},
	{"COLOR", Skateboard::ShaderDataType_::Float3}
	};
	float vertices[] = {
		-1.0f, -1.0f, 0.0f,			1.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,			0.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,			0.0f, 0.0f, 1.0f
	};
	p_VertexBuffer.reset(Skateboard::VertexBuffer::Create(L"Triangle Vertices", vertices, sizeof(vertices) , vertexLayout));
	uint32_t indices[] = {
		0u, 1u, 2u
	};
	p_IndexBuffer.reset(Skateboard::IndexBuffer::Create(L"Triangle Indices", indices, _countof(indices)));
	Skateboard::RasterizationPipelineDesc pipelineDesc = {};
	pipelineDesc.SetType(Skateboard::RasterizationPipelineType_Default);
	pipelineDesc.SetDepthBias(PIPELINE_SETTINGS_DEFAULT_DEPTH_BIAS);
	pipelineDesc.SetInputLayout(vertexLayout);
	pipelineDesc.SetWireFrame(false);


	pipelineDesc.SetVertexShader(L"VertexShader.cso", L"main");
	pipelineDesc.SetPixelShader(L"PixelShader.cso", L"main");
	p_Pipeline.reset(Skateboard::RasterizationPipeline::Create(L"Pipeline", pipelineDesc));
	
}

GameLayer::~GameLayer()
{

	


}

void GameLayer::OnResize(int newClientWidth, int newClientHeight)
{
	// This is called when the windows' window resizes, not the imgui viewports!
}

bool GameLayer::OnHandleInput(float dt)
{
	return true;
}

bool GameLayer::OnUpdate(float dt)
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

void GameLayer::OnRender()
{

	
	Skateboard::RenderCommand::DrawIndexed(p_Pipeline.get(), p_VertexBuffer.get(), p_IndexBuffer.get());

}

void GameLayer::OnImGuiRender()
{
	ImGui::Begin("Info");
	{
		ImGui::Text("FPS: %f", m_FPS);
		ImGui::Text("FrameTime: %f", m_FrameTime);
	}
	ImGui::End();
}
