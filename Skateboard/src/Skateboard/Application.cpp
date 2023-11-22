#include "sktbdpch.h"
#include "Application.h"

#include "Renderer/Renderer.h"

#include "Platform/Windows/WindowsPlatform.h"
#include "Platform/DirectX12/D3DGraphicsContext.h"
#include "Skateboard/Memory/MemoryManager.h"

namespace Skateboard
{
	Application* Application::s_Instance = nullptr;

	Application::Application() :
		m_Platform(Platform::GetPlatform()),
		p_GraphicsContext(m_Platform.GetGraphicsContext()),
		p_ImGuiMasterOverlay(new ImGuiLayer())	// Ownership is transferred to the layer stack, no need to worry about delete
	{
		PushOverlay(p_ImGuiMasterOverlay);
		p_GraphicsContext->Reset();

		// Initialise the renderer
		Renderer::Init();
		//TODO: Need a more graceful solution, for now should work.

		SKTBD_CORE_ASSERT(!s_Instance, "Cannot create two application instances. Consider using layers to create other windows!");
		s_Instance = this;
	}

	void Application::Run()
	{
		bool running = true;
		while (running)
		{
			// Release some CPU consumption if the application is not used
			/*if (m_ApplicationPaused)
			{
				Sleep(100);
				continue;
			}*/

			// Update the platform (could be window messages, controller inputs, ...)
			// We will avoid uncessary rendering by quitting immediately if the context is destroyed
			if (!m_Platform.Update())
				break;

			// Get delta time from the platform (the procedure of getting time may differ!)
			float deltaTime = Platform::GetTime();

			// Generic game loop
			for (Layer* layer : m_LayerStack)
				running &= layer->OnHandleInput(deltaTime);
			for (Layer* layer : m_LayerStack)
				running &= layer->OnUpdate(deltaTime);
			MemoryManager::Update();

			// Prepare internal engine for rendering scene.
			Skateboard::Renderer::Begin();

			for (Layer* layer : m_LayerStack)
				layer->OnRender();

			//// Prepare internal renderer for presenting final image to the registered window.
			Skateboard::Renderer::End();

			p_ImGuiMasterOverlay->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGuiRender();
			p_ImGuiMasterOverlay->End();

			// finally present the current back buffer to the registered window.
			p_GraphicsContext->Present();
		}

		// When exitting the app, ensure all GPU work has concluded
		SKTBD_CORE_INFO("Exiting App, GPU idle expected..");
		MemoryManager::Clean();
		p_GraphicsContext->WaitUntilIdle();
		p_GraphicsContext->Clean();
	}

	void Application::OnResize(int newClientWidth, int newClientHeight)
	{
		for (Layer* layer : m_LayerStack)
			layer->OnResize(newClientWidth, newClientHeight);
	}
}