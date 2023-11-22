#pragma once

namespace Skateboard
{
	class GraphicsContext;

	enum class RendererAPI_
	{
		RendererAPI_None = 0,
		RendererAPI_DirectX12,
		RendererAPI_Vulkan,
		RendererAPI_AGC
	};

	class Renderer
	{
	public:

		static void Init();
		static void Begin();
		static void End();
		static void Draw(uint32_t meshID);

		static RendererAPI_ GetAPI() { return s_API; }

	private:
		static RendererAPI_ s_API;
	};
}