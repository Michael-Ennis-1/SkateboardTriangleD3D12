#include "sktbdpch.h"
#include "D3DPipeline.h"
#include "Platform/Windows/WindowsPlatform.h"
#include "Platform/DirectX12/D3DGraphicsContext.h"
#include "Platform/DirectX12/D3DBuffer.h"
#include "Platform/DirectX12/Memory/D3DTableAllocator.h"



namespace Skateboard
{
	void LoadShader(const wchar_t* filename, void** blob)
	{
		// The blob is expected to be released (free) and therfore the below code is disabled.
		// Use ReleaseAndGetAddressOf() on the ComPtr of your IDxcBlob as the 'blob' parameter for correct behaviour.
		//		IDxcBlob* pShaderBuffer = static_cast<IDxcBlob*>(*blob);
		// 		if (pShaderBuffer) pShaderBuffer->Release();
#ifndef SKTBD_SHIP
		IDxcBlob* pShaderBuffer = static_cast<IDxcBlob*>(*blob);
		SKTBD_CORE_ASSERT(pShaderBuffer == nullptr, "Invalid shader buffer as it already exists. Use ReleaseAndGetAddressOf() on the ComPtr of your IDxcBlob as the 'blob' parameter for correct behaviour.");
#endif

		// Loading is expected to retrieve the raw binary data of a CSO file
		Microsoft::WRL::ComPtr<IDxcUtils> utils;
		HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf()));
		hr = utils->LoadFile(filename, nullptr, reinterpret_cast<IDxcBlobEncoding**>(blob));
		D3D_CHECK_FAILURE(hr);
	}

	D3DRasterizationPipeline::D3DRasterizationPipeline(const std::wstring& debugName, const RasterizationPipelineDesc& desc) :
		RasterizationPipeline(debugName, desc),
		m_PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
	{
		// Load Shaders
		LoadAllShaders();
		// Create Root signature
		BuildRootSignature();
		// Build PSO
		BuildPSO();
	}

	void D3DRasterizationPipeline::Bind(uint32_t startInstanceLocation)
	{
		ID3D12GraphicsCommandList* pCommandList = gD3DContext->GraphicsCommandList();
		pCommandList->IASetPrimitiveTopology(m_PrimitiveTopology);
		pCommandList->SetPipelineState(m_PipelineStateObject.Get());
		pCommandList->SetGraphicsRootSignature(m_RootSignature.Get());

		uint32_t rootInputSlot = 0u;
		pCommandList->SetGraphicsRoot32BitConstant(rootInputSlot++, startInstanceLocation, 0u);
		for (const ShaderResourceDesc& desc : m_Desc.vCBV)
		{
			D3DUploadBuffer* pBuffer = static_cast<D3DUploadBuffer*>(desc.pResource);
			pCommandList->SetGraphicsRootConstantBufferView(rootInputSlot, pBuffer->GetGPUVirtualAddress());
			++rootInputSlot;
		}
		for (const ShaderResourceDesc& desc : m_Desc.vDescriptorTables)
		{
			D3DDescriptorTable* table = static_cast<D3DDescriptorTable*>(desc.pDescriptorTable);
			pCommandList->SetGraphicsRootDescriptorTable(rootInputSlot, table->GetGPUHandle());
			++rootInputSlot;
		}

		if(m_DescriptorTableRootHandle.IsValid())
			pCommandList->SetGraphicsRootDescriptorTable(rootInputSlot, m_DescriptorTableRootHandle.GetGPUHandle());
	}

	void D3DRasterizationPipeline::Release()
	{
	}

	void D3DRasterizationPipeline::LoadAllShaders()
	{
		// Can we do that?
		/*LoadShader(m_Desc.VertexShader.FileName, (void**)m_VertexShader.ReleaseAndGetAddressOf());
		D3D12_EXPORT_DESC desc = {};
		std::vector<D3D12_EXPORT_DESC> vShaderExports;
		desc.Name = m_Desc.VertexShader.EntryPoint;
		desc.ExportToRename = nullptr;
		desc.Flags = D3D12_EXPORT_FLAG_NONE;
		vShaderExports.push_back(desc);
		desc.Name = m_Desc.HullShader.EntryPoint;
		if(desc.Name) vShaderExports.push_back(desc);
		desc.Name = m_Desc.DomainShader.EntryPoint;
		if (desc.Name) vShaderExports.push_back(desc);
		desc.Name = m_Desc.GeometryShader.EntryPoint;
		if (desc.Name) vShaderExports.push_back(desc);
		desc.Name = m_Desc.PixelShader.EntryPoint;
		vShaderExports.push_back(desc);

		D3D12_DXIL_LIBRARY_DESC libDesc = {};
		libDesc.pExports = vShaderExports.data();
		libDesc.NumExports = vShaderExports.size();
		libDesc.DXILLibrary = {
			m_VertexShader->GetBufferPointer(),
			m_VertexShader->GetBufferSize()
		};*/

		LoadShader(m_Desc.VertexShader.FileName, (void**)m_VertexShader.ReleaseAndGetAddressOf());
		if (m_Desc.HullShader.FileName) LoadShader(m_Desc.HullShader.FileName, (void**)m_HullShader.ReleaseAndGetAddressOf());
		if (m_Desc.DomainShader.FileName) LoadShader(m_Desc.DomainShader.FileName, (void**)m_DomainShader.ReleaseAndGetAddressOf());
		if (m_Desc.GeometryShader.FileName) LoadShader(m_Desc.GeometryShader.FileName, (void**)m_GeoShader.ReleaseAndGetAddressOf());
		if (m_Desc.PixelShader.FileName) LoadShader(m_Desc.PixelShader.FileName, (void**)m_PixelShader.ReleaseAndGetAddressOf());
	}

	void D3DRasterizationPipeline::BuildRootSignature()
	{
		//Before filling out the root signature, grab the heap allocator.
		auto* pSrvHeap = const_cast<D3DDescriptorHeap*>(gD3DContext->GetSrvHeap());

		// Using auto here, because we may need to change the device interface
		// in order to access special api. 
		auto* pDevice = gD3DContext->Device();

		std::vector<D3D12_ROOT_PARAMETER1> vRootParams;

		// Define a root constant that will be used to index the instance buffer
		// We'll put it in register(0, space 1)
		D3D12_ROOT_PARAMETER1 param = {};
		param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		param.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		param.Constants.ShaderRegister = 0u;
		param.Descriptor.RegisterSpace = 1u;
		param.Constants.Num32BitValues = 1u;
		vRootParams.emplace_back(std::move(param));

		for (const ShaderResourceDesc& desc : m_Desc.vCBV)
		{
			D3D12_ROOT_PARAMETER1 param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.ShaderVisibility = ShaderVisibilityToD3D(desc.ShaderVisibility);
			param.Descriptor.ShaderRegister = desc.ShaderRegister;
			param.Descriptor.RegisterSpace = desc.ShaderRegisterSpace;
			vRootParams.emplace_back(std::move(param));
		}

		std::vector<D3D12_DESCRIPTOR_RANGE1> vSuppliedTableRanges;
		vSuppliedTableRanges.reserve(m_Desc.vDescriptorTables.size());
		for (const ShaderResourceDesc& desc : m_Desc.vDescriptorTables)
		{
			D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
			descriptorRange.RangeType = ShaderDescriptorTableTypeToD3D(desc.pDescriptorTable->GetTableType());
			descriptorRange.NumDescriptors = desc.pDescriptorTable->GetDescriptorCount();
			descriptorRange.BaseShaderRegister = desc.ShaderRegister;
			descriptorRange.RegisterSpace = desc.ShaderRegisterSpace;
			descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			vSuppliedTableRanges.emplace_back(std::move(descriptorRange));

			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = ShaderVisibilityToD3D(desc.ShaderVisibility);
			descriptorTable.DescriptorTable.NumDescriptorRanges = 1;
			descriptorTable.DescriptorTable.pDescriptorRanges = &vSuppliedTableRanges.back();
			vRootParams.emplace_back(std::move(descriptorTable));
		}

		// Construct a descriptor table for all other types of resources
		// This is done so that we can bind a lot more resources than what the Root Signature limits us to
		const uint32_t descriptorCount = static_cast<uint32_t>(m_Desc.vSRV.size() + m_Desc.vUAV.size());
		std::vector<D3D12_DESCRIPTOR_RANGE1> vDescriptorRanges;
		if (descriptorCount)
		{
			D3DDescriptorHandle tempHandle = m_DescriptorTableRootHandle = pSrvHeap->Allocate(descriptorCount);
			const uint32_t handleSize = pSrvHeap->GetDescriptorIncrementSize();

			for (const ShaderResourceDesc& desc : m_Desc.vSRV)		// SRVs
			{
				D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
				descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				descriptorRange.NumDescriptors = 1;
				descriptorRange.BaseShaderRegister = desc.ShaderRegister;
				descriptorRange.RegisterSpace = desc.ShaderRegisterSpace;
				descriptorRange.OffsetInDescriptorsFromTableStart = static_cast<uint32_t>(vDescriptorRanges.size());
				descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
				vDescriptorRanges.emplace_back(std::move(descriptorRange));

				switch (desc.pResource->GetType())
				{
				case GPUResourceType_DefaultBuffer:
				{
					D3DDefaultBuffer* pBuffer = static_cast<D3DDefaultBuffer*>(desc.pResource);
					gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					tempHandle += handleSize;
					break;
				}
				case GPUResourceType_UploadBuffer:
				{
					D3DUploadBuffer* pBuffer = static_cast<D3DUploadBuffer*>(desc.pResource);
					gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					tempHandle += handleSize;
					break;
				}
				case GPUResourceType_Texture2D:
				{
					D3DTexture* pBuffer = static_cast<D3DTexture*>(desc.pResource);
					gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					tempHandle += handleSize;
					break;
				}
				case GPUResourceType_UnorderedAccessBuffer:
				{
					D3DUnorderedAccessBuffer* pBuffer = static_cast<D3DUnorderedAccessBuffer*>(desc.pResource);
					gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					tempHandle += handleSize;
					break;
				}
				case GPUResourceType_FrameBuffer:
				{
					// TODO: GetSRVHandle(0) -> Do we need to index anything else really?
					D3DFrameBuffer* pBuffer = static_cast<D3DFrameBuffer*>(desc.pResource);
					gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle(0).GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					tempHandle += handleSize;
					break;
				}
				default:
					SKTBD_CORE_ASSERT(false, "unsupported");
					break;
				}
			}

			// UAVs
			for (const ShaderResourceDesc& desc : m_Desc.vUAV)
			{
				D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
				descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				descriptorRange.NumDescriptors = 1;
				descriptorRange.BaseShaderRegister = desc.ShaderRegister;
				descriptorRange.RegisterSpace = desc.ShaderRegisterSpace;
				descriptorRange.OffsetInDescriptorsFromTableStart = static_cast<uint32_t>(vDescriptorRanges.size());
				descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
				vDescriptorRanges.emplace_back(std::move(descriptorRange));

				D3DUnorderedAccessBuffer* pBuffer = static_cast<D3DUnorderedAccessBuffer*>(desc.pResource);
				gD3DContext->Device()->CopyDescriptors(1, &tempHandle.GetCPUHandle(), &handleSize, 1, &pBuffer->GetUAVHandle().GetCPUHandle(), &handleSize, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				tempHandle += handleSize;
			}

			// Add a descriptor table with all ranges
			SKTBD_CORE_ASSERT(!vDescriptorRanges.empty(), "The logic went wrong, this vector should never be empty considering descriptorCount > 0");
			m_DescriptorTableRootHandle.SetValid(true);
			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// TODO: Maybe not all the time??
			descriptorTable.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(vDescriptorRanges.size());
			descriptorTable.DescriptorTable.pDescriptorRanges = vDescriptorRanges.data();
			vRootParams.emplace_back(std::move(descriptorTable));
		}
		else m_DescriptorTableRootHandle.SetValid(false);

		// Add the supplied samplers
		std::vector<D3D12_STATIC_SAMPLER_DESC> vStaticSamplers;
		vStaticSamplers.reserve(m_Desc.vStaticSamplers.size());
		for (const SamplerDesc& samplerDesc : m_Desc.vStaticSamplers)
	{
		D3D12_STATIC_SAMPLER_DESC d3dSamplerDesc = {};
		d3dSamplerDesc.ShaderRegister = samplerDesc.ShaderRegister;
		d3dSamplerDesc.RegisterSpace = samplerDesc.ShaderRegisterSpace;
		d3dSamplerDesc.Filter = SamplerFilterToD3D(samplerDesc.Filter);
		d3dSamplerDesc.AddressU = SamplerModeToD3D(samplerDesc.Mode);
		d3dSamplerDesc.AddressV = SamplerModeToD3D(samplerDesc.Mode);
		d3dSamplerDesc.AddressW = SamplerModeToD3D(samplerDesc.Mode);
		d3dSamplerDesc.MipLODBias = samplerDesc.MipMapLevelOffset;
		d3dSamplerDesc.MaxAnisotropy = samplerDesc.MaxAnisotropy;
		d3dSamplerDesc.ComparisonFunc = SamplerComparisonFunctionToD3D(samplerDesc.ComparisonFunction);
		d3dSamplerDesc.BorderColor = SamplerBorderColourToD3D(samplerDesc.BorderColour);
		d3dSamplerDesc.MinLOD = samplerDesc.MipMapMinSampleLevel;
		d3dSamplerDesc.MaxLOD = samplerDesc.MipMapMaxSampleLevel;
		d3dSamplerDesc.ShaderVisibility = ShaderVisibilityToD3D(samplerDesc.ShaderVisibility);
		vStaticSamplers.emplace_back(std::move(d3dSamplerDesc));
	}

		// Describe the root signature
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.NumParameters = static_cast<UINT>(vRootParams.size());
		rootSignatureDesc.Desc_1_1.pParameters = vRootParams.data();
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(vStaticSamplers.size());
		rootSignatureDesc.Desc_1_1.pStaticSamplers = vStaticSamplers.data();
		rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Serialize a root signature into a blob that can be passed into the CreateRootSignature function
		// The version of this root signature will be 1.1
		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		Microsoft::WRL::ComPtr<ID3DBlob> error;
		if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), error.GetAddressOf())))
		{
			SKTBD_CORE_CRITICAL((char*)error->GetBufferPointer());
			throw std::exception();
		}

		D3D_CHECK_FAILURE(gD3DContext->Device()->CreateRootSignature(
			0,															// For single GPU, set this to 0
			signature->GetBufferPointer(),								// Pointer to the serialized signature
			signature->GetBufferSize(),									// Length, in bytes, of the signature to be read
			IID_PPV_ARGS(m_RootSignature.ReleaseAndGetAddressOf())));

