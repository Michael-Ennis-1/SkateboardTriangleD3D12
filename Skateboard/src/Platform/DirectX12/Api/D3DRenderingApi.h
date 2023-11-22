#pragma once
#include "Skateboard/Renderer/Api/RenderingApi.h"

namespace Skateboard
{
	class D3DGraphicsContext;
	class Scene;

	// <summary>
	class D3DRenderingApi : public RenderingApi
	{
	public:
		~D3DRenderingApi() override;

		D3DRenderingApi();

		void OnBeginSceneRender(float4 clearColour) override;

		void OnEndSceneRender() override;

		virtual void WaitUntilIdle() final override;

		void Flush() override;

		virtual void BeginScene(Scene* pScene) final override;
		virtual void EndScene() final override;
		virtual void Draw(RasterizationPipeline* pipeline, VertexBuffer* vb) final override;
		virtual void DrawIndexed(RasterizationPipeline* pipeline, VertexBuffer* vb, IndexBuffer* ib) final override;
		virtual void DrawIndexedInstanced(RasterizationPipeline* pipeline, MeshID meshID) final override;

		void Dispatch(ComputePipeline* pipeline) override;

		virtual void DispatchRays(RaytracingPipeline* pipeline) final override;
		virtual void CopyUAVToBackBuffer(UnorderedAccessBuffer* pUAV) final override;
		virtual void CopyUAVToFrameBuffer(FrameBuffer* pFrame, UnorderedAccessBuffer* pUAV) final override;

		void DispatchMeshlet(MeshletPipeline* pipeline) override;
		void DispatchInstancedMeshlet(MeshletPipeline* pipeline) override;

		void DispatchMeshletModel(MeshletPipeline* pipeline, MeshletModel* mesh) override;
		void DispatchMeshletModelInstanced(MeshletPipeline* pipeline, MeshletModel* mesh) override;

		void DrawMesh(RasterizationPipeline* pipeline, Model* mesh) override;
		void DrawInstanced(RasterizationPipeline* pipeline, Model* mesh) override;
	private:
		Scene* p_ActiveScene;

	
	};
}