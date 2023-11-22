#pragma once
#include "Pipeline.h"
#include "Skateboard/Renderer/Api/RenderingApi.h"

namespace Skateboard
{
	// Forward declarations
	class GraphicsContext;
	class Pipeline;
	class VertexBuffer;
	class IndexBuffer;



	class RenderCommand
	{
		friend class Renderer;
	public:
		~RenderCommand();

		static void OnBeginRender(float4 clearColour = {0.1f,0.1f,0.1f,0.1f})
		{
			RenderingApi->OnBeginSceneRender(clearColour);
		}

		static void OnEndRender()
		{
			RenderingApi->OnEndSceneRender();
		}

		static void BeginScene(class Scene* pScene)
		{
			RenderingApi->BeginScene(pScene);
		}

		static void EndScene()
		{
			RenderingApi->EndScene();
		}

		static void Draw(RasterizationPipeline* pipeline, VertexBuffer* vb)
		{
			RenderingApi->Draw(pipeline, vb);
		}

		static void DrawIndexed(RasterizationPipeline* pipeline, VertexBuffer* vb, IndexBuffer* ib)
		{
			RenderingApi->DrawIndexed(pipeline, vb, ib);
		}

		static void DrawIndexedInstanced(RasterizationPipeline* pipeline, MeshID meshID)
		{
			RenderingApi->DrawIndexedInstanced(pipeline, meshID);
		}

		static void DispatchMeshletModel(MeshletPipeline* pipeline, MeshletModel* model)
		{
			RenderingApi->DispatchMeshletModel(pipeline, model);
		}

		static void DispatchMeshletModelInstance(MeshletPipeline* pipeline, MeshletModel* model)
		{
			RenderingApi->DispatchMeshletModelInstanced(pipeline, model);
		}

		static void DrawMesh(RasterizationPipeline* pipeline, Model* mesh)
		{
				RenderingApi->DrawMesh(pipeline, mesh);
		}

		static void DrawInstanced(RasterizationPipeline* pipeline, Model* mesh)
		{
				RenderingApi->DrawInstanced(pipeline, mesh);
		}

		static void Dispatch(ComputePipeline* pipeline)
		{
			RenderingApi->Dispatch(pipeline);
		}

		static void DispatchMesh(MeshletPipeline* pipeline)
		{
			RenderingApi->DispatchMeshlet(pipeline);
		}

		static void DispatchInstancedMesh(MeshletPipeline* pipeline)
		{
			RenderingApi->DispatchInstancedMeshlet(pipeline);
		}
		
		static void DispatchRays(RaytracingPipeline* pipeline)
		{
			RenderingApi->DispatchRays(pipeline);
		}

		static void CopyUAVToBackBuffer(UnorderedAccessBuffer* pUAV)
		{
			RenderingApi->CopyUAVToBackBuffer(pUAV);
		}

		static void CopyUAVToFrameBuffer(FrameBuffer* pFrame, UnorderedAccessBuffer* pUAV)
		{
			RenderingApi->CopyUAVToFrameBuffer(pFrame, pUAV);
		}

		static void WaitUntilIdle()
		{
			RenderingApi->WaitUntilIdle();
		}

		static void Flush()
		{
			RenderingApi->Flush();
		}

	private:
		static void Init();

		static RenderingApi* RenderingApi;


	};

	
}
