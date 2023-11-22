#include "sktbdpch.h"
#include "D3DMeshletPipeline.h"

#include "Platform/DirectX12/D3DGraphicsContext.h"
#include "Platform/DirectX12/D3DBuffer.h"
#include "Skateboard/Renderer/Pipeline.h"

#include "d3dx12.h"

namespace Skateboard
{

    D3DMeshletPipeline::D3DMeshletPipeline(const std::wstring& debugName, const MeshletPipelineDesc& desc)
	    :
			MeshletPipeline(debugName, desc)
    {

        if (desc.AmplificationShaderDesc.FileName != nullptr)
        {
            LoadMeshShader(desc.AmplificationShaderDesc.FileName, (void**)m_AmplificationShader.ReleaseAndGetAddressOf());
        }

        LoadMeshShader(desc.MeshShaderDesc.FileName, (void**)m_MeshShader.ReleaseAndGetAddressOf());

        if (desc.PixelShaderDesc.FileName != nullptr)
        {
            LoadMeshShader(desc.PixelShaderDesc.FileName, (void**)m_PixelShader.ReleaseAndGetAddressOf());
        }

        BuildRootSignature();
        BuildMeshletPSO();
    }
    
    

    void D3DMeshletPipeline::LoadMeshShader(const wchar_t* filename, void** blob)
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
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(utils.ReleaseAndGetAddressOf()));
        HRESULT hr = utils->LoadFile(filename, nullptr, reinterpret_cast<IDxcBlobEncoding**>(blob));
        D3D_CHECK_FAILURE(hr);
	}

	void D3DMeshletPipeline::BuildRootSignature()
	{
        // Before we build resources, grab a pointer to our SRV heap.
        auto* pSrvHeap = const_cast<D3DDescriptorHeap*>(gD3DContext->GetSrvHeap());

        auto* pDevice = gD3DContext->MeshShaderDevice();

        // Optionally, if the user has defined a root signature in the shader, attempt
        // to pull the definition.
        if(m_Desc.UseShaderRootSignatureDefinition)
        {
            D3D_FEATURE_LEVEL minFeatureLevel = gD3DContext->GetMinimumFeatureLevel();
            // Check the feature level is not 10 or 10.1 or any level below!
            // Compiling and fetching root signatures is available from feature levels 11.0 and up!
            // (Shader Model 5.0)
            if(!(minFeatureLevel == D3D_FEATURE_LEVEL_10_0 || minFeatureLevel == D3D_FEATURE_LEVEL_10_1))
            {
                // Pull root signature from the precompiled mesh shader.
                D3D_CHECK_FAILURE(pDevice->CreateRootSignature
                (
                    0,
                    m_MeshShader->GetBufferPointer(),
                    m_MeshShader->GetBufferSize(),
                    IID_PPV_ARGS(&m_RootSignature))
                );

                return;
            }
        }


       // Otherwise, build the root signature with the arguments provided in the description.

        std::vector<D3D12_ROOT_PARAMETER1> vRootParams;

        const uint32_t tableSize = (m_Desc.vSRV.size() > 1u) ? 1u : 0u;
        const uint32_t size = tableSize + m_Desc.vCBV.size() + m_Desc.vUAV.size() + m_Desc.v32bitConstants.size();

        vRootParams.resize(size);

        // Bind any constant buffers to the root signature.
        for(const ShaderResourceDesc& desc : m_Desc.vCBV)
        {
            D3D12_ROOT_PARAMETER1 param = {};
            
            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            param.ShaderVisibility = ShaderVisibilityToD3D(desc.ShaderVisibility);

            param.Descriptor.ShaderRegister = desc.ShaderRegister;
            param.Descriptor.RegisterSpace = desc.ShaderRegisterSpace;
            param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
            //vRootParams.emplace_back(std::move(param));
            vRootParams[desc.RootIndex] = std::move(param);
        }

        for(const ShaderResourceDesc& desc : m_Desc.v32bitConstants)
        {
            D3D12_ROOT_PARAMETER1 param = {};

            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            param.ShaderVisibility = ShaderVisibilityToD3D(desc.ShaderVisibility);

            param.Constants.ShaderRegister = desc.ShaderRegister;
            param.Constants.RegisterSpace = desc.ShaderRegisterSpace;
            param.Constants.Num32BitValues = desc.NumOf32BitConstants;

            //vRootParams.emplace_back(std::move(param));
            vRootParams[desc.RootIndex] = std::move(param);
        }

        // Allocate the descriptor table
        D3DDescriptorHandle tempHandle = m_DescriptorTableRootHandle = pSrvHeap->Allocate(static_cast<uint32_t>(m_Desc.vSRV.size() + m_Desc.vUAV.size()));
        const uint32_t handleSize = pSrvHeap->GetHeapSize();

        std::vector<D3D12_DESCRIPTOR_RANGE1> vDescriptorRanges;

        // Bind the shader resources to the root signature.
        for(const ShaderResourceDesc& desc : m_Desc.vSRV)
        {
            D3D12_DESCRIPTOR_RANGE1 descriptorRange                     = {};
            descriptorRange.RangeType                                   = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            descriptorRange.NumDescriptors                              = desc.NumOfShaderResources;
            descriptorRange.BaseShaderRegister                          = desc.ShaderRegister;
            descriptorRange.RegisterSpace                               = desc.ShaderRegisterSpace;
            descriptorRange.OffsetInDescriptorsFromTableStart           = static_cast<uint32_t>(vDescriptorRanges.size());
            descriptorRange.Flags                                       = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            vDescriptorRanges.emplace_back(std::move(descriptorRange));

            if(desc.pResource != nullptr)
            {
                switch (desc.pResource->GetType())
                {
                case GPUResourceType_DefaultBuffer:
                {
                    D3DDefaultBuffer* pBuffer = static_cast<D3DDefaultBuffer*>(desc.pResource);
                    pDevice->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    tempHandle += handleSize;
                    break;
                }
                case GPUResourceType_UploadBuffer:
                {
                    D3DUploadBuffer* pBuffer = static_cast<D3DUploadBuffer*>(desc.pResource);
                    pDevice->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    tempHandle += handleSize;
                    break;
                }
                case GPUResourceType_Texture2D:
                {
                    D3DTexture* pBuffer = static_cast<D3DTexture*>(desc.pResource);
                    pDevice->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    tempHandle += handleSize;
                    break;
                }
                case GPUResourceType_UnorderedAccessBuffer:
                {
                    D3DUnorderedAccessBuffer* pBuffer = static_cast<D3DUnorderedAccessBuffer*>(desc.pResource);
                    pDevice->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    tempHandle += handleSize;
                    break;
                }
                case GPUResourceType_FrameBuffer:
                {
                    // TODO: GetSRVHandle(0) -> Do we need to index anything else really?
                    D3DFrameBuffer* pBuffer = static_cast<D3DFrameBuffer*>(desc.pResource);
                    pDevice->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle(0).GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    tempHandle += handleSize;
                    break;
                }
                case GPUResourceType_ByteAddress:
                {
                    D3DByteAddressBuffer* pBuffer = static_cast<D3DByteAddressBuffer*>(desc.pResource);
                    pDevice->CopyDescriptorsSimple(1, tempHandle.GetCPUHandle(), pBuffer->GetSRVHandle().GetCPUHandle(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    tempHandle += handleSize;
                    break;
                }
                default:
                    SKTBD_CORE_ASSERT(false, "unsupported");
                    break;
                }
            }
        }

        // Add a descriptor table with all ranges
        if (!vDescriptorRanges.empty())
        {
            D3D12_ROOT_PARAMETER1 descriptorTable = {};
            descriptorTable.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            descriptorTable.ShaderVisibility                    = D3D12_SHADER_VISIBILITY_ALL;	// TODO: Maybe not all the time??
            descriptorTable.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(vDescriptorRanges.size());
            descriptorTable.DescriptorTable.pDescriptorRanges   = vDescriptorRanges.data();

            //After placing the shader table, record it's root index for binding later.
            m_Desc.ShaderResourceIndex = vRootParams.size() - 1;

            vRootParams[m_Desc.ShaderResourceIndex] = std::move(descriptorTable);
        }

       
        // Create a default static sampler (TODO: have them added via DESCs)
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
        rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

        // Serialize a root signature into a blob that can be passed into the CreateRootSignature function
        // The version of this root signature will be 1.1
        Microsoft::WRL::ComPtr<ID3DBlob> signature;
        Microsoft::WRL::ComPtr<ID3DBlob> error;
        if (FAILED(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, signature.GetAddressOf(), error.GetAddressOf())))
        {
            SKTBD_CORE_CRITICAL((char*)error->GetBufferPointer());
            throw std::exception();
        }

        D3D_CHECK_FAILURE(pDevice->CreateRootSignature(
            0,															// For single GPU, set this to 0
            signature->GetBufferPointer(),								// Pointer to the serialized signature
            signature->GetBufferSize(),									// Length, in bytes, of the signature to be read
            IID_PPV_ARGS(m_RootSignature.ReleaseAndGetAddressOf()))
        );

	}

	void D3DMeshletPipeline::BuildMeshletPSO()
	{

        D3D12_RASTERIZER_DESC rasterizerDesc{};
        rasterizerDesc.FillMode = m_Desc.IsWireFrame ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FrontCounterClockwise = TRUE;
        rasterizerDesc.DepthBias = m_Desc.DepthBias;
        rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterizerDesc.SlopeScaledDepthBias = m_Desc.DepthBias ? 1.f : D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterizerDesc.DepthClipEnable = TRUE;
        rasterizerDesc.MultisampleEnable = FALSE;
        rasterizerDesc.AntialiasedLineEnable = FALSE;
        rasterizerDesc.ForcedSampleCount = 0;
        rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
   
        D3D12_BLEND_DESC blendDesc{};
        blendDesc.AlphaToCoverageEnable     = FALSE;
        blendDesc.IndependentBlendEnable    = FALSE;
        constexpr D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
        {
            FALSE,FALSE,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        };

        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        {
            blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
        }

        D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        depthStencilDesc.StencilEnable = FALSE;
        depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        constexpr D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
        {
        	D3D12_STENCIL_OP_KEEP,
        	D3D12_STENCIL_OP_KEEP,
        	D3D12_STENCIL_OP_KEEP,
        	D3D12_COMPARISON_FUNC_ALWAYS
        };
        depthStencilDesc.FrontFace = defaultStencilOp;
        depthStencilDesc.BackFace = defaultStencilOp;

        auto backBufferFormat       = gD3DContext->GetBackBufferFormat();
        auto depthStencilFormat     = gD3DContext->GetDepthStencilFormat();

        DXGI_SAMPLE_DESC sampleDesc = {};
        sampleDesc.Count = 1;
        sampleDesc.Quality = 0;

        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc     = {};
        psoDesc.pRootSignature                             = m_RootSignature.Get();
        psoDesc.MS                                         = { m_MeshShader->GetBufferPointer(), m_MeshShader->GetBufferSize() };
        if (m_PixelShader.Get() != nullptr)
        {
            psoDesc.PS = { m_PixelShader->GetBufferPointer(), m_PixelShader->GetBufferSize() };
        }
        if (m_AmplificationShader.Get() != nullptr)
        {
            psoDesc.AS = { m_AmplificationShader->GetBufferPointer(), m_AmplificationShader->GetBufferSize() };
        }
        psoDesc.NumRenderTargets                            = 1;
        psoDesc.RTVFormats[0]                               = backBufferFormat;
        psoDesc.DSVFormat                                   = depthStencilFormat;
        psoDesc.RasterizerState                             = rasterizerDesc;     // CW front; cull back
        psoDesc.BlendState                                  = blendDesc;         // Opaque
        psoDesc.DepthStencilState                           = depthStencilDesc; // Less-equal depth test w/ writes; no stencil
        psoDesc.SampleMask                                  = UINT_MAX;
        psoDesc.SampleDesc                                  = sampleDesc;
        psoDesc.PrimitiveTopologyType                       = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        psoDesc.Flags                                       = D3D12_PIPELINE_STATE_FLAG_NONE;
        
        auto psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc;
        streamDesc.pPipelineStateSubobjectStream    = &psoStream;
        streamDesc.SizeInBytes                      = sizeof(psoStream);

        auto* pDevice = gD3DContext->MeshShaderDevice();
        const HRESULT hr = pDevice->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_PipelineStateObject));
        D3D_CHECK_FAILURE(hr);
        m_PipelineStateObject->SetName(L"Mesh Shader Pipeline");
	}

	void __inline D3DMeshletPipeline::SetDefaultInputLayout()
	{
        m_Desc.Layout = BufferLayout::GetDefaultLayout();

        D3D12_INPUT_ELEMENT_DESC c_elementDescs[5];
        // Mesh shader file expects a certain vertex layout; assert our mesh conforms to that layout.
        c_elementDescs[0] = { "POSITION",       0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 1 };
        c_elementDescs[1] = { "TEXCOORD",       0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 1 };
        c_elementDescs[2] = { "NORMAL",         0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 1 };
        c_elementDescs[3] = { "TANGENT",        0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 1 };
        c_elementDescs[4] = { "BITANGENT",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 1 };
	}

    void D3DMeshletPipeline::BindMeshPipeline(MeshletModel* model)
    {
        auto* pCommandList = gD3DContext->MeshShaderCommandList();
        pCommandList->SetPipelineState(m_PipelineStateObject.Get());
        pCommandList->SetGraphicsRootSignature(m_RootSignature.Get());

        if(!m_Desc.UseModelAttribsAsShaderDispatchDesc)
        {
            // If no mesh is bound, simply bind the resources once.
            BindAllResources(pCommandList);
            pCommandList->DispatchMesh(m_Desc.DispatchSize.ThreadCountX, 
                m_Desc.DispatchSize.ThreadCountY, m_Desc.DispatchSize.ThreadCountZ);
        }
        else if(model != nullptr)
        {
            auto* pModel = dynamic_cast<D3DModel*>(model);

            // Bind the resources for each mesh chunk within the model.
            for (auto mesh : *pModel)
            {
                BindAllResources(pCommandList);
                pCommandList->SetGraphicsRoot32BitConstant(1, mesh.IndexSize, 0);
                
                /*auto meshletCount = static_cast<uint32_t>(pModel->GetRawMeshletData().size());
                auto dispatchX = (meshletCount + 31) / 32;
                pCommandList->DispatchMesh(meshletCount, 1, 1);*/

                for (auto subset : mesh.MeshletSubsets)
                {
                    pCommandList->SetGraphicsRoot32BitConstant(1, 0, 1);
                    pCommandList->DispatchMesh(subset.Count, 1, 1);
                }
	        }
        }
    }

    void __inline D3DMeshletPipeline::BindAllResources(ID3D12GraphicsCommandList6* pCommandList)
    {
        for (const ShaderResourceDesc& desc : m_Desc.vCBV)
        {
            const D3DUploadBuffer* pUpload = static_cast<D3DUploadBuffer*>(desc.pResource);
            pCommandList->SetGraphicsRootConstantBufferView(desc.RootIndex, pUpload->GetGPUVirtualAddress());
        }

        for (const ShaderResourceDesc& desc : m_Desc.v32bitConstants)
        {
            D3DUploadBuffer* pUpload = static_cast<D3DUploadBuffer*>(desc.pResource);
            pCommandList->SetGraphicsRoot32BitConstant(desc.RootIndex, desc.Constant32BitSrc, desc.Constant32BitOffset);
        }

        if(!m_Desc.UseShaderRootSignatureDefinition)
        {
            if (m_Desc.vSRV.size() || m_Desc.vUAV.size())
            {
                pCommandList->SetGraphicsRootDescriptorTable(m_Desc.ShaderResourceIndex, m_DescriptorTableRootHandle.GetGPUHandle());
            }
        }
        else
        {
            for(const auto& srv : m_Desc.vSRV)
            {
                auto* d3dBuffer = static_cast<D3DDefaultBuffer*>(srv.pResource);
                pCommandList->SetGraphicsRootShaderResourceView(srv.RootIndex, d3dBuffer->GetGPUVirtualAddress());
            }
        }
    }

    void __inline D3DMeshletPipeline::SetInputLayout(const BufferLayout& layout)
    {
        m_Desc.Layout = layout;
    }

    void D3DMeshletPipeline::SetResource(GPUResource* buffer, uint32_t shaderRegister, uint32_t registerSpace)
    {
        // TODO: this can be updated soon to utilise the table allocator system.

        const auto* pBuffer = static_cast<D3DUploadBuffer*>(buffer);
        auto* pDevice = gD3DContext->Device();

        const uint32_t size = gD3DContext->GetSrvHeap()->GetHeapSize();

        D3DDescriptorHandle tempHandle = m_DescriptorTableRootHandle;

        tempHandle += size * shaderRegister;

        pDevice->CopyDescriptorsSimple
    	(
            1, 
            tempHandle.GetCPUHandle(),
            pBuffer->GetSRVHandle().GetCPUHandle(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );

    }

    void D3DMeshletPipeline::Unbind()
    {
    }

    void D3DMeshletPipeline::Release()
    {
    }
}
