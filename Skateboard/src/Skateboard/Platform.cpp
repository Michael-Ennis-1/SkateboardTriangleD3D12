#include "sktbdpch.h"
#include "Platform.h"
#include "Platform/Windows/WindowsPlatform.h"

namespace Skateboard
{
	Platform& Platform::GetPlatform()
	{
#ifdef SKTBD_PLATFORM_WINDOWS
		static WindowsPlatform platform;
		return platform;
#else
#error Platform not yet supported.
#endif
	}

	float Platform::GetTime()
	{
#ifdef SKTBD_PLATFORM_WINDOWS
		return WindowsPlatform::GetTimeImpl();
#else
#error Platform not yet supported.
#endif
	}

}