#ifndef SKTBD_SHIP
		m_RootSignature->SetName((m_DebugName + L" - RootSignature").c_str());
#endif // !SKTBD_SHIP

	}

	


	void D3DRasterizationPipeline::BuildPSO()
	{
		// Create the input layout based on the given description layout
		const BufferLayout& layout = m_Desc.InputVertexLayout;
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
		for (const BufferElement& element : layout)
		{
			const DXGI_FORMAT format = ShaderDataTypeToD3D(element.Type);
			inputElementDescs.push_back({ element.SemanticName.c_str(), 0, format, element.InputSlot , element.Offset, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		}

		// Identify primitive topology
		bool tessellationEnabled = false;
		switch (m_Desc.Type)
		{
		case RasterizationPipelineType_DepthOnly:
		case RasterizationPipelineType_Default:				m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;												break;
		case RasterizationPipelineType_Tessellation_Tri:	m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST; tessellationEnabled = true;		break;
		case RasterizationPipelineType_Tessellation_Quad:	m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST; tessellationEnabled = true;		break;
		default:											SKTBD_CORE_ASSERT(false, "Unsupported pipeline.");														break;
		}

		// Describe the rasterizer state (how will objects be rasterized ?)
		D3D12_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = m_Desc.Wireframe ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;		// Specify which fill mode to use when rendering
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;														// Specify which face to cull (none, front, back)
		rasterizerDesc.FrontCounterClockwise = FALSE;														// Determines if a triangle is front or back facing
		rasterizerDesc.DepthBias = m_Desc.DepthBias;														// Depth value added to a given pixel
		rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;										// Maximum depth bias for a given pixel
		rasterizerDesc.SlopeScaledDepthBias = m_Desc.DepthBias ? 1.f : D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;// Scalar on a given pixel's slope. More info on Depth Bias on MSDN
		rasterizerDesc.DepthClipEnable = TRUE;																// Specifies whether or not to enable clipping based on distance
		rasterizerDesc.MultisampleEnable = FALSE;															// Set to TRUE to use the quadrilateral line anti-aliasing algorithm and to FALSE to use the alpha line anti-aliasing algorithm
		rasterizerDesc.AntialiasedLineEnable = FALSE;														// Specifies whether to enable line antialiasing
		rasterizerDesc.ForcedSampleCount = 0;																// The sample count that is forced while UAV rendering or rasterizing. 0 is not forced
		rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;						// Identifies whether conservative rasterization is on or off

		// Describe a blend state
		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = FALSE;										// Specifies whether to use alpha-to-coverage as a multisampling technique when setting a pixel to a render target
		blendDesc.IndependentBlendEnable = FALSE;										// Specifies whether to enable independent blending in simultaneous render targets (FALSE only uses RenderTarget[0])
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =				// These correspond to the eight render targets that can be bound to the output-merger stage at one time
		{
			FALSE,FALSE,																// No blending, No logical operations
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,						// The following describes how the blend should happen but we do not do it here
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0u; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		{
			blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
		}

		// Describe the depth/stencil state
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
		{
			D3D12_STENCIL_OP_KEEP,													// The operation to perform when the stencil test fails
			D3D12_STENCIL_OP_KEEP,													// The operation to perform when the stencil test passes but depth fails
			D3D12_STENCIL_OP_KEEP,													// The operation to perform when both tests passes
			D3D12_COMPARISON_FUNC_ALWAYS											// The function that compares new stencil data against existing stencil data
		};
		D3D12_DEPTH_STENCIL_DESC dsDesc = {};
		dsDesc.DepthEnable = TRUE;													// Specify whether to enable depth testing
		dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;							// Enable or disable writing to sections or all of the depth testing buffer
		dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;								// Function to compare new depth data to existing depth data
		dsDesc.StencilEnable = FALSE;												// Spicify whether to enable stencil testing
		dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;					// Identify a portion of the depth-stencil buffer for reading stencil data
		dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;					// Identify a portion of the depth-stencil buffer for writing stencil data
		dsDesc.FrontFace = defaultStencilOp;										// for pixels whose surface normal is facing towards the camera
		dsDesc.BackFace = defaultStencilOp;											// for pixels whose surface normal is facing away from the camera

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs.data(), (uint32_t)inputElementDescs.size() };
		psoDesc.pRootSignature = m_RootSignature.Get();
		if (m_VertexShader.Get())	psoDesc.VS = { m_VertexShader->GetBufferPointer(), m_VertexShader->GetBufferSize() };
		if (m_HullShader.Get())		psoDesc.HS = { m_HullShader->GetBufferPointer(), m_HullShader->GetBufferSize() };
		if (m_DomainShader.Get())	psoDesc.DS = { m_DomainShader->GetBufferPointer(), m_DomainShader->GetBufferSize() };
		if (m_GeoShader.Get())		psoDesc.GS = { m_GeoShader->GetBufferPointer(), m_GeoShader->GetBufferSize() };
		if (m_PixelShader.Get())	psoDesc.PS = { m_PixelShader->GetBufferPointer(), m_PixelShader->GetBufferSize() };
		psoDesc.RasterizerState = rasterizerDesc;								// Specify how objects should be rasterized
		psoDesc.BlendState = blendDesc;											// Specify how pixels should be blended
		psoDesc.DepthStencilState = dsDesc;										// Specify how the depth/stencil tests should be performed
		psoDesc.SampleMask = UINT_MAX;											// The sample mask for the blend state
		psoDesc.PrimitiveTopologyType = tessellationEnabled ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;	// Specify the primitive typology of the data
		psoDesc.NumRenderTargets = m_Desc.Type == Skateboard::RasterizationPipelineType_DepthOnly ? 0 : 1; // The number of render targets
		psoDesc.RTVFormats[0] = m_Desc.Type == Skateboard::RasterizationPipelineType_DepthOnly ? DXGI_FORMAT_UNKNOWN : gD3DContext->GetBackBufferFormat(); // Specify the render target formats
		psoDesc.SampleDesc.Count = 1;											// TODO: Add support of MSAA?
		psoDesc.SampleDesc.Quality = 0;											// 
		psoDesc.DSVFormat = gD3DContext->GetDepthStencilFormat();				// Specify the format of the depth/stencil buffer

		// Create the pipeline state objects
		D3D_CHECK_FAILURE(gD3DContext->Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_PipelineStateObject.ReleaseAndGetAddressOf())));

#ifndef SKTBD_SHIP
		m_PipelineStateObject->SetName((m_DebugName + L" - Graphics PSO").c_str());
#endif // !SKTBD_SHIP
	}

	D3DComputePipeline::D3DComputePipeline(const std::wstring& debugName, const ComputePipelineDesc& desc) :
		ComputePipeline(debugName, desc)
	{
		// Load Shaders
		LoadAllShaders();
		// Create Root signature
		BuildRootSignature();
		// Build PSO
		BuildPSO();
	}

	void D3DComputePipeline::Bind()
	{
		ID3D12GraphicsCommandList* pCommandList = gD3DContext->GraphicsCommandList();
		pCommandList->SetPipelineState(m_PipelineStateObject.Get());
		pCommandList->SetComputeRootSignature(m_RootSignature.Get());
		uint32_t rootInputSlot = 0u;
		for (const ShaderResourceDesc& desc : m_Desc.vCBV)
		{
			D3DUploadBuffer* pBuffer = static_cast<D3DUploadBuffer*>(desc.pResource);
			pCommandList->SetComputeRootConstantBufferView(rootInputSlot, pBuffer->GetResource()->GetGPUVirtualAddress());
			++rootInputSlot;
		}
		for (const ShaderResourceDesc& desc : m_Desc.vDescriptorTables)
		{
			D3DDescriptorTable* table = static_cast<D3DDescriptorTable*>(desc.pDescriptorTable);
			pCommandList->SetGraphicsRootDescriptorTable(rootInputSlot, table->GetGPUHandle());
			++rootInputSlot;
		}
		if(m_DescriptorTableRootHandle.IsValid())
			pCommandList->SetComputeRootDescriptorTable(rootInputSlot, m_DescriptorTableRootHandle.GetGPUHandle());
	}

	void D3DComputePipeline::Release()
	{
	}

	void D3DComputePipeline::LoadAllShaders()
	{
		LoadShader(m_Desc.ComputeShader.FileName, (void**)m_ComputeShader.ReleaseAndGetAddressOf());
	}

	void D3DComputePipeline::BuildRootSignature()
	{
		std::vector<D3D12_ROOT_PARAMETER1> vRootParams;

		for (const ShaderResourceDesc& desc : m_Desc.vCBV)
		{
			D3D12_ROOT_PARAMETER1 param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.ShaderVisibility = ShaderVisibilityToD3D(desc.ShaderVisibility);
			param.Descriptor.ShaderRegister = desc.ShaderRegister;
			param.Descriptor.RegisterSpace = desc.ShaderRegisterSpace;
			vRootParams.emplace_back(std::move(param));
		}

		std::vector<D3D12_DESCRIPTOR_RANGE1> vSuppliedTableRanges;
		vSuppliedTableRanges.reserve(m_Desc.vDescriptorTables.size());
		for (const ShaderResourceDesc& desc : m_Desc.vDescriptorTables)
		{
			D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
			descriptorRange.RangeType = ShaderDescriptorTableTypeToD3D(desc.pDescriptorTable->GetTableType());
			descriptorRange.NumDescriptors = desc.pDescriptorTable->GetDescriptorCount();
			descriptorRange.BaseShaderRegister = desc.ShaderRegister;
			descriptorRange.RegisterSpace = desc.ShaderRegisterSpace;
			descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			vSuppliedTableRanges.emplace_back(std::move(descriptorRange));

			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = ShaderVisibilityToD3D(desc.ShaderVisibility);
			descriptorTable.DescriptorTable.NumDescriptorRanges = 1;
			descriptorTable.DescriptorTable.pDescriptorRanges = &vSuppliedTableRanges.back();
			vRootParams.emplace_back(std::move(descriptorTable));
		}

		// Construct a descriptor table for all other types of resources
		// This is done so that we can bind a lot more resources than what the Root Signature limits us to
		const uint32_t descriptorCount = static_cast<uint32_t>(m_Desc.vSRV.size() + m_Desc.vUAV.size());
		std::vector<D3D12_DESCRIPTOR_RANGE1> vDescriptorRanges;
		if (descriptorCount)
		{
			auto* pSrvHeap = gD3DContext->GetSrvHeap();
			D3DDescriptorHandle tempHandle = m_DescriptorTableRootHandle = pSrvHeap->Allocate(descriptorCount);
			const uint32_t handleSize = gD3DContext->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			for (const ShaderResourceDesc& desc : m_Desc.vSRV)
			{
				D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
				descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				descriptorRange.NumDescriptors = 1;
				descriptorRange.BaseShaderRegister = desc.ShaderRegister;
				descriptorRange.RegisterSpace = desc.ShaderRegisterSpace;
				descriptorRange.OffsetInDescriptorsFromTableStart = static_cast<uint32_t>(vDescriptorRanges.size());
				descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
				vDescriptorRanges.emplace_back(std::move(descriptorRange));

				switch (desc.pResource->GetType())
				{
				case GPUResourceType_DefaultBuffer:
				{
					D3DDefaultBuffer* pBuffer = static_cast<D3DDefaultBuffer*>(desc.pResource);
					gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					tempHandle += handleSize;
					break;
				}
				case GPUResourceType_UploadBuffer:
				{
					D3DUploadBuffer* pBuffer = static_cast<D3DUploadBuffer*>(desc.pResource);
					gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					tempHandle += handleSize;
					break;
				}
				case GPUResourceType_Texture2D:
				{
					D3DTexture* pBuffer = static_cast<D3DTexture*>(desc.pResource);
					gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					tempHandle += handleSize;
					break;
				}
				case GPUResourceType_UnorderedAccessBuffer:
				{
					D3DUnorderedAccessBuffer* pBuffer = static_cast<D3DUnorderedAccessBuffer*>(desc.pResource);
					gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					tempHandle += handleSize;
					break;
				}
				case GPUResourceType_FrameBuffer:
				{
					// TODO: GetSRVHandle(0) -> Do we need to index anything else really?
					D3DFrameBuffer* pBuffer = static_cast<D3DFrameBuffer*>(desc.pResource);
					gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle(0).GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
					tempHandle += handleSize;
					break;
				}
				default:
					SKTBD_CORE_ASSERT(false, "unsupported");
					break;
				}
			}

			// UAVs
			for (const ShaderResourceDesc& desc : m_Desc.vUAV)
			{
				D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
				descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				descriptorRange.NumDescriptors = 1;
				descriptorRange.BaseShaderRegister = desc.ShaderRegister;
				descriptorRange.RegisterSpace = desc.ShaderRegisterSpace;
				descriptorRange.OffsetInDescriptorsFromTableStart = static_cast<uint32_t>(vDescriptorRanges.size());
				descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
				vDescriptorRanges.emplace_back(std::move(descriptorRange));

				D3DUnorderedAccessBuffer* pBuffer = static_cast<D3DUnorderedAccessBuffer*>(desc.pResource);
				gD3DContext->Device()->CopyDescriptors(1, &tempHandle.GetCPUHandle(), &handleSize, 1, &pBuffer->GetUAVHandle().GetCPUHandle(), &handleSize, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				tempHandle += handleSize;
			}

			// Add a descriptor table with all ranges
			SKTBD_CORE_ASSERT(!vDescriptorRanges.empty(), "The logic went wrong, this vector should never be empty considering descriptorCount > 0");
			m_DescriptorTableRootHandle.SetValid(true);
			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// TODO: Maybe not all the time??
			descriptorTable.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(vDescriptorRanges.size());
			descriptorTable.DescriptorTable.pDescriptorRanges = vDescriptorRanges.data();
			vRootParams.emplace_back(std::move(descriptorTable));
		}
		else m_DescriptorTableRootHandle.SetValid(false);

		// Add the supplied samplers
		std::vector<D3D12_STATIC_SAMPLER_DESC> vStaticSamplers;
		vStaticSamplers.reserve(m_Desc.vStaticSamplers.size());
		for (const SamplerDesc& samplerDesc : m_Desc.vStaticSamplers)
		{
			D3D12_STATIC_SAMPLER_DESC d3dSamplerDesc = {};
			d3dSamplerDesc.ShaderRegister = samplerDesc.ShaderRegister;
			d3dSamplerDesc.RegisterSpace = samplerDesc.ShaderRegisterSpace;
			d3dSamplerDesc.Filter = SamplerFilterToD3D(samplerDesc.Filter);
			d3dSamplerDesc.AddressU = SamplerModeToD3D(samplerDesc.Mode);
			d3dSamplerDesc.AddressV = SamplerModeToD3D(samplerDesc.Mode);
			d3dSamplerDesc.AddressW = SamplerModeToD3D(samplerDesc.Mode);
			d3dSamplerDesc.MipLODBias = samplerDesc.MipMapLevelOffset;
			d3dSamplerDesc.MaxAnisotropy = samplerDesc.MaxAnisotropy;
			d3dSamplerDesc.ComparisonFunc = SamplerComparisonFunctionToD3D(samplerDesc.ComparisonFunction);
			d3dSamplerDesc.BorderColor = SamplerBorderColourToD3D(samplerDesc.BorderColour);
			d3dSamplerDesc.MinLOD = samplerDesc.MipMapMinSampleLevel;
			d3dSamplerDesc.MaxLOD = samplerDesc.MipMapMaxSampleLevel;
			d3dSamplerDesc.ShaderVisibility = ShaderVisibilityToD3D(samplerDesc.ShaderVisibility);
			vStaticSamplers.emplace_back(std::move(d3dSamplerDesc));
		}

		// Describe the root signature
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.NumParameters = static_cast<UINT>(vRootParams.size());
		rootSignatureDesc.Desc_1_1.pParameters = vRootParams.data();
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(vStaticSamplers.size());
		rootSignatureDesc.Desc_1_1.pStaticSamplers = vStaticSamplers.data();
		rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		// Serialize a root signature into a blob that can be passed into the CreateRootSignature function
		// The version of this root signature will be 1.1
		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		Microsoft::WRL::ComPtr<ID3DBlob> error;
		if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), error.GetAddressOf())))
		{
			SKTBD_CORE_CRITICAL((char*)error->GetBufferPointer());
			throw std::exception();
		}

		D3D_CHECK_FAILURE(gD3DContext->Device()->CreateRootSignature(
			0,															// For single GPU, set this to 0
			signature->GetBufferPointer(),								// Pointer to the serialized signature
			signature->GetBufferSize(),									// Length, in bytes, of the signature to be read
			IID_PPV_ARGS(m_RootSignature.ReleaseAndGetAddressOf()))
		);

