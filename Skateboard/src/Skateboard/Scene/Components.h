#pragma once


#include <string>
#include <vector>

#include "Skateboard/Camera.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Renderer/Materials/Material.h"
#include "Skateboard/Scene/InstanceData.h"
#include "Skateboard/Renderer/FrameResources.h"
#include "Skateboard/Renderer/Model/Model.h"

namespace Skateboard
{

	struct TagComponent
	{
		std::string tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& string)
			:
			tag(string) {}
	};

	struct TransformComponent
	{
		float3 Translation = { 0.f,0.f,0.f };
		float3 Rotation	  = { 0.f, 0.f,0.f };
		float3 Scale		  = { 1.f, 1.f, 1.f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const float3& trans)
			:
			Translation(trans){}

		_NODISCARD float4x4 GetTransform() const
		{
			//TODO: Needs to be implemented using skateboard mathematics.
			//TODO: Should perform typical SRT matrix transform.
			const auto scale = Skateboard::Vector3Load(&Scale);
			const auto translation = Skateboard::Vector3Load(&Translation);

			return	MatrixScalingFromVector(scale) *
					MatrixRotationPitchYawRoll(DegToRad(Rotation.x), DegToRad(Rotation.y), DegToRad(Rotation.z)) *
					MatrixTranslationFromVector(translation);
		}

	};

	struct CameraComponent
	{
		PerspectiveCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	//Forward Declarations
	class ScriptableEntity;

	struct NativeScriptComponent
	{
		ScriptableEntity* Instance = nullptr;

		ScriptableEntity* (*InstantiateScript)();
		void (*DestoryScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestoryScript = [](NativeScriptComponent* nsc) { delete nsc->Instance; nsc->Instance = nullptr; };
		}
	};


	struct MaterialComponent
	{
		MaterialComponent() = default;
		~MaterialComponent() = default;
		Material Material;
	};

	//	<summary>
	//	The static mesh component contains a reference to the geometric properties 
	//	which are to be processed by the render engine. How this geometry is displayed is
	//	governed by the renderer component.
	//	</summary>
	struct StaticMeshComponent
	{
		StaticMeshComponent() = default;
		StaticMeshComponent(const StaticMeshComponent&) = default;
		auto operator=(const StaticMeshComponent&)->StaticMeshComponent & = default;
		StaticMeshComponent(StaticMeshComponent&&) = default;
		auto operator=(StaticMeshComponent&&) noexcept -> StaticMeshComponent & = default;

	};

	//	<summary>
	//	The static mesh instance component contains a mesh ID to a pre-existing mesh
	//  geometry which will be instanced in the scene. In addition, an instance ID,
	//  is assigned upon being instantiated.
	//	</summary>
	struct StaticMeshInstanceComponent
	{
		StaticMeshInstanceComponent() : MeshId(UINT32_MAX), InstanceId(UINT32_MAX), MaterialId(-1) {}
		StaticMeshInstanceComponent(uint32_t meshID) : MeshId(meshID), InstanceId(UINT32_MAX), MaterialId(-1) {}
		StaticMeshInstanceComponent(const StaticMeshInstanceComponent&) = default;
		auto operator=(const StaticMeshInstanceComponent&) noexcept ->StaticMeshInstanceComponent & = default;
		StaticMeshInstanceComponent(StaticMeshInstanceComponent&&) = default;
		auto operator=(StaticMeshInstanceComponent&) noexcept -> StaticMeshInstanceComponent& = default;
		~StaticMeshInstanceComponent() = default;

		MeshID MeshId;
		InstanceID InstanceId;
		int32_t MaterialId;
	};

	struct MeshRendererComponent
	{
		MeshRendererComponent() = default;
	
		MeshRendererComponent(const MeshRendererComponent&) = default;
		auto operator=(const MeshRendererComponent&) noexcept ->MeshRendererComponent & = default;
		MeshRendererComponent(MeshRendererComponent&&) = default;
		auto operator=(MeshRendererComponent&) noexcept -> MeshRendererComponent & = default;
		~MeshRendererComponent() = default;

		std::vector<Material> MaterialList;
	};

	
	struct StaticMeshletComponent
	{
		StaticMeshletComponent() = default;
		StaticMeshletComponent(const std::wstring_view& meshTag)
			:
			MeshTag(std::move(meshTag))
		{}
		~StaticMeshletComponent() = default;

		std::wstring MeshTag{L""};
		uint32_t MeshID{ 1 };

	};

	//	<summary>
	//	The meshlet renderer component contains vital information
	//	which the renderer will use in order to render the mesh correctly.
	//	</summary>
	struct MeshletRendererComponent
	{
		MeshletRendererComponent() = default;
		
		// TODO: Implement lighting override settings.

		// TODO: Implement material override settings.
		// The idea is that the meshlet renderer drives the rendering of the current object.
		// Renderer can extrapolate the data it needs to render the object with the desired 
		// attributes.

		std::array<Material, 32> MaterialList;
		bool CastShadows{ false };
		bool ReceiveShadows{false};
		bool ContributeToGI{false};
		GIOptions_ GI{ GIOptions_::GIOptions_Baked };
	};


}


