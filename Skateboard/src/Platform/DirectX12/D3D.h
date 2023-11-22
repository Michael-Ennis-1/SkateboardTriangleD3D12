#pragma once
#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxcapi.h>
#include <wrl.h>
#include <DirectXMath.h>

#include <string>

#ifndef D3D_CHECK_FAILURE
#define D3D_CHECK_FAILURE(x) if (FAILED(x)) throw std::exception();
#endif

#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment) (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

namespace Skateboard
{
	class D3DGraphicsContext;
	extern D3DGraphicsContext* gD3DContext;

	namespace D3D
	{
		inline D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
		{
			// Helper function to avoid redundancy in code
			// Creates a transition barrier between two states on a given resource
			D3D12_RESOURCE_BARRIER barrier = {};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;						// A transition barrier that indicates a transition of a set of subresources between different usages
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;							// 
			barrier.Transition.pResource = pResource;									// The resources in transition
			barrier.Transition.StateBefore = stateBefore;								// The "before" usages of the SUBresources
			barrier.Transition.StateAfter = stateAfter;									// The "after" usages of the subresources
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;	// The index of the subresource for transition: here we transition all the subresources at the same time
			return barrier;
		}
	}
}