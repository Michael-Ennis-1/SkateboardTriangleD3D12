#include "sktbdpch.h"
#include "D3DRenderingApi.h"
#include "Platform/DirectX12/D3DGraphicsContext.h"
#include "Platform/DirectX12/Model/D3DMeshletModel.h"

#include "Skateboard/Renderer/Pipeline.h"
#include "Skateboard/Scene/SceneBuilder.h"
#include "Platform/DirectX12/D3DBuffer.h"

//TODO: Need to move these around!
#define MAX_VERTS 64
#define MAX_PRIMS 126

constexpr uint32_t gMaxGroupDispatchCount = 65536;

// Calculates the size required for constant buffer alignment
template <typename T>
size_t GetAlignedSize(T size)
{
	const size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	const size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);
	return alignedSize;
}

// An integer version of ceil(value / divisor)
template <typename T, typename U>
T DivRoundUp(T value, U divisor)
{
	return (value + divisor - 1) / divisor;
}

namespace Skateboard
{
	D3DRenderingApi::D3DRenderingApi() : p_ActiveScene(nullptr)
	{
	}

	D3DRenderingApi::~D3DRenderingApi()
	{
	}

	void D3DRenderingApi::OnBeginSceneRender(float4 clearColour)
	{
		auto const commandList = gD3DContext->GraphicsCommandList();
		SKTBD_CORE_ASSERT(commandList, "Null command list!");
		
		const auto frameResource = gD3DContext->GetCurrentFrameResource();
		SKTBD_CORE_ASSERT(frameResource, "Null frame resource!");


		// Check if the GPU is done with the next frame resource before proceeding
		// If not wait to avoid buffer overwrites
		frameResource->NextFrameResource();
		const uint64_t currentFence = frameResource->GetCurrentFence();
		if (currentFence && gD3DContext->GetFence()->GetCompletedValue() < currentFence)
		{
#ifdef SKTBD_LOG_CPU_IDLE
			SKTBD_CORE_WARN("CPU idle, waiting on GPU to finish tasks!");
#endif
			HANDLE eventHandle = CreateEventEx(nullptr, NULL, NULL, EVENT_ALL_ACCESS);
			SKTBD_CORE_ASSERT(eventHandle, "Fence event creation failed!");

			// Fire the previously created event when GPU hits current fence
			D3D_CHECK_FAILURE(gD3DContext->GetFence()->SetEventOnCompletion(currentFence, eventHandle));

			// Wait until the GPU hits the current fence and the event is received
			WaitForSingleObject(eventHandle, INFINITE);

			// Close the event so that it does not exist anymore
			CloseHandle(eventHandle);
		}

		// Reset the command list allocator to reuse its memory
		// Note: this can only be reset once the GPU has finished processing all the commands
		ID3D12CommandAllocator* currentAllocator = frameResource->GetCurrrentCommandAllocator();
		D3D_CHECK_FAILURE(currentAllocator->Reset());

		// Also reset the command list
		// Note: It can be reset anytime after calling ExecuteCommandList on the command queue
		// Note: The second argument will be used when drawing geometry
		D3D_CHECK_FAILURE(commandList->Reset(currentAllocator, nullptr));


		// Transition the back buffer from a PRESENT state to a RT state so that it can be written onto
		D3D12_RESOURCE_BARRIER barrier = D3D::TransitionBarrier(gD3DContext->GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &barrier);

		
		const D3D12_VIEWPORT viewport = gD3DContext->GetViewport();
		const D3D12_RECT scissorsRect = gD3DContext->GetScissorsRect();

		// Set the viewport and scissor rects. They need to be reset everytime the command list is reset
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorsRect);

		// Specify the buffers we are going to render to
		// We must first retrieve the handles in order to be able to reference them
		const D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferHandle = gD3DContext->CurrentBackBufferView();
		const D3D12_CPU_DESCRIPTOR_HANDLE currentDepthStencilHandle = gD3DContext->DepthStencilView();

