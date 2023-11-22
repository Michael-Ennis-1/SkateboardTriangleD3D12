#pragma once
// <summary>
// Informs the compiler not to generate a copy constructor and copy assignment operator.
// </summary>
#ifndef DISABLE_COPY
#define DISABLE_COPY(T)\
	    T(const T&) = delete;\
		auto operator=(const T&) noexcept -> T& = delete;
#endif

// <summary>
// Informs the compiler not to generate a move constructor and move assignment operator.
// </summary>
#ifndef DISABLE_MOVE
#define DISABLE_MOVE(T)\
	    T(T&&) = delete;\
		auto operator=(T&&) noexcept -> T& = delete;
#endif

#ifndef DISBALE_DEFAULT
#define DISABLE_DEFAULT(T)\
		T() = delete;
#endif

#ifndef GENERATE_DEFAULT_CLASS
#define GENERATE_DEFAULT_CLASS(T)\
	T() = default;\
	T(const T&) = default;\
	T(T&&) noexcept = default;\
	auto operator=(const T&) -> T& = default;\
	auto operator=(T&&) noexcept -> T& = default;
#endif
	

#ifndef DISABLE_COPY_AND_MOVE
#define DISABLE_COPY_AND_MOVE(T) DISABLE_COPY(T) DISABLE_MOVE(T)
#endif

#ifdef SKTBD_PLATFORM_WINDOWS
	// Core platform specifics here
#else
	#error Skateboard only supports Windows for now!
#endif

#ifdef SKTBD_DEBUG
	#define SKTBD_ASSERT(x, ...) { if(!(x)) { SKTBD_APP_ERROR("Assertion failed: {0}", __VA_ARGS__); __debugbreak(); } }
	#define SKTBD_CORE_ASSERT(x, ...) { if(!(x)) { SKTBD_CORE_ERROR("Assertion failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
	#define SKTBD_ASSERT(x, ...)
	#define SKTBD_CORE_ASSERT(x, ...)
#endif

// Uncomment this definition to log the CPU idles (warning: may flood console as currently the CPU always
// waits on the GPU -> not enough CPU tasks!
//#define SKTBD_LOG_CPU_IDLE


