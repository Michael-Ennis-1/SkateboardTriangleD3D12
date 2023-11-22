#pragma once
#include "Skateboard/Mathematics.h"

#define GRAPHICS_SETTINGS_SWAPCHAINBUFFERS 2
#define GRAPHICS_BACKBUFFER_DEFAULT_CLEAR_COLOUR float4(0.1f, 0.1f, 0.1f, 1.f)
#define GRAPHICS_FRAMEBUFFER_DEFAULT_CLEAR_COLOUR float4(0.690196097f, 0.768627524f, 0.870588303f, 1.f)

namespace Skateboard
{
	class GraphicsContext
	{
	public:
		virtual void Present(void* args=nullptr) = 0;

		virtual void StartDraw(float4 clearColour = GRAPHICS_BACKBUFFER_DEFAULT_CLEAR_COLOUR) = 0;	// Light blue default clear colour
		virtual void EndDraw() = 0;
		virtual void SetRenderTargetToBackBuffer() = 0;

		virtual void Resize(int clientWidth, int clientHeight) {}
		virtual void OnResized() {}
		virtual int GetClientSizeX() const = 0;
		virtual int GetClientSizeY() const = 0;
		virtual float GetClientAspectRatio() const = 0;

		virtual void Reset() {}
		virtual void Flush() {}
		virtual void WaitUntilIdle() {}

		virtual uint64_t GetCurrentFrameResourceIndex() const = 0;

		virtual bool IsRaytracingSupported() const = 0;
		virtual void Clean() = 0;
		static GraphicsContext* Context;
	};
}