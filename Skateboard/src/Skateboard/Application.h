#pragma once
#include "Core.h"
#include "Platform.h"
#include "Renderer/GraphicsContext.h"
#include "LayerStack.h"
#include "ImGui/ImGuiLayer.h"
#include "Renderer/Buffer.h"
#include "Renderer/Pipeline.h"
#include "Scene/SceneBuilder.h"

namespace Skateboard
{
	class Application
	{
	public:
		Application();
		Application(const Application& rhs) = delete;
		Application(Application&& rhs) = delete;
		~Application() = default;
		static Application* Singleton() { return s_Instance; }

		void Run();

		void OnResize(int newClientWidth, int newClientHeight);

		void PushLayer(Layer* layer) { m_LayerStack.PushLayer(layer); }
		void PushOverlay(Layer* overlay) { m_LayerStack.PushOverlay(overlay); }

	protected:
		Platform& m_Platform;
		std::shared_ptr<GraphicsContext> p_GraphicsContext;
		LayerStack m_LayerStack;
		ImGuiLayer* p_ImGuiMasterOverlay;	// Raw pointer as ownership will be transferred to LayerStack

	private:
		static Application* s_Instance;
	};

	Application* CreateApplication(int argc, char** argv);
}


