#pragma once
#include "Skateboard/Renderer/GraphicsContext.h"
#include "Skateboard/Platform.h"
#include "D3D.h"
#include "D3DFrameResources.h"
#include "Memory/D3DMemoryAllocator.h"

namespace Skateboard
{
	class D3DGraphicsContext : public GraphicsContext
	{
	public:
		HMODULE m_GpuCaptureLib;
		HMODULE m_GpuTimingLib;
	public:
		// Let's not make things confusing and initialise everything in the constructor
		D3DGraphicsContext(HWND window, const PlatformProperties& props);
		// And release all in destructor
		~D3DGraphicsContext();
		void Clean() override;

		virtual void Present(void* args = nullptr) final override;
		
		// Public functions to resize the buffers according to the new dimensions stored in lParam based on the size description in wParam
		virtual void Resize(int clientWidth, int clientHeight) final override;
		virtual void OnResized() final override;

		// These two functions should be used before and after drawing objects in the inherited application
		// We cannot inline them otherwise the compiler gets confused and does not find the definitions anymore
		virtual void StartDraw(float4 clearColour) final override;
		virtual void EndDraw() final override;
		virtual void SetRenderTargetToBackBuffer() final override;

		virtual void Reset() final override;
		virtual void Flush() final override;
		virtual void WaitUntilIdle() final override;

		void ExecuteGraphicsCommandList(void* args = nullptr);

		void SetDeferredReleasesFlag();
		void DeferredRelease(IUnknown* resource);
		void ProcessDeferrals(uint32_t frame);

	public:
		// Inline

		_NODISCARD D3DDescriptorHandle GetImGuiDescriptorHandle()	const { return m_ImGuiHandle; }
		
		_NODISCARD uint64_t GetCurrentFrameResourceIndex() const final override { return m_FrameResources.GetCurrentFrameResourceIndex(); }
		_NODISCARD int32_t GetClientSizeX() const final override { return m_ClientWidth; }
		_NODISCARD int32_t GetClientSizeY() const final override { return m_ClientHeight; }
		_NODISCARD float GetClientAspectRatio() const final override { return static_cast<float>(m_ClientWidth) / m_ClientHeight; };

		_NODISCARD DXGI_FORMAT GetBackBufferFormat()		const	{ return m_BackBufferFormat; }
		_NODISCARD DXGI_FORMAT GetDepthStencilFormat()	const	{ return m_DepthStencilFormat; }

		_NODISCARD ID3D12Device* Device()							const	{ return m_Device.Get(); }
		_NODISCARD ID3D12GraphicsCommandList* GraphicsCommandList() const	{ return m_CommandList.Get(); }
		_NODISCARD ID3D12CommandQueue* CommandQueue()				const	{ return m_CommandQueue.Get(); }
		_NODISCARD ID3D12CommandAllocator* CommandAllocator()		const	{ return m_DirectCommandAllocator.Get(); }

		_NODISCARD bool IsRaytracingSupported() const final override { return m_HasDXR; }

		_NODISCARD ID3D12Device5* RaytracingDevice() const { return m_DxrDevice.Get(); }
		_NODISCARD ID3D12GraphicsCommandList4* RaytracingCommandList() const { return m_DxrCommandList.Get(); }

		_NODISCARD ID3D12Device8* MeshShaderDevice() const { return m_DxmDevice.Get(); }
		_NODISCARD ID3D12GraphicsCommandList6* MeshShaderCommandList() const { return m_DxmCommandList.Get(); }

		_NODISCARD D3DDescriptorHeap* GetSrvHeap()	{ return &m_SRVDescriptorHeap; }
		_NODISCARD D3DDescriptorHeap* GetRWSrvHeap()	{ return &m_RWSRVDescriptorHeap; }
		_NODISCARD D3DDescriptorHeap* GetDsvHeap()	{ return &m_DSVDescriptorHeap; }
		_NODISCARD D3DDescriptorHeap* GetRtvHeap()	{ return &m_RTVDescriptorHeap; }

		_NODISCARD D3D12_RECT GetScissorsRect()			{ return m_ScissorRect; }
		_NODISCARD D3D12_RECT GetScissorsRect() const	{ return m_ScissorRect; }
		_NODISCARD D3D12_VIEWPORT GetViewport()			{ return m_Viewport; }
		_NODISCARD D3D12_VIEWPORT GetViewport() const	{ return m_Viewport; }

		_NODISCARD ID3D12Resource* GetCurrentBackBuffer() { return m_SwapChainBuffers[m_CurrentBackBuffer].Get(); }
		_NODISCARD const ID3D12Resource* GetCurrentBackBuffer() const { return m_SwapChainBuffers[m_CurrentBackBuffer].Get(); }

		_NODISCARD ID3D12Fence* GetFence() { return m_Fence.Get(); }
		_NODISCARD const ID3D12Fence* GetFence() const { return m_Fence.Get(); }

