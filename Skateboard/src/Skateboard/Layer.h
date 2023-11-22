#pragma once
namespace Skateboard
{
	class Layer
	{
	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void OnResize(int newClientWidth, int newClientHeight) {}
		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual bool OnHandleInput(float dt) { return true; }
		virtual bool OnUpdate(float dt) { return true; }
		virtual void OnRender() {}
		virtual void OnImGuiRender() {}
		//virtual void OnEvent() {}

#ifdef SKTBD_SHIP
		Layer(const std::string& debugName) {}
		void SetName(const std::string& name) {}
		const sd::string& GetName() const { return ""; }
#else
		Layer(const std::string& debugName) : m_DebugName(debugName) {}
		void SetName(const std::string& debugName) { m_DebugName = debugName; }
		const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
#endif // SKTBD_SHIP
	};
}
