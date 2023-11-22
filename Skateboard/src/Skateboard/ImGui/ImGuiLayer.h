#pragma once
#include "Skateboard/Layer.h"
#include "Platform/DirectX12/D3D.h"

namespace Skateboard
{
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		virtual ~ImGuiLayer() final override = default;

		virtual void OnAttach() final override;
		virtual void OnDetach() final override;
		//virtual void OnUpdate() final override;
		virtual void OnImGuiRender() final override;


		void Begin();
		void End();

	private:
	
	};
}