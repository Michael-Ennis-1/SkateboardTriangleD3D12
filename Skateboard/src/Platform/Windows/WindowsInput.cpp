#include "sktbdpch.h"
#include "Skateboard/Input.h"
#include "Skateboard/Camera.h"
#include "WindowsPlatform.h"
#include "Platform/DirectX12/D3DGraphicsContext.h"

namespace Skateboard
{
	struct WindowsInputData
	{
		struct MouseData {
			int x = 0, y = 0;	// Integers because they can be negative on multiple monitors setups
			bool active = false;
		} Mouse;
		bool CurrentFrameKeys[UINT8_MAX];		// See https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes for key codes
		bool PreviousFrameKeys[UINT8_MAX];
	} s_WindowsInputData;

	void Input::ReadyForNextFrame()
	{
		// Copy the frame previous keys (really bad, but really simple. Can you think of smth better? hint: pointer)
		memcpy(s_WindowsInputData.PreviousFrameKeys, s_WindowsInputData.CurrentFrameKeys, sizeof(s_WindowsInputData.CurrentFrameKeys));
	}

	void Input::SetKeyDown(uint8_t keyCode)
	{
		s_WindowsInputData.CurrentFrameKeys[keyCode] = true;
	}

	void Input::SetKeyUp(uint8_t keyCode)
	{
		s_WindowsInputData.CurrentFrameKeys[keyCode] = false;
	}

	void Input::SetMousePos(int x, int y)
	{
		s_WindowsInputData.Mouse.x = x;
		s_WindowsInputData.Mouse.y = y;
	}

	void Input::SetMouseActive(bool active)
	{
		s_WindowsInputData.Mouse.active = active;
	}

	void Input::ReleaseAllKeys()
	{
		memset(s_WindowsInputData.CurrentFrameKeys, NULL, sizeof(s_WindowsInputData.CurrentFrameKeys));
	}

	bool Input::IsKeyPressed(uint8_t keyCode)
	{
		return !s_WindowsInputData.PreviousFrameKeys[keyCode] && s_WindowsInputData.CurrentFrameKeys[keyCode];
	}

	bool Input::IsKeyDown(uint8_t keyCode)
	{
		return s_WindowsInputData.CurrentFrameKeys[keyCode];
	}

	bool Input::IsKeyReleased(uint8_t keyCode)
	{
		return s_WindowsInputData.PreviousFrameKeys[keyCode] && !s_WindowsInputData.CurrentFrameKeys[keyCode];
	}

	int Input::GetMouseX()
	{
		return s_WindowsInputData.Mouse.x;
	}

	int Input::GetMouseY()
	{
		return s_WindowsInputData.Mouse.y;
	}

	bool Input::IsMouseActive()
	{
		return s_WindowsInputData.Mouse.active;
	}

