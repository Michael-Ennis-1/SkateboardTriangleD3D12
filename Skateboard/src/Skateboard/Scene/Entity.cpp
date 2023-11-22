#include "sktbdpch.h"
#include "Entity.h"

namespace Skateboard
{
	Entity::Entity(entt::entity handle, class Scene* scene)
		:
		m_EntityHandle(handle),
		m_Scene(scene)
	{}
}