#pragma once
#include <Skateboard.h>
class BaseGameLayer : public Skateboard::Layer
{
public:
	BaseGameLayer();
	virtual ~BaseGameLayer() final override;

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

	std::shared_ptr<Skateboard::Scene> p_Scene{ nullptr };

	Skateboard::Entity m_Cube;

	Skateboard::PerspectiveCamera m_Camera;

	// Buffer ID
	uint32_t generalPassCBV;
};
