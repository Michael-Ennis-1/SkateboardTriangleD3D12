#include "sktbdpch.h"
#include "SceneRenderer.h"
#include "Skateboard/Renderer/MeshletEngine/MeshEngine.h"
#include "Skateboard/Renderer/Pipeline.h"

namespace Skateboard
{
	SceneRenderer::SceneRenderer()
	{
		m_MeshletBank = std::make_unique<MeshletBank>();
		m_MeshBank = std::make_unique<MeshBank>();
	}

	SceneRenderer::~SceneRenderer()
	{
		Clean();
	}

	void SceneRenderer::Clean()
	{

		for(auto& mPipelines : m_MeshletPipelines)
		{
			mPipelines.second->Release();
		}

	}
}
