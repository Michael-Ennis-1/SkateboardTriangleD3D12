#pragma once
#include "Skateboard/Log.h"


namespace Skateboard
{
	class MeshBank;
	class MeshletPipeline;
	class RasterizationPipeline;
	class MeshletBank;

	class SceneRenderer
	{
		friend class Scene;
	public:
		SceneRenderer();
		DISABLE_COPY_AND_MOVE(SceneRenderer);
		~SceneRenderer();

		void Clean();


	private:
		// In the scene renderer we have all of the necessary stuff to render entities.
		std::unique_ptr<MeshletBank> m_MeshletBank;
		std::unique_ptr<MeshBank> m_MeshBank;
		std::unordered_map<std::wstring_view, std::unique_ptr<RasterizationPipeline>> m_RasterizationPipelines;
		std::unordered_map<std::wstring_view, std::unique_ptr<MeshletPipeline>> m_MeshletPipelines;
	};

}
