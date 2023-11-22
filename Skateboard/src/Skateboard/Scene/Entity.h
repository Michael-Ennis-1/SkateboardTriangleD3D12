#pragma once
#include <cstdint>

#include "entt.hpp"
#include "Scene.h"
#include "Skateboard/Log.h"

namespace Skateboard
{
	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene);
		Entity(const Entity& other) = default;
	

		template<typename T, typename ... Args> T& AddComponent(Args&&... args)
		{
			if (HasComponent<T>())
			{
				SKTBD_CORE_WARN("Tried to add a component that already exists. Cancelling operation..");
				return GetComponent<T>();
			}
			T& component = m_Scene->Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);

			m_Scene->OnComponentAdded<T>(*this, component);

			return component;
		}

		template<typename T> T& GetComponent() const 
		{
			SKTBD_CORE_ASSERT(HasComponent<T>(), "Entity  does not have component!");

			return m_Scene->Registry.get<T>(m_EntityHandle);
		}

		template<typename T> bool HasComponent() const
		{
			return m_Scene->Registry.any_of<T>(m_EntityHandle);
		}

		template<typename T> void RemoveComponent() const
		{
			SKTBD_CORE_ASSERT(HasComponent<T>(), "Entity does not have the component to be removed!");

			T componentCopy = m_Scene->Registry.get<T>(m_EntityHandle);
			m_Scene->Registry.remove<T>(m_EntityHandle);
			m_Scene->OnComponentRemoved<T>(*this, componentCopy);
		}

		operator bool() const { return m_EntityHandle != entt::null; }

		// @brief Implicit conversion operator to an unsigned 32-bit integer.
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		// @brief Implicit conversion operator to a entity handle.
		operator entt::entity() const { return m_EntityHandle; }

		// @brief Comparator operator for a valid entity handle.
		bool operator ==(const Entity& other) const 
		{
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		}

		// @brief Comparator operator for entity object comparisons.
		bool operator !=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene = nullptr;
	};
}


