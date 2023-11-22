#pragma once
#include <Skateboard.h>
class GameLayer : public Skateboard::Layer
{
public:
	GameLayer();
	virtual ~GameLayer() final override;

	virtual void OnResize(int newClientWidth, int newClientHeight) final override;
	virtual bool OnHandleInput(float dt) final override;
	virtual bool OnUpdate(float dt) final override;
	virtual void OnRender() final override;
	virtual void OnImGuiRender() final override;

private:
	float m_FPSClock;
	float m_FPS;
	float m_FrameTime;
	std::unique_ptr<Skateboard::VertexBuffer> p_VertexBuffer;
	std::unique_ptr<Skateboard::IndexBuffer> p_IndexBuffer;
	std::unique_ptr<Skateboard::RasterizationPipeline> p_Pipeline;

};

