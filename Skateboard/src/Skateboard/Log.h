#pragma once
#include "Core.h"
// This ignores all warnings raised inside External headers
#pragma warning(push)
#pragma warning(disable : 4616)
#pragma warning(disable : 6285)
#pragma warning(disable : 26437)
#pragma warning(disable : 26450)
#pragma warning(disable : 26451)
#pragma warning(disable : 26495)
#pragma warning(disable : 26498)
#pragma warning(disable : 26800)
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#pragma warning(pop)

namespace Skateboard
{
	class Log
	{
	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetAppLogger() { return s_AppLogger; }


	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_AppLogger;
	};
}

#if SKTBD_SHIP
	// Core log macros
	#define SKTBD_CORE_CRITICAL(...)
	#define SKTBD_CORE_ERROR(...)
	#define SKTBD_CORE_WARN(...)
	#define SKTBD_CORE_INFO(...)
	#define SKTBD_CORE_TRACE(...)
	
	// Client log macros
	#define SKTBD_APP_CRITICAL(...)
	#define SKTBD_APP_ERROR(...)
	#define SKTBD_APP_WARN(...)
	#define SKTBD_APP_INFO(...)
	#define SKTBD_APP_TRACE(...)
#else
	// Core log macros
	#define SKTBD_CORE_CRITICAL(...)		{ ::Skateboard::Log::GetCoreLogger()->critical(__VA_ARGS__); throw std::exception(); }
	#define SKTBD_CORE_ERROR(...)		::Skateboard::Log::GetCoreLogger()->error(__VA_ARGS__)
	#define SKTBD_CORE_WARN(...)			::Skateboard::Log::GetCoreLogger()->warn(__VA_ARGS__)
	#define SKTBD_CORE_INFO(...)			::Skateboard::Log::GetCoreLogger()->info(__VA_ARGS__)
	#define SKTBD_CORE_TRACE(...)		::Skateboard::Log::GetCoreLogger()->trace(__VA_ARGS__)
	
	// Client log macros
	#define SKTBD_APP_CRITICAL(...)		{ ::Skateboard::Log::GetAppLogger()->critical(__VA_ARGS__); throw std::exception(); }
	#define SKTBD_APP_ERROR(...)			::Skateboard::Log::GetAppLogger()->error(__VA_ARGS__)
	#define SKTBD_APP_WARN(...)			::Skateboard::Log::GetAppLogger()->warn(__VA_ARGS__)
	#define SKTBD_APP_INFO(...)			::Skateboard::Log::GetAppLogger()->info(__VA_ARGS__)
	#define SKTBD_APP_TRACE(...)			::Skateboard::Log::GetAppLogger()->trace(__VA_ARGS__)
#endif