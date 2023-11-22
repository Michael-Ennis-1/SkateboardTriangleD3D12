#include "sktbdpch.h"
#include "Log.h"

namespace Skateboard
{
	// Define the static variables (they are declared but not defined within the header)
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_AppLogger;

	void Log::Init()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_CoreLogger = spdlog::stdout_color_mt("Engine");
		s_CoreLogger->set_level(spdlog::level::trace);
		spdlog::sinks::stderr_color_sink_mt* color = static_cast<spdlog::sinks::stderr_color_sink_mt*>(s_CoreLogger->sinks().back().get());
		color->set_color(spdlog::level::level_enum::info, 0x0003);	// first 8 bits not relevant for us - background RGBA - RGBA -> 1 blue and 1 alpha
		s_AppLogger = spdlog::stdout_color_mt("App");
		s_AppLogger->set_level(spdlog::level::trace);
	}
}
