#pragma once

#include <memory>
#include <utility>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <string>

#include "Skateboard/Core.h"
#include "Skateboard/Log.h"

#ifdef SKTBD_PLATFORM_WINDOWS
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif // !WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#include <windowsx.h>
#endif // SKTBD_PLATFORM_WINDOWS