	void Input::ControlCamera(PerspectiveCamera& camera, float dt)
	{
		bool hasMoved = camera.HasMoved();
		float3 position = camera.GetPosition();
		float3 rotation = camera.GetRotation();
		float moveSpeed = camera.GetMoveSpeed();
		const float sensitivity = camera.GetSensitivity();

		// Get window specifics
		WindowsPlatform& windowsPlatform = static_cast<WindowsPlatform&>(Platform::GetPlatform());
		D3DGraphicsContext* context = static_cast<D3DGraphicsContext*>(windowsPlatform.GetGraphicsContext().get());
		const int32_t clientWidth = context->GetClientSizeX();
		const int32_t clientHeight = context->GetClientSizeY();

		auto CenterCursor = [&]() {
			// Now warp the cursor back to the center of the window
			POINT cursorPos = { clientWidth / 2, clientHeight / 2 };
			ClientToScreen(windowsPlatform.GetWindow(), &cursorPos);       // Converts the position from th client window to the screen position
			SetCursorPos(cursorPos.x, cursorPos.y);             // Sets the cursor on a given position on the screen (not the window!)
		};

		hasMoved = false;

		// Handle the keyboard inputs to move the camera
		if (IsMouseActive())
		{
			const float speedMultiplier = IsKeyDown(LOBYTE(VK_SHIFT)) ? 10.f : IsKeyDown(LOBYTE(VK_CONTROL)) ? .2f : 1.f;
			moveSpeed *= speedMultiplier;

			if (IsKeyDown('W'))
			{
				float Yangle = DegToRad(rotation.y);
				position.x += sinf(Yangle) * moveSpeed * dt;    // Moving using the idea of a ZX trigonometric unit circle
				position.z += cosf(Yangle) * moveSpeed * dt;    // sinf(0) = 0 -> no movement in X, cosf(0) = 1 -> movement in Z
				hasMoved = true;
			}
			if (IsKeyDown('S'))
			{
				float Yangle = DegToRad(rotation.y);
				position.x -= sinf(Yangle) * moveSpeed * dt;
				position.z -= cosf(Yangle) * moveSpeed * dt;
				hasMoved = true;
			}
			if (IsKeyDown('A'))
			{
				float Yangle = DegToRad(rotation.y);
				position.z += sinf(Yangle) * moveSpeed * dt;
				position.x -= cosf(Yangle) * moveSpeed * dt;
				hasMoved = true;
			}
			if (IsKeyDown('D'))
			{
				float Yangle = DegToRad(rotation.y);
				position.z -= sinf(Yangle) * moveSpeed * dt;
				position.x += cosf(Yangle) * moveSpeed * dt;
				hasMoved = true;
			}
			if (IsKeyDown('Q'))
			{
				position.y -= moveSpeed * dt;   // Lock the up movement to the world axis, do not consider the up vector (makes more sense)
				hasMoved = true;
			}
			if (IsKeyDown('E'))
			{
				position.y += moveSpeed * dt;
				hasMoved = true;
			}
			if (IsKeyDown(VK_UP))
			{
				rotation.x -= sensitivity * 5.f * dt; // Using the keyboard, it needs to be about 5 times faster than the mouse sensibility to be coherent
				if (rotation.x < -CAMERA_PITCH_MAX) rotation.x = -CAMERA_PITCH_MAX;
				hasMoved = true;
			}
			if (IsKeyDown(VK_DOWN))
			{
				rotation.x += sensitivity * 5.f * dt;
				if (rotation.x > CAMERA_PITCH_MAX) rotation.x = CAMERA_PITCH_MAX;
				hasMoved = true;
			}
			if (IsKeyDown(VK_LEFT))
			{
				rotation.y -= sensitivity * 5.f * dt;
				rotation.y += (rotation.y < 0.f) * 360.f;
				hasMoved = true;
			}
			if (IsKeyDown(VK_RIGHT))
			{
				rotation.y += sensitivity * 5.f * dt;
				rotation.y -= (rotation.y > 360.f) * 360.f;
				hasMoved = true;
			}

			// When the mouse is currently active, move the mouse
			const int mouseX = GetMouseX(), mouseY = GetMouseY();
			const float deltaX = static_cast<float>(mouseX - (clientWidth / 2));
			const float deltaY = static_cast<float>(mouseY - (clientHeight / 2));
			rotation.y += deltaX * sensitivity * dt;
			rotation.x += deltaY * sensitivity * dt;

			// Now warp the cursor back to the center of the window
			CenterCursor();
			hasMoved = true;
		}
		if (IsKeyPressed(VK_RBUTTON))   // Mouse right click
		{
			SetMouseActive(true);
			CenterCursor();
			SetMousePos(clientWidth / 2, clientHeight / 2);		// Make sure the input system registered this new pos to avoid an unintended drag
			ShowCursor(false);
		}
		// IsMouseActive prevents a undesired push in ShowCursor stack when dragging from outside the camera viewport into it
		if (IsMouseActive() && IsKeyReleased(VK_RBUTTON))
		{
			SetMouseActive(false);
			ShowCursor(true);
		}

		// Finally, update the camera matrix
		if (hasMoved)
		{
			camera.SetMoved(true);
			camera.SetPosition(position);
			camera.SetRotation(rotation);
			camera.UpdateViewMatrix();
		}
	}
}