#ifndef SKTBD_SHIP
		m_RootSignature->SetName((m_DebugName + L" - RootSignature").c_str());
#endif // !SKTBD_SHIP
	}

	void D3DComputePipeline::BuildPSO()
	{
		// Describe and create the compute pipeline state object (PSO).
		D3D12_COMPUTE_PIPELINE_STATE_DESC computePsoDesc = {};
		computePsoDesc.pRootSignature = m_RootSignature.Get();
		computePsoDesc.CS = { m_ComputeShader->GetBufferPointer(), m_ComputeShader->GetBufferSize() };
		computePsoDesc.NodeMask = 0u;
		computePsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		D3D_CHECK_FAILURE(gD3DContext->Device()->CreateComputePipelineState(&computePsoDesc, IID_PPV_ARGS(m_PipelineStateObject.ReleaseAndGetAddressOf())));

#ifndef SKTBD_SHIP
		m_PipelineStateObject->SetName((m_DebugName + L" - Compute PSO").c_str());
#endif // !SKTBD_SHIP
	}

	D3DRaytracingPipeline::D3DRaytracingPipeline(const std::wstring& debugName, const RaytracingPipelineDesc& desc) :
		RaytracingPipeline(debugName, desc)
	{
		// Create the output UAV
		m_RaytracingOutput.reset(Skateboard::UnorderedAccessBuffer::Create(
			L"UAV",
			Skateboard::UnorderedAccessBufferDesc::Init(m_Desc.DispatchSize.ThreadCountX, m_Desc.DispatchSize.ThreadCountY, m_Desc.DispatchSize.ThreadCountZ)
		));

		// Load the shader library with the raytracing shaders
		LoadShader(m_Desc.RaytracingShaders.FileName, (void**)m_ShaderDXIL.ReleaseAndGetAddressOf());

		// Build the root signature
		BuildRootSignature();

		// Build Raytracing State Object
		BuildSO();

		// Create the shader table
		BuildShaderTable();
	}

	void D3DRaytracingPipeline::Bind()
	{
		ID3D12GraphicsCommandList4* pCommandList = gD3DContext->RaytracingCommandList();

		pCommandList->SetComputeRootSignature(m_GlobalRootSignature.Get());
		uint32_t rootInputSlot = 0u;
		for (D3DUploadBuffer*& buffer : v_GlobalConstantBuffers)
		{
			pCommandList->SetComputeRootConstantBufferView(rootInputSlot, buffer->GetGPUVirtualAddress());
			++rootInputSlot;
		}
		for (D3DDescriptorTable*& table : v_GlobalDescriptorTables)
		{
			pCommandList->SetGraphicsRootDescriptorTable(rootInputSlot, table->GetGPUHandle());
			++rootInputSlot;
		}
		if (m_GlobalDescriptorTableHandle.IsValid())
			pCommandList->SetComputeRootDescriptorTable(rootInputSlot, m_GlobalDescriptorTableHandle.GetGPUHandle());

		// Update the shader table entries with the current local arugments
		uint32_t recordIndex = 0u;
		uint32_t argumentIndex = 0u;
		{ // Ray gen record (there will always be only one!)
			// Constant buffers are directly bound to the root signature
			for (D3DUploadBuffer*& buffer : v_LocalRayGenConstantBuffers)
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, buffer->GetGPUVirtualAddress());

			// Input (Skateboard) Descriptor tables
			for (D3DDescriptorTable*& table : v_LocalRayGenDescriptorTables)
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, table->GetGPUHandle().ptr);

			// SRVs and UAVs are part of the Descriptor table
			if (m_LocalRaygenDescriptorTableHandle.IsValid())
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, m_LocalRaygenDescriptorTableHandle.GetGPUHandle().ptr);

			// We are done with this record
			++recordIndex;
			argumentIndex = 0u;
		}
		for (size_t i = 0u; i < m_Desc.RaytracingShaders.MissShaderEntryPoints.size(); ++i)
		{
			// Constant buffers are directly bound to the root signature
			for (D3DUploadBuffer*& buffer : v_LocalMissConstantBuffers)
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, buffer->GetGPUVirtualAddress());

			// Input (Skateboard) Descriptor tables
			for (D3DDescriptorTable*& table : v_LocalMissDescriptorTables)
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, table->GetGPUHandle().ptr);

			// SRVs and UAVs are part of the Descriptor table
			if (m_LocalMissDescriptorTableHandle.IsValid())
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, m_LocalMissDescriptorTableHandle.GetGPUHandle().ptr);
			
			// We are done with this record
			++recordIndex;
			argumentIndex = 0u;
		}
		for (size_t i = 0u; i < m_Desc.RaytracingShaders.HitGroups.size(); ++i)
		{
			// Constant buffers are directly bound to the root signature
			for (D3DUploadBuffer*& buffer : v_LocalHitGroupConstantBuffers)
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, buffer->GetGPUVirtualAddress());

			// Input (Skateboard) Descriptor tables
			for (D3DDescriptorTable*& table : v_LocalHitGroupDescriptorTables)
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, table->GetGPUHandle().ptr);

			// SRVs and UAVs are part of the Descriptor table
			if (m_LocalHitGroupDescriptorTableHandle.IsValid())
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, m_LocalHitGroupDescriptorTableHandle.GetGPUHandle().ptr);

			// We are done with this record
			++recordIndex;
			argumentIndex = 0u;
		}
		for (size_t i = 0u; i < m_Desc.RaytracingShaders.CallableShaders.size(); ++i)
		{
			// Constant buffers are directly bound to the root signature
			for (D3DUploadBuffer*& buffer : v_LocalCallableConstantBuffers)
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, buffer->GetGPUVirtualAddress());

			// Input (Skateboard) Descriptor tables
			for (D3DDescriptorTable*& table : v_LocalCallableDescriptorTables)
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, table->GetGPUHandle().ptr);

			// SRVs and UAVs are part of the Descriptor table
			if (m_LocalCallableDescriptorTableHandle.IsValid())
				m_ShaderTable.UpdateShaderRecord(recordIndex, argumentIndex++, m_LocalCallableDescriptorTableHandle.GetGPUHandle().ptr);

			// We are done with this record
			++recordIndex;
			argumentIndex = 0u;
		}

		// Declare Ray
		D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
		dispatchDesc.RayGenerationShaderRecord.StartAddress = m_ShaderTable.GetRaygenRecordStartAddress();                              // Ray Gen shader table
		dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_ShaderTable.GetRaygenShaderTableSize();
		dispatchDesc.MissShaderTable.StartAddress = m_ShaderTable.GetMissRecordStartAddress();                                          // Miss shader table
		dispatchDesc.MissShaderTable.SizeInBytes = m_ShaderTable.GetMissShaderTableSize();
		dispatchDesc.MissShaderTable.StrideInBytes = m_ShaderTable.GetMissShaderTableStride();
		dispatchDesc.HitGroupTable.StartAddress = m_ShaderTable.GetHitgroupRecordStartAddress();                                        // Hit Group shader table
		dispatchDesc.HitGroupTable.SizeInBytes = m_ShaderTable.GetHitGroupShaderTableSize();
		dispatchDesc.HitGroupTable.StrideInBytes = m_ShaderTable.GetHitGroupShaderTableStride();
		dispatchDesc.CallableShaderTable.StartAddress = m_ShaderTable.GetCallableRecordStartAddress();
		dispatchDesc.CallableShaderTable.SizeInBytes = m_ShaderTable.GetCallableShaderTableSize();
		dispatchDesc.CallableShaderTable.StrideInBytes = m_ShaderTable.GetCallableShaderTableStride();
		dispatchDesc.Width = m_Desc.DispatchSize.ThreadCountX;
		dispatchDesc.Height = m_Desc.DispatchSize.ThreadCountY;
		dispatchDesc.Depth = m_Desc.DispatchSize.ThreadCountZ;

		// Set the pipeline state and dispatch
		pCommandList->SetPipelineState1(m_StateObject.Get());
		pCommandList->DispatchRays(&dispatchDesc);
	}

	void D3DRaytracingPipeline::ResizeDispatchAndOutputUAV(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth)
	{
		// Sanity check
		SKTBD_CORE_ASSERT(m_StateObject.Get() != nullptr, "Cannot call resize on non-existant raytracing pipeline!");

		// Resize UAV output and resupply the new buffer
		m_RaytracingOutput->Resize(newWidth, newHeight, newDepth);
		D3DUnorderedAccessBuffer* pBuffer = static_cast<D3DUnorderedAccessBuffer*>(m_RaytracingOutput.get());
		gD3DContext->Device()->CopyDescriptorsSimple(1, m_GlobalDescriptorTableHandle.GetCPUHandle(), pBuffer->GetUAVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// Resize the dispatch
		m_Desc.DispatchSize = { newWidth, newHeight, newDepth };
	}

	void D3DRaytracingPipeline::Release()
	{
	}

	void D3DRaytracingPipeline::BuildRootSignature()
	{
		ID3D12Device5* pDevice = gD3DContext->RaytracingDevice();

		auto* pSrvHeap = const_cast<D3DDescriptorHeap*>(gD3DContext->GetSrvHeap());

		// TODO: Refactor this based on Rhys' work with tables (as well as in ResizeDispatchAndOutputUAV())
		// Note: This really needs refactoring it feels utterly wrong
		D3DDescriptorHandle tempHandle = m_GlobalDescriptorTableHandle = pSrvHeap->Allocate(static_cast<uint32_t>(1u + m_Desc.vSRV.size() + m_Desc.vUAV.size()));
		const uint32_t handleSize = gD3DContext->Device()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> vGlobalDescriptorHandles;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> vLocalRaygenDescriptorHandles;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> vLocalHitGroupDescriptorHandles;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> vLocalMissDescriptorHandles;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> vLocalCallableDescriptorHandles;

		// The global root signature will be shared accross all raytracing shaders following a DispatchRays() call
		// The local root signature holds unique arguments from a shader table to a particular shader
		std::vector<D3D12_ROOT_PARAMETER1> vGlobalRootParams;
		std::vector<D3D12_ROOT_PARAMETER1> vLocalRaygenRootParams;
		std::vector<D3D12_ROOT_PARAMETER1> vLocalHitGroupRootParams;
		std::vector<D3D12_ROOT_PARAMETER1> vLocalMissRootParams;
		std::vector<D3D12_ROOT_PARAMETER1> vLocalCallableRootParams;
		std::vector<D3D12_DESCRIPTOR_RANGE1> vGlobalDescriptorRanges;
		std::vector<D3D12_DESCRIPTOR_RANGE1> vLocalRaygenDescriptorRanges;
		std::vector<D3D12_DESCRIPTOR_RANGE1> vLocalHitGroupDescriptorRanges;
		std::vector<D3D12_DESCRIPTOR_RANGE1> vLocalMissDescriptorRanges;
		std::vector<D3D12_DESCRIPTOR_RANGE1> vLocalCallableDescriptorRanges;
		std::vector<D3D12_STATIC_SAMPLER_DESC> vGlobalStaticSamplers;
		std::vector<D3D12_STATIC_SAMPLER_DESC> vLocalRaygenStaticSamplers;
		std::vector<D3D12_STATIC_SAMPLER_DESC> vLocalHitGroupStaticSamplers;
		std::vector<D3D12_STATIC_SAMPLER_DESC> vLocalMissStaticSamplers;
		std::vector<D3D12_STATIC_SAMPLER_DESC> vLocalCallableStaticSamplers;
		
		auto AddRootParameter = [&](D3D12_ROOT_PARAMETER1& param, ShaderVisibility_ visibility)
		{
			switch (visibility)
			{
			case RaytracingShaderVisibility_Global:			vGlobalRootParams.emplace_back(std::move(param));			break;
			case RaytracingShaderVisibility_Local_RayGen:	vLocalRaygenRootParams.emplace_back(std::move(param));		break;
			case RaytracingShaderVisibility_Local_Hitgroup: vLocalHitGroupRootParams.emplace_back(std::move(param));	break;
			case RaytracingShaderVisibility_Local_Miss:		vLocalMissRootParams.emplace_back(std::move(param));		break;
			case RaytracingShaderVisibility_Local_Callable:	vLocalCallableRootParams.emplace_back(std::move(param));	break;
			default: SKTBD_CORE_ASSERT(false, "Incorrect shader visibility supplied. Ensure that you input a RaytracingShaderVisibility_"); break;
			}
		};
		auto SortConstantBuffer = [&](const ShaderResourceDesc& desc)
		{
			switch (desc.ShaderVisibility)
			{
			case RaytracingShaderVisibility_Global:			v_GlobalConstantBuffers.push_back(static_cast<D3DUploadBuffer*>(desc.pResource));				break;
			case RaytracingShaderVisibility_Local_RayGen:	v_LocalRayGenConstantBuffers.push_back(static_cast<D3DUploadBuffer*>(desc.pResource));			break;
			case RaytracingShaderVisibility_Local_Hitgroup: v_LocalHitGroupConstantBuffers.push_back(static_cast<D3DUploadBuffer*>(desc.pResource));		break;
			case RaytracingShaderVisibility_Local_Miss:		v_LocalMissConstantBuffers.push_back(static_cast<D3DUploadBuffer*>(desc.pResource));			break;
			case RaytracingShaderVisibility_Local_Callable:	v_LocalCallableConstantBuffers.push_back(static_cast<D3DUploadBuffer*>(desc.pResource));		break;
			default: SKTBD_CORE_ASSERT(false, "Incorrect shader visibility supplied. Ensure that you input a RaytracingShaderVisibility_"); break;
			}
		};
		auto SortDescriptorTable = [&](const ShaderResourceDesc& desc)
		{
			switch (desc.ShaderVisibility)
			{
			case RaytracingShaderVisibility_Global:			v_GlobalDescriptorTables.push_back(static_cast<D3DDescriptorTable*>(desc.pDescriptorTable));			break;
			case RaytracingShaderVisibility_Local_RayGen:	v_LocalRayGenDescriptorTables.push_back(static_cast<D3DDescriptorTable*>(desc.pDescriptorTable));		break;
			case RaytracingShaderVisibility_Local_Hitgroup: v_LocalHitGroupDescriptorTables.push_back(static_cast<D3DDescriptorTable*>(desc.pDescriptorTable));		break;
			case RaytracingShaderVisibility_Local_Miss:		v_LocalMissDescriptorTables.push_back(static_cast<D3DDescriptorTable*>(desc.pDescriptorTable));			break;
			case RaytracingShaderVisibility_Local_Callable:	v_LocalCallableDescriptorTables.push_back(static_cast<D3DDescriptorTable*>(desc.pDescriptorTable));		break;
			default: SKTBD_CORE_ASSERT(false, "Incorrect shader visibility supplied. Ensure that you input a RaytracingShaderVisibility_"); break;
			}
		};
		auto AddDescriptorRange = [&](D3D12_DESCRIPTOR_RANGE1& range, ShaderVisibility_ visibility, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
		{
			switch (visibility)
			{
			case RaytracingShaderVisibility_Global:
				range.OffsetInDescriptorsFromTableStart = static_cast<uint32_t>(vGlobalDescriptorRanges.size());
				vGlobalDescriptorRanges.emplace_back(std::move(range));
				vGlobalDescriptorHandles.push_back(cpuHandle);
				break;
			case RaytracingShaderVisibility_Local_RayGen:
				range.OffsetInDescriptorsFromTableStart = static_cast<uint32_t>(vLocalRaygenDescriptorRanges.size());
				vLocalRaygenDescriptorRanges.emplace_back(std::move(range));
				vLocalRaygenDescriptorHandles.push_back(cpuHandle);
				break;
			case RaytracingShaderVisibility_Local_Hitgroup:
				range.OffsetInDescriptorsFromTableStart = static_cast<uint32_t>(vLocalHitGroupDescriptorRanges.size());
				vLocalHitGroupDescriptorRanges.emplace_back(std::move(range));
				vLocalHitGroupDescriptorHandles.push_back(cpuHandle);
				break;
			case RaytracingShaderVisibility_Local_Miss:
				range.OffsetInDescriptorsFromTableStart = static_cast<uint32_t>(vLocalMissDescriptorRanges.size());
				vLocalMissDescriptorRanges.emplace_back(std::move(range));
				vLocalMissDescriptorHandles.push_back(cpuHandle);
				break;
			case RaytracingShaderVisibility_Local_Callable:
				range.OffsetInDescriptorsFromTableStart = static_cast<uint32_t>(vLocalCallableDescriptorRanges.size());
				vLocalCallableDescriptorRanges.emplace_back(std::move(range));
				vLocalCallableDescriptorHandles.push_back(cpuHandle);
				break;
			default:
				SKTBD_CORE_ASSERT(false, "Incorrect shader visibility supplied. Ensure that you input a RaytracingShaderVisibility_");
				break;
			}
		};
		auto AddStaticSampler = [&](D3D12_STATIC_SAMPLER_DESC& param, ShaderVisibility_ visibility)
		{
			switch (visibility)
			{
			case RaytracingShaderVisibility_Global:			vGlobalStaticSamplers.emplace_back(std::move(param));			break;
			case RaytracingShaderVisibility_Local_RayGen:	vLocalRaygenStaticSamplers.emplace_back(std::move(param));		break;
			case RaytracingShaderVisibility_Local_Hitgroup: vLocalHitGroupStaticSamplers.emplace_back(std::move(param));	break;
			case RaytracingShaderVisibility_Local_Miss:		vLocalMissStaticSamplers.emplace_back(std::move(param));		break;
			case RaytracingShaderVisibility_Local_Callable:	vLocalCallableStaticSamplers.emplace_back(std::move(param));	break;
			default: SKTBD_CORE_ASSERT(false, "Incorrect shader visibility supplied. Ensure that you input a RaytracingShaderVisibility_"); break;
			}
		};

		// Output UAV
		{
			D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
			descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRange.NumDescriptors = 1;
			descriptorRange.BaseShaderRegister = 0;
			descriptorRange.RegisterSpace = 1u;
			descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			AddDescriptorRange(descriptorRange, RaytracingShaderVisibility_Global, static_cast<D3DUnorderedAccessBuffer*>(m_RaytracingOutput.get())->GetSRVHandle().GetCPUHandle());
		}

		for (const ShaderResourceDesc& desc : m_Desc.vCBV)
		{
			D3D12_ROOT_PARAMETER1 param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			param.Descriptor.ShaderRegister = desc.ShaderRegister;
			param.Descriptor.RegisterSpace = desc.ShaderRegisterSpace;
			AddRootParameter(param, desc.ShaderVisibility);
			SortConstantBuffer(desc);
		}

		std::vector<D3D12_DESCRIPTOR_RANGE1> vSuppliedTableRanges;
		vSuppliedTableRanges.reserve(m_Desc.vDescriptorTables.size());
		for (const ShaderResourceDesc& desc : m_Desc.vDescriptorTables)
		{
			D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
			descriptorRange.RangeType = ShaderDescriptorTableTypeToD3D(desc.pDescriptorTable->GetTableType());
			descriptorRange.NumDescriptors = desc.pDescriptorTable->GetDescriptorCount();
			descriptorRange.BaseShaderRegister = desc.ShaderRegister;
			descriptorRange.RegisterSpace = desc.ShaderRegisterSpace;
			descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			vSuppliedTableRanges.emplace_back(std::move(descriptorRange));

			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			descriptorTable.DescriptorTable.NumDescriptorRanges = 1;
			descriptorTable.DescriptorTable.pDescriptorRanges = &vSuppliedTableRanges.back();
			AddRootParameter(descriptorTable, desc.ShaderVisibility);
			SortDescriptorTable(desc);
		}

		for (const ShaderResourceDesc& desc : m_Desc.vSRV)
		{
			D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
			descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			descriptorRange.NumDescriptors = 1;
			descriptorRange.BaseShaderRegister = desc.ShaderRegister;
			descriptorRange.RegisterSpace = desc.ShaderRegisterSpace;
			descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
			switch (desc.pResource->GetType())
			{
			case GPUResourceType_DefaultBuffer:					cpuHandle = static_cast<D3DDefaultBuffer*>(desc.pResource)->GetSRVHandle().GetCPUHandle();					break;
			case GPUResourceType_UploadBuffer:					cpuHandle = static_cast<D3DUploadBuffer*>(desc.pResource)->GetSRVHandle().GetCPUHandle();					break;
			case GPUResourceType_Texture2D:						cpuHandle = static_cast<D3DTexture*>(desc.pResource)->GetSRVHandle().GetCPUHandle();						break;
			case GPUResourceType_UnorderedAccessBuffer:			cpuHandle = static_cast<D3DUnorderedAccessBuffer*>(desc.pResource)->GetSRVHandle().GetCPUHandle();			break;
			case GPUResourceType_FrameBuffer:					cpuHandle = static_cast<D3DFrameBuffer*>(desc.pResource)->GetSRVHandle(0).GetCPUHandle();					break;
			case GPUResourceType_TopLevelAccelerationStructure:	cpuHandle = static_cast<D3DTopLevelAccelerationStructure*>(desc.pResource)->GetSRVHandle().GetCPUHandle();	break;
			default:
				SKTBD_CORE_ASSERT(false, "unsupported");
				break;
			}
			AddDescriptorRange(descriptorRange, desc.ShaderVisibility, cpuHandle);
		}
		
		// UAVS
		for (const ShaderResourceDesc& desc : m_Desc.vUAV)
		{
			D3D12_DESCRIPTOR_RANGE1 descriptorRange = {};
			descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			descriptorRange.NumDescriptors = 1;
			descriptorRange.BaseShaderRegister = desc.ShaderRegister;
			descriptorRange.RegisterSpace = desc.ShaderRegisterSpace;
			descriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			AddDescriptorRange(descriptorRange, desc.ShaderVisibility, static_cast<D3DUnorderedAccessBuffer*>(desc.pResource)->GetUAVHandle().GetCPUHandle());
		}

		// Assign descriptors
		for (const D3D12_CPU_DESCRIPTOR_HANDLE& handle : vGlobalDescriptorHandles)
		{
			gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			tempHandle += handleSize;
		}
		m_LocalRaygenDescriptorTableHandle = tempHandle;
		m_LocalRaygenDescriptorTableHandle.SetValid(!vLocalRaygenDescriptorHandles.empty());
		for (const D3D12_CPU_DESCRIPTOR_HANDLE& handle : vLocalRaygenDescriptorHandles)
		{
			gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			tempHandle += handleSize;
		}
		m_LocalHitGroupDescriptorTableHandle = tempHandle;
		m_LocalHitGroupDescriptorTableHandle.SetValid(!vLocalHitGroupDescriptorHandles.empty());
		for (const D3D12_CPU_DESCRIPTOR_HANDLE& handle : vLocalHitGroupDescriptorHandles)
		{
			gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			tempHandle += handleSize;
		}
		m_LocalMissDescriptorTableHandle = tempHandle;
		m_LocalMissDescriptorTableHandle.SetValid(!vLocalMissDescriptorHandles.empty());
		for (const D3D12_CPU_DESCRIPTOR_HANDLE& handle : vLocalMissDescriptorHandles)
		{
			gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			tempHandle += handleSize;
		}
		m_LocalCallableDescriptorTableHandle = tempHandle;
		m_LocalCallableDescriptorTableHandle.SetValid(!vLocalCallableDescriptorHandles.empty());
		for (const D3D12_CPU_DESCRIPTOR_HANDLE& handle : vLocalCallableDescriptorHandles)
		{
			gD3DContext->Device()->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), handle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			tempHandle += handleSize;
		}

		for (const SamplerDesc& samplerDesc : m_Desc.vStaticSamplers)
		{
			D3D12_STATIC_SAMPLER_DESC d3dSamplerDesc = {};
			d3dSamplerDesc.ShaderRegister = samplerDesc.ShaderRegister;
			d3dSamplerDesc.RegisterSpace = samplerDesc.ShaderRegisterSpace;
			d3dSamplerDesc.Filter = SamplerFilterToD3D(samplerDesc.Filter);
			d3dSamplerDesc.AddressU = SamplerModeToD3D(samplerDesc.Mode);
			d3dSamplerDesc.AddressV = SamplerModeToD3D(samplerDesc.Mode);
			d3dSamplerDesc.AddressW = SamplerModeToD3D(samplerDesc.Mode);
			d3dSamplerDesc.MipLODBias = samplerDesc.MipMapLevelOffset;
			d3dSamplerDesc.MaxAnisotropy = samplerDesc.MaxAnisotropy;
			d3dSamplerDesc.ComparisonFunc = SamplerComparisonFunctionToD3D(samplerDesc.ComparisonFunction);
			d3dSamplerDesc.BorderColor = SamplerBorderColourToD3D(samplerDesc.BorderColour);
			d3dSamplerDesc.MinLOD = samplerDesc.MipMapMinSampleLevel;
			d3dSamplerDesc.MaxLOD = samplerDesc.MipMapMaxSampleLevel;
			d3dSamplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			AddStaticSampler(d3dSamplerDesc, samplerDesc.ShaderVisibility);
		}

		// Add a descriptor table with all ranges
		if (!vGlobalDescriptorRanges.empty())
		{
			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			descriptorTable.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(vGlobalDescriptorRanges.size());
			descriptorTable.DescriptorTable.pDescriptorRanges = vGlobalDescriptorRanges.data();
			vGlobalRootParams.emplace_back(std::move(descriptorTable));
		}
		if (!vLocalRaygenDescriptorRanges.empty())
		{
			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			descriptorTable.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(vLocalRaygenDescriptorRanges.size());
			descriptorTable.DescriptorTable.pDescriptorRanges = vLocalRaygenDescriptorRanges.data();
			vLocalRaygenRootParams.emplace_back(std::move(descriptorTable));
		}
		if (!vLocalHitGroupDescriptorRanges.empty())
		{
			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			descriptorTable.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(vLocalHitGroupDescriptorRanges.size());
			descriptorTable.DescriptorTable.pDescriptorRanges = vLocalHitGroupDescriptorRanges.data();
			vLocalHitGroupRootParams.emplace_back(std::move(descriptorTable));
		}
		if (!vLocalMissDescriptorRanges.empty())
		{
			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			descriptorTable.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(vLocalMissDescriptorRanges.size());
			descriptorTable.DescriptorTable.pDescriptorRanges = vLocalMissDescriptorRanges.data();
			vLocalMissRootParams.emplace_back(std::move(descriptorTable));
		}
		if (!vLocalCallableDescriptorRanges.empty())
		{
			D3D12_ROOT_PARAMETER1 descriptorTable = {};
			descriptorTable.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			descriptorTable.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			descriptorTable.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(vLocalCallableDescriptorRanges.size());
			descriptorTable.DescriptorTable.pDescriptorRanges = vLocalCallableDescriptorRanges.data();
			vLocalCallableRootParams.emplace_back(std::move(descriptorTable));
		}

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.NumParameters = static_cast<UINT>(vGlobalRootParams.size());
		rootSignatureDesc.Desc_1_1.pParameters = vGlobalRootParams.data();
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(vGlobalStaticSamplers.size());
		rootSignatureDesc.Desc_1_1.pStaticSamplers = vGlobalStaticSamplers.data();
		rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		Microsoft::WRL::ComPtr<ID3DBlob> signature;
		Microsoft::WRL::ComPtr<ID3DBlob> error;
		if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), error.GetAddressOf())))
		{
			SKTBD_CORE_CRITICAL((char*)error->GetBufferPointer());
			throw std::exception();
		}
		D3D_CHECK_FAILURE(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_GlobalRootSignature.ReleaseAndGetAddressOf())));

		if (!vLocalRaygenRootParams.empty())
		{
			rootSignatureDesc.Desc_1_1.NumParameters = static_cast<UINT>(vLocalRaygenRootParams.size());
			rootSignatureDesc.Desc_1_1.pParameters = vLocalRaygenRootParams.data();
			rootSignatureDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(vLocalRaygenStaticSamplers.size());
			rootSignatureDesc.Desc_1_1.pStaticSamplers = vLocalRaygenStaticSamplers.data();
			rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
			Microsoft::WRL::ComPtr<ID3DBlob> signature;
			Microsoft::WRL::ComPtr<ID3DBlob> error;
			if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), error.GetAddressOf())))
			{
				SKTBD_CORE_CRITICAL((char*)error->GetBufferPointer());
				throw std::exception();
			}
			D3D_CHECK_FAILURE(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_LocalRaygenRootSignature.ReleaseAndGetAddressOf())));
		}

		if (!vLocalHitGroupRootParams.empty())
		{
			rootSignatureDesc.Desc_1_1.NumParameters = static_cast<UINT>(vLocalHitGroupRootParams.size());
			rootSignatureDesc.Desc_1_1.pParameters = vLocalHitGroupRootParams.data();
			rootSignatureDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(vLocalHitGroupStaticSamplers.size());
			rootSignatureDesc.Desc_1_1.pStaticSamplers = vLocalHitGroupStaticSamplers.data();
			rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
			Microsoft::WRL::ComPtr<ID3DBlob> signature;
			Microsoft::WRL::ComPtr<ID3DBlob> error;
			if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), error.GetAddressOf())))
			{
				SKTBD_CORE_CRITICAL((char*)error->GetBufferPointer());
				throw std::exception();
			}
			D3D_CHECK_FAILURE(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_LocalHitGroupRootSignature.ReleaseAndGetAddressOf())));
		}

		if (!vLocalMissRootParams.empty())
		{
			rootSignatureDesc.Desc_1_1.NumParameters = static_cast<UINT>(vLocalMissRootParams.size());
			rootSignatureDesc.Desc_1_1.pParameters = vLocalMissRootParams.data();
			rootSignatureDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(vLocalMissStaticSamplers.size());
			rootSignatureDesc.Desc_1_1.pStaticSamplers = vLocalMissStaticSamplers.data();
			rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
			Microsoft::WRL::ComPtr<ID3DBlob> signature;
			Microsoft::WRL::ComPtr<ID3DBlob> error;
			if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), error.GetAddressOf())))
			{
				SKTBD_CORE_CRITICAL((char*)error->GetBufferPointer());
				throw std::exception();
			}
			D3D_CHECK_FAILURE(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_LocalMissRootSignature.ReleaseAndGetAddressOf())));
		}

		if (!vLocalCallableRootParams.empty())
		{
			rootSignatureDesc.Desc_1_1.NumParameters = static_cast<UINT>(vLocalCallableRootParams.size());
			rootSignatureDesc.Desc_1_1.pParameters = vLocalCallableRootParams.data();
			rootSignatureDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(vLocalCallableStaticSamplers.size());
			rootSignatureDesc.Desc_1_1.pStaticSamplers = vLocalCallableStaticSamplers.data();
			rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
			Microsoft::WRL::ComPtr<ID3DBlob> signature;
			Microsoft::WRL::ComPtr<ID3DBlob> error;
			if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), error.GetAddressOf())))
			{
				SKTBD_CORE_CRITICAL((char*)error->GetBufferPointer());
				throw std::exception();
			}
			D3D_CHECK_FAILURE(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_LocalCallableRootSignature.ReleaseAndGetAddressOf())));
		}
	}

	void D3DRaytracingPipeline::BuildSO()
	{
		ID3D12Device5* pDevice = gD3DContext->RaytracingDevice();

		std::vector<D3D12_STATE_SUBOBJECT> subObjects;
		constexpr const uint32_t maxSubobjects = 11u;
		subObjects.reserve(maxSubobjects);

		// Perform a few sanity checks here; there needs to be at least one raygen shader, and all
		SKTBD_CORE_ASSERT(m_Desc.RaytracingShaders.RayGenShaderEntryPoint != nullptr, "A Ray Generation Shader is mandatory for the execution of the pipeline. Ensure you identify your RayGen shader.");

		//// DXIL SUBOBJECT
		std::vector<D3D12_EXPORT_DESC> shaderExports;	// Each export defines the name of the individual shaders
		shaderExports.reserve(1u + m_Desc.RaytracingShaders.HitGroups.size() * 3u + m_Desc.RaytracingShaders.MissShaderEntryPoints.size());
		D3D12_EXPORT_DESC exportDesc = {};
		exportDesc.ExportToRename = nullptr; // TODO: If multiple shaders use identical function names, move it here instead and use the .Name field to give a unique identifier
		exportDesc.Flags = D3D12_EXPORT_FLAG_NONE;

		// Start by pushing the Ray Generation shader export
		exportDesc.Name = m_Desc.RaytracingShaders.RayGenShaderEntryPoint;
		shaderExports.push_back(exportDesc);

		// Then export all hit groups
		std::unordered_set<const wchar_t*> existingExports;
		for (const RaytracingHitGroup& group : m_Desc.RaytracingShaders.HitGroups)
		{
			if (group.IntersectionShaderEntryPoint != nullptr && !existingExports.contains(group.IntersectionShaderEntryPoint))
			{
				exportDesc.Name = group.IntersectionShaderEntryPoint;
				existingExports.insert(group.IntersectionShaderEntryPoint);
				shaderExports.push_back(exportDesc);
			}
			if (group.AnyHitShaderEntryPoint != nullptr && !existingExports.contains(group.AnyHitShaderEntryPoint))
			{
				exportDesc.Name = group.AnyHitShaderEntryPoint;
				existingExports.insert(group.AnyHitShaderEntryPoint);
				shaderExports.push_back(exportDesc);
			}
			if (group.ClosestHitShaderEntryPoint != nullptr && !existingExports.contains(group.ClosestHitShaderEntryPoint))
			{
				exportDesc.Name = group.ClosestHitShaderEntryPoint;
				existingExports.insert(group.ClosestHitShaderEntryPoint);
				shaderExports.push_back(exportDesc);
			}
		}

		// Export all miss shaders
		for (const wchar_t*& missEntryPoint : m_Desc.RaytracingShaders.MissShaderEntryPoints)
		{
			if (existingExports.contains(missEntryPoint))
				continue;
			exportDesc.Name = missEntryPoint;
			existingExports.insert(missEntryPoint);
			shaderExports.push_back(exportDesc);
		}

		// Export all callable shaders
		for (const wchar_t*& callableShader : m_Desc.RaytracingShaders.CallableShaders)
		{
			if (existingExports.contains(callableShader))
				continue;
			exportDesc.Name = callableShader;
			existingExports.insert(callableShader);
			shaderExports.push_back(exportDesc);
		}

		D3D12_DXIL_LIBRARY_DESC libDesc = {};
		libDesc.pExports = shaderExports.data();
		libDesc.NumExports = static_cast<uint32_t>(shaderExports.size());
		libDesc.DXILLibrary = {
			m_ShaderDXIL->GetBufferPointer(),
			m_ShaderDXIL->GetBufferSize()
		};

		D3D12_STATE_SUBOBJECT dxilSubObject = {};
		dxilSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		dxilSubObject.pDesc = &libDesc;
		subObjects.emplace_back(std::move(dxilSubObject));

		//// HIT GROUP SUBOBJECT
		std::vector<D3D12_HIT_GROUP_DESC> vHitGroupDescs;	// They need to be alive on pipeline creation!
		vHitGroupDescs.reserve(m_Desc.RaytracingShaders.HitGroups.size());
		for (const RaytracingHitGroup& group : m_Desc.RaytracingShaders.HitGroups)
		{
			// Describe the hit group (intersection shader, any hit shader, closest hit shader)
			// Note that intersection and any hit shaders are assigned to default (triangle) shaders when undefined (nullptr)
			D3D12_HIT_GROUP_DESC hitGroupDesc = {};
			hitGroupDesc.AnyHitShaderImport = group.AnyHitShaderEntryPoint;
			hitGroupDesc.ClosestHitShaderImport = group.ClosestHitShaderEntryPoint;
			hitGroupDesc.IntersectionShaderImport = group.IntersectionShaderEntryPoint;
			hitGroupDesc.HitGroupExport = group.HitGroupName;
			hitGroupDesc.Type = RaytracingHitGroupTypeToD3D(group.Type);
			vHitGroupDescs.emplace_back(std::move(hitGroupDesc));

			D3D12_STATE_SUBOBJECT hitGroupSubObject = {};
			hitGroupSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			hitGroupSubObject.pDesc = &vHitGroupDescs.back();
			subObjects.emplace_back(std::move(hitGroupSubObject));
		}

		//// RAYTRACING SHADER CONFIG SUBOBJECT
		D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
		shaderConfig.MaxPayloadSizeInBytes = m_Desc.MaxPayloadSize;
		shaderConfig.MaxAttributeSizeInBytes = m_Desc.MaxAttributeSize;

		D3D12_STATE_SUBOBJECT shaderConfigSubObject = {};
		shaderConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		shaderConfigSubObject.pDesc = &shaderConfig;
		subObjects.emplace_back(std::move(shaderConfigSubObject));

		//// LOCAL ROOT SIGNATURE SUBOBJECTS
		if (m_LocalRaygenRootSignature.Get())
		{
			D3D12_STATE_SUBOBJECT rayGenLocalRootSignatureSubObject = {};
			rayGenLocalRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			rayGenLocalRootSignatureSubObject.pDesc = m_LocalRaygenRootSignature.GetAddressOf();
			subObjects.emplace_back(std::move(rayGenLocalRootSignatureSubObject));

			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSignatureRayGenAssiciation = {};
			localRootSignatureRayGenAssiciation.pSubobjectToAssociate = &subObjects.back();
			localRootSignatureRayGenAssiciation.NumExports = 1;
			localRootSignatureRayGenAssiciation.pExports = &m_Desc.RaytracingShaders.RayGenShaderEntryPoint;

			D3D12_STATE_SUBOBJECT localRootSignatureRayGenAssiciationSubObject = {};
			localRootSignatureRayGenAssiciationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			localRootSignatureRayGenAssiciationSubObject.pDesc = &localRootSignatureRayGenAssiciation;
			subObjects.emplace_back(std::move(localRootSignatureRayGenAssiciationSubObject));
		}
		existingExports.clear();
		std::vector<const wchar_t*> vHitGroupExports;
		if (m_LocalHitGroupRootSignature.Get())
		{
			D3D12_STATE_SUBOBJECT hitgroupLocalRootSignatureSubObject = {};
			hitgroupLocalRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			hitgroupLocalRootSignatureSubObject.pDesc = m_LocalHitGroupRootSignature.GetAddressOf();
			subObjects.emplace_back(std::move(hitgroupLocalRootSignatureSubObject));

			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSignatureHitgroupAssiciation = {};
			localRootSignatureHitgroupAssiciation.pSubobjectToAssociate = &subObjects.back();
			for (const RaytracingHitGroup& group : m_Desc.RaytracingShaders.HitGroups)
			{
				if (group.AnyHitShaderEntryPoint && !existingExports.contains(group.AnyHitShaderEntryPoint))
				{
					existingExports.insert(group.AnyHitShaderEntryPoint);
					vHitGroupExports.push_back(group.AnyHitShaderEntryPoint);
				}
				if (group.ClosestHitShaderEntryPoint && !existingExports.contains(group.ClosestHitShaderEntryPoint))
				{
					existingExports.insert(group.ClosestHitShaderEntryPoint);
					vHitGroupExports.push_back(group.ClosestHitShaderEntryPoint);
				}
				if (group.IntersectionShaderEntryPoint && !existingExports.contains(group.IntersectionShaderEntryPoint))
				{
					existingExports.insert(group.IntersectionShaderEntryPoint);
					vHitGroupExports.push_back(group.IntersectionShaderEntryPoint);
				}
			}
			localRootSignatureHitgroupAssiciation.NumExports = static_cast<uint32_t>(vHitGroupExports.size());
			localRootSignatureHitgroupAssiciation.pExports = vHitGroupExports.data();

			D3D12_STATE_SUBOBJECT localRootSignatureHitgroupAssiciationSubObject = {};
			localRootSignatureHitgroupAssiciationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			localRootSignatureHitgroupAssiciationSubObject.pDesc = &localRootSignatureHitgroupAssiciation;
			subObjects.emplace_back(std::move(localRootSignatureHitgroupAssiciationSubObject));
		}
		if (m_LocalMissRootSignature.Get())
		{
			D3D12_STATE_SUBOBJECT missLocalRootSignatureSubObject = {};
			missLocalRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			missLocalRootSignatureSubObject.pDesc = m_LocalMissRootSignature.GetAddressOf();
			subObjects.emplace_back(std::move(missLocalRootSignatureSubObject));

			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSignatureMissAssiciation = {};
			localRootSignatureMissAssiciation.pSubobjectToAssociate = &subObjects.back();
			localRootSignatureMissAssiciation.NumExports = static_cast<uint32_t>(m_Desc.RaytracingShaders.MissShaderEntryPoints.size());
			localRootSignatureMissAssiciation.pExports = m_Desc.RaytracingShaders.MissShaderEntryPoints.data();

			D3D12_STATE_SUBOBJECT localRootSignatureMissAssiciationSubObject = {};
			localRootSignatureMissAssiciationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			localRootSignatureMissAssiciationSubObject.pDesc = &localRootSignatureMissAssiciation;
			subObjects.emplace_back(std::move(localRootSignatureMissAssiciationSubObject));
		}
		if (m_LocalCallableRootSignature.Get())
		{
			D3D12_STATE_SUBOBJECT callableLocalRootSignatureSubObject = {};
			callableLocalRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			callableLocalRootSignatureSubObject.pDesc = m_LocalCallableRootSignature.GetAddressOf();
			subObjects.emplace_back(std::move(callableLocalRootSignatureSubObject));

			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION localRootSignatureCallableAssiciation = {};
			localRootSignatureCallableAssiciation.pSubobjectToAssociate = &subObjects.back();
			localRootSignatureCallableAssiciation.NumExports = static_cast<uint32_t>(m_Desc.RaytracingShaders.CallableShaders.size());
			localRootSignatureCallableAssiciation.pExports = m_Desc.RaytracingShaders.CallableShaders.data();

			D3D12_STATE_SUBOBJECT localRootSignatureCallableAssiciationSubObject = {};
			localRootSignatureCallableAssiciationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			localRootSignatureCallableAssiciationSubObject.pDesc = &localRootSignatureCallableAssiciation;
			subObjects.emplace_back(std::move(localRootSignatureCallableAssiciationSubObject));
		}

		//// GLOBAL ROOT SIGNATURE SUBOBJECT
		D3D12_STATE_SUBOBJECT globalRootSignatureSubObject = {};
		globalRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
		globalRootSignatureSubObject.pDesc = m_GlobalRootSignature.GetAddressOf();
		subObjects.emplace_back(std::move(globalRootSignatureSubObject));

		//// RAYTRACING PIPELINE SUBOBJECT
		D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
		pipelineConfig.MaxTraceRecursionDepth = m_Desc.MaxTraceRecursionDepth;

		D3D12_STATE_SUBOBJECT pipelineConfigSubObject = {};
		pipelineConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		pipelineConfigSubObject.pDesc = &pipelineConfig;
		subObjects.emplace_back(std::move(pipelineConfigSubObject));

		// Sanity check
		SKTBD_CORE_ASSERT(subObjects.size() <= maxSubobjects,
			"Subobject vector reallocated memory and is now corrupted. I've not tried this before, but it may crash if it didnt copy on associations. Let's see it for science!!");

		//// PIPELINE STATE OBJECT
		// Create the state object.
		D3D12_STATE_OBJECT_DESC raytracingPipeline = {};
		raytracingPipeline.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
		raytracingPipeline.NumSubobjects = static_cast<uint32_t>(subObjects.size());
		raytracingPipeline.pSubobjects = subObjects.data();
		D3D_CHECK_FAILURE(pDevice->CreateStateObject(&raytracingPipeline, IID_PPV_ARGS(m_StateObject.ReleaseAndGetAddressOf())));
	}

	void D3DRaytracingPipeline::BuildShaderTable()
	{
		ID3D12Device5* pDevice = gD3DContext->RaytracingDevice();

		// Get the identifiers of the shaders
		// A shader identifier is an opaque data blob of 32 bytes that uniquely identifies (within the current device / process)
		// one of the raytracing shaders: ray generation shader, hit group, miss shader, callable shader.
		// The application can request the shader identifier for any of these shaders from the system. It can be thought of as a pointer to a shader (MSDN).
		// The shader table also provides the local root arguments of local root signatures associated with these shaders.
		Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> rtpsoProperties;
		D3D_CHECK_FAILURE(m_StateObject.As(&rtpsoProperties));	// Get the RTPSO properties

		SizedPtr pRayGenShaderIdentifier;
		std::vector<SizedPtr> vHitGroupShaderIdentifiers;
		std::vector<SizedPtr> vMissShaderIdentifiers;
		std::vector<SizedPtr> vCallableShaderIdentifiers;
		std::vector<UINT64> vRootArguments;
		constexpr const size_t maxRootArguments = 255u;
		vRootArguments.reserve(maxRootArguments);
		SizedPtr pRootArguments	= { nullptr, 0 };

		// Retrieve the shader identifiers
		pRayGenShaderIdentifier = { rtpsoProperties->GetShaderIdentifier(m_Desc.RaytracingShaders.RayGenShaderEntryPoint), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES };
		for (const RaytracingHitGroup& hitGroup : m_Desc.RaytracingShaders.HitGroups)
			vHitGroupShaderIdentifiers.push_back({ rtpsoProperties->GetShaderIdentifier(hitGroup.HitGroupName), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES });
		for (const wchar_t*& missShaderEntryPoint : m_Desc.RaytracingShaders.MissShaderEntryPoints)
			vMissShaderIdentifiers.push_back({ rtpsoProperties->GetShaderIdentifier(missShaderEntryPoint), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES });
		for (const wchar_t*& callableShader : m_Desc.RaytracingShaders.CallableShaders)
			vCallableShaderIdentifiers.push_back({ rtpsoProperties->GetShaderIdentifier(callableShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES });

		// When using callable shaders, we need to ensure that the stack size is set to a sensible value.
		// The default stack size defined internally by D3D may not be sufficient depending on the recursions with callable shaders.
		// See: https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#pipeline-stack
		// Here we'll use the same calulcation they presented but simply replace the recursion depth with our internal value
		if (!vCallableShaderIdentifiers.empty() && m_Desc.MaxCallableShaderRecursionDepth > 2u)
		{
			const UINT64 RGSMax = rtpsoProperties->GetShaderStackSize(m_Desc.RaytracingShaders.RayGenShaderEntryPoint);
			UINT64 ISMax = 0u, AHSMax = 0u, CHSMax = 0u, MSMax = 0u, CSMax = 0u;
			for (const RaytracingHitGroup& hitGroup : m_Desc.RaytracingShaders.HitGroups)
			{
				std::wstring identifier(hitGroup.HitGroupName);
				identifier.append(L"::");
				if(hitGroup.IntersectionShaderEntryPoint)	ISMax = std::max(ISMax, rtpsoProperties->GetShaderStackSize((hitGroup.HitGroupName + std::wstring(L"::intersection")).data()));
				if(hitGroup.AnyHitShaderEntryPoint)			AHSMax = std::max(AHSMax, rtpsoProperties->GetShaderStackSize((hitGroup.HitGroupName + std::wstring(L"::anyhit")).data()));
				if(hitGroup.ClosestHitShaderEntryPoint)		CHSMax = std::max(CHSMax, rtpsoProperties->GetShaderStackSize((hitGroup.HitGroupName + std::wstring(L"::closesthit")).data()));
			}
			for (const wchar_t*& missShaderEntryPoint : m_Desc.RaytracingShaders.MissShaderEntryPoints)
				MSMax = std::max(MSMax, rtpsoProperties->GetShaderStackSize(missShaderEntryPoint));
			for (const wchar_t*& callableShader : m_Desc.RaytracingShaders.CallableShaders)
				CSMax = std::max(CSMax, rtpsoProperties->GetShaderStackSize(callableShader));

			const UINT64 stackSize = RGSMax
				+ std::max(CHSMax, std::max(MSMax, ISMax + AHSMax)) * std::min(1u, m_Desc.MaxTraceRecursionDepth)
				+ std::max(CHSMax, MSMax) * std::max(m_Desc.MaxTraceRecursionDepth - 1u, 0u)
				+ m_Desc.MaxCallableShaderRecursionDepth * CSMax;
			rtpsoProperties->SetPipelineStackSize(stackSize);
		}

		// RayGenerationShader record
		if (m_LocalRaygenRootSignature.Get())
		{
			// Constant buffers are directly bound to the root signature
			for (D3DUploadBuffer*& buffer : v_LocalRayGenConstantBuffers)
				vRootArguments.push_back(buffer->GetGPUVirtualAddress());

			// Input (Skateboard) Descriptor tables
			for (D3DDescriptorTable*& table: v_LocalRayGenDescriptorTables)
				vRootArguments.push_back(table->GetGPUHandle().ptr);

			// SRVs and UAVs are part of the Descriptor table
			if (m_LocalRaygenDescriptorTableHandle.IsValid())
				vRootArguments.push_back(m_LocalRaygenDescriptorTableHandle.GetGPUHandle().ptr);

			pRootArguments = { vRootArguments.data(), static_cast<uint32_t>(vRootArguments.size() * sizeof(UINT64)) };
			m_ShaderTable.SetRayGenShader(ShaderRecord(pRayGenShaderIdentifier, pRootArguments));
		}
		else m_ShaderTable.SetRayGenShader(ShaderRecord(pRayGenShaderIdentifier));

		// HitGroup record
		if (m_LocalHitGroupRootSignature.Get())
		{
			const size_t vectorStart = vRootArguments.size();

			// Constant buffers are directly bound to the root signature
			for (D3DUploadBuffer*& buffer : v_LocalHitGroupConstantBuffers)
				vRootArguments.push_back(buffer->GetGPUVirtualAddress());

			// Input (Skateboard) Descriptor tables
			for (D3DDescriptorTable*& table : v_LocalHitGroupDescriptorTables)
				vRootArguments.push_back(table->GetGPUHandle().ptr);

			// SRVs and UAVs are part of the Descriptor table
			if (m_LocalHitGroupDescriptorTableHandle.IsValid())
				vRootArguments.emplace_back(m_LocalHitGroupDescriptorTableHandle.GetGPUHandle().ptr);

			pRootArguments = { vRootArguments.data() + vectorStart, static_cast<uint32_t>(vRootArguments.size() - vectorStart * sizeof(UINT64)) };
			for (const SizedPtr& ptr : vHitGroupShaderIdentifiers)
				m_ShaderTable.AddHitGroupShaders(ShaderRecord(ptr, pRootArguments));
		}
		else for(const SizedPtr& ptr : vHitGroupShaderIdentifiers)
			m_ShaderTable.AddHitGroupShaders(ShaderRecord(ptr));

		// MissShader records
		if (m_LocalMissRootSignature.Get())
		{
			const size_t vectorStart = vRootArguments.size();

			// Constant buffers are directly bound to the root signature
			for (D3DUploadBuffer*& buffer : v_LocalMissConstantBuffers)
				vRootArguments.push_back(buffer->GetGPUVirtualAddress());

			// Input (Skateboard) Descriptor tables
			for (D3DDescriptorTable*& table : v_LocalMissDescriptorTables)
				vRootArguments.push_back(table->GetGPUHandle().ptr);

			// SRVs and UAVs are part of the Descriptor table
			if (m_LocalMissDescriptorTableHandle.IsValid())
				vRootArguments.push_back(m_LocalMissDescriptorTableHandle.GetGPUHandle().ptr);

			pRootArguments = { vRootArguments.data() + vectorStart, static_cast<uint32_t>(vRootArguments.size() - vectorStart * sizeof(UINT64)) };
			for (const SizedPtr& ptr : vMissShaderIdentifiers)
				m_ShaderTable.AddMissShader(ShaderRecord(ptr, pRootArguments));
		}
		else for(const SizedPtr& ptr : vMissShaderIdentifiers)
			m_ShaderTable.AddMissShader(ShaderRecord(ptr));

		if (m_LocalCallableRootSignature.Get())
		{
			const size_t vectorStart = vRootArguments.size();

			// Constant buffers are directly bound to the root signature
			for (D3DUploadBuffer*& buffer : v_LocalCallableConstantBuffers)
				vRootArguments.push_back(buffer->GetGPUVirtualAddress());

			// Input (Skateboard) Descriptor tables
			for (D3DDescriptorTable*& table : v_LocalCallableDescriptorTables)
				vRootArguments.push_back(table->GetGPUHandle().ptr);

			// SRVs and UAVs are part of the Descriptor table
			if (m_LocalCallableDescriptorTableHandle.IsValid())
				vRootArguments.push_back(m_LocalCallableDescriptorTableHandle.GetGPUHandle().ptr);

			pRootArguments = { vRootArguments.data() + vectorStart, static_cast<uint32_t>(vRootArguments.size() - vectorStart * sizeof(UINT64)) };
			for (const SizedPtr& ptr : vCallableShaderIdentifiers)
				m_ShaderTable.AddCallableShader(ShaderRecord(ptr, pRootArguments));
		}
		else for (const SizedPtr& ptr : vCallableShaderIdentifiers)
			m_ShaderTable.AddCallableShader(ShaderRecord(ptr));

		// Finally, store everything in the buffer
		SKTBD_CORE_ASSERT(vRootArguments.size() < maxRootArguments, "Corrupeted vector. TODO: Refactor this in a nicer way.");
		m_ShaderTable.GenerateShaderTableBuffer(pDevice);
	}
}