		_NODISCARD D3DFrameResources* GetCurrentFrameResource() { return &m_FrameResources; }
		_NODISCARD const D3DFrameResources* GetCurrentFrameResource() const { return &m_FrameResources; }

		// We need to be able to access the descriptors stored in the respective heaps of our buffer
		_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
		_NODISCARD D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

		_NODISCARD D3D_FEATURE_LEVEL GetMinimumFeatureLevel() const { return m_D3DFeatureLevel; }


	private:
		void CreateDevice();
		void CreateFence();
		void CreateDescriptorSizes();
		void Check4xMSAAQualitySupport();
		void CheckSupportForGPUUploadHeaps();
		void CreateCommandQueueAndCommandList();
		void CreateSwapChain();
		void CreateRaytracingInterfaces();
		void CreateMeshShaderInterfaces();
		void CreateRenderTargetViews();
		void CreateDepthStencilBuffer();
		void SetViewPort();
		void SetScissorRectangles();
		void CreateDescriptorHeaps();
	private:
		// Derived class should set these in derived constructor to customize starting values.
		D3D_FEATURE_LEVEL m_D3DFeatureLevel{};
		D3D_DRIVER_TYPE	m_D3DDriverType;					// Driver type options
		DXGI_FORMAT		m_BackBufferFormat;					// Format of the back buffer
		DXGI_FORMAT		m_DepthStencilFormat;				// Format of the depth/stencil buffer

		// Window settings, mostly in regards with resizing
		int32_t			m_ClientWidth, m_ClientHeight;			// Width and height of the client area (does not include the top bar and menus, this is the drawable surface)
		bool			m_Vsync;								// Note: Fullscreen in this framework is handled by the platform API

		// MSAA support (not used in this project, but could be enabled)
		bool m_MSAAEnable;
		UINT m_MSAAQuality;

		// Working environment
		HWND m_MainWindow;

		// COM Objects
		// Device and DXGI Factory
		Microsoft::WRL::ComPtr<ID3D12Device>				m_Device;						// The device is the display adapter, like the graphics card
		Microsoft::WRL::ComPtr<IDXGIFactory4>				m_DXGIInterface;				// The interface to generate any DXGI objects (version 4 provides more functionalities, such as EnumWarpAdapters)

		// Fence
		UINT64												m_CurrentFence;					// Identify a fence point in time. Everytime we mark a new fence point, increment this integer
		Microsoft::WRL::ComPtr<ID3D12Fence>					m_Fence;						// The fence object to synchronise the CPU/GPU

		// Command Objects
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_CommandQueue;					// The command queue we will use for this application to submit commands to the GPU
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		m_DirectCommandAllocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	m_CommandList;

		// SwapChain
		Microsoft::WRL::ComPtr<IDXGISwapChain>				m_SwapChain;

		// Buffers
		static const int32_t								g_SwapChainBufferCount = GRAPHICS_SETTINGS_SWAPCHAINBUFFERS;
		int32_t												m_CurrentBackBuffer;
	
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_SwapChainBuffers[g_SwapChainBufferCount];	// The back buffers to use in the swapchain
		Microsoft::WRL::ComPtr<ID3D12Resource>				m_DepthStencilBuffer;						// The depth/stencil buffer

		// Descriptor sizes
		UINT m_RTVDescriptorSize;
		UINT m_DSVDescriptorSize;
		UINT m_CBVSRVUAVDescriptorSize;

		// Viewports and scissor rectangles
		D3D12_VIEWPORT	m_Viewport;							// The viewport to which the 3D world will be rendered onto
		D3D12_RECT		m_ScissorRect;						// Pixels outside of this rectangle are culled (not rasterized onto the back buffer)

		// Frame Resources
		D3DFrameResources									m_FrameResources;

		// Raytracing
		Microsoft::WRL::ComPtr<ID3D12Device5>				m_DxrDevice;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4>	m_DxrCommandList;

		// Mesh Shaders
		Microsoft::WRL::ComPtr<ID3D12Device8>				m_DxmDevice;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>	m_DxmCommandList;

		// Bools
		bool	m_HasDXR;			// A bool that will be checked on init to initialise raytracing components
		bool	m_HasDXM;			// A bool that will be checked on init to initialise mesh shaders components
		bool	m_ClientResized;

		std::vector<IUnknown*> m_DeferredReleases[SKTBD_SETTINGS_NUMFRAMERESOURCES]{};
		uint32_t m_DeferredFlags[SKTBD_SETTINGS_NUMFRAMERESOURCES];
		std::mutex m_DeferredMutex;

		D3DDescriptorHeap m_RWSRVDescriptorHeap;		// Read-Write
		D3DDescriptorHeap m_SRVDescriptorHeap;			// Write only as shader visible
		D3DDescriptorHeap m_RTVDescriptorHeap;
		D3DDescriptorHeap m_DSVDescriptorHeap;

		D3DDescriptorHandle m_ImGuiHandle;
		D3DDescriptorHandle m_RTVDescriptorHandle;
		D3DDescriptorHandle m_DSVDescriptorHandle;
	};
}
