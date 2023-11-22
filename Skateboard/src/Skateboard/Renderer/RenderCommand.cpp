#include "sktbdpch.h"
#include "RenderCommand.h"

#include "Platform/DirectX12/D3DGraphicsContext.h"
#include "Platform/DirectX12/Api/D3DRenderingApi.h"

namespace Skateboard
{
	RenderingApi* RenderCommand::RenderingApi{ nullptr };

	RenderCommand::~RenderCommand()
	{
		delete RenderingApi;
		RenderingApi = nullptr;
	}

	void RenderCommand::Init()
	{
		// TODO: Need to make this switch based on selected api.
		// Note: Hard coded for now.
		RenderingApi = new D3DRenderingApi();
	}

}
