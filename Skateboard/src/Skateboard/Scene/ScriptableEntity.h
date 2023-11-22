#pragma once
#include "Entity.h"

namespace Skateboard
{
	class ScriptableEntity
	{
	public:

		friend class Scene;

	public:
		virtual ~ScriptableEntity() = default;

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}
	protected:
		virtual void OnUpdate(float time){}
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
	private:
		Entity m_Entity;
	};
}