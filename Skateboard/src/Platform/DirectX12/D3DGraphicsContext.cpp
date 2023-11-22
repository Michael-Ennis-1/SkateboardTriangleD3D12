#include "sktbdpch.h"
#include "D3DGraphicsContext.h"
#include "Skateboard/Platform.h"
#include "Skateboard/Renderer/Buffer.h"
#include "Skateboard/Renderer/Pipeline.h"

#define USE_PIX
#include <pix3.h>
#include <filesystem>
#include <shlobj.h>

namespace Skateboard
{

	static std::wstring GetLatestWinPixGpuCapturerPath_Cpp17()
	{
		LPWSTR programFilesPath = nullptr;
		SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

		std::filesystem::path pixInstallationPath = programFilesPath;
		pixInstallationPath /= "Microsoft PIX";

		std::wstring newestVersionFound;

		//for (auto const& directory_entry : std::filesystem::directory_iterator(pixInstallationPath))
		//{
		//	if (directory_entry.is_directory())
		//	{
		//		if (newestVersionFound.empty() || newestVersionFound < directory_entry.path().filename().c_str())
		//		{
		//			newestVersionFound = directory_entry.path().filename().c_str();
		//		}
		//	}
		//}

		if (newestVersionFound.empty())
		{
			return L"EMPTY";
		}

		return pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
	}

	D3DGraphicsContext* gD3DContext = nullptr;
	GraphicsContext* GraphicsContext::Context = nullptr;

