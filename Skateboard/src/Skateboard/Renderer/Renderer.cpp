#include "sktbdpch.h"
#include "Renderer.h"
#include "RenderCommand.h"

namespace Skateboard
{
	// Hard coded for now
	RendererAPI_ Renderer::s_API = RendererAPI_::RendererAPI_DirectX12;

	void Renderer::Init()
	{
		RenderCommand::Init();
	}

	void Renderer::Begin()
	{
		RenderCommand::OnBeginRender();
	}

	void Renderer::End()
	{
		RenderCommand::OnEndRender();
	}

	void Renderer::Draw(uint32_t meshID)
	{

	}
}
