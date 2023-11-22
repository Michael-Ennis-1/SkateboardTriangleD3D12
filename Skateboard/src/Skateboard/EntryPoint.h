#pragma once
// The goal of this file is to have the engine handle the entry point for the client
// This is mainly because of the different platforms
// We can ignore the errors (if any)

#ifdef SKTBD_PLATFORM_WINDOWS

extern Skateboard::Application* Skateboard::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	Skateboard::Log::Init();
	Skateboard::Platform::GetPlatform().Init();

	std::unique_ptr<Skateboard::Application> app(Skateboard::CreateApplication(argc, argv));
	app->Run();
	return 0;
}

#else
#error Skateboard only supports windows for now!
#endif