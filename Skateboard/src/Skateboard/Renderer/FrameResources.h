#pragma once
#define SKTBD_SETTINGS_NUMFRAMERESOURCES 3u

#define SKTBD_SETTINGS_CONSTANT_BUFFER_ALIGN __declspec(align(256u))

namespace Skateboard
{
	class FrameResources
	{
	protected:
		FrameResources() : m_CurrentFrameResourceIndex(0u) {}

	public:
		~FrameResources() = default;

		void NextFrameResource();
		uint64_t GetCurrentFrameResourceIndex() const { return m_CurrentFrameResourceIndex; }

	protected:
		uint64_t m_CurrentFrameResourceIndex{0};
	};
}