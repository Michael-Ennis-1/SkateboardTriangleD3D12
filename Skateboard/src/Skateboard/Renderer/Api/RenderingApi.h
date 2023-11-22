#pragma once
#include "Skateboard/Mathematics.h"
#include "Skateboard/Scene/SceneBuilder.h"

namespace Skateboard
{
	class MeshletModel;
	class Model;

	typedef std::function<void> RenderLambda;
	typedef std::function<void> ComputeLambda;

	// Forward declarations
	class Pipeline;
	class MeshletPipeline;
	class RasterizationPipeline;
	class ComputePipeline;
	class RaytracingPipeline;
	class VertexBuffer;
	class IndexBuffer;
	class UnorderedAccessBuffer;
	class FrameBuffer;
	class Scene;


	class RenderingApi
	{
	public:
		virtual ~RenderingApi(){}

		// RENDERER UTILITIES //////////////////////////////////////////////////////////////

		virtual void OnBeginSceneRender(float4 clearColour) = 0;

		virtual void OnEndSceneRender() = 0;

		virtual void WaitUntilIdle() = 0;

		virtual void Flush() = 0;

		// DRAW COMMANDS ///////////////////////////////////////////////////////////////////

		virtual void BeginScene(Scene* pScene) = 0;
		virtual void EndScene() = 0;

		virtual void Draw(RasterizationPipeline* pipeline, VertexBuffer* vb) = 0;

		virtual void DrawIndexed(RasterizationPipeline* pipeline, VertexBuffer* vb, IndexBuffer* ib) = 0;
		virtual void DrawIndexedInstanced(RasterizationPipeline* pipeline, MeshID meshID) = 0;

		virtual void DrawMesh(RasterizationPipeline* pipeline, Model* mesh) = 0;
		virtual void DrawInstanced(RasterizationPipeline* pipeline, Model* mesh) = 0;

		virtual void Dispatch(ComputePipeline* pipeline) = 0;

		virtual void DispatchRays(RaytracingPipeline* pipeline) = 0;
		virtual void CopyUAVToBackBuffer(UnorderedAccessBuffer* pUAV) = 0;
		virtual void CopyUAVToFrameBuffer(FrameBuffer* pFrame, UnorderedAccessBuffer* pUAV) = 0;


		// Mesh shader pipeline api
		virtual void DispatchMeshlet(MeshletPipeline* pipeline) = 0;
		virtual void DispatchInstancedMeshlet(MeshletPipeline* pipeline) = 0;

		virtual void DispatchMeshletModel(MeshletPipeline* pipeline, MeshletModel* mesh) = 0;
		virtual void DispatchMeshletModelInstanced(MeshletPipeline* pipeline, MeshletModel* mesh) = 0;
	

	};

}