		// Clear the back buffer and depth buffer
		commandList->ClearRenderTargetView(
			gD3DContext->CurrentBackBufferView(),								// RTV to the resource we want to clear
			&clearColour.x,											// The colour to clear the render target to
			0,														// The number of items in the pRects array (next parameter)
			nullptr													// An array of D3D12_RECTs that identify rectangle regions on the render target to clear. When nullptr, the entire render target is cleared
		);
		commandList->ClearDepthStencilView(
			gD3DContext->DepthStencilView(),										// DSV to the resource we want to clear
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,		// Flags indicating which part of the depth/stencil buffer to clear (here both)
			1.f,													// Defines the value to clear the depth buffer
			0,														// Defines the value to clear the stencil buffer
			0,														// The number of items in the pRects array (next parameter)
			nullptr													// An array of D3D12_RECTs that identify rectangle regions on the render target to clear. When nullptr, the entire render target is cleared
		);

		
		commandList->OMSetRenderTargets(
			1,														// Defines the number of RTVs we are going to bind (next param)
			&currentBackBufferHandle,								// Pointer to an array of RTVs we want to bind to the pipeline
			true,													// Specify true if all the RTVs in the previous array are contiguous in the descriptor heap
			&currentDepthStencilHandle								// Pointer to a DSV we want to bind to the pipeline
		);


