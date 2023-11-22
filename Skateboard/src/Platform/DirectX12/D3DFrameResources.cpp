#include "sktbdpch.h"
#include "D3DFrameResources.h"
#include "D3DGraphicsContext.h"

namespace Skateboard
{
	void D3DFrameResources::Init()
	{
		for (uint32_t i = 0u; i < SKTBD_SETTINGS_NUMFRAMERESOURCES; ++i)
		{
			D3D_CHECK_FAILURE(gD3DContext->Device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(a_CommandAllocators[i].ReleaseAndGetAddressOf())));
		}
	}
}