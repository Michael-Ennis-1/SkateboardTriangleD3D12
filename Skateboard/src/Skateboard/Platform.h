#pragma once
#include "Skateboard/Renderer/GraphicsContext.h"
#include "sktbdpch.h"
namespace Skateboard
{
	struct PlatformProperties
	{
		std::wstring Title = L"Skateboard Engine";
		uint32_t BackBufferWidth = 1280u, BackBufferHeight = 720u;
	};

	class Platform
	{
	public:
		virtual ~Platform() {}
		// look at https://refactoring.guru/design-patterns/singleton
		static Platform& GetPlatform();
		static float GetTime();

		virtual void Init(const PlatformProperties& props = PlatformProperties()) = 0;
		virtual bool Update() = 0;

		virtual void InitImGui() = 0;
		virtual void BeginImGuiPass() = 0;
		virtual void EndImGuiPass() = 0;
		virtual void ShutdownImGui() = 0;

		const std::shared_ptr<GraphicsContext>& GetGraphicsContext() const { return p_GraphicsContext; }

	protected:
		std::shared_ptr<GraphicsContext> p_GraphicsContext;
	};
}