		// Set the main heap - no need for a const cast here.
		auto* pSrvHeap = gD3DContext->GetSrvHeap();
		commandList->SetDescriptorHeaps(1, pSrvHeap->GetHeap());
	}

	void D3DRenderingApi::OnEndSceneRender()
	{
		auto const commandList = gD3DContext->GraphicsCommandList();
		SKTBD_CORE_ASSERT(commandList, "Null command list!");
		const auto frameResource = gD3DContext->GetCurrentFrameResource();
		SKTBD_CORE_ASSERT(frameResource, "Null frame resource!");

		//	It might be a good idea to execute the graphics command list at this point.
		//	The idea would be to render as much as we can into the current buffer, flush then proceed
		//	to render the rest of the scene.

		//	TODO: Though in future we should implement a scene class (perhaps even a scene renderer class) to handle
		//	TODO: the scene world entities.

		// Now we bind the main swap chain buffer.
		gD3DContext->SetRenderTargetToBackBuffer();
	}

	void D3DRenderingApi::WaitUntilIdle()
	{
		gD3DContext->WaitUntilIdle();
	}

	void D3DRenderingApi::Flush()
	{

		gD3DContext->Flush();

	}

	void D3DRenderingApi::BeginScene(Scene* pScene)
	{
		p_ActiveScene = pScene;
	}

	void D3DRenderingApi::EndScene()
	{
		p_ActiveScene = nullptr;
	}

	void D3DRenderingApi::Draw(RasterizationPipeline* pipeline, VertexBuffer* vb)
	{
		// Fetch and check the command list is valid.
		auto const commandList = gD3DContext->GraphicsCommandList();
		SKTBD_CORE_ASSERT(commandList, "Null command list!");

		// Adds an instruction to the graphics command list to set the current vertex buffer active.
		vb->Bind();
		pipeline->Bind();

		// Adds a draw instanced command to the graphics command list. Does not use index buffer.
		// Please note, drawing/rendering does not occur at this stage. Use the ExecuteCommandList() api appropriately to do so.
		const DefaultBufferDesc desc = vb->GetBuffer()->GetDesc();
		commandList->DrawInstanced(desc.ElementCount, 1, 0, 0);
	}

	void D3DRenderingApi::DrawIndexed(RasterizationPipeline* pipeline, VertexBuffer* vb, IndexBuffer* ib)
	{
		// Fetch and validate the graphics command list.
		auto const commandList = gD3DContext->GraphicsCommandList();
		SKTBD_CORE_ASSERT(commandList, "Null command list!");

		// Adds instructions to the graphics command list, setting the vertex buffer and index buffer.
		vb->Bind();
		ib->Bind();
		pipeline->Bind();

		// Finally this adds an instruction to the command list to draw an entity with respect to the associated index buffer and vertex buffer.
		// Please note, drawing/rendering does not occur at this stage. Use the ExecuteCommandList() api appropriately to do so.
		commandList->DrawIndexedInstanced(ib->GetIndexCount(), 1, 0, 0, 0);
	}

	void D3DRenderingApi::DrawIndexedInstanced(RasterizationPipeline* pipeline, MeshID meshID)
	{
		SKTBD_CORE_ASSERT(p_ActiveScene != nullptr, "No active was set. Did you forget to call BeginScene()?");

		// Do not call a draw with empty data
		if (!p_ActiveScene->IsMeshValid(meshID))
			return;

		ID3D12GraphicsCommandList* const commandList = gD3DContext->GraphicsCommandList();
		SKTBD_CORE_ASSERT(commandList, "Null command list!");

		const uint32_t indexCount = p_ActiveScene->GetMeshIndexCount(meshID);
		const uint32_t instanceCount = p_ActiveScene->GetMeshInstanceCount(meshID);			// In D3D instance indices always start from 0
		const uint32_t startIndexLocation = p_ActiveScene->GetMeshStartIndexLocation(meshID);
		const uint32_t startVertexLocation = p_ActiveScene->GetMeshStartVertexLocation(meshID);
		const uint32_t startInstanceLocation = p_ActiveScene->GetMeshStartInstanceLocation(meshID);

		pipeline->Bind(startInstanceLocation);
		commandList->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, startVertexLocation, 0);
	}

	void D3DRenderingApi::Dispatch(ComputePipeline* pipeline)
	{
		// Validate the graphics command list before recording any further instructions.
		auto const commandList = gD3DContext->GraphicsCommandList();
		SKTBD_CORE_ASSERT(commandList, "Null command list!");

		// Adds a dispatch api call to the graphics command list.
		// Please note, the kernel does not execute at this stage. Use the ExecuteCommandList() api appropriately to do so.
		pipeline->Bind();
		const ComputePipelineDesc& desc = pipeline->GetDesc();
		commandList->Dispatch(desc.DispatchSize.ThreadCountX, desc.DispatchSize.ThreadCountY, desc.DispatchSize.ThreadCountZ);
	}

	void D3DRenderingApi::DispatchMeshlet(MeshletPipeline* pipeline)
	{
		// Validate the graphics command list before recording any further instructions.
		auto const commandList = gD3DContext->GraphicsCommandList();
		SKTBD_CORE_ASSERT(commandList, "Null command list!");

		// Adds a dispatch api call to the graphics command list.
		// Please note, the kernel does not execute at this stage. Use the ExecuteCommandList() api appropriately to do so.
		pipeline->BindMeshPipeline();
		// Dispatching mesh shader is now invoked within pipeline.
		//const MeshletPipelineDesc& desc = pipeline->GetDesc();
		//commandList->DispatchMesh(desc.DispatchSize.X, desc.DispatchSize.Y, desc.DispatchSize.Z);
	}

	void D3DRenderingApi::DispatchInstancedMeshlet(MeshletPipeline* pipeline)
	{
		// TODO: Implement meshlet instancing
		auto* pCommandList = gD3DContext->GraphicsCommandList();

		//	TODO: add support for complex mesh
		auto& desc = pipeline->GetDesc();

		pipeline->BindMeshPipeline();

		
	}

	void D3DRenderingApi::DispatchMeshletModel(MeshletPipeline* pipeline, MeshletModel* model)
	{
		// Validate the graphics command list before recording any further instructions.
		auto const commandList = gD3DContext->GraphicsCommandList();
		SKTBD_CORE_ASSERT(commandList, "Null command list!");

		// Adds a dispatch api call to the graphics command list.
		// Please note, the kernel does not execute at this stage. Use the ExecuteCommandList() api appropriately to do so.
		pipeline->BindMeshPipeline(model);
		// Dispatching mesh shader is now invoked within pipeline.
		//const MeshletPipelineDesc& desc = pipeline->GetDesc();
		//commandList->DispatchMesh(desc.DispatchSize.X, desc.DispatchSize.Y, desc.DispatchSize.Z);
	}

	void D3DRenderingApi::DispatchMeshletModelInstanced(MeshletPipeline* pipeline, MeshletModel* model)
	{
		// Validate the graphics command list before recording any further instructions.
		auto const commandList = gD3DContext->GraphicsCommandList();
		SKTBD_CORE_ASSERT(commandList, "Null command list!");

		// Adds a dispatch api call to the graphics command list.
		// Please note, the kernel does not execute at this stage. Use the ExecuteCommandList() api appropriately to do so.
		pipeline->BindMeshPipeline(model);
		// Dispatching mesh shader is now invoked within pipeline.
		//const MeshletPipelineDesc& desc = pipeline->GetDesc();
		//commandList->DispatchMesh(desc.DispatchSize.X, desc.DispatchSize.Y, desc.DispatchSize.Z);
	}

	void D3DRenderingApi::DrawMesh(RasterizationPipeline* pipeline, Model* mesh)
	{
		pipeline->Bind();

	}

	void D3DRenderingApi::DrawInstanced(RasterizationPipeline* pipeline, Model* mesh)
	{
	}


	void D3DRenderingApi::DispatchRays(RaytracingPipeline* pipeline)
	{
		pipeline->Bind();
	}

	void D3DRenderingApi::CopyUAVToBackBuffer(UnorderedAccessBuffer* pUAV)
	{
		ID3D12GraphicsCommandList* pCommandList = gD3DContext->GraphicsCommandList();
		ID3D12Resource* pCurrentBackBuffer = gD3DContext->GetCurrentBackBuffer();
		ID3D12Resource* pUAVResource = static_cast<D3DUnorderedAccessBuffer*>(pUAV)->GetResource();

		D3D12_RESOURCE_BARRIER preCopyBarriers[2];
		preCopyBarriers[0] = D3D::TransitionBarrier(pCurrentBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
		preCopyBarriers[1] = D3D::TransitionBarrier(pUAVResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pCommandList->ResourceBarrier(2, preCopyBarriers);

		pCommandList->CopyResource(pCurrentBackBuffer, pUAVResource);

		D3D12_RESOURCE_BARRIER postCopyBarriers[2];
		postCopyBarriers[0] = D3D::TransitionBarrier(pCurrentBackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
		postCopyBarriers[1] = D3D::TransitionBarrier(pUAVResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		pCommandList->ResourceBarrier(2, postCopyBarriers);
	}

	void D3DRenderingApi::CopyUAVToFrameBuffer(FrameBuffer* pFrame, UnorderedAccessBuffer* pUAV)
	{
		ID3D12GraphicsCommandList* pCommandList = gD3DContext->GraphicsCommandList();
		ID3D12Resource* pFrameBufferResource = static_cast<D3DFrameBuffer*>(pFrame)->GetResource();
		ID3D12Resource* pUAVResource = static_cast<D3DUnorderedAccessBuffer*>(pUAV)->GetResource();

		D3D12_RESOURCE_BARRIER preCopyBarriers[2];
		preCopyBarriers[0] = D3D::TransitionBarrier(pFrameBufferResource, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		preCopyBarriers[1] = D3D::TransitionBarrier(pUAVResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pCommandList->ResourceBarrier(2, preCopyBarriers);

		pCommandList->CopyResource(pFrameBufferResource, pUAVResource);

		D3D12_RESOURCE_BARRIER postCopyBarriers[2];
		postCopyBarriers[0] = D3D::TransitionBarrier(pFrameBufferResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		postCopyBarriers[1] = D3D::TransitionBarrier(pUAVResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		pCommandList->ResourceBarrier(2, postCopyBarriers);
	}

	
}
