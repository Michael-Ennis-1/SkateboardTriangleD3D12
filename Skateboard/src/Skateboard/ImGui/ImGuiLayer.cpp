#include "sktbdpch.h"
#include "Platform/Windows/WindowsPlatform.h"
#include "ImGuiLayer.h"

namespace Skateboard
{
	ImGuiLayer::ImGuiLayer() :
		Layer("ImGuiLayer")
	{
	}

	void ImGuiLayer::OnAttach()
	{
		Platform::GetPlatform().InitImGui();
	}

	void ImGuiLayer::OnDetach()
	{
		Platform::GetPlatform().ShutdownImGui();
	}

	void ImGuiLayer::OnImGuiRender()
	{
	}

	void ImGuiLayer::Begin()
	{
		Platform::GetPlatform().BeginImGuiPass();
	}

	void ImGuiLayer::End()
	{
		Platform::GetPlatform().EndImGuiPass();
	}
}
