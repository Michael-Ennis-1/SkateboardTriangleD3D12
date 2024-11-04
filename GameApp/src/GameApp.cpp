#include <Skateboard.h>
#include "Skateboard/EntryPoint.h"
#include "GameLayer.h"
#include "BaseGameLayer.h"

class GameApp : public Skateboard::Application
{
public:
	GameApp()
	{
		// Initialise some layers
		PushLayer(new BaseGameLayer());

		// Execute any GPU initialisation required from these layers
		SKTBD_APP_INFO("Flushing GPU for post-initialisation");
		p_GraphicsContext->Flush();
	}
};

Skateboard::Application* Skateboard::CreateApplication(int argc, char** argv)
{
	return new GameApp();
}