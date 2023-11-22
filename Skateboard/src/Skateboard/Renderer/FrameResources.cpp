#include "sktbdpch.h"
#include "FrameResources.h"

namespace Skateboard
{
	void FrameResources::NextFrameResource()
	{
		m_CurrentFrameResourceIndex = (m_CurrentFrameResourceIndex + 1) % SKTBD_SETTINGS_NUMFRAMERESOURCES;
	}
}