	D3DGraphicsContext::D3DGraphicsContext(HWND window, const PlatformProperties& props) :
		GraphicsContext(),
		m_D3DDriverType(D3D_DRIVER_TYPE_HARDWARE),
		m_BackBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM),			// Each element has four 8-bit unsigned components mapped to the [0,1] range
		m_DepthStencilFormat(DXGI_FORMAT_D24_UNORM_S8_UINT),	// Specifies an unsigned 24-bit depth buffer mapped to the [0,1] range, with an 8-bit unsigned integer reserved for the stencil buffer ([0, 255] range) 
		m_ClientWidth(props.BackBufferWidth),
		m_ClientHeight(props.BackBufferHeight),
		m_Vsync(false),
		m_MSAAEnable(false),									// Set to false by default, change it to true in the constructor to enable MSAA
		m_MSAAQuality(0),										// Needs to be queried in Check4xMSAAQualitySupport
		m_MainWindow(window),
		m_CurrentFence(0u),
		m_CurrentBackBuffer(0),									// Start by working on the first buffer
		m_RTVDescriptorSize(0u),
		m_DSVDescriptorSize(0u),
		m_CBVSRVUAVDescriptorSize(0u),
		m_HasDXR(false),										// This will be checked on initialisation, if the GPU supports DXR
		m_ClientResized(true)
	{
		//Load PIX
		// Check to see if a copy of WinPixGpuCapturer.dll has already been injected into the application.
		// This may happen if the application is launched through the PIX UI. 
		if (GetModuleHandle(L"WinPixGpuCapturer.dll") == 0)
		{
			SKTBD_CORE_TRACE("Injecting WinPixCapturer.dll...");
			const wchar_t* path = GetLatestWinPixGpuCapturerPath_Cpp17().c_str();
			if(path != L"EMPTY")
			{
				LoadLibrary(path);
				m_GpuCaptureLib = PIXLoadLatestWinPixGpuCapturerLibrary();
				m_GpuTimingLib  = PIXLoadLatestWinPixTimingCapturerLibrary();
			}
			else
			{
				SKTBD_CORE_WARN("PIX Capturer Invalid!");
			}
		}
		else
		{
			SKTBD_CORE_WARN("Failed to inject WinPixCapturer.dll");
		}

		// Assign the global context
		SKTBD_CORE_TRACE("Initialising the Graphics Context..");
		SKTBD_CORE_ASSERT(!gD3DContext, "Cannot initialise a second D3D graphics context. Illegal context creation.");
		gD3DContext = this;
		GraphicsContext::Context = this;

		// Initialise D3D components
		CreateDevice();
		CreateFence();
		CreateDescriptorSizes();
		Check4xMSAAQualitySupport();
		CheckSupportForGPUUploadHeaps();
		CreateCommandQueueAndCommandList();
		CreateSwapChain();
		CreateRaytracingInterfaces();
		CreateMeshShaderInterfaces();

		m_FrameResources.Init();

		// Allocate basic descriptor heaps
		CreateDescriptorHeaps();

		// Create the output buffers
		OnResized();

		m_SwapChainBuffers[0]->SetName(L"SwapChain - Buffer(0)");
		m_SwapChainBuffers[1]->SetName(L"SwapChain - Buffer(1)");
	}

	D3DGraphicsContext::~D3DGraphicsContext()
	{
		SKTBD_CORE_TRACE("Destroying Device..");
		ProcessDeferrals(m_FrameResources.GetCurrentFrameResourceIndex());
	
	}

	void D3DGraphicsContext::Clean()
	{
		ProcessDeferrals(0);
		ProcessDeferrals(1);
		ProcessDeferrals(2);
	}

	void D3DGraphicsContext::Present(void* args)
	{

		// Transition the back buffer from a PRESENT state to a RT state so that it can be written onto
		D3D12_RESOURCE_BARRIER barrier = D3D::TransitionBarrier(m_SwapChainBuffers[m_CurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_CommandList->ResourceBarrier(1, &barrier);

		// Close the command list for recording.
		D3D_CHECK_FAILURE(m_CommandList->Close());


		if(args != nullptr)
		{
			auto* pCommandList = static_cast<ID3D12GraphicsCommandList*>(args);
			//Add the command list to the command queue for execution
			ID3D12CommandList* commandLists[] = { pCommandList };
			m_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
		}
		else
		{
			//Add the command list to the command queue for execution
			ID3D12CommandList* commandLists[] = { m_CommandList.Get() };
			m_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
		}


		// Swap the back and front buffers
		// Arg1 - An integer that specifies how to synchronize presentation of a frame with the vertical blank:
		//	0 - Cancel the remaining time on the previously presented frame and discard this frame if a newer frame is queued
		// Arg2 - An integer value that contains swap-chain presentation options. These options are defined by the DXGI_PRESENT constants
		//D3D_CHECK_FAILURE(m_SwapChain->Present(m_Vsync, 0));
		if (FAILED(m_SwapChain->Present(m_Vsync, 0)))
		{
			HRESULT hr = m_Device->GetDeviceRemovedReason();
			D3D_CHECK_FAILURE(hr);
		}

		// Update the index to the current back buffer so that we render to the other buffer next frame
		m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % g_SwapChainBufferCount;			// either 0 or 1

		// Frame processed, mark the frame fence
		m_FrameResources.SetCurrentFence(++m_CurrentFence);	// ++m_CurrentFence <=> m_CurrentFence += 1 & then supply it as arg
		m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence);
	}

	void D3DGraphicsContext::StartDraw(float4 clearColour)
	{

		// Check if the GPU is done with the next frame resource before proceeding
		// If not wait to avoid buffer overwrites
		m_FrameResources.NextFrameResource();
		const uint64_t currentFence = m_FrameResources.GetCurrentFence();
		if (currentFence && m_Fence->GetCompletedValue() < currentFence)
		{
#ifdef SKTBD_LOG_CPU_IDLE
			SKTBD_CORE_WARN("CPU idle, waiting on GPU to finish tasks!");
#endif
			HANDLE eventHandle = CreateEventEx(nullptr, NULL, NULL, EVENT_ALL_ACCESS);
			SKTBD_CORE_ASSERT(eventHandle, "Fence event creation failed!");

			// Fire the previously created event when GPU hits current fence
			D3D_CHECK_FAILURE(m_Fence->SetEventOnCompletion(currentFence, eventHandle));

			// Wait until the GPU hits the current fence and the event is received
			WaitForSingleObject(eventHandle, INFINITE);

			// Close the event so that it does not exist anymore
			CloseHandle(eventHandle);
		}

		// Reset the command list allocator to reuse its memory
		// Note: this can only be reset once the GPU has finished processing all the commands
		ID3D12CommandAllocator* currentAllocator = m_FrameResources.GetCurrrentCommandAllocator();
		D3D_CHECK_FAILURE(currentAllocator->Reset());

		// Also reset the command list
		// Note: It can be reset anytime after calling ExecuteCommandList on the command queue
		// Note: The second argument will be used when drawing geometry
		D3D_CHECK_FAILURE(m_CommandList->Reset(currentAllocator, nullptr));

		// Transition the back buffer from a PRESENT state to a RT state so that it can be written onto
		D3D12_RESOURCE_BARRIER barrier = D3D::TransitionBarrier(m_SwapChainBuffers[m_CurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_CommandList->ResourceBarrier(1, &barrier);

		// Set the viewport and scissor rects. They need to be reset everytime the command list is reset
		m_CommandList->RSSetViewports(1, &m_Viewport);
		m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

		// Clear the back buffer and depth buffer
		m_CommandList->ClearRenderTargetView(
			CurrentBackBufferView(),								// RTV to the resource we want to clear
			&clearColour.x,											// The colour to clear the render target to
			0,														// The number of items in the pRects array (next parameter)
			nullptr													// An array of D3D12_RECTs that identify rectangle regions on the render target to clear. When nullptr, the entire render target is cleared
		);
		m_CommandList->ClearDepthStencilView(
			DepthStencilView(),										// DSV to the resource we want to clear
			D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,		// Flags indicating which part of the depth/stencil buffer to clear (here both)
			1.f,													// Defines the value to clear the depth buffer
			0,														// Defines the value to clear the stencil buffer
			0,														// The number of items in the pRects array (next parameter)
			nullptr													// An array of D3D12_RECTs that identify rectangle regions on the render target to clear. When nullptr, the entire render target is cleared
		);

		// Specify the buffers we are going to render to
		// We must first retrieve the handles in order to be able to reference them
		D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferHandle = CurrentBackBufferView();
		D3D12_CPU_DESCRIPTOR_HANDLE currentDepthStencilHandle = DepthStencilView();

		m_CommandList->OMSetRenderTargets(
			1,														// Defines the number of RTVs we are going to bind (next param)
			&currentBackBufferHandle,								// Pointer to an array of RTVs we want to bind to the pipeline
			true,													// Specify true if all the RTVs in the previous array are contiguous in the descriptor heap
			&currentDepthStencilHandle								// Pointer to a DSV we want to bind to the pipeline
		);

		// Set the main heap
		m_CommandList->SetDescriptorHeaps(1, m_SRVDescriptorHeap.GetHeap());
	}

	void D3DGraphicsContext::EndDraw()
	{
		// Transition the back buffer from a RT state to a PRESENT state so that it can be displayed on the screen
		D3D12_RESOURCE_BARRIER barrier = D3D::TransitionBarrier(m_SwapChainBuffers[m_CurrentBackBuffer].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_CommandList->ResourceBarrier(1, &barrier);

		// We are done recording commands, close the command list
		D3D_CHECK_FAILURE(m_CommandList->Close());

		//Add the command list to the command queue for execution
		ID3D12CommandList* commandLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		// Swap the back and front buffers
		// Arg1 - An integer that specifies how to synchronize presentation of a frame with the vertical blank:
		//	0 - Cancel the remaining time on the previously presented frame and discard this frame if a newer frame is queued
		// Arg2 - An integer value that contains swap-chain presentation options. These options are defined by the DXGI_PRESENT constants
		//D3D_CHECK_FAILURE(m_SwapChain->Present(m_Vsync, 0));
		if (FAILED(m_SwapChain->Present(m_Vsync, 0)))
		{
			HRESULT hr = m_Device->GetDeviceRemovedReason();
			D3D_CHECK_FAILURE(hr);
		}

		// Update the index to the current back buffer so that we render to the other buffer next frame
		m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % g_SwapChainBufferCount;			// either 0 or 1

		// Frame processed, mark the frame fence
		m_FrameResources.SetCurrentFence(++m_CurrentFence);	// ++m_CurrentFence <=> m_CurrentFence += 1 & then supply it as arg
		m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence);
	}

	void D3DGraphicsContext::SetRenderTargetToBackBuffer()
	{
		m_CommandList->RSSetViewports(1, &m_Viewport);
		m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

		D3D12_CPU_DESCRIPTOR_HANDLE currentBackBufferHandle = CurrentBackBufferView();
		D3D12_CPU_DESCRIPTOR_HANDLE currentDepthStencilHandle = DepthStencilView();
		m_CommandList->OMSetRenderTargets(1, &currentBackBufferHandle, true, &currentDepthStencilHandle);
	}

	void D3DGraphicsContext::CreateDevice()
	{
		SKTBD_CORE_TRACE("Creating d3d12 device..");

		// First, enable the debug layer of D3D12 when running the application in debug mode
		// This will output debug messages when warnings, errors or crashes occur during rutime in the output window
#ifndef SKTBD_SHIP
		Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
		HRESULT res = D3D12GetDebugInterface(IID_PPV_ARGS(debugController.ReleaseAndGetAddressOf()));
		D3D_CHECK_FAILURE(res);
		debugController->EnableDebugLayer();
#endif

		// Create the DXGI interface 1.1
		D3D_CHECK_FAILURE(CreateDXGIFactory1(IID_PPV_ARGS(m_DXGIInterface.ReleaseAndGetAddressOf())));

		// Create a list of every GFX adapter available
		IDXGIAdapter* pAdapter = nullptr;
		std::vector<IDXGIAdapter*> vAdapters;
		for (UINT i = 0u; m_DXGIInterface->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
			vAdapters.push_back(pAdapter);

		// Find the best GFX adapter on this machine
		SIZE_T maximumVideoMemory = 0;
		DXGI_ADAPTER_DESC adapterDescription;
		for (auto& currentAdapter : vAdapters)
		{
			// Get the description of this adapter
			// This will help identify the adapter's properties
			currentAdapter->GetDesc(&adapterDescription);

			// Select the best adapter based on the maximum video memory
			if (adapterDescription.DedicatedVideoMemory > maximumVideoMemory)
			{
				maximumVideoMemory = adapterDescription.DedicatedVideoMemory;
				pAdapter = currentAdapter;
			}
		}

		// Try to create a device on the selected adapter
		m_D3DFeatureLevel = D3D_FEATURE_LEVEL_11_0;				// We want to limit this application to adapter capable of handling DX12
		if (!pAdapter || FAILED(D3D12CreateDevice(pAdapter, m_D3DFeatureLevel, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf()))))
		{
#ifndef SKTBD_SHIP
			// Fallback to a WARP device
			SKTBD_CORE_WARN(L"Could not create device or no GPU detected, fallback to a warp device...\n");
#endif
			// Get a suitable WARP adapter and create a device with it
			D3D_CHECK_FAILURE(m_DXGIInterface->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter)));

			// Create a device on the warp adapter. If failed, throw an exception as no adapter was selected
			D3D_CHECK_FAILURE(D3D12CreateDevice(pAdapter, m_D3DFeatureLevel, IID_PPV_ARGS(m_Device.ReleaseAndGetAddressOf())));

			// Get description and release memory
			pAdapter->GetDesc(&adapterDescription);
			pAdapter->Release(), pAdapter = nullptr;
		}

#ifndef SKTBD_SHIP
		// Output the selected device on the console
		std::wstring selectedAdapterInfo;
		if (pAdapter) pAdapter->GetDesc(&adapterDescription);
		selectedAdapterInfo += L"Selected device:\n\t";
		selectedAdapterInfo += adapterDescription.Description;
		selectedAdapterInfo += L"\n\tAvailable Dedicated Video Memory: ";
		selectedAdapterInfo += std::to_wstring(adapterDescription.DedicatedVideoMemory / 1000000000.f);
		selectedAdapterInfo += L" GB";
		SKTBD_CORE_INFO(selectedAdapterInfo.c_str());
#endif

		// Release adapters to clean memory
		for (auto& currentAdapter : vAdapters) currentAdapter->Release(), currentAdapter = nullptr;

		// Verify compatibility with raytracing
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
		if (FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData))) || featureSupportData.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
		{
			SKTBD_CORE_WARN(L"DirectX Raytracing unsupported on this hardware. Raytracing functionalities disabled.");
			return;
		}

		// If compatible, enable mesh shaders support on this application
		m_HasDXR = true;

		// Verify compatibility with mesh shaders
		D3D12_FEATURE_DATA_D3D12_OPTIONS7 meshShaderFeatures = {};
		if (FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &meshShaderFeatures, sizeof(meshShaderFeatures))) 
			|| meshShaderFeatures.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
		{
			SKTBD_CORE_WARN(L"Mesh Shaders are not supported on the hardware.");
			return;
		}

		m_HasDXM = true;

		/*
		D3D12_FEATURE_DATA_D3D12_OPTIONS12 featureData12= {};
		if (FAILED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &featureData12, sizeof(featureData12))))
		{
			SKTBD_CORE_WARN(L"Culled primitives is not supported on current hardware.");
			return;
		}
		m_HasCulledPrimitives = featureData12.MSPrimitivesPipelineStatisticIncludesCulledPrimitives == D3D12_TRI_STATE_TRUE;

		*/
	}

	void D3DGraphicsContext::CreateFence()
	{
		SKTBD_CORE_TRACE("Creating fence..");

		// Create a fence object for CPU/GPU synchornisation
		D3D_CHECK_FAILURE(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_Fence.ReleaseAndGetAddressOf())));
	}

	void D3DGraphicsContext::CreateDescriptorSizes()
	{
		// When we will work with descriptors, we are going to need their sizes
		// Descriptor sizes can vary across GPUs, so we need to query this information
		// We cache the descriptors sizes on this class so that it is available when we need them for various descriptor types
		// It could be queried every time we need them instead

		m_RTVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_DSVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_CBVSRVUAVDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	void D3DGraphicsContext::Check4xMSAAQualitySupport()
	{
		// Check for 4x MultiSample Anti-Aliasing support
		// 4x is chosen because it guaranteed to be supported by DX12 hardware
		// and because it provides good improvement without being too expensive
		// However, we do have to check for the supported quality level

		D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multisampleQualityLevels = {};
		multisampleQualityLevels.Format = m_BackBufferFormat;							//
		multisampleQualityLevels.SampleCount = 4;										// 4 samples for a 4x MSAA
		multisampleQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;	//
		multisampleQualityLevels.NumQualityLevels = 0;									// Initially 0, will be determined by the next call

		D3D_CHECK_FAILURE(m_Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &multisampleQualityLevels, sizeof(multisampleQualityLevels)));

		// Since 4x should natively be supported, m_MSAAQuality should always be greater than 0
		m_MSAAQuality = multisampleQualityLevels.NumQualityLevels;
		SKTBD_CORE_ASSERT(m_MSAAQuality > 0, "Unexpected MSAA quality level.");
	}

	void D3DGraphicsContext::CheckSupportForGPUUploadHeaps()
	{
		SKTBD_CORE_TRACE("Checking for GPU upload heap support...");
		//D3D12_FEATURE_DATA_D3D12_OPTIONS16 gpuUploadSupport = {};
		//D3D_CHECK_FAILURE(m_Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &gpuUploadSupport, sizeof(gpuUploadSupport)));
		SKTBD_CORE_WARN("GPU upload heap is not supported on this device, please install DirectX 12 Agility SDK 1.711.3");
	}

	void D3DGraphicsContext::CreateCommandQueueAndCommandList()
	{
		SKTBD_CORE_TRACE("Creating command queue and command list..");

		// Start with describing the command queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// Specifies a command buffer that the GPU can execute. A direct command list doesn't inherit any GPU state.
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// No particular flags

		// Create the command queue
		D3D_CHECK_FAILURE(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_CommandQueue.ReleaseAndGetAddressOf())));

		// Create the command allocator
		D3D_CHECK_FAILURE(m_Device->CreateCommandAllocator(queueDesc.Type, IID_PPV_ARGS(m_DirectCommandAllocator.ReleaseAndGetAddressOf())));

		// Create the command list
		D3D_CHECK_FAILURE(m_Device->CreateCommandList(
			0,											// For single GPU operation, set this to zero. Otherwise use this to identify the adapter
			queueDesc.Type,
			m_DirectCommandAllocator.Get(),				// Associated command allocator on this list
			nullptr,									// Initial PipelineStateObject (nullptr when not drawing objects)
			IID_PPV_ARGS(m_CommandList.ReleaseAndGetAddressOf())
		));

		// Start off in a closed state
		// The first time we will refer to the command list we will reset it. Thus it needs to be closed beforehand
		m_CommandList->Close();
	}

	void D3DGraphicsContext::CreateSwapChain()
	{
		SKTBD_CORE_TRACE("Creating swap chain..");

		// First reset any previous swap chain that was created, in the event that a new swap chain is being created during runtime (i.e. if the user changes some settings!)
		m_SwapChain.Reset();

		// Then describe the characteristics of the swap chain
		// DXGI_SWAP_CHAIN_DESC contains a structure DXGI_MODE_DESC inside, which represents the properties of the back buffer
		// The main properties we are concerned with are width and height, as well as pixel format
		DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
		swapchainDesc.BufferDesc.Width = m_ClientWidth;
		swapchainDesc.BufferDesc.Height = m_ClientHeight;
		swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;								// 60/1 = 60 Hertz; Note: this seems to have no real effect on the refresh rate
		swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapchainDesc.BufferDesc.Format = m_BackBufferFormat;
		swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;					// Scaling (how the image is stretched over the monitor) is unspecified
		swapchainDesc.SampleDesc.Count = 1;													// MSAA is not supported on pipeline state buffers anymore, so sample count is 1 and quality is 0
		swapchainDesc.SampleDesc.Quality = 0;												// Use MSAA on render targets instead
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;						// Use the back buffer as a render target
		swapchainDesc.BufferCount = g_SwapChainBufferCount;									// Single, Double or Triple Buffering
		swapchainDesc.OutputWindow = m_MainWindow;											// Reference the window to which we need to swap these buffers onto
		swapchainDesc.Windowed = true;														// Always starts in windowed mode, the application layer is responsible for fullscreen
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;							// Discard the content of the back buffer after you present it. All Windows Store apps must use a flip effect
		swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;// Allow to switch to fullscreen or windowed mode in runtime

		// Create the swapchain
		// It uses the queue to perform a flush
		D3D_CHECK_FAILURE(m_DXGIInterface->CreateSwapChain(m_CommandQueue.Get(), &swapchainDesc, m_SwapChain.ReleaseAndGetAddressOf()));
	}

	void D3DGraphicsContext::CreateRaytracingInterfaces()
	{
		SKTBD_CORE_TRACE("Checking ray tracing interfaces..");

		if (!m_HasDXR) return;
		D3D_CHECK_FAILURE(m_Device->QueryInterface(IID_PPV_ARGS(m_DxrDevice.ReleaseAndGetAddressOf())));
		D3D_CHECK_FAILURE(m_CommandList->QueryInterface(IID_PPV_ARGS(m_DxrCommandList.ReleaseAndGetAddressOf())));
	}

	void D3DGraphicsContext::CreateMeshShaderInterfaces()
	{
		SKTBD_CORE_TRACE("Checking mesh shaders interfaces..");

		if (!m_HasDXM) return;
		D3D_CHECK_FAILURE(m_Device->QueryInterface(IID_PPV_ARGS(m_DxmDevice.ReleaseAndGetAddressOf())));
		D3D_CHECK_FAILURE(m_CommandList->QueryInterface(IID_PPV_ARGS(m_DxmCommandList.ReleaseAndGetAddressOf())));

	}

	void D3DGraphicsContext::Resize(int clientWidth, int clientHeight)
	{
		// Assign new dimensions if existing (we don't want a 0 dimension, would crash)
		SKTBD_CORE_ASSERT(clientWidth || clientHeight, "Impossible client surface dimensions");

		m_ClientWidth = clientWidth;
		m_ClientHeight = clientHeight;
		m_ClientResized = true;
	}

	void D3DGraphicsContext::OnResized()
	{
		// Do not perform unnecessary resizing
		if (!m_ClientResized)
			return;

		SKTBD_CORE_INFO("Resizing back buffers, two GPU flush expected..");

		// This function will be called everytime the application is being resized
		// It needs to re-create RTVs, a DSV, a viewport and scissor rectangles
		// First verify that we have all the required objects
		SKTBD_CORE_ASSERT(m_Device, "Device does not exists when trying to resize buffers!");
		SKTBD_CORE_ASSERT(m_SwapChain, "Swapchain does not exists when trying to resize buffers!");
		SKTBD_CORE_ASSERT(m_DirectCommandAllocator, "Command allocator does not exists when trying to resize buffers!");

		// Flush before changing any resource
		WaitUntilIdle();

		// Reset the command list to a NULL state
		D3D_CHECK_FAILURE(m_CommandList->Reset(m_DirectCommandAllocator.Get(), nullptr));

		// Release the previous resources we will be recreating.
		for (int i = 0; i < g_SwapChainBufferCount; ++i) m_SwapChainBuffers[i].Reset();
		m_DepthStencilBuffer.Reset();

		// Resize the swap chain.
		D3D_CHECK_FAILURE(m_SwapChain->ResizeBuffers(g_SwapChainBufferCount, m_ClientWidth, m_ClientHeight, m_BackBufferFormat, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH | DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING));

		// Reset the current back buffer to the first so we can properly re-create the render targets
		m_CurrentBackBuffer = 0;

		// Create new render target views
		CreateRenderTargetViews();

		// Create the detph/stencil buffer view
		CreateDepthStencilBuffer();

		// Transition the state of the depth/stencil resource from its initial state so it can be used as a depth buffer
		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;						// A transition barrier that indicates a transition of a set of subresources between different usages
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;							// 
		barrier.Transition.pResource = m_DepthStencilBuffer.Get();					// The resources in transition
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;				// The "before" usages of the SUBresources
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;			// The "after" usages of the subresources
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;	// The index of the subresource for transition: here we transition all the subresources at the same time
		m_CommandList->ResourceBarrier(1, &barrier);

		// Execute the resize commands
		Flush();

		// Update the viewport transform to cover the client area
		SetViewPort();

		// Update the scissor rectangles
		SetScissorRectangles();

		// Reset resized tracker
		m_ClientResized = false;
	}

	void D3DGraphicsContext::CreateDescriptorHeaps()
	{
		// Create basic descriptor heaps
		m_RWSRVDescriptorHeap.Create(L"Main SRV Descriptor Heap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096, false);
		m_SRVDescriptorHeap.Create(L"Main SRV Descriptor Heap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096, true);
		m_RTVDescriptorHeap.Create(L"Main RTV Descriptor Heap", D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 4096, false);
		m_DSVDescriptorHeap.Create(L"Main DSV Descriptor Heap", D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 4096, false);

		// Allocate the backbuffers and depth/stencil buffer
		m_RTVDescriptorHandle = m_RTVDescriptorHeap.Allocate(g_SwapChainBufferCount);
		m_DSVDescriptorHandle = m_DSVDescriptorHeap.Allocate(1);
		m_ImGuiHandle = m_SRVDescriptorHeap.Allocate(1);
	}

	void D3DGraphicsContext::CreateRenderTargetViews()
	{
		// Get the descriptor handle of the first buffer in the heap
		//D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
		D3DDescriptorHandle rtvHeapHandle = m_RTVDescriptorHandle;

		// For all buffers
		for (UINT i = 0u; i < (UINT)g_SwapChainBufferCount; ++i)
		{
			// First get the i-th buffer resource in the swap chain
			// This increases the COM reference count to the back buffer, so make sure to release it when we're done
			D3D_CHECK_FAILURE(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(m_SwapChainBuffers[i].ReleaseAndGetAddressOf())));

			// Create a render target view to this buffer
			// If the resource was created with a typed format (i.e. not typeless), then the D3D12_RENDER_TARGET_VIEW_DESC parameter can be null
			// It indicates to create a view to the first mipmap level of this resource with the format the resource was created with
			m_Device->CreateRenderTargetView(m_SwapChainBuffers[i].Get(), nullptr, rtvHeapHandle.GetCPUHandle());

			// Progress to the next descriptor handle
			rtvHeapHandle += m_RTVDescriptorSize;
		}
	}

	void D3DGraphicsContext::CreateDepthStencilBuffer()
	{
		// The depth/stencil buffer is a 2D texture, which differs from the render target views
		// Therefore, we must first initialise a D3D12_RESOURCE_DESC with the given charasteristics for this resource
		D3D12_RESOURCE_DESC dsvDesc = {};
		dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;			// Specify the dimensions of this resource
		dsvDesc.Alignment = 0;											// A power-of-two number which offsets the beginning of the resource (research memory alignment for more info)
		dsvDesc.Width = m_ClientWidth;									// The width of the texture in texels; for buffers, this is the number of bytes in the buffer
		dsvDesc.Height = m_ClientHeight;								// The height of the texture in texels
		dsvDesc.DepthOrArraySize = 1;									// The depth of the texture in texels, or the texture array size (for 1D and 2D textures)
		dsvDesc.MipLevels = 1;											// 
		dsvDesc.Format = m_DepthStencilFormat;							// 
		dsvDesc.SampleDesc.Count = 1;									// MSAA is not supported on pipeline state buffers anymore, so sample count is 1 and quality is 0
		dsvDesc.SampleDesc.Quality = 0;									// Use MSAA on render targets instead
		dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;					// Layout is unknown, which means it is likely adapter-dependent. The driver chooses the most efficient layout based on the other properties
		dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;		// Allows a depth stencil view to be created for the resource, as well as enables the resource transitions to write/read on the depth buffer

		// Describe the properties of the heap (chunk of memory) on the GPU for this resource
		D3D12_HEAP_PROPERTIES gpuHeapProperties = {};
		gpuHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;					 // Optimised. This is where we commit resources that will be solely accessed by the GPU. The CPU will never read or write to the depth buffer
		gpuHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN; // 
		gpuHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;	 // 
		gpuHeapProperties.CreationNodeMask = 0u;							 // Passing zero is equivalent to passing one, in order to simplify the usage of single-GPU adapters.
		gpuHeapProperties.VisibleNodeMask = 0u;								 // Passing zero is equivalent to passing one, in order to simplify the usage of single-GPU adapters.

		// Describe an optimised clear value to be used when clearing the buffer
		// Clear calls that match the optimised clear value can potentially be faster than otherwise
		// NULL can also be used to not specify a clear value
		D3D12_CLEAR_VALUE optimisedClearValue = {};
		optimisedClearValue.Format = m_DepthStencilFormat;		//
		optimisedClearValue.DepthStencil.Depth = 1.f;			// The depth buffer will be set to all ones
		optimisedClearValue.DepthStencil.Stencil = 0u;			// The stencil buffer will be set to all zeros

		// Create the texture resource
		D3D_CHECK_FAILURE(m_Device->CreateCommittedResource(
			&gpuHeapProperties,									//
			D3D12_HEAP_FLAG_NONE,								// 
			&dsvDesc,											// 
			D3D12_RESOURCE_STATE_COMMON,						// Use this parameter to set the initial state of the resource when it is created. We will have to transition the state to bind it to the pipeline 
			&optimisedClearValue,								// 
			IID_PPV_ARGS(m_DepthStencilBuffer.ReleaseAndGetAddressOf())	// 
		));

		// Create the Depth/Stencil view
		// If the resource was created with a typed format (i.e. not typeless), then the D3D12_DEPTH_STENCIL_VIEW_DESC parameter can be null
		// It indicates to create a view to the first mipmap level of this resource with the format the resource was created with
		m_Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), nullptr, DepthStencilView());
	}

	void D3DGraphicsContext::SetViewPort()
	{
		// Describe the viewport charasteristics to fit the entire window with a normal depth buffer range
		m_Viewport.TopLeftX = 0.f;
		m_Viewport.TopLeftY = 0.f;
		m_Viewport.Width = static_cast<float>(m_ClientWidth);
		m_Viewport.Height = static_cast<float>(m_ClientHeight);
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 1.f;
	}

	void D3DGraphicsContext::SetScissorRectangles()
	{
		// For now we don't have any HUDs, so assign the area to fit the client surface
		m_ScissorRect = { 0, 0, m_ClientWidth, m_ClientHeight };
	}

	void D3DGraphicsContext::Reset()
	{
		D3D_CHECK_FAILURE(m_CommandList->Reset(m_DirectCommandAllocator.Get(), nullptr));
	}

	void D3DGraphicsContext::Flush()
	{
		// Execute the new commands and wait until completion
		D3D_CHECK_FAILURE(m_CommandList->Close());
		ID3D12CommandList* cmdListsToExecute[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(_countof(cmdListsToExecute), cmdListsToExecute);
		WaitUntilIdle();
	}

	void D3DGraphicsContext::WaitUntilIdle()
	{
		SKTBD_CORE_WARN("Flushing GPU commands -- Woosh!");

		// Advance the fence value to mark commands up to this fence point
		++m_CurrentFence;

		// Set a new fence point on the command queue. Because we are on the GPU timeline, the new fence point won't
		// be set until the GPU finishes processing all the commands prior to this Signal()
		D3D_CHECK_FAILURE(m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence));

		// Wait until the GPU has completed commands up to the fence point
		if (m_Fence->GetCompletedValue() < m_CurrentFence)
		{
			HANDLE eventHandle = CreateEventEx(nullptr, NULL, NULL, EVENT_ALL_ACCESS);
			SKTBD_CORE_ASSERT(eventHandle != NULL, "Fence event created an invalid handle!");

			// Fire the previously created event when GPU hits current fence
			D3D_CHECK_FAILURE(m_Fence->SetEventOnCompletion(m_CurrentFence, eventHandle));

			// Wait until the GPU hits the current fence and the event is received
			WaitForSingleObject(eventHandle, INFINITE);

			// Close the event so that it does not exist anymore
			CloseHandle(eventHandle);
		}
	}

	void D3DGraphicsContext::SetDeferredReleasesFlag()
	{
		m_DeferredFlags[GetCurrentFrameResourceIndex()] = 1;
	}

	void D3DGraphicsContext::DeferredRelease(IUnknown* resource)
	{
		const uint64_t frame = GetCurrentFrameResourceIndex();

		// Lock this function, prevents threads from over writing deferrals.
		std::lock_guard lock(m_DeferredMutex);

		m_DeferredReleases[frame].push_back(resource);
		SetDeferredReleasesFlag();
	}

	void __declspec(noinline) D3DGraphicsContext::ProcessDeferrals(uint32_t frame)
	{
		std::lock_guard lock(m_DeferredMutex);

		m_DeferredFlags[frame] = 0;

		// Process deferred descriptors.
		m_RWSRVDescriptorHeap.ProcessDeferredFree(frame);
		m_SRVDescriptorHeap.ProcessDeferredFree(frame);
		m_RTVDescriptorHeap.ProcessDeferredFree(frame);
		m_DSVDescriptorHeap.ProcessDeferredFree(frame);

		//Clear the deferred resources.
		std::vector<IUnknown*>& resources{ m_DeferredReleases[frame] };
		if(!resources.empty())
		{
			for(const auto& resource : resources)
			{
				/*if(resource != nullptr)
				{
					resource->Release();
				}*/
			}
			resources.clear();
		}

	}

	void D3DGraphicsContext::ExecuteGraphicsCommandList(void* args)
	{

		if(args != nullptr)
		{
			auto* pGraphicsCommandList = static_cast<ID3D12GraphicsCommandList*>(args);
			// Execute the new commands and wait until completion
			D3D_CHECK_FAILURE(pGraphicsCommandList->Close());
			ID3D12CommandList* cmdListsToExecute[] = { pGraphicsCommandList };
			m_CommandQueue->ExecuteCommandLists(1, cmdListsToExecute);
		}
		else
		{
			// Execute the new commands and wait until completion
			D3D_CHECK_FAILURE(m_CommandList->Close());
			ID3D12CommandList* cmdListsToExecute[] = { m_CommandList.Get() };
			m_CommandQueue->ExecuteCommandLists(1, cmdListsToExecute);
		}

	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3DGraphicsContext::CurrentBackBufferView() const
	{
		// Offset to the RTV of the current back buffer and return the location of the according descriptor
		//D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
		//handle.ptr = m_RTVHeap->GetCPUDescriptorHandleForHeapStart().ptr + m_CurrentBackBuffer * m_RTVDescriptorSize;		// Note: Overflow higly improbable as m_CurrentBackBuffer usually ranges in [0,3]
		//return handle;
		return (m_RTVDescriptorHandle + m_CurrentBackBuffer * m_RTVDescriptorSize).GetCPUHandle();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE D3DGraphicsContext::DepthStencilView() const
	{
		// There is only one descriptor for this heap, just return where it starts
		//return m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
		return m_DSVDescriptorHandle.GetCPUHandle();
	}
}