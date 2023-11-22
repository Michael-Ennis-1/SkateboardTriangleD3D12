#pragma once
#include "Skateboard/Platform.h"
#include "Timer.h"
#define MINIMUM_WINDOW_SIZE_WIDTH 640
#define MINIMUM_WINDOW_SIZE_HEIGHT 480

struct ImGuiViewport;
namespace Skateboard
{
	class WindowsPlatform : public Platform
	{
	public:
		WindowsPlatform();
		WindowsPlatform(const WindowsPlatform& rhs) = delete;
		WindowsPlatform(WindowsPlatform&& rhs) = delete;
		WindowsPlatform operator= (const WindowsPlatform& rhs) = delete;
		virtual ~WindowsPlatform() final override;

		void Init(const PlatformProperties& props) final override;

		// Update the system app
		virtual bool Update() final override;

		void ResizeBackBuffers(WPARAM wParam, LPARAM lParam);

		HWND GetWindow() const { return m_ActiveWindow; }

		static float GetTimeImpl() { return static_cast<WindowsPlatform&>(Platform::GetPlatform()).m_Timer.DeltaTime(); }

		virtual void InitImGui() final override;
		virtual void BeginImGuiPass() final override;
		virtual void EndImGuiPass() final override;
		virtual void ShutdownImGui() final override;
		//static void ImGui_ImplWin32_CreateWindow_Custom(ImGuiViewport* viewport);
	private:
		// Inistialise the window
		BOOL InitWindowsApp(const PlatformProperties& props);
		void GoFullScreen();
		// Procedure to handle the messages of the window
		// Needs to be static otherwise it cannot be used when initialing the window, since it is a member of the class System
		static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK ImGuiWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		HWND m_MainWindow;
		HWND m_ActiveWindow;
		RECT m_WindowRectSave;
		bool m_FullscreenMode;
		bool m_ApplicationMinimised;
		bool m_ApplicationMaximised;
		bool m_ApplicationResizing;
		Timer m_Timer;
	};
}