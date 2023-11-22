#pragma once
#include <stdint.h>

namespace Skateboard
{
	// This is compile time dependant -> we do not need to check some settings file to initialise the input like the graphics API would
	// There is no need to overload virtual functions, simply have the correct cpp file compile these definitions instead!
	class Input
	{
	public:
		static void ReadyForNextFrame();
		static void ReleaseAllKeys();

		static bool IsKeyPressed(uint8_t key);
		static bool IsKeyDown(uint8_t key);
		static bool IsKeyReleased(uint8_t key);

		static void SetKeyDown(uint8_t key);
		static void SetKeyUp(uint8_t key);

		static void SetMousePos(int x, int y);
		static void SetMouseActive(bool active);
		static int GetMouseX();
		static int GetMouseY();
		static bool IsMouseActive();

		static void ControlCamera(class PerspectiveCamera& camera, float dt);
	};
}