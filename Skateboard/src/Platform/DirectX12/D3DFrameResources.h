#pragma once
#include "Skateboard/Renderer/FrameResources.h"
#include "D3D.h"

namespace Skateboard
{
	class D3DFrameResources : public FrameResources
	{
	public:
		D3DFrameResources() : a_Fences{ 0u } {};

		void Init();

		ID3D12CommandAllocator* GetCurrrentCommandAllocator() const { return a_CommandAllocators[m_CurrentFrameResourceIndex].Get(); }

		uint64_t GetCurrentFence() const { return a_Fences[m_CurrentFrameResourceIndex]; }
		void SetCurrentFence(uint64_t value) { a_Fences[m_CurrentFrameResourceIndex] = value; }

	private:
		std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, SKTBD_SETTINGS_NUMFRAMERESOURCES> a_CommandAllocators;
		std::array<uint64_t, SKTBD_SETTINGS_NUMFRAMERESOURCES> a_Fences;
